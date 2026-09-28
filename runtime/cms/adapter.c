/* CMS file service logic adapted from the public-domain CMS-370-GCCLIB
   cmssys.assemble. C implementation and ELF binding for this proof.
   Internal strings are ASCII. Names and console use an explicitly bounded
   character set. File record contents are opaque bytes, without conversion. */
#include "adapter.h"
#ifdef LAB_CMS_APPLICATION
#include "characters.h"
int lab_ebcdic_to_ascii(unsigned c)
{
    for (unsigned i = 0; i < sizeof lab_ascii_1047; ++i)
        if (lab_ascii_1047[i] == c) return (int)i + 32;
    return -1;
}
#endif

static void copy(void *d, const void *s, unsigned n)
{
    unsigned char *to = d;
    const unsigned char *from = s;
    while (n--) *to++ = *from++;
}
static void zero(void *p, unsigned n)
{
    unsigned char *s = p;
    while (n--) *s++ = 0;
}
int lab_ascii_to_ebcdic(unsigned c)
{
#ifdef LAB_CMS_APPLICATION
    return c >= 32 && c <= 126 ? lab_ascii_1047[c - 32] : -1;
#else
    if (c >= 'A' && c <= 'I') return c - 'A' + 0xc1;
    if (c >= 'J' && c <= 'R') return c - 'J' + 0xd1;
    if (c >= 'S' && c <= 'Z') return c - 'S' + 0xe2;
    if (c >= 'a' && c <= 'i') return c - 'a' + 0x81;
    if (c >= 'j' && c <= 'r') return c - 'j' + 0x91;
    if (c >= 's' && c <= 'z') return c - 's' + 0xa2;
    if (c >= '0' && c <= '9') return c - '0' + 0xf0;
    if (c == ' ') return 0x40;
    if (c == '+') return 0x4e;
    if (c == '-') return 0x60;
    if (c == '.') return 0x4b;
    return -1;
#endif
}
static void command(char *p, const char *name)
{
    unsigned i = 0;
    while (name[i]) { p[i] = (char)lab_ascii_to_ebcdic(name[i]); ++i; }
    while (i < 8) p[i++] = 0x40;
}
int lab_cms_name(const char *name, unsigned char id[18])
{
    unsigned widths[3] = {8, 8, 2}, offset = 0;
    if (!name) return -1;
    for (unsigned field = 0; field < 3; ++field) {
        unsigned n = 0;
        if (!*name || *name == ' ') return -1;
        while (*name && *name != ' ') {
            unsigned c = (unsigned char)*name++;
            if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
            if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) ||
                n >= widths[field]) return -1;
            id[offset + n++] = (unsigned char)lab_ascii_to_ebcdic(c);
        }
        while (n < widths[field]) id[offset + n++] = 0x40;
        offset += widths[field];
        if (field < 2) { if (*name++ != ' ') return -1; }
    }
    return *name ? -1 : 0;
}
static int open_records(LabCmsFile *f, const unsigned char id[18], void *buf, int cap, int fixed)
{
    unsigned long regs[2];
    zero(f, sizeof *f);
    copy(f->filename, id, 18);
    command(f->command, "STATE");
    int rc = lab_cms_service(f, regs);
    if (rc == 0) {
        /* STATE returns FST in FSCB+28. The guest FSOPEN macro copies
           filemode, LRECL and RECFM from FST+24,+32,+30. */
        unsigned char *fst = f->buffer;
        copy(f->filemode, fst + 24, 2);
        f->format[0] = fst[30];
        if (f->format[0] != 0xe5 && (!fixed || f->format[0] != 0xc6)) return 20;
        int lrecl;
        copy(&lrecl, fst + 32, 4);
        if (lrecl > cap) return 24;
    } else if (rc == 28) {
        f->format[0] = 0xe5;
    } else return rc;
    f->format[1] = 0x40;
    f->buffer = buf;
    f->capacity = cap;
    f->record = 1;
    f->count = 1;
    f->bytes = 0;
    return rc; /* 28 means new file: first WRBUF creates it. */
}
int lab_cms_open(LabCmsFile *f,const unsigned char id[18],void *buf,int cap)
{ return open_records(f,id,buf,cap,0); }
int lab_cms_open_text(LabCmsFile *f,const unsigned char id[18],void *buf,int cap)
{ return open_records(f,id,buf,cap,1); }
int lab_cms_read(LabCmsFile *f, int *length)
{
    unsigned long regs[2];
    command(f->command, "RDBUF");
    int rc = lab_cms_service(f, regs);
    *length = rc ? 0 : f->bytes;
    f->record = 0;
    return rc;
}
int lab_cms_write(LabCmsFile *f, int length)
{
    unsigned long regs[2];
    if (length < 1 || length > f->capacity) return 24;
    int capacity = f->capacity;
    f->capacity = length;
    command(f->command, "WRBUF");
    int rc = lab_cms_service(f, regs);
    f->capacity = capacity;
    f->record = 0;
    return rc;
}
int lab_cms_close(LabCmsFile *f)
{
    unsigned long regs[2];
    command(f->command, "FINIS");
    return lab_cms_service(f, regs);
}
int lab_cms_erase(const unsigned char id[18])
{
    LabCmsFile f;
    unsigned long regs[2];
    zero(&f, sizeof f);
    copy(f.filename, id, 18);
    command(f.command, "ERASE");
    return lab_cms_service(&f, regs);
}
int lab_cms_state(const unsigned char id[18], unsigned char fst[40])
{
    LabCmsFile f;
    unsigned long regs[2];
    zero(&f, sizeof f);
    copy(f.filename, id, 18);
    command(f.command, "STATE");
    int rc = lab_cms_service(&f, regs);
    if (!rc) copy(fst, f.buffer, 40); /* STATE copy is transient: own it now. */
    return rc;
}
int lab_cms_rename(const unsigned char oldid[18], const unsigned char newid[18])
{
    /* DMSRNM: six eight-byte tokens, then the FF fence. No wildcard/options. */
    unsigned char plist[64];
    unsigned long regs[2];
    for (unsigned i = 0; i < 56; ++i) plist[i] = 0x40;
    for (unsigned i = 56; i < sizeof plist; ++i) plist[i] = 255;
    command((char *)plist, "RENAME");
    copy(plist + 8, oldid, 16); copy(plist + 24, oldid + 16, 2);
    copy(plist + 32, newid, 16); copy(plist + 48, newid + 16, 2);
#if defined(__MAINFRAME_LAB_CMS20_ESA31__)
    /* RENAME is an AMODE 24 CMS command in this CMS 20 guest. CMSCALL must
       copy its token list below 16 MiB and respect the FF fence. */
    return lab_cms_svc204_copy(plist, regs);
#else
    return lab_cms_service(plist, regs);
#endif
}
int lab_cms_line(const char *text, unsigned length)
{
    unsigned char bytes[130];
#if defined(__MAINFRAME_LAB_CMS20_ESA31__)
    struct {
        char command[8]; unsigned char *text; unsigned length;
        unsigned char reserved[28], fence[8];
    } plist;
#else
    struct { char command[8]; unsigned char *text; unsigned flags; } plist;
#endif
    unsigned long regs[2];
    if (!text || length > sizeof bytes) return -1;
    for (unsigned i = 0; i < length; ++i) {
        int c = lab_ascii_to_ebcdic((unsigned char)text[i]);
        if (c < 0) return -1;
        bytes[i] = (unsigned char)c;
    }
#if defined(__MAINFRAME_LAB_CMS20_ESA31__)
    zero(&plist,sizeof plist);
    command(plist.command,"LINEWRT");
    plist.text=bytes;
    plist.length=length;
    for (unsigned i=0;i<8;++i) plist.fence[i]=255;
#else
    command(plist.command, "TYPLIN");
    plist.text = (unsigned char *)((unsigned long)bytes | 0x01000000UL);
    plist.flags = 0xc2800000UL | length;
#endif
    return lab_cms_service(&plist, regs);
}
int lab_cms_input(unsigned char *bytes, unsigned *length)
{
    /* The historical WAITRD field is 24-bit only. CMS 20 uses the preferred
       LINERD plist, which accepts a full 31-bit buffer under CMSCALL. */
#if defined(__MAINFRAME_LAB_CMS20_ESA31__)
    struct {
        unsigned char command[8];
        unsigned char *data;
        unsigned capacity;
        unsigned name, line, column, prompt, prompt_length;
        unsigned char flags1, flags2, reserved[2];
        unsigned count, next_length;
        unsigned char fence[8];
    } plist;
    unsigned long regs[2];
    if (!bytes || !length || !*length || *length>130) return -1;
    zero(&plist,sizeof plist);
    command((char *)plist.command,"LINERD");
    plist.data=bytes;
    plist.capacity=*length;
    /* PAD=BLANK, TYPE=STACK, LOGICAL=YES, TRANS=YES, CASE=MIXED;
       ATTREST=YES and FORM=SINGLE. PAD=NONE is invalid in line mode. */
    plist.flags1=0xce;
    plist.flags2=0x40;
    for (unsigned i=0;i<8;++i) plist.fence[i]=255;
    int rc=lab_cms_service(&plist,regs);
    if (!rc) *length=(unsigned)regs[0];
    return rc;
#else
    /* RDTERM EDIT=NO: WAITRD, console 1, translation T, returned count.
       The 24-bit field carries the CMS console selector in its high byte. */
    struct {
        unsigned char command[8];
        unsigned char address[4];
        unsigned char edit, direct, count[2];
    } plist;
    unsigned long regs[2];
    unsigned long pointer=(unsigned long)bytes;
    if (!bytes || !length || !*length || *length>130) return -1;
    if (pointer>=0x01000000UL) return -1;
    zero(&plist,sizeof plist);
    command((char *)plist.command,"WAITRD");
    plist.address[0]=1;
    plist.address[1]=(unsigned char)(pointer>>16);
    plist.address[2]=(unsigned char)(pointer>>8);
    plist.address[3]=(unsigned char)pointer;
    plist.edit=(unsigned char)lab_ascii_to_ebcdic('T');
    plist.count[0]=(unsigned char)(*length>>8);
    plist.count[1]=(unsigned char)*length;
    int rc=lab_cms_service(&plist,regs);
    if (!rc) *length=(unsigned)plist.count[0]*256+plist.count[1];
    return rc;
#endif
}
int lab_cms_failure(unsigned where)
{
    char message[] = "LIBC FAIL 0000";
    const char digits[] = "0123456789ABCDEF";
    for (unsigned i = 0; i < 4; ++i)
        message[10 + i] = digits[(where >> ((3 - i) << 2)) & 15];
    lab_cms_line(message, sizeof message - 1);
    return (int)where;
}

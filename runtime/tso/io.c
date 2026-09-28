/* Newlib byte streams over pinned PDPCLIB MVS record services.
   Existing sequential data sets and PDS members; no implicit allocation of
   disk space, append/update, concatenations or POSIX processes. */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <reent.h>
#include "services.h"
#include "text-codec.h"
#include "sdk-path.h"
#include "native-args.h"
#define SLOTS 8
#ifndef LAB_TSO_HEAP_SIZE
#define LAB_TSO_HEAP_SIZE (32U*1024U*1024U)
#endif
#if LAB_TSO_HEAP_SIZE < 1024 || LAB_TSO_HEAP_SIZE > 536870912 || (LAB_TSO_HEAP_SIZE & 7)
#error TSO heap must be doubleword aligned and between 1 KiB and 512 MiB
#endif
#define HEAP_SIZE LAB_TSO_HEAP_SIZE
#ifndef LAB_TSO_INPUT_CHUNK
#define LAB_TSO_INPUT_CHUNK 256
#endif
#if LAB_TSO_INPUT_CHUNK < 1 || LAB_TSO_INPUT_CHUNK > 256
#error TSO terminal chunk must fit the 256-byte native buffer
#endif
#undef errno
int errno;
static int error(int e) { errno=e; *__errno()=e; return -1; }
typedef struct {
    void *handle;
    unsigned char dd[8], member[8], *buffer;
    unsigned capacity, used, at, available, position;
    int active, dynamic, writing, text, recfm, lrecl, blksize, eof, failed;
    LabUtf8 decoder;
} Slot;
static Slot slots[SLOTS];
static unsigned char *heap;
static unsigned heap_used;
static unsigned heap_failed_request;
static void report_heap(void);
static int last_open_status;
int lab_tso_last_open_status(void) { return last_open_status; }
static unsigned char line[132];
static unsigned line_used;
static LabUtf8 console_decoder;
static unsigned char terminal_input[513];
static unsigned terminal_at,terminal_used;
static int putline(void);
#ifdef LAB_TSO24
/* Reject a buffer that AMODE24 would wrap before any byte is accessed. */
static int low_buffer(const void *p,size_t n)
{
    uintptr_t a=(uintptr_t)p;
    return n==0 || (a!=0 && a<0x1000000U && n<=0x1000000U-a);
}
#endif
unsigned lab_tso_heap_used(void) { return heap_used; }
void *_sbrk(ptrdiff_t amount)
{
    void *p;
    if (amount<0 || (size_t)amount>HEAP_SIZE-heap_used) {
        heap_failed_request=(unsigned)amount;
        report_heap();
        error(ENOMEM); return (void *)-1;
    }
    if (!heap) heap=lab_tso_services->allocate(HEAP_SIZE);
    if (!heap) {
        heap_failed_request=(unsigned)amount;report_heap();
        error(ENOMEM); return (void *)-1;
    }
    p=heap+heap_used; heap_used+=(unsigned)amount;
    return p;
}
static Slot *get(int fd)
{
    if (fd<3 || fd>=SLOTS+3 || !slots[fd-3].active) {
        error(EBADF); return 0;
    }
    return &slots[fd-3];
}
static int name_byte(unsigned c,int first)
{
    if (c>='a' && c<='z') c-=32;
    if ((c>='A' && c<='Z') || c=='$' || c=='#' || c=='@' ||
        (!first && ((c>='0' && c<='9') || c=='-')))
        return lab_unicode_1047(c);
    return -1;
}
/* DD:NAME[(MEMBER)] uses the caller's allocation. Otherwise a fully
   qualified DSN[(MEMBER)] is allocated with a system-assigned DD name.
   No TSO prefix is guessed, and names are never silently truncated. */
static int path(Slot *s,const char *p)
{
    char normalized[64];
    unsigned char dsn[44];
    unsigned n=0, qualifier=0, m=0;
    int dd=0, quoted=0;
    p=lab_tso_path(p,normalized,sizeof normalized);
    if (!p || !*p) return error(EINVAL);
    if (!strncmp(p,"DSN:",4) || !strncmp(p,"dsn:",4)) p+=4;
    if ((!strncmp(p,"DD:",3)) || (!strncmp(p,"dd:",3))) { dd=1;p+=3; }
    if (*p=='\'') { quoted=1;++p; }
    memset(s->dd,0x40,8); memset(s->member,0x40,8);
    while (*p && *p!='(' && *p!='\'') {
        int c;
        if (*p=='.' && !dd) {
            if (!qualifier || n==44) return error(EINVAL);
            dsn[n++]=0x4b;qualifier=0;++p;continue;
        }
        c=name_byte((unsigned char)*p++,qualifier==0);
        if (c<0 || qualifier==8 || n==(dd?8U:44U)) return error(EINVAL);
        dsn[n++]=(unsigned char)c;++qualifier;
    }
    if (!n || !qualifier) return error(EINVAL);
    if (*p=='(') {
        ++p;
        while (*p && *p!=')') {
            int c=name_byte((unsigned char)*p++,m==0);
            if (c<0 || m==8) return error(EINVAL);
            s->member[m++]=(unsigned char)c;
        }
        if (!m || *p++!=')') return error(EINVAL);
    }
    if (quoted && *p=='\'') ++p;
    else if (quoted) return error(EINVAL);
    if (*p) return error(EINVAL);
    if (dd) memcpy(s->dd,dsn,n);
    else {
        DynArgs args={8,s->dd,n,dsn};
        if (lab_tso_filecall(4,&args)) return error(ENOENT);
        s->dynamic=1;
    }
    return 0;
}
static int native_open(Slot *s)
{
    int mode=s->writing;
    void *buffer=0;
    OpenArgs args={s->dd,&mode,&s->recfm,&s->lrecl,&s->blksize,
                   &buffer,s->member};
    int h=lab_tso_filecall(0,&args);
    last_open_status=h>0?0:h;
    if (h<=0) return error(h==-2008 || h==-2068 ? ENOENT : EIO);
    s->handle=(void *)(unsigned long)(unsigned)h;
    if ((s->recfm!=0 && s->recfm!=1) || s->lrecl<1 || s->lrecl>32760 ||
        (s->recfm==1 && s->lrecl<5) ||
        (s->writing && !s->text && s->recfm!=1)) return error(ENOTSUP);
    s->capacity=(unsigned)s->lrecl-(s->recfm==1?4U:0U);
    return 0;
}
static int close_native(Slot *s)
{
    int rc=0;
    if (s->handle) {
        if (lab_tso_filecall(3,&s->handle)) rc=error(EIO);
        s->handle=0;
    }
    return rc;
}
static int open_stream(const char *p,int writing,int text)
{
    unsigned i;
    Slot *s;
    for (i=0;i<SLOTS && slots[i].active;++i) {}
    if (i==SLOTS) return error(EMFILE);
    s=&slots[i];memset(s,0,sizeof *s);
    s->writing=writing;s->text=text;s->recfm=1;
    s->lrecl=32756;s->blksize=32760;
    if (path(s,p)) return -1;
    for (unsigned j=0;j<SLOTS;++j)
        if (slots[j].active && !memcmp(s->dd,slots[j].dd,8)) {
            error(EBUSY);goto fail;
        }
    if (native_open(s)) goto fail;
    s->buffer=malloc(s->writing?s->capacity+4:2*s->capacity+1);
    if (!s->buffer) { error(ENOMEM);goto fail; }
    s->active=1;
    return (int)i+3;
fail:
    { int e=errno;
      close_native(s);
      if (s->dynamic) lab_tso_filecall(5,s->dd);
      memset(s,0,sizeof *s);return error(e); }
}
int _open(const char *p,int flags,...)
{
    if (flags==O_RDONLY) return open_stream(p,0,0);
    if (flags==(O_WRONLY|O_CREAT|O_TRUNC)) return open_stream(p,1,0);
    return error(ENOTSUP);
}
static int emit(Slot *s)
{
    unsigned n=s->used, length;
    unsigned char *record=s->buffer;
    IoArgs args={s->handle,&record,&length};
    if (s->recfm==1) {
        length=n+4;
        record[0]=(unsigned char)(length>>8);record[1]=(unsigned char)length;
        record[2]=record[3]=0;
    } else {
        /* Fixed text records are padded explicitly with EBCDIC blanks. */
        record+=4;length=s->capacity;
        memset(record+n,0x40,s->capacity-n);
    }
    if (lab_tso_filecall(2,&args)) {
        s->failed=EIO;return error(EIO);
    }
    s->used=0;return 0;
}
static int record(Slot *s)
{
    unsigned char *raw=0;
    unsigned n=0, start=0;
    IoArgs args={s->handle,&raw,&n};
    int rc=lab_tso_filecall(1,&args);
    if (rc==-1) { s->eof=1;return 0; }
    if (rc || !raw || n>(unsigned)s->lrecl) return error(EIO);
    if (s->recfm==1) {
        if (n<4 || n!=((unsigned)raw[0]<<8)+raw[1] || raw[2] || raw[3])
            return error(EIO);
        start=4;
    }
    s->at=s->available=0;
    if (s->text) {
        /* Preserve fixed-record padding; lexical readers may ignore it. */
        for (unsigned i=start;i<n;++i) {
            unsigned char bytes[2];
            unsigned count=lab_1047_utf8(raw[i],bytes);
            if (!bytes[0]) return error(EILSEQ);
            for (unsigned j=0;j<count;++j) s->buffer[s->available++]=bytes[j];
        }
        s->buffer[s->available++]='\n';
    } else {
        s->available=n-start;memcpy(s->buffer,raw+start,s->available);
    }
    return 0;
}
static _ssize_t read_terminal(void *out,size_t length)
{
    unsigned n;
    if (!length) return 0;
    if (length>INT_MAX) return error(EINVAL);
    if (terminal_at==terminal_used) {
        unsigned char native[LAB_TSO_INPUT_CHUNK];
        unsigned count=0;
        int rc;
        if (lab_tso_services->version<4 || !lab_tso_services->readline)
            return error(ENOTSUP);
        /* Flush both FILE buffering and the native line buffer before TGET.
           A terminal read must not wait with the program's prompt hidden. */
        if (fflush(stdout) || fflush(stderr) || (line_used && putline())) return -1;
        rc=lab_tso_services->readline(native,sizeof native,&count);
        if (rc==8) return error(EINTR);
        if ((rc!=0 && rc!=12 && rc!=24 && rc!=28) || count>sizeof native ||
            (!count && (rc==12 || rc==28))) return error(EIO);
        terminal_at=terminal_used=0;
        for (unsigned i=0;i<count;++i) {
            unsigned char bytes[2];unsigned size=lab_1047_utf8(native[i],bytes);
            if (!bytes[0]) return error(EILSEQ);
            for (unsigned j=0;j<size;++j) terminal_input[terminal_used++]=bytes[j];
        }
        /* A short native buffer is a continuation, not a line ending. */
        if (rc==0 || rc==24) terminal_input[terminal_used++]='\n';
    }
    n=terminal_used-terminal_at;if (n>length) n=(unsigned)length;
    memcpy(out,terminal_input+terminal_at,n);terminal_at+=n;
    return (_ssize_t)n;
}
_ssize_t _read(int fd,void *out,size_t length)
{
#ifdef LAB_TSO24
    if (!low_buffer(out,length)) return error(EFAULT);
#endif
    if (fd==0) return read_terminal(out,length);
    Slot *s=get(fd);unsigned done=0;
    if (!s) return -1;
    if (s->writing) return error(EBADF);
    if (length>INT_MAX) return error(EINVAL);
    if (s->failed) return error(s->failed);
    while (done<length) {
        unsigned n;
        if (s->at==s->available) {
            if (s->eof) break;
            if (record(s)) { s->failed=errno;return done?(_ssize_t)done:-1; }
            if (s->eof) break;
        }
        n=s->available-s->at;if (n>length-done) n=(unsigned)length-done;
        memcpy((unsigned char *)out+done,s->buffer+s->at,n);
        s->at+=n;done+=n;s->position+=n;
    }
    return (_ssize_t)done;
}
static int putline(void)
{
#ifdef LAB_TSO_BATCH_OUTPUT_DD
    /* TSO CALL has an argument list, not the CPPL required by PUTLINE.
       A caller-supplied DD gives the historical batch profile a real
       record stream without depending on a terminal session. */
    static int batch_fd;
    Slot *s;
    if (!batch_fd) {
        batch_fd=open_stream("DD:LABOUT",1,1);
        if (batch_fd<3) return -1;
    }
    s=get(batch_fd);
    if (!s || line_used>s->capacity) return error(EOVERFLOW);
    for (unsigned i=0;i<line_used;++i) {
        int native=lab_unicode_1047(line[i]);
        if (native<0) return error(EILSEQ);
        s->buffer[4+i]=(unsigned char)native;
    }
    s->used=line_used;
    line_used=0;
    return emit(s);
#else
    int rc=lab_tso_services->putline((const char *)line,line_used);
    line_used=0;return rc?error(EIO):0;
#endif
}
_ssize_t _write(int fd,const void *input,size_t length)
{
#ifdef LAB_TSO24
    if (!low_buffer(input,length)) return error(EFAULT);
#endif
    const unsigned char *bytes=input;
    Slot *s=0;
    if (length>INT_MAX) return error(EINVAL);
    if (fd!=1 && fd!=2) {
        s=get(fd);if (!s) return -1;
        if (!s->writing) return error(EBADF);
        if (s->failed) return error(s->failed);
    }
    for (unsigned i=0;i<length;++i) {
        if (!s || s->text) {
            unsigned scalar=0;
            int rc=lab_utf8_byte(s?&s->decoder:&console_decoder,bytes[i],&scalar);
            if (rc<0 || (rc && !scalar)) goto bad_encoding;
            if (!rc) continue;
            if (scalar=='\n') {
                if (s ? emit(s) : putline()) return -1;
                continue;
            }
            if (lab_unicode_1047(scalar)<0) goto bad_encoding;
            if (s) {
                if (s->used==s->capacity) { s->failed=EOVERFLOW;return error(EOVERFLOW); }
                s->buffer[4+s->used++]=(unsigned char)lab_unicode_1047(scalar);
            } else {
                if (line_used==sizeof line && putline()) return -1;
                /* Native PUTLINE translates Latin-1 bytes to IBM1047. */
                line[line_used++]=(unsigned char)scalar;
            }
        } else {
            s->buffer[4+s->used++]=bytes[i];
            if (s->used==s->capacity && emit(s)) return -1;
        }
    }
    if (s) s->position+=(unsigned)length;
    return (_ssize_t)length;
bad_encoding:
    if (s) s->failed=EILSEQ;
    return error(EILSEQ);
}
int _close(int fd)
{
    Slot *s=get(fd);int e;
    if (!s) return -1;
    e=s->failed;
    if (s->writing && s->decoder.remaining) e=EILSEQ;
    if (!e && s->writing && s->used && emit(s)) e=errno;
    if (close_native(s) && !e) e=errno;
    if (s->dynamic && lab_tso_filecall(5,s->dd) && !e) e=EIO;
    free(s->buffer);memset(s,0,sizeof *s);
    return e?error(e):0;
}
off_t _lseek(int fd,off_t offset,int whence)
{
    Slot *s=get(fd);
    unsigned target;
    unsigned char discard[256];
    if (!s) return (off_t)-1;
    if (whence==SEEK_CUR && !offset) return (off_t)s->position;
    if (s->writing) return error(ESPIPE);
    if (whence==SEEK_END) {
        _ssize_t n;
        while ((n=_read(fd,discard,sizeof discard))>0) {}
        if (n<0) return -1;
        whence=SEEK_CUR;
    }
    if (whence!=SEEK_CUR && whence!=SEEK_SET) return error(EINVAL);
    if (whence==SEEK_CUR) {
        if (offset>=0) {
            if ((uintmax_t)offset>INT_MAX-s->position) return error(EINVAL);
            target=s->position+(unsigned)offset;
        } else {
            uintmax_t back=(uintmax_t)(-(offset+1));
            if (back>=s->position) return error(EINVAL);
            target=s->position-(unsigned)back-1;
        }
    } else {
        if (offset<0 || (uintmax_t)offset>INT_MAX) return error(EINVAL);
        target=(unsigned)offset;
    }
    if (target<s->position) {
        if (close_native(s) || native_open(s)) return -1;
        s->position=s->at=s->available=0;s->eof=s->failed=0;
    }
    while (s->position<target) {
        unsigned n=target-s->position;
        _ssize_t actual;
        if (n>sizeof discard) n=sizeof discard;
        actual=_read(fd,discard,n);
        if (actual<0) return -1;
        if (!actual) return error(EINVAL);
    }
    return (off_t)s->position;
}
int _fstat(int fd,struct stat *st)
{
    if (!st) return error(EINVAL);
    if (fd<0 || (fd>2 && !get(fd))) return error(EBADF);
    memset(st,0,sizeof *st);
    /* Record streams have no cheap POSIX size. S_IFIFO disables newlib's
       regular-file seek shortcut; _lseek still implements bounded scans. */
    st->st_mode=fd<=2?S_IFCHR:S_IFIFO;st->st_blksize=256;
    return 0;
}
int stat(const char *path,struct stat *st)
{
    FILE *f;
    long length;
    int bad;
    if (!st) return error(EINVAL);
    f=fopen(path,"rb");if (!f) return -1;
    bad=fseek(f,0,SEEK_END);length=bad?-1:ftell(f);
    if (fclose(f)) bad=1;
    if (bad || length<0) return error(EIO);
    memset(st,0,sizeof *st);st->st_mode=S_IFREG|0444;
    st->st_nlink=1;st->st_size=length;st->st_blksize=256;
    return 0;
}
/* Dataset/member replacement and deletion require a separately qualified
   native lifecycle contract. Report the gap without destructive emulation. */
int remove(const char *path) { (void)path;return error(ENOTSUP); }
int rename(const char *from,const char *to)
{ (void)from;(void)to;return error(ENOTSUP); }
int _isatty(int fd)
{
    if (fd>=0 && fd<=2) return 1;
    if (get(fd)) error(ENOTTY);
    return 0;
}
ssize_t write(int fd,const void *p,size_t n) { return _write_r(_REENT,fd,p,n); }
ssize_t read(int fd,void *p,size_t n) { return _read_r(_REENT,fd,p,n); }
int open(const char *p,int flags,...) { return _open(p,flags); }
int close(int fd) { return _close(fd); }
off_t lseek(int fd,off_t offset,int whence) { return _lseek(fd,offset,whence); }
int fstat(int fd,struct stat *st) { return _fstat(fd,st); }
int isatty(int fd) { return _isatty(fd); }
/* Generic newlib loses fopen's 'b' flag. Wrap at the ELF link boundary so
   text mode is decided before that loss, with the normal FILE implementation. */
extern FILE *__real_fopen(const char *,const char *);
static int text_read(void *p,char *b,int n) { return (int)_read((int)(uintptr_t)p,b,(size_t)n); }
static int text_write(void *p,const char *b,int n) { return (int)_write((int)(uintptr_t)p,b,(size_t)n); }
static fpos_t text_seek(void *p,fpos_t off,int whence) { return _lseek((int)(uintptr_t)p,off,whence); }
static int text_close(void *p) { return _close((int)(uintptr_t)p); }
FILE *__wrap_fopen(const char *p,const char *mode)
{
    FILE *f;int fd,writing;
    if (!mode) { error(EINVAL);return 0; }
    if (!strcmp(mode,"rb") || !strcmp(mode,"wb")) return __real_fopen(p,mode);
    if (strcmp(mode,"r") && strcmp(mode,"w")) { error(ENOTSUP);return 0; }
    writing=*mode=='w';fd=open_stream(p,writing,1);
    if (fd<0) return 0;
    f=funopen((void *)(uintptr_t)(unsigned)fd,writing?0:text_read,writing?text_write:0,
              text_seek,text_close);
    if (!f) { int e=*__errno();_close(fd);error(e); }
    return f;
}
static void report_heap(void)
{
#ifdef LAB_TSO_REPORT_HEAP
    /* No libc allocation is needed to report an exhausted libc heap. This
       measures the monotonic _sbrk high-water mark, not all native OS storage. */
    const char *labels[]={"TSO HEAP HIGH-WATER="," LIMIT="," FAILED-REQUEST="};
    unsigned values[]={heap_used,HEAP_SIZE,heap_failed_request};
    char report[112];unsigned at=0;
    for (unsigned i=0;i<3;++i) {
        const char *p=labels[i];char digits[10];unsigned n=0,value=values[i];
        while (*p) report[at++]=*p++;
        do { digits[n++]=(char)('0'+value%10);value/=10; } while (value);
        while (n) report[at++]=digits[--n];
    }
    lab_tso_services->putline(report,at);
    {
        const char *label="TSO HEAP BASE=0x";
        uintptr_t value=(uintptr_t)heap;
        static const char hex[]="0123456789ABCDEF";
        at=0;
        while (*label) report[at++]=*label++;
        for (unsigned i=sizeof(value)*2;i>0;--i)
            report[at++]=hex[(value>>((i-1)*4))&15U];
        lab_tso_services->putline(report,at);
    }
#endif
}
int lab_tso_io_finish(void)
{
    int failed=0;
    report_heap();
    if (console_decoder.remaining) failed=1;
    if (line_used && putline()) failed=1;
    for (unsigned i=0;i<SLOTS;++i)
        if (slots[i].active && _close((int)i+3)) failed=1;
    if (heap && lab_tso_services->release(heap,HEAP_SIZE)) failed=1;
    heap=0;
    return failed?-1:0;
}

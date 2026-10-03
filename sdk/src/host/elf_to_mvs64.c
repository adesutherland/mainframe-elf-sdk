/* Bounded ELF64 S/390 -> native MVS object exporter.
 * Derived from the project-owned ELF32 validator, kept separate during TSO64 qualification.
 * The default below-bar profile retains its original low-word RLD output.
 * --high emits complete eight-byte RLD fields for an RMODE64 CSECT. This
 * makes the same image relocatable to any representable high load address.
 * Already-linked relative displacements are checked, not relocated.
 * Project-authored code; no PDOS/GNU implementation is copied.
 * Format evidence: VM370CE DMSLDR/DMSMOD/NUCON and Assembler F Appendix B.
 * See docs/compiler/TSO64.md for the deliberately narrow input contract.
 * Orchestration is cREXX; this binary-format utility uses C for explicit
 * byte/range checks. It is not an instruction translator or a general linker.
 */
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define OUTPUT_LABEL "ELF-MVS64"

#define MAX_FILE (64U * 1024U * 1024U)
#define MAX_IMAGE (8U * 1024U * 1024U)
/* Historical CMS's file record count is a 16-bit field. MVS sequential/PDSE
   object input has no such CMS FST limit. Bound that independent route to
   500,000 cards; the traditional object's 24-bit offsets still bound images. */
#define MAX_CARDS 500000U
enum { PROGBITS=1, SYMTAB=2, STRTAB=3, RELA=4, NOBITS=8, REL=9 };
typedef struct {
    uint32_t type, flags, addr, off, size, link, info, align, ent, name;
} Section;
static unsigned char *file, image[MAX_IMAGE], occupied[MAX_IMAGE], fixes[MAX_IMAGE];
static unsigned char relocated[MAX_IMAGE];
static size_t file_size;
static uint32_t image_size, fix_count, bss_size;
static const char *output;
static char *temporary;
static int temporary_owned;
static unsigned char card[80];

static void cleanup(void) {
    if (temporary_owned) unlink(temporary);
}

_Noreturn static void fail(const char *reason)
{
    fprintf(stderr, "elf_to_mvs64: %s\n", reason);
    exit(1);
}
static uint32_t get16(const unsigned char *p) { return (uint32_t)p[0]*256+p[1]; }
static uint32_t get32(const unsigned char *p)
{ return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3]; }
static uint64_t get64(const unsigned char *p)
{ return (uint64_t)get32(p)<<32 | get32(p+4); }
static uint32_t bounded64(const unsigned char *p)
{
    uint64_t n=get64(p);
    if (n>UINT32_MAX) fail("ELF field exceeds bounded image/file range");
    return (uint32_t)n;
}
static void put16(unsigned char *p, uint32_t v) { p[0]=(unsigned char)(v>>8); p[1]=(unsigned char)v; }
static void put24(unsigned char *p, uint32_t v)
{ p[0]=(unsigned char)(v>>16); p[1]=(unsigned char)(v>>8); p[2]=(unsigned char)v; }
static const unsigned char *slice(uint32_t off, uint32_t size)
{
    if (off > file_size || size > file_size-off) fail("file range out of bounds");
    return file+off;
}
static int allocated(const Section *s) { return (s->flags & 2) != 0; }
static void begin_card(const unsigned char tag[3])
{ memset(card, 0x40, sizeof card); card[0]=2; memcpy(card+1,tag,3); }
static void emit(FILE *f)
{ if (fwrite(card,1,sizeof card,f)!=sizeof card) fail("deck write failed"); }

int main(int argc, char **argv)
{
    FILE *f;
    struct stat in_stat, out_stat;
    Section *sec;
    uint32_t shoff, shnum, phoff, phnum, entry, i, j, rela_sections=0, symtabs=0, loads=0;
    uint32_t rld_per_card, deck_cards, used, n;
    int fd;
    int entry_valid=0;
    int input_exists;
    int high=(argc>=2 && (!strcmp(argv[1],"--high") || !strcmp(argv[1],"--check-high")));
    int check_only=argc==3 && (!strcmp(argv[1],"--check") || !strcmp(argv[1],"--check-high"));
    const char *input=check_only?argv[2]:(high?(argc>2?argv[2]:""):(argc>1?argv[1]:""));
    /* No truncation or removal of an aliased input, even on a rejected input. */
    if ((!high && argc!=3) || (high && argc!=(check_only?3:4))) {
        fprintf(stderr,"usage: elf_to_mvs64 [--high] INPUT.elf OUTPUT.text | --check[-high] INPUT.elf\n");
        return 2;
    }
    if (atexit(cleanup)) fail("cannot register temporary cleanup");
    input_exists=(stat(input,&in_stat)==0);
    if (!check_only) {
        const char *dest=argv[high?3:2];
        if (!strcmp(input,dest)) fail("input/output alias");
        if (input_exists && !stat(dest,&out_stat) && in_stat.st_dev==out_stat.st_dev && in_stat.st_ino==out_stat.st_ino)
            fail("input/output alias");
        output=dest;
        if (!stat(output,&out_stat) && !S_ISREG(out_stat.st_mode)) fail("output is not a regular file");
        if (unlink(output) && errno!=ENOENT) fail("cannot remove old output");
    }
    if (!input_exists) fail("cannot stat input");
    if (!S_ISREG(in_stat.st_mode) || in_stat.st_size<64 || in_stat.st_size>MAX_FILE) fail("input size/type outside bounded profile");
    file_size=(size_t)in_stat.st_size;
    file=malloc(file_size);
    if (!file) fail("allocation failed");
    f=fopen(input,"rb");
    if (!f || fread(file,1,file_size,f)!=file_size) fail("input read failed");
    if (fclose(f)) fail("input close failed");
    if (memcmp(file,"\177ELF\2\2\1",7) || file[7]!=0 || file[8]!=0)
        fail("requires ELF64 big-endian System V version 1");
    if (get16(file+16)!=2 || get16(file+18)!=22 || get32(file+20)!=1 || get32(file+48)!=0)
        fail("requires linked static ET_EXEC EM_S390 with no flags");
    if (get16(file+52)!=64 || get16(file+58)!=64) fail("unexpected ELF header sizes");
    shoff=bounded64(file+40); shnum=get16(file+60); entry=bounded64(file+24);
    if (!shnum || shnum>1024 || shoff<64 || !get16(file+62) || get16(file+62)>=shnum) fail("invalid section table");
    slice(shoff,shnum*64);
    phoff=bounded64(file+32); phnum=get16(file+56);
    if (!phnum || phnum>1024 || phoff<64 || get16(file+54)!=56) fail("invalid program header size/count");
    slice(phoff,phnum*56);
    for (i=0;i<phnum;i++) {
        const unsigned char *p=slice(phoff+i*56,56);
        uint32_t type=get32(p), off=bounded64(p+8), addr=bounded64(p+16);
        uint32_t filesz=bounded64(p+32), memsz=bounded64(p+40), flags=get32(p+4), align=bounded64(p+48);
        if (type!=1 && type!=0x6474e551U) fail("only static LOAD/GNU_STACK program headers supported");
        if (flags&~7U) fail("invalid segment flags");
        if (type!=1) {
            if (off || addr || bounded64(p+24) || filesz || memsz || (flags&1)) fail("unsupported GNU_STACK segment");
            continue;
        }
        loads++;
        slice(off,filesz);
        if (filesz>memsz || addr>=MAX_IMAGE || memsz>MAX_IMAGE-addr || bounded64(p+24)!=addr)
            fail("invalid LOAD file/memory range");
        if (align>0x1000000U || (align && (align&(align-1))) || (align>1 && addr%align!=off%align))
            fail("invalid LOAD alignment");
        for (j=0;j<i;j++) {
            const unsigned char *q=slice(phoff+j*56,56);
            uint32_t start=bounded64(q+16), size=bounded64(q+40);
            if (get32(q)==1 && memsz && size && addr<start+size && start<addr+memsz)
                fail("overlapping LOAD segments");
        }
    }
    if (!loads) fail("requires a LOAD segment");
    sec=calloc(shnum,sizeof *sec);
    if (!sec) fail("allocation failed");
    for (i=0;i<shnum;i++) {
        const unsigned char *p=slice(shoff+i*64,64);
        Section *s=sec+i;
        s->type=get32(p+4); s->flags=bounded64(p+8); s->addr=bounded64(p+16);
        s->off=bounded64(p+24); s->size=bounded64(p+32); s->link=get32(p+40);
        s->info=get32(p+44); s->align=bounded64(p+48); s->ent=bounded64(p+56);
        s->name=get32(p);
        if (!i) { for (j=0;j<64;j++) if (p[j]) fail("nonzero null section"); }
        if (s->type!=NOBITS) slice(s->off,s->size);
        if (!allocated(s)) continue;
        if ((s->flags & ~7U) || (s->type!=PROGBITS && s->type!=NOBITS))
            fail("unsupported allocated section (TLS/dynamic/etc.)");
        if (!s->align || s->align>8 || (s->align&(s->align-1)) || s->addr%s->align)
            fail("allocated alignment must be a power of two <= 8");
        if ((s->flags&4) && s->type!=PROGBITS) fail("executable NOBITS unsupported");
        if (s->addr>=MAX_IMAGE || s->size>MAX_IMAGE-s->addr) fail("image exceeds configured limit");
        int mapped=0;
        for (j=0;j<phnum;j++) {
            const unsigned char *q=slice(phoff+j*56,56);
            uint32_t start=bounded64(q+16), size=bounded64(q+40), flags=get32(q+4);
            if (get32(q)!=1 || s->addr<start || s->addr-start>size || s->size>size-(s->addr-start)) continue;
            if (((s->flags&1) && !(flags&2)) || ((s->flags&4) && !(flags&1)) || !(flags&4))
                fail("section/segment permissions disagree");
            if (s->type==PROGBITS && (s->addr-start>bounded64(q+32) || s->size>bounded64(q+32)-(s->addr-start) ||
                (uint64_t)s->off!=(uint64_t)bounded64(q+8)+s->addr-start)) fail("section/segment file mapping disagrees");
            if (s->type==NOBITS && s->size && s->addr-start<bounded64(q+32)) fail("BSS overlaps initialized segment bytes");
            mapped=1;
        }
        if (!mapped) fail("allocated section is not covered by LOAD");
        for (j=s->addr;j<s->addr+s->size;j++) {
            if (occupied[j]) fail("overlapping allocated sections");
            occupied[j]=1;
        }
        if (s->type==PROGBITS) memcpy(image+s->addr,slice(s->off,s->size),s->size);
        else bss_size+=s->size; /* image is initially zero, including gaps/BSS */
        if (s->addr+s->size>image_size) image_size=s->addr+s->size;
        if ((s->flags&4) && s->type==PROGBITS && entry>=s->addr && entry-s->addr<s->size)
            entry_valid=1;
    }
    if (entry!=0) fail("native ELFPOC boundary requires entry at image offset zero");
    if (!image_size || !occupied[0] || !entry_valid || (entry&1))
        fail("image must start at zero with an aligned executable entry");
    for (i=0;i<phnum;i++) {
        const unsigned char *p=slice(phoff+i*56,56);
        if (get32(p)!=1) continue;
        uint32_t addr=bounded64(p+16), size=bounded64(p+32), off=bounded64(p+8);
        if (addr+bounded64(p+40)>image_size) fail("LOAD extends beyond section image");
        for (j=0;j<size;j++) if (!occupied[addr+j] && file[off+j])
            fail("nonzero LOAD bytes outside allocated sections");
    }
    Section *names=sec+get16(file+62);
    if (names->type!=STRTAB || !names->size) fail("missing section string table");
    for (i=0;i<shnum;i++)
        if (sec[i].name>=names->size || !memchr(file+names->off+sec[i].name,0,names->size-sec[i].name))
            fail("invalid section name");
    for (i=1;i<shnum;i++) {
        Section *s=sec+i;
        if (s->type==SYMTAB) {
            symtabs++;
            if (s->ent!=24 || s->size<24 || s->size%24 || s->link>=shnum || sec[s->link].type!=STRTAB || s->info>s->size/24)
                fail("invalid symbol table");
            for (j=0;j<24;j++) if (file[s->off+j]) fail("nonzero null symbol");
            for (j=24;j<s->size;j+=24) {
                const unsigned char *p=slice(s->off+j,24);
                if (get16(p+6)==0) fail("undefined symbol in linked image");
                uint32_t name=get32(p), index=get16(p+6);
                if (name>=sec[s->link].size || !memchr(file+sec[s->link].off+name,0,sec[s->link].size-name))
                    fail("invalid symbol name");
                if (index>=shnum && index!=0xfff1U) fail("unsupported symbol section index");
            }
        }
        if (s->type==REL) fail("REL relocations unsupported; require RELA");
        if (s->type!=RELA) continue;
        if (s->info>=shnum || s->link>=shnum || sec[s->link].type!=SYMTAB ||
            s->ent!=24 || s->size%24) fail("invalid RELA section");
        if (!allocated(sec+s->info)) continue; /* nonloaded debug references */
        if (sec[s->info].type!=PROGBITS) fail("relocation target must be initialized");
        rela_sections++;
        for (j=0;j<s->size;j+=24) {
            const unsigned char *p=slice(s->off+j,24), *sym;
            uint32_t offset=bounded64(p), type=get32(p+12), symno=get32(p+8);
            uint32_t value, symsec, width, fix_offset=offset;
            int relative=0, scale=1;
            Section *target=sec+s->info, *symbols=sec+s->link;
            if (type==22) width=8; /* R_390_64 */
            else if (type==4) width=4; /* R_390_32 */
            else if (type==5 || type==19 || type==20) {
                width=4;relative=1;scale=type==5?1:2;
            } else fail("unsupported ELF64 relocation");
            if (symbols->ent!=24 || symbols->size%24 || !symno || symno>=symbols->size/24)
                fail("invalid relocation symbol");
            if (offset<target->addr || target->size<width || offset-target->addr>target->size-width ||
                (offset & (relative?1U:3U))) fail("relocation outside initialized aligned field");
            sym=slice(symbols->off+symno*24,24);symsec=get16(sym+6);value=bounded64(sym+8);
            if (!symsec || symsec>=shnum || !allocated(sec+symsec))
                fail("relocation requires a defined image-relative symbol");
            if (value<sec[symsec].addr || value-sec[symsec].addr>sec[symsec].size)
                fail("symbol value outside its section");
            int64_t addend=(int64_t)get64(p+16);
            if (addend<-(int64_t)value || addend>(int64_t)image_size-value)
                fail("relocation target outside image");
            int64_t expected=(int64_t)value+addend;
            /* Track every byte, including PC-relative fields, so overlapping
               or duplicate relocations cannot disguise a second write. */
            for (uint32_t k=0;k<width;k++) {
                if (relocated[offset+k]) fail("overlapping relocation fields");
                relocated[offset+k]=1;
            }
            if (relative) {
                expected-=offset;
                if (expected%scale || expected/scale<INT32_MIN || expected/scale>INT32_MAX)
                    fail("relative displacement out of range");
                if (get32(image+offset)!=(uint32_t)(int32_t)(expected/scale))
                    fail("linked relative displacement disagrees with RELA");
                continue;
            }
            if (high && width==4)
                fail("RMODE64 image has a four-byte absolute relocation that would truncate its high target");
            if (width==8) {
                if (get64(image+offset)!=(uint64_t)expected)
                    fail("linked doubleword disagrees with RELA or has a high image address");
                if (!high) fix_offset+=4;
            } else if (get32(image+offset)!=(uint32_t)expected)
                fail("linked word disagrees with RELA");
            fixes[fix_offset]=high?0x5c:0x0c;fix_count++;
        }
    }
    if (symtabs!=1) fail("requires one retained symbol table");
    if (!rela_sections || !fix_count) fail("requires retained relocations; link with --emit-relocs");
    /* Preserve existing small decks. Dense images use seven complete eight-
       byte RLD entries per card; flag bit 7 remains clear on every entry. */
    rld_per_card = 2U + (image_size+55U)/56U + fix_count > MAX_CARDS ? 7U : 1U;
    deck_cards = 2U + (image_size+55U)/56U + (fix_count+rld_per_card-1U)/rld_per_card;
    if (deck_cards > MAX_CARDS) {
        fail("MVS object deck exceeds configured record limit");
    }
    if (check_only) {
        printf(OUTPUT_LABEL " CHECK: image=%u initialized-and-gaps=%u bss=%u entry=%u fixups=%u limit=%u cards=%u\n",
               image_size,image_size-bss_size,bss_size,entry,fix_count,MAX_IMAGE,deck_cards);
        free(sec); free(file); return 0;
    }
    temporary=malloc(strlen(output)+32);
    if (!temporary) fail("allocation failed");
    if (snprintf(temporary,strlen(output)+32,"%s.tmp.XXXXXX",output)<0)
        fail("temporary filename formatting failed");
    fd=mkstemp(temporary);
    temporary_owned=fd>=0;
    f=fd<0?NULL:fdopen(fd,"wb");
    if (!f) fail("cannot create temporary output");
    /* The high object needs a real PC section CESD entry. PDLD turns a lone
       named SD into a label but then emits RLD P-index zero, which is not a
       valid source CSECT. Keep the established low SD unchanged. */
    begin_card((const unsigned char *)"\305\342\304"); /* ESD */
    put16(card+10,high?32:16); put16(card+14,1);
    if (!high) memcpy(card+16,"\305\323\306\327\326\303\100\100",8);
    card[24]=high?4:0; put24(card+25,0);
    /* AMODE64/RMODE ANY below the bar or AMODE64/RMODE64 above it.
       The high application has its own native entry and is loaded by a
       separately bound low launcher through ordinary LOAD. */
    card[28]=high?0x31:0x06;
    put24(card+29,image_size);
    if (high) {
        memcpy(card+32,"\305\323\306\327\326\303\100\100",8); /* ELFPOC */
        card[40]=1; put24(card+41,entry); put24(card+45,1);
    }
    emit(f);
    for (i=0;i<image_size;i+=n) {
        n=image_size-i; if (n>56) n=56;
        begin_card((const unsigned char *)"\343\347\343"); /* TXT */
        put24(card+5,i); put16(card+10,n); put16(card+14,1);
        memcpy(card+16,image+i,n); emit(f);
    }
    used=0;
    for (i=0;i<image_size;i++) if (fixes[i]) {
        if (!used) begin_card((const unsigned char *)"\331\323\304"); /* RLD */
        put16(card+16+used,1); put16(card+18+used,1);
        card[20+used]=fixes[i]; /* 0x5c: eight-byte field; 0x0c: low word */
        put24(card+21+used,i); used+=8;
        if (used==8*rld_per_card) { put16(card+10,used); emit(f); used=0; }
    }
    if (used) { put16(card+10,used); emit(f); }
    begin_card((const unsigned char *)"\305\325\304"); /* END */
    put24(card+5,entry); put16(card+14,1); emit(f);
    if (fclose(f)) fail("output close failed");
    if (rename(temporary,output)) fail("output rename failed");
    temporary_owned=0;
    printf(OUTPUT_LABEL " image=%u bss=%u entry=%u native-%s-fixups=%u section=ELFPOC\n",
           image_size,bss_size,entry,high?"eight-byte":"low-word",fix_count);
    free(temporary); temporary=NULL; free(sec); free(file);
    return 0;
}

/* Bounded ELF32 S/390 -> CMS TEXT or fixed-origin MODULE exporter.
 * Project-authored code; no PDOS/GNU implementation is copied.
 * Format evidence: VM370CE DMSLDR/DMSMOD/NUCON and Assembler F Appendix B.
 * See docs/ELF-CMS-LINK.md for the deliberately narrow input contract.
 * Orchestration is cREXX; this binary-format utility uses C for explicit
 * byte/range checks. It is not an instruction translator or a general linker.
 */
#ifdef SEGMENTED_OUTPUT
#define _DARWIN_C_SOURCE
#endif
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef MVS_AMODE24
#define OUTPUT_LABEL "ELF-MVS24"
#elif defined(MVS_OUTPUT)
#define OUTPUT_LABEL "ELF-MVS31"
#else
#define OUTPUT_LABEL "ELF-CMS"
#endif

#if defined(SEGMENTED_OUTPUT) || defined(MODULE_OUTPUT) || defined(MVS_OUTPUT)
#define MAX_FILE (64U * 1024U * 1024U)
#define MAX_IMAGE (8U * 1024U * 1024U)
#else
#define MAX_FILE (16U * 1024U * 1024U)
#endif
#ifndef MAX_IMAGE
#define MAX_IMAGE (2U * 1024U * 1024U)
#endif
/* Historical CMS's file record count is a 16-bit field. MVS sequential/PDSE
   object input has no such CMS FST limit. Bound that independent route to
   500,000 cards; the traditional object's 24-bit offsets still bound images. */
#ifndef MAX_CARDS
#ifdef MVS_OUTPUT
#define MAX_CARDS 500000U
#else
#define MAX_CARDS 65535U
#endif
#endif
enum { PROGBITS=1, SYMTAB=2, STRTAB=3, RELA=4, NOBITS=8, REL=9 };
typedef struct {
    uint32_t type, flags, addr, off, size, link, info, align, ent, name;
} Section;
static unsigned char *file, image[MAX_IMAGE], occupied[MAX_IMAGE], fixes[MAX_IMAGE];
static size_t file_size;
static uint32_t image_size, fix_count, bss_size;
static const char *output;
static char *temporary;
static int temporary_owned;
#ifndef MODULE_OUTPUT
static unsigned char card[80];
#endif
#ifdef SEGMENTED_OUTPUT
#include "elf_to_cms_segments.h"
#endif

static void cleanup(void) {
#ifdef SEGMENTED_OUTPUT
    segments_cleanup();
#endif
    if (temporary_owned) unlink(temporary);
}

static void fail(const char *reason)
{
    fprintf(stderr, "elf_to_cms: %s\n", reason);
    exit(1);
}
static uint32_t get16(const unsigned char *p) { return (uint32_t)p[0]*256+p[1]; }
static uint32_t get32(const unsigned char *p)
{ return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3]; }
static void put16(unsigned char *p, uint32_t v) { p[0]=(unsigned char)(v>>8); p[1]=(unsigned char)v; }
#ifndef MODULE_OUTPUT
static void put24(unsigned char *p, uint32_t v)
{ p[0]=(unsigned char)(v>>16); p[1]=(unsigned char)(v>>8); p[2]=(unsigned char)v; }
#endif
static const unsigned char *slice(uint32_t off, uint32_t size)
{
    if (off > file_size || size > file_size-off) fail("file range out of bounds");
    return file+off;
}
static int allocated(const Section *s) { return (s->flags & 2) != 0; }
#ifndef MODULE_OUTPUT
static void begin_card(const unsigned char tag[3])
{ memset(card, 0x40, sizeof card); card[0]=2; memcpy(card+1,tag,3); }
#ifndef SEGMENTED_OUTPUT
static void emit(FILE *f)
{ if (fwrite(card,1,sizeof card,f)!=sizeof card) fail("deck write failed"); }
#else
#include "elf_to_cms_segments_impl.h"
#endif
#else
#include "elf_to_cms_module_impl.h"
#endif

int main(int argc, char **argv)
{
    FILE *f;
    struct stat in_stat, out_stat;
    Section *sec;
    uint32_t shoff, shnum, phoff, phnum, entry, i, j, rela_sections=0, symtabs=0, loads=0;
#if !defined(SEGMENTED_OUTPUT) && !defined(MODULE_OUTPUT)
    uint32_t rld_per_card, deck_cards, used, n;
    int fd;
#endif
    int entry_valid=0;
    int input_exists;
#ifdef MODULE_OUTPUT
    int check_only=argc==4 && !strcmp(argv[1],"--check");
    int verify_only=argc==5 && !strcmp(argv[1],"--verify");
    if ((!check_only && argc!=4 && !verify_only) || (argc==4 && !strcmp(argv[1],"--verify"))) {
        fprintf(stderr,"usage: elf_to_cms_module INPUT.elf OUTPUT.module ORIGIN_HEX | --check INPUT.elf ORIGIN_HEX | --verify INPUT.elf OUTPUT.module ORIGIN_HEX\n"); return 2;
    }
    const char *input=(check_only || verify_only)?argv[2]:argv[1];
    const char *origin_arg=argv[argc-1];
#else
    int check_only=argc==3 && !strcmp(argv[1],"--check");
#ifdef SEGMENTED_OUTPUT
    int verify_only=argc==4 && !strcmp(argv[1],"--verify");
    const char *input=verify_only?argv[2]:(check_only?argv[2]:(argc>1?argv[1]:""));
#else
    const char *input=check_only?argv[2]:(argc>1?argv[1]:"");
#endif
#endif
    /* No truncation or removal of an aliased input, even on a rejected input. */
#if defined(MODULE_OUTPUT)
    /* Argument shape was checked above. */
#elif defined(SEGMENTED_OUTPUT)
    if (argc!=3 && !verify_only) { fprintf(stderr,"usage: elf_to_cms_segments INPUT.elf NEW_DIRECTORY | --check INPUT.elf | --verify INPUT.elf DIRECTORY\n"); return 2; }
#else
    if (argc!=3) { fprintf(stderr,"usage: elf_to_cms INPUT.elf OUTPUT.text | --check INPUT.elf\n"); return 2; }
#endif
    if (atexit(cleanup)) fail("cannot register temporary cleanup");
    input_exists=(stat(input,&in_stat)==0);
#ifdef MODULE_OUTPUT
    if (!check_only) {
        output=verify_only?argv[3]:argv[2];
        if (!strcmp(input,output) || (input_exists && !stat(output,&out_stat) && in_stat.st_dev==out_stat.st_dev && in_stat.st_ino==out_stat.st_ino))
            fail("input/output alias");
        if (!lstat(output,&out_stat)) {
            if (!S_ISREG(out_stat.st_mode)) fail("output must be a regular file, not a symlink");
        } else if (errno!=ENOENT || verify_only) fail("cannot access module file");
        if (!verify_only && unlink(output) && errno!=ENOENT) fail("cannot remove old output");
    }
    uint32_t origin=module_origin(origin_arg);
#elif defined(SEGMENTED_OUTPUT)
    if (!check_only) {
        output=verify_only?argv[3]:argv[2];
        if (!strcmp(input,output)) fail("input/output alias");
        if (!lstat(output,&out_stat)) {
            if (!verify_only || !S_ISDIR(out_stat.st_mode)) fail("requires a new output directory or a real directory to verify");
        } else if (errno!=ENOENT || verify_only) fail("cannot access package directory");
    }
#else
    if (!check_only) {
        if (!strcmp(input,argv[2])) fail("input/output alias");
        if (input_exists && !stat(argv[2],&out_stat) && in_stat.st_dev==out_stat.st_dev && in_stat.st_ino==out_stat.st_ino)
            fail("input/output alias");
        output=argv[2];
        if (!stat(output,&out_stat) && !S_ISREG(out_stat.st_mode)) fail("output is not a regular file");
        if (unlink(output) && errno!=ENOENT) fail("cannot remove old output");
    }
#endif
    if (!input_exists) fail("cannot stat input");
    if (!S_ISREG(in_stat.st_mode) || in_stat.st_size<52 || in_stat.st_size>MAX_FILE) fail("input size/type outside bounded profile");
    file_size=(size_t)in_stat.st_size;
    file=malloc(file_size);
    if (!file) fail("allocation failed");
    f=fopen(input,"rb");
    if (!f || fread(file,1,file_size,f)!=file_size) fail("input read failed");
    if (fclose(f)) fail("input close failed");
    if (memcmp(file,"\177ELF\1\2\1",7) || file[7]!=0 || file[8]!=0)
        fail("requires ELF32 big-endian System V version 1");
    if (get16(file+16)!=2 || get16(file+18)!=22 || get32(file+20)!=1 || get32(file+36)!=0)
        fail("requires linked static ET_EXEC EM_S390 with no flags");
    if (get16(file+40)!=52 || get16(file+46)!=40) fail("unexpected ELF header sizes");
    shoff=get32(file+32); shnum=get16(file+48); entry=get32(file+24);
    if (!shnum || shnum>1024 || shoff<52 || !get16(file+50) || get16(file+50)>=shnum) fail("invalid section table");
    slice(shoff,shnum*40);
    phoff=get32(file+28); phnum=get16(file+44);
    if (!phnum || phnum>1024 || phoff<52 || get16(file+42)!=32) fail("invalid program header size/count");
    slice(phoff,phnum*32);
    for (i=0;i<phnum;i++) {
        const unsigned char *p=slice(phoff+i*32,32);
        uint32_t type=get32(p), off=get32(p+4), addr=get32(p+8);
        uint32_t filesz=get32(p+16), memsz=get32(p+20), flags=get32(p+24), align=get32(p+28);
        if (type!=1 && type!=0x6474e551U) fail("only static LOAD/GNU_STACK program headers supported");
        if (flags&~7U) fail("invalid segment flags");
        if (type!=1) {
            if (off || addr || get32(p+12) || filesz || memsz || (flags&1)) fail("unsupported GNU_STACK segment");
            continue;
        }
        loads++;
        slice(off,filesz);
        if (filesz>memsz || addr>=MAX_IMAGE || memsz>MAX_IMAGE-addr || get32(p+12)!=addr)
            fail("invalid LOAD file/memory range");
        if (align>0x1000000U || (align && (align&(align-1))) || (align>1 && addr%align!=off%align))
            fail("invalid LOAD alignment");
        for (j=0;j<i;j++) {
            const unsigned char *q=slice(phoff+j*32,32);
            uint32_t start=get32(q+8), size=get32(q+20);
            if (get32(q)==1 && memsz && size && addr<start+size && start<addr+memsz)
                fail("overlapping LOAD segments");
        }
    }
    if (!loads) fail("requires a LOAD segment");
    sec=calloc(shnum,sizeof *sec);
    if (!sec) fail("allocation failed");
    for (i=0;i<shnum;i++) {
        const unsigned char *p=slice(shoff+i*40,40);
        Section *s=sec+i;
        s->type=get32(p+4); s->flags=get32(p+8); s->addr=get32(p+12);
        s->off=get32(p+16); s->size=get32(p+20); s->link=get32(p+24);
        s->info=get32(p+28); s->align=get32(p+32); s->ent=get32(p+36);
        s->name=get32(p);
        if (!i) { for (j=0;j<40;j++) if (p[j]) fail("nonzero null section"); }
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
            const unsigned char *q=slice(phoff+j*32,32);
            uint32_t start=get32(q+8), size=get32(q+20), flags=get32(q+24);
            if (get32(q)!=1 || s->addr<start || s->addr-start>size || s->size>size-(s->addr-start)) continue;
            if (((s->flags&1) && !(flags&2)) || ((s->flags&4) && !(flags&1)) || !(flags&4))
                fail("section/segment permissions disagree");
            if (s->type==PROGBITS && (s->addr-start>get32(q+16) || s->size>get32(q+16)-(s->addr-start) ||
                (uint64_t)s->off!=(uint64_t)get32(q+4)+s->addr-start)) fail("section/segment file mapping disagrees");
            if (s->type==NOBITS && s->size && s->addr-start<get32(q+16)) fail("BSS overlaps initialized segment bytes");
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
    if (!image_size || !occupied[0] || !entry_valid || (entry&1))
        fail("image must start at zero with an aligned executable entry");
    for (i=0;i<phnum;i++) {
        const unsigned char *p=slice(phoff+i*32,32);
        if (get32(p)!=1) continue;
        uint32_t addr=get32(p+8), size=get32(p+16), off=get32(p+4);
        if (addr+get32(p+20)>image_size) fail("LOAD extends beyond section image");
        for (j=0;j<size;j++) if (!occupied[addr+j] && file[off+j])
            fail("nonzero LOAD bytes outside allocated sections");
    }
    Section *names=sec+get16(file+50);
    if (names->type!=STRTAB || !names->size) fail("missing section string table");
    for (i=0;i<shnum;i++)
        if (sec[i].name>=names->size || !memchr(file+names->off+sec[i].name,0,names->size-sec[i].name))
            fail("invalid section name");
    for (i=1;i<shnum;i++) {
        Section *s=sec+i;
        if (s->type==SYMTAB) {
            symtabs++;
            if (s->ent!=16 || s->size<16 || s->size%16 || s->link>=shnum || sec[s->link].type!=STRTAB || s->info>s->size/16)
                fail("invalid symbol table");
            for (j=0;j<16;j++) if (file[s->off+j]) fail("nonzero null symbol");
            for (j=16;j<s->size;j+=16) {
                const unsigned char *p=slice(s->off+j,16);
                if (get16(p+14)==0) fail("undefined symbol in linked image");
                uint32_t name=get32(p), index=get16(p+14);
                if (name>=sec[s->link].size || !memchr(file+sec[s->link].off+name,0,sec[s->link].size-name))
                    fail("invalid symbol name");
                if (index>=shnum && index!=0xfff1U) fail("unsupported symbol section index");
            }
        }
        if (s->type==REL) fail("REL relocations unsupported; require RELA");
        if (s->type!=RELA) continue;
        if (s->info>=shnum || s->link>=shnum || sec[s->link].type!=SYMTAB ||
            s->ent!=12 || s->size%12) fail("invalid RELA section");
        if (!allocated(sec+s->info)) continue; /* nonloaded debug references */
        if (sec[s->info].type!=PROGBITS) fail("relocation target must be initialized");
        rela_sections++;
        for (j=0;j<s->size;j+=12) {
            const unsigned char *p=slice(s->off+j,12), *sym;
            uint32_t offset=get32(p), info=get32(p+4), symno=info>>8;
            uint32_t value, symsec, expected;
            Section *target=sec+s->info, *symbols=sec+s->link;
            if ((info&255)!=4) fail("only R_390_32 relocation is supported");
            if (symbols->ent!=16 || symbols->size%16 || !symno || symno>=symbols->size/16)
                fail("invalid relocation symbol");
            if (offset<target->addr || target->size<4 || offset-target->addr>target->size-4 || (offset&3))
                fail("relocation field outside initialized aligned fullword");
            sym=slice(symbols->off+symno*16,16); symsec=get16(sym+14); value=get32(sym+4);
            if (!symsec || symsec>=shnum || !allocated(sec+symsec))
                fail("relocation requires a defined image-relative symbol");
            if (value<sec[symsec].addr || value-sec[symsec].addr>sec[symsec].size)
                fail("symbol value outside its section");
            /* The link has already applied S+A. Origin zero makes it the
             * correct addend for one positive CMS A-type base relocation. */
            {
                int64_t sum=(int64_t)value+(int64_t)(int32_t)get32(p+8);
                if (sum<0 || sum>image_size) fail("address constant outside image");
                expected=(uint32_t)sum;
            }
            if (get32(image+offset)!=expected) fail("linked word disagrees with RELA");
            if (fixes[offset]) fail("duplicate relocation field");
            fixes[offset]=1; fix_count++;
        }
    }
    if (symtabs!=1) fail("requires one retained symbol table");
    if (!rela_sections || !fix_count) fail("requires retained relocations; link with --emit-relocs");
#ifdef MODULE_OUTPUT
    module_run(check_only,verify_only,entry,origin);
    free(sec); free(file); return 0;
#elif defined(SEGMENTED_OUTPUT)
    segments_run(check_only,verify_only,entry);
    free(sec); free(file); return 0;
#else
    /* Preserve existing small decks. Dense images use seven complete eight-
       byte RLD entries per card; flag bit 7 remains clear on every entry. */
    rld_per_card = 2U + (image_size+55U)/56U + fix_count > MAX_CARDS ? 7U : 1U;
    deck_cards = 2U + (image_size+55U)/56U + (fix_count+rld_per_card-1U)/rld_per_card;
    if (deck_cards > MAX_CARDS) {
#ifdef MVS_OUTPUT
        fail("MVS object deck exceeds configured record limit");
#else
        fail("TEXT deck exceeds CMS file record limit");
#endif
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
    /* One SD, ESDID 1; only the boundary name ELFPOC enters CMS. */
    begin_card((const unsigned char *)"\305\342\304"); /* ESD */
    put16(card+10,16); put16(card+14,1);
    memcpy(card+16,"\305\323\306\327\326\303\100\100",8); /* ELFPOC */
    card[24]=0; put24(card+25,0);
#if defined(MVS_OUTPUT) && !defined(MVS_AMODE24)
    /* Native z/OS 1.3 ASMA90 reference: AMODE 31, RMODE ANY. The
       exported C image has no MVS entry bridge or LE dependency. */
    card[28]=0x06;
#endif
    put24(card+29,image_size); emit(f);
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
        card[20+used]=0x0c; /* A type, positive, four-byte field, full next entry */
        put24(card+21+used,i); used+=8;
        if (used==8*rld_per_card) { put16(card+10,used); emit(f); used=0; }
    }
    if (used) { put16(card+10,used); emit(f); }
    begin_card((const unsigned char *)"\305\325\304"); /* END */
    put24(card+5,entry); put16(card+14,1); emit(f);
    if (fclose(f)) fail("output close failed");
    if (rename(temporary,output)) fail("output rename failed");
    temporary_owned=0;
    printf(OUTPUT_LABEL " image=%u bss=%u entry=%u R_390_32=%u section=ELFPOC\n",
           image_size,bss_size,entry,fix_count);
    free(temporary); temporary=NULL; free(sec); free(file);
    return 0;
#endif
}

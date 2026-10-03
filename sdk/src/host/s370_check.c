/* Mainframe Lab trusted-producer ELF profile/range and instruction checker.
 * Project-authored byte parser, separate from GNU as and the CMS exporter.
 * See docs/S370-PROFILE.md for provenance, scope and explicit non-goals. */
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define LIMIT (64U*1024U*1024U)
#define PROFILE "vm370-4381-v1"
#define ID "MLAB1;isa=s370-int1;addr=24;abi=s390-ilp32-r1-v1;chars=ascii;svc=cms1;profile=" PROFILE
#define KPROFILE "vmkernel"
#define KID "MLAB1;isa=s370-int1;addr=24;abi=s390-ilp32-r1-v1;chars=binary;svc=vmkernel1;profile=" KPROFILE
#define C31PROFILE "cms20-esa31-v1"
#define C31ID "MLAB1;isa=s370-int1-bsm;addr=31;abi=s390-ilp32-r1-v1;chars=utf8;svc=cms20-204;profile=" C31PROFILE
#define T31PROFILE "tso-zos31-v1"
#define T31ID "MLAB1;isa=s370-int1;addr=31;abi=s390-ilp32-r1-v1;chars=utf8;svc=mvs31-native1;profile=" T31PROFILE
#define T24PROFILE "tso-zos24-v1"
#define T24ID "MLAB1;isa=s370-int1;addr=24;abi=s390-ilp32-r1-v1;chars=utf8;svc=mvs24-native1;profile=" T24PROFILE
static const char *selected_id = ID;
static int kernel_profile, cms31_profile, tso31_profile, tso24_profile;
static uint32_t address_limit = 0x1000000U;
static int component_profile;
#define TAG ".Llab_"
typedef struct { uint32_t type,flags,addr,off,size,link,info,ent; const char *name; } Sec;
typedef struct { uint32_t sec,start,size; char kind; } Range;
static const char *context;
static unsigned objects,images,members,instructions,data_bytes,padding_bytes;
static void die(const char *s) { fprintf(stderr,"s370_check: %s: %s\n",context,s); exit(1); }
static uint32_t u16(const unsigned char *p) { return (uint32_t)p[0]<<8|p[1]; }
static uint32_t u32(const unsigned char *p) { return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3]; }
static const unsigned char *part(const unsigned char *p,size_t n,uint32_t off,uint32_t size) {
    if (off>n || size>n-off) die("file range out of bounds");
    return p+off;
}
static const char *string(const unsigned char *p,size_t n,uint32_t off) {
    if (off>=n || !memchr(p+off,0,n-off)) die("invalid string table");
    return (const char *)p+off;
}
static unsigned char *read_file(const char *path,size_t *size) {
    struct stat s; FILE *f; unsigned char *p;
    if (stat(path,&s) || s.st_size<1 || s.st_size>LIMIT) die("unreadable or oversized input");
    *size=(size_t)s.st_size; p=malloc(*size);
    if (!p) die("out of memory");
    f=fopen(path,"rb");
    if (!f || fread(p,1,*size,f)!=*size || fclose(f)) die("read failed");
    return p;
}
static void clear_output(const char *out,int count,char **inputs) {
    struct stat a,b;
    if (!stat(out,&a)) for (int i=0;i<count;i++)
        if (!stat(inputs[i],&b) && a.st_dev==b.st_dev && a.st_ino==b.st_ino) die("input/output alias");
    for (int i=0;i<count;i++) if (!strcmp(out,inputs[i])) die("input/output alias");
    if (unlink(out) && errno!=ENOENT) die("cannot remove stale output");
}
/* Preflight the entire output set before deleting anything. In particular,
   an assembly/map/manifest path must never erase a different primary input. */
static void output_set(int argc,char **argv,int remove_outputs) {
    int split=2; struct stat a,b;
    while(split<argc && strcmp(argv[split],"--")) split++;
    if(split==2 || split==argc || split+1==argc) die("outputs require -- and protected inputs");
    for(int i=2;i<split;i++) {
        if(!*argv[i]) die("empty output path");
        int exists=!stat(argv[i],&a);
        if(exists && !S_ISREG(a.st_mode)) die("output is not a regular file");
        for(int j=i+1;j<argc;j++) {
            if(j==split) continue;
            if(!strcmp(argv[i],argv[j]) || (exists && !stat(argv[j],&b) && a.st_dev==b.st_dev && a.st_ino==b.st_ino))
                die("input/output or output/output alias");
        }
    }
    if(remove_outputs) for(int i=2;i<split;i++)
        if(unlink(argv[i]) && errno!=ENOENT) die("cannot remove stale output");
}
/* One entry per accepted primary opcode, independent of mnemonic spelling.
 * RR/RX/RS/SI/SS encodings: IBM GA22-7000-6, Appendix B (corpus M05).
 * BAS/BASR are retained 4381-profile instructions; pinned SDL opcode.c
 * entries 0D/4D use GENx370x390x900, not the optional HERC_370_EXTENSION.
 * Deliberately not the complete S/370 opcode set. */
static void instruction(const unsigned char *p,uint32_t n) {
    unsigned op=p[0], len=0;
    if(kernel_profile && op==0x0a) die("kernel C objects require explicit native service glue, not SVC");
    if((tso31_profile || tso24_profile) && op==0x0a) die("TSO ELF objects require explicit native MVS service glue, not SVC");
    switch(op) {
    case 0x0b:
        if (!cms31_profile) die("BSM requires the explicit 31-bit CMS profile");
        len=2; break;
    case 0x05: case 0x07: case 0x0a: case 0x0d:
    case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:
    case 0x16: case 0x17: case 0x18: case 0x19: case 0x1a: case 0x1b:
    case 0x1c: case 0x1d: case 0x1e: case 0x1f: len=2; break;
    case 0x40: case 0x41: case 0x42: case 0x43: case 0x45: case 0x47:
    case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d:
    case 0x50: case 0x54: case 0x55: case 0x56: case 0x57: case 0x58:
    case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
    case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d:
    case 0x8e: case 0x8f: case 0x90: case 0x91: case 0x92: case 0x94:
    /* CLM: all four mask bits are defined, including zero. GA22-7000-6 7-12/13. */
    case 0x95: case 0x96: case 0x97: case 0x98: case 0xbd: case 0xbf: len=4; break;
    case 0xd2: case 0xd4: case 0xd5: case 0xd6: case 0xd7: len=6; break;
    default: die("instruction opcode outside historical integer profile");
    }
    if (len!=n) die("instruction length does not match range");
    if (cms31_profile && op==0x0a && p[1]!=204)
        die("31-bit CMS service bridge requires SVC 204");
    if ((op==0x1c || op==0x1d || op==0x5c || op==0x5d || (op>=0x8c && op<=0x8f))
        && (p[1]&0x10)) die("instruction requires an even register pair");
    if (op>=0x88 && op<=0x8f && (p[1]&15)) die("nonzero reserved shift field");
    instructions++;
}
static void padding(const unsigned char *p,uint32_t n) {
    /* GNU as/ld use zero or two-byte BCR 0,r padding. Neither is decoded
       as application code. Control-flow safety still needs execution tests. */
    for (uint32_t i=0;i<n;) {
        if (!p[i]) i++;
        else if (i+1<n && p[i]==7 && (p[i+1]>>4)==0) i+=2;
        else die("unexpected alignment fill");
    }
    padding_bytes+=n;
}
static int order(const void *a,const void *b) {
    const Range *x=a,*y=b;
    if(x->sec!=y->sec) return x->sec<y->sec?-1:1;
    if(x->start!=y->start) return x->start<y->start?-1:1;
    return 0;
}
static void elf(const unsigned char *p,size_t n,int want_linked) {
    if (n<52 || memcmp(p,"\177ELF\1\2\1",7) || p[7] || p[8] || u16(p+18)!=22 || u32(p+20)!=1 || u32(p+36)) die("requires ELF32 big-endian System V EM_S390");
    unsigned type=u16(p+16), count=u16(p+48), names=u16(p+50);
    if (type!=(want_linked?2:1)) die("unexpected ELF type (object versus linked image)");
    if (!count || count>8192 || names>=count || u16(p+40)!=52 || u16(p+46)!=40) die("invalid ELF section header");
    const unsigned char *tab=part(p,n,u32(p+32),count*40);
    Sec *s=calloc(count,sizeof *s); Range *ranges=NULL; size_t nr=0,cap=0;
    if (!s) die("out of memory");
    uint32_t executable_size=0;
    for(unsigned i=0;i<count;i++) {
        const unsigned char *h=tab+40*i;
        s[i]=(Sec){u32(h+4),u32(h+8),u32(h+12),u32(h+16),u32(h+20),u32(h+24),u32(h+28),u32(h+36),NULL};
        if (s[i].type!=8) part(p,n,s[i].off,s[i].size);
        if ((s[i].flags&2) && ((s[i].flags&~7U) || (s[i].type!=1 && s[i].type!=8))) die("unsupported allocated section");
        if (s[i].flags&4) {
            if (!(s[i].flags&2) || s[i].type!=1 || s[i].size>LIMIT-executable_size)
                die("invalid/oversized executable section");
            executable_size+=s[i].size;
        }
        if ((s[i].flags&2) && ((type==1 && s[i].addr) || s[i].addr>=address_limit || s[i].size>address_limit-s[i].addr))
            die("allocated section outside profile address contract");
    }
    if(s[names].type!=3) die("missing section names");
    unsigned profiles=0,symtabs=0;
    for(unsigned i=1;i<count;i++) {
        s[i].name=string(p+s[names].off,s[names].size,u32(tab+40*i));
        if(!strcmp(s[i].name,".lab.profile")) {
            size_t id_size=strlen(selected_id)+1;
            if(s[i].type!=1 || s[i].flags || !s[i].size || s[i].size%id_size) die("invalid profile section");
            for(uint32_t j=0;j<s[i].size;j+=(uint32_t)id_size) {
                if(memcmp(p+s[i].off+j,selected_id,id_size)) die("unknown or incompatible profile/ABI identity");
                profiles++;
            }
        }
        if(s[i].type!=2) continue;
        symtabs++;
        if(s[i].ent!=16 || s[i].size%16 || s[i].link>=count || s[s[i].link].type!=3) die("invalid symbol table");
        const Sec *str=&s[s[i].link];
        for(uint32_t j=16;j<s[i].size;j+=16) {
            const unsigned char *sym=p+s[i].off+j;
            const char *name=string(p+str->off,str->size,u32(sym));
            if(strncmp(name,TAG,strlen(TAG))) continue;
            if(strlen(name)<strlen(TAG)+2) die("invalid range tag");
            char kind=name[strlen(TAG)]; const char *digits=name+strlen(TAG)+1;
            if(!strchr("IDPE",kind) || !*digits) die("invalid range tag");
            while(*digits) if(!isdigit((unsigned char)*digits++)) die("invalid range tag");
            uint32_t sec=u16(sym+14),start=u32(sym+4),size=u32(sym+8);
            if(!sec || sec>=count || sym[12] || sym[13]) die("invalid range symbol");
            if(!(s[sec].flags&4)) continue; /* data/BSS outside executable sections */
            if(s[sec].type!=1 || !(s[sec].flags&2) || start<s[sec].addr || start-s[sec].addr>s[sec].size || size>s[sec].size-(start-s[sec].addr)) die("range outside executable section");
            if(!size && kind!='E') { if(kind=='I') die("empty instruction range"); continue; }
            if(kind=='E' && size) die("nonempty section-end marker");
            if(nr==cap) { cap=cap?cap*2:1024; ranges=realloc(ranges,cap*sizeof *ranges); if(!ranges) die("out of memory"); }
            ranges[nr++]=(Range){sec,start-s[sec].addr,size,kind};
        }
    }
    if(!profiles || (!want_linked && !component_profile && profiles!=1)) die("missing or repeated object profile identity");
    if(symtabs!=1) die("requires one retained symbol table");
    if(nr>1) qsort(ranges,nr,sizeof *ranges,order);
    unsigned char **coverage=calloc(count,sizeof *coverage);
    if(!coverage) die("out of memory");
    for(unsigned i=1;i<count;i++) if(s[i].flags&4) {
        if(s[i].size>LIMIT) die("oversized executable section");
        coverage[i]=calloc(s[i].size?s[i].size:1,1);
        if(!coverage[i]) die("out of memory");
    }
    for(size_t r=0;r<nr;r++) {
        Range *v=ranges+r; if(v->kind=='E') continue;
        const unsigned char *bytes=p+s[v->sec].off+v->start;
        for(uint32_t j=v->start;j<v->start+v->size;j++) {
            if(coverage[v->sec][j]) die("overlapping instruction/data ranges");
            coverage[v->sec][j]=(unsigned char)v->kind;
        }
        if(v->kind=='I') {
            if((s[v->sec].addr+v->start)&1) die("unaligned instruction");
            instruction(bytes,v->size);
        } else if(v->kind=='P') padding(bytes,v->size);
        else data_bytes+=v->size;
    }
    int entry_ok=!want_linked;
    for(unsigned i=1;i<count;i++) if(s[i].flags&4) {
        for(uint32_t j=0;j<s[i].size;) {
            if(coverage[i][j]) { j++; continue; }
            uint32_t start=j;
            while(j<s[i].size && !coverage[i][j]) j++;
            /* as rounds section ends up; ld also aligns collected sections.
               A retained source section-end marker must precede the gap. */
            int end_marker=0;
            for(size_t r=0;r<nr;r++) if(ranges[r].sec==i && ranges[r].kind=='E' && ranges[r].start==start) end_marker=1;
            if(!end_marker || j-start>7) die("unclassified executable bytes (missing ranges)");
            padding(p+s[i].off+start,j-start);
        }
        for(size_t r=0;r<nr;r++) if(ranges[r].sec==i && ranges[r].kind=='I' && s[i].addr+ranges[r].start==u32(p+24)) entry_ok=1;
    }
    if(!entry_ok) die("entry is not a classified instruction boundary");
    /* Relocations may address inline literals/tables, never rewrite an
       instruction or its classification. The exporter checks S+A details. */
    for(unsigned i=1;i<count;i++) if(s[i].type==4 || s[i].type==9) {
        if(s[i].type!=4 || s[i].ent!=12 || s[i].size%12 || s[i].info>=count || s[i].link>=count || s[s[i].link].type!=2) die("invalid/unsupported relocation section");
        Sec *target=&s[s[i].info];
        if(!(target->flags&2)) continue;
        for(uint32_t j=0;j<s[i].size;j+=12) {
            const unsigned char *rel=p+s[i].off+j; uint32_t off=u32(rel), info=u32(rel+4);
            if((info&255)!=4 || (info>>8)>=s[s[i].link].size/16 || off<target->addr || target->size<4 || off-target->addr>target->size-4) die("unsupported/out-of-range relocation");
            if(target->flags&4) for(unsigned k=0;k<4;k++)
                if(coverage[s[i].info][off-target->addr+k]!='D') die("relocation must be in classified data");
        }
    }
    for(unsigned i=0;i<count;i++) free(coverage[i]);
    free(coverage); free(ranges); free(s);
    if(want_linked) images++; else objects++;
}
static void archive(const unsigned char *p,size_t n) {
    size_t off=8; unsigned before=members;
    while(off<n) {
        if(n-off<60 || memcmp(p+off+58,"`\n",2)) die("invalid archive member header");
        char sizebuf[11]; memcpy(sizebuf,p+off+48,10); sizebuf[10]=0;
        char *end; unsigned long size=strtoul(sizebuf,&end,10);
        while(*end==' ') end++;
        if(*end || size>n-off-60) die("invalid archive member size");
        const unsigned char *body=p+off+60; size_t bytes=size;
        /* GNU symbol/string indexes only; thin/BSD archives are excluded. */
        int index=(p[off]=='/' && (p[off+1]==' ' || p[off+1]=='/'));
        if(!index) { elf(body,bytes,0); members++; }
        off+=60+size+(size&1);
        if(off>n) die("invalid archive padding");
    }
    if(members==before) die("empty archive");
}
static int in_words(const char *word,const char *words) {
    char pattern[80];
    if(strlen(word)>70) return 0;
    snprintf(pattern,sizeof pattern," %s ",word);
    return strstr(words,pattern)!=NULL;
}
static void annotate(const char *in,const char *out) {
    size_t n; unsigned char *raw=read_file(in,&n);
    if(memchr(raw,0,n)) die("NUL in assembly");
    char *text=malloc(n+1); if(!text) die("out of memory");
    memcpy(text,raw,n); text[n]=0; free(raw);
    if(strstr(text,TAG) || strstr(text,".lab.profile")) die("reserved annotation name in input");
    char *inputs[1]={(char *)in}; clear_output(out,1,inputs);
    size_t length=strlen(out)+20; char *tmp=malloc(length); if(!tmp) die("out of memory");
    snprintf(tmp,length,"%s.XXXXXX",out); int fd=mkstemp(tmp); FILE *f=fd<0?NULL:fdopen(fd,"w");
    if(!f) die("cannot create annotated assembly");
    unsigned index=0; int executable=1;
    char *save,*line=strtok_r(text,"\n",&save);
    for(;line;line=strtok_r(NULL,"\n",&save)) {
        /* Reject statement separators outside strings, including hidden
           instructions after a directive. No macros/conditionals/.insn. */
        int quote=0,escape=0;
        for(char *c=line;*c;c++) {
            if(escape) { escape=0; continue; }
            if(*c=='\\' && quote) { escape=1; continue; }
            if(*c=='"') quote=!quote;
            if(*c=='#' && !quote) break;
            if(*c==';' && !quote) die("multiple assembly statements unsupported");
        }
        char *at=line; while(isspace((unsigned char)*at)) at++;
        char *end=at; while(*end && !isspace((unsigned char)*end) && *end!=':') end++;
        if(*end==':') { fwrite(line,1,(size_t)(end-line+1),f); fputc('\n',f); at=end+1; while(isspace((unsigned char)*at)) at++; }
        if(!*at || *at=='#') { fprintf(f,"%s\n",at); continue; }
        char op[80]; size_t k=0; while(at[k] && !isspace((unsigned char)at[k]) && k<sizeof(op)-1) {op[k]=at[k];k++;} op[k]=0;
        if(!strcmp(op,".text")) executable=1;
        else if(!strcmp(op,".data") || !strcmp(op,".bss")) executable=0;
        else if(!strcmp(op,".section")) {
            const char *name=at+strlen(op);
            while(isspace((unsigned char)*name)) name++;
            executable=!strncmp(name,".text",5) || strstr(at,",\"ax\"") || strstr(at,",\"xa\"");
        }
        if(!strcmp(op,".base64") && executable)
            die("base64 data directive in executable section");
        if(in_words(op," .text .data .bss .section ")) fprintf(f,TAG "E%u:\n",++index);
        char kind='I';
        if(*op=='.') {
            if(in_words(op," .align .balign .p2align ")) kind='P';
            else if(in_words(op," .quad .long .short .word .byte .string .ascii .base64 .zero .skip ")) kind='D';
            else if(in_words(op," .file .ident .text .data .bss .section .globl .type .size .local .comm .set .weak .hidden ")) kind=0;
            else die("unsupported assembly directive");
        }
        if(kind) fprintf(f,TAG "%c%u:\n",kind,++index);
        fprintf(f,"%s\n",at);
        if(kind) fprintf(f,".size " TAG "%c%u,.-" TAG "%c%u\n",kind,index,kind,index);
    }
    fprintf(f,TAG "E%u:\n",++index);
    fprintf(f,".section .lab.profile,\"\",@progbits\n.asciz \"%s\"\n",selected_id);
    if(fclose(f) || rename(tmp,out)) die("annotated assembly write failed");
    free(tmp); free(text);
}
static uint32_t address_hex(const char *s) {
    char *end; errno=0;
    if(!*s || *s=='-' || *s=='+') die("expected unsigned hexadecimal guest address");
    unsigned long v=strtoul(s,&end,16);
    if(errno || *end || v>=address_limit) die("guest address outside selected profile");
    return (uint32_t)v;
}
static void locate(const unsigned char *p,size_t n,const char *origin_text,const char *address_text) {
    uint32_t origin=address_hex(origin_text),address=address_hex(address_text);
    if(address<origin) die("guest address precedes image origin");
    uint32_t offset=address-origin, best=0; const char *name=NULL,*section=NULL;
    unsigned count=u16(p+48); const unsigned char *tab=part(p,n,u32(p+32),count*40);
    const unsigned char *names=tab+40*u16(p+50);
    for(unsigned i=1;i<count;i++) {
        const unsigned char *s=tab+40*i; uint32_t start=u32(s+12),size=u32(s+20);
        if(u32(s+8)&2) {
            if(start+size>address_limit-origin) die("image does not fit at supplied origin");
            if(offset>=start && offset-start<size) section=string(p+u32(names+16),u32(names+20),u32(s));
        }
        if(u32(s+4)!=2) continue;
        const unsigned char *str=tab+40*u32(s+24);
        for(uint32_t j=16;j<u32(s+20);j+=16) {
            const unsigned char *sym=p+u32(s+16)+j;
            uint32_t value=u32(sym+4),length=u32(sym+8),index=u16(sym+14);
            if(!index || index>=count || !(u32(tab+40*index+8)&2) || value>offset) continue;
            const unsigned char *owner=tab+40*index;
            if(offset<u32(owner+12) || offset-u32(owner+12)>=u32(owner+20)) continue;
            const char *candidate=string(p+u32(str+16),u32(str+20),u32(sym));
            if(!*candidate || !strncmp(candidate,".L",2) || (sym[12]&15)==3) continue;
            if(length && offset-value>=length) continue;
            if(!name || value>best) { best=value; name=candidate; }
        }
    }
    if(!section) die("guest address is outside allocated sections");
    printf("S370 LOCATION: origin=%06x address=%06x image-offset=%06x section=%s symbol=%s+0x%x\n",
           origin,address,offset,section,name?name:"<no retained symbol>",name?offset-best:0);
}
int main(int argc,char **argv) {
    context=argc>2?argv[2]:"arguments";
    if(argc>=5 && (!strcmp(argv[1],"clear-set") || !strcmp(argv[1],"guard-set"))) {
        output_set(argc,argv,!strcmp(argv[1],"clear-set")); return 0;
    }
    if(argc>=4 && !strcmp(argv[1],"clear")) { clear_output(argv[2],argc-3,argv+3); return 0; }
    if(argc<4) die("missing profile/input");
    if(!strcmp(argv[2],KPROFILE)) { selected_id=KID; kernel_profile=1; }
    else if(!strcmp(argv[2],C31PROFILE)) { selected_id=C31ID; cms31_profile=1; address_limit=0x80000000U; }
    else if(!strcmp(argv[2],T31PROFILE)) { selected_id=T31ID; tso31_profile=1; address_limit=0x80000000U; }
    else if(!strcmp(argv[2],T24PROFILE)) { selected_id=T24ID; tso24_profile=1; }
    else if(strcmp(argv[2],PROFILE)) die("unknown profile");
    context=argv[3];
    if(!strcmp(argv[1],"locate") && argc==6) {
        size_t n; unsigned char *p=read_file(argv[3],&n);
        elf(p,n,1); locate(p,n,argv[4],argv[5]); free(p); return 0;
    }
    if(!strcmp(argv[1],"annotate") && argc==5) { annotate(argv[3],argv[4]); return 0; }
    if(!strcmp(argv[1],"component")) {
        if(!kernel_profile) die("component composition requires kernel profile");
        component_profile=1;
    }
    if(argc!=4 || (strcmp(argv[1],"input") && strcmp(argv[1],"image") && !component_profile)) die("usage: s370_check annotate|input|component|image PROFILE INPUT [OUTPUT]");
    size_t n; unsigned char *p=read_file(argv[3],&n);
    if(n>=8 && !memcmp(p,"!<arch>\n",8) && !strcmp(argv[1],"input")) archive(p,n);
    else elf(p,n,!strcmp(argv[1],"image"));
    free(p);
    printf("S370 ENCODED PASS: %s objects=%u members=%u images=%u instructions=%u inline-data=%u padding=%u\n",context,objects,members,images,instructions,data_bytes,padding_bytes);
    return 0;
}

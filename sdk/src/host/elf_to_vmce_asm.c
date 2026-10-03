/* Bounded relocatable ELF32 -> native CMS assembler component.
 * Mainframe Lab original implementation. GNU as owns instruction encoding;
 * ASSEMBLE owns native ESD/RLD generation; VMFLOAD owns final nucleus layout.
 * No linked application image or fixed nucleus addresses enter this route. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#define LIMIT (1024u*1024u)
#define KID "MLAB1;isa=s370-int1;addr=24;abi=s390-ilp32-r1-v1;chars=binary;svc=vmkernel1;profile=vmkernel"
typedef struct { uint32_t type,flags,off,size,link,info,align,ent,base; } Section;
typedef struct { uint32_t value,size; unsigned bind,type,index; const char *name; } Symbol;
typedef struct { uint32_t offset,value; const char *external; } Fix;
static unsigned char *file,*bytes;
static size_t length;
static Section *sections;
static Symbol *symbols;
static Fix *fixes;
static unsigned nsections,nsymbols,nfix;
static void fail(const char *s) { fprintf(stderr,"elf_to_vmce_asm: %s\n",s); exit(1); }
static uint32_t u16(const unsigned char *p) {return (uint32_t)p[0]<<8|p[1];}
static uint32_t u32(const unsigned char *p) {return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static const unsigned char *part(uint32_t off,uint32_t size) {
    if(off>length || size>length-off) fail("file range outside input");
    return file+off;
}
static const char *str(Section s,uint32_t off) {
    if(s.type!=3 || off>=s.size || !memchr(part(s.off+off,s.size-off),0,s.size-off)) fail("invalid string");
    return (const char *)file+s.off+off;
}
static int native_name(const char *s) {
    size_t n=strlen(s);
    if(!n || n>8 || !(s[0]>='A' && s[0]<='Z')) return 0;
    for(size_t i=1;i<n;i++) if(!((s[i]>='A' && s[i]<='Z') || isdigit((unsigned char)s[i]))) return 0;
    return 1;
}
static int compare(const void *a,const void *b) {
    const Fix *x=a,*y=b; return (x->offset>y->offset)-(x->offset<y->offset);
}
int main(int argc,char **argv) {
    int resident=argc==5 && !strcmp(argv[4],"--resident-data");
    if((argc!=4 && !resident) || !native_name(argv[2])) fail("usage: elf_to_vmce_asm INPUT CSECT OUTPUT [--resident-data] (native uppercase names, <=8)");
    struct stat in,out;
    if(stat(argv[1],&in) || in.st_size<52 || in.st_size>16*LIMIT) fail("invalid input size");
    if(!stat(argv[3],&out) && in.st_dev==out.st_dev && in.st_ino==out.st_ino) fail("input/output alias");
    /* Remove a stale product only after checking aliases. Never publish partial output. */
    if(unlink(argv[3]) && access(argv[3],F_OK)==0) fail("cannot clear output");
    length=(size_t)in.st_size; file=malloc(length); bytes=calloc(LIMIT,1);
    if(!file || !bytes) fail("allocation failed");
    FILE *f=fopen(argv[1],"rb");
    if(!f || fread(file,1,length,f)!=length || fclose(f)) fail("read failed");
    if(memcmp(file,"\177ELF\1\2\1",7) || file[7] || file[8] || u16(file+16)!=1 ||
       u16(file+18)!=22 || u32(file+20)!=1 || u32(file+36) || u16(file+40)!=52 ||
       u16(file+46)!=40) fail("requires relocatable System V ELF32 big-endian EM_S390");
    nsections=u16(file+48); unsigned names=u16(file+50), symtab=0;
    if(!nsections || nsections>4096 || names>=nsections) fail("invalid section count");
    const unsigned char *table=part(u32(file+32),40*nsections);
    sections=calloc(nsections,sizeof *sections); if(!sections) fail("allocation failed");
    uint32_t size=0;
    for(unsigned i=0;i<nsections;i++) {
        const unsigned char *h=table+40*i;
        Section *s=&sections[i];
        *s=(Section){u32(h+4),u32(h+8),u32(h+16),u32(h+20),u32(h+24),u32(h+28),u32(h+32),u32(h+36),0};
        if(s->type!=8) part(s->off,s->size);
        if(s->type==2) {if(symtab) fail("multiple symbol tables");symtab=i;}
        if(!(s->flags&2)) continue;
        if((s->flags&~7u) || (s->type!=1 && s->type!=8) || u32(h+12)) fail("unsupported allocated section");
        if((s->flags&1) && s->size && !resident) fail("initial kernel component forbids writable static storage");
        if((s->flags&5)==5) fail("writable executable sections are unsupported");
        uint32_t a=s->align?s->align:1;
        if(a>8 || (a&(a-1))) fail("unsupported alignment");
        size=(size+a-1)&~(a-1); s->base=size;
        if(s->size>LIMIT-size) fail("component exceeds 1 MiB");
        if(s->type!=8) memcpy(bytes+size,part(s->off,s->size),s->size);
        size+=s->size;
    }
    unsigned identity=0;
    for(unsigned i=1;i<nsections;i++) {
        const char *name=str(sections[names],u32(table+40*i));
        if(!strcmp(name,".lab.profile")) {
            Section s=sections[i];
            if(s.type!=1 || s.flags || !s.size || s.size%sizeof(KID) || (!resident && s.size!=sizeof(KID))) fail("wrong kernel profile");
            for(uint32_t at=0;at<s.size;at+=sizeof(KID))
                if(memcmp(part(s.off+at,sizeof(KID)),KID,sizeof(KID))) fail("wrong kernel profile");
            identity++;
        }
    }
    if(identity!=1 || !symtab || !size) fail("missing profile, symbols or code");
    Section sy=sections[symtab];
    if(sy.ent!=16 || sy.size%16 || sy.link>=nsections) fail("invalid symbol table");
    nsymbols=sy.size/16; symbols=calloc(nsymbols,sizeof *symbols); if(!symbols) fail("allocation failed");
    for(unsigned i=0;i<nsymbols;i++) {
        const unsigned char *s=part(sy.off+i*16,16);
        Symbol *t=&symbols[i];
        *t=(Symbol){u32(s+4),u32(s+8),s[12]>>4,s[12]&15,u16(s+14),str(sections[sy.link],u32(s))};
        if(t->index && t->index<nsections && (t->value>sections[t->index].size ||
           t->size>sections[t->index].size-t->value)) fail("symbol outside section");
        if(t->bind && t->type!=4) {
            if(t->bind!=1 || (s[13]&3) || !native_name(t->name) || !strcmp(t->name,argv[2])) fail("unsupported external name/binding or CSECT collision");
            for(unsigned j=1;j<i;j++) if(symbols[j].bind && !strcmp(t->name,symbols[j].name)) fail("external name collision");
            if(t->index && (t->index>=nsections || !(sections[t->index].flags&2))) fail("unsupported external definition");
        }
    }
    fixes=calloc(size/4+1,sizeof *fixes); if(!fixes) fail("allocation failed");
    for(unsigned i=1;i<nsections;i++) {
        Section s=sections[i];
        if(s.type!=4 && s.type!=9) continue;
        if(s.info>=nsections) fail("relocation target invalid");
        Section dst=sections[s.info]; if(!(dst.flags&2)) continue;
        if(s.type!=4 || s.ent!=12 || s.size%12 || s.link!=symtab) fail("requires RELA linked to symbol table");
        for(uint32_t j=0;j<s.size;j+=12) {
            const unsigned char *r=part(s.off+j,12);
            uint32_t at=u32(r),info=u32(r+4); int32_t add=(int32_t)u32(r+8);
            if((info&255)!=4 || (info>>8)>=nsymbols || dst.size<4 || at>dst.size-4 || ((dst.base+at)&3) || nfix>=size/4) fail("unsupported relocation kind/range/alignment");
            if(u32(bytes+dst.base+at)) fail("RELA place must contain zero");
            Symbol t=symbols[info>>8]; Fix fix={dst.base+at,0,NULL};
            if(!t.index) {
                if(t.bind!=1 || !native_name(t.name) || add<0 || add>0xffffff) fail("invalid native external reference/addend");
                fix.external=t.name;fix.value=(uint32_t)add;
            } else {
                if(t.index>=nsections || !(sections[t.index].flags&2)) fail("relocation to unsupported symbol");
                int64_t value=(int64_t)sections[t.index].base+t.value+add;
                if(value<0 || value>size) fail("relocation outside component");
                fix.value=(uint32_t)value;
            }
            fixes[nfix++]=fix;
        }
    }
    qsort(fixes,nfix,sizeof *fixes,compare);
    for(unsigned i=1;i<nfix;i++) if(fixes[i].offset<fixes[i-1].offset+4) fail("duplicate relocation");
    size_t nt=strlen(argv[3])+16; char *tmp=malloc(nt);if(!tmp) fail("allocation failed");
    snprintf(tmp,nt,"%s.work.XXXXXX",argv[3]);int fd=mkstemp(tmp);if(fd<0 || !(f=fdopen(fd,"w")))fail("output staging failed");
    fprintf(f,"* GENERATED RELOCATABLE COMPONENT; RETAIN SOURCE ELF FOR DIAGNOSIS\n%-8s CSECT\n",argv[2]);
    for(unsigned i=1;i<nsymbols;i++) if(symbols[i].bind==1 && symbols[i].type!=4) {
        Symbol t=symbols[i];
        if(!t.index) fprintf(f,"         EXTRN %s\n",t.name);
        else fprintf(f,"         ENTRY %s\n%-8s EQU   %s+%u\n",t.name,t.name,argv[2],sections[t.index].base+t.value);
    }
    unsigned next=0;
    for(uint32_t at=0;at<size;) {
        if(next<nfix && fixes[next].offset==at) {
            Fix fix=fixes[next++];fprintf(f,"         DC    A(%s+%u)\n",fix.external?fix.external:argv[2],fix.value);at+=4;
        } else {
            uint32_t n=size-at;if(n>20)n=20;
            if(next<nfix && n>fixes[next].offset-at)n=fixes[next].offset-at;
            fprintf(f,"         DC    X'");
            for(uint32_t j=0;j<n;j++)fprintf(f,"%02X",bytes[at+j]);
            fprintf(f,"'\n");at+=n;
        }
    }
    fprintf(f,"         END\n");
    if(fclose(f) || rename(tmp,argv[3]))fail("publish failed");
    printf("VMCE COMPONENT: bytes=%u relocations=%u symbols=%u CSECT=%s\n",size,nfix,nsymbols,argv[2]);
    free(tmp);free(fixes);free(symbols);free(sections);free(bytes);free(file);return 0;
}

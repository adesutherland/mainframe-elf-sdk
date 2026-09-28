/* Trusted-producer identity gate for the separate standard-z900 LP64 profile.
 * GNU target selection enforces the selected instruction baseline. This is
 * not the independent historical S/370 instruction decoder. */
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#define LIMIT (64U*1024U*1024U)
#define ID "MLAB1;isa=z900;addr=64;abi=s390x-lp64-v1;chars=utf8;svc=mvs64-bridge1;profile=tso-zos64-v1"
static const char *context;
static unsigned objects;
_Noreturn static void fail(const char *why) {
    fprintf(stderr,"tso64_check: %s: %s\n",context,why);exit(1);
}
/* Validate every alias before removing any product from an earlier attempt. */
static void clear_set(int argc,char **argv) {
    int split=2;struct stat a,b;
    while(split<argc && strcmp(argv[split],"--")) split++;
    if(split==2 || split+1>=argc) fail("outputs require protected inputs");
    for(int i=2;i<split;i++) {
        if(!*argv[i]) fail("empty output path");
        int exists=!stat(argv[i],&a);
        if(exists && !S_ISREG(a.st_mode)) fail("output is not a regular file");
        for(int j=i+1;j<argc;j++) {
            if(j==split) continue;
            if(!strcmp(argv[i],argv[j]) || (exists && !stat(argv[j],&b) &&
               a.st_dev==b.st_dev && a.st_ino==b.st_ino))
                fail("input/output or output/output alias");
        }
    }
    for(int i=2;i<split;i++)
        if(unlink(argv[i]) && errno!=ENOENT) fail("cannot clear stale output");
}
static uint32_t u16(const unsigned char *p) { return (uint32_t)p[0]<<8|p[1]; }
static uint32_t u32(const unsigned char *p) { return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3]; }
static uint64_t u64(const unsigned char *p) { return (uint64_t)u32(p)<<32|u32(p+4); }
static const unsigned char *part(const unsigned char *p,size_t n,uint64_t off,uint64_t count) {
    if(off>n || count>n-off) fail("file range outside input");
    return p+(size_t)off;
}
static const unsigned char *section(const unsigned char *p,size_t n,unsigned index) {
    unsigned count=u16(p+60);
    if(index>=count) fail("section index out of bounds");
    return part(p,n,u64(p+40)+(uint64_t)index*64,64);
}
static void elf(const unsigned char *p,size_t n,int image) {
    if(n<64 || memcmp(p,"\177ELF\2\2\1\0\0",9) || u16(p+16)!=(image?2U:1U) ||
       u16(p+18)!=22 || u32(p+20)!=1 || u32(p+48) || u16(p+52)!=64 ||
       u16(p+58)!=64) fail("requires big-endian S390 ELF64 of the requested kind");
    unsigned count=u16(p+60),names=u16(p+62),identities=0;
    if(!count || count>4096 || !names || names>=count || u64(p+40)<64) fail("invalid section table");
    part(p,n,u64(p+40),(uint64_t)count*64);
    const unsigned char *strings=section(p,n,names);
    uint64_t strings_size=u64(strings+32);
    if(u32(strings+4)!=3 || !strings_size) fail("missing section names");
    const unsigned char *names_data=part(p,n,u64(strings+24),strings_size);
    for(unsigned i=0;i<count;i++) {
        const unsigned char *s=section(p,n,i);
        uint32_t name=u32(s);
        if(name>=strings_size || !memchr(names_data+name,0,(size_t)strings_size-name)) fail("invalid section name");
        if(u32(s+4)!=8) part(p,n,u64(s+24),u64(s+32));
        if(strcmp((const char *)names_data+name,".lab.profile")) continue;
        if(u32(s+4)!=1 || u64(s+8)) fail("identity must be nonallocated PROGBITS");
        uint64_t bytes=u64(s+32);
        const unsigned char *id=part(p,n,u64(s+24),bytes);
        if(!bytes || bytes%sizeof(ID)) fail("invalid identity framing");
        for(uint64_t at=0;at<bytes;at+=sizeof(ID))
            if(memcmp(id+at,ID,sizeof(ID))) fail("foreign object identity");
        identities++;
    }
    if(identities!=1) fail("requires exactly one TSO64 identity section");
    objects++;
}
static void input(const unsigned char *p,size_t n,int image) {
    if(n<8 || memcmp(p,"!<arch>\n",8)) { elf(p,n,image);return; }
    if(image) fail("archive is not an executable image");
    size_t off=8;unsigned before=objects;
    while(off<n) {
        const unsigned char *h=part(p,n,off,60);
        if(memcmp(h+58,"`\n",2)) fail("bad archive member header");
        uint64_t bytes=0;int digits=0,spaces=0;
        for(unsigned i=48;i<58;i++) {
            if(h[i]==' ') { spaces=1;continue; }
            if(spaces || h[i]<'0' || h[i]>'9') fail("invalid archive member size");
            bytes=bytes*10+h[i]-'0';digits=1;
        }
        if(!digits || bytes>LIMIT) fail("oversized archive member");
        const unsigned char *member=part(p,n,off+60,bytes);
        int special=(h[0]=='/' && (h[1]==' ' || h[1]=='/' || !memcmp(h,"/SYM64/",7)));
        if(!special) {
            if(!memcmp(h,"#1/",3)) fail("BSD archive names are outside the GNU producer contract");
            elf(member,(size_t)bytes,0);
        }
        off+=60+(size_t)bytes;
        if(off&1) { if(off>=n || p[off]!='\n') fail("invalid archive padding");off++; }
    }
    if(objects==before) fail("empty object archive");
}
int main(int argc,char **argv) {
    context="output set";
    if(argc>=3 && !strcmp(argv[1],"clear-set")) {
        clear_set(argc,argv);return 0;
    }
    if(argc<3 || (strcmp(argv[1],"input") && strcmp(argv[1],"image") && strcmp(argv[1],"stamp"))) return 2;
    int stamp=!strcmp(argv[1],"stamp"),image=!strcmp(argv[1],"image");
    if((stamp && argc!=4) || (!stamp && argc!=3)) return 2;
    context=argv[2];struct stat st,out;
    if(stat(context,&st) || !S_ISREG(st.st_mode) || st.st_size<=0 || st.st_size>LIMIT) fail("invalid input file");
    size_t n=(size_t)st.st_size;unsigned char *p=malloc(n);
    if(!p) fail("allocation failed");
    FILE *f=fopen(context,"rb");
    if(!f || fread(p,1,n,f)!=n || fclose(f)) fail("input read failed");
    if(stamp) {
        if(!strcmp(context,argv[3]) || (!stat(argv[3],&out) && st.st_dev==out.st_dev && st.st_ino==out.st_ino)) fail("input/output alias");
        if(!lstat(argv[3],&out) && !S_ISREG(out.st_mode)) fail("output must be a regular file");
        if(memchr(p,0,n)) fail("assembly contains NUL");
        f=fopen(argv[3],"wb");
        if(!f || fwrite(p,1,n,f)!=n || fprintf(f,"\n.section .lab.profile,\"\",@progbits\n.asciz \"%s\"\n",ID)<0 || fclose(f)) fail("stamp write failed");
    } else {
        input(p,n,image);printf("TSO64 identity checked: %u object/image(s)\n",objects);
    }
    free(p);return 0;
}

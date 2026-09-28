/* Test-only mutations of known ELF fixtures; not a general ELF writer.
   The harness independently requires the checker to reject each mutation. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t get(const unsigned char *p) { return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3]; }
static unsigned get16(const unsigned char *p) { return p[0]*256U+p[1]; }
static void put(unsigned char *p,uint32_t x) { p[0]=x>>24;p[1]=x>>16;p[2]=x>>8;p[3]=x; }
int main(int argc,char **argv) {
    if(argc!=4) return 2;
    FILE *f=fopen(argv[1],"rb"); if(!f) return 2;
    fseek(f,0,SEEK_END);long length=ftell(f);rewind(f);
    if(length<52 || length>16000000) return 2;
    unsigned char *p=malloc((size_t)length);if(!p) return 2;
    if(fread(p,1,(size_t)length,f)!=(size_t)length || fclose(f))return 2;
    unsigned char *tab=p+get(p+32),*symbols=NULL,*strings=NULL,*insn=NULL,*wide=NULL,*data=NULL,*quad=NULL,*profile=NULL;
    unsigned count=get16(p+48),nsyms=0;
    for(unsigned i=1;i<count;i++) {
        unsigned char *s=tab+40*i;
        if(get(s+4)==2) { symbols=p+get(s+16); nsyms=get(s+20)/16; strings=p+get(tab+40*get(s+24)+16); }
        if(get(s+20)>=6 && !memcmp(p+get(s+16),"MLAB1;",6)) profile=p+get(s+16);
    }
    if(!symbols || !strings || !profile) return 2;
    for(unsigned i=1;i<nsyms;i++) {
        unsigned char *s=symbols+i*16; char *name=(char *)strings+get(s);
        if(!strncmp(name,".Llab_I",7)) {if(!insn) insn=s;if(!wide && get(s+8)==4) wide=s;}
        if(!strncmp(name,".Llab_D",7) && !data)data=s;
        if(!strncmp(name,".Llab_D",7) && get(s+8)==8 && !quad)quad=s;
    }
    if(!insn || !wide || !data)return 2;
    unsigned char *section=tab+40*get16(insn+14);
    unsigned char *bytes=p+get(section+16)+get(insn+4)-get(section+12);
    const char *mode=argv[3];
    if(!strcmp(mode,"opcode")) bytes[0]=0xb9;
    else if(!strcmp(mode,"odd-pair")) {bytes[0]=0x1c;bytes[1]=0x12;}
    else if(!strcmp(mode,"shift-field")) {unsigned char *s=tab+40*get16(wide+14);unsigned char *b=p+get(s+16)+get(wide+4)-get(s+12);b[0]=0x88;b[1]=0x21;}
    else if(!strcmp(mode,"length"))put(insn+8,1);
    else if(!strcmp(mode,"missing-range"))strings[get(insn)]='X';
    else if(!strcmp(mode,"overlap"))put(wide+4,get(insn+4));
    else if(!strcmp(mode,"entry-data"))put(p+24,get(data+4));
    else if(!strcmp(mode,"entry-quad-data")) { if(!quad)return 2; put(p+24,get(quad+4)); }
    else if(!strcmp(mode,"identity"))profile[4]='9';
    else if(!strcmp(mode,"isa"))profile[10]='9';
    else if(!strcmp(mode,"address"))profile[22]='3';
    else if(!strcmp(mode,"abi"))profile[30]='X';
    else if(!strcmp(mode,"short-header"))length=40;
    else if(!strcmp(mode,"bad-offset"))put(p+32,0xffffffffU);
    else if(!strcmp(mode,"executable-nobits"))put(section+4,8);
    else if(!strcmp(mode,"executable-unallocated"))put(section+8,4);
    else if(!strcmp(mode,"large-address"))put(section+12,0x1000000U);
    else if(!strcmp(mode,"short-range-tag"))strings[get(insn)+6]=0;
    else if(!strcmp(mode,"relocation-code")) {
        int found=0;
        for(unsigned i=1;i<count;i++) {
            unsigned char *s=tab+40*i;
            if(get(s+4)==4 && get(s+20)>=12 && get(s+28)==get16(insn+14)) {put(p+get(s+16),get(insn+4));found=1;break;}
        }
        if(!found)return 2;
    } else return 2;
    f=fopen(argv[2],"wb");if(!f)return 2;
    if(fwrite(p,1,(size_t)length,f)!=(size_t)length || fclose(f))return 2;
    free(p);return 0;
}

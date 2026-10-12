/* Host record-format controls; native PDS linkage is a separate guest gate. */
#define _POSIX_C_SOURCE 200809L
#undef NDEBUG
#include "tso/directory.h"
#include "cms/text-codec.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static void entry(unsigned char *p,const char *name,unsigned flags) {
    unsigned i;
    memset(p,0,12+(flags&31)*2); memset(p,0x40,8); p[11]=(unsigned char)flags;
    for(i=0;name[i];++i) p[i]=(unsigned char)lab_unicode_1047((unsigned char)name[i]);
}
int main(void) {
    char path[]="/tmp/sdk-pds-directory-XXXXXX",name[9]; unsigned char block[256]={0};
    int fd=mkstemp(path); FILE *file; void *dir;
    assert(fd>=0); file=fdopen(fd,"wb"); assert(file);
    block[1]=36; entry(block+2,"ABC",1); entry(block+16,"A$#@9",0x80);
    memset(block+28,0xff,8);
    assert(fwrite(block,1,256,file)==256 && !fclose(file));
    dir=lab_tso_directory_open(path); assert(dir);
    assert(lab_tso_directory_next(dir,name,9)==1 && !strcmp(name,"ABC"));
    assert(lab_tso_directory_next(dir,name,9)==1 && !strcmp(name,"A$#@9"));
    assert(lab_tso_directory_next(dir,name,9)==0 && lab_tso_directory_next(dir,name,9)==0);
    assert(!lab_tso_directory_close(dir));
    file=fopen(path,"wb"); assert(file); block[1]=13;
    assert(fwrite(block,1,256,file)==256 && !fclose(file));
    dir=lab_tso_directory_open(path); assert(dir);
    assert(lab_tso_directory_next(dir,name,9)==-1 && errno==EIO);
    assert(lab_tso_directory_next(dir,name,9)==-1 && !lab_tso_directory_close(dir));
    file=fopen(path,"wb"); assert(file);
    assert(fwrite(block,1,128,file)==128 && !fclose(file));
    dir=lab_tso_directory_open(path); assert(dir);
    assert(lab_tso_directory_next(dir,name,9)==-1 && !lab_tso_directory_close(dir));
    assert(!unlink(path)); puts("PDS DIRECTORY FORMAT PASS"); return 0;
}

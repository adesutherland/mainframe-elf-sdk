/* SPDX-License-Identifier: MIT
 * PDS directory records over the selected native binary record service.
 * Directory blocks are 256 bytes: big-endian used length, NAME[8], TTR[3],
 * flags/user-halfword-count, optional user bytes; eight FF bytes mark EOF.
 * This owns a stream and adds no cREXX logical name/root convention. */
#include "directory.h"
#include "text-codec.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct { FILE *stream; unsigned char block[256]; unsigned at,used; int done; } Directory;
void *lab_tso_directory_open(const char *path) {
    Directory *d;
    if(!path || !*path || strchr(path,'(')) { errno=EINVAL; return NULL; }
    d=(Directory *)calloc(1,sizeof(*d));
    if(!d) { errno=ENOMEM; return NULL; }
    d->stream=fopen(path,"rb");
    if(!d->stream) { free(d); return NULL; }
    return d;
}
int lab_tso_directory_next(void *opaque,char *name,size_t capacity) {
    static const unsigned char last[8]={255,255,255,255,255,255,255,255};
    Directory *d=(Directory *)opaque; unsigned n,extra;
    if(!d || !name || capacity<9) { errno=EINVAL; return -1; }
    if(d->done) return d->done>0 ? 0:-1;
    for(;;) {
        const unsigned char *entry;
        if(d->at==d->used) {
            size_t read=fread(d->block,1,256,d->stream);
            if(read!=256) { d->done=-1; errno=EIO; return -1; }
            d->used=(unsigned)d->block[0]*256+d->block[1]; d->at=2;
            if(d->used<2 || d->used>256) break;
            if(d->used==2) continue;
        }
        if(d->used-d->at<8) break;
        entry=d->block+d->at;
        if(!memcmp(entry,last,8)) { d->done=1; return 0; }
        if(d->used-d->at<12) break;
        extra=2U*(entry[11]&31U);
        if(12U+extra>d->used-d->at) break;
        d->at+=12U+extra;
        for(n=0;n<8 && entry[n]!=0x40;++n) {
            unsigned char bytes[2]; unsigned count=lab_1047_utf8(entry[n],bytes),c=bytes[0];
            if(count!=1 || !((c>='A' && c<='Z') || (c>='0' && c<='9') || c=='@' || c=='#' || c=='$')) break;
            name[n]=(char)c;
        }
        if(!n || (n<8 && entry[n]!=0x40)) break;
        for(extra=n;extra<8;++extra) if(entry[extra]!=0x40) break;
        if(extra<8) break;
        name[n]=0; return 1; /* Alias entries are preserved too. */
    }
    d->done=-1; errno=EIO; return -1;
}
int lab_tso_directory_close(void *opaque) {
    Directory *d=(Directory *)opaque; int rc;
    if(!d) { errno=EINVAL; return -1; }
    rc=fclose(d->stream); free(d); return rc;
}

/* Exercise actual DD-backed syscalls with independent native record mocks. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#define write batch_write
#define read batch_read
#define open batch_open
#define close batch_close
#define lseek batch_lseek
#define fstat batch_fstat
#define isatty batch_isatty
#include "../../libc/src/adapters/tso/io.c"
#undef write
#undef read
#undef open
#undef close
#undef lseek
#undef fstat
#undef isatty
struct _reent batch_reent;
static int saved_errno,opens,closes,reads,writes,terminal_calls,fail_open=-1,fail_write;
static unsigned char captured[3][64];static unsigned sizes[3];
int *__errno(void){return &saved_errno;}
ssize_t _write_r(struct _reent *r,int fd,const void *p,size_t n){(void)r;return _write(fd,p,n);}
ssize_t _read_r(struct _reent *r,int fd,void *p,size_t n){(void)r;return _read(fd,p,n);}
FILE *__real_fopen(const char *p,const char *m){return fopen(p,m);}
static int terminal(const char *p,unsigned n,unsigned flags){(void)p;(void)n;(void)flags;++terminal_calls;return 0;}
static int filecall(unsigned op,const void *p)
{
    static const unsigned char input[16]={0xc9,0x95,0x97,0xa4,0xa3,0x40,0x83,0x81,0x86,0x51,0x5a,0x40,0x40,0x40,0x40,0x40};
    if(op==0){const OpenArgs *a=p;int id=opens++;
        const unsigned char *want=(const unsigned char *[]) {
            (const unsigned char *)"\xe2\xe8\xe2\xc9\xd5\x40\x40\x40",
            (const unsigned char *)"\xe2\xe8\xe2\xd7\xd9\xc9\xd5\xe3",
            (const unsigned char *)"\xe2\xe8\xe2\xe3\xc5\xd9\xd4\x40"}[id];
        assert(id<3 && !memcmp(a->dd,want,8));assert(*a->mode==(id?1:0));
        if(id==fail_open)return -2008;
        *a->recfm=id?1:0;*a->lrecl=id?20:16;*a->blksize=id?24:16;return 100+id;}
    if(op==1){const IoArgs *a=p;assert((uintptr_t)a->handle==100);if(reads++)return -1;
        *a->record=(unsigned char *)input;*a->length=sizeof input;return 0;}
    if(op==2){const IoArgs *a=p;unsigned id=(unsigned)(uintptr_t)a->handle-100;assert(id==1||id==2);
        ++writes;if(fail_write)return 12;assert(*a->length<=64);sizes[id]=*a->length;memcpy(captured[id],*a->record,*a->length);return 0;}
    if(op==3){++closes;return 0;}assert(!"prebound standard streams must not allocate/free a DD");return -1;
}
static const LabTsoServices batch_services={.version=8,.putline=terminal,.filecall=filecall};
static const LabTsoServices foreground_services={.version=7,.putline=terminal,.filecall=filecall};
const LabTsoServices *lab_tso_services=&batch_services;
static void reset(void){opens=closes=reads=writes=terminal_calls=fail_write=0;fail_open=-1;memset(sizes,0,sizeof sizes);}
int main(void)
{
    unsigned char b[64];reset();assert(!lab_tso_io_start());mainframe_set_text_conversion(0);
    assert(_read(0,b,sizeof b)==18 && !memcmp(b,"Input caf\xc3\xa9!     \n",18));assert(_read(0,b,sizeof b)==0);
    assert(!batch_isatty(0)&&!batch_isatty(1)&&!batch_isatty(2));
    assert(_write(1,"A\xc3",2)==2);assert(_write(2,"ERR\n",4)==4);assert(writes==1);
    assert(_write(1,"\xa9\n",2)==2);assert(writes==2 && sizes[1]==6);
    assert(!memcmp(captured[1],"\0\6\0\0\xc1\x51",6));assert(sizes[2]==7 && !memcmp(captured[2],"\0\7\0\0\xc5\xd9\xd9",7));
    assert(!lab_tso_io_finish()&&opens==3&&closes==3&&!terminal_calls);
    reset();fail_open=2;assert(lab_tso_io_start()==-1);assert(!lab_tso_io_finish()&&opens==3&&closes==2&&!terminal_calls);
    reset();assert(!lab_tso_io_start());fail_write=1;assert(_write(1,"bad\n",4)==-1);assert(lab_tso_io_finish()==-1&&closes==3&&writes==1&&!terminal_calls);
    reset();lab_tso_services=&foreground_services;mainframe_set_text_conversion(1);assert(!lab_tso_io_start()&&batch_isatty(1));assert(_write(1,"old\n",4)==4&&terminal_calls==1);assert(!lab_tso_io_finish()&&!opens);
    puts("SDK batch standard streams, encoding, failures and foreground control PASS");return 0;
}

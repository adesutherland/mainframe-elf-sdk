/* Marshal LP64 service arguments to the pinned PDPCLIB ILP32 OS linkage.
   One synchronous call at a time; low scratch belongs to this invocation. */
#include <stdint.h>
#include <string.h>
#include "services.h"
#include "native-args.h"

static uint32_t low(const void *p) { return (uint32_t)(uintptr_t)p; }
static int native(unsigned op,const void *p)
{ return lab_tso_services->filecall(op,p); }
int lab_tso_filecall(unsigned op,const void *input)
{
    unsigned char *w=lab_tso_services->work;
    if(!w || (uintptr_t)w>=0x80000000UL ||
       lab_tso_services->work_size<65536 || !input) return -1;
    uint32_t *list=(uint32_t *)w;
    int32_t *values=(int32_t *)(w+64);
    unsigned char *dd=w+128,*dsn=w+144,*member=w+192,*record=w+512;
    memset(w,0,256);
    if(op==0) {
        const OpenArgs *a=input;
        memcpy(dd,a->dd,8);memcpy(member,a->member,8);
        values[0]=*a->mode;values[1]=*a->recfm;
        values[2]=*a->lrecl;values[3]=*a->blksize;
        list[0]=low(dd);list[1]=low(values);list[2]=low(values+1);
        list[3]=low(values+2);list[4]=low(values+3);
        list[5]=low(values+4);list[6]=low(member);
        int rc=native(0,list);
        *a->mode=values[0];*a->recfm=values[1];*a->lrecl=values[2];*a->blksize=values[3];
        *a->buffer=(void *)(uintptr_t)(uint32_t)values[4];
        return rc;
    }
    if(op==1 || op==2) {
        const IoArgs *a=input;
        if((uintptr_t)a->handle>=0x80000000UL) return -1;
        list[0]=low(a->handle);list[1]=low(values);list[2]=low(values+1);
        if(op==2) {
            if(*a->length>32760) return -1;
            values[0]=(int32_t)low(record);values[1]=(int32_t)*a->length;
            memcpy(record,*a->record,*a->length);
        }
        int rc=native(op,list);
        if(op==1 && rc==0) {
            if(values[0]<0 || values[1]<0 || values[1]>32760) return -1;
            *a->record=(unsigned char *)(uintptr_t)(uint32_t)values[0];
            *a->length=(unsigned)values[1];
        }
        return rc;
    }
    if(op==3) {
        void *handle=*(void *const *)input;
        if((uintptr_t)handle>=0x80000000UL) return -1;
        list[0]=low(handle);return native(3,list);
    }
    if(op==4) {
        const DynArgs *a=input;
        if(a->ddlen!=8 || !a->dsnlen || a->dsnlen>44) return -1;
        memcpy(dd,a->dd,8);memcpy(dsn,a->dsn,a->dsnlen);
        list[0]=8;list[1]=low(dd);list[2]=a->dsnlen;list[3]=low(dsn);
        int rc=native(4,list);if(!rc) memcpy(a->dd,dd,8);return rc;
    }
    if(op==5) { memcpy(dd,input,8);return native(5,dd); }
    return -1;
}

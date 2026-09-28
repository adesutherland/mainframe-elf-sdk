/* Direct bridge/ABI and command-lifecycle checks for both CMS profiles.
   RUN and ERROR deliberately leave a buffered FILE open for exit cleanup;
   VERIFY is a separate CMS invocation that checks the persisted bytes. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "adapter.h"
#if defined(__MAINFRAME_LAB_CMS20_ESA31__)
#include "storage31.h"
extern int lab_cms_storage_call(void *, unsigned, void *, unsigned long[2]);
#endif

extern int lab_probe_call(void (*)(void), const unsigned long[4], unsigned long[5]);
extern void lab_probe_clobber(void);
static int failures;
#define CHECK(x) do { if (!(x)) { ++failures; printf("CNL LINK FAIL %d\n", __LINE__); } } while (0)
static void ending(void) { puts("CNL LINK ATEXIT"); }

static void name(unsigned char *dst, const char *src)
{
    for (unsigned i=0;i<8;++i) dst[i]=(unsigned char)lab_ascii_to_ebcdic(src[i]);
}

static void checked_call(void (*fn)(void), const unsigned long args[4],
                         int status, unsigned mismatch)
{
    unsigned long got[5]={0,0,0,0,0};
    int rc=lab_probe_call(fn,args,got);
    printf("CNL LINK OBS %lu %08lx %08lx %08lx %08lx\n",
           got[0],got[1],got[2],got[3],got[4]);
    CHECK(rc==(int)mismatch && got[0]==mismatch);
    CHECK(got[1]==(unsigned long)status);
    CHECK(got[2]==got[3] && !(got[2]&7));
#if defined(__MAINFRAME_LAB_CMS20_ESA31__)
    CHECK((got[4]&0x80000000UL)!=0);
    CHECK((got[4]&0x7fffffffUL)>=0x01000000UL);
#else
    CHECK((got[4]&0x80000000UL)==0);
    CHECK((got[4]&0x00ffffffUL)<0x01000000UL);
#endif
}

static void bridges(void)
{
    LabCmsFile state={0};
    unsigned char id[18];
    unsigned long regs[2]={0,0}, args[4]={0,0,0,0};
    CHECK(lab_cms_name("CNLABSNT BIN A1",id)==0);
    memcpy(state.filename,id,18);
    name((unsigned char *)state.command,"STATE   ");
    args[0]=(unsigned long)&state;
    args[1]=(unsigned long)regs;
#if defined(__MAINFRAME_LAB_CMS20_ESA31__)
    CHECK((unsigned long)&state>=0x01000000UL && (unsigned long)regs>=0x01000000UL);
#else
    CHECK((unsigned long)&state<0x01000000UL && (unsigned long)regs<0x01000000UL);
#endif
    checked_call((void (*)(void))lab_cms_service,args,28,0);
    checked_call(lab_probe_clobber,args,0,1); /* negative control */
#if defined(__MAINFRAME_LAB_CMS20_ESA31__)
    {
        /* The AMODE-24 RENAME command needs CMSCALL COPY and a fence. */
        unsigned char plist[64], oldid[18], newid[18];
        FILE *f=fopen("CNLPROBE.BIN","wb");
        CHECK(f!=0);
        if (f) { CHECK(fwrite("P",1,1,f)==1); CHECK(fclose(f)==0); }
        CHECK(lab_cms_name("CNLPROBE BIN A1",oldid)==0);
        CHECK(lab_cms_name("CNLPRB2 BIN A1",newid)==0);
        memset(plist,0x40,56); memset(plist+56,0xff,8);
        name(plist,"RENAME  ");
        memcpy(plist+8,oldid,16); memcpy(plist+24,oldid+16,2);
        memcpy(plist+32,newid,16); memcpy(plist+48,newid+16,2);
        args[0]=(unsigned long)plist; args[1]=(unsigned long)regs;
        CHECK((unsigned long)plist>=0x01000000UL);
        checked_call((void (*)(void))lab_cms_svc204_copy,args,0,0);
        CHECK(remove("CNLPRB2.BIN")==0);
    }
    {
        /* Probe the storage SVC itself, then release the exact reservation. */
        struct {
            unsigned char command[8], subpool[8];
            unsigned bytes, reserved, address_register;
            unsigned char flags, flags2, subpool_code, reserved2;
        } st={ {0}, {0}, 4096,0,0,0x82,0,3,0 };
        name(st.command,"DMSFROSV"); name(st.subpool,"USER    ");
        regs[0]=regs[1]=0;
        args[0]=(unsigned long)&st; args[1]=4096;
        args[2]=0; args[3]=(unsigned long)regs;
        CHECK((unsigned long)&st>=0x01000000UL);
        checked_call((void (*)(void))lab_cms_storage_call,args,0,0);
        CHECK(regs[1]>=0x01000000UL && regs[1]<0x80000000UL);
        if (regs[1]) CHECK(lab_cms_release31((void *)regs[1],4096)==0);
        CHECK(lab_cms_release31(0,4096)==-1);
    }
#endif
}

int main(int argc,char **argv)
{
    static const char payload[]="CMS EXIT FLUSH";
    FILE *f;
    char bytes[sizeof payload]={0};
    CHECK(argc==2 && argv && argv[1]);
    if (argc!=2 || !argv || !argv[1]) return 16;
    CHECK(atexit(ending)==0);
    if (!strcmp(argv[1],"VERIFY")) {
        f=fopen("CNLFLUSH.BIN","rb");
        CHECK(f!=0);
        if (f) {
            CHECK(fread(bytes,1,sizeof payload-1,f)==sizeof payload-1);
            CHECK(!memcmp(bytes,payload,sizeof payload-1));
            CHECK(fclose(f)==0);
        }
        CHECK(remove("CNLFLUSH.BIN")==0);
        if (!failures) puts("CNL LINK VERIFY PASS");
        return failures?16:46;
    }
    if (strcmp(argv[1],"RUN") && strcmp(argv[1],"ERROR")) return 16;
    bridges();
    f=fopen("CNLFLUSH.BIN","wb");
    CHECK(f!=0);
    if (f) CHECK(fwrite(payload,1,sizeof payload-1,f)==sizeof payload-1);
    /* fclose is intentionally omitted: exit must flush, FINIS and free slots. */
    if (!failures) puts("CNL LINK PASS");
    return failures?16:(!strcmp(argv[1],"ERROR")?23:45);
}

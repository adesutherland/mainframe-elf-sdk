/* SPDX-License-Identifier: MIT
   Copyright (c) 2026 Adrian Sutherland
   CMS SUBCOM query and invocation, with a full four-word extended PLIST.
   Uses CMSCALL's 31-bit SVC 204 path, never tags a 31-bit R1 pointer. */
#include "../command-internal.h"
#include <stdlib.h>
#include <string.h>
#if defined(__MAINFRAME_LAB_CMS20_ESA31__) || defined(MF_COMMAND_HOST_TEST)
extern int mf_cms_command_call(void *, void *, unsigned long [2], unsigned int);
int mf_command_open(MfCommandSession *s, MfCommandResult *r)
{
    mf_command_result_init(r);
    if (!s || !r || s->active) return MF_COMMAND_INVALID;
    s->environment = 0; s->active = 1; r->service_rc = 0;
    return MF_COMMAND_OK;
}
int mf_command_close(MfCommandSession *s)
{
    if (!s || !s->active) return MF_COMMAND_INVALID;
    s->environment = 0; s->active = 0;
    return MF_COMMAND_OK;
}
int mf_command_query(MfCommandSession *s, const char *name, int *present,
                     MfCommandResult *r)
{
    struct { unsigned char verb[8], name[8]; unsigned long block, query;
             unsigned char fence[8]; } p;
    unsigned long regs[2];
    int status;
    mf_command_result_init(r);
    if (!s || !s->active || !r || !present) return MF_COMMAND_INVALID;
    status = mf_command_name(name, p.name);
    if (status) return status;
    /* EBCDIC SUBCOM, independent of the C execution character set. */
    { static const unsigned char verb[8] =
        {0xe2,0xe4,0xc2,0xc3,0xd6,0xd4,0x40,0x40};
      memcpy(p.verb, verb, 8); }
    p.block = 0; p.query = 0xffffffffUL; memset(p.fence, 0xff, 8);
    r->service_rc = mf_cms_command_call(&p, 0, regs, 0);
    if (r->service_rc != 0 && r->service_rc != 1) return MF_COMMAND_NATIVE_ERROR;
    *present = r->service_rc == 0;
    return MF_COMMAND_OK;
}
static int execute(MfCommandSession *s,const char *name,const char *text,
                    unsigned int length,MfCommandResult *r,int optional)
{
    struct { unsigned char name[8], fence[8]; } p;
    void *eplist[4];
    unsigned long regs[2];
    unsigned char *native;
    unsigned int bytes;
    int status,present=0;
    mf_command_result_init(r);
    if (!s || !s->active || !r || !length || length > MF_COMMAND_MAX_BYTES)
        return MF_COMMAND_INVALID;
    status = mf_command_name(name, p.name);
    if (status) return status;
    native = (unsigned char *)malloc(length);
    if (!native) return MF_COMMAND_NO_MEMORY;
    status = mf_command_text(text, length, native, &bytes);
    if(!status&&optional){
        status=mf_command_query(s,name,&present,r);
        if(!status&&!present)status=MF_COMMAND_UNSUPPORTED;
        if(!status)mf_command_result_init(r);
    }
    if (!status) {
        memset(p.fence, 0xff, 8);
        eplist[0] = p.name; eplist[1] = native;
        eplist[2] = native + bytes; eplist[3] = 0;
        r->service_rc = mf_cms_command_call(&p, eplist, regs, 2);
        r->command_rc = r->service_rc;
        r->command_rc_valid = 1;
    }
    free(native);
    return status;
}
int mf_command_execute(MfCommandSession *s,const char *name,const char *text,
                        unsigned int length,MfCommandResult *r)
{return execute(s,name,text,length,r,0);}
int mf_pdos_command_execute(MfCommandSession *s,const char *text,
                            unsigned int length,MfCommandResult *r)
{
    return execute(s,"PDOS",text,length,r,1);
}
#endif

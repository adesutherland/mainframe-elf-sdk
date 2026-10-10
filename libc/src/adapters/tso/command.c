/* SPDX-License-Identifier: MIT
   Copyright (c) 2026 Adrian Sutherland
   Borrows a TSO Rexx processor via IRXINIT FINDENVB; no lifetime mutation.
   Query IRXSUBCM. Generic C invokes TSO through IKJEFTSR, not the
   compiler-runtime-only IRXHST interface. Preserve original command text. */
#include "../command-internal.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#if defined(__MAINFRAME_LAB_TSO31__) || defined(MF_COMMAND_HOST_TEST)
#ifdef MF_COMMAND_HOST_TEST
extern int mf_tso_command_call(const unsigned char [8], unsigned long *,
                               unsigned long, int *);
#else
#include "services.h"
static int mf_tso_command_call(const unsigned char name[8], unsigned long *p,
                                unsigned long env, int *load_rc)
{
    if (!lab_tso_services ||
        lab_tso_services->version != LAB_TSO_COMMAND_SERVICE_VERSION ||
        !lab_tso_services->commandcall) { *load_rc = -1; return -1; }
    return lab_tso_services->commandcall(name, p, env, load_rc);
}
#endif
#define PTR(p) ((unsigned long)(p))
static void last(unsigned long *p)
{ *p |= 1UL << (sizeof(unsigned long) * CHAR_BIT - 1); }
static int call(const char *name, unsigned long *p, unsigned long env,
                MfCommandResult *r)
{
    unsigned char native[8];
    int load_rc = 0;
    mf_command_name(name, native);
    r->service_rc = mf_tso_command_call(native, p, env, &load_rc);
    r->module_rc = load_rc;
    return load_rc ? MF_COMMAND_NATIVE_ERROR : MF_COMMAND_OK;
}
int mf_command_open(MfCommandSession *s, MfCommandResult *r)
{
    unsigned char function[8], module[8];
    unsigned long zero = 0, env = 0, reason = 0, p[7];
    int status;
    mf_command_result_init(r);
    if (!s || !r || s->active) return MF_COMMAND_INVALID;
    mf_command_name("FINDENVB", function); memset(module, 0x40, 8);
    p[0]=PTR(function); p[1]=PTR(module); p[2]=PTR(&zero);
    p[3]=PTR(&zero); p[4]=PTR(&zero); p[5]=PTR(&env); p[6]=PTR(&reason);
    last(&p[6]);
    status = call("IRXINIT", p, 0, r);
    if (status) return status;
    if (r->service_rc == 28) return MF_COMMAND_NO_ENVIRONMENT;
    if ((r->service_rc != 0 && r->service_rc != 4) || !env)
        return MF_COMMAND_NATIVE_ERROR;
    s->environment = env; s->active = 1;
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
    unsigned char function[8], native[8], entry[32];
    unsigned long entry_ptr = PTR(entry), size = sizeof(entry), p[5];
    int status;
    mf_command_result_init(r);
    if (!s || !s->active || !r || !present) return MF_COMMAND_INVALID;
    status = mf_command_name(name, native);
    if (status) return status;
    mf_command_name("QUERY", function); memset(entry, 0, sizeof(entry));
    p[0]=PTR(function); p[1]=PTR(&entry_ptr); p[2]=PTR(&size);
    p[3]=PTR(native); p[4]=PTR(&s->environment); last(&p[4]);
    status = call("IRXSUBCM", p, s->environment, r);
    if (status) return status;
    if (r->service_rc != 0 && r->service_rc != 8) return MF_COMMAND_NATIVE_ERROR;
    *present = r->service_rc == 0;
    return MF_COMMAND_OK;
}
int mf_command_execute(MfCommandSession *s, const char *name, const char *text,
                       unsigned int length, MfCommandResult *r)
{
    unsigned char environment[8], tso[8], *native;
    unsigned int bytes;
    /* Unauthorized, unisolated, synchronous command; no abend dump request.
       Native facility contains command abends and reports their codes. */
    unsigned long flags = 0x00010001UL, cmd_len, p[6];
    int status;
    mf_command_result_init(r);
    if (!s || !s->active || !r || !length || length > MF_COMMAND_MAX_BYTES)
        return MF_COMMAND_INVALID;
    status = mf_command_name(name, environment);
    if (status) return status;
    mf_command_name("TSO", tso);
    if (memcmp(environment, tso, 8)) return MF_COMMAND_UNSUPPORTED;
    native = (unsigned char *)malloc(length);
    if (!native) return MF_COMMAND_NO_MEMORY;
    status = mf_command_text(text, length, native, &bytes);
    if (!status) {
        cmd_len=bytes;
        r->command_rc=-1; r->reason_rc=-1;
        p[0]=PTR(&flags); p[1]=PTR(native); p[2]=PTR(&cmd_len);
        p[3]=PTR(&r->command_rc); p[4]=PTR(&r->reason_rc);
        p[5]=PTR(&r->abend_code); last(&p[5]);
        status = call("IKJEFTSR", p, 0, r);
        if (!status) {
            if (r->service_rc == 0 || r->service_rc == 4)
                r->command_rc_valid = 1;
            else status = MF_COMMAND_NATIVE_ERROR;
        }
    }
    free(native);
    return status;
}
#endif

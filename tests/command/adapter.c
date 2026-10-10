/* SPDX-License-Identifier: MIT
   Actual adapter C with deterministic native-boundary stubs. Guest ABI,
   native linkage and real output are qualified separately. */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "command.h"
#include "command-internal.h"
static unsigned int calls;
static int missing, command_failure;
#ifdef TEST_TSO
static int facility_error, load_failure;
static int init_rc;
static void *parameter(unsigned long *p, unsigned int index)
{
    return (void *)(p[index] & ~(1UL << (sizeof(unsigned long)*CHAR_BIT-1)));
}
int mf_tso_command_call(const unsigned char name[8], unsigned long *p,
                        unsigned long environment, int *load_rc)
{
    unsigned char expected[8];
    ++calls; *load_rc=load_failure;
    if (load_failure) return 8;
    mf_command_name("IRXINIT", expected);
    if (!memcmp(name, expected, 8)) {
        assert(environment == 0);
        mf_command_name("FINDENVB", expected);
        assert(!memcmp(parameter(p,0), expected, 8));
        assert(*(unsigned long *)parameter(p,2) == 0);
        *(unsigned long *)parameter(p,5) = 0x12345678UL;
        return init_rc;
    }
    mf_command_name("IRXSUBCM", expected);
    if (!memcmp(name, expected, 8)) {
        assert(environment == 0x12345678UL);
        mf_command_name("QUERY", expected);
        assert(!memcmp(parameter(p,0), expected, 8));
        assert(*(unsigned long *)parameter(p,2) == 32);
        return missing ? 8 : 0;
    }
    mf_command_name("IKJEFTSR", expected); assert(!memcmp(name, expected, 8));
    assert(environment == 0);
    assert(*(unsigned long *)parameter(p,0) == 0x00010001UL);
    assert(*(unsigned long *)parameter(p,2) == 13);
    { static const unsigned char text[13] =
        {0xe3,0xc9,0xd4,0xc5,0x40,0x40,0xc1,0x7f,0xc2,0x7f,0x40,0x4d,0x5d};
      assert(!memcmp(parameter(p,1), text, 13)); }
    *(int *)parameter(p,3)=command_failure ? -3 : 0;
    *(int *)parameter(p,4)=facility_error == 20 ? 44 : 0;
    *(unsigned int *)parameter(p,5)=facility_error == 12 ? 0xc4 : 0;
    return facility_error ? facility_error : (command_failure ? 4 : 0);
}
#else
int mf_cms_command_call(void *plist, void *extended, unsigned long regs[2],
                        unsigned int type)
{
    struct Query { unsigned char verb[8], name[8]; unsigned long block, query;
                   unsigned char fence[8]; };
    unsigned char expected[8];
    unsigned int i;
    ++calls; regs[0]=regs[1]=0;
    if (!type) {
        struct Query *q=(struct Query *)plist;
        assert(!extended);
        mf_command_name("SUBCOM",expected); assert(!memcmp(q->verb,expected,8));
        assert(q->block == 0 && q->query == 0xffffffffUL);
        for(i=0;i<8;++i) assert(q->fence[i] == 255);
        return missing ? 1 : 0;
    }
    assert(type == 2);
    { void **e=(void **)extended;
      static const unsigned char text[13] =
        {0xe3,0xc9,0xd4,0xc5,0x40,0x40,0xc1,0x7f,0xc2,0x7f,0x40,0x4d,0x5d};
      assert(e[0] == plist && !e[3]);
      assert((unsigned char *)e[2] - (unsigned char *)e[1] == 13);
      assert(!memcmp(e[1],text,13));
      for(i=8;i<16;++i) assert(((unsigned char *)plist)[i] == 255); }
    return command_failure ? -3 : 0;
}
#endif
int main(void)
{
    MfCommandSession s;
    MfCommandResult r;
    unsigned char name[8], text[40];
    unsigned int bytes=99, previous;
    int present=-1;
    memset(&s,0,sizeof(s));
    assert(mf_command_name("tso",name) == 0);
    assert(mf_command_name("toolonggg",name) == MF_COMMAND_INVALID);
    assert(mf_command_name("bad name",name) == MF_COMMAND_INVALID);
    assert(mf_command_text("a\xc3\xa9",3,text,&bytes) == 0 && bytes == 2);
    assert(mf_command_text("\xc3",1,text,&bytes) == MF_COMMAND_INVALID);
    assert(mf_command_text("\xe2\x82\xac",3,text,&bytes) == MF_COMMAND_INVALID);
    assert(mf_command_text("x\0y",3,text,&bytes) == MF_COMMAND_INVALID);
    assert(mf_command_open(&s,&r) == 0);
    previous=calls;
    assert(mf_command_open(&s,&r) == MF_COMMAND_INVALID && calls == previous);
    assert(mf_command_query(&s,"TSO",&present,&r) == 0 && present == 1);
    missing=1;
    assert(mf_command_query(&s,"ZZNOENV",&present,&r) == 0 && present == 0);
    missing=0;
    assert(mf_command_execute(&s,"TSO","TIME  A\"B\" ()",13,&r) == 0 &&
           r.command_rc_valid && r.command_rc == 0);
    command_failure=1;
    assert(mf_command_execute(&s,"TSO","TIME  A\"B\" ()",13,&r) == 0 &&
           r.command_rc_valid && r.command_rc == -3);
    command_failure=0;
#ifdef TEST_TSO
    facility_error=20;
    assert(mf_command_execute(&s,"TSO","TIME  A\"B\" ()",13,&r) == MF_COMMAND_NATIVE_ERROR &&
           !r.command_rc_valid && r.service_rc == 20 && r.reason_rc == 44);
    facility_error=12;
    assert(mf_command_execute(&s,"TSO","TIME  A\"B\" ()",13,&r) == MF_COMMAND_NATIVE_ERROR &&
           !r.command_rc_valid && r.service_rc == 12 && r.abend_code == 0xc4);
    facility_error=0; load_failure=1;
    assert(mf_command_execute(&s,"TSO","TIME  A\"B\" ()",13,&r) == MF_COMMAND_NATIVE_ERROR &&
           !r.command_rc_valid && r.module_rc == 1);
    load_failure=0;
    previous=calls;
    assert(mf_command_execute(&s,"MVS","TIME",4,&r) == MF_COMMAND_UNSUPPORTED);
    assert(calls == previous);
#endif
    previous=calls;
    assert(mf_command_execute(&s,"BAD NAME","TIME",4,&r) == MF_COMMAND_INVALID);
    assert(mf_command_execute(&s,"TSO","TIME",MF_COMMAND_MAX_BYTES+1,&r) == MF_COMMAND_INVALID);
    assert(mf_command_execute(&s,"TSO",0,4,&r) == MF_COMMAND_INVALID);
    assert(calls == previous);
    assert(mf_command_close(&s) == 0);
#ifdef TEST_TSO
    init_rc=4;
    assert(mf_command_open(&s,&r) == 0 && r.service_rc == 4);
    assert(mf_command_close(&s) == 0);
    init_rc=28;
    assert(mf_command_open(&s,&r) == MF_COMMAND_NO_ENVIRONMENT && !s.active);
    init_rc=20;
    assert(mf_command_open(&s,&r) == MF_COMMAND_NATIVE_ERROR && !s.active);
    init_rc=0;
    previous=calls;
#endif
    assert(mf_command_close(&s) == MF_COMMAND_INVALID);
    assert(mf_command_query(&s,"TSO",&present,&r) == MF_COMMAND_INVALID);
    assert(calls == previous);
    puts("COMMAND ADAPTER PASS");
    return 0;
}

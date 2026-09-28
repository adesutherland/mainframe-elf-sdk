#include "abi.h"
extern int lab_arithmetic_tests(void);
extern int lab_gcc_69709(void);
extern int lab_write_ascii_line(const char *,unsigned);
volatile unsigned lab_check_count;
static volatile int regression_failed;
void lab_regression_abort(void) { regression_failed=1; }
static int check(int truth) { ++lab_check_count; return !truth; }
int lab_application(void)
{
    int errors=0;
    struct pair p=pair_return(0x12345678U,0x80000000U);
    struct triple t={4,5,6};
    unsigned (*indirect)(unsigned,unsigned)=dispatch;
    lab_check_count=0; regression_failed=0;
    errors+=check(p.a==0x80000003U && p.b==0xb7910c22U);
    p.a=2; p.b=3;
    errors+=check(aggregate_args(1,p,7,t,8,9)==279);
    errors+=check(varargs_copy(8,1U,2U,3U,4U,5U,6U,7U,8U)==240);
    errors+=check(varargs_copy(0)==0);
    errors+=check(large_frame(17)==99);
    errors+=check(large_frame(0)==765);
    errors+=check(indirect(0,10)==91);
    errors+=check(indirect(1,10)==89);
    errors+=check(indirect(2,10)==99);
    errors+=check(indirect(3,10)==10);
    errors+=check(indirect(4,10)==0xdeadU);
    errors+=check(shifts(0x80000001U,0)==0);
    errors+=check(shifts(0x80000001U,1)==0x40000002U);
    errors+=check(shifts(0x80000001U,31)==0x80000001U);
    errors+=check(signed_edges(-2147483647-1,3)==0x80000000U);
    errors+=check(signed_edges(-17,5)==0xfffffff5U);
    errors+=check(signed_edges(17,-5)==0xfffffff9U);
    errors+=check(lab_arithmetic_tests()==0);
    errors+=check(lab_gcc_69709()==0 && regression_failed==0);
    /* Independent expected formula; generator emits 1300 literal constants. */
    { const unsigned indices[]={0,1,255,511,1023,1299};
      for(unsigned i=0;i<sizeof indices/sizeof indices[0];i++)
        errors+=check(generated_pool(indices[i])==0x81000000U+indices[i]*0x12345U); }
    errors+=check(generated_pool(1300)==0xdeadbeefU);
    errors+=check(sizeof(unsigned)==4 && sizeof(short)==2 && sizeof(char)==1);
    if(!errors && lab_write_ascii_line("PROFILE MATRIX PASS",19)) ++errors;
    return errors;
}

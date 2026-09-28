#include <stdarg.h>
#include "abi.h"
struct pair pair_return(unsigned a, unsigned b) { struct pair r={b+3,a^0xa5a55a5aU}; return r; }
unsigned aggregate_args(unsigned x,struct pair p,unsigned y,struct triple t,unsigned z,unsigned w)
{ return x+p.a*2+p.b*3+y*4+t.a*5+t.b*6+t.c*7+z*8+w*9; }
unsigned varargs_copy(unsigned n, ...)
{
    va_list a,b; unsigned sum=0,weighted=0;
    va_start(a,n); va_copy(b,a);
    for(unsigned i=0;i<n;i++) { sum+=va_arg(a,unsigned); weighted+=(i+1)*va_arg(b,unsigned); }
    va_end(a); va_end(b); return sum+weighted;
}
unsigned large_frame(unsigned seed)
{
    volatile unsigned char frame[6144];
    for(unsigned i=0;i<sizeof frame;i++) frame[i]=(unsigned char)(i+seed);
    return frame[0]+frame[255]+frame[256]+frame[4095]+frame[4096]+frame[6143];
}
unsigned shifts(unsigned x,unsigned n) { return (x<<n)^(x>>n); }
unsigned signed_edges(int a,int b) { return (unsigned)(a/b)*3U+(unsigned)(a%b); }

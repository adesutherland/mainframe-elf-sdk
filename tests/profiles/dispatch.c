/* Explicit GNU computed-goto extension; separate from the C99 baseline. */
__extension__ unsigned dispatch(unsigned pc,unsigned x)
{
    static void *labels[]={&&add,&&sub,&&xor_value,&&done};
    unsigned steps=0;
    if(pc>3) return 0xdeadU;
    goto *labels[pc];
add: x+=7; if(++steps==3) goto done; goto *labels[1];
sub: x-=3; if(++steps==3) goto done; goto *labels[2];
xor_value: x^=0x55U; if(++steps==3) goto done; goto *labels[0];
done: return x;
}

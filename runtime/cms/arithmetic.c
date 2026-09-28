/* Bounded compiler-support routines, original code. unsigned division is
   outside the old machine's DR instruction contract. No divide instruction
   or multiplication is used here; zero divisor traps explicitly. */
static unsigned divide(unsigned n, unsigned d, unsigned *rem)
{
    unsigned q = 0, r = 0;
    if (!d) {
        /* Deliberate historical operation exception, without a modern J. */
        __asm__ volatile (".short 0");
        __builtin_unreachable();
    }
    for (unsigned i = 0; i < 32; ++i) {
        unsigned carry = r >> 31;
        r = (r << 1) | (n >> 31);
        n <<= 1;
        q <<= 1;
        if (carry || r >= d) { r -= d; q |= 1; }
    }
    *rem = r;
    return q;
}
unsigned __udivsi3(unsigned n, unsigned d)
{
    unsigned r;
    return divide(n, d, &r);
}
unsigned __umodsi3(unsigned n, unsigned d)
{
    unsigned r;
    divide(n, d, &r);
    return r;
}

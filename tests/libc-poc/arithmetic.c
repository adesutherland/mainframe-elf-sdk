/* Independently tabulated 32-bit results, checked by Clang on macOS and
   by the experimental compiler on the guest. Volatile prevents folding. */
struct ArithmeticCase { unsigned n, d, q, r, product; };
static const struct ArithmeticCase cases[] = {
    {0x0U, 0x1U, 0x0U, 0x0U, 0x0U},
    {0x1U, 0x1U, 0x1U, 0x0U, 0x1U},
    {0xffffffffU, 0x1U, 0xffffffffU, 0x0U, 0xffffffffU},
    {0xffffffffU, 0x2U, 0x7fffffffU, 0x1U, 0xfffffffeU},
    {0xffffffffU, 0xaU, 0x19999999U, 0x5U, 0xfffffff6U},
    {0xffffffffU, 0x80000000U, 0x1U, 0x7fffffffU, 0x80000000U},
    {0x80000000U, 0x3U, 0x2aaaaaaaU, 0x2U, 0x80000000U},
    {0x80000000U, 0xffffffffU, 0x0U, 0x80000000U, 0x80000000U},
    {0xffffffffU, 0xffffffffU, 0x1U, 0x0U, 0x1U},
    {0x75bcd15U, 0x3039U, 0x2710U, 0x1a85U, 0xd9e499adU},
    {0x7fffffffU, 0x61U, 0x151d07eU, 0x41U, 0x7fffff9fU},
    {0x11U, 0x21U, 0x0U, 0x11U, 0x231U},
};
int lab_arithmetic_tests(void)
{
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        volatile unsigned n = cases[i].n, d = cases[i].d;
        if (n / d != cases[i].q || n % d != cases[i].r ||
            n * d != cases[i].product) return 1;
    }
    volatile unsigned char bytes[] = {0, 1, 0x80, 0xff};
    volatile unsigned short halves[] = {0, 1, 0x8000, 0xffff};
    for (unsigned i = 0; i < 4; ++i) {
        if ((bytes[i] == 0) != (i == 0)) return 2;
        if ((bytes[i] == 255) != (i == 3)) return 3;
        if ((halves[i] == 0) != (i == 0)) return 4;
        if ((halves[i] == 65535) != (i == 3)) return 5;
        if (((bytes[i] & 0xc0) == 0x80) != (i == 2)) return 6;
        if (((halves[i] & 0xc000) == 0x8000) != (i == 2)) return 7;
    }
    volatile int left = -17, right = 5;
    if (left / right != -3 || left % right != -2 || left * right != -85) return 8;
    return 0;
}
#ifdef LAB_HOST_REFERENCE
#include <stdio.h>
int main(void) {
    int rc = lab_arithmetic_tests();
    printf("Arithmetic reference %s\n", rc ? "FAIL" : "PASS");
    return rc;
}
#endif

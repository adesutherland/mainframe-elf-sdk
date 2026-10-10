/* SPDX-License-Identifier: MIT; deliberate child DAT fault on the PDOS profile.
 * Compatibility pages are at 0 and 0x10000; 0x2000 is explicitly unmapped.
 * Avoid a null store, which GCC can replace with the runtime abort hook. */
int main(void)
{
    volatile unsigned int *unmapped=(volatile unsigned int *)0x2000U;
    *unmapped=1U;
    return 99;
}

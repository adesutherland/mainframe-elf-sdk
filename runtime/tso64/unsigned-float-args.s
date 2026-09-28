/* GCC's S/390 conversion libcalls pass the SI operand sign-extended in R2.
 * A normal LP64 C function with an unsigned SI parameter assumes a zero-
 * extended argument. Normalize the low 32 bits before the generic fp-bit
 * implementation; this also accepts ordinary zero-extended C callers.
 * The original implementations are renamed only in their archive members.
 */
.section .text.__floatunsidf,"ax",@progbits
.align 8
.globl __floatunsidf
.type __floatunsidf,@function
__floatunsidf:
    llgfr %r2,%r2
    jg __lab_floatunsidf
.size __floatunsidf,.-__floatunsidf
.section .text.__floatunsisf,"ax",@progbits
.align 8
.globl __floatunsisf
.type __floatunsisf,@function
__floatunsisf:
    llgfr %r2,%r2
    jg __lab_floatunsisf
.size __floatunsisf,.-__floatunsisf
.section .note.GNU-stack,"",@progbits

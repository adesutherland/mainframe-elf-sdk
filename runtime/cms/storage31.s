# CMSSTOR SVC interface: r2=STPL, r3=bytes, r4=address, r5=results.
# Release STPL names r8; the bridge places the C address in that register.
# The CMSSTOR macro's documented service path uses CMSCALL UFLAGS E0.
 .text
 .balign 8
 .globl lab_cms_storage_call
 .type lab_cms_storage_call,@function
lab_cms_storage_call:
 stm %r6,%r15,24(%r15)
 lr %r10,%r15
 basr %r12,0
.Lbase:
 sh %r15,.Lframe-.Lbase(%r12)
 la %r13,96(%r15)
 xc 0(96,%r13),0(%r13)
 lr %r9,%r5
 lr %r8,%r4
 lr %r0,%r3
 lr %r1,%r2
 l %r15,.Lflags-.Lbase(%r12)
 svc 204
 stm %r0,%r1,0(%r9)
 lr %r2,%r15
 lm %r6,%r15,24(%r10)
 br %r14
 .balign 4
.Lflags: .long 0x00e00000
.Lframe: .short 256
 .size lab_cms_storage_call,.-lab_cms_storage_call
 .section .note.GNU-stack,"",@progbits

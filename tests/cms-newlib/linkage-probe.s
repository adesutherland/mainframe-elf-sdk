# Independent C ABI sentinel around a direct CMS bridge call. The argument
# vector supplies R2-R5; result words are mismatch count, R2 service status,
# stack before/after the call, and its return-link R14. Only R6-R13 are
# required to survive a C call. The deliberately broken control changes R6.
 .text
 .balign 8
 .globl lab_probe_call
 .type lab_probe_call,@function
lab_probe_call:
 stm %r6,%r15,24(%r15)
 lr %r10,%r15
 balr %r12,0
.Lentry_base:
 l %r0,.Lframe-.Lentry_base(%r12)
 sr %r15,%r0
 # The first 96 bytes belong to the callee's save area. Keep our state above it.
 st %r2,96(%r15)
 st %r3,100(%r15)
 st %r4,104(%r15)
 st %r10,108(%r15)
 st %r15,124(%r15)
 l %r6,.Ls6-.Lentry_base(%r12)
 l %r7,.Ls7-.Lentry_base(%r12)
 l %r8,.Ls8-.Lentry_base(%r12)
 l %r9,.Ls9-.Lentry_base(%r12)
 l %r10,.Ls10-.Lentry_base(%r12)
 l %r11,.Ls11-.Lentry_base(%r12)
 l %r13,.Ls13-.Lentry_base(%r12)
 l %r12,.Ls12-.Lentry_base(%r12)
 l %r1,100(%r15)
 lm %r2,%r5,0(%r1)
 l %r1,96(%r15)
 balr %r14,%r1
 st %r2,112(%r15)
 st %r14,116(%r15)
 balr %r1,0
.Lcheck_base:
 sr %r0,%r0
 c %r6,.Lc6-.Lcheck_base(%r1)
 bc 8,.Lgood6-.Lcheck_base(%r1)
 la %r0,1(%r0)
.Lgood6:
 c %r7,.Lc7-.Lcheck_base(%r1)
 bc 8,.Lgood7-.Lcheck_base(%r1)
 la %r0,1(%r0)
.Lgood7:
 c %r8,.Lc8-.Lcheck_base(%r1)
 bc 8,.Lgood8-.Lcheck_base(%r1)
 la %r0,1(%r0)
.Lgood8:
 c %r9,.Lc9-.Lcheck_base(%r1)
 bc 8,.Lgood9-.Lcheck_base(%r1)
 la %r0,1(%r0)
.Lgood9:
 c %r10,.Lc10-.Lcheck_base(%r1)
 bc 8,.Lgood10-.Lcheck_base(%r1)
 la %r0,1(%r0)
.Lgood10:
 c %r11,.Lc11-.Lcheck_base(%r1)
 bc 8,.Lgood11-.Lcheck_base(%r1)
 la %r0,1(%r0)
.Lgood11:
 c %r12,.Lc12-.Lcheck_base(%r1)
 bc 8,.Lgood12-.Lcheck_base(%r1)
 la %r0,1(%r0)
.Lgood12:
 c %r13,.Lc13-.Lcheck_base(%r1)
 bc 8,.Lgood13-.Lcheck_base(%r1)
 la %r0,1(%r0)
.Lgood13:
 l %r3,104(%r15)
 st %r0,0(%r3)
 l %r2,112(%r15)
 st %r2,4(%r3)
 l %r2,124(%r15)
 st %r2,8(%r3)
 st %r15,12(%r3)
 l %r2,116(%r15)
 st %r2,16(%r3)
 lr %r2,%r0
 l %r15,108(%r15)
 lm %r6,%r15,24(%r15)
 br %r14
 .balign 4
.Lframe: .long 256
.Ls6: .long 0x00610006
.Ls7: .long 0x00710007
.Ls8: .long 0x00810008
.Ls9: .long 0x00910009
.Ls10: .long 0x00a1000a
.Ls11: .long 0x00b1000b
.Ls12: .long 0x00c1000c
.Ls13: .long 0x00d1000d
.Lc6: .long 0x00610006
.Lc7: .long 0x00710007
.Lc8: .long 0x00810008
.Lc9: .long 0x00910009
.Lc10: .long 0x00a1000a
.Lc11: .long 0x00b1000b
.Lc12: .long 0x00c1000c
.Lc13: .long 0x00d1000d
 .size lab_probe_call,.-lab_probe_call

 .balign 8
 .globl lab_probe_clobber
 .type lab_probe_clobber,@function
lab_probe_clobber:
 balr %r1,0
.Lbad_base:
 l %r6,.Lbad_value-.Lbad_base(%r1)
 sr %r2,%r2
 br %r14
 .balign 4
.Lbad_value: .long 0x00000001
 .size lab_probe_clobber,.-lab_probe_clobber
 .section .note.GNU-stack,"",@progbits

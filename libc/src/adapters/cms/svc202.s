# Modern ELF C to CMS SVC 202. r2=parameter block, r3=two-word result.
# Stack/save-area discipline follows the qualified console bridge.
# CMS R15 status -> C r2; returned R0/R1 -> result[0..1].
 .text
 .balign 8
 .globl lab_cms_svc202
 .type lab_cms_svc202,@function
lab_cms_svc202:
 stm %r6,%r15,24(%r15)
 lr %r10,%r15
 balr %r12,0
.Lconsole_base:
 sh %r15,.Lframe_size-.Lconsole_base(%r12)
 la %r13,96(%r15)
 xc 0(72,%r13),0(%r13)
 lr %r9,%r3
 lr %r1,%r2
 # Align the inline SVC error-return fullword for R_390_32 export.
 .balign 4
 bcr 0,%r0
 svc 202
 .long .Lconsole_return
.Lconsole_return:
 stm %r0,%r1,0(%r9)
 lr %r2,%r15
 lm %r6,%r15,24(%r10)
 br %r14
.Lframe_size: .short 256
 .size lab_cms_svc202,.-lab_cms_svc202
 .section .note.GNU-stack,"",@progbits

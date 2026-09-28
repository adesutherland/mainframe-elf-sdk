# ELF C -> CMSCALL, AMODE 31. COPY=NO; require 31-bit-capable services.
# r2=parameter block, r3=returned r0/r1. Return CMS r15 in C r2.
# Own implementation of the CMS 20 SVC 204 interface, not macro source.
 .text
 .balign 8
 .globl lab_cms_svc204
 .type lab_cms_svc204,@function
lab_cms_svc204:
 stm %r6,%r15,24(%r15)
 lr %r10,%r15
 basr %r12,0
.Lbase:
 sh %r15,.Lframe-.Lbase(%r12)
 la %r13,96(%r15)
 xc 0(96,%r13),0(%r13)
 lr %r9,%r3
 lr %r1,%r2
 sr %r0,%r0
 sr %r15,%r15
 svc 204
 stm %r0,%r1,0(%r9)
 lr %r2,%r15
 lm %r6,%r15,24(%r10)
 br %r14
.Lframe: .short 256
 .size lab_cms_svc204,.-lab_cms_svc204

# CMSCALL COPY=YES,FENCE=YES for a tokenized plist passed to an AMODE 24
# command from high C. CMSCALL copies the complete list through its FF fence.
 .balign 8
 .globl lab_cms_svc204_copy
 .type lab_cms_svc204_copy,@function
lab_cms_svc204_copy:
 stm %r6,%r15,24(%r15)
 lr %r10,%r15
 basr %r12,0
.Lcopy_base:
 sh %r15,.Lcopy_frame-.Lcopy_base(%r12)
 la %r13,96(%r15)
 xc 0(96,%r13),0(%r13)
 lr %r9,%r3
 lr %r1,%r2
 sr %r0,%r0
 l %r15,.Lcopy_flags-.Lcopy_base(%r12)
 svc 204
 stm %r0,%r1,0(%r9)
 lr %r2,%r15
 lm %r6,%r15,24(%r10)
 br %r14
 .balign 4
.Lcopy_flags: .long 0x0000a000
.Lcopy_frame: .short 256
 .size lab_cms_svc204_copy,.-lab_cms_svc204_copy
 .section .note.GNU-stack,"",@progbits

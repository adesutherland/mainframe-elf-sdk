# GCC ELF to the pinned CMS FSTLKP nucleus function, not a Linux service.
# NUCON: ASYSREF at X'14'; AFSTLKP at SYSREF+12 is the SVCENT veneer.
# Direct VCFSTLKP entry requires the nucleus storage key; do not call it here.
# r2=PLIST, r3=two-word cursor/results, r4=resume. No other CMS calls may
# intervene in a scan: DMSLFS keeps its current position in the shared ADT.
 .text
 .balign 8
 .globl lab_cms_fst_lookup
 .type lab_cms_fst_lookup,@function
lab_cms_fst_lookup:
 stm %r6,%r15,24(%r15)
 lr %r10,%r15
 balr %r12,0
.Lfst_base:
 sh %r15,.Lfst_frame-.Lfst_base(%r12)
 la %r13,96(%r15)
 xc 0(72,%r13),0(%r13)
 lr %r9,%r3
 l %r0,0(%r9)
 lr %r1,%r2
 ltr %r4,%r4
 bc 8,.Lfst_first-.Lfst_base(%r12)
 o %r1,.Lfst_resume-.Lfst_base(%r12)
.Lfst_first:
 l %r15,20
 l %r15,12(%r15)
 balr %r14,%r15
 stm %r0,%r1,0(%r9)
 lr %r2,%r15
 lm %r6,%r15,24(%r10)
 br %r14
.Lfst_frame: .short 256
 .balign 4
.Lfst_resume: .long 0x80000000
 .size lab_cms_fst_lookup,.-lab_cms_fst_lookup
 .section .note.GNU-stack,"",@progbits

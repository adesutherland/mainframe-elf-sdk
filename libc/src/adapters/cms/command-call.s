# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Adrian Sutherland
# C32 r2=tokenized plist, r3=extended plist (or zero), r4=returned R0/R1,
# r5=CALLTYP. Independent implementation of CMSCALL/SVC204 interface.
# CMS20 receives call type in R15's high byte, EPLIST in bit X'00001000'.
# The supervisor publishes the called program's USERSAVE information.
 .text
 .balign 8
 .globl mf_cms_command_call
 .type mf_cms_command_call,@function
mf_cms_command_call:
 stm %r6,%r15,24(%r15)
 lr %r10,%r15
 basr %r12,0
.Lbase:
 sh %r15,.Lframe-.Lbase(%r12)
 la %r13,96(%r15)
 xc 0(104,%r13),0(%r13)
 lr %r9,%r4
 lr %r1,%r2
 lr %r0,%r3
 lr %r15,%r5
 sll %r15,24
 o %r15,.Lcopy-.Lbase(%r12)
 ltr %r0,%r0
 bz .Lnoeplist-.Lbase(%r12)
 o %r15,.Leplist-.Lbase(%r12)
.Lnoeplist:
 svc 204
 stm %r0,%r1,0(%r9)
 lr %r2,%r15
 lm %r6,%r15,24(%r10)
 br %r14
 .balign 4
.Lcopy: .long 0x0000a000
.Leplist: .long 0x00001000
.Lframe: .short 256
 .size mf_cms_command_call,.-mf_cms_command_call
 .section .note.GNU-stack,"",@progbits

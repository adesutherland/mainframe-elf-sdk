# Ordinary CMS command -> bounded ELF C application. Retain CMS's save area
# independently of the C call stack so exit() can return from any call depth.
# Static, single invocation at a time. MODULEs must be saved before execution.
 .section .text.entry,"ax",@progbits
 .balign 8
 .globl cms_entry
 .type cms_entry,@function
cms_entry:
 stm %r14,%r12,12(%r13)
 balr %r12,0
.Lbase:
 l %r3,.Lsave_pointer-.Lbase(%r12)
 st %r13,0(%r3)
 lr %r6,%r1
 l %r15,.Lstack-.Lbase(%r12)
 l %r2,.Lguard_low-.Lbase(%r12)
 mvc 0(4,%r2),.Lguard_value-.Lbase(%r12)
 l %r2,.Lguard_high-.Lbase(%r12)
 mvc 0(4,%r2),.Lguard_value-.Lbase(%r12)
 l %r2,.Lbottom-.Lbase(%r12)
 l %r3,.Lblocks-.Lbase(%r12)
.Lpaint:
 mvi 0(%r2),165
 mvc 1(255,%r2),0(%r2)
 la %r2,256(%r2)
 sh %r3,.Lone-.Lbase(%r12)
 bc 7,.Lpaint-.Lbase(%r12)
 # LA discards the CMS call-type byte in this historical 24-bit environment.
 la %r2,0(%r6)
 l %r1,.Lstart-.Lbase(%r12)
 balr %r14,%r1
 # lab_cms_start is nonreturning; a broken contract must not fall into data.
 .short 0
 .balign 4
.Lstart: .long lab_cms_start
.Lstack: .long lab_stack_caller
.Lbottom: .long lab_stack_bottom
.Lblocks: .long 256
.Lsave_pointer: .long lab_cms_saved
.Lguard_low: .long lab_stack_guard_low
.Lguard_high: .long lab_stack_guard_high
.Lguard_value: .long 0x5aa55aa5
.Lone: .short 1
 .size cms_entry,.-cms_entry

 .globl lab_cms_return
 .type lab_cms_return,@function
lab_cms_return:
 balr %r1,0
.Lreturn_base:
 l %r3,.Lscan_bottom-.Lreturn_base(%r1)
 l %r4,.Lscan_size-.Lreturn_base(%r1)
.Lscan:
 cli 0(%r3),165
 bc 7,.Lmeasured-.Lreturn_base(%r1)
 la %r3,1(%r3)
 sh %r4,.Lscan_one-.Lreturn_base(%r1)
 bc 7,.Lscan-.Lreturn_base(%r1)
.Lmeasured:
 l %r3,.Lcheck_low-.Lreturn_base(%r1)
 clc 0(4,%r3),.Lcheck_value-.Lreturn_base(%r1)
 bc 7,.Lguard_failed-.Lreturn_base(%r1)
 l %r3,.Lcheck_high-.Lreturn_base(%r1)
 clc 0(4,%r3),.Lcheck_value-.Lreturn_base(%r1)
 bc 8,.Lrecord-.Lreturn_base(%r1)
.Lguard_failed:
 la %r2,28
.Lrecord:
 l %r3,.Lmeasure-.Lreturn_base(%r1)
 st %r4,0(%r3)
 st %r2,4(%r3)
 l %r3,.Lsaved-.Lreturn_base(%r1)
 l %r13,0(%r3)
 lr %r15,%r2
 l %r14,12(%r13)
 lm %r0,%r12,20(%r13)
 br %r14
 .balign 4
.Lscan_bottom: .long lab_stack_bottom
.Lscan_size: .long 65536
.Lmeasure: .long lab_application_observed
.Lsaved: .long lab_cms_saved
.Lcheck_low: .long lab_stack_guard_low
.Lcheck_high: .long lab_stack_guard_high
.Lcheck_value: .long 0x5aa55aa5
.Lscan_one: .short 1
 .size lab_cms_return,.-lab_cms_return
 .section .data,"aw",@progbits
 .balign 8
 .globl lab_application_observed
lab_application_observed: .long 0,0,0,0,0
lab_cms_saved: .long 0
 .section .bss,"aw",@nobits
 .balign 8
lab_stack_guard_low: .skip 8
lab_stack_bottom: .skip 65536
lab_stack_caller: .skip 96
lab_stack_guard_high: .skip 8
 .section .note.GNU-stack,"",@progbits

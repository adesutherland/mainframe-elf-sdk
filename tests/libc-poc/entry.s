# CMS entry, explicit modern stack and save-area space, register/stack guards.
 .section .text.entry,"ax",@progbits
 .balign 8
 .globl cms_entry
 .type cms_entry,@function
cms_entry:
 stm %r14,%r12,12(%r13)
 balr %r12,0
.Lbase:
 st %r13,.Lcms_save-.Lbase(%r12)
 l %r15,.Lstack_pointer-.Lbase(%r12)
 la %r6,106
 la %r7,107
 la %r8,108
 la %r9,109
 la %r10,110
 la %r11,111
 stm %r6,%r13,.Lexpected_regs-.Lbase(%r12)
 l %r1,.Lguard_bottom-.Lbase(%r12)
 mvc 0(4,%r1),.Lguard_value-.Lbase(%r12)
 l %r1,.Lguard_top-.Lbase(%r12)
 mvc 0(4,%r1),.Lguard_value-.Lbase(%r12)
 l %r1,.Lapplication-.Lbase(%r12)
 balr %r14,%r1
 # Re-establish our base without trusting any callee-saved register.
 balr %r1,0
.Lresume:
 st %r2,.Ldetail-.Lresume(%r1)
 stm %r6,%r13,.Lobserved_regs-.Lresume(%r1)
 clc .Lobserved_regs-.Lresume(32,%r1),.Lexpected_regs-.Lresume(%r1)
 bc 7,.Lfail-.Lresume(%r1)
 c %r15,.Lstack_pointer-.Lresume(%r1)
 bc 7,.Lfail-.Lresume(%r1)
 ch %r2,.Lexpected_result-.Lresume(%r1)
 bc 7,.Lfail-.Lresume(%r1)
 l %r3,.Lguard_bottom-.Lresume(%r1)
 clc 0(4,%r3),.Lguard_value-.Lresume(%r1)
 bc 7,.Lfail-.Lresume(%r1)
 l %r3,.Lguard_top-.Lresume(%r1)
 clc 0(4,%r3),.Lguard_value-.Lresume(%r1)
 bc 7,.Lfail-.Lresume(%r1)
 l %r3,.Lcounter-.Lresume(%r1)
 l %r4,0(%r3)
 ch %r4,.Lexpected_checks-.Lresume(%r1)
 bc 7,.Lfail-.Lresume(%r1)
 la %r2,0
 b .Lreturn-.Lresume(%r1)
.Lfail:
 la %r2,1
.Lreturn:
 l %r3,.Lobserved-.Lresume(%r1)
 la %r4,0(%r1)
 sh %r4,.Lresume_offset-.Lresume(%r1)
 st %r4,0(%r3)
 st %r2,4(%r3)
 l %r13,.Lcms_save-.Lresume(%r1)
 ltr %r2,%r2
 lm %r14,%r12,12(%r13)
 la %r15,0
 bcr 8,%r14
 la %r15,1
 br %r14
 .balign 4
.Lapplication: .long lab_application
.Lstack_pointer: .long stack_caller
.Lguard_bottom: .long stack_guard_bottom
.Lguard_top: .long stack_guard_top
.Lcounter: .long lab_check_count
.Lobserved: .long observed_origin
 .globl observed_detail
observed_detail:
.Ldetail: .long 0
.Lguard_value: .long 0x5aa55aa5
.Lcms_save: .long 0
.Lexpected_regs: .skip 32
.Lobserved_regs: .skip 32
.Lresume_offset: .short .Lresume-cms_entry
.Lexpected_checks: .short 1
.Lexpected_result: .short 0
 .size cms_entry,.-cms_entry
 .section .data,"aw",@progbits
 .balign 8
 .globl observed_origin,observed_result
observed_origin: .long 0
observed_result: .long 0
 .section .bss,"aw",@nobits
 .balign 8
stack_guard_bottom: .skip 8
stack_bottom: .skip 16384
stack_caller: .skip 96
stack_guard_top: .skip 8
 .section .note.GNU-stack,"",@progbits

# Trusted control with instruction-looking inline data and a removable function.
.section .text.entry,"ax",@progbits
.balign 8
.globl cms_entry
.type cms_entry,@function
cms_entry:
 balr %r12,0
.Lbase:
 l %r2,.Lliteral-.Lbase(%r12)
 br %r14
.balign 4
.Lliteral: .long cms_entry
.Lmodern_data: .long 0xb9020000
.Lwide_data: .quad 0xb9020000c0000000
.size cms_entry,.-cms_entry
.section .text.unused,"ax",@progbits
.globl unused_function
.type unused_function,@function
unused_function:
 lr %r2,%r3
 br %r14
.size unused_function,.-unused_function
.section .note.GNU-stack,"",@progbits

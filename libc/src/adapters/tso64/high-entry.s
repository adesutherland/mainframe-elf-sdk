/* The high module's only entry. The low launcher supplies R2/R3/R4 and a
   return in R14; C starts with an independent stack inside this module.
   LARL and JG are relative, so neither embeds a below-bar address. */
.section .text.high-entry,"ax",@progbits
.globl lab_high_entry
.type lab_high_entry,@function
lab_high_entry:
    larl %r15,lab_high_stack_end
    jg lab_tso_start
.size lab_high_entry,.-lab_high_entry
.section .note.GNU-stack,"",@progbits

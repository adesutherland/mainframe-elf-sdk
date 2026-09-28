OUTPUT_FORMAT(elf64-s390)
OUTPUT_ARCH(s390:64-bit)
ENTRY(lab_high_entry)
SECTIONS
{
  . = 0;
  .text : { *(.text.high-entry) *(.text.entry) *(.text .text.*) }
  .rela.text.high-entry : { *(.rela.text.high-entry) }
  .rodata : ALIGN(8) { *(.rodata .rodata.*) }
  .data : ALIGN(8) { *(.data .data.*) }
  .bss : ALIGN(8) {
    *(.bss .bss.*) *(COMMON)
    . = ALIGN(16);
    lab_high_stack_start = .;
    . += 0x100000;
    /* lab_tso_start saves R6-R15 at 48(R15)..127(R15).  Keep its full
       160-byte initial caller area within the allocated stack image. */
    lab_high_stack_end = . - 160;
  }
  .iplt : { *(.iplt) }
  .rela.iplt : { *(.rela.iplt) }
  .igot.plt : { *(.igot.plt) }
  ASSERT(SIZEOF(.iplt) + SIZEOF(.rela.iplt) + SIZEOF(.igot.plt) == 0,
         "Indirect PLT is outside the static TSO profile")
  .lab.profile 0 : { KEEP(*(.lab.profile)) }
  /DISCARD/ : { *(.note.GNU-stack) *(.comment) }
}

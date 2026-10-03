OUTPUT_FORMAT(elf64-s390)
OUTPUT_ARCH(s390:64-bit)
ENTRY(lab_tso_start)
SECTIONS
{
  . = 0;
  .text : { *(.text.entry) *(.text .text.*) }
  .rodata : ALIGN(8) { *(.rodata .rodata.*) }
  .data : ALIGN(8) { *(.data .data.*) }
  .bss : ALIGN(8) { *(.bss .bss.*) *(COMMON) }
  .iplt : { *(.iplt) }
  .rela.iplt : { *(.rela.iplt) }
  .igot.plt : { *(.igot.plt) }
  ASSERT(SIZEOF(.iplt) + SIZEOF(.rela.iplt) + SIZEOF(.igot.plt) == 0,
         "Indirect PLT is outside the static TSO profile")
  .lab.profile 0 : { KEEP(*(.lab.profile)) }
  /DISCARD/ : { *(.note.GNU-stack) *(.comment) }
}

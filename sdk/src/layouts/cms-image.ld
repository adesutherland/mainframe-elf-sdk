OUTPUT_FORMAT(elf32-s390)
OUTPUT_ARCH(s390:31-bit)
ENTRY(cms_entry)
SECTIONS
{
  . = 0;
  .text : { *(.text.entry) *(.text .text.*) }
  .rodata : ALIGN(8) { *(.rodata .rodata.*) }
  .data : ALIGN(8) { *(.data .data.*) }
  .bss : ALIGN(8) { *(.bss .bss.*) *(COMMON) }
  .rela.rodata 0 : { *(.rela.rodata .rela.rodata.*) }
  .iplt : { *(.iplt) }
  .rela.iplt : { *(.rela.iplt) }
  .igot.plt : { *(.igot.plt) }
  ASSERT(SIZEOF(.iplt) + SIZEOF(.rela.iplt) + SIZEOF(.igot.plt) == 0,
         "Indirect PLT is outside the static CMS proof")
  .lab.profile 0 : { KEEP(*(.lab.profile)) }
  /DISCARD/ : { *(.note.GNU-stack) *(.comment) }
}

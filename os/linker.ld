OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)

SECTIONS {
  . = 0x100000;

  .text : {
    *(.text)
  }

  .rodata : {
    *(.rodata)
  }

  .data : {
    *(.data)
  }

  .bss : {
    *(.bss COMMON)
  }
}
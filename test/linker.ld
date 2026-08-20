
ENTRY(_start)

MEMORY
{
  RAM (rwx)       : ORIGIN = 0x0000, LENGTH = 8K
  MMIO (rw)       : ORIGIN = 0x8000, LENGTH = 1K /* mmio */
}

SECTIONS
{
  .text : {
    *(.text.start)
    *(.text*)
  } > RAM

  .rodata : {
    . = ALIGN(4);
    *(.rodata*)
    *(.srodata*)
  } > RAM

  .data : {
    . = ALIGN(4);
    *(.data*)
    *(.sdata*)
  } > RAM

  .bss  : {
    . = ALIGN(4);
    *(.bss*)
    *(.sbss*)
    *(COMMON) 
  } > RAM
  
  .stack : {
      . = ALIGN(4);
      __stack_bottom = .;
      . += 512;
      __stack_top = .;
  } > RAM
}


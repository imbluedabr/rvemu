
ENTRY(_start)

MEMORY
{
  RAM (rwx)       : ORIGIN = 0x0000, LENGTH = 4K
  MMIO (rw)       : ORIGIN = 0x8000, LENGTH = 1K /* mmio */
}

SECTIONS
{
  .text : {
    *(.text.start)
    *(.text*)
  } > RAM

  .rodata : {
    *(.rodata*)
    *(.srodata*)
  } > RAM

  .data : {
     *(.data*)
     *(.sdata*)
  } > RAM

  .bss  : {
    *(.bss*)
    *(.sbss*)
    *(COMMON) 
  } > RAM
  
  .stack : {
      . = ALIGN(4);
      __stack_bottom = .;
      . += 1K;
      __stack_top = .;
  } > RAM
}


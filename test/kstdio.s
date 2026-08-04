.section .text
.global kputs
.global kputchar

# void kputs(const char *);
    # Print string by accessing MMIO directly
kputs:
    lui t1, %hi(0x8000)
1:
    lb t0, 0(a0)
    beq t0, zero, 2f
    sb t0, 0(t1)
    addi a0, a0, 1
    j 1b
2:
    ret

    # void kputchar(char);
    # Print byte by accessing MMIO directly
kputchar:
    lui t1, %hi(0x8000)
    sb a0, (t1)
    ret



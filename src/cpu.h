#pragma once
#include <stdint.h>

#define MODE_M 0
#define MODE_U 1
#define MODE_D 2

#define FAULT_IACCESS 1
#define FAULT_ILLINSTR 2
#define FAULT_DEBUG 3
#define FAULT_LACCESS 5
#define FAULT_SACCESS 7
#define FAULT_UCALL 8
#define FAULT_MCALL 11

#define MSTATUS_MPRV(X) (((X) & 0x3) << 17)
#define MSTATUS_MPRV_MSK (0x3 << 17)
#define MSTATUS_GETMPRV(X) (((X) >> 17) & 0x3)
#define MSTATUS_MPP(X) (X << 11)
#define MSTATUS_MPIE(X) (X << 7)
#define MSTATUS_MIE(X) (X << 3)

#define MIE_FAST_IRQ(X) (((X) & 0xFFFF) << 16)
#define MIE_FAST_IRQ_MSK (0xFFFF << 16)
#define MIE_MEIE(X) (X << 11)
#define MIE_MTIE(X) (X << 7)
#define MIE_MSIE(X) (X << 3)

#define MCAUSE_INTR(X) ((X) << 31)
#define MCAUSE_CODE(X) ((X) & 0x1F)
#define MCAUSE_CODE_MSK 0x1F

#define MIP_FAST_IRQ(X) ((X) << 16)
#define MIP_FAST_IRQ_MSK (0xFFFF << 16)
#define MIP_MEIP(X) (X << 11)
#define MIP_MTIP(X) (X << 7)
#define MIP_MSIP(X) (X << 3)

#define DCSR_CAUSE(X) (((X) & 0x3) << 6)
#define DCSR_CAUSE_MSK (0x3 << 6)
#define DCSR_GETCAUSE(X) (((X) >> 6) & 0x3)
#define DCSR_EBREAK(X) (X << 15)
#define DCSR_VCATCH(X) (X << 16)

typedef struct CPU {
    uint32_t registers[32];
    uint32_t pc;
    uint8_t trap_pending;
    uint8_t mode;

    uint32_t csr_mstatus;
    uint32_t csr_misa;
    uint32_t csr_mie;
    uint32_t csr_mtvec;
    uint32_t csr_mstatush;
    uint32_t csr_mscratch;
    uint32_t csr_mepc;
    uint32_t csr_mcause;
    uint32_t csr_mtval;
    uint32_t csr_mip;
    uint32_t csr_dcsr;
    uint32_t csr_dpc;

    struct MainBus* system_bus;
} CPU;

void CPU_init(CPU* cpu);
void CPU_interrupt_fast(CPU* cpu, uint8_t irq);
void CPU_exception(CPU* cpu, uint8_t exception, uint32_t mtval);
void CPU_tick(CPU* cpu);


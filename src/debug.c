#include "debug.h"
#include "bus.h"
#include "mem.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void DebugModule_init(DebugModule* dbg, MainBus* systemBus) {
    dbg->systemBus = systemBus;
    dbg->running = 1;
}

void DebugModule_sendHalt(DebugModule* dbg, int cause) {
    CPU* cpu = dbg->systemBus->cpu;
    cpu->mode = MODE_D;
    cpu->csr_dpc = cpu->pc;
    cpu->csr_dcsr &= DCSR_CAUSE_MSK;
    cpu->csr_dcsr |= DCSR_CAUSE(cause);
}

void DebugModule_sendCmd(DebugModule* dbg, DbgCmd cmd, ...) {
    va_list args;
    va_start(args, cmd);
    CPU* cpu = dbg->systemBus->cpu;

    switch(cmd) {
        case DBG_HALT:
            DebugModule_sendHalt(dbg, 3); //halt request
            break;
        case DBG_LOAD:
            {
                char* path = va_arg(args, char*);
                int base = va_arg(args, int);
                int size = va_arg(args, int);
                BusDevice* dev = MainBus_getDevice(dbg->systemBus, base);
                if (!dev)  {
                    printf("rvemu: no device mapped at address 0x%x\r\n", base);
                } else if (dev->type == DEV_SRAM) {
                    SRAM_loadFile((SRAM*) dev, path, base, size);
                    printf("rvemu: successfully loaded image\r\n");
                } else {
                    printf("rvemu: %d: not a memory device\r\n", dev->type);
                }
                
            }
            break;
        case DBG_CATCH_VEC:
            cpu->csr_dcsr |= DCSR_VCATCH(1);
            break;
        case DBG_CATCH_EBREAK:
            cpu->csr_dcsr |= DCSR_EBREAK(1);
            break;
        case DBG_CONTINUE:
            cpu->mode = MODE_M;
            cpu->pc = cpu->csr_dpc;
            break;
        default:
            printf("rvemu: unknown debug command %d\r\n", cmd);
    }

    va_end(args);
}

static const char* dcsr_causes[] = {
    "none",
    "ebreak",
    "exception",
    "halt request",
    "step"
};

void DebugModule_tick(DebugModule* dbg) {
    CPU* hart0 = dbg->systemBus->cpu;
    
    if (hart0->mode == MODE_D) {
        
        printf("\r\nrvemu: entered debug mode!\r\n");
        printf("dcsr.cause: %s, pc: 0x%x, mcause.code: %d, mcause.int: %b, mtval: 0x%x\r\n", dcsr_causes[DCSR_GETCAUSE(hart0->csr_dcsr)], hart0->pc, hart0->csr_mcause & MCAUSE_CODE_MSK, hart0->csr_mcause & MCAUSE_INTR(1), hart0->csr_mtval);
        dbg->running = 0;

        char cmd[256];
        snprintf(cmd, sizeof(cmd),
                 "riscv64-unknown-elf-addr2line -e ./test/test.elf 0x%x", hart0->pc);

        FILE *fp = popen(cmd, "r");
        if (!fp) { perror("popen"); return; }

        char buf[512];
        while (fgets(buf, sizeof(buf), fp)) {
            printf("line: %s\r\n", buf);
        }

        pclose(fp);
        
    }
}






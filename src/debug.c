#include "debug.h"
#include <stdio.h>
#include <stdlib.h>

void DebugModule_init(DebugModule* dbg, MainBus* systemBus) {
    dbg->systemBus = systemBus;
    dbg->running = 1;
}

void DebugModule_tick(DebugModule* dbg) {
    CPU* hart0 = dbg->systemBus->cpu;
    
    if (hart0->mode == MODE_D) {
        
        printf("rvemu: entered debug mode!\r\n");
        printf("pc: 0x%x, mcause.code: %d, mcause.int: %b, mtval: 0x%x\r\n", hart0->pc, hart0->csr_mcause & MCAUSE_CODE_MSK, hart0->csr_mcause & MCAUSE_INTR(1), hart0->csr_mtval);
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






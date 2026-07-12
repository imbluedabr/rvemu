#include "debug.h"
#include "bus.h"
#include "mem.h"
#include "cpu.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/termios.h>

void DebugModule_init(DebugModule* dbg, MainBus* systemBus) {
    dbg->systemBus = systemBus;
    dbg->running = 1;
    memset(dbg->file, 0, sizeof(dbg->file));
}

void DebugModule_sendHalt(DebugModule* dbg, int cause) {
    printf("rvemu: halting core\r\n");
    CPU* cpu = dbg->systemBus->cpu;
    cpu->mode = MODE_D;
    cpu->csr_dpc = cpu->pc;
    cpu->csr_dcsr &= ~DCSR_CAUSE_MSK;
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
                MMEntry* mm = MainBus_getMMEntry(dbg->systemBus, base);
                if (!mm)  {
                    printf("rvemu: no device mapped at address 0x%x\r\n", base);
                } else if (mm->dev->type == DEV_SRAM) {
                    SRAM_loadFile((SRAM*) mm->dev, path, base - mm->base, size);
                    printf("rvemu: successfully loaded image\r\n");
                } else {
                    printf("rvemu: %d: not a memory device\r\n", mm->dev->type);
                }
                
            }
            break;
        case DBG_FILE:
            {
                char* path = va_arg(args, char*);
                snprintf(dbg->file, 256, "%s", path);
            }
            break;
        case DBG_INFO:
            printf("registers: ");
            for (int i = 1; i < 32; i++) printf("x%d=%d, ", i, cpu->registers[i]);
            printf("pc=%d\r\n", cpu->pc);
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

static char* get_line(DebugModule* dbg, uint32_t address) {
    if (dbg->file[0] == '\0') {
        printf("file: no elf file loaded\n");
        return "";
    }
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
             "riscv64-unknown-elf-addr2line -e %s 0x%x", dbg->file, address);

    FILE *fp = popen(cmd, "r");
    if (!fp) { perror("popen"); return ""; }

    static char buf[512];
    fgets(buf, sizeof(buf), fp);

    pclose(fp);
    return buf;
}

void DebugModule_shell(DebugModule* dbg) {
    
    while (1) {
        printf("debug> ");
        char buff[64];
        read(STDIN_FILENO, buff, 64);
        
        char* buffer = buff;
        static char* arg_vec[8];
        int argc = 0;
        char* current_word = buffer;
        while(*buffer) { //handle endline character
            if (*buffer == '\n') {
                *buffer = '\0';
            }
            if (*buffer == ' ') {
                *buffer = '\0';
                arg_vec[argc++] = current_word;
                current_word = buffer + 1;
            }
            if (argc > 7) break;
            buffer++;
        }
        if (current_word != buffer && *current_word) arg_vec[argc++] = current_word;
        arg_vec[argc] = NULL;
        if (argc == 0) continue;

        if (strcmp(arg_vec[0], "q") == 0) {
            dbg->running = 0;
            break;
        } else if (strcmp(arg_vec[0], "c") == 0) {
            printf("Continuing\n");
            DebugModule_sendCmd(dbg, DBG_CONTINUE);
            break;
        } else if (strcmp(arg_vec[0], "load") == 0) {
            if (argc != 4) {
                printf("load: invalid argument(s)\n");
            } else {
                DebugModule_sendCmd(dbg, DBG_LOAD, arg_vec[1], atoi(arg_vec[2]), atoi(arg_vec[3]));
            }
        } else if (strcmp(arg_vec[0], "file") == 0) {
            if (argc != 2) {
                printf("file: invalid argument(s)\n");
            } else {
                DebugModule_sendCmd(dbg, DBG_FILE, arg_vec[1]);
            }
        } else if (strcmp(arg_vec[0], "line") == 0) {
            if (argc != 2) {
                printf("line: invalid argument(s)\n");
            } else {
                printf("line: %s\n", get_line(dbg, atoi(arg_vec[1])));
            }
        } else if (strcmp(arg_vec[0], "info") == 0) {
            DebugModule_sendCmd(dbg, DBG_INFO);
        } else {
            printf("rvemu: %s: unkown debug command\n", arg_vec[0]);
        }
    }
}

void DebugModule_tick(DebugModule* dbg) {
    CPU* hart0 = dbg->systemBus->cpu;
    
    if (hart0->mode == MODE_D) {
        
        struct termios t, old;
        tcgetattr(STDIN_FILENO, &t);
        old = t;
        t.c_lflag |= ICANON | ECHO;
        t.c_iflag |= ICRNL | IXON;
        t.c_oflag |= OPOST;
        t.c_lflag |= ISIG;
        t.c_cc[VMIN]  = 1;
        t.c_cc[VTIME] = 0;

        tcsetattr(STDIN_FILENO, TCSANOW, &t);

        setvbuf(stdout, NULL, _IONBF, 0);
        printf("rvemu: entered debug mode!\n");
        printf("dcsr.cause: %s, pc: 0x%x, mcause.code: %d, mcause.int: %b, mtval: 0x%x\n", dcsr_causes[DCSR_GETCAUSE(hart0->csr_dcsr)], hart0->pc, hart0->csr_mcause & MCAUSE_CODE_MSK, hart0->csr_mcause & MCAUSE_INTR(1), hart0->csr_mtval);
        printf("line: %s\n", get_line(dbg, hart0->pc));
        
        DebugModule_shell(dbg);
        
        tcsetattr(STDIN_FILENO, TCSANOW, &old);
    }
}






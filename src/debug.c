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

void init_bfd(DebugModule* dbg, const char *filename) {
    bfd_init();

    dbg->abfd = bfd_openr(filename, NULL);
    if (!dbg->abfd) {
        bfd_perror("bfd_openr");
        exit(1);
    }

    if (!bfd_check_format(dbg->abfd, bfd_object)) {
        fprintf(stderr, "Not an object file\n");
        exit(1);
    }

    long symtab_size = bfd_get_symtab_upper_bound(dbg->abfd);
    dbg->syms = malloc(symtab_size);
    bfd_canonicalize_symtab(dbg->abfd, dbg->syms);
}

void addr_to_line(DebugModule* dbg, bfd_vma addr) {
    const char *file, *func;
    unsigned int line;

    if (bfd_find_nearest_line(dbg->abfd, dbg->abfd->sections, dbg->syms,
                              addr, &file, &func, &line)) {
        printf("Address 0x%lx -> %s:%u (%s)\n",
               addr, file, line, func);
    } else {
        printf("No debug info for 0x%lx\n", addr);
    }
}

bfd_vma symbol_to_addr(DebugModule* dbg, const char *name) {
    unsigned int count = bfd_get_symtab_upper_bound(dbg->abfd) / sizeof(asymbol *);
    for (unsigned int i = 0; i < count; i++) {
        const char *symname = bfd_asymbol_name(dbg->syms[i]);
        if (symname && strcmp(symname, name) == 0) {
            return bfd_asymbol_value(dbg->syms[i]);
        }
    }
    return 0;
}

bfd_vma line_to_addr(DebugModule* dbg, const char *file, unsigned int target_line) {
    for (asection *sec = dbg->abfd->sections; sec; sec = sec->next) {
        if (!(sec->flags & SEC_CODE)) continue;  //only scan .text

        bfd_vma start = sec->vma;
        bfd_vma end   = sec->vma + sec->size;

        for (bfd_vma addr = start; addr < end; addr++) {
            const char *srcfile, *func;
            unsigned int line;

            if (bfd_find_nearest_line(dbg->abfd, sec, dbg->syms,
                                      addr, &srcfile, &func, &line)) {
                if (srcfile && strcmp(srcfile, file) == 0 &&
                    line == target_line) {
                    return addr;
                }
            }
        }
    }
    return 0;
}

void DebugModule_init(DebugModule* dbg, MainBus* systemBus) {
    dbg->systemBus = systemBus;
    dbg->running = 1;
    dbg->syms = NULL;
    dbg->abfd = NULL;
    for (int i = 0; i < BKPT_LEN; i++)
        dbg->breakpoint[i] = 0;
}

int DebugModule_addBreakpoint(DebugModule* dbg, int address) {
    for (int i = 0; i < BKPT_LEN; i++) {
        if (dbg->breakpoint[i] == 0) {
            dbg->breakpoint[i] = address;
            return i;
        }
    }
    return 0;
}

void DebugModule_sendHalt(DebugModule* dbg, int cause) {
    CPU* cpu = dbg->systemBus->cpu;
    cpu->csr_dpc = cpu->pc;
    cpu->csr_dcsr &= ~(DCSR_CAUSE_MSK | DCSR_PRV_MSK);
    cpu->csr_dcsr |= DCSR_CAUSE(cause) | DCSR_PRV(cpu->mode);
    cpu->debug_mode = 1;
    printf("rvemu: halting core\r\n");
}

void DebugModule_sendCmd(DebugModule* dbg, DbgCmd cmd, ...) {
    va_list args;
    va_start(args, cmd);
    CPU* cpu = dbg->systemBus->cpu;

    switch(cmd) {
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
                init_bfd(dbg, path);
            }
            break;
        case DBG_INFO:
            printf("registers: ");
            for (int i = 0; i < 32; i++) printf("x%d=%d, ", i, cpu->registers[i]);
            printf("pc=0x%x\r\n", cpu->pc);
            printf("csr: mstatus=%d, mtvec=0x%x, mepc=0x%x, mcause=%d, mip=%d\n", cpu->csr_mstatus, cpu->csr_mtvec, cpu->csr_mepc, cpu->csr_mcause, cpu->csr_mip);
            break;
        case DBG_LINE:
            {
                uint32_t vma = va_arg(args, int);
                addr_to_line(dbg, vma);
            }
            break;
        case DBG_BREAK:
            {
                uint32_t vma = va_arg(args, int);
                int bkpt = DebugModule_addBreakpoint(dbg, vma);
                printf("breakpoint %d at 0x%x\r\n", bkpt, vma);
            }
            break;
        case DBG_CATCH_VEC:
            cpu->csr_dcsr |= DCSR_VCATCH(1);
            break;
        case DBG_CATCH_EBREAK:
            cpu->csr_dcsr |= DCSR_EBREAK(1);
            break;
        case DBG_HALT:
            DebugModule_sendHalt(dbg, 3); //halt request
            break;
        case DBG_STEP:
            cpu->csr_dcsr |= DCSR_STEP(1);
            cpu->mode = DCSR_GETPRV(cpu->csr_dcsr);
            cpu->debug_mode = 0;
            cpu->pc = cpu->csr_dpc;
            break;
        case DBG_CONTINUE:
            cpu->csr_dcsr &= ~DCSR_STEP(1);
            cpu->mode = DCSR_GETPRV(cpu->csr_dcsr);
            cpu->debug_mode = 0;
            cpu->pc = cpu->csr_dpc;
            break;
        case DBG_SKIP:
            cpu->csr_dpc += 4;
            break;
        default:
            printf("rvemu: unknown debug command %d\r\n", cmd);
    }

    va_end(args);
}

static const char* dcsr_causes[] = {
    "ebreak",
    "exception",
    "halt request",
    "step"
};

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
        } else if (strcmp(arg_vec[0], "s") == 0) {
            DebugModule_sendCmd(dbg, DBG_STEP);
            break;
        } else if (strcmp(arg_vec[0], "b") == 0) {
            uint32_t vma = 0;
            if (argc == 3) {
                vma = line_to_addr(dbg, arg_vec[1], atoi(arg_vec[2]));
            } else if (argc == 2) {
                vma = symbol_to_addr(dbg, arg_vec[1]);
            }
            if (vma != 0) {
                DebugModule_sendCmd(dbg, DBG_BREAK, vma);
            } else {
                printf("break: symbol/line not found\n");
            }
        } else if (strcmp(arg_vec[0], "skip") == 0) {
            DebugModule_sendCmd(dbg, DBG_SKIP);
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
            if (argc == 1) {
                DebugModule_sendCmd(dbg, DBG_LINE, dbg->systemBus->cpu->csr_dpc);
            } else if (argc == 2) {
                DebugModule_sendCmd(dbg, DBG_LINE, atoi(arg_vec[1]));
            } else {
                printf("line: invalid argument(s)\n");
            }
        } else if (strcmp(arg_vec[0], "info") == 0) {
            DebugModule_sendCmd(dbg, DBG_INFO);
        } else if (strcmp(arg_vec[0], "vret") == 0) {
            DebugModule_sendCmd(dbg, DBG_LINE, dbg->systemBus->cpu->csr_mepc);
        } else if (strcmp(arg_vec[0], "address") == 0) {
            if (argc != 2) {
                printf("address: invalid argument(s)\n");
            } else {
                printf("address: 0x%lx\n", symbol_to_addr(dbg, arg_vec[1]));
            }
        } else {
            printf("rvemu: %s: unkown debug command\n", arg_vec[0]);
        }
    }
}

void DebugModule_tick(DebugModule* dbg) {
    CPU* hart0 = dbg->systemBus->cpu;

    for (int i = 0; i < BKPT_LEN; i++) {
        if (dbg->breakpoint[i] != 0 && dbg->breakpoint[i] == hart0->pc) {
            printf("hit breakpoint %d\r\n", i);
            DebugModule_sendHalt(dbg, 2);
        }
    }

    if (hart0->debug_mode) {
        
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
        printf("mode: %d, dcsr.cause: %s, pc: 0x%x, mcause{code: %d, int: %d}, mtval: 0x%x\n", hart0->mode, dcsr_causes[DCSR_GETCAUSE(hart0->csr_dcsr)], hart0->pc, hart0->csr_mcause & MCAUSE_CODE_MSK, (hart0->csr_mcause & MCAUSE_INTR(1)) >> 31, hart0->csr_mtval);
        DebugModule_sendCmd(dbg, DBG_LINE, hart0->pc);        
        DebugModule_shell(dbg);
        
        tcsetattr(STDIN_FILENO, TCSANOW, &old);
    }

}






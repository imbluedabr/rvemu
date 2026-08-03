#include "cpu.h"
#include "bus.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

void CPU_init(CPU* cpu, MainBus* systemBus) {
    memset(cpu, 0, sizeof(CPU));
    cpu->mode = MODE_M;
    cpu->system_bus = systemBus;
}

static inline void csr_write(CPU* cpu, uint32_t csr, uint32_t val) {
    switch (csr) {
        case 0x300: //mstatus
            cpu->csr_mstatus = val;
            break;
        case 0x301: //misa
            cpu->csr_misa = val;
            break;
        case 0x304: //mie
            cpu->csr_mie = val;
            break;
        case 0x305: //mtvec
            cpu->csr_mtvec = val;
            break;
        case 0x340: //mscratch
            cpu->csr_mscratch = val;
            break;
        case 0x341: //mepc
            cpu->csr_mepc = val;
            break;
        case 0x342: //mcause
            cpu->csr_mcause = val;
            break;
        case 0x343: //mtval
            cpu->csr_mtval = val;
            break;
        case 0x344: //mip
            cpu->csr_mip = val;
            break;
    }
}

static inline uint32_t csr_read(CPU* cpu, uint32_t csr) {
    switch (csr) {
        case 0x300: //mstatus
            return cpu->csr_mstatus;
        case 0x301: //misa
            return cpu->csr_misa;
        case 0x304: //mie
            return cpu->csr_mie;
        case 0x305: //mtvec
            return cpu->csr_mtvec;
        case 0x340: //mscratch
            return cpu->csr_mscratch;
        case 0x341: //mepc
            return cpu->csr_mepc;
        case 0x342: //mcause
            return cpu->csr_mcause;
        case 0x343: //mtval
            return cpu->csr_mtval;
        case 0x344: //mip
            return cpu->csr_mip;
        default:
            return 0;
    }
}

void CPU_exception(CPU* cpu, uint8_t exception, uint32_t mtval) {
    if (cpu->csr_dcsr & DCSR_VCATCH(1)) {
        DebugModule_sendHalt(cpu->system_bus->dbg, 1);
    }
    cpu->csr_mcause = MCAUSE_CODE(exception) | MCAUSE_INTR(0);
    cpu->csr_mtval = mtval;
    cpu->trap_pending = 1;
}

void CPU_interrupt_fast(CPU* cpu, uint8_t irq) {
    if (cpu->csr_dcsr & DCSR_VCATCH(1)) {
        DebugModule_sendHalt(cpu->system_bus->dbg, 1);
    }
    cpu->csr_mcause = MCAUSE_CODE(irq + 16) | MCAUSE_INTR(1);
    cpu->csr_mip |= MIP_FAST_IRQ(1 << irq);
}

void CPU_iret(CPU* cpu) {
    cpu->mode = MSTATUS_GETMPP(cpu->csr_mstatus);
    cpu->csr_mstatus &= ~MSTATUS_MPP_MSK;
    if (cpu->csr_mstatus & MSTATUS_MPIE(1))
        cpu->csr_mstatus |= MSTATUS_MIE(1);
    cpu->pc = cpu->csr_mepc;
}

void CPU_handleTrap(CPU* cpu) {

    if (cpu->csr_mstatus & MSTATUS_MIE(1))
        cpu->csr_mstatus |= MSTATUS_MPIE(1);
    cpu->csr_mstatus &= ~MSTATUS_MIE(1);
    cpu->csr_mstatus &= ~MSTATUS_MPP_MSK;
    cpu->csr_mstatus |= MSTATUS_MPP(cpu->mode);
    cpu->mode = MODE_M;
    cpu->csr_mepc = cpu->pc;
    cpu->pc = cpu->csr_mtvec;
    return;
}

static inline uint32_t SEXT(uint32_t VAL, uint32_t N) {
    return (VAL | ((VAL & ((uint32_t)1 << N)) ? (~((1 << N) - 1)) : 0x0));
}

static void CPU_checkInterrupts(CPU* cpu) {

    if (cpu->trap_pending) {
        cpu->trap_pending = 0;
        CPU_handleTrap(cpu);
    } else if (cpu->csr_mstatus & MSTATUS_MIE(1)) {
        for (int i = 0; i < 16; i++) {
            if (cpu->csr_mie & MIE_FAST_IRQ(i) && cpu->csr_mip & MIP_FAST_IRQ(i)) {
                cpu->csr_mip &= ~MIP_FAST_IRQ(1);
                CPU_handleTrap(cpu);
                break;
            }
        }
    }
}

void CPU_tick(CPU* cpu) {
    if (cpu->mode == MODE_D) return;
    Bus* bus = &cpu->system_bus->_Bus;
    uint32_t instruction;
    uint32_t temp;
    uint8_t byte;
    uint16_t halfword;
    uint32_t word;

    //interrupt handeling
    CPU_checkInterrupts(cpu);

    uint32_t pc = cpu->pc;
    //fetch
    if (bus->read(bus, pc, &instruction, 4) < 0) {
        return CPU_exception(cpu, FAULT_IACCESS, pc);
    }
    
    //decode
    uint32_t opcode = instruction & 0x7F;
    uint32_t rd = (instruction >> 7) & 0x1F;
    uint32_t funct3 = (instruction >> 12) & 0x7;
    uint32_t rs1 = (instruction >> 15) & 0x1F;
    uint32_t rs2 = (instruction >> 20) & 0x1F;
    uint32_t funct7 = (instruction >> 25) & 0x7F;
    
    uint32_t i_imm = (instruction >> 20) & 0xFFF;
    uint32_t s_imm = ((instruction >> 7) & 0x1F) | ((instruction & 0xFE000000) >> 20);
    uint32_t b_imm = ((instruction >> 7) & 0x1E) | ((instruction & 0x7E000000) >> 20) | ((instruction & 0x80) << 4);
    uint32_t u_imm = instruction & 0xFFFFF000;

    //execute
    
    switch (opcode) {
        case 0b0110111: //LUI
            if (rd) cpu->registers[rd] = u_imm;
            pc += 4;
            break;
        case 0b0010111: //AUIPC
            if (rd) cpu->registers[rd] = cpu->pc + u_imm;
            pc += 4;
            break;
        case 0b0010011:
            switch (funct3) {
                case 0b000: //ADDI
                    temp = cpu->registers[rs1] + SEXT(i_imm, 11);
                    break;
                case 0b001: //SLLI
                    temp = cpu->registers[rs1] << (i_imm & 0x1F);
                    break;
                case 0b010: //SLTI
                    temp = ((int) cpu->registers[rs1]) < (int) SEXT(i_imm, 11);
                    break;
                case 0b011: //SLTIU
                    temp = cpu->registers[rs1] < i_imm;
                    break;
                case 0b100: //XORI
                    temp = cpu->registers[rs1] ^ SEXT(i_imm, 11);
                    break;
                case 0b101: //SRAI, SRLI
                    if (i_imm & 0x400) {
                        temp = ((int) cpu->registers[rs1]) >> (i_imm & 0x1F);
                    } else {
                        temp = cpu->registers[rs1] >> (i_imm & 0x1F);
                    }
                    break;
                case 0b110: //ORI
                    temp = cpu->registers[rs1] | SEXT(i_imm, 11);
                    break;
                case 0b111: //ANDI
                    temp = cpu->registers[rs1] & SEXT(i_imm, 11);
                    break;
            }
            if (rd) cpu->registers[rd] = temp;
            pc += 4;
            break;

        case 0b0110011:
            switch (funct3) {
                case 0b000: //ADD, SUB
                    if (funct7 & 0x20) {
                        temp = cpu->registers[rs1] - cpu->registers[rs2];
                    } else {
                        temp = cpu->registers[rs1] + cpu->registers[rs2];
                    }
                    break;
                case 0b001: //SLL
                    temp = cpu->registers[rs1] << (cpu->registers[rs2] & 0x1F);
                    break;
                case 0b010: //SLT
                    temp = ((int) cpu->registers[rs1]) < (int) cpu->registers[rs2];
                    break;
                case 0b011: //SLTU
                    temp = cpu->registers[rs1] < cpu->registers[rs2];
                    break;
                case 0b100: //XOR
                    temp = cpu->registers[rs1] ^ cpu->registers[rs2];
                    break;
                case 0b101: //SRA, SRL
                    if (funct7 & 0x20) {
                        temp = ((int) cpu->registers[rs1]) >> (cpu->registers[rs2] & 0x1F);
                    } else {
                        temp = cpu->registers[rs1] >> (cpu->registers[rs2] & 0x1F);
                    }
                    break;
                case 0b110: //OR
                    temp = cpu->registers[rs1] | cpu->registers[rs2];
                    break;
                case 0b111: //AND
                    temp = cpu->registers[rs1] & cpu->registers[rs2];
                    break;
            }
            if (rd) cpu->registers[rd] = temp;
            pc += 4;
            break;

        case 0b1110011:
            if (funct3 > 0) {
                temp = 0;
                if (rd) temp = csr_read(cpu, i_imm);
            }

            switch (funct3) {
                case 0b000:
                    if (rs2 == 0) { //ECALL
                        if (cpu->mode == MODE_M) {
                            CPU_exception(cpu, FAULT_MCALL, 0);
                        } else if (cpu->mode == MODE_U) {
                            CPU_exception(cpu, FAULT_UCALL, 0);
                        } else {
                            return CPU_exception(cpu, FAULT_ILLINSTR, instruction);
                        }
                    } else if (rs2 == 1) { //EBREAK
                        if (cpu->csr_dcsr & DCSR_EBREAK(1)) {
                            DebugModule_sendHalt(cpu->system_bus->dbg, 0);
                        } else {
                            return CPU_exception(cpu, FAULT_DEBUG, 0);
                        }
                    } else if (rs2 == 2) { //MRET
                        if (cpu->mode != MODE_M) {
                            return CPU_exception(cpu, FAULT_ILLINSTR, instruction);
                        } else {
                            CPU_iret(cpu);
                            pc = cpu->pc;
                        }
                    }
                    break;
                case 0b001: //CSRRW
                    csr_write(cpu, i_imm, cpu->registers[rs1]);
                    break;
                case 0b010: //CSRRS
                    csr_write(cpu, i_imm, temp | cpu->registers[rs1]);
                    break;
                case 0b011: //CSRRC
                    csr_write(cpu, i_imm, temp & ~cpu->registers[rs1]);
                    break;
                case 0b101: //CSRRWI
                    csr_write(cpu, i_imm, rs1);
                    break;
                case 0b110: //CSRRSI
                    csr_write(cpu, i_imm, temp | rs1);
                    break;
                case 0b111: //CSRRCI
                    csr_write(cpu, i_imm, temp & ~rs1);
                    break;
                default:
                    return CPU_exception(cpu, FAULT_ILLINSTR, instruction);
            }
            if (funct3 > 0) {
                if (rd) cpu->registers[rd] = temp;
                pc += 4;
            }
            break;

        case 0b0000011:
            temp = cpu->registers[rs1] + SEXT(i_imm, 11);
            switch (funct3) {
                case 0b000: //LB
                    bus->read(bus, temp, &byte, 1);
                    temp = SEXT(byte, 7);
                    break;
                case 0b001: //LH
                    bus->read(bus, temp, &halfword, 2);
                    temp = SEXT(halfword, 15);
                    break;
                case 0b010: //LW
                    bus->read(bus, temp, &word, 4);
                    temp = word;
                    break;
                case 0b100: //LBU
                    bus->read(bus, temp, &byte, 1);
                    temp = byte;
                    break;
                case 0b101: //LHU
                    bus->read(bus, temp, &halfword, 2);
                    temp = halfword;
                    break;
                default:
                    return CPU_exception(cpu, FAULT_ILLINSTR, instruction);
            }
            if (rd) cpu->registers[rd] = temp;
            pc += 4;
            break;

        case 0b0100011:
            temp = cpu->registers[rs1] + SEXT(s_imm, 11);

            switch (funct3) {
                case 0b000: //SB
                    bus->write(bus, temp, &cpu->registers[rs2], 1);
                    break;
                case 0b001: //SH
                    bus->write(bus, temp, &cpu->registers[rs2], 2);
                    break;
                case 0b010: //SW
                    bus->write(bus, temp, &cpu->registers[rs2], 4);
                    break;
                default:
                    return CPU_exception(cpu, FAULT_ILLINSTR, instruction);
            }
            
            pc += 4;
            break;
        
        case 0b1101111: //JAL
            if (rd) cpu->registers[rd] = pc + 4;
            temp = ((instruction & 0xFF000) | ((instruction & 0x7FE00000) >> 20) | ((instruction & 0x80000000) >> 11) | ((instruction & 0x100000) >> 9));
            pc += SEXT(temp, 19);
            break;

        case 0b1100111: //JALR
            if (rd) cpu->registers[rd] = pc + 4;
            temp = cpu->registers[rs1] + SEXT(i_imm, 11);
            pc = temp & ~1;
            break;

        case 0b1100011:
            temp = 0;
            switch (funct3) {
                case 0b000: //BEQ
                    temp = cpu->registers[rs1] == cpu->registers[rs2];
                    break;
                case 0b001: //BNE
                    temp = cpu->registers[rs1] != cpu->registers[rs2];
                    break;
                case 0b100: //BLT
                    temp = ((int) cpu->registers[rs1]) < (int) cpu->registers[rs2];
                    break;
                case 0b101: //BGE
                    temp = ((int) cpu->registers[rs1]) >= (int) cpu->registers[rs2];
                    break;
                case 0b110: //BLTU
                    temp = cpu->registers[rs1] < cpu->registers[rs2];
                    break;
                default:
                    return CPU_exception(cpu, FAULT_ILLINSTR, instruction);
            }
            if (temp) {
                pc += SEXT(b_imm, 11);
            } else {
                pc += 4;
            }
            break;

        default:
            return CPU_exception(cpu, FAULT_ILLINSTR, instruction);
    }
    cpu->pc = pc;
    if (cpu->csr_dcsr & DCSR_STEP(1)) {
        DebugModule_sendHalt(cpu->system_bus->dbg, 3);
    }
    return;
}








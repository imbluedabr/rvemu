#include "cpu.h"
#include "bus.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

void CPU_init(CPU* cpu) {
    memset(cpu, 0, sizeof(CPU));
    cpu->mode = MODE_M;
}

static inline void csr_write(CPU* cpu, uint32_t csr, uint32_t val) {
    switch (csr) {
        case 0x300: //mstatus
            cpu->csr_mstatus = val;
            break;
        case 0x301: //misa
            cpu->csr_misa = val;
        case 0x304: //mie
            cpu->csr_mie = val;
            break;
        case 0x305: //mtvec
            cpu->csr_mtvec = val;
            break;
        case 0x310: //mstatush
            cpu->csr_mstatush = val;
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
        case 0x310: //mstatush
            return cpu->csr_mstatush;
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
        DebugModule_sendHalt(cpu->system_bus->dbg, 2);
    } else {
        cpu->csr_mcause = MCAUSE_CODE(exception) | MCAUSE_INTR(0);
        cpu->csr_mtval = mtval;
        cpu->trap_pending = 1;
    }
}

void CPU_interrupt_fast(CPU* cpu, uint8_t irq) {
    cpu->csr_mcause = MCAUSE_CODE(irq + 16) | MCAUSE_INTR(1);
    cpu->csr_mip |= MIP_FAST_IRQ(1 << irq);
}

void CPU_iret(CPU* cpu) {
    cpu->mode = MSTATUS_GETMPRV(cpu->csr_mstatus);
    cpu->csr_mstatus &= ~MSTATUS_MIE(1);
    if (cpu->csr_mstatus & MSTATUS_MPIE(1))
        cpu->csr_mstatus |= MSTATUS_MIE(1);

    cpu->pc = cpu->csr_mepc;
}

void CPU_handleTrap(CPU* cpu) {
    Bus* bus = &cpu->system_bus->_Bus;

    if (cpu->mode == MODE_D) return;

    if (cpu->csr_mstatus & MSTATUS_MIE(1))
        cpu->csr_mstatus |= MSTATUS_MPIE(1);
    cpu->csr_mstatus &= ~MSTATUS_MIE(1);
    cpu->csr_mstatus &= ~MSTATUS_MPRV(1);
    cpu->csr_mstatus |= MSTATUS_MPRV(cpu->mode);

    cpu->csr_mepc = cpu->pc;
    uint32_t temp;
    if (bus->read(bus, cpu->csr_mtvec, &temp, 4) < 0) {
        CPU_exception(cpu, FAULT_LACCESS, cpu->csr_mtvec);
    }

    cpu->pc = temp;
    return;
}

void CPU_tick(CPU* cpu) {
    if (cpu->mode == MODE_D) return;
    Bus* bus = &cpu->system_bus->_Bus;
    uint32_t instruction;
    uint32_t pc = cpu->pc;
    uint32_t temp;
    uint8_t byte;
    uint16_t halfword;
    cpu->registers[0] = 0; //force x0 to zero

    //interrupt handeling
    if (cpu->trap_pending) {
        cpu->trap_pending = 0;
        CPU_handleTrap(cpu);
    } else {
        for (int i = 0; i < 16; i++) {
            if (cpu->csr_mip & (1 << i)) {
                cpu->csr_mip &= ~(1 << i);
                CPU_handleTrap(cpu);
                break;
            }
        }
    }

    //fetch
    if (bus->read(bus, pc, &instruction, 4) < 0) {
        CPU_exception(cpu, FAULT_IACCESS, pc);
        return;
    }
    
    //decode
    uint32_t opcode = instruction & 0x7F;
    uint32_t rd = (instruction >> 7) & 0x1F;
    uint32_t funct3 = (instruction >> 12) & 0x7;
    uint32_t rs1 = (instruction >> 15) & 0x1F;
    uint32_t rs2 = (instruction >> 20) & 0x1F;
    uint32_t funct7 = (instruction >> 25) & 0x7F;
    
    uint32_t i_imm = (instruction >> 20) & 0xFFF;
    uint32_t s_imm = ((instruction >> 7) & 0x1F) | ((instruction & 0xFF000000) >> 20);
    uint32_t u_imm = instruction & 0xFFFFF000;

    //execute
    
    switch (opcode) {
        case 0b0110111: //LUI
            cpu->registers[rd] = u_imm;
            pc += 4;
            break;
        case 0b0010111: //AUIPC
            cpu->registers[rd] = cpu->pc + u_imm;
            pc += 4;
            break;
        case 0b0010011:
            switch (funct3) {
                case 0b000: //ADDI
                    cpu->registers[rd] = cpu->registers[rs1] + (int) i_imm;
                    break;
                case 0b001: //SLLI
                    cpu->registers[rd] = cpu->registers[rs1] << (i_imm & 0x1F);
                    break;
                case 0b010: //SLTI
                    cpu->registers[rd] = ((int) cpu->registers[rs1]) < (int) i_imm;
                    break;
                case 0b011: //SLTIU
                    cpu->registers[rd] = cpu->registers[rs1] < i_imm;
                    break;
                case 0b100: //XORI
                    cpu->registers[rd] = cpu->registers[rs1] ^ (int) i_imm;
                    break;
                case 0b101: //SRAI, SRLI
                    if (i_imm & 0x400) {
                        cpu->registers[rd] = ((int) cpu->registers[rs1]) >> (i_imm & 0x1F);
                    } else {
                        cpu->registers[rd] = cpu->registers[rs1] >> (i_imm & 0x1F);
                    }
                    break;
                case 0b110: //ORI
                    cpu->registers[rd] = cpu->registers[rs1] | (int) i_imm;
                    break;
                case 0b111: //ANDI
                    cpu->registers[rd] = cpu->registers[rs1] & (int) i_imm;
                    break;
            }
            pc += 4;
            break;

        case 0b0110011:
            switch (funct3) {
                case 0b000: //ADD, SUB
                    if (funct7 & 0x20) {
                        cpu->registers[rd] = cpu->registers[rs1] - cpu->registers[rs2];
                    } else {
                        cpu->registers[rd] = cpu->registers[rs1] + cpu->registers[rs2];
                    }
                    break;
                case 0b001: //SLL
                    cpu->registers[rd] = cpu->registers[rs1] << (cpu->registers[rs2] & 0x1F);
                    break;
                case 0b010: //SLT
                    cpu->registers[rd] = ((int) cpu->registers[rs1]) < (int) cpu->registers[rs2];
                    break;
                case 0b011: //SLTU
                    cpu->registers[rd] = cpu->registers[rs1] < cpu->registers[rs2];
                    break;
                case 0b100: //XOR
                    cpu->registers[rd] = cpu->registers[rs1] ^ cpu->registers[rs2];
                    break;
                case 0b101: //SRA, SRL
                    if (funct7 & 0x20) {
                        cpu->registers[rd] = ((int) cpu->registers[rs1]) >> (cpu->registers[rs2] & 0x1F);
                    } else {
                        cpu->registers[rd] = cpu->registers[rs1] >> (cpu->registers[rs2] & 0x1F);
                    }
                    break;
                case 0b110: //OR
                    cpu->registers[rd] = cpu->registers[rs1] | cpu->registers[rs2];
                    break;
                case 0b111: //AND
                    cpu->registers[rd] = cpu->registers[rs1] & cpu->registers[rs2];
                    break;
            }
            pc += 4;
            break;

        case 0b1110011:
            temp = csr_read(cpu, i_imm);
            cpu->registers[rd] = temp;

            switch (funct3) {
                case 0b000:
                    if (rs2 == 0) { //ECALL
                        if (cpu->mode == MODE_M) {
                            CPU_exception(cpu, FAULT_MCALL, 0);
                        } else if (cpu->mode == MODE_U) {
                            CPU_exception(cpu, FAULT_UCALL, 0);
                        } else {
                            CPU_exception(cpu, FAULT_ILLINSTR, instruction);
                        }
                    } else if (rs2 == 1) { //EBREAK
                        if (cpu->csr_dcsr & DCSR_EBREAK(1)) {
                            DebugModule_sendHalt(cpu->system_bus->dbg, 1);
                        } else {
                            CPU_exception(cpu, FAULT_DEBUG, 0);
                        }
                    } else if (rs2 == 2) { //MRET
                        if (cpu->mode != MODE_M) {
                            CPU_exception(cpu, FAULT_ILLINSTR, instruction);
                        } else {
                            CPU_iret(cpu);
                        }
                    }
                case 0b001: //CSRRW
                    csr_write(cpu, i_imm, cpu->registers[rs1]);
                    pc += 4;
                    break;
                case 0b010: //CSRRS
                    csr_write(cpu, i_imm, temp | cpu->registers[rs1]);
                    pc += 4;
                    break;
                case 0b011: //CSRRC
                    csr_write(cpu, i_imm, temp & ~cpu->registers[rs1]);
                    pc += 4;
                    break;
                case 0b101: //CSRRWI
                    csr_write(cpu, i_imm, (int) rs1);
                    pc += 4;
                    break;
                case 0b110: //CSRRSI
                    csr_write(cpu, i_imm, temp | (int)rs1);
                    pc += 4;
                    break;
                case 0b111: //CSRRCI
                    csr_write(cpu, i_imm, temp & ~(int)rs1);
                    pc += 4;
                    break;
            }
            break;

        case 0b0000011:
            switch (funct3) {
                case 0b000: //LB
                    bus->read(bus, cpu->registers[rs1] + i_imm, &byte, 1);
                    cpu->registers[rd] = (int) byte;
                    break;
                case 0b001: //LH
                    bus->read(bus, cpu->registers[rs1] + i_imm, &halfword, 2);
                    cpu->registers[rd] = (int) halfword;
                    break;
                case 0b010: //LW
                    bus->read(bus, cpu->registers[rs1] + i_imm, &temp, 4);
                    cpu->registers[rd] = (int) temp;
                    break;
                case 0b100: //LBU
                    bus->read(bus, cpu->registers[rs1] + i_imm, &byte, 1);
                    cpu->registers[rd] = byte;
                    break;
                case 0b101: //LHU
                    bus->read(bus, cpu->registers[rs1] + i_imm, &halfword, 2);
                    cpu->registers[rd] = halfword;
                    break;
            }
            pc += 4;
            break;

        case 0b0100011:
            switch (funct3) {
                case 0b000: //SB
                    bus->write(bus, cpu->registers[rs1] + (int) s_imm, &cpu->registers[rs2], 1);
                    break;
                case 0b001: //SH
                    bus->write(bus, cpu->registers[rs1] + (int) s_imm, &cpu->registers[rs2], 2);
                    break;
                case 0b010: //SW
                    bus->write(bus, cpu->registers[rs1] + (int) s_imm, &cpu->registers[rs2], 1);
                    break;
            }
            pc += 4;
            break;
        
        case 0b1101111: //JAL
            cpu->registers[rd] = pc + 4;
            temp = ((instruction & 0xFF000) | ((instruction & 0x7FE00000) >> 20) | ((instruction & 0x80000000) >> 11) | ((instruction & 0x100000) >> 9));
            if (temp & (1 << 20)) temp |= 0xFFE00000;
            pc += temp;
            break;

        case 0b1100111: //JALR
            cpu->registers[rd] = pc + 4;
            temp = cpu->registers[rs1] + (int) i_imm;
            pc = temp & ~1;
            break;

        case 0b1100011:
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
            }
            if (temp) {
                pc += (int) s_imm;
            } else {
                pc += 4;
            }
            break;

        default:
            CPU_exception(cpu, FAULT_ILLINSTR, instruction);
    }
    cpu->pc = pc;
}








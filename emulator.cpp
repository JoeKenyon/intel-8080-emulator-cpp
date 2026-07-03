#include "hardware.h"
#include "opcodes.h"
#include <array>
#include <iostream>
#include <fstream>
#include <iomanip>


bool loadRom(const std::string& filename, uint16_t startAddress)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file)
    {
        std::cerr << "Error: Could not open file " << filename << "\n";
        return false;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (startAddress + fileSize > 65536)
    {
        std::cerr << "Error: ROM file is too large for memory.\n";
        return false;
    }

    file.read(reinterpret_cast<char*>(&memory[startAddress]), fileSize);

    return true;
}

// helper: HL as a single 16-bit value for DAD/INX/DCX
static inline uint16_t HL() { return ((uint16_t)regs.H << 8) | regs.L; }
static inline uint16_t BC() { return ((uint16_t)regs.B << 8) | regs.C; }
static inline uint16_t DE() { return ((uint16_t)regs.D << 8) | regs.E; }
static inline void setHL(uint16_t v) { regs.H = v >> 8; regs.L = v & 0xFF; }
static inline void setBC(uint16_t v) { regs.B = v >> 8; regs.C = v & 0xFF; }
static inline void setDE(uint16_t v) { regs.D = v >> 8; regs.E = v & 0xFF; }
static inline uint16_t MHL() { return (regs.H << 8) | regs.L; }

// ── dispatch table ──────────────────────────────────────────────────────────
// Each handler has a uniform signature: (op1, op2) -> continue?
// true  = keep stepping
// false = stop the emulator loop (HLT-as-exit / program termination signal)
using OpHandler = bool(*)(uint8_t op1, uint8_t op2);

static bool h_NOP(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; PC += 1; return true; }
static bool h_HLT(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_HLT(); return true; }
static bool h_EI(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_EI(); return true; }
static bool h_DI(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_DI(); return true; }
static bool h_EXIT(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; return false; }
static bool h_LXI_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_LXI_Reg(regs.B, regs.C, op1, op2); return true; }
static bool h_LXI_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_LXI_Reg(regs.D, regs.E, op1, op2); return true; }
static bool h_LXI_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_LXI_Reg(regs.H, regs.L, op1, op2); return true; }
static bool h_LXI_SP(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_LXI_SP(op1, op2); return true; }
static bool h_MVI_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MVI(regs.B, op1); return true; }
static bool h_MVI_C(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MVI(regs.C, op1); return true; }
static bool h_MVI_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MVI(regs.D, op1); return true; }
static bool h_MVI_E(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MVI(regs.E, op1); return true; }
static bool h_MVI_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MVI(regs.H, op1); return true; }
static bool h_MVI_L(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MVI(regs.L, op1); return true; }
static bool h_MVI_M(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MVI(memory[(regs.H << 8) | regs.L], op1); return true; }
static bool h_MVI_A(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MVI(regs.A, op1); return true; }
static bool h_MOV_B_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.B, regs.B); return true; }
static bool h_MOV_B_C(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.B, regs.C); return true; }
static bool h_MOV_B_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.B, regs.D); return true; }
static bool h_MOV_B_E(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.B, regs.E); return true; }
static bool h_MOV_B_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.B, regs.H); return true; }
static bool h_MOV_B_L(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.B, regs.L); return true; }
static bool h_MOV_B_M(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.B, memory[(regs.H << 8) | regs.L]); return true; }
static bool h_MOV_B_A(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.B, regs.A); return true; }
static bool h_MOV_C_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.C, regs.B); return true; }
static bool h_MOV_C_C(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.C, regs.C); return true; }
static bool h_MOV_C_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.C, regs.D); return true; }
static bool h_MOV_C_E(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.C, regs.E); return true; }
static bool h_MOV_C_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.C, regs.H); return true; }
static bool h_MOV_C_L(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.C, regs.L); return true; }
static bool h_MOV_C_M(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.C, memory[(regs.H << 8) | regs.L]); return true; }
static bool h_MOV_C_A(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.C, regs.A); return true; }
static bool h_MOV_D_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.D, regs.B); return true; }
static bool h_MOV_D_C(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.D, regs.C); return true; }
static bool h_MOV_D_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.D, regs.D); return true; }
static bool h_MOV_D_E(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.D, regs.E); return true; }
static bool h_MOV_D_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.D, regs.H); return true; }
static bool h_MOV_D_L(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.D, regs.L); return true; }
static bool h_MOV_D_M(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.D, memory[(regs.H << 8) | regs.L]); return true; }
static bool h_MOV_D_A(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.D, regs.A); return true; }
static bool h_MOV_E_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.E, regs.B); return true; }
static bool h_MOV_E_C(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.E, regs.C); return true; }
static bool h_MOV_E_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.E, regs.D); return true; }
static bool h_MOV_E_E(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.E, regs.E); return true; }
static bool h_MOV_E_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.E, regs.H); return true; }
static bool h_MOV_E_L(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.E, regs.L); return true; }
static bool h_MOV_E_M(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.E, memory[(regs.H << 8) | regs.L]); return true; }
static bool h_MOV_E_A(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.E, regs.A); return true; }
static bool h_MOV_H_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.H, regs.B); return true; }
static bool h_MOV_H_C(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.H, regs.C); return true; }
static bool h_MOV_H_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.H, regs.D); return true; }
static bool h_MOV_H_E(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.H, regs.E); return true; }
static bool h_MOV_H_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.H, regs.H); return true; }
static bool h_MOV_H_L(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.H, regs.L); return true; }
static bool h_MOV_H_M(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.H, memory[(regs.H << 8) | regs.L]); return true; }
static bool h_MOV_H_A(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.H, regs.A); return true; }
static bool h_MOV_L_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.L, regs.B); return true; }
static bool h_MOV_L_C(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.L, regs.C); return true; }
static bool h_MOV_L_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.L, regs.D); return true; }
static bool h_MOV_L_E(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.L, regs.E); return true; }
static bool h_MOV_L_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.L, regs.H); return true; }
static bool h_MOV_L_L(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.L, regs.L); return true; }
static bool h_MOV_L_M(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.L, memory[(regs.H << 8) | regs.L]); return true; }
static bool h_MOV_L_A(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.L, regs.A); return true; }
static bool h_MOV_M_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(memory[(regs.H << 8) | regs.L], regs.B); return true; }
static bool h_MOV_M_C(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(memory[(regs.H << 8) | regs.L], regs.C); return true; }
static bool h_MOV_M_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(memory[(regs.H << 8) | regs.L], regs.D); return true; }
static bool h_MOV_M_E(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(memory[(regs.H << 8) | regs.L], regs.E); return true; }
static bool h_MOV_M_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(memory[(regs.H << 8) | regs.L], regs.H); return true; }
static bool h_MOV_M_L(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(memory[(regs.H << 8) | regs.L], regs.L); return true; }
static bool h_MOV_M_A(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(memory[(regs.H << 8) | regs.L], regs.A); return true; }
static bool h_MOV_A_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.A, regs.B); return true; }
static bool h_MOV_A_C(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.A, regs.C); return true; }
static bool h_MOV_A_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.A, regs.D); return true; }
static bool h_MOV_A_E(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.A, regs.E); return true; }
static bool h_MOV_A_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.A, regs.H); return true; }
static bool h_MOV_A_L(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.A, regs.L); return true; }
static bool h_MOV_A_M(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.A, memory[(regs.H << 8) | regs.L]); return true; }
static bool h_MOV_A_A(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_MOV(regs.A, regs.A); return true; }
static bool h_STAX_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_STAX(regs.B, regs.C); return true; }
static bool h_STAX_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_STAX(regs.D, regs.E); return true; }
static bool h_LDAX_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_LDAX(regs.B, regs.C); return true; }
static bool h_LDAX_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_LDAX(regs.D, regs.E); return true; }
static bool h_STA(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_STA(op1, op2); return true; }
static bool h_LDA(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_LDA(op1, op2); return true; }
static bool h_SHLD(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SHLD(op1, op2); return true; }
static bool h_LHLD(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_LHLD(op1, op2); return true; }
static bool h_XCHG(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_XCHG(); return true; }
static bool h_XTHL(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_XTHL(); return true; }
static bool h_PCHL(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_PCHL(); return true; }
static bool h_SPHL(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SPHL(); return true; }
static bool h_ADD_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ADD(regs.B); return true; }
static bool h_ADD_C(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ADD(regs.C); return true; }
static bool h_ADD_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ADD(regs.D); return true; }
static bool h_ADD_E(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ADD(regs.E); return true; }
static bool h_ADD_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ADD(regs.H); return true; }
static bool h_ADD_L(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ADD(regs.L); return true; }
static bool h_ADD_M(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ADD(memory[(regs.H << 8) | regs.L]); return true; }
static bool h_ADD_A(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ADD(regs.A); return true; }
static bool h_ADC_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ADC(regs.B); return true; }
static bool h_ADC_C(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ADC(regs.C); return true; }
static bool h_ADC_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ADC(regs.D); return true; }
static bool h_ADC_E(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ADC(regs.E); return true; }
static bool h_ADC_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ADC(regs.H); return true; }
static bool h_ADC_L(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ADC(regs.L); return true; }
static bool h_ADC_M(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ADC(memory[(regs.H << 8) | regs.L]); return true; }
static bool h_ADC_A(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ADC(regs.A); return true; }
static bool h_ADI(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ADI(op1); return true; }
static bool h_ACI(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ACI(op1); return true; }
static bool h_SUB_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SUB(regs.B); return true; }
static bool h_SUB_C(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SUB(regs.C); return true; }
static bool h_SUB_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SUB(regs.D); return true; }
static bool h_SUB_E(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SUB(regs.E); return true; }
static bool h_SUB_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SUB(regs.H); return true; }
static bool h_SUB_L(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SUB(regs.L); return true; }
static bool h_SUB_M(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SUB(memory[(regs.H << 8) | regs.L]); return true; }
static bool h_SUB_A(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SUB(regs.A); return true; }
static bool h_SBB_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SBB(regs.B); return true; }
static bool h_SBB_C(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SBB(regs.C); return true; }
static bool h_SBB_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SBB(regs.D); return true; }
static bool h_SBB_E(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SBB(regs.E); return true; }
static bool h_SBB_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SBB(regs.H); return true; }
static bool h_SBB_L(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SBB(regs.L); return true; }
static bool h_SBB_M(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SBB(memory[(regs.H << 8) | regs.L]); return true; }
static bool h_SBB_A(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SBB(regs.A); return true; }
static bool h_SUI(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SUI(op1); return true; }
static bool h_SBI(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_SBI(op1); return true; }
static bool h_INR_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_INR(regs.B); return true; }
static bool h_INR_C(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_INR(regs.C); return true; }
static bool h_INR_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_INR(regs.D); return true; }
static bool h_INR_E(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_INR(regs.E); return true; }
static bool h_INR_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_INR(regs.H); return true; }
static bool h_INR_L(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_INR(regs.L); return true; }
static bool h_INR_M(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_INR(memory[(regs.H << 8) | regs.L]); return true; }
static bool h_INR_A(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_INR(regs.A); return true; }
static bool h_DCR_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_DCR(regs.B); return true; }
static bool h_DCR_C(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_DCR(regs.C); return true; }
static bool h_DCR_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_DCR(regs.D); return true; }
static bool h_DCR_E(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_DCR(regs.E); return true; }
static bool h_DCR_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_DCR(regs.H); return true; }
static bool h_DCR_L(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_DCR(regs.L); return true; }
static bool h_DCR_M(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_DCR(memory[(regs.H << 8) | regs.L]); return true; }
static bool h_DCR_A(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_DCR(regs.A); return true; }
static bool h_INX_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; uint16_t v = BC(); op_INX(v); setBC(v); return true; }
static bool h_INX_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; uint16_t v = DE(); op_INX(v); setDE(v); return true; }
static bool h_INX_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; uint16_t v = HL(); op_INX(v); setHL(v); return true; }
static bool h_INX_SP(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_INX(SP); return true; }
static bool h_DCX_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; uint16_t v = BC(); op_DCX(v); setBC(v); return true; }
static bool h_DCX_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; uint16_t v = DE(); op_DCX(v); setDE(v); return true; }
static bool h_DCX_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; uint16_t v = HL(); op_DCX(v); setHL(v); return true; }
static bool h_DCX_SP(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_DCX(SP); return true; }
static bool h_DAD_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_DAD(BC()); return true; }
static bool h_DAD_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_DAD(DE()); return true; }
static bool h_DAD_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_DAD(HL()); return true; }
static bool h_DAD_SP(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_DAD(SP); return true; }
static bool h_DAA(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_DAA(); return true; }
static bool h_ANA_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ANA(regs.B); return true; }
static bool h_ANA_C(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ANA(regs.C); return true; }
static bool h_ANA_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ANA(regs.D); return true; }
static bool h_ANA_E(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ANA(regs.E); return true; }
static bool h_ANA_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ANA(regs.H); return true; }
static bool h_ANA_L(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ANA(regs.L); return true; }
static bool h_ANA_M(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ANA(memory[(regs.H << 8) | regs.L]); return true; }
static bool h_ANA_A(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ANA(regs.A); return true; }
static bool h_XRA_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_XRA(regs.B); return true; }
static bool h_XRA_C(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_XRA(regs.C); return true; }
static bool h_XRA_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_XRA(regs.D); return true; }
static bool h_XRA_E(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_XRA(regs.E); return true; }
static bool h_XRA_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_XRA(regs.H); return true; }
static bool h_XRA_L(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_XRA(regs.L); return true; }
static bool h_XRA_M(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_XRA(memory[(regs.H << 8) | regs.L]); return true; }
static bool h_XRA_A(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_XRA(regs.A); return true; }
static bool h_ORA_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ORA(regs.B); return true; }
static bool h_ORA_C(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ORA(regs.C); return true; }
static bool h_ORA_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ORA(regs.D); return true; }
static bool h_ORA_E(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ORA(regs.E); return true; }
static bool h_ORA_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ORA(regs.H); return true; }
static bool h_ORA_L(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ORA(regs.L); return true; }
static bool h_ORA_M(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ORA(memory[(regs.H << 8) | regs.L]); return true; }
static bool h_ORA_A(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ORA(regs.A); return true; }
static bool h_CMP_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CMP(regs.B); return true; }
static bool h_CMP_C(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CMP(regs.C); return true; }
static bool h_CMP_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CMP(regs.D); return true; }
static bool h_CMP_E(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CMP(regs.E); return true; }
static bool h_CMP_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CMP(regs.H); return true; }
static bool h_CMP_L(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CMP(regs.L); return true; }
static bool h_CMP_M(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CMP(memory[(regs.H << 8) | regs.L]); return true; }
static bool h_CMP_A(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CMP(regs.A); return true; }
static bool h_ANI(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ANI(op1); return true; }
static bool h_XRI(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_XRI(op1); return true; }
static bool h_ORI(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_ORI(op1); return true; }
static bool h_CPI(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CPI(op1); return true; }
static bool h_CMA(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CMA(); return true; }
static bool h_STC(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_STC(); return true; }
static bool h_CMC(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CMC(); return true; }
static bool h_RLC(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RLC(); return true; }
static bool h_RRC(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RRC(); return true; }
static bool h_RAL(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RAL(); return true; }
static bool h_RAR(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RAR(); return true; }
static bool h_JMP(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_JMP(op1, op2); return true; }
static bool h_JC(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_JC (op1, op2); return true; }
static bool h_JNC(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_JNC(op1, op2); return true; }
static bool h_JZ(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_JZ (op1, op2); return true; }
static bool h_JNZ(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_JNZ(op1, op2); return true; }
static bool h_JPE(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_JPE(op1, op2); return true; }
static bool h_JPO(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_JPO(op1, op2); return true; }
static bool h_JM(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_JM (op1, op2); return true; }
static bool h_JP(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_JP (op1, op2); return true; }
static bool h_CALL(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CALL(op1, op2); return true; }
static bool h_CC(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CC (op1, op2); return true; }
static bool h_CNC(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CNC(op1, op2); return true; }
static bool h_CZ(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CZ (op1, op2); return true; }
static bool h_CNZ(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CNZ(op1, op2); return true; }
static bool h_CPO(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CPO(op1, op2); return true; }
static bool h_CPE(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CPE(op1, op2); return true; }
static bool h_CM(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CM (op1, op2); return true; }
static bool h_CP(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_CP (op1, op2); return true; }
static bool h_RET(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RET(); return true; }
static bool h_RC(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RC(); return true; }
static bool h_RNC(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RNC(); return true; }
static bool h_RZ(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RZ(); return true; }
static bool h_RNZ(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RNZ(); return true; }
static bool h_RM(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RM(); return true; }
static bool h_RP(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RP(); return true; }
static bool h_RPE(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RPE(); return true; }
static bool h_RPO(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RPO(); return true; }
static bool h_RST_0(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RST(0); return true; }
static bool h_RST_1(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RST(1); return true; }
static bool h_RST_2(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RST(2); return true; }
static bool h_RST_3(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RST(3); return true; }
static bool h_RST_4(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RST(4); return true; }
static bool h_RST_5(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RST(5); return true; }
static bool h_RST_6(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RST(6); return true; }
static bool h_RST_7(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_RST(7); return true; }
static bool h_PUSH_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_PUSH(regs.B, regs.C); return true; }
static bool h_PUSH_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_PUSH(regs.D, regs.E); return true; }
static bool h_PUSH_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_PUSH(regs.H, regs.L); return true; }
static bool h_PUSH_PSW(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_PUSH_PSW(); return true; }
static bool h_POP_B(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_POP(regs.B, regs.C); return true; }
static bool h_POP_D(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_POP(regs.D, regs.E); return true; }
static bool h_POP_H(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_POP(regs.H, regs.L); return true; }
static bool h_POP_PSW(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_POP_PSW(); return true; }
static bool h_OUT(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_OUT(op1); return true; }
static bool h_IN(uint8_t op1, uint8_t op2) { (void)op1; (void)op2; op_IN(op1); return true; }

static bool h_undefined(uint8_t opcode)
{
    std::cerr << "Unimplemented opcode: 0x" << std::hex << (int)opcode
              << " at PC=0x" << PC << "\n";
    return false;
}

static std::array<OpHandler, 256> build_dispatch()
{
    std::array<OpHandler, 256> table{}; // zero-initialized -> nullptr for unmapped slots
    table[OP_NOP] = h_NOP;
    table[OP_HLT] = h_HLT;
    table[OP_EI] = h_EI;
    table[OP_DI] = h_DI;
    table[OP_EXIT] = h_EXIT;
    table[OP_LXI_B] = h_LXI_B;
    table[OP_LXI_D] = h_LXI_D;
    table[OP_LXI_H] = h_LXI_H;
    table[OP_LXI_SP] = h_LXI_SP;
    table[OP_MVI_B] = h_MVI_B;
    table[OP_MVI_C] = h_MVI_C;
    table[OP_MVI_D] = h_MVI_D;
    table[OP_MVI_E] = h_MVI_E;
    table[OP_MVI_H] = h_MVI_H;
    table[OP_MVI_L] = h_MVI_L;
    table[OP_MVI_M] = h_MVI_M;
    table[OP_MVI_A] = h_MVI_A;
    table[OP_MOV_B_B] = h_MOV_B_B;
    table[OP_MOV_B_C] = h_MOV_B_C;
    table[OP_MOV_B_D] = h_MOV_B_D;
    table[OP_MOV_B_E] = h_MOV_B_E;
    table[OP_MOV_B_H] = h_MOV_B_H;
    table[OP_MOV_B_L] = h_MOV_B_L;
    table[OP_MOV_B_M] = h_MOV_B_M;
    table[OP_MOV_B_A] = h_MOV_B_A;
    table[OP_MOV_C_B] = h_MOV_C_B;
    table[OP_MOV_C_C] = h_MOV_C_C;
    table[OP_MOV_C_D] = h_MOV_C_D;
    table[OP_MOV_C_E] = h_MOV_C_E;
    table[OP_MOV_C_H] = h_MOV_C_H;
    table[OP_MOV_C_L] = h_MOV_C_L;
    table[OP_MOV_C_M] = h_MOV_C_M;
    table[OP_MOV_C_A] = h_MOV_C_A;
    table[OP_MOV_D_B] = h_MOV_D_B;
    table[OP_MOV_D_C] = h_MOV_D_C;
    table[OP_MOV_D_D] = h_MOV_D_D;
    table[OP_MOV_D_E] = h_MOV_D_E;
    table[OP_MOV_D_H] = h_MOV_D_H;
    table[OP_MOV_D_L] = h_MOV_D_L;
    table[OP_MOV_D_M] = h_MOV_D_M;
    table[OP_MOV_D_A] = h_MOV_D_A;
    table[OP_MOV_E_B] = h_MOV_E_B;
    table[OP_MOV_E_C] = h_MOV_E_C;
    table[OP_MOV_E_D] = h_MOV_E_D;
    table[OP_MOV_E_E] = h_MOV_E_E;
    table[OP_MOV_E_H] = h_MOV_E_H;
    table[OP_MOV_E_L] = h_MOV_E_L;
    table[OP_MOV_E_M] = h_MOV_E_M;
    table[OP_MOV_E_A] = h_MOV_E_A;
    table[OP_MOV_H_B] = h_MOV_H_B;
    table[OP_MOV_H_C] = h_MOV_H_C;
    table[OP_MOV_H_D] = h_MOV_H_D;
    table[OP_MOV_H_E] = h_MOV_H_E;
    table[OP_MOV_H_H] = h_MOV_H_H;
    table[OP_MOV_H_L] = h_MOV_H_L;
    table[OP_MOV_H_M] = h_MOV_H_M;
    table[OP_MOV_H_A] = h_MOV_H_A;
    table[OP_MOV_L_B] = h_MOV_L_B;
    table[OP_MOV_L_C] = h_MOV_L_C;
    table[OP_MOV_L_D] = h_MOV_L_D;
    table[OP_MOV_L_E] = h_MOV_L_E;
    table[OP_MOV_L_H] = h_MOV_L_H;
    table[OP_MOV_L_L] = h_MOV_L_L;
    table[OP_MOV_L_M] = h_MOV_L_M;
    table[OP_MOV_L_A] = h_MOV_L_A;
    table[OP_MOV_M_B] = h_MOV_M_B;
    table[OP_MOV_M_C] = h_MOV_M_C;
    table[OP_MOV_M_D] = h_MOV_M_D;
    table[OP_MOV_M_E] = h_MOV_M_E;
    table[OP_MOV_M_H] = h_MOV_M_H;
    table[OP_MOV_M_L] = h_MOV_M_L;
    table[OP_MOV_M_A] = h_MOV_M_A;
    table[OP_MOV_A_B] = h_MOV_A_B;
    table[OP_MOV_A_C] = h_MOV_A_C;
    table[OP_MOV_A_D] = h_MOV_A_D;
    table[OP_MOV_A_E] = h_MOV_A_E;
    table[OP_MOV_A_H] = h_MOV_A_H;
    table[OP_MOV_A_L] = h_MOV_A_L;
    table[OP_MOV_A_M] = h_MOV_A_M;
    table[OP_MOV_A_A] = h_MOV_A_A;
    table[OP_STAX_B] = h_STAX_B;
    table[OP_STAX_D] = h_STAX_D;
    table[OP_LDAX_B] = h_LDAX_B;
    table[OP_LDAX_D] = h_LDAX_D;
    table[OP_STA] = h_STA;
    table[OP_LDA] = h_LDA;
    table[OP_SHLD] = h_SHLD;
    table[OP_LHLD] = h_LHLD;
    table[OP_XCHG] = h_XCHG;
    table[OP_XTHL] = h_XTHL;
    table[OP_PCHL] = h_PCHL;
    table[OP_SPHL] = h_SPHL;
    table[OP_ADD_B] = h_ADD_B;
    table[OP_ADD_C] = h_ADD_C;
    table[OP_ADD_D] = h_ADD_D;
    table[OP_ADD_E] = h_ADD_E;
    table[OP_ADD_H] = h_ADD_H;
    table[OP_ADD_L] = h_ADD_L;
    table[OP_ADD_M] = h_ADD_M;
    table[OP_ADD_A] = h_ADD_A;
    table[OP_ADC_B] = h_ADC_B;
    table[OP_ADC_C] = h_ADC_C;
    table[OP_ADC_D] = h_ADC_D;
    table[OP_ADC_E] = h_ADC_E;
    table[OP_ADC_H] = h_ADC_H;
    table[OP_ADC_L] = h_ADC_L;
    table[OP_ADC_M] = h_ADC_M;
    table[OP_ADC_A] = h_ADC_A;
    table[OP_ADI] = h_ADI;
    table[OP_ACI] = h_ACI;
    table[OP_SUB_B] = h_SUB_B;
    table[OP_SUB_C] = h_SUB_C;
    table[OP_SUB_D] = h_SUB_D;
    table[OP_SUB_E] = h_SUB_E;
    table[OP_SUB_H] = h_SUB_H;
    table[OP_SUB_L] = h_SUB_L;
    table[OP_SUB_M] = h_SUB_M;
    table[OP_SUB_A] = h_SUB_A;
    table[OP_SBB_B] = h_SBB_B;
    table[OP_SBB_C] = h_SBB_C;
    table[OP_SBB_D] = h_SBB_D;
    table[OP_SBB_E] = h_SBB_E;
    table[OP_SBB_H] = h_SBB_H;
    table[OP_SBB_L] = h_SBB_L;
    table[OP_SBB_M] = h_SBB_M;
    table[OP_SBB_A] = h_SBB_A;
    table[OP_SUI] = h_SUI;
    table[OP_SBI] = h_SBI;
    table[OP_INR_B] = h_INR_B;
    table[OP_INR_C] = h_INR_C;
    table[OP_INR_D] = h_INR_D;
    table[OP_INR_E] = h_INR_E;
    table[OP_INR_H] = h_INR_H;
    table[OP_INR_L] = h_INR_L;
    table[OP_INR_M] = h_INR_M;
    table[OP_INR_A] = h_INR_A;
    table[OP_DCR_B] = h_DCR_B;
    table[OP_DCR_C] = h_DCR_C;
    table[OP_DCR_D] = h_DCR_D;
    table[OP_DCR_E] = h_DCR_E;
    table[OP_DCR_H] = h_DCR_H;
    table[OP_DCR_L] = h_DCR_L;
    table[OP_DCR_M] = h_DCR_M;
    table[OP_DCR_A] = h_DCR_A;
    table[OP_INX_B] = h_INX_B;
    table[OP_INX_D] = h_INX_D;
    table[OP_INX_H] = h_INX_H;
    table[OP_INX_SP] = h_INX_SP;
    table[OP_DCX_B] = h_DCX_B;
    table[OP_DCX_D] = h_DCX_D;
    table[OP_DCX_H] = h_DCX_H;
    table[OP_DCX_SP] = h_DCX_SP;
    table[OP_DAD_B] = h_DAD_B;
    table[OP_DAD_D] = h_DAD_D;
    table[OP_DAD_H] = h_DAD_H;
    table[OP_DAD_SP] = h_DAD_SP;
    table[OP_DAA] = h_DAA;
    table[OP_ANA_B] = h_ANA_B;
    table[OP_ANA_C] = h_ANA_C;
    table[OP_ANA_D] = h_ANA_D;
    table[OP_ANA_E] = h_ANA_E;
    table[OP_ANA_H] = h_ANA_H;
    table[OP_ANA_L] = h_ANA_L;
    table[OP_ANA_M] = h_ANA_M;
    table[OP_ANA_A] = h_ANA_A;
    table[OP_XRA_B] = h_XRA_B;
    table[OP_XRA_C] = h_XRA_C;
    table[OP_XRA_D] = h_XRA_D;
    table[OP_XRA_E] = h_XRA_E;
    table[OP_XRA_H] = h_XRA_H;
    table[OP_XRA_L] = h_XRA_L;
    table[OP_XRA_M] = h_XRA_M;
    table[OP_XRA_A] = h_XRA_A;
    table[OP_ORA_B] = h_ORA_B;
    table[OP_ORA_C] = h_ORA_C;
    table[OP_ORA_D] = h_ORA_D;
    table[OP_ORA_E] = h_ORA_E;
    table[OP_ORA_H] = h_ORA_H;
    table[OP_ORA_L] = h_ORA_L;
    table[OP_ORA_M] = h_ORA_M;
    table[OP_ORA_A] = h_ORA_A;
    table[OP_CMP_B] = h_CMP_B;
    table[OP_CMP_C] = h_CMP_C;
    table[OP_CMP_D] = h_CMP_D;
    table[OP_CMP_E] = h_CMP_E;
    table[OP_CMP_H] = h_CMP_H;
    table[OP_CMP_L] = h_CMP_L;
    table[OP_CMP_M] = h_CMP_M;
    table[OP_CMP_A] = h_CMP_A;
    table[OP_ANI] = h_ANI;
    table[OP_XRI] = h_XRI;
    table[OP_ORI] = h_ORI;
    table[OP_CPI] = h_CPI;
    table[OP_CMA] = h_CMA;
    table[OP_STC] = h_STC;
    table[OP_CMC] = h_CMC;
    table[OP_RLC] = h_RLC;
    table[OP_RRC] = h_RRC;
    table[OP_RAL] = h_RAL;
    table[OP_RAR] = h_RAR;
    table[OP_JMP] = h_JMP;
    table[OP_JC] = h_JC;
    table[OP_JNC] = h_JNC;
    table[OP_JZ] = h_JZ;
    table[OP_JNZ] = h_JNZ;
    table[OP_JPE] = h_JPE;
    table[OP_JPO] = h_JPO;
    table[OP_JM] = h_JM;
    table[OP_JP] = h_JP;
    table[OP_CALL] = h_CALL;
    table[OP_CC] = h_CC;
    table[OP_CNC] = h_CNC;
    table[OP_CZ] = h_CZ;
    table[OP_CNZ] = h_CNZ;
    table[OP_CPO] = h_CPO;
    table[OP_CPE] = h_CPE;
    table[OP_CM] = h_CM;
    table[OP_CP] = h_CP;
    table[OP_RET] = h_RET;
    table[OP_RC] = h_RC;
    table[OP_RNC] = h_RNC;
    table[OP_RZ] = h_RZ;
    table[OP_RNZ] = h_RNZ;
    table[OP_RM] = h_RM;
    table[OP_RP] = h_RP;
    table[OP_RPE] = h_RPE;
    table[OP_RPO] = h_RPO;
    table[OP_RST_0] = h_RST_0;
    table[OP_RST_1] = h_RST_1;
    table[OP_RST_2] = h_RST_2;
    table[OP_RST_3] = h_RST_3;
    table[OP_RST_4] = h_RST_4;
    table[OP_RST_5] = h_RST_5;
    table[OP_RST_6] = h_RST_6;
    table[OP_RST_7] = h_RST_7;
    table[OP_PUSH_B] = h_PUSH_B;
    table[OP_PUSH_D] = h_PUSH_D;
    table[OP_PUSH_H] = h_PUSH_H;
    table[OP_PUSH_PSW] = h_PUSH_PSW;
    table[OP_POP_B] = h_POP_B;
    table[OP_POP_D] = h_POP_D;
    table[OP_POP_H] = h_POP_H;
    table[OP_POP_PSW] = h_POP_PSW;
    table[OP_OUT] = h_OUT;
    table[OP_IN] = h_IN;    return table;
}

static const std::array<OpHandler, 256> dispatch = build_dispatch();

bool step()
{
// ── CP/M Program Exit Interception ───────────────────────────────────────
    if (PC == 0x0000)
    {
        std::cout << "\nProgram terminated cleanly via warm boot (PC=0x0000).\n";
        return false; // Returning false breaks the main loop and exits the emulator
    }

// ── CP/M bdos interception for cpudiag.bin ───────────────────────────────
    if (PC == 0x0005)
    {
        if (regs.C == 9)
        {
            // Register DE points to a '$'-terminated string in memory
            uint16_t strAddr = ((uint16_t)regs.D << 8) | regs.E;
            while (memory[strAddr] != '$')
            {
                std::cout << (char)memory[strAddr];
                strAddr++;
            }
        }
        else if (regs.C == 2)
        {
            // Register E contains a single character to print
            std::cout << (char)regs.E;
        }

        // Simulate an immediate RET instruction to jump back to the caller
        // (Pop the return address off the stack)
        uint16_t lo = memory[SP];
        uint16_t hi = memory[SP + 1];
        SP += 2;
        PC = (hi << 8) | lo;
        return true;
    }

    // interrupt check
    if (interrupts.pending && interrupts.enabled)
    {
        interrupts.enabled  = false; // INTE clears when interrupt is accepted
        interrupts.pending  = false;
        interrupts.halted   = false; // HLT is cancelled by an interrupt

        // the vector byte on the data bus is treated as an RST instruction.
        // the hardware puts 0xC7 | (n<<3) on the bus for RST n.
        uint8_t n = (interrupts.vector >> 3) & 0x07;
        op_RST(n);
        return true;
    }

    // if halted and no interrupt, do nothing
    if (interrupts.halted) return true;

    // fetch instruction
    uint8_t opcode = memory[PC];
    uint8_t op1    = memory[PC + 1];
    uint8_t op2    = memory[PC + 2];

    std::cout << "[ "
              << std::left << std::setw(12) << std::setfill(' ') << MNEMONICS[opcode]
              << " ] 0x"
              << std::hex << std::setw(2) << std::setfill('0') << (int)opcode
              << "  PC=0x" << std::setw(4) << PC
              << "\n";

    // decode and exectuet
    OpHandler handler = dispatch[opcode];
    if (!handler)
        return h_undefined(opcode);

    return handler(op1, op2);
}

#pragma once
#include <cstdint>

// Physical 64KB memory array
inline uint8_t memory[65536] = {0};

// CPU State Components
struct Registers {
    uint8_t A, B, C, D, E, H, L;
};

struct Flags
{
    uint8_t Zero;
    uint8_t Sign;
    uint8_t Parity;
    uint8_t Carry;
    uint8_t AuxCarry;
};

struct InterruptState
{
    bool enabled; // INTE flip-flop - set by EI, cleared by DI or accepted interrupt
    bool halted;  // CPU stopped by HLT, resumes on interrupt
    bool pending; // set externally when an interrupt is asserted
    uint8_t vector; // RST vector byte to execute on next step
};

inline uint16_t PC = 0x0000;
inline uint16_t SP = 0x0000;
inline Registers regs = {};
inline Flags flags = {};
inline InterruptState interrupts = {};

static const char* MNEMONICS[] =
{
//     x0               x1         x2         x3         x4         x5            x6     x7          x8           x9          xA         xB         xC         xD            xE     xF
    "NOP",     "LXI B,D16",    "STAX B",   "INX B",   "INR B",   "DCR B",     "MVI B,D8", "RLC",      "NOP",     "DAD B",   "LDAX B",   "DCX B",   "INR C",   "DCR C",    "MVI C,D8", "RRC",      //0x
    "NOP",     "LXI D,D16",    "STAX D",   "INX D",   "INR D",   "DCR D",     "MVI D,D8", "RAL",      "NOP",     "DAD D",   "LDAX D",   "DCX D",   "INR E",   "DCR E",    "MVI E,D8", "RAR",      //1x
    "NOP",     "LXI H,D16",    "SHLD A16", "INX H",   "INR H",   "DCR H",     "MVI H,D8", "DAA",      "NOP",     "DAD H",   "LHLD A16", "DCX H",   "INR L",   "DCR L",    "MVI L,D8", "RMA",      //2x
    "NOP",     "LXI SP,D16",   "STA A16",  "INX SP",  "INR M",   "DCR M",     "MVI M,D8", "STC",      "NOP",     "DAD SP",  "LDA A16",  "DCX SP",  "INR A",   "DCR A",    "MVI A,D8", "CMC",      //3x
    "MOV B,B", "MOV B,C",      "MOV B,D",  "MOV B,E", "MOV B,H", "MOV B,L",   "MOV B,M",   "MOV B,A", "MOV C,B", "MOV C,C", "MOV C,D",  "MOV C,E", "MOV C,H", "MOV C,L",  "MOV C,M",  "MOV C,A",  //4x
    "MOV D,B", "MOV D,C",      "MOV D,D",  "MOV D,E", "MOV D,H", "MOV D,L",   "MOV D,M",   "MOV D,A", "MOV E,B", "MOV E,C", "MOV E,D",  "MOV E,E", "MOV E,H", "MOV E,L",  "MOV E,M",  "MOV E,A",  //5x
    "MOV H,B", "MOV H,C",      "MOV H,D",  "MOV H,E", "MOV H,H", "MOV H,L",   "MOV H,M",   "MOV H,A", "MOV L,B", "MOV L,C", "MOV L,D",  "MOV L,E", "MOV L,H", "MOV L,L",  "MOV L,M",  "MOV L,A",  //6x
    "MOV M,B", "MOV M,C",      "MOV M,D",  "MOV M,E", "MOV M,H", "MOV M,L",   "HLT",       "MOV M,A", "MOV A,B", "MOV A,C", "MOV A,D",  "MOV A,E", "MOV A,H", "MOV A,L",  "MOV A,M",  "MOV A,A",  //7x
    "ADD B",   "ADD C",        "ADD D",    "ADD E",   "ADD H",   "ADD L",     "ADD M",     "ADD A",   "ADC B",   "ADC C",   "ADC D",    "ADC E",   "ADC H",   "ADC L",    "ADC M",    "ADC A",    //8x
    "SUB B",   "SUB C",        "SUB D",    "SUB E",   "SUB H",   "SUB L",     "SUB M",     "SUB A",   "SBB B",   "SBB C",   "SBB D",    "SBB E",   "SBB H",   "SBB L",    "SBB M",    "SBB A",    //9x
    "ANA B",   "ANA C",        "ANA D",    "ANA E",   "ANA H",   "ANA L",     "ANA M",     "ANA A",   "XRA B",   "XRA C",   "XRA D",    "XRA E",   "XRA H",   "XRA L",    "XRA M",    "XRA A",    //Ax
    "ORA B",   "ORA C",        "ORA D",    "ORA E",   "ORA H",   "ORA L",     "ORA M",     "ORA A",   "CMP B",   "CMP C",   "CMP D",    "CMP E",   "CMP H",   "CMP L",    "CMP M",    "CMP A",    //Bx
    "RNZ",     "POP B",        "JNZ A16",  "JMP A16", "CNZ A16", "PUSH B",    "ADI D8",    "RST 0",   "RZ",      "RET",     "JZ A16",   "JMP A16", "CZ A16",  "CALL A16", "ACI D8",   "RST 1",    //Cx
    "RNC",     "POP D",        "JNC A16",  "OUT D8",  "CNC A16", "PUSH D",    "AUI D8",    "RST 2",   "RC",      "RET",     "JC A16",   "IN  D8",  "CC A16",  "CALL A16", "SBI D8",   "RST 3",    //Dx
    "RPO",     "POP H",        "JPO A16",  "XTHL",    "CPO A16", "PUSH H",    "ANI D8",    "RST 4",   "RPE",     "PCHL",    "JPE A16",  "XCHG",    "CPE A16", "CALL A16", "XRI D8",   "RST 5",    //Ex
    "RP",      "POP PSW",      "JP  A16",  "DI",      "CP  A16", "PUSH PSW",  "ORI D8",    "RST 6",   "RM",      "SPHL",    "JM A16",   "EI",      "CM A16",  "CALL A16", "CPI D8",   "RST 7"     //Fx
};

typedef enum {
    // 0x0x
    OP_NOP         = 0x00,
    OP_LXI_B       = 0x01,
    OP_STAX_B      = 0x02,
    OP_INX_B       = 0x03,
    OP_INR_B       = 0x04,
    OP_DCR_B       = 0x05,
    OP_MVI_B       = 0x06,
    OP_RLC         = 0x07,

    OP_DAD_B       = 0x09,
    OP_LDAX_B      = 0x0A,
    OP_DCX_B       = 0x0B,
    OP_INR_C       = 0x0C,
    OP_DCR_C       = 0x0D,
    OP_MVI_C       = 0x0E,
    OP_RRC         = 0x0F,

    // 0x1x
    OP_LXI_D       = 0x11,
    OP_STAX_D      = 0x12,
    OP_INX_D       = 0x13,
    OP_INR_D       = 0x14,
    OP_DCR_D       = 0x15,
    OP_MVI_D       = 0x16,
    OP_RAL         = 0x17,
    OP_DAD_D       = 0x19,
    OP_LDAX_D      = 0x1A,
    OP_DCX_D       = 0x1B,
    OP_INR_E       = 0x1C,
    OP_DCR_E       = 0x1D,
    OP_MVI_E       = 0x1E,
    OP_RAR         = 0x1F,

    // 0x2x
    OP_LXI_H       = 0x21,
    OP_SHLD        = 0x22,
    OP_INX_H       = 0x23,
    OP_INR_H       = 0x24,
    OP_DCR_H       = 0x25,
    OP_MVI_H       = 0x26,
    OP_DAA         = 0x27,
    OP_DAD_H       = 0x29,
    OP_LHLD        = 0x2A,
    OP_DCX_H       = 0x2B,
    OP_INR_L       = 0x2C,
    OP_DCR_L       = 0x2D,
    OP_MVI_L       = 0x2E,
    OP_CMA         = 0x2F,

    // 0x3x
    OP_LXI_SP      = 0x31,
    OP_STA         = 0x32,
    OP_INX_SP      = 0x33,
    OP_INR_M       = 0x34,
    OP_DCR_M       = 0x35,
    OP_MVI_M       = 0x36,
    OP_STC         = 0x37,
    OP_DAD_SP      = 0x39,
    OP_LDA         = 0x3A,
    OP_DCX_SP      = 0x3B,
    OP_INR_A       = 0x3C,
    OP_DCR_A       = 0x3D,
    OP_MVI_A       = 0x3E,
    OP_CMC         = 0x3F,

    // 0x4x
    OP_MOV_B_B     = 0x40, OP_MOV_B_C = 0x41, OP_MOV_B_D = 0x42, OP_MOV_B_E = 0x43,
    OP_MOV_B_H     = 0x44, OP_MOV_B_L = 0x45, OP_MOV_B_M = 0x46, OP_MOV_B_A = 0x47,
    OP_MOV_C_B     = 0x48, OP_MOV_C_C = 0x49, OP_MOV_C_D = 0x4A, OP_MOV_C_E = 0x4B,
    OP_MOV_C_H     = 0x4C, OP_MOV_C_L = 0x4D, OP_MOV_C_M = 0x4E, OP_MOV_C_A = 0x4F,

    // 0x5x
    OP_MOV_D_B     = 0x50, OP_MOV_D_C = 0x51, OP_MOV_D_D = 0x52, OP_MOV_D_E = 0x53,
    OP_MOV_D_H     = 0x54, OP_MOV_D_L = 0x55, OP_MOV_D_M = 0x56, OP_MOV_D_A = 0x57,
    OP_MOV_E_B     = 0x58, OP_MOV_E_C = 0x59, OP_MOV_E_D = 0x5A, OP_MOV_E_E = 0x5B,
    OP_MOV_E_H     = 0x5C, OP_MOV_E_L = 0x5D, OP_MOV_E_M = 0x5E, OP_MOV_E_A = 0x5F,

    // 0x6x
    OP_MOV_H_B     = 0x60, OP_MOV_H_C = 0x61, OP_MOV_H_D = 0x62, OP_MOV_H_E = 0x63,
    OP_MOV_H_H     = 0x64, OP_MOV_H_L = 0x65, OP_MOV_H_M = 0x66, OP_MOV_H_A = 0x67,
    OP_MOV_L_B     = 0x68, OP_MOV_L_C = 0x69, OP_MOV_L_D = 0x6A, OP_MOV_L_E = 0x6B,
    OP_MOV_L_H     = 0x6C, OP_MOV_L_L = 0x6D, OP_MOV_L_M = 0x6E, OP_MOV_L_A = 0x6F,

    // 0x7x
    OP_MOV_M_B     = 0x70, OP_MOV_M_C = 0x71, OP_MOV_M_D = 0x72, OP_MOV_M_E = 0x73,
    OP_MOV_M_H     = 0x74, OP_MOV_M_L = 0x75, OP_HLT = 0x76,       OP_MOV_M_A = 0x77,
    OP_MOV_A_B     = 0x78, OP_MOV_A_C = 0x79, OP_MOV_A_D = 0x7A, OP_MOV_A_E = 0x7B,
    OP_MOV_A_H     = 0x7C, OP_MOV_A_L = 0x7D, OP_MOV_A_M = 0x7E, OP_MOV_A_A = 0x7F,

    // 0x8x
    OP_ADD_B       = 0x80, OP_ADD_C = 0x81, OP_ADD_D = 0x82, OP_ADD_E = 0x83,
    OP_ADD_H       = 0x84, OP_ADD_L = 0x85, OP_ADD_M = 0x86, OP_ADD_A = 0x87,
    OP_ADC_B       = 0x88, OP_ADC_C = 0x89, OP_ADC_D = 0x8A, OP_ADC_E = 0x8B,
    OP_ADC_H       = 0x8C, OP_ADC_L = 0x8D, OP_ADC_M = 0x8E, OP_ADC_A = 0x8F,

    // 0x9x
    OP_SUB_B       = 0x90, OP_SUB_C = 0x91, OP_SUB_D = 0x92, OP_SUB_E = 0x93,
    OP_SUB_H       = 0x94, OP_SUB_L = 0x95, OP_SUB_M = 0x96, OP_SUB_A = 0x97,
    OP_SBB_B       = 0x98, OP_SBB_C = 0x99, OP_SBB_D = 0x9A, OP_SBB_E = 0x9B,
    OP_SBB_H       = 0x9C, OP_SBB_L = 0x9D, OP_SBB_M = 0x9E, OP_SBB_A = 0x9F,

    // 0xAx
    OP_ANA_B       = 0xA0, OP_ANA_C = 0xA1, OP_ANA_D = 0xA2, OP_ANA_E = 0xA3,
    OP_ANA_H       = 0xA4, OP_ANA_L = 0xA5, OP_ANA_M = 0xA6, OP_ANA_A = 0xA7,
    OP_XRA_B       = 0xA8, OP_XRA_C = 0xA9, OP_XRA_D = 0xAA, OP_XRA_E = 0xAB,
    OP_XRA_H       = 0xAC, OP_XRA_L = 0xAD, OP_XRA_M = 0xAE, OP_XRA_A = 0xAF,

    // 0xBx
    OP_ORA_B       = 0xB0, OP_ORA_C = 0xB1, OP_ORA_D = 0xB2, OP_ORA_E = 0xB3,
    OP_ORA_H       = 0xB4, OP_ORA_L = 0xB5, OP_ORA_M = 0xB6, OP_ORA_A = 0xB7,
    OP_CMP_B       = 0xB8, OP_CMP_C = 0xB9, OP_CMP_D = 0xBA, OP_CMP_E = 0xBB,
    OP_CMP_H       = 0xBC, OP_CMP_L = 0xBD, OP_CMP_M = 0xBE, OP_CMP_A = 0xBF,

    // 0xCx
    OP_RNZ         = 0xC0,
    OP_POP_B       = 0xC1,
    OP_JNZ         = 0xC2,
    OP_JMP         = 0xC3,
    OP_CNZ         = 0xC4,
    OP_PUSH_B      = 0xC5,
    OP_ADI         = 0xC6,
    OP_RST_0       = 0xC7,
    OP_RZ          = 0xC8,
    OP_RET         = 0xC9,
    OP_JZ          = 0xCA,
    OP_CZ          = 0xCC,
    OP_CALL        = 0xCD,
    OP_ACI         = 0xCE,
    OP_RST_1       = 0xCF,

    // 0xDx
    OP_RNC         = 0xD0,
    OP_POP_D       = 0xD1,
    OP_JNC         = 0xD2,
    OP_OUT         = 0xD3,
    OP_CNC         = 0xD4,
    OP_PUSH_D      = 0xD5,
    OP_SUI         = 0xD6,
    OP_RST_2       = 0xD7,
    OP_RC          = 0xD8,
    OP_JC          = 0xDA,
    OP_IN          = 0xDB,
    OP_CC          = 0xDC,
    OP_SBI         = 0xDE,
    OP_RST_3       = 0xDF,

    // 0xEx
    OP_RPO         = 0xE0,
    OP_POP_H       = 0xE1,
    OP_JPO         = 0xE2,
    OP_XTHL        = 0xE3,
    OP_CPO         = 0xE4,
    OP_PUSH_H      = 0xE5,
    OP_ANI         = 0xE6,
    OP_RST_4       = 0xE7,
    OP_RPE         = 0xE8,
    OP_PCHL        = 0xE9,
    OP_JPE         = 0xEA,
    OP_XCHG        = 0xEB,
    OP_CPE         = 0xEC,
    OP_XRI         = 0xEE,
    OP_RST_5       = 0xEF,

    // 0xFx
    OP_RP          = 0xF0,
    OP_POP_PSW     = 0xF1,
    OP_JP          = 0xF2,
    OP_DI          = 0xF3,
    OP_CP          = 0xF4,
    OP_PUSH_PSW    = 0xF5,
    OP_ORI         = 0xF6,
    OP_RST_6       = 0xF7,
    OP_RM          = 0xF8,
    OP_SPHL        = 0xF9,
    OP_JM          = 0xFA,
    OP_EI          = 0xFB,
    OP_CM          = 0xFC,
    OP_EXIT        = 0xFD, // Custom mapping for exit handling
    OP_CPI         = 0xFE,
    OP_RST_7       = 0xFF
} OPCODES;

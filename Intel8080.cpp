#include "Intel8080.h"

const Intel8080::Instruction Intel8080::OPCODE_TABLE[256] =
{
    // [Opcode] = { "Mnemonic", Cycles, &Intel8080::HandlerMethod }

    // 0x00
    [0x00] = { "NOP",         4,  &Intel8080::op_NOP },
    [0x01] = { "LXI B,D16",  10,  &Intel8080::op_LXI_Reg },
    [0x02] = { "STAX B",      7,  &Intel8080::op_STAX },
    [0x03] = { "INX B",       5,  &Intel8080::op_INX },
    [0x04] = { "INR B",       5,  &Intel8080::op_INR },
    [0x05] = { "DCR B",       5,  &Intel8080::op_DCR },
    [0x06] = { "MVI B,D8",    7,  &Intel8080::op_MVI },
    [0x07] = { "RLC",         4,  &Intel8080::op_RLC },
    [0x08] = { "NOP",         4,  &Intel8080::op_NOP },
    [0x09] = { "DAD B",      10,  &Intel8080::op_DAD },
    [0x0A] = { "LDAX B",      7,  &Intel8080::op_LDAX },
    [0x0B] = { "DCX B",       5,  &Intel8080::op_DCX },
    [0x0C] = { "INR C",       5,  &Intel8080::op_INR },
    [0x0D] = { "DCR C",       5,  &Intel8080::op_DCR },
    [0x0E] = { "MVI C,D8",    7,  &Intel8080::op_MVI },
    [0x0F] = { "RRC",         4,  &Intel8080::op_RRC },

    // 0x10
    [0x10] = { "NOP",         4,  &Intel8080::op_NOP },
    [0x11] = { "LXI D,D16",  10,  &Intel8080::op_LXI_Reg },
    [0x12] = { "STAX D",      7,  &Intel8080::op_STAX },
    [0x13] = { "INX D",       5,  &Intel8080::op_INX },
    [0x14] = { "INR D",       5,  &Intel8080::op_INR },
    [0x15] = { "DCR D",       5,  &Intel8080::op_DCR },
    [0x16] = { "MVI D,D8",    7,  &Intel8080::op_MVI },
    [0x17] = { "RAL",         4,  &Intel8080::op_RAL },
    [0x18] = { "NOP",         4,  &Intel8080::op_NOP },
    [0x19] = { "DAD D",      10,  &Intel8080::op_DAD },
    [0x1A] = { "LDAX D",      7,  &Intel8080::op_LDAX },
    [0x1B] = { "DCX D",       5,  &Intel8080::op_DCX },
    [0x1C] = { "INR E",       5,  &Intel8080::op_INR },
    [0x1D] = { "DCR E",       5,  &Intel8080::op_DCR },
    [0x1E] = { "MVI E,D8",    7,  &Intel8080::op_MVI },
    [0x1F] = { "RAR",         4,  &Intel8080::op_RAR },

    // 0x20
    [0x20] = { "NOP",         4,  &Intel8080::op_NOP },
    [0x21] = { "LXI H,D16",  10,  &Intel8080::op_LXI_Reg },
    [0x22] = { "SHLD A16",   16,  &Intel8080::op_SHLD },
    [0x23] = { "INX H",       5,  &Intel8080::op_INX },
    [0x24] = { "INR H",       5,  &Intel8080::op_INR },
    [0x25] = { "DCR H",       5,  &Intel8080::op_DCR },
    [0x26] = { "MVI H,D8",    7,  &Intel8080::op_MVI },
    [0x27] = { "DAA",         4,  &Intel8080::op_DAA },
    [0x28] = { "NOP",         4,  &Intel8080::op_NOP },
    [0x29] = { "DAD H",      10,  &Intel8080::op_DAD },
    [0x2A] = { "LHLD A16",   16,  &Intel8080::op_LHLD },
    [0x2B] = { "DCX H",       5,  &Intel8080::op_DCX },
    [0x2C] = { "INR L",       5,  &Intel8080::op_INR },
    [0x2D] = { "DCR L",       5,  &Intel8080::op_DCR },
    [0x2E] = { "MVI L,D8",    7,  &Intel8080::op_MVI },
    [0x2F] = { "CMA",         4,  &Intel8080::op_CMA },

    // 0x30
    [0x30] = { "NOP",         4,  &Intel8080::op_NOP },
    [0x31] = { "LXI SP,D16", 10,  &Intel8080::op_LXI_SP },
    [0x32] = { "STA A16",    13,  &Intel8080::op_STA },
    [0x33] = { "INX SP",      5,  &Intel8080::op_INX },
    [0x34] = { "INR M",      10,  &Intel8080::op_INR },
    [0x35] = { "DCR M",      10,  &Intel8080::op_DCR },
    [0x36] = { "MVI M,D8",   10,  &Intel8080::op_MVI },
    [0x37] = { "STC",         4,  &Intel8080::op_STC },
    [0x38] = { "NOP",         4,  &Intel8080::op_NOP },
    [0x39] = { "DAD SP",     10,  &Intel8080::op_DAD },
    [0x3A] = { "LDA A16",    13,  &Intel8080::op_LDA },
    [0x3B] = { "DCX SP",      5,  &Intel8080::op_DCX },
    [0x3C] = { "INR A",       5,  &Intel8080::op_INR },
    [0x3D] = { "DCR A",       5,  &Intel8080::op_DCR },
    [0x3E] = { "MVI A,D8",    7,  &Intel8080::op_MVI },
    [0x3F] = { "CMC",         4,  &Intel8080::op_CMC },

    // 0x40
    [0x40] = { "MOV B,B",     5,  &Intel8080::op_MOV },
    [0x41] = { "MOV B,C",     5,  &Intel8080::op_MOV },
    [0x42] = { "MOV B,D",     5,  &Intel8080::op_MOV },
    [0x43] = { "MOV B,E",     5,  &Intel8080::op_MOV },
    [0x44] = { "MOV B,H",     5,  &Intel8080::op_MOV },
    [0x45] = { "MOV B,L",     5,  &Intel8080::op_MOV },
    [0x46] = { "MOV B,M",     7,  &Intel8080::op_MOV },
    [0x47] = { "MOV B,A",     5,  &Intel8080::op_MOV },
    [0x48] = { "MOV C,B",     5,  &Intel8080::op_MOV },
    [0x49] = { "MOV C,C",     5,  &Intel8080::op_MOV },
    [0x4A] = { "MOV C,D",     5,  &Intel8080::op_MOV },
    [0x4B] = { "MOV C,E",     5,  &Intel8080::op_MOV },
    [0x4C] = { "MOV C,H",     5,  &Intel8080::op_MOV },
    [0x4D] = { "MOV C,L",     5,  &Intel8080::op_MOV },
    [0x4E] = { "MOV C,M",     7,  &Intel8080::op_MOV },
    [0x4F] = { "MOV C,A",     5,  &Intel8080::op_MOV },

    // 0x50
    [0x50] = { "MOV D,B",     5,  &Intel8080::op_MOV },
    [0x51] = { "MOV D,C",     5,  &Intel8080::op_MOV },
    [0x52] = { "MOV D,D",     5,  &Intel8080::op_MOV },
    [0x53] = { "MOV D,E",     5,  &Intel8080::op_MOV },
    [0x54] = { "MOV D,H",     5,  &Intel8080::op_MOV },
    [0x55] = { "MOV D,L",     5,  &Intel8080::op_MOV },
    [0x56] = { "MOV D,M",     7,  &Intel8080::op_MOV },
    [0x57] = { "MOV D,A",     5,  &Intel8080::op_MOV },
    [0x58] = { "MOV E,B",     5,  &Intel8080::op_MOV },
    [0x59] = { "MOV E,C",     5,  &Intel8080::op_MOV },
    [0x5A] = { "MOV E,D",     5,  &Intel8080::op_MOV },
    [0x5B] = { "MOV E,E",     5,  &Intel8080::op_MOV },
    [0x5C] = { "MOV E,H",     5,  &Intel8080::op_MOV },
    [0x5D] = { "MOV E,L",     5,  &Intel8080::op_MOV },
    [0x5E] = { "MOV E,M",     7,  &Intel8080::op_MOV },
    [0x5F] = { "MOV E,A",     5,  &Intel8080::op_MOV },

    // 0x60
    [0x60] = { "MOV H,B",     5,  &Intel8080::op_MOV },
    [0x61] = { "MOV H,C",     5,  &Intel8080::op_MOV },
    [0x62] = { "MOV H,D",     5,  &Intel8080::op_MOV },
    [0x63] = { "MOV H,E",     5,  &Intel8080::op_MOV },
    [0x64] = { "MOV H,H",     5,  &Intel8080::op_MOV },
    [0x65] = { "MOV H,L",     5,  &Intel8080::op_MOV },
    [0x66] = { "MOV H,M",     7,  &Intel8080::op_MOV },
    [0x67] = { "MOV H,A",     5,  &Intel8080::op_MOV },
    [0x68] = { "MOV L,B",     5,  &Intel8080::op_MOV },
    [0x69] = { "MOV L,C",     5,  &Intel8080::op_MOV },
    [0x6A] = { "MOV L,D",     5,  &Intel8080::op_MOV },
    [0x6B] = { "MOV L,E",     5,  &Intel8080::op_MOV },
    [0x6C] = { "MOV L,H",     5,  &Intel8080::op_MOV },
    [0x6D] = { "MOV L,L",     5,  &Intel8080::op_MOV },
    [0x6E] = { "MOV L,M",     7,  &Intel8080::op_MOV },
    [0x6F] = { "MOV L,A",     5,  &Intel8080::op_MOV },

    // 0x70
    [0x70] = { "MOV M,B",     7,  &Intel8080::op_MOV },
    [0x71] = { "MOV M,C",     7,  &Intel8080::op_MOV },
    [0x72] = { "MOV M,D",     7,  &Intel8080::op_MOV },
    [0x73] = { "MOV M,E",     7,  &Intel8080::op_MOV },
    [0x74] = { "MOV M,H",     7,  &Intel8080::op_MOV },
    [0x75] = { "MOV M,L",     7,  &Intel8080::op_MOV },
    [0x76] = { "HLT",         5,  &Intel8080::op_HLT },
    [0x77] = { "MOV M,A",     7,  &Intel8080::op_MOV },
    [0x78] = { "MOV A,B",     5,  &Intel8080::op_MOV },
    [0x79] = { "MOV A,C",     5,  &Intel8080::op_MOV },
    [0x7A] = { "MOV A,D",     5,  &Intel8080::op_MOV },
    [0x7B] = { "MOV A,E",     5,  &Intel8080::op_MOV },
    [0x7C] = { "MOV A,H",     5,  &Intel8080::op_MOV },
    [0x7D] = { "MOV A,L",     5,  &Intel8080::op_MOV },
    [0x7E] = { "MOV A,M",     7,  &Intel8080::op_MOV },
    [0x7F] = { "MOV A,A",     5,  &Intel8080::op_MOV },

    // 0x80
    [0x80] = { "ADD B",       4,  &Intel8080::op_ADD },
    [0x81] = { "ADD C",       4,  &Intel8080::op_ADD },
    [0x82] = { "ADD D",       4,  &Intel8080::op_ADD },
    [0x83] = { "ADD E",       4,  &Intel8080::op_ADD },
    [0x84] = { "ADD H",       4,  &Intel8080::op_ADD },
    [0x85] = { "ADD L",       4,  &Intel8080::op_ADD },
    [0x86] = { "ADD M",       7,  &Intel8080::op_ADD },
    [0x87] = { "ADD A",       4,  &Intel8080::op_ADD },
    [0x88] = { "ADC B",       4,  &Intel8080::op_ADC },
    [0x89] = { "ADC C",       4,  &Intel8080::op_ADC },
    [0x8A] = { "ADC D",       4,  &Intel8080::op_ADC },
    [0x8B] = { "ADC E",       4,  &Intel8080::op_ADC },
    [0x8C] = { "ADC H",       4,  &Intel8080::op_ADC },
    [0x8D] = { "ADC L",       4,  &Intel8080::op_ADC },
    [0x8E] = { "ADC M",       7,  &Intel8080::op_ADC },
    [0x8F] = { "ADC A",       4,  &Intel8080::op_ADC },

    // 0x90
    [0x90] = { "SUB B",       4,  &Intel8080::op_SUB },
    [0x91] = { "SUB C",       4,  &Intel8080::op_SUB },
    [0x92] = { "SUB D",       4,  &Intel8080::op_SUB },
    [0x93] = { "SUB E",       4,  &Intel8080::op_SUB },
    [0x94] = { "SUB H",       4,  &Intel8080::op_SUB },
    [0x95] = { "SUB L",       4,  &Intel8080::op_SUB },
    [0x96] = { "SUB M",       7,  &Intel8080::op_SUB },
    [0x97] = { "SUB A",       4,  &Intel8080::op_SUB },
    [0x98] = { "SBB B",       4,  &Intel8080::op_SBB },
    [0x99] = { "SBB C",       4,  &Intel8080::op_SBB },
    [0x9A] = { "SBB D",       4,  &Intel8080::op_SBB },
    [0x9B] = { "SBB E",       4,  &Intel8080::op_SBB },
    [0x9C] = { "SBB H",       4,  &Intel8080::op_SBB },
    [0x9D] = { "SBB L",       4,  &Intel8080::op_SBB },
    [0x9E] = { "SBB M",       7,  &Intel8080::op_SBB },
    [0x9F] = { "SBB A",       4,  &Intel8080::op_SBB },

    // 0xA0
    [0xA0] = { "ANA B",       4,  &Intel8080::op_ANA },
    [0xA1] = { "ANA C",       4,  &Intel8080::op_ANA },
    [0xA2] = { "ANA D",       4,  &Intel8080::op_ANA },
    [0xA3] = { "ANA E",       4,  &Intel8080::op_ANA },
    [0xA4] = { "ANA H",       4,  &Intel8080::op_ANA },
    [0xA5] = { "ANA L",       4,  &Intel8080::op_ANA },
    [0xA6] = { "ANA M",       7,  &Intel8080::op_ANA },
    [0xA7] = { "ANA A",       4,  &Intel8080::op_ANA },
    [0xA8] = { "XRA B",       4,  &Intel8080::op_XRA },
    [0xA9] = { "XRA C",       4,  &Intel8080::op_XRA },
    [0xAA] = { "XRA D",       4,  &Intel8080::op_XRA },
    [0xAB] = { "XRA E",       4,  &Intel8080::op_XRA },
    [0xAC] = { "XRA H",       4,  &Intel8080::op_XRA },
    [0xAD] = { "XRA L",       4,  &Intel8080::op_XRA },
    [0xAE] = { "XRA M",       7,  &Intel8080::op_XRA },
    [0xAF] = { "XRA A",       4,  &Intel8080::op_XRA },

    // 0xB0
    [0xB0] = { "ORA B",       4,  &Intel8080::op_ORA },
    [0xB1] = { "ORA C",       4,  &Intel8080::op_ORA },
    [0xB2] = { "ORA D",       4,  &Intel8080::op_ORA },
    [0xB3] = { "ORA E",       4,  &Intel8080::op_ORA },
    [0xB4] = { "ORA H",       4,  &Intel8080::op_ORA },
    [0xB5] = { "ORA L",       4,  &Intel8080::op_ORA },
    [0xB6] = { "ORA M",       7,  &Intel8080::op_ORA },
    [0xB7] = { "ORA A",       4,  &Intel8080::op_ORA },
    [0xB8] = { "CMP B",       4,  &Intel8080::op_CMP },
    [0xB9] = { "CMP C",       4,  &Intel8080::op_CMP },
    [0xBA] = { "CMP D",       4,  &Intel8080::op_CMP },
    [0xBB] = { "CMP E",       4,  &Intel8080::op_CMP },
    [0xBC] = { "CMP H",       4,  &Intel8080::op_CMP },
    [0xBD] = { "CMP L",       4,  &Intel8080::op_CMP },
    [0xBE] = { "CMP M",       7,  &Intel8080::op_CMP },
    [0xBF] = { "CMP A",       4,  &Intel8080::op_CMP },

    // 0xC0
    [0xC0] = { "RNZ",         5,  &Intel8080::op_RNZ },
    [0xC1] = { "POP B",      10,  &Intel8080::op_POP },
    [0xC2] = { "JNZ A16",    10,  &Intel8080::op_JNZ },
    [0xC3] = { "JMP A16",    10,  &Intel8080::op_JMP },
    [0xC4] = { "CNZ A16",    11,  &Intel8080::op_CNZ },
    [0xC5] = { "PUSH B",     11,  &Intel8080::op_PUSH },
    [0xC6] = { "ADI D8",      7,  &Intel8080::op_ADI },
    [0xC7] = { "RST 0",      11,  &Intel8080::op_RST },
    [0xC8] = { "RZ",          5,  &Intel8080::op_RZ },
    [0xC9] = { "RET",        10,  &Intel8080::op_RET },
    [0xCA] = { "JZ A16",     10,  &Intel8080::op_JZ },
    [0xCB] = { "JMP A16",    10,  &Intel8080::op_JMP },
    [0xCC] = { "CZ A16",     11,  &Intel8080::op_CZ },
    [0xCD] = { "CALL A16",   17,  &Intel8080::op_CALL },
    [0xCE] = { "ACI D8",      7,  &Intel8080::op_ACI },
    [0xCF] = { "RST 1",      11,  &Intel8080::op_RST },

    // 0xD0
    [0xD0] = { "RNC",         5,  &Intel8080::op_RNC },
    [0xD1] = { "POP D",      10,  &Intel8080::op_POP },
    [0xD2] = { "JNC A16",    10,  &Intel8080::op_JNC },
    [0xD3] = { "OUT D8",     10,  &Intel8080::op_OUT },
    [0xD4] = { "CNC A16",    11,  &Intel8080::op_CNC },
    [0xD5] = { "PUSH D",     11,  &Intel8080::op_PUSH },
    [0xD6] = { "SUI D8",      7,  &Intel8080::op_SUI },
    [0xD7] = { "RST 2",      11,  &Intel8080::op_RST },
    [0xD8] = { "RC",          5,  &Intel8080::op_RC },
    [0xD9] = { "RET",        10,  &Intel8080::op_RET },
    [0xDA] = { "JC A16",     10,  &Intel8080::op_JC },
    [0xDB] = { "IN D8",      10,  &Intel8080::op_IN },
    [0xDC] = { "CC A16",     11,  &Intel8080::op_CC },
    [0xDD] = { "CALL A16",   17,  &Intel8080::op_CALL },
    [0xDE] = { "SBI D8",      7,  &Intel8080::op_SBI },
    [0xDF] = { "RST 3",      11,  &Intel8080::op_RST },

    // 0xE0
    [0xE0] = { "RPO",         5,  &Intel8080::op_RPO },
    [0xE1] = { "POP H",      10,  &Intel8080::op_POP },
    [0xE2] = { "JPO A16",    10,  &Intel8080::op_JPO },
    [0xE3] = { "XTHL",       18,  &Intel8080::op_XTHL },
    [0xE4] = { "CPO A16",    11,  &Intel8080::op_CPO },
    [0xE5] = { "PUSH H",     11,  &Intel8080::op_PUSH },
    [0xE6] = { "ANI D8",      7,  &Intel8080::op_ANI },
    [0xE7] = { "RST 4",      11,  &Intel8080::op_RST },
    [0xE8] = { "RPE",         5,  &Intel8080::op_RPE },
    [0xE9] = { "PCHL",        5,  &Intel8080::op_PCHL },
    [0xEA] = { "JPE A16",    10,  &Intel8080::op_JPE },
    [0xEB] = { "XCHG",        4,  &Intel8080::op_XCHG },
    [0xEC] = { "CPE A16",    11,  &Intel8080::op_CPE },
    [0xED] = { "CALL A16",   17,  &Intel8080::op_CALL },
    [0xEE] = { "XRI D8",      7,  &Intel8080::op_XRI },
    [0xEF] = { "RST 5",      11,  &Intel8080::op_RST },

    // 0xF0
    [0xF0] = { "RP",          5,  &Intel8080::op_RP },
    [0xF1] = { "POP PSW",    10,  &Intel8080::op_POP_PSW },
    [0xF2] = { "JP A16",     10,  &Intel8080::op_JP },
    [0xF3] = { "DI",          4,  &Intel8080::op_DI },
    [0xF4] = { "CP A16",     11,  &Intel8080::op_CP },
    [0xF5] = { "PUSH PSW",   11,  &Intel8080::op_PUSH_PSW },
    [0xF6] = { "ORI D8",      7,  &Intel8080::op_ORI },
    [0xF7] = { "RST 6",      11,  &Intel8080::op_RST },
    [0xF8] = { "RM",          5,  &Intel8080::op_RM },
    [0xF9] = { "SPHL",        5,  &Intel8080::op_SPHL },
    [0xFA] = { "JM A16",     10,  &Intel8080::op_JM },
    [0xFB] = { "EI",          4,  &Intel8080::op_EI },
    [0xFC] = { "CM A16",     11,  &Intel8080::op_CM },
    [0xFD] = { "CALL A16",   17,  &Intel8080::op_CALL },
    [0xFE] = { "CPI D8",      7,  &Intel8080::op_CPI },
    [0xFF] = { "RST 7",      11,  &Intel8080::op_RST }
};


// set zero flag if result is 0 lol
inline bool __calculateZero(uint8_t result)
{
    return result == 0;
}

// returns true if the top bit is set
inline bool __calculateSign(uint8_t result)
{
    return (result & 0x80) != 0;
}

// returns true if the number of set bits is even
inline bool __calculateParity(uint8_t result)
{
    uint8_t p = result;
    p ^= p >> 4;
    p ^= p >> 2;
    p ^= p >> 1;
    return !(p & 1);
}

// For ANI d8: Extract bit 3 of the immediate data byte
inline bool __calculateAuxCarryANI(uint8_t op1)
{
    return (op1 & 0x08) != 0;
}

inline bool __calculateAuxCarryANI(uint8_t a, uint8_t op1)
{
    return ((a | op1) & 0x08) != 0;
}

// For ANA r: Logical OR of bit 3 from both registers
inline bool __calculateAuxCarryANA(uint8_t a, uint8_t b)
{
    return ((a | b) & 0x08) != 0;
}

// Returns true if addition carries out of bit 3 (lower nibble overflow)
inline bool __calculateAuxCarryAdd(uint8_t a, uint8_t b)
{
    return (((a & 0x0F) + (b & 0x0F)) > 0x0F);
}

// For SUB, SUI, CMP, CPI, DCR
inline bool __calculateAuxCarrySub(uint8_t a, uint8_t b)
{
    // 8080 ALU calculates A - B as: A + (~B) + 1
    // AC is the carry out of bit 3 of this addition.
    return (((a & 0x0F) + (~b & 0x0F) + 1) > 0x0F);
}

// For SBB, SBI
inline bool __calculateAuxCarrySBB(uint8_t a, uint8_t b, bool carryFlag)
{
    // A - B - CY is calculated as A + (~B) + (carryFlag ? 0 : 1)
    uint8_t twosCompCarry = carryFlag ? 0 : 1;
    return (((a & 0x0F) + (~b & 0x0F) + twosCompCarry) > 0x0F);
}

Intel8080::Intel8080()
{
    // Power-on state setup
    PC = 0x0000;
    SP = 0x0000; // Often set by the loaded program, but good to clear

    // Clear all registers to 0
    regs.A = 0;
    regs.BC = 0;
    regs.DE = 0;
    regs.HL = 0;

    // Clear all flags
    flags.Zero = 0;
    flags.Sign = 0;
    flags.Parity = 0;
    flags.Carry = 0;
    flags.AuxCarry = 0;

    interrupts.enabled = false;
    interrupts.halted  = false;
    interrupts.pending = false;
    interrupts.vector  = 0x00;
}

Intel8080::~Intel8080()
{
    // Nothing to manually clean up since we aren't using raw pointers!
}

uint8_t Intel8080::getByte()
{
    return mem.read(PC++);
}

uint16_t Intel8080::getWord()
{
    uint16_t val = mem.readWord(PC);
    PC += 2;
    return val;
}

int Intel8080::step()
{
    // check if an interrupt is pending and interrupts are enabled
    if (interrupts.pending && interrupts.enabled)
    {
        // Wake up if we were halted
        interrupts.halted = false;

        // Disable further interrupts automatically (hardware behavior)
        interrupts.enabled = false;
        interrupts.pending = false;

        // Fetch the injected opcode
        uint8_t interruptOpcode = interrupts.vector;

        // Execute the injected instruction instead of the one at PC
        const Instruction& instr = OPCODE_TABLE[interruptOpcode];
        (this->*instr.handler)(interruptOpcode);

        return instr.cycles;
    }

    // If we are locked in a HLT state and no interrupt happened, burn cycles
    if (interrupts.halted) return 4;

    // exectute instruction
    uint8_t opcode = getByte();
    const Instruction& instr = OPCODE_TABLE[opcode];
    (this->*instr.handler)(opcode);

    return instr.cycles;
}

uint8_t& Intel8080::decodeReg(uint8_t index)
{
    static uint8_t* table[] = { &regs.B, &regs.C, &regs.D, &regs.E, &regs.H, &regs.L, nullptr, &regs.A };
    return *table[index];
}

void Intel8080::push16(uint16_t value)
{
    SP -= 2;
    mem.writeWord(SP, value);
}

uint16_t Intel8080::pop16()
{
    uint16_t val = mem.readWord(SP); // Read the 16-bit word cleanly
    SP += 2;                         // Shrink stack back up
    return val;
}

void Intel8080::doCall(uint16_t targetAddress)
{
    push16(PC); // PC has already advanced past the immediate address bytes
    PC = targetAddress;
}

void Intel8080::doReturn()
{
    PC = pop16();
}

void Intel8080::op_NOP(uint8_t opcode)
{
}

void Intel8080::op_MOV(uint8_t opcode)
{
    uint8_t dstIdx = (opcode >> 3) & 0x07;
    uint8_t srcIdx = opcode & 0x07;

    if (srcIdx == 6)
    {
        decodeReg(dstIdx) = mem.read(regs.HL);
    }
    else if (dstIdx == 6)
    {
        mem.write(regs.HL, decodeReg(srcIdx));
    }
    else
    {
        decodeReg(dstIdx) = decodeReg(srcIdx);
    }
}

void Intel8080::op_MVI(uint8_t opcode)
{
    uint8_t dstIdx = (opcode >> 3) & 0x07;
    uint8_t immediateValue = getByte(); // advances PC by 1

    if (dstIdx == 6)
    {
        mem.write(regs.HL, immediateValue);
    }
    else
    {
        decodeReg(dstIdx) = immediateValue;
    }
}

void Intel8080::op_STA(uint8_t opcode)
{
    uint16_t addr = getWord(); // advances PC by 2
    mem.write(addr, regs.A);
}

void Intel8080::op_LDA(uint8_t opcode)
{
    uint16_t addr = getWord(); // advances PC by 2
    regs.A = mem.read(addr);
}

void Intel8080::op_STAX(uint8_t opcode)
{
    // 0x02 = BC, 0x12 = DE
    uint16_t addr = ((opcode >> 4) == 0) ? regs.BC : regs.DE;
    mem.write(addr, regs.A);
}

void Intel8080::op_LDAX(uint8_t opcode)
{
    // 0x0A = BC, 0x1A = DE
    uint16_t addr = ((opcode >> 4) == 0) ? regs.BC : regs.DE;
    regs.A = mem.read(addr);
}

void Intel8080::op_LXI_SP(uint8_t opcode)
{
    SP = getWord(); // advances PC by 2
}

void Intel8080::op_LXI_Reg(uint8_t opcode)
{
    uint8_t pairIdx = (opcode >> 4) & 0x03;
    uint16_t val = getWord(); // advances PC by 2

    if (pairIdx == 0) regs.BC = val;
    else if (pairIdx == 1) regs.DE = val;
    else if (pairIdx == 2) regs.HL = val;
}

void Intel8080::op_SHLD(uint8_t opcode)
{
    uint16_t addr = getWord();
    mem.writeWord(addr, regs.HL);
}

void Intel8080::op_LHLD(uint8_t opcode)
{
    uint16_t addr = getWord();
    regs.HL = mem.readWord(addr);
}

void Intel8080::op_INR(uint8_t opcode)
{
    uint8_t regIdx = (opcode >> 3) & 0x07;

    if (regIdx == 6)
    {
        uint8_t val = mem.read(regs.HL);
        flags.AuxCarry = __calculateAuxCarryAdd(val, 1);
        val += 1;
        flags.Zero   = __calculateZero(val);
        flags.Sign   = __calculateSign(val);
        flags.Parity = __calculateParity(val);
        mem.write(regs.HL, val);
    }
    else
    {
        uint8_t& reg = decodeReg(regIdx);
        flags.AuxCarry = __calculateAuxCarryAdd(reg, 1);
        reg += 1;
        flags.Zero   = __calculateZero(reg);
        flags.Sign   = __calculateSign(reg);
        flags.Parity = __calculateParity(reg);
    }
}

void Intel8080::op_DCR(uint8_t opcode)
{
    uint8_t regIdx = (opcode >> 3) & 0x07;

    if (regIdx == 6)
    {
        uint8_t val = mem.read(regs.HL);
        flags.AuxCarry = __calculateAuxCarrySub(val, 1);
        val -= 1;
        flags.Zero   = __calculateZero(val);
        flags.Sign   = __calculateSign(val);
        flags.Parity = __calculateParity(val);
        mem.write(regs.HL, val);
    }
    else
    {
        uint8_t& reg = decodeReg(regIdx);
        flags.AuxCarry = __calculateAuxCarrySub(reg, 1);
        reg -= 1;
        flags.Zero   = __calculateZero(reg);
        flags.Sign   = __calculateSign(reg);
        flags.Parity = __calculateParity(reg);
    }
}

void Intel8080::op_INX(uint8_t opcode)
{
    uint8_t pairIdx = (opcode >> 4) & 0x03;
    if (pairIdx == 0) regs.BC += 1;
    else if (pairIdx == 1) regs.DE += 1;
    else if (pairIdx == 2) regs.HL += 1;
    else if (pairIdx == 3) SP += 1;
}

void Intel8080::op_DCX(uint8_t opcode)
{
    uint8_t pairIdx = (opcode >> 4) & 0x03;
    if (pairIdx == 0) regs.BC -= 1;
    else if (pairIdx == 1) regs.DE -= 1;
    else if (pairIdx == 2) regs.HL -= 1;
    else if (pairIdx == 3) SP -= 1;
}

void Intel8080::op_DAD(uint8_t opcode)
{
    uint8_t pairIdx = (opcode >> 4) & 0x03;
    uint16_t src = 0;
    if (pairIdx == 0) src = regs.BC;
    else if (pairIdx == 1) src = regs.DE;
    else if (pairIdx == 2) src = regs.HL;
    else if (pairIdx == 3) src = SP;

    uint32_t result = (uint32_t)regs.HL + src;
    flags.Carry = (result > 0xFFFF);
    regs.HL = result & 0xFFFF;
}

void Intel8080::op_DAA(uint8_t opcode)
{
    uint8_t correction = 0;
    bool newCarry = flags.Carry;

    if ((regs.A & 0x0F) > 9 || flags.AuxCarry)
    {
        correction |= 0x06;
    }

    if ((regs.A >> 4) > 9 || flags.Carry || ((regs.A >> 4) == 9 && (regs.A & 0x0F) > 9))
    {
        correction |= 0x60;
        newCarry = true;
    }

    flags.AuxCarry = __calculateAuxCarryAdd(regs.A, correction);
    regs.A += correction;
    flags.Carry  = newCarry;
    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
}

void Intel8080::op_ADD(uint8_t opcode)
{
    uint8_t srcIdx = opcode & 0x07;
    uint8_t src = (srcIdx == 6) ? mem.read(regs.HL) : decodeReg(srcIdx);

    uint16_t result = (uint16_t)regs.A + src;
    flags.AuxCarry = ((regs.A ^ src ^ result) & 0x10) != 0;
    flags.Carry    = (result > 0xFF);
    regs.A = (uint8_t)result;

    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
}

void Intel8080::op_ADI(uint8_t opcode)
{
    uint8_t op1 = getByte();
    uint16_t result = (uint16_t)regs.A + op1;

    flags.AuxCarry = ((regs.A ^ op1 ^ result) & 0x10) != 0;
    flags.Carry    = (result > 0xFF);
    regs.A = (uint8_t)result;

    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
}

void Intel8080::op_ADC(uint8_t opcode)
{
    uint8_t srcIdx = opcode & 0x07;
    uint8_t src = (srcIdx == 6) ? mem.read(regs.HL) : decodeReg(srcIdx);
    uint8_t carryIn = flags.Carry ? 1 : 0;

    uint16_t result = (uint16_t)regs.A + src + carryIn;
    flags.AuxCarry = ((regs.A ^ src ^ result) & 0x10) != 0;
    flags.Carry    = (result > 0xFF);
    regs.A = (uint8_t)result;

    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
}

void Intel8080::op_ACI(uint8_t opcode)
{
    uint8_t op1 = getByte();
    uint8_t carryIn = flags.Carry ? 1 : 0;
    uint16_t result = (uint16_t)regs.A + op1 + carryIn;

    flags.AuxCarry = ((regs.A ^ op1 ^ result) & 0x10) != 0;
    flags.Carry    = (result > 0xFF);
    regs.A = (uint8_t)result;

    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
}

void Intel8080::op_SUB(uint8_t opcode)
{
    uint8_t srcIdx = opcode & 0x07;
    uint8_t src = (srcIdx == 6) ? mem.read(regs.HL) : decodeReg(srcIdx);

    uint16_t result = (uint16_t)regs.A - src;
    flags.AuxCarry = (~(regs.A ^ src ^ result) & 0x10) != 0;
    flags.Carry    = (regs.A < src);
    regs.A = (uint8_t)result;

    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
}

void Intel8080::op_SUI(uint8_t opcode)
{
    uint8_t op1 = getByte();
    uint16_t result = (uint16_t)regs.A - op1;

    flags.AuxCarry = (~(regs.A ^ op1 ^ result) & 0x10) != 0;
    flags.Carry    = (regs.A < op1);
    regs.A = (uint8_t)result;

    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
}

void Intel8080::op_SBB(uint8_t opcode)
{
    uint8_t srcIdx = opcode & 0x07;
    uint8_t src = (srcIdx == 6) ? mem.read(regs.HL) : decodeReg(srcIdx);
    uint8_t borrowIn = flags.Carry ? 1 : 0;

    uint16_t result = (uint16_t)regs.A - src - borrowIn;
    flags.AuxCarry = (~(regs.A ^ src ^ result) & 0x10) != 0;
    flags.Carry    = (regs.A < (uint16_t)src + borrowIn);
    regs.A = (uint8_t)result;

    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
}

void Intel8080::op_SBI(uint8_t opcode)
{
    uint8_t op1 = getByte();
    uint8_t borrowIn = flags.Carry ? 1 : 0;
    uint16_t result = (uint16_t)regs.A - op1 - borrowIn;

    flags.AuxCarry = (~(regs.A ^ op1 ^ result) & 0x10) != 0;
    flags.Carry    = (regs.A < (uint16_t)op1 + borrowIn);
    regs.A = (uint8_t)result;

    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
}

void Intel8080::op_CMP(uint8_t opcode)
{
    uint8_t srcIdx = opcode & 0x07;
    uint8_t src = (srcIdx == 6) ? mem.read(regs.HL) : decodeReg(srcIdx);

    uint16_t result = (uint16_t)regs.A - src;
    flags.AuxCarry = (~(regs.A ^ src ^ result) & 0x10) != 0;
    flags.Carry    = (regs.A < src);

    uint8_t res8 = (uint8_t)result;
    flags.Zero   = __calculateZero(res8);
    flags.Sign   = __calculateSign(res8);
    flags.Parity = __calculateParity(res8);
}

void Intel8080::op_CPI(uint8_t opcode)
{
    uint8_t op1 = getByte();
    uint16_t result = (uint16_t)regs.A - op1;

    flags.AuxCarry = (~(regs.A ^ op1 ^ result) & 0x10) != 0;
    flags.Carry    = (regs.A < op1);

    uint8_t res8 = (uint8_t)result;
    flags.Zero   = __calculateZero(res8);
    flags.Sign   = __calculateSign(res8);
    flags.Parity = __calculateParity(res8);
}

void Intel8080::op_ANI(uint8_t opcode)
{
    uint8_t op1 = getByte();
    flags.AuxCarry = __calculateAuxCarryANI(regs.A, op1);

    regs.A = regs.A & op1;
    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Carry  = 0;
    flags.Parity = __calculateParity(regs.A);
}

void Intel8080::op_ANA(uint8_t opcode)
{
    uint8_t srcIdx = opcode & 0x07;
    uint8_t src = (srcIdx == 6) ? mem.read(regs.HL) : decodeReg(srcIdx);

    flags.AuxCarry = ((regs.A | src) & 0x08) != 0;
    flags.Carry    = 0;

    regs.A = regs.A & src;
    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
}

void Intel8080::op_XRA(uint8_t opcode)
{
    uint8_t srcIdx = opcode & 0x07;
    uint8_t src = (srcIdx == 6) ? mem.read(regs.HL) : decodeReg(srcIdx);

    flags.AuxCarry = 0;
    flags.Carry    = 0;

    regs.A = regs.A ^ src;
    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
}

void Intel8080::op_XRI(uint8_t opcode)
{
    uint8_t op1 = getByte();
    regs.A = regs.A ^ op1;

    flags.AuxCarry = 0;
    flags.Carry    = 0;

    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
}

void Intel8080::op_ORA(uint8_t opcode)
{
    uint8_t srcIdx = opcode & 0x07;
    uint8_t src = (srcIdx == 6) ? mem.read(regs.HL) : decodeReg(srcIdx);

    flags.AuxCarry = 0;
    flags.Carry    = 0;

    regs.A = regs.A | src;
    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
}

void Intel8080::op_ORI(uint8_t opcode)
{
    uint8_t op1 = getByte();
    regs.A = regs.A | op1;

    flags.AuxCarry = 0;
    flags.Carry    = 0;

    flags.Zero   = __calculateZero(regs.A);
    flags.Sign   = __calculateSign(regs.A);
    flags.Parity = __calculateParity(regs.A);
}

void Intel8080::op_RLC(uint8_t opcode)
{
    flags.Carry = (regs.A >> 7) & 1;
    regs.A = (regs.A << 1) | flags.Carry;
}

void Intel8080::op_RRC(uint8_t opcode)
{
    flags.Carry = regs.A & 1;
    regs.A = (regs.A >> 1) | (flags.Carry << 7);
}

void Intel8080::op_RAL(uint8_t opcode)
{
    uint8_t oldCarry = flags.Carry;
    flags.Carry = (regs.A >> 7) & 1;
    regs.A = (regs.A << 1) | oldCarry;
}

void Intel8080::op_RAR(uint8_t opcode)
{
    uint8_t oldCarry = flags.Carry;
    flags.Carry = regs.A & 1;
    regs.A = (regs.A >> 1) | (oldCarry << 7);
}

void Intel8080::op_CMA(uint8_t opcode)
{
    regs.A = ~regs.A;
}

void Intel8080::op_STC(uint8_t opcode)
{
    flags.Carry = 1;
}

void Intel8080::op_CMC(uint8_t opcode)
{
    flags.Carry = !flags.Carry;
}

void Intel8080::op_XCHG(uint8_t opcode)
{
    uint16_t tmp = regs.DE;
    regs.DE = regs.HL;
    regs.HL = tmp;
}

void Intel8080::op_XTHL(uint8_t opcode)
{
    uint16_t tmpHL = regs.HL;
    regs.HL = mem.readWord(SP);
    mem.writeWord(SP, tmpHL);
}

void Intel8080::op_PCHL(uint8_t opcode)
{
    PC = regs.HL;
}

void Intel8080::op_SPHL(uint8_t opcode)
{
    SP = regs.HL;
}

void Intel8080::op_JMP(uint8_t opcode)
{
    PC = getWord();
}

void Intel8080::op_JC(uint8_t opcode)
{
    uint16_t target = getWord();
    if (flags.Carry) PC = target;
}

void Intel8080::op_JNC(uint8_t opcode)
{
    uint16_t target = getWord();
    if (!flags.Carry) PC = target;
}

void Intel8080::op_JZ(uint8_t opcode)
{
    uint16_t target = getWord();
    if (flags.Zero) PC = target;
}

void Intel8080::op_JNZ(uint8_t opcode)
{
    uint16_t target = getWord();
    if (!flags.Zero) PC = target;
}

void Intel8080::op_JPE(uint8_t opcode)
{
    uint16_t target = getWord();
    if (flags.Parity) PC = target;
}

void Intel8080::op_JPO(uint8_t opcode)
{
    uint16_t target = getWord();
    if (!flags.Parity) PC = target;
}

void Intel8080::op_JM(uint8_t opcode)
{
    uint16_t target = getWord();
    if (flags.Sign) PC = target;
}

void Intel8080::op_JP(uint8_t opcode)
{
    uint16_t target = getWord();
    if (!flags.Sign) PC = target;
}

void Intel8080::op_CALL(uint8_t opcode)
{
    doCall(getWord());
}

void Intel8080::op_RET(uint8_t opcode)
{
    doReturn();
}

void Intel8080::op_CNC(uint8_t opcode) { uint16_t target = getWord(); if (!flags.Carry)  doCall(target); }
void Intel8080::op_CNZ(uint8_t opcode) { uint16_t target = getWord(); if (!flags.Zero)   doCall(target); }
void Intel8080::op_CZ(uint8_t opcode)  { uint16_t target = getWord(); if (flags.Zero)    doCall(target); }
void Intel8080::op_CPE(uint8_t opcode) { uint16_t target = getWord(); if (flags.Parity)  doCall(target); }
void Intel8080::op_CC(uint8_t opcode)  { uint16_t target = getWord(); if (flags.Carry)   doCall(target); }
void Intel8080::op_CPO(uint8_t opcode) { uint16_t target = getWord(); if (!flags.Parity) doCall(target); }
void Intel8080::op_CM(uint8_t opcode)  { uint16_t target = getWord(); if (flags.Sign)    doCall(target); }
void Intel8080::op_CP(uint8_t opcode)  { uint16_t target = getWord(); if (!flags.Sign)   doCall(target); }

void Intel8080::op_RNC(uint8_t opcode) { if (!flags.Carry)  doReturn(); }
void Intel8080::op_RNZ(uint8_t opcode) { if (!flags.Zero)   doReturn(); }
void Intel8080::op_RC(uint8_t opcode)  { if (flags.Carry)   doReturn(); }
void Intel8080::op_RZ(uint8_t opcode)  { if (flags.Zero)    doReturn(); }
void Intel8080::op_RM(uint8_t opcode)  { if (flags.Sign)    doReturn(); }
void Intel8080::op_RP(uint8_t opcode)  { if (!flags.Sign)   doReturn(); }
void Intel8080::op_RPE(uint8_t opcode) { if (flags.Parity)  doReturn(); }
void Intel8080::op_RPO(uint8_t opcode) { if (!flags.Parity) doReturn(); }

void Intel8080::op_RST(uint8_t opcode)
{
    uint8_t vector = (opcode >> 3) & 0x07;
    push16(PC);
    PC = (uint16_t)vector * 8;
}

void Intel8080::op_PUSH(uint8_t opcode)
{
    uint8_t pairIdx = (opcode >> 4) & 0x03;
    if (pairIdx == 0)      push16(regs.BC);
    else if (pairIdx == 1) push16(regs.DE);
    else if (pairIdx == 2) push16(regs.HL);
}

void Intel8080::op_PUSH_PSW(uint8_t opcode)
{
    uint8_t psw = (flags.Sign      << 7)
                | (flags.Zero      << 6)
                | (0               << 5)
                | (flags.AuxCarry  << 4)
                | (0               << 3)
                | (flags.Parity    << 2)
                | (1               << 1) // Bit 1 is hardware hardwired to 1
                | (flags.Carry     << 0);
    push16(((uint16_t)regs.A << 8) | psw);
}

void Intel8080::op_POP(uint8_t opcode)
{
    uint8_t pairIdx = (opcode >> 4) & 0x03;
    uint16_t val = pop16();
    if (pairIdx == 0)      regs.BC = val;
    else if (pairIdx == 1) regs.DE = val;
    else if (pairIdx == 2) regs.HL = val;
}

void Intel8080::op_POP_PSW(uint8_t opcode)
{
    uint16_t val = pop16();
    regs.A = (val >> 8) & 0xFF;
    uint8_t psw = val & 0xFF;

    flags.Sign     = (psw >> 7) & 1;
    flags.Zero     = (psw >> 6) & 1;
    flags.AuxCarry = (psw >> 4) & 1;
    flags.Parity   = (psw >> 2) & 1;
    flags.Carry    = (psw >> 0) & 1;
}

static uint16_t shift_register = 0;
static uint8_t shift_offset = 0;
extern uint8_t port1;
extern uint8_t port2;

void Intel8080::op_OUT(uint8_t opcode)
{
    uint8_t port = getByte();
    uint8_t value = regs.A;

    if (port == 2) {
        shift_offset = value & 0x07;
    } else if (port == 4) {
        shift_register = (shift_register >> 8) | (value << 8);
    }
}

void Intel8080::op_IN(uint8_t opcode)
{
    uint8_t port = getByte();

    if (port == 1)      regs.A = port1;
    else if (port == 2) regs.A = port2;
    else if (port == 3) regs.A = (shift_register >> (8 - shift_offset)) & 0xFF;
    else                regs.A = 0;
}

void Intel8080::op_EI(uint8_t opcode)
{
    interrupts.enabled = true;
}

void Intel8080::op_DI(uint8_t opcode)
{
    interrupts.enabled = false;
}

void Intel8080::op_HLT(uint8_t opcode) {
    interrupts.halted = true;
}

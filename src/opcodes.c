#include "opcodes.h"

WORD readNextOpcode(const BYTE* memory) {
    WORD out = 0;

    out |= (memory[0] << 8);
    out |= memory[1];

    return out;
}

OpcodeType getOpcodeType(WORD opcode) {
    // Jumping through the constant opcodes.
    switch (opcode) {
        case 0x00E0: return OP_CLEAR_DISPLAY;
        case 0x00EE: return OP_RETURN;
    }

    uint8_t leadingByte = (opcode & 0xF000) >> 12;

    switch (leadingByte) {
        case 0x0: return OP_CALL_ASSEMBLY;
        case 0x1: return OP_JUMP;
        case 0x2: return OP_CALL;
        case 0x3: return OP_SKIP_EQ;
        case 0x4: return OP_SKIP_NE;
        case 0x5: return OP_SKIP_CMP;
        case 0x6: return OP_SET;
        case 0x7: return OP_ADD;
        case 0x8: {
            uint8_t trailingByte = (opcode & 0x000F);
            switch (trailingByte) {
                case 0x0: return OP_ASS;
                case 0x1: return OP_OR;
                case 0x2: return OP_AND;
                case 0x3: return OP_XOR;
                case 0x4: return OP_ASS_ADD;
                case 0x5: return OP_ASS_SUB;
                case 0x6: return OP_SHR;
                case 0x7: return OP_ASS_NSUB;
                case 0xE: return OP_SHL;
            }
        }
        case 0x9: return OP_SKIP_NCMP;
        case 0xA: return OP_ADR_SET;
        case 0xB: return OP_JUMP_V0;
        case 0xC: return OP_RAND;
        case 0xD: return OP_DRAW;
        case 0xE: {
            if ((opcode & 0x009E) != 0) {
                return OP_KEY_SKIP;
            } else if ((opcode & 0x00A1) != 0) {
                return OP_KEY_NSKIP;
            }
        }
        case 0xF: {
            uint16_t trailingWord = (opcode & 0x00FF);
            switch (trailingWord) {
                case 0x07: return OP_GET_DELAY;
                case 0x0A: return OP_AWAIT_KEY;
                case 0x15: return OP_SET_DELAY;
                case 0x18: return OP_SET_SOUND;
                case 0x1E: return OP_ADR_ADD;
                case 0x29: return OP_SPRITE;
                case 0x33: return OP_BCD;
                case 0x55: return OP_REG_DUMP;
                case 0x65: return OP_REG_LOAD;
            }
        }
    }

    // Unreachable in valid CHIP-8 ROM.
    return OP_UNKNOWN;
}

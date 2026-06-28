#ifndef CHIP8VM_OPCODES_H_
#define CHIP8VM_OPCODES_H_

#include "types.h"

typedef enum {
    /**
     * Opcode: ONNN
     * Calls machine code routine at address NNN.
     */
    OP_CALL_ASSEMBLY,
    /**
     * Opcode: 00E0
     * Clears the screen.
     */
    OP_CLEAR_DISPLAY,
    /**
     * Opcode: 00EE
     * Returns from a subroutine.
     */
    OP_RETURN,
    /**
     * Opcode: 1NNN
     * Jumps to address NNN.
     */
    OP_JUMP,
    /**
     * Opcode: 2NNN
     * Calls subroutine at NNN.
     */
    OP_CALL,
    /**
     * Opcode: 3XNN
     * Skips the next instruction if VX equals NN.
     */
    OP_SKIP_EQ,
    /**
     * Opcode: 4XNN
     * Skips the next instruction if VX does not equal NN.
     */
    OP_SKIP_NE,
    /**
     * Opcode: 5XY0
     * Skips the next instruction if VX equals VY.
     */
    OP_SKIP_CMP,
    /**
     * Opcode: 6XNN
     * Sets VX to NN.
     */
    OP_SET,
    /**
     * Opcode: 7XNN
     * Adds NN to VX (carry flag is not changed).
     */
    OP_ADD,
    /**
     * Opcode: 8XY0
     * Sets VX to the value of VY.
     */
    OP_ASS,
    /**
     * Opcode: 8XY1
     * Sets VX to VX or VY.
     */
    OP_OR,
    /**
     * Opcode: 8XY2
     * Sets VX to VX and VY.
     */
    OP_AND,
    /**
     * Opcode: 8XY3
     * Sets VX to VX xor VY.
     */
    OP_XOR,
    /**
     * Opcode: 8XY4
     * Adds VY to VX. VF is set to 1 when there's an overflow, and to 0 when
     * there is not.
     */
    OP_ASS_ADD,
    /**
     * Opcode: 8XY5
     * VY is subtracted from VX. VF is set to 0 when there's an underflow, and 1
     * when there is not. (i.e. VF set to 1 if VX >= VY and 0 if not).
     */
    OP_ASS_SUB,
    /**
     * Opcode: 8XY6
     * Store the value of register VY shifted right one bit in register VX.
     * Set register VF to the least significant bit prior to the shift.
     */
    OP_SHR,
    /**
     * Opcode: 8XY7
     * Sets VX to VY minus VX. VF is set to 0 when there's an underflow, and 1
     * when there is not. (i.e. VF set to 1 if VY >= VX).
     */
    OP_ASS_NSUB,
    /**
     * Opcode: 8XYE
     * Store the value of register VY shifted left one bit in register VX.
     * Set register VF to the most significant bit prior to the shift.
     */
    OP_SHL,
    /**
     * Opcode: 9XY0
     * Skips the next instruction if VX does not equal VY.
     * (Usually the next instruction is a jump to skip a code block).
     */
    OP_SKIP_NCMP,
    /**
     * Opcode: ANNN
     * Sets I to the address NNN.
     */
    OP_ADR_SET,
    /**
     * Opcode: BNNN
     * Jumps to the address NNN plus V0.
     */
    OP_JUMP_V0,
    /**
     * Opcode: CXNN
     * Sets VX to the result of a bitwise and operation on a random number
     * (Typically: 0 to 255) and NN.
     */
    OP_RAND,
    /**
     * Opcode: DXYN
     * Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a
     * height of N pixels. Each row of 8 pixels is read as bit-coded starting
     * from memory location I; I value does not change after the execution of
     * this instruction. As described above, VF is set to 1 if any screen pixels
     * are flipped from set to unset when the sprite is drawn, and to 0 if that
     * does not happen.
     */
    OP_DRAW,
    /**
     * Opcode: EX9E
     * Skips the next instruction if the key stored in VX (only consider the
     * lowest nibble) is pressed (usually the next instruction is a jump to skip
     * a code block).
     */
    OP_KEY_SKIP,
    /**
     * Opcode: EXA1
     * Skips the next instruction if the key stored in VX(only consider the
     * lowest nibble) is not pressed (usually the next instruction is a jump
     * to skip a code block).
     */
    OP_KEY_NSKIP,
    /**
     * Opcode: FX07
     * Sets VX to the value of the delay timer.
     */
    OP_GET_DELAY,
    /**
     * Opcode: FX0A
     * A key press is awaited, and then stored in VX (blocking operation, all
     * instruction halted until next key event, delay and sound timers should
     * continue processing).
     */
    OP_AWAIT_KEY,
    /**
     * Opcode: FX15
     * Sets the delay timer to VX.
     */
    OP_SET_DELAY,
    /**
     * Opcode: FX18
     * Sets the sound timer to VX.
     */
    OP_SET_SOUND,
    /**
     * Opcode: FX1E
     * Adds VX to I. VF is not affected.
     */
    OP_ADR_ADD,
    /**
     * Opcode: FX29
     * Sets I to the location of the sprite for the character in VX (only
     * consider the lowest nibble). Characters 0-F (in hexadecimal) are
     * represented by a 4x5 font.
     */
    OP_SPRITE,
    /**
     * Opcode: FX33
     * Stores the binary-coded decimal representation of VX, with the hundreds
     * digit in memory at location in I, the tens digit at location I+1,
     * and the ones digit at location I+2.
     * 
     * Can be implemented in the following fasion
     * 
     * set_BCD(Vx)
     *     *(I+0) = BCD(3);
     *     *(I+1) = BCD(2);
     *     *(I+2) = BCD(1);
     */
    OP_BCD,
    /**
     * Opcode: FX55
     * Stores from V0 to VX (including VX) in memory, starting at address I.
     * The offset from I is increased by 1 for each value written, but I itself
     * is left unmodified.
     */
    OP_REG_DUMP,
    /**
     * Opcode: FX65
     * Fills from V0 to VX (including VX) with values from memory, starting at
     * address I. The offset from I is increased by 1 for each value read, but I
     * itself is left unmodified.
     */
    OP_REG_LOAD,

    /**
     * Not a CHIP-8 command, rather a signaling opcode that the provided 2 bytes
     * do not correspond to any meaningful instruction.
     */
    OP_UNKNOWN,
} OpcodeType;

/**
 * CHIP-8 has 35 opcodes, which are all two bytes long and stored big-endian. 
 */
WORD readNextOpcode(const BYTE* memory);

/**
 * Gets opcode type for the raw provided opcode.
 */
OpcodeType getOpcodeType(WORD opcode);

#endif  // CHIP8VM_OPCODES_H_

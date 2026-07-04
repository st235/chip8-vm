#ifndef CHIP8VM_VM_H_
#define CHIP8VM_VM_H_

#include <stddef.h>
#include <stdbool.h>

#include "types.h"

#define CHIP8_ADDRESS_SPACE 0x1000

typedef void(*ClearDisplayFunc)();
typedef bool(*DrawFunc)(BYTE x, BYTE y, BYTE* sprite, size_t size);

/**
 * Virtual machine implementation a fantasy console CHIP-8 created in 1977,
 * initially designed to ease game development for the COSMAC VIP kit computer.
 */
typedef struct {
    // CHIP-8 machines had 4096 memory locations,
    // all of which are 8 bits (aka a byte).
    BYTE memory[CHIP8_ADDRESS_SPACE];

    // CHIP-8 has 16 8-bit data registered named V0 to VF.
    // VF doubles as a flag for some instructions.
    BYTE registers[16];

    // Address register - named I - is 12 bits, though we can
    // allocate 16 bits as they will be padded anyway.
    WORD address_register;

    // The CHIP-8 interpreter must guarantee sufficient stack space for up to
    // twelve successive subroutine calls. Modern implementations may wish to
    // allocate enough memory for more than twelve calls.
    WORD* stack;
    uint8_t stack_size;
    uint8_t stack_capacity;

    // Address of a currently executed instruction.
    WORD program_counter;

    ClearDisplayFunc clear_display_func;
    DrawFunc draw_func;
} VM;

/**
 * Initialises a virtual machine instance with some default state.
 *
 * <p> This is the first call that should be done to a vm instance. Whenever
 * 
 */
void initVM(VM* vm);

/**
 * Releases resources associated with the given virtual machine instance.
 *
 * <p> Does not free the vm pointer itself as the method assumes non-owning
 * semantic.
 */
void freeVM(VM* vm);

/**
 * Initialises virtual machine memory with the provided ROM data.
 * 
 * @returns true if vm is initialized successfully with the given data, and
 *          false otherwise.
 */
bool loadROM(VM* vm, const BYTE* data, size_t size);

void registerClearDisplayFunc(VM* vm, ClearDisplayFunc func);
void registerDrawFunc(VM* vm, DrawFunc func);

/**
 * Current instruction execution status.
 */
typedef enum {
    /**
     * Tried to execute an unsupported operation.
     *
     * <p> So far only OP_CALL_ASSEMBLY (aka ONNN) is unsupported as it requires
     * an additional layer of inderection and not widely adopted.
     */
    STATUS_UNSUPPORTED_INSTRUCTION,
    /**
     * Saw unknown instruction in the bytecode.
     */
    STATUS_UNKNOWN_INSTRUCTION,
    /**
     * There was an error while evaluating an instruction.
     *
     * <p> For instance, jumping to the unsupported address.
     */
    STATUS_RUNTIME_ERROR,
    /**
     * PC is outside of the VM memory bounds.
     *
     * <p> Most likely something is wrong with the ROM, or - in much rare case -
     * it was intended to have a linear program with no inner loop.
     */
    STATUS_OUT_OF_MEMORY_BOUNDS,
    /**
     * Execution went well, can evaluate next instruction.
     */
    STATUS_STEP_EXECUTED,
} StepStatus;

/**
 * Evaluates current instruction.
 * 
 * @returns StepStatus as an operation status code.
 */
StepStatus step(VM* vm);

#endif  // CHIP8VM_VM_H_

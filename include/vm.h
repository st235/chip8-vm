#ifndef CHIP8VM_VM_H_
#define CHIP8VM_VM_H_

#include <stddef.h>
#include <stdbool.h>

#include "types.h"

/**
 * The CHIP-8 interpreter allows for graphics output onto a monochrome screen
 * of 64 × 32 pixels. The top-left corner of the screen is assigned (x,y)
 * coordinates of (0x00, 0x00), and the bottom-right is assigned (0x3F, 0x1F).
 */
#define CHIP8_SCREEN_WIDTH 64
#define CHIP8_SCREEN_HEIGHT 32

#define CHIP8_ADDRESS_SPACE 0x1000

/**
 * Ticks in milliseconds required to keep internal timers updated.
 */
typedef uint64_t delta_time_t;

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

    // 64 (width) by 32 (height) pixels screen data.
    BYTE virtual_display[CHIP8_SCREEN_HEIGHT * CHIP8_SCREEN_WIDTH];

    // The CHIP-8 interpreter will accept input from a 16-key keypad,
    // with each key corresponding to a single unique hexadecimal digit.
    uint16_t keyboard_state;

    // The sound timer provides CHIP-8's only facility for sound output.
    // While the sound timer's value is non-zero, a tone will be emitted from
    // the system's speaker.
    BYTE sound_timer;

    // When a timer is set to a non-zero value, it will count down at a
    // rate of sixty hertz until zero is reached.
    BYTE delay_timer;
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

/**
 * Sets a state for the given keycode.
 * 
 * <p> There are 16 keycodes (corresponding to keys 1 to F) and the key can
 * either be pressed or not.
 * 
 * <p> Keycodes are 0-indexed and should be within [0, 0xF] bytes interval.
 * 
 * <p> The layout of the 16-key keypad is as follows:
 * ╔═══╦═══╦═══╦═══╗
 * ║ 1 ║ 2 ║ 3 ║ C ║
 * ╠═══╬═══╬═══╬═══╣
 * ║ 4 ║ 5 ║ 6 ║ D ║
 * ╠═══╬═══╬═══╬═══╣
 * ║ 7 ║ 8 ║ 9 ║ E ║
 * ╠═══╬═══╬═══╬═══╣
 * ║ A ║ 0 ║ B ║ F ║
 * ╚═══╩═══╩═══╩═══╝
 * 
 * @returns true if key status was changed successfully and false otherwise.
 */
bool setKeyState(VM* vm, BYTE keycode, bool isPressed);

/**
 * Clear key states for the given virtual machine.
 */
void clearKeyStates(VM* vm);

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

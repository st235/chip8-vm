#ifndef CHIP8VM_VM_H_
#define CHIP8VM_VM_H_

#include <stddef.h>
#include <stdbool.h>

#include "types.h"

#define CHIP8_ADDRESS_SPACE 0x1000

typedef void(*ClearDisplayFunc)();
typedef bool(*DrawFunc)(BYTE x, BYTE y, BYTE* sprite, size_t size);

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

    WORD* stack;
    uint8_t stack_size;
    uint8_t stack_capacity;

    WORD program_counter;

    ClearDisplayFunc clear_display_func;
    DrawFunc draw_func;
} VM;

typedef enum {
    UNSUPPORTED_INSTRUCTION,
    UNKNOWN_INSTRUCTION,
    RUNTIME_ERROR,
    STEP,
    COMPLETED,
} RunStatus;

void initVM(VM* vm);
void freeVM(VM* vm);

void registerClearDisplayFunc(VM* vm, ClearDisplayFunc func);
void registerDrawFunc(VM* vm, DrawFunc func);

bool pushReturnAddress(VM* vm, WORD address);
bool popReturnAddress(VM* vm, WORD* out_address);

bool loadROM(VM* vm, const char* filename);
RunStatus step(VM* vm);
RunStatus run(VM* vm);

#endif  // CHIP8VM_VM_H_

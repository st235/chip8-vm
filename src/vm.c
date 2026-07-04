#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "opcodes.h"

#define STACK_GROWTH_FACTOR 2
#define STACK_INITIAL_SIZE 2
#define STACK_MAX_SIZE 256

static BYTE kFont[16][5] = {
    {0xF0, 0x90, 0x90, 0x90, 0xF0},
    {0x20, 0x60, 0x20, 0x20, 0x70},
    {0xF0, 0x10, 0xF0, 0x80, 0xF0},
    {0xF0, 0x10, 0xF0, 0x10, 0xF0},
    {0x90, 0x90, 0xF0, 0x10, 0x10},
    {0xF0, 0x80, 0xF0, 0x10, 0xF0},
    {0xF0, 0x80, 0xF0, 0x90, 0xF0},
    {0xF0, 0x10, 0x20, 0x40, 0x40},
    {0xF0, 0x90, 0xF0, 0x90, 0xF0},
    {0xF0, 0x90, 0xF0, 0x10, 0xF0},
    {0xF0, 0x90, 0xF0, 0x90, 0x90},
    {0xE0, 0x90, 0xE0, 0x90, 0xE0},
    {0xF0, 0x80, 0x80, 0x80, 0xF0},
    {0xE0, 0x90, 0x90, 0x90, 0xE0},
    {0xF0, 0x80, 0xF0, 0x80, 0xF0},
    {0xF0, 0x80, 0xF0, 0x80, 0xF0},
};

static bool pushReturnAddress(VM* vm, WORD address) {
    if (vm->stack_size + 1 >= STACK_MAX_SIZE) {
        return false;
    }

    if (vm->stack_size + 1 > vm->stack_capacity) {
        WORD* old_stack = vm->stack;
        vm->stack_capacity *= STACK_GROWTH_FACTOR;
        vm->stack = realloc(old_stack, sizeof(WORD) * vm->stack_capacity);
    }

    vm->stack[vm->stack_size] = address;
    vm->stack_size += 1;

    return true;
}

static bool popReturnAddress(VM* vm, WORD* out_address) {
    if (vm->stack_size == 0) {
        return false;
    }

    *out_address = vm->stack[vm->stack_size - 1];
    vm->stack_size -= 1;
    return true;
}

static bool executeOpReturn(VM* vm) {
    WORD poped_address = 0;
    if (!popReturnAddress(vm, &poped_address)) {
        return false;
    }
    vm->program_counter = poped_address;
    return true;
}

static void executeOpJump(VM* vm, WORD opcode, BYTE offset) {
    WORD new_address = (opcode & 0x0FFF);
    vm->program_counter = new_address + offset;
}

static void executeOpCall(VM* vm, WORD opcode) {
    WORD new_address = (opcode & 0x0FFF);
    if (!pushReturnAddress(vm, vm->program_counter)) {
    }
    vm->program_counter = new_address;
}

void initVM(VM* vm) {
    srand(time(NULL));
    memset(vm->memory, 0, sizeof(vm->memory) / sizeof(BYTE));
    memcpy(vm->memory, kFont, sizeof(kFont) / sizeof(BYTE));

    memset(vm->registers, 0, sizeof(vm->registers) / sizeof(BYTE));
    vm->program_counter = 0;
    vm->program_counter = 0x200;
    vm->stack = malloc(sizeof(WORD) * STACK_INITIAL_SIZE);
    vm->stack_size = 0U;
    vm->stack_capacity = STACK_INITIAL_SIZE;

    vm->clear_display_func = NULL;
    vm->draw_func = NULL;
}

void freeVM(VM* vm) {
    free(vm->stack);
}

void registerClearDisplayFunc(VM* vm, ClearDisplayFunc func) {
    vm->clear_display_func = func;
}

void registerDrawFunc(VM* vm, DrawFunc func) {
    vm->draw_func = func;
}

bool loadROM(VM* vm, const BYTE* data, size_t size) {
    if (data == NULL || size > 0xE00) {
        return false;
    }
    memcpy(&vm->memory[0x200], data, size);
    return true;
}

StepStatus step(VM* vm) {
    if (vm->program_counter >= CHIP8_ADDRESS_SPACE) {
        // We are most likely long outside of our allocated memory bounds,
        // aborting.
        return STATUS_OUT_OF_MEMORY_BOUNDS;
    }

    WORD opcode = readNextOpcode(&vm->memory[vm->program_counter]);
    vm->program_counter += 2;
    OpcodeType type = getOpcodeType(opcode);

    // printf("pc %x\nI %x\nopcode: %x\n", vm->program_counter - 2, vm->address_register, opcode);
    // for (int i = 0; i < 16; i++) {
    //     printf("v%x: %x ", i, vm->registers[i]);
    // }
    // printf("\n");

    switch (type) {
        case OP_CALL_ASSEMBLY: {
            return STATUS_UNSUPPORTED_INSTRUCTION;
        }
        case OP_CLEAR_DISPLAY: {
            if (vm->clear_display_func != NULL) {
                vm->clear_display_func();
            }
            break;
        }
        case OP_RETURN: {
            if (!executeOpReturn(vm)) {
                return STATUS_RUNTIME_ERROR;
            }
            break;
        }
        case OP_JUMP: {
            executeOpJump(vm, opcode, /* offset= */ 0);
            break;
        }
        case OP_CALL: {
            executeOpCall(vm, opcode);
            break;
        }
        case OP_SKIP_EQ: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            uint8_t val = (opcode & 0x00FF);
            if (vm->registers[reg] == val) {
                vm->program_counter += 2;
            }
            break;
        }
        case OP_SKIP_NE: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            uint8_t val = (opcode & 0x00FF);
            if (vm->registers[reg] != val) {
                vm->program_counter += 2;
            }
            break;
        }
        case OP_SKIP_CMP: {
            uint8_t reg1 = (opcode & 0x0F00) >> 8;
            uint8_t reg2 = (opcode & 0x00F0) >> 4;
            if (vm->registers[reg1] == vm->registers[reg2]) {
                vm->program_counter += 2;
            }
            break;
        }
        case OP_SET: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            uint8_t val = (opcode & 0x00FF);
            vm->registers[reg] = val;
            break;
        }
        case OP_ADD: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            uint8_t val = (opcode & 0x00FF);
            vm->registers[reg] += val;
            break;
        }
        case OP_ASS: {
            uint8_t reg1 = (opcode & 0x0F00) >> 8;
            uint8_t reg2 = (opcode & 0x00F0) >> 4;
            vm->registers[reg1] = vm->registers[reg2];
            break;
        }
        case OP_OR: {
            uint8_t reg1 = (opcode & 0x0F00) >> 8;
            uint8_t reg2 = (opcode & 0x00F0) >> 4;
            vm->registers[reg1] |= vm->registers[reg2];
            break;
        }
        case OP_AND: {
            uint8_t reg1 = (opcode & 0x0F00) >> 8;
            uint8_t reg2 = (opcode & 0x00F0) >> 4;
            vm->registers[reg1] &= vm->registers[reg2];
            break;
        }
        case OP_XOR: {
            uint8_t reg1 = (opcode & 0x0F00) >> 8;
            uint8_t reg2 = (opcode & 0x00F0) >> 4;
            vm->registers[reg1] ^= vm->registers[reg2];
            break;
        }
        case OP_ASS_ADD: {
            uint8_t reg1 = (opcode & 0x0F00) >> 8;
            uint8_t reg2 = (opcode & 0x00F0) >> 4;
            BYTE val1 = vm->registers[reg1];
            BYTE val2 = vm->registers[reg2];
            vm->registers[0xF] = ((uint16_t)val1 + val2 >= 256) ? 1 : 0;
            vm->registers[reg1] = val1 + val2;
            break;
        }
        case OP_ASS_SUB: {
            uint8_t reg1 = (opcode & 0x0F00) >> 8;
            uint8_t reg2 = (opcode & 0x00F0) >> 4;
            vm->registers[0xF] =
                (vm->registers[reg1] >= vm->registers[reg2]) ? 1 : 0;
            vm->registers[reg1] -= vm->registers[reg2];
            break;
        }
        case OP_SHR: {
            uint8_t reg1 = (opcode & 0x0F00) >> 8;
            uint8_t reg2 = (opcode & 0x00F0) >> 4;
            vm->registers[0xF] = (0x01 & vm->registers[reg2]);
            vm->registers[reg1] = vm->registers[reg2] >> 1;
            break;
        }
        case OP_ASS_NSUB: {
            uint8_t reg1 = (opcode & 0x0F00) >> 8;
            uint8_t reg2 = (opcode & 0x00F0) >> 4;
            vm->registers[0xF] =
                (vm->registers[reg2] >= vm->registers[reg1]) ? 1 : 0;
            vm->registers[reg1] = vm->registers[reg2] - vm->registers[reg1];
            break;
        }
        case OP_SHL: {
            uint8_t reg1 = (opcode & 0x0F00) >> 8;
            uint8_t reg2 = (opcode & 0x00F0) >> 4;
            vm->registers[0xF] = (0x10 & vm->registers[reg2]);
            vm->registers[reg1] = vm->registers[reg2] << 1;
            break;
        }
        case OP_SKIP_NCMP: {
            uint8_t reg1 = (opcode & 0x0F00) >> 8;
            uint8_t reg2 = (opcode & 0x00F0) >> 4;
            if (vm->registers[reg1] != vm->registers[reg2]) {
                vm->program_counter += 2;
            }
            break;
        }
        case OP_ADR_SET: {
            WORD val = (opcode & 0x0FFF);
            vm->address_register = val;
            break;
        }
        case OP_JUMP_V0: {
            executeOpJump(vm, opcode, /* offset= */ vm->registers[0]);
            break;
        }
        case OP_RAND: {
            uint8_t reg1 = (opcode & 0x0F00) >> 8;
            uint8_t val = (opcode & 0x00FF);
            vm->registers[reg1] = val & rand();
            break;
        }
        case OP_DRAW: {
            uint8_t x = vm->registers[((opcode & 0x0F00) >> 8)];
            uint8_t y = vm->registers[((opcode & 0x00F0) >> 4)];
            uint8_t height = (opcode & 0x000F);
            // printf("x %d y %d h %d\n", x, y, height);
            if (vm->draw_func != NULL && vm->draw_func(x, y, &vm->memory[vm->address_register], height)) {
                vm->registers[0xF] = 1;
            } else {
                vm->registers[0xF] = 0;
            }
            break;
        }
        case OP_KEY_SKIP: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            BYTE key = vm->registers[reg];
            // TODO
            break;
        }
        case OP_KEY_NSKIP: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            BYTE key = vm->registers[reg];
            // TODO
            break;
        }
        case OP_GET_DELAY: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            // TODO
            break;
        }
        case OP_AWAIT_KEY: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            // TODO
            break;
        }
        case OP_SET_DELAY: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            BYTE delay = vm->registers[reg];
            // TODO
            break;
        }
        case OP_SET_SOUND: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            BYTE sound = vm->registers[reg];
            break;
        }
        case OP_ADR_ADD: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            vm->address_register += vm->registers[reg];
            break;
        }
        case OP_SPRITE: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            BYTE hexdigit = vm->registers[reg];
            vm->address_register = sizeof(kFont[0]) * hexdigit;
            break;
        }
        case OP_BCD: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            BYTE val = vm->registers[reg];

            vm->memory[vm->address_register + 0] = (BYTE)(val / 100);
            vm->memory[vm->address_register + 1] = (BYTE)((val / 10) % 10);
            vm->memory[vm->address_register + 2] = (BYTE)(val % 10);
            break;
        }
        case OP_REG_DUMP: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            memcpy(&vm->memory[vm->address_register], vm->registers, reg + 1);
            break;
        }
        case OP_REG_LOAD: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            memcpy(vm->registers, &vm->memory[vm->address_register], reg + 1);
            break;
        }
        case OP_UNKNOWN: {
            return STATUS_UNKNOWN_INSTRUCTION;
        }
    }

    return STATUS_STEP_EXECUTED;
}

#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "opcodes.h"

#define STACK_GROWTH_FACTOR 2
#define STACK_INITIAL_SIZE 2
#define STACK_MAX_SIZE 256
#define TIMERS_UPDATE_INTERVAL_MS 17

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
    {0xF0, 0x80, 0xF0, 0x80, 0x80},
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

static bool executeOpDraw(VM* vm, BYTE x, BYTE y, BYTE height) {
    const BYTE* sprite = &vm->memory[vm->address_register];

    // Set VF to 01 if any set pixels are changed to unset, and 00 otherwise.
    bool was_unset = false;

    for (uint8_t row = 0; row < height; row++) {
        for (uint8_t column = 0; column < 8; column++) {
            uint8_t sprite_value = (sprite[row] & (1 << (7 - column))) == 0 ? 0 : 1;
            
            uint16_t display_index = (y + row) * CHIP8_SCREEN_WIDTH + (x + column);
            BYTE new_value = vm->virtual_display[display_index] ^ sprite_value;

            if (vm->virtual_display[display_index] && !new_value) {
                was_unset = true;
            }

            vm->virtual_display[display_index] = new_value;
        }
    }

    return was_unset;
}

static void updateTimers(VM* vm, uint64_t time_ms) {
    uint64_t dt = time_ms - vm->last_timers_update_time_ms;
    vm->last_timers_update_time_ms = time_ms;

    if (dt < TIMERS_UPDATE_INTERVAL_MS) {
        return;
    }

    if (vm->delay_timer > 0) {
        vm->delay_timer--;
    }

    if (vm->sound_timer > 0) {
        OnBeepFunc func = vm->on_beep_func;
        if (func != NULL) {
            func();
        }
        vm->sound_timer--;
    }
}

static void resetVM(VM* vm) {
    memset(vm->registers, 0, sizeof(vm->registers) / sizeof(BYTE));

    vm->address_register = 0U;
    vm->program_counter = 0x200;

    vm->stack_size = 0U;

    memset(&vm->virtual_display,
            /* ch= */ 0,
            /* count= */ CHIP8_SCREEN_HEIGHT * CHIP8_SCREEN_WIDTH);

    vm->keyboard_state = 0U;

    vm->last_timers_update_time_ms = 0U;
    vm->sound_timer = 0U;
    vm->delay_timer = 0U;
    vm->on_beep_func = NULL;
}

void initVM(VM* vm) {
    srand(time(NULL));
    memset(vm->memory, 0, sizeof(vm->memory) / sizeof(BYTE));
    memcpy(vm->memory, kFont, sizeof(kFont) / sizeof(BYTE));

    vm->stack = malloc(sizeof(WORD) * STACK_INITIAL_SIZE);
    vm->stack_size = 0U;
    vm->stack_capacity = STACK_INITIAL_SIZE;

    resetVM(vm);
}

void freeVM(VM* vm) {
    free(vm->stack);
}

bool loadROM(VM* vm, const BYTE* data, size_t size) {
    if (data == NULL || size > 0xE00) {
        return false;
    }

    // Loading ROM in memory.
    memcpy(&vm->memory[0x200], data, size);

    // Resetting VM state.
    resetVM(vm);

    return true;
}

bool setKeyState(VM* vm, BYTE keycode, bool isPressed) {
    if (keycode > 0xF) {
        return false;
    }

    uint8_t key_state = isPressed ? 1U : 0U;
    vm->keyboard_state &= ~(1 << keycode);
    vm->keyboard_state |= (isPressed << keycode);
    return true;
}

void clearKeyStates(VM* vm) {
    vm->keyboard_state = 0;
}

StepStatus step(VM* vm, uint64_t time_ms) {
    if (vm->program_counter >= CHIP8_ADDRESS_SPACE) {
        // We are most likely long outside of our allocated memory bounds,
        // aborting.
        return STATUS_OUT_OF_MEMORY_BOUNDS;
    }

    updateTimers(vm, time_ms);

    WORD opcode = readNextOpcode(&vm->memory[vm->program_counter]);
    OpcodeType type = getOpcodeType(opcode);

#ifdef CHIP8_DEBUG_VM
    printf("    PC: 0x%04X\n     I: 0x%04X\nOpcode: 0x%04X (%02d)\n Stack: ",
           vm->program_counter,
           vm->address_register,
            opcode, type);
    if (vm->stack_size) {
        for (int i = 0; i < vm->stack_size; i++) {
            if (i > 0) {
                printf(" -> ");
            }
            printf("0x%04X", vm->stack[i]);
        }
    } else {
        printf("<empty>");
    }

    printf("\nRegisters:\n");
    for (int r = 0; r <= 0xF; r++) {
        printf("V%X: %02X ", r, vm->registers[r]);
    }
    printf("\n\n");
#endif

    vm->program_counter += 2;

    switch (type) {
        case OP_CALL_ASSEMBLY: {
            return STATUS_UNSUPPORTED_INSTRUCTION;
        }
        case OP_CLEAR_DISPLAY: {
            memset(&vm->virtual_display,
                   /* ch= */ 0,
                   /* count= */ CHIP8_SCREEN_HEIGHT * CHIP8_SCREEN_WIDTH);
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
            if (executeOpDraw(vm, x, y, height)) {
                vm->registers[0xF] = 1;
            } else {
                vm->registers[0xF] = 0;
            }
            break;
        }
        case OP_KEY_SKIP: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            BYTE key = vm->registers[reg];
            if ((vm->keyboard_state & (1 << key)) != 0) {
                vm->program_counter += 2;
            }
            break;
        }
        case OP_KEY_NSKIP: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            BYTE key = vm->registers[reg];
            if ((vm->keyboard_state & (1 << key)) == 0) {
                vm->program_counter += 2;
            }
            break;
        }
        case OP_GET_DELAY: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            vm->registers[reg] = vm->delay_timer;
            break;
        }
        case OP_AWAIT_KEY: {
            uint8_t reg = (opcode & 0x0F00) >> 8;

            bool key_pressed = false;
            for (uint8_t i = 0; i < 0xF; i++) {
                if (vm->keyboard_state & (1 << i)) {
                    key_pressed = true;
                }
            }

            if (!key_pressed) {
                // Key was not pressed this cycle, rewind back to AWAIT
                // instruction.
                vm->program_counter -= 2;
            }

            break;
        }
        case OP_SET_DELAY: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            BYTE delay = vm->registers[reg];
            vm->delay_timer = delay;
            break;
        }
        case OP_SET_SOUND: {
            uint8_t reg = (opcode & 0x0F00) >> 8;
            BYTE sound = vm->registers[reg];
            vm->sound_timer = sound;
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

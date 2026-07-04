#ifndef CHIP8VM_TYPES_H_
#define CHIP8VM_TYPES_H_

#include <stdint.h>

typedef uint8_t BYTE;
typedef uint16_t WORD;

#ifdef _CHIP8_DEBUG

#define CHIP8_DEBUG_VM

#endif  // _CHIP8_DEBUG

#endif  // CHIP8VM_TYPES_H_

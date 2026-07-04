#ifndef CHIP8VM_ROM_H_
#define CHIP8VM_ROM_H_

#include <stddef.h>

#include "types.h"

/**
 * A helper function that reads a game ROM content from disk.
 * 
 * @returns a pointer to allocated memory that should be freed when used,
 *          or NULL if error.
 */
const BYTE* readROM(const char* filename, size_t* out_size);

#endif  // CHIP8VM_ROM_H_

#include "rom.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

const BYTE* readROM(const char* filename, size_t* out_size) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END)) {
        fclose(file);
        return NULL;
    }

    size_t file_size = ftell(file);
    if (file_size > 0xE00) {
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, 0)) {
        fclose(file);
        return NULL;
    }

    BYTE* buffer = (BYTE*)malloc(file_size * sizeof(BYTE));
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    if (!fread(buffer, sizeof(BYTE), file_size, file) || ferror(file)) {
        free(buffer);
        fclose(file);
        return NULL;
    }

    if (out_size != NULL) {
        *out_size = file_size;
    }
    return buffer;
}

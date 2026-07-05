#define SDL_MAIN_USE_CALLBACKS

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

#include "rom.h"
#include "vm.h"

typedef struct {
    VM vm;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Palette* palette;
    bool debug_step_over;
} AppState;

static void initAppState(AppState* state, const char* romfile) {
    initVM(&state->vm);

    state->window = NULL;
    state->renderer = NULL;
    state->palette = NULL;

    size_t rom_size = 0;
    const BYTE* rom_data = readROM(romfile, &rom_size);
    loadROM(&state->vm, rom_data, rom_size);
    free((void*)rom_data);

    state->debug_step_over = false;
}

static void freeAppState(AppState* state) {
    freeVM(&state->vm);
    SDL_DestroyPalette(state->palette);
    SDL_DestroyRenderer(state->renderer);
    SDL_DestroyWindow(state->window);
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    AppState* state = (AppState*)appstate;
    VM* vm = &state->vm;

#ifdef CHIP8_DEBUG_VM
    if (!((AppState*)appstate)->debug_step_over) {
        return SDL_APP_CONTINUE;
    }
    ((AppState*)appstate)->debug_step_over = false;
#endif

    if (step(vm, SDL_GetTicks()) != STATUS_STEP_EXECUTED) {
        return SDL_APP_FAILURE;
    }

    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        CHIP8_SCREEN_WIDTH,
        CHIP8_SCREEN_HEIGHT,
        SDL_PIXELFORMAT_INDEX8,
        &vm->virtual_display,
        CHIP8_SCREEN_WIDTH
    );

    SDL_RenderClear(state->renderer);

    SDL_SetSurfacePalette(surface, state->palette);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(state->renderer, surface);

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    SDL_RenderTexture(state->renderer, texture, NULL, NULL);

    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);

    SDL_RenderPresent(state->renderer);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    VM* vm = &((AppState*)appstate)->vm;
    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_KEY_UP:
        case SDL_EVENT_KEY_DOWN:
            switch (event->key.scancode) {
                case SDL_SCANCODE_ESCAPE: return SDL_APP_SUCCESS;
#ifdef CHIP8_DEBUG_VM
                case SDL_SCANCODE_N: {
                    if (event->key.down) {
                        ((AppState*)appstate)->debug_step_over = true;
                    }
                    break;
                }
#endif
                case SDL_SCANCODE_1: {
                    setKeyState(vm, 0x0, event->key.down);
                    break;
                }
                case SDL_SCANCODE_2: {
                    setKeyState(vm, 0x1, event->key.down);
                    break;
                }
                case SDL_SCANCODE_3: {
                    setKeyState(vm, 0x2, event->key.down);
                    break;
                }
                case SDL_SCANCODE_4: {
                    setKeyState(vm, 0x3, event->key.down);
                    break;
                }
                case SDL_SCANCODE_Q: {
                    setKeyState(vm, 0x4, event->key.down);
                    break;
                }
                case SDL_SCANCODE_W: {
                    setKeyState(vm, 0x5, event->key.down);
                    break;
                }
                case SDL_SCANCODE_E: {
                    setKeyState(vm, 0x6, event->key.down);
                    break;
                }
                case SDL_SCANCODE_R: {
                    setKeyState(vm, 0x7, event->key.down);
                    break;
                }
                case SDL_SCANCODE_A: {
                    setKeyState(vm, 0x8, event->key.down);
                    break;
                }
                case SDL_SCANCODE_S: {
                    setKeyState(vm, 0x9, event->key.down);
                    break;
                }
                case SDL_SCANCODE_D: {
                    setKeyState(vm, 0xA, event->key.down);
                    break;
                }
                case SDL_SCANCODE_F: {
                    setKeyState(vm, 0xB, event->key.down);
                    break;
                }
                case SDL_SCANCODE_Z: {
                    setKeyState(vm, 0xC, event->key.down);
                    break;
                }
                case SDL_SCANCODE_X: {
                    setKeyState(vm, 0xD, event->key.down);
                    break;
                }
                case SDL_SCANCODE_C: {
                    setKeyState(vm, 0xE, event->key.down);
                    break;
                }
                case SDL_SCANCODE_V: {
                    setKeyState(vm, 0xF, event->key.down);
                    break;
                }
            }
            break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char *argv[]) {
    SDL_SetAppMetadata("CHIP-8 Virtual Machine", "1.0", "com.github.st235.chip8vm");

    AppState* state = SDL_malloc(sizeof(AppState));
    initAppState(state, argv[1]);

    *appstate = state;

#ifndef __EMSCRIPTEN__
    if (argc != 2) {
        // Uses printf to avoid SDL logging prefix.
        printf("ROM was not provided. Usage: chip8vm rom.ch8\n");
        return SDL_APP_FAILURE;
    }
#endif

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init(SDL_INIT_VIDEO) failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer(argv[1], 640, 320, SDL_WINDOW_RESIZABLE,
            &state->window, &state->renderer)) {
        SDL_Log("SDL_CreateWindowAndRenderer() failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->palette = SDL_CreatePalette(256);

    SDL_Color colors[2] = {
        {  0,   0,   0, 255},
        {255, 255, 255, 255},
    };
    SDL_SetPaletteColors(state->palette, colors, 0, 2);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    freeAppState(appstate);
    SDL_free(appstate);
    SDL_Quit();
}

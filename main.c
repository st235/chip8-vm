#define SDL_MAIN_USE_CALLBACKS

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "vm.h"
#include "rom.h"

#define VIRTUAL_SCREEN_WIDTH 64
#define VIRTUAL_SCREEN_HEIGHT 32

typedef struct {
    VM vm;
    bool debug_step_over;
} AppState;

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Palette* display_palette = NULL;
static AppState* appstate = NULL;

static void initAppState(AppState* state, const char* romfile) {
    appstate = state;

    initVM(&state->vm);

    size_t rom_size = 0;
    const BYTE* rom_data = readROM(romfile, &rom_size);
    loadROM(&state->vm, rom_data, rom_size);
    free((void*)rom_data);

    state->debug_step_over = false;
}

static void freeAppState(AppState* state) {
    freeVM(&state->vm);
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    VM* vm = &((AppState*)appstate)->vm;

#ifdef CHIP8_DEBUG_VM
    if (!((AppState*)appstate)->debug_step_over) {
        return SDL_APP_CONTINUE;
    }
    ((AppState*)appstate)->debug_step_over = false;
#endif

    if (step(vm) != STATUS_STEP_EXECUTED) {
        return SDL_APP_FAILURE;
    }

    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        VIRTUAL_SCREEN_WIDTH,
        VIRTUAL_SCREEN_HEIGHT,
        SDL_PIXELFORMAT_INDEX8,
        &vm->virtual_display,
        VIRTUAL_SCREEN_WIDTH
    );

    SDL_RenderClear(renderer);

    SDL_SetSurfacePalette(surface, display_palette);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    SDL_RenderTexture(renderer, texture, NULL, NULL);

    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);

    SDL_RenderPresent(renderer);
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

    if (argc != 2) {
        // Uses printf to avoid SDL logging prefix.
        printf("ROM was not provided. Usage: chip8vm rom.ch8\n");
        return SDL_APP_FAILURE;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init(SDL_INIT_VIDEO) failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer(argv[1], 640, 320, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("SDL_CreateWindowAndRenderer() failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    display_palette = SDL_CreatePalette(256);

    SDL_Color colors[2] = {
        {0,   0,   0,   255}, // index 0 = black
        {255, 255, 255, 255}  // index 1 = white
    };
    SDL_SetPaletteColors(display_palette, colors, 0, 2);

    *appstate = SDL_malloc(sizeof(AppState));
    initAppState(*appstate, argv[1]);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    freeAppState(appstate);
    SDL_free(appstate);
    SDL_DestroyPalette(display_palette);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

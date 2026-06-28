#define SDL_MAIN_USE_CALLBACKS

#include <stdio.h>
#include <stdbool.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "vm.h"

#define VIRTUAL_SCREEN_WIDTH 64
#define VIRTUAL_SCREEN_HEIGHT 32

typedef struct {
    VM vm;
    BYTE* display_data;
} AppState;

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Palette* display_palette = NULL;
static AppState* appstate = NULL;

static bool next = false;

static void clearDisplay() {
    memset(appstate->display_data, 0, VIRTUAL_SCREEN_WIDTH * VIRTUAL_SCREEN_HEIGHT);
}

static bool draw(BYTE x, BYTE y, BYTE* sprite, size_t height) {
    uint8_t tx = (x + 8 < VIRTUAL_SCREEN_WIDTH) ? (x + 8) : VIRTUAL_SCREEN_WIDTH;
    uint8_t ty = (y + height < VIRTUAL_SCREEN_HEIGHT) ? (y + height) : VIRTUAL_SCREEN_HEIGHT;
    // printf("tx: %d, ty: %d\n", tx, ty);

    bool flag = false;

    for (uint8_t cx = 0; cx < 8; cx++) {
        for (uint8_t cy = 0; cy < height; cy++) {
            uint8_t sprite_value = sprite[cy] & (1 << (7 - cx));
            
            uint16_t display_index = (y + cy) * VIRTUAL_SCREEN_WIDTH + (x + cx);
            uint8_t new_value = appstate->display_data[display_index] ^ sprite_value;

            if (appstate->display_data[display_index] && !new_value) {
                flag = true;
            }

            // printf("cx %d cy %d %x\n", cx, cy, new_value);

            appstate->display_data[display_index] = new_value;
        }
    }

    return flag;
}

static void initAppState(AppState* state, const char* romfile) {
    appstate = state;
    state->display_data = SDL_malloc(VIRTUAL_SCREEN_WIDTH * VIRTUAL_SCREEN_HEIGHT);
    memset(state->display_data, 0, VIRTUAL_SCREEN_WIDTH * VIRTUAL_SCREEN_HEIGHT);

    initVM(&state->vm);
    loadROM(&state->vm, romfile);
    registerClearDisplayFunc(&state->vm, clearDisplay);
    registerDrawFunc(&state->vm, draw);
}

static void freeAppState(AppState* state) {
    freeVM(&state->vm);
    SDL_free(state->display_data);
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    if (!next) {
        return SDL_APP_CONTINUE;
    }

    // next = false;
    if (step(&((AppState*)appstate)->vm) != STEP) {
        return SDL_APP_FAILURE;
    }

    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        VIRTUAL_SCREEN_WIDTH,
        VIRTUAL_SCREEN_HEIGHT,
        SDL_PIXELFORMAT_INDEX8,
        ((AppState*)appstate)->display_data,
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
    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_KEY_DOWN:
            if (event->key.scancode == SDL_SCANCODE_ESCAPE) {
                return SDL_APP_SUCCESS;
            } else if (event->key.scancode == SDL_SCANCODE_N) {
                next = true;
            }
            break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char *argv[]) {
    SDL_SetAppMetadata("CHIP-8 Virtual Machine", "1.0", "com.github.st235.chip8vm");

    if (argc != 2) {
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

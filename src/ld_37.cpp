
#include <SDL2/SDL.h>

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

typedef struct {
    i32 width;
    i32 height;
    i64 pixel_buffer_size;
    i32 pixel_buffer_pitch;
    i32 pixels_per_meter;
    SDL_PixelFormat* pixel_format;
    void* pixel_buffer;
    char* title;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
} Display;

typedef union {
    struct {
        f32 x;
        f32 y;
    };

    struct {
        f32 w;
        f32 h;
    };
} Vec2;

typedef union {
    struct {
        f32 x;
        f32 y;
        f32 z;
        f32 w;
    };

    struct {
        f32 a;
        f32 r;
        f32 g;
        f32 b;
    };
} Vec4;

#define TARGET_MS_PER_FRAME 16
#define CAP_FRAMERATE       1

u32 ColourVec4ToU32(Vec4* vec4_colour) {
    u8 a = vec4_colour->a * 255;
    u8 r = vec4_colour->r * 255;
    u8 g = vec4_colour->g * 255;
    u8 b = vec4_colour->b * 255;

    u32 u32_colour = (a << 24) | (r << 16) | (g << 8) | b;
    return u32_colour;
}

void RenderFilledRect(Display* display, Vec2* pos, Vec2* size, u32 colour) {
    i32 y_min           = pos->y * display->pixels_per_meter;
    i32 y_max           = (pos->y + size->h) * display->pixels_per_meter;
    i32 render_width    = size->w * display->pixels_per_meter;

    for(i32 y = y_min; y < y_max; y++) {
        if(y < 0) {
            continue;
        }
        // if the rect is off the screen completly don't render
        else if(y >= display->height) {
            return;
        }

        i32 x_min = pos->x * display->pixels_per_meter;
        i32 x_max = (pos->x + size->w) * display->pixels_per_meter;

        if(x_min < 0) {
            // if the rect is off the screen completly don't render
            if(x_min <= -render_width) {
                return;
            }

            // NOTE (Mathew): x is negative here so the result will be a smaller width; the width
            //                of the remaining rectangle we want to render
            x_max = x_min + render_width;
            render_width = x_max - x_min;
            x_min = 0;
        }

        u32* pixel = (u32*)display->pixel_buffer + (y * display->width) + x_min;

        for(i32 x = x_min; x < x_max; x++) {
            if(y < display->height  && y >= 0 && x < display->width && x >= 0) {
                *pixel++ = colour;
            }
        }
    }
}

void RenderFilledRect(Display* display, Vec2* pos, Vec2* size, Vec4* vec4_colour) {
    RenderFilledRect(display, pos, size, ColourVec4ToU32(vec4_colour));
}

i32 main(i32 argc, char** argv) {
    srand(time(0));
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0) {
        printf("Error - Could not init SDL. SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    Display display = {0};
    display.width = 1024;
    display.height = 576;
    display.title = "LD 37";
    display.pixels_per_meter = 64;

    display.window = SDL_CreateWindow(display.title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, display.width, 
                                      display.height, SDL_WINDOW_SHOWN);
    if(!display.window) {
        printf("Error - Could not create window. SDL_Error: %s.\n", SDL_GetError());
        return -1;
    }

    display.renderer = SDL_CreateRenderer(display.window, -1, SDL_RENDERER_ACCELERATED);
    if(!display.renderer) {
        printf("Error - Could not create renderer. SDL_Error: %s.\n", SDL_GetError());
        return -1;
    }

    display.pixel_format = SDL_GetWindowSurface(display.window)->format;
    display.texture = SDL_CreateTexture(display.renderer, display.pixel_format->format, SDL_TEXTUREACCESS_STREAMING, display.width, display.height);
    if(!display.texture) {
        printf("Error - Could not create display texture. %s.\n", SDL_GetError());
        return -1;
    }

    display.pixel_buffer_pitch = display.width * display.pixel_format->BytesPerPixel;
    display.pixel_buffer_size = display.pixel_buffer_pitch * display.height;
    display.pixel_buffer = malloc(display.pixel_buffer_size);
    if(!display.pixel_buffer) {
        printf("Error - Could not create pixel buffer. %s.\n", SDL_GetError());
        return -1;
    }

    bool running = true;
    SDL_Event event = {0};
    Vec2 rect_pos = {100 / display.pixels_per_meter, 100 / display.pixels_per_meter};
    Vec2 rect_size = {100 / display.pixels_per_meter, 100 / display.pixels_per_meter};
    Vec4 rect_colour = {1.0f, 0.2f, 0.2f, 0.2f};
    f32 speed = 2 * ((f32)display.pixels_per_meter / (1000.0 / (f32)TARGET_MS_PER_FRAME));
    f32 last_delta = 0.0f;

    const i32 MAP_WIDTH = 17;
    const i32 MAP_HEIGHT = 10;
    const f32 TILE_SIZE = 64.0 / (f32)display.pixels_per_meter;
    u32 tile_map[MAP_WIDTH * MAP_HEIGHT];

    for(i32 y = 0; y < MAP_HEIGHT; y++) {
        for(i32 x = 0; x < MAP_WIDTH; x++) {
            u8 r = rand() % 255;
            u8 g = rand() % 255;
            u8 b = rand() % 255;
            tile_map[(y * MAP_WIDTH) + x] = (u32)(0xFF | (r << 16) | (g << 8) | b);
        }
    }

    while(running) {
        i32 ms_before = SDL_GetTicks();
        while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_QUIT: {
                    running = false;
                } break;
            }
        }

        u8* key_states = (u8*)SDL_GetKeyboardState(0);

        if(key_states[SDL_SCANCODE_UP] && !key_states[SDL_SCANCODE_DOWN]) {
            rect_pos.y -= speed * last_delta;
        }
        else if(key_states[SDL_SCANCODE_DOWN] && !key_states[SDL_SCANCODE_UP]) {
            rect_pos.y += speed * last_delta;
        }

        if(key_states[SDL_SCANCODE_LEFT] && !key_states[SDL_SCANCODE_RIGHT]) {
            rect_pos.x -= speed * last_delta;
        }
        else if(key_states[SDL_SCANCODE_RIGHT] && !key_states[SDL_SCANCODE_LEFT]) {
            rect_pos.x += speed * last_delta;
        }

        SDL_RenderClear(display.renderer);
        for(i32 i = 0; i < display.height * display.width; i++) {
            *((u32*)display.pixel_buffer + i) = 0xFF00FFFF;
        }

        for(i32 y = 0; y < MAP_HEIGHT; y++) {
            for(i32 x = 0; x < MAP_WIDTH; x++) {
                Vec2 pos = {x * TILE_SIZE, y * TILE_SIZE};
                Vec2 size = {TILE_SIZE, TILE_SIZE};
                RenderFilledRect(&display, &pos, &size, tile_map[(y * MAP_WIDTH) + x]);
            }
        }

        RenderFilledRect(&display, &rect_pos, &rect_size, &rect_colour);

        SDL_UpdateTexture(display.texture, 0, display.pixel_buffer, display.pixel_buffer_pitch);
        SDL_RenderCopy(display.renderer, display.texture, NULL, NULL);
        SDL_RenderPresent(display.renderer);

        i32 ms_after = SDL_GetTicks();
        i32 ms_this_frame = ms_after - ms_before;
#if CAP_FRAMERATE
        if(ms_this_frame < TARGET_MS_PER_FRAME) {
            i32 ms_to_wait = TARGET_MS_PER_FRAME - ms_this_frame;
            SDL_Delay(ms_to_wait);
            ms_this_frame += ms_to_wait;
        }
#endif 
        //printf("%d ms this frame\n", ms_this_frame);
        last_delta = ms_this_frame / 1000.0f;
    }

    free(display.pixel_buffer);
    SDL_DestroyTexture(display.texture);
    SDL_DestroyRenderer(display.renderer);
    SDL_DestroyWindow(display.window);
    display = {0};
    SDL_Quit();

    return 0;
}
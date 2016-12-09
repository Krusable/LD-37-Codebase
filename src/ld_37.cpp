
/*
    TODO For LD:
    - Load pngs
    - Render textures
    - Render sub textures
    - load map files
    - load audio files
    - play sound effects & music 
*/

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

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

typedef struct {
    i32 width;
    i32 height;
    void* pixels;
} Texture;

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
#define TRANSPARENT_COLOUR  0x00FF00FF

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

void RenderTexture(Display* display, Vec2* pos, Texture* texture) {
    i32 x_min = pos->x * display->pixels_per_meter;
    i32 x_max = x_min + texture->width;
    i32 y_min = pos->y * display->pixels_per_meter;
    i32 y_max = y_min + texture->width;

    // Don't bother rendering if the texture is completly off the screen.
    if(x_max < 0 || x_min >= display->width || x_max < 0 || x_min >= display->width) {
        return;
    }

    for(i32 y = y_min, ty = 0; y < y_max; y++, ty++) {
        for(i32 x = x_min, tx = 0; x < x_max; x++, tx++) {
            if(x >= 0 && x < display->width && y >= 0 && y < display->height) {
                u32* display_pixel = (u32*)display->pixel_buffer + (y * display->width) + x;
                u32* texture_pixel = (u32*)texture->pixels + (ty * texture->width) + tx;
                if(*texture_pixel != TRANSPARENT_COLOUR) {
                    *display_pixel = *texture_pixel;
                }
            }
        }
    }
}

void RenderSubTexture(Display* display, Vec2* pos, Texture* texture, i32 sub_texture_x, i32 sub_texture_y, i32 sub_texture_w, i32 sub_texture_h) {
    i32 x_min = pos->x * display->pixels_per_meter;
    i32 x_max = x_min + sub_texture_w;
    i32 y_min = pos->y * display->pixels_per_meter;
    i32 y_max = y_min + sub_texture_h;

    // Don't bother rendering if the texture is completly off the screen.
    if(x_max < 0 || x_min >= display->width || x_max < 0 || x_min >= display->width) {
        return;
    }

    for(i32 y = y_min, ty = sub_texture_y; y < y_max; y++, ty++) {
        for(i32 x = x_min, tx = sub_texture_x; x < x_max; x++, tx++) {
            if(x >= 0 && x < display->width && y >= 0 && y < display->height) {
                u32* display_pixel = (u32*)display->pixel_buffer + (y * display->width) + x;
                u32* texture_pixel = (u32*)texture->pixels + (ty * texture->width) + tx;
                if(*texture_pixel != TRANSPARENT_COLOUR) {
                    *display_pixel = *texture_pixel;
                }
            }
        }
    }
}

void LoadMeSomePNG(char* file_path, Texture* dst_texture, SDL_PixelFormat* pixel_format) {
    SDL_assert(dst_texture->pixels);

    SDL_Surface* loaded_texture = NULL;

    loaded_texture = IMG_Load(file_path);
    if(!loaded_texture) {
        printf("Warning - could not load image '%s'.\n", file_path);
        return;
    }

    SDL_Surface* corrected_texture = SDL_ConvertSurface(loaded_texture, pixel_format, 0);
    if(!corrected_texture) {
        printf("Warning - could not convert image. %s.\n", SDL_GetError());
        return;
    }

    dst_texture->width = corrected_texture->w;
    dst_texture->height = corrected_texture->h;
    i32 tex_size = dst_texture->width * dst_texture->height;
    dst_texture->pixels = malloc(tex_size * pixel_format->BytesPerPixel);
    for(i32 i = 0; i < tex_size; i++) {
        ((u32*)dst_texture->pixels)[i] = ((u32*)corrected_texture->pixels)[i];
    }

    SDL_FreeSurface(loaded_texture);
    SDL_FreeSurface(corrected_texture);
}

void FreeTexture(Texture* texture) {
    free(texture->pixels);
    texture->pixels = 0;
}

i32 main(i32 argc, char** argv) {
    srand(time(0));
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0) {
        printf("Error - Could not init SDL. SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    int flags = IMG_INIT_PNG;
    if(IMG_Init(flags) & flags != flags) {
        printf("Error - Could not init SDL_Image. IMG_Error: %s\n", IMG_GetError());
        return -1;
    } \

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
    Vec2 player_pos = {300 / display.pixels_per_meter, 300 / display.pixels_per_meter};
    Vec2 player_size = {100 / display.pixels_per_meter, 100 / display.pixels_per_meter};
    Vec4 player_colour = {1.0f, 0.2f, 0.2f, 0.2f};
    f32 speed = 2 * ((f32)display.pixels_per_meter / (1000.0 / (f32)TARGET_MS_PER_FRAME));
    f32 last_delta = 0.0f;

    const i32 MAP_WIDTH = 17;
    const i32 MAP_HEIGHT = 10;
    const f32 TILE_SIZE = 64.0 / (f32)display.pixels_per_meter;
    u32 tile_map[MAP_WIDTH * MAP_HEIGHT];

    Vec2 font_rect_pos = {10.0 / (f32)display.pixels_per_meter, 10.0 / (f32)display.pixels_per_meter};
    Vec2 font_rect_size = {10, 4};
    Vec4 font_rect_colour = {1.0f, 0.8f, 0.8f, 0.8f};

    Vec2 texture_pos = {1, 1};
    Texture font_texture = {0};
    LoadMeSomePNG("../res/textures/font_texture.png", &font_texture, display.pixel_format);

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
            player_pos.y -= speed * last_delta;
        }
        else if(key_states[SDL_SCANCODE_DOWN] && !key_states[SDL_SCANCODE_UP]) {
            player_pos.y += speed * last_delta;
        }

        if(key_states[SDL_SCANCODE_LEFT] && !key_states[SDL_SCANCODE_RIGHT]) {
            player_pos.x -= speed * last_delta;
        }
        else if(key_states[SDL_SCANCODE_RIGHT] && !key_states[SDL_SCANCODE_LEFT]) {
            player_pos.x += speed * last_delta;
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

        RenderFilledRect(&display, &font_rect_pos, &font_rect_size, &font_rect_colour);
        RenderFilledRect(&display, &player_pos, &player_size, &player_colour);
        //RenderTexture(&display, &texture_pos, &font_texture);
        RenderSubTexture(&display, &texture_pos, &font_texture, 32, 32, 32, 32);

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

    FreeTexture(&font_texture);
    free(display.pixel_buffer);
    SDL_DestroyTexture(display.texture);
    SDL_DestroyRenderer(display.renderer);
    SDL_DestroyWindow(display.window);
    display = {0};
    SDL_Quit();

    return 0;
}
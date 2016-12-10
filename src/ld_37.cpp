
/*
    TODO For LD:
    - load audio files
    - play sound effects & music 
*/

#include "ld_37_lib.h"

#define TARGET_MS_PER_FRAME 16
#define CAP_FRAMERATE       1

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
    Vec2 player_pos = {300 / display.pixels_per_meter, 300 / display.pixels_per_meter};
    Vec2 player_size = {100 / display.pixels_per_meter, 100 / display.pixels_per_meter};
    u32 player_colour = 0xFF111111;
    f32 speed = 2 * ((f32)display.pixels_per_meter / (1000.0 / (f32)TARGET_MS_PER_FRAME));
    f32 last_delta = 0.0f;

    const i32 MAP_WIDTH = 17;
    const i32 MAP_HEIGHT = 10;
    const f32 TILE_SIZE = 64.0 / (f32)display.pixels_per_meter;
    u32 tile_map[MAP_WIDTH * MAP_HEIGHT];

    Vec2 font_rect_pos = {10.0 / (f32)display.pixels_per_meter, 10.0 / (f32)display.pixels_per_meter};
    Vec2 font_rect_size = {10, 4};
    u32 font_rect_colour = 0xFFAAAAAA;

    Vec2 test_string_pos = {1.1, 1.1};

    Font test_font = {0};
    LoadFont(&test_font, display.pixel_format);

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

        RenderFilledRect(&display, &font_rect_pos, &font_rect_size, font_rect_colour);
        RenderString(&display, &test_font, &test_string_pos, "TEST 123 ABC", 0xFFE7E7E7);
        RenderFilledRect(&display, &player_pos, &player_size, player_colour);

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

    FreeFont(&test_font);
    free(display.pixel_buffer);
    SDL_DestroyTexture(display.texture);
    SDL_DestroyRenderer(display.renderer);
    SDL_DestroyWindow(display.window);
    display = {0};
    SDL_Quit();

    return 0;
}

#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdint.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

i32 main(i32 argc, char** argv) {
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Error - Could not init SDL. SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    return 0;
}
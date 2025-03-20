#include "addshader.h"
#include "rpi.h"
#include <stdint.h>

#define N 1024 // must be a multiple of 64

struct addGPU
{
    uint32_t A[N];
    uint32_t B[N];
    uint32_t C[N];
    unsigned code[sizeof(addshader) / sizeof(uint32_t)];
    unsigned unif[4];
    unsigned mail[2];
    unsigned handle;
};

void vec_add_init(volatile struct addGPU **gpu);

int vec_add_exec(volatile struct addGPU *gpu);

void vec_add_release(volatile struct addGPU *gpu);
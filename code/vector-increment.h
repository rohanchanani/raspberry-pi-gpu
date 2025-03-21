#include "incrementshader.h"
#include "rpi.h"
#include <stdint.h>

#define MAX_INC 64 // must be a multiple of 64

struct incrementGPU
{
    uint32_t A[MAX_INC];
    uint32_t B[MAX_INC];
    unsigned code[sizeof(incrementshader) / sizeof(uint32_t)];
    unsigned unif[4];
    unsigned mail[2];
    unsigned handle;
};

void vec_increment_init(volatile struct incrementGPU **gpu);

// A += B
int vec_increment_exec(volatile struct incrementGPU *gpu);

void vec_increment_release(volatile struct incrementGPU *gpu);
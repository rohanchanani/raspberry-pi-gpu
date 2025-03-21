#include "mulshader.h"
#include "rpi.h"
#include <stdint.h>
#include "max.h"

struct mulGPU
{
    uint32_t A[N];
    uint32_t B[N];
    uint32_t C[N];
    unsigned code[sizeof(mulshader) / sizeof(uint32_t)];
    unsigned unif[4];
    unsigned mail[2];
    unsigned handle;
};

void vec_mul_init(volatile struct mulGPU **gpu, int n);

int vec_mul_exec(volatile struct mulGPU * gpu);

void vec_mul_release(volatile struct mulGPU * gpu);
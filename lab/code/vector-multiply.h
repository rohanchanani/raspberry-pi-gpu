#include "mulshader.h"
#include "rpi.h"
#include <stdint.h>
#include "max.h"

struct mulGPU
{
    uint32_t A[N];
    uint32_t B[N];
    uint32_t C[N];
    uint32_t code[sizeof(mulshader) / sizeof(uint32_t)];
    uint32_t unif[NUM_QPUS][5];
    uint32_t unif_ptr[NUM_QPUS];
    uint32_t mail[2];
    uint32_t handle;
};

void vec_mul_init(volatile struct mulGPU **gpu, int n);

int vec_mul_exec(volatile struct mulGPU * gpu);

void vec_mul_release(volatile struct mulGPU * gpu);
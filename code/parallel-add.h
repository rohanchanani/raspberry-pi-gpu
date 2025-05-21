#include "addshader.h"
#include "rpi.h"
#include <stdint.h>


#define N 1048576
#define NUM_QPUS 8



struct addGPU
{
	uint32_t A[N];
	uint32_t B[N];
	uint32_t C[N];
	uint32_t code[sizeof(addshader) / sizeof(uint32_t)];
	uint32_t unif[NUM_QPUS][5];
	uint32_t unif_ptr[NUM_QPUS];
	uint32_t mail[2];
	uint32_t handle;
};

void vec_add_init(volatile struct addGPU **gpu, int n);

int vec_add_exec(volatile struct addGPU *gpu);

void vec_add_release(volatile struct addGPU *gpu);
#include "rpi.h"
#include <stddef.h>
#include <string.h>
// #include "vector-multiply.h"
#include "mailbox.h"
#include "mulshader.h"

#define N 256

#define GPU_MEM_FLG 0xC // cached=0xC; direct=0x4

#define NUM_UNIF 4
#define NUM_QPUS 4

struct GPU
{
	uint32_t A[N];
	uint32_t B[N];
	uint32_t C[N];
	unsigned code[sizeof(mulshader) / sizeof(uint32_t)];
	unsigned unif[6];
	unsigned mail[2];
	unsigned handle;
};

int gpu_prepare(
	volatile struct GPU **gpu)
{
	unsigned handle, vc;
	volatile struct GPU *ptr;

	if (qpu_enable(1))
		return -2;

	handle = mem_alloc(sizeof(struct GPU), 4096, GPU_MEM_FLG);
	if (!handle)
	{
		qpu_enable(0);
		return -3;
	}
	vc = mem_lock(handle);

	ptr = (volatile struct GPU *)(vc - 0x40000000);
	if (ptr == NULL)
	{
		mem_free(handle);
		mem_unlock(handle);
		qpu_enable(0);
		return -4;
	}

	ptr->handle = handle;
	ptr->mail[0] = vc + offsetof(struct GPU, code);
	ptr->mail[1] = vc + offsetof(struct GPU, unif);

	*gpu = ptr;
	return 0;
}

unsigned gpu_execute(volatile struct GPU *gpu)
{
	qpu_enable(1);
	return gpu_fft_base_exec_direct(
		(uint32_t)gpu->mail[0],
		(uint32_t)gpu->mail[1],
		NUM_QPUS
	);
}

void gpu_release(volatile struct GPU *gpu)
{
	unsigned handle = gpu->handle;
	mem_unlock(handle);
	mem_free(handle);
	qpu_enable(0);
}

uint32_t vector_multiply(uint32_t A[], uint32_t B[], uint32_t C[]) {
	volatile struct GPU *gpu;
	int ret = gpu_prepare(&gpu);
	if (ret < 0)
		return -1;

	memcpy((void *)gpu->A, A, N * sizeof(uint32_t));
	memcpy((void *)gpu->B, B, N * sizeof(uint32_t));
	memcpy((void *)gpu->C, C, N * sizeof(uint32_t));

	memcpy((void *)gpu->code, mulshader, sizeof gpu->code);

	gpu->unif[0] = N / (16 * NUM_QPUS);
	gpu->unif[1] = gpu->mail[0] - offsetof(struct GPU, code) + offsetof(struct GPU, A);
	gpu->unif[2] = gpu->mail[0] - offsetof(struct GPU, code) + offsetof(struct GPU, B);
	gpu->unif[3] = gpu->mail[0] - offsetof(struct GPU, code) + offsetof(struct GPU, C);
	gpu->unif[4] = N / NUM_QPUS;
	gpu->unif[5] = 69;

	int iret = gpu_execute(gpu);

	memcpy(C, (void *)gpu->C, N * sizeof(uint32_t));

	gpu_release(gpu);

	return iret;
}

#if 1
void notmain(void)
{
	int i, j;

	uint32_t A[N];
	uint32_t B[N];
	uint32_t C[N];

	for (i = 0; i < N; i++) {
		A[i] = 0;
		B[i] = 64+i;
		C[i] = 0x69;
	}

	printk("Size of mulshader: %d\n", sizeof(mulshader));

	
	printk("Running code on GPU...\n");
	printk("Memory before running code: %x %x %x %x\n", C[0], C[1], C[2], C[3]);

	int start_time = timer_get_usec();
	int iret = vector_multiply(A, B, C);
	int end_time = timer_get_usec();

	int gpu_time = end_time - start_time;

	printk("Memory after running code:  %d %d %d %d\n", C[0], C[1], C[2], C[3]);

	for (i = 0; i < N; i++) {
	    if(C[i] != (32+i)*(64+i)) {
	        printk("Iteration %d: %d * %d = %d. Answer is INCORRECT\n", i, A[i], B[i], C[i]);
	    } else if (i % 64 == 0) {
	        printk("Iteration %d: %d * %d = %d. Answer is CORRECT\n", i, A[i], B[i], C[i]);
	    }
	}

	int cpu_time = 0;

	start_time = timer_get_usec();
	for (i = 0; i < N; i++) {
		C[i] = A[i] * B[i];
	}
	end_time = timer_get_usec();
	cpu_time = end_time - start_time;

	printk("Time taken on CPU: %d us\n", cpu_time);
	printk("Time taken on GPU: %d us\n", gpu_time);
}

#endif

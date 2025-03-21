#include "rpi.h"
#include <stddef.h>
#include <string.h>
#include "vector-increment.h"
#include "mailbox.h"
#include "incrementshader.h"

#define GPU_MEM_FLG 0xC // cached=0xC; direct=0x4

int increment_gpu_prepare(
	volatile struct incrementGPU **gpu)
{
	unsigned handle, vc;
	volatile struct incrementGPU *ptr;

	if (qpu_enable(1))
		return -2;

	handle = mem_alloc(sizeof(struct incrementGPU), 4096, GPU_MEM_FLG);
	if (!handle)
	{
		qpu_enable(0);
		return -3;
	}
	vc = mem_lock(handle);

	ptr = (volatile struct incrementGPU *)(vc - 0x40000000);
	if (ptr == NULL)
	{
		mem_free(handle);
		mem_unlock(handle);
		qpu_enable(0);
		return -4;
	}

	ptr->handle = handle;
	ptr->mail[0] = vc + offsetof(struct incrementGPU, code);
	ptr->mail[1] = vc + offsetof(struct incrementGPU, unif);

	*gpu = ptr;
	return 0;
}

unsigned increment_gpu_execute(volatile struct incrementGPU *gpu)
{
	return gpu_fft_base_exec_direct(
		(uint32_t)gpu->mail[0],
		(uint32_t)gpu->mail[1],
		4);
}

void vec_increment_release(volatile struct incrementGPU *gpu)
{
	unsigned handle = gpu->handle;
	mem_unlock(handle);
	mem_free(handle);
	qpu_enable(0);
}

void vec_increment_init(volatile struct incrementGPU **gpu)
{
	
	int ret = increment_gpu_prepare(gpu);
	if (ret < 0)
		return;

	volatile struct incrementGPU *ptr = *gpu;
	memcpy((void *)ptr->code, incrementshader, sizeof ptr->code);

	ptr->unif[0] = MAX_INC / 64; // gpu->mail[0] - offsetof(struct addGPU, code);
	ptr->unif[1] = ptr->mail[0] - offsetof(struct incrementGPU, code) + offsetof(struct incrementGPU, A);
	ptr->unif[2] = ptr->mail[0] - offsetof(struct incrementGPU, code) + offsetof(struct incrementGPU, B);
}

int vec_increment_exec(volatile struct incrementGPU *gpu)
{
	int start_time = timer_get_usec();
	int iret = increment_gpu_execute(gpu);
	int end_time = timer_get_usec();

	return end_time - start_time;
}

#if 0
void notmain(void)
{
	int i, j;
	volatile struct addGPU *gpu;

	vec_add_init(&gpu);

	for (i = 0; i < MAX_INC; i++)
	{
		gpu->A[i] = 32 + i;
		gpu->B[i] = 64 + i;
		gpu->C[i] = 0;
	}

	printk("Running code on GPU...\n");
	printk("Memory before running code: %d %d %d %d\n", gpu->C[0], gpu->C[1], gpu->C[2], gpu->C[3]);

	int start_time = timer_get_usec();
	int iret = vec_add_exec(gpu);
	int end_time = timer_get_usec();

	int gpu_time = end_time - start_time;

	printk("Memory after running code:  %d %d %d %d\n", gpu->C[0], gpu->C[1], gpu->C[2], gpu->C[3]);

	for (i = 0; i < N; i++)
	{
		if (gpu->C[i] != (32 + i) + (64 + i))
		{
			printk("Iteration %d: %d + %d = %d. Answer is INCORRECT\n", i, gpu->A[i], gpu->B[i], gpu->C[i]);
		}
		else if (i % 64 == 0)
		{
			printk("Iteration %d: %d + %d = %d. Answer is CORRECT\n", i, gpu->A[i], gpu->B[i], gpu->C[i]);
		}
	}

	int cpu_time = 0;

	start_time = timer_get_usec();
	for (i = 0; i < N; i++)
	{
		gpu->C[i] = gpu->A[i] + gpu->B[i];
	}
	end_time = timer_get_usec();
	cpu_time = end_time - start_time;

	printk("Time taken on CPU: %d us\n", cpu_time);
	printk("Time taken on GPU: %d us\n", gpu_time);

	vec_add_release(gpu);
}

#endif

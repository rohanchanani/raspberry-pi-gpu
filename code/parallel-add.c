#include "rpi.h"
#include <stddef.h>
#include <string.h>
#include "parallel-add.h"
#include "mailbox.h"
#include "addshader.h"

#define GPU_MEM_FLG 0xC // cached=0xC; direct=0x4

int add_gpu_prepare(
	volatile struct addGPU **gpu)
{
	unsigned handle, vc;
	volatile struct addGPU *ptr;

	if (qpu_enable(1))
		return -2;

	handle = mem_alloc(sizeof(struct addGPU), 4096, GPU_MEM_FLG);
	if (!handle)
	{
		qpu_enable(0);
		return -3;
	}
	vc = mem_lock(handle);

	ptr = (volatile struct addGPU *)(vc - 0x40000000);
	if (ptr == NULL)
	{
		mem_free(handle);
		mem_unlock(handle);
		qpu_enable(0);
		return -4;
	}

	qpu_enable(1);
	ptr->handle = handle;
	ptr->mail[0] = vc + offsetof(struct addGPU, code);
	ptr->mail[1] = vc + offsetof(struct addGPU, unif);

	*gpu = ptr;
	return 0;
}

unsigned add_gpu_execute(volatile struct addGPU *gpu)
{
	return gpu_fft_base_exec_direct(
		(uint32_t)gpu->mail[0],
		(uint32_t)gpu->mail[1],
		4);
}

void vec_add_release(volatile struct addGPU *gpu)
{
	unsigned handle = gpu->handle;
	mem_unlock(handle);
	mem_free(handle);
	qpu_enable(0);
}

void vec_add_init(volatile struct addGPU **gpu)
{
	int ret = add_gpu_prepare(gpu);
	if (ret < 0)
		return;

	volatile struct addGPU *ptr = *gpu;
	memcpy((void *)ptr->code, addshader, sizeof ptr->code);

	ptr->unif[0] = N / 64; // gpu->mail[0] - offsetof(struct addGPU, code);
	ptr->unif[1] = ptr->mail[0] - offsetof(struct addGPU, code) + offsetof(struct addGPU, A);
	ptr->unif[2] = ptr->mail[0] - offsetof(struct addGPU, code) + offsetof(struct addGPU, B);
	ptr->unif[3] = ptr->mail[0] - offsetof(struct addGPU, code) + offsetof(struct addGPU, C);
}

int vec_add_exec(volatile struct addGPU *gpu)
{
	int start_time = timer_get_usec();
	int iret = add_gpu_execute(gpu);
	int end_time = timer_get_usec();

	return end_time - start_time;
}

#if 0
void notmain(void)
{
	int i, j;
	volatile struct addGPU *gpu;

	vec_add_init(&gpu);

	for (i = 0; i < N; i++)
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

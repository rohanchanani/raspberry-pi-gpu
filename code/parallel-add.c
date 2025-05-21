#include "rpi.h"
#include <stddef.h>
#include <string.h>
#include "parallel-add.h"
#include "mailbox.h"
#include "addshader.h"

#define GPU_MEM_FLG 0xC // cached=0xC; direct=0x4
#define GPU_BASE 0x40000000

int add_gpu_prepare(
	volatile struct addGPU **gpu)
{
	uint32_t handle, vc;
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

	ptr->handle = handle;
	ptr->mail[0] = GPU_BASE + (uint32_t)&ptr->code;
	ptr->mail[1] = GPU_BASE + (uint32_t)&ptr->unif;

	*gpu = ptr;
	return 0;
}

uint32_t add_gpu_execute(volatile struct addGPU *gpu)
{
	return gpu_fft_base_exec_direct(
		(uint32_t)gpu->mail[0],
		(uint32_t *)gpu->unif_ptr,
		NUM_QPUS
	);
}

void vec_add_release(volatile struct addGPU *gpu)
{
	uint32_t handle = gpu->handle;
	mem_unlock(handle);
	mem_free(handle);
	qpu_enable(0);
}

void vec_add_init(volatile struct addGPU **gpu, int n)
{
	int ret = add_gpu_prepare(gpu);
	if (ret < 0)
		return;

	volatile struct addGPU *ptr = *gpu;
	memcpy((void *)ptr->code, addshader, sizeof ptr->code);
	for (int i=0; i<NUM_QPUS; i++) {
	    ptr->unif[i][0] = N / (16*NUM_QPUS); // gpu->mail[0] - offsetof(struct GPU, code);
	    ptr->unif[i][1] = GPU_BASE + (uint32_t)&ptr->A + i*N*4 / NUM_QPUS;
	    ptr->unif[i][2] = GPU_BASE + (uint32_t)&ptr->B + i*N*4 / NUM_QPUS;
	    ptr->unif[i][3] = GPU_BASE + (uint32_t)&ptr->C + i*N*4 / NUM_QPUS;
	    ptr->unif[i][4] = i;
	    ptr->unif_ptr[i] = GPU_BASE + (uint32_t)&ptr->unif[i];
	}
}

int vec_add_exec(volatile struct addGPU *gpu)
{
	int start_time = timer_get_usec();
	int iret = add_gpu_execute(gpu);
	int end_time = timer_get_usec();

	return end_time - start_time;
}


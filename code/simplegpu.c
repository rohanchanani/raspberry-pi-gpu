#include "rpi.h"
#include <stddef.h>
#include <string.h>

#include "mailbox.h"
#include "shader.h"

#define GPU_MEM_FLG 0xC // cached=0xC; direct=0x4


struct GPU
{
	uint32_t output[256];
	unsigned code[sizeof(shader) / sizeof(uint32_t)];
	unsigned unif[1];
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
	return gpu_fft_base_exec_direct(
		(uint32_t)gpu->mail[0],
		(uint32_t)gpu->mail[1],
		4
	);
}

void gpu_release(volatile struct GPU *gpu)
{
	unsigned handle = gpu->handle;
	mem_unlock(handle);
	mem_free(handle);
	qpu_enable(0);
}

void notmain(void)
{
	int i, j;
	volatile struct GPU *gpu;
	int ret = gpu_prepare(&gpu);
	if (ret < 0)
		return;

	memcpy((void *)gpu->code, shader, sizeof gpu->code);

	gpu->unif[0] = gpu->mail[0] - offsetof(struct GPU, code);
	memset((void *)gpu->output, 0xff, sizeof gpu->output);

	printk("Memory before running code: %x %x %x %x\n", gpu->output[0], gpu->output[1], gpu->output[2], gpu->output[3]);
	trace("Executing process... result: %d\n", gpu_execute(gpu));
	printk("Memory after running code:  %x %x %x %x\n", gpu->output[0], gpu->output[1], gpu->output[2], gpu->output[3]);

	gpu_release(gpu);
}

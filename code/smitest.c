#include "rpi.h"
#include <stddef.h>
#include <string.h>

#include "mailbox.h"
#include "shader.h"

#define VEC_COUNT 3 * 16
#define GPU_MEM_FLG 0xC // cached=0xC; direct=0x4

struct GPU
{
	uint32_t output[1024 / 4];
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
	// printk("handle=%x\n", handle);
	if (!handle)
	{
		qpu_enable(0);
		return -3;
	}
	vc = mem_lock(handle);
	trace("vc=%x\n", vc);
	ptr = (volatile struct GPU *)(vc - 0x40000000);
	trace("ptr=%p\n", ptr);
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
	trace("Should be identical: %x, %x\n", ptr->mail[1] - ptr->mail[0], offsetof(struct GPU, unif));

	*gpu = ptr;
	return 0;
}

unsigned gpu_execute(volatile struct GPU *gpu)
{
	// printk("msg=%x\n", gpu->mail[1] + offsetof(struct GPU, mail));
	//  return execute_qpu(
	//  	1 /* 1 QPU */,
	//  	gpu->mail[0] + offsetof(struct GPU, mail),
	//  	0 /* no flush */,
	//  	1000 /* timeout */);

	return gpu_fft_base_exec_direct(
		(uint32_t)gpu->mail[0],
		(uint32_t)gpu->mail[1],
		2 /* 1 QPU */
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

	printk("Before: %x %x %x %x\n", gpu->output[0], gpu->output[1], gpu->output[2], gpu->output[3]);

	trace("Exec: %x\n", gpu_execute(gpu));
	printk("After: %x %x %x %x\n", gpu->output[0], gpu->output[1], gpu->output[2], gpu->output[3]);

	printk("DONE\n");

	gpu_release(gpu);
}

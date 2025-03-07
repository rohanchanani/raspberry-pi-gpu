#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "mailbox.h"
#include "hello.h"
#include "rpi.h"

#define VEC_COUNT 3*16
#define GPU_MEM_FLG 0xC // cached=0xC; direct=0x4


struct GPU
{
	unsigned code[sizeof(output) / sizeof(uint32_t)];
	unsigned A[64];
	unsigned B[64];
	unsigned C[64];
	unsigned unif[4];
	unsigned mail[2];
	unsigned handle;
};

struct GPU_old
{
        unsigned code[sizeof(output) / sizeof(uint32_t)];
        unsigned A;
        unsigned unif[1];
        unsigned mail[2];
        unsigned handle;
};

int gpu_prepare(
	volatile struct GPU **gpu)
{
	unsigned handle, vc;
	volatile struct GPU* ptr;

	if (qpu_enable(1)) {
		trace("Couldn't enable\n");
		return -1;
	}

	handle = mem_alloc(sizeof(struct GPU), 4096, GPU_MEM_FLG);
	trace("handle=%x\n", handle);
	if (!handle)
	{	qpu_enable(0);
		return -3;
	}
	vc = mem_lock(handle);
	trace("vc=%x\n", vc);
	ptr = (volatile struct GPU*) (vc - 0x40000000);
	//trace("ptr=%p\n", ptr);
	if (ptr == NULL)
	{	mem_free(handle);
		mem_unlock(handle);
		qpu_enable(0);
		return -4;
	}

	ptr->handle = handle;
	ptr->mail[0] = vc + offsetof(struct GPU, unif);
	ptr->mail[1] = vc;

	*gpu = ptr;
	return 0;
}

unsigned gpu_execute(volatile struct GPU *gpu)
{
	//trace("msg=%x\n", gpu->mail[1] + offsetof(struct GPU, mail));
	return execute_qpu(
		1 /* 1 QPU */,
		gpu->mail[1] + offsetof(struct GPU, mail),
		1 /* no flush */,
		1000 /* timeout */);
}

void gpu_release(volatile struct GPU *gpu)
{
	unsigned handle = gpu->handle;
	mem_unlock(handle);
	mem_free(handle);
	qpu_enable(0);
}

/*
void old_notmain() {
	volatile struct GPU* gpu;
        int ret = gpu_prepare(&gpu);
        if (ret < 0)
                return;

        memcpy((void*)gpu->code, output, sizeof gpu->code);
        gpu->unif[0] = gpu->mail[1] + offsetof(struct GPU, A);
		
}
*/

void notmain()
{
	unsigned A[64];
	unsigned B[64];
	unsigned C[64];
	for (int i=0; i<64; i++) {
	    A[i] = i;
	    B[i] = i;
	    C[i] = i;
	}
	volatile struct GPU* gpu;
	int ret = gpu_prepare(&gpu);
	if (ret < 0)
		return;

	memcpy((void*)gpu->code, output, sizeof gpu->code);

	gpu->unif[0] = 4;
	gpu->unif[1] = gpu->mail[1] + offsetof(struct GPU, A);
	gpu->unif[2] = gpu->mail[1] + offsetof(struct GPU, B);
	gpu->unif[3] = gpu->mail[1] + offsetof(struct GPU, C);
	
	memcpy((void*)gpu->A, A, sizeof gpu->A);
	memcpy((void*)gpu->B, B, sizeof gpu->B);
	
	trace("about to execute\n");

	trace("Exec: %x\n", gpu_execute(gpu));

	memcpy(C, (void*)gpu->C, sizeof gpu->C);

	for (int i=0; i<64; i++) {
	    trace("%d\n", C[i]);
	}
	
	gpu_release(gpu);
}

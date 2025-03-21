#include "rpi.h"
#include <stddef.h>
#include <string.h>
#include "fat32/code/pi-sd.h"
#include "fat32/code/fat32.h"
#include "mailbox.h"
#include "mandelbrotshader.h"

#define GPU_MEM_FLG 0xC // cached=0xC; direct=0x4

#define RESOLUTION 64
#define MAX_ITERS 1
#define NUM_QPUS 1

struct GPU
{
	uint32_t output[2*RESOLUTION][2*RESOLUTION];
	uint32_t code[sizeof(mandelbrotshader) / sizeof(uint32_t)];
	uint32_t unif[NUM_QPUS][6];
	uint32_t unif_ptr[NUM_QPUS];
	uint32_t mail[2];
	uint32_t handle;
};

int gpu_prepare(
	volatile struct GPU **gpu)
{
	uint32_t handle, vc;
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

uint32_t gpu_execute(volatile struct GPU *gpu)
{
	return gpu_fft_base_exec_direct(
		(uint32_t)gpu->mail[0],
		(uint32_t *)gpu->unif_ptr,
		NUM_QPUS
	);
}

void gpu_release(volatile struct GPU *gpu)
{
	uint32_t handle = gpu->handle;
	mem_unlock(handle);
	mem_free(handle);
	qpu_enable(0);
}

// A small helper function to convert a positive integer to decimal ASCII.
// Returns the number of characters written into 'buf'. No sign handling.
static int int_to_ascii(int val, char *buf)
{
    // We'll build digits in reverse order in 'temp', then copy them to 'buf'.
    char temp[16];
    int idx = 0;

    // Handle zero explicitly
    if (val == 0) {
        buf[0] = '0';
        return 1;
    }

    // Extract digits in reverse
    while (val > 0) {
        int digit = val % 10;
        val /= 10;
        temp[idx++] = (char)('0' + digit);
    }

    // Now reverse them into buf
    int written = 0;
    while (idx > 0) {
        buf[written++] = temp[--idx];
    }

    return written;
}



// Build a small 16x16 PGM image in one buffer.
// Returns pointer to the data and sets *outSize to total bytes.
char *buildPGM(int *outSize, int HEIGHT, int WIDTH, const unsigned char input_image[HEIGHT][WIDTH])
{
    int pos = 0;
    char *g_pgmData = kmalloc(32 + HEIGHT*WIDTH);
    // -- 1) Write the PGM "magic number": "P5\n"
    g_pgmData[pos++] = 'P';
    g_pgmData[pos++] = '5';
    g_pgmData[pos++] = '\n';

    // -- 2) Write "WIDTH HEIGHT\n"
    pos += int_to_ascii(WIDTH, &g_pgmData[pos]);
    g_pgmData[pos++] = ' ';
    pos += int_to_ascii(HEIGHT, &g_pgmData[pos]);
    g_pgmData[pos++] = '\n';

    // -- 3) Write "255\n" as the max pixel value
    g_pgmData[pos++] = '2';
    g_pgmData[pos++] = '5';
    g_pgmData[pos++] = '5';
    g_pgmData[pos++] = '\n';

    // -- 4) Fill the raw 16x16 pixel data.
    //    Each pixel is a single byte in [0..255].
    //    We'll do a small gradient for demonstration.
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            // Simple pattern: add x+y, just to have something visually distinct
            g_pgmData[pos++] = input_image[y][x];
        }
    }

    // total bytes in the g_pgmData
    *outSize = pos;

    // Return pointer to the buffer
    return g_pgmData;
}


void notmain(void)
{
	volatile struct GPU *gpu;
	int ret = gpu_prepare(&gpu);
	if (ret < 0)
		return;

	float res = (float) RESOLUTION;
	union {
		float f;
		uint32_t i;
	} pun;
	pun.f = res;
	pun.i = 0x7EF127EA - pun.i;
	float r = pun.f;
	r = r*(2.0f - res*r);
	r = r*(2.0f - res*r);
	r = r*(2.0f - res*r);
	pun.f = r;
	memcpy((void *)gpu->code, mandelbrotshader, sizeof gpu->code);
	for (int i=0; i<NUM_QPUS; i++) {
	    gpu->unif[i][0] = RESOLUTION;
	    gpu->unif[i][1] = pun.i;
	    gpu->unif[i][2] = MAX_ITERS;
	    gpu->unif[i][3] = NUM_QPUS;
	    gpu->unif[i][4] = i;
	    gpu->unif[i][5] = gpu->mail[0] - offsetof(struct GPU, code) + offsetof(struct GPU, output);
	    gpu->unif_ptr[i] = gpu->mail[0] - offsetof(struct GPU, code) + (uint32_t) &gpu->unif[i][0] - (uint32_t) gpu;
	    //printk("UNIFORM %d: NUM_ITERS: %d A: %x, B: %x, C: %x ADDRESS: %x\n", i, gpu->unif[i][0], gpu->unif[i][1], gpu->unif[i][2], gpu->unif[i][3], gpu->unif_ptr[i]);
	}
	printk("Running code on GPU...\n");
	//printk("Memory before running code: %x %x %x %x\n", gpu->C[0], gpu->C[1], gpu->C[2], gpu->C[3]);

	int start_time = timer_get_usec();
	int iret = gpu_execute(gpu);
	int end_time = timer_get_usec();
	printk("DONE!\n");
	int gpu_time = end_time - start_time;

	//printk("Memory after running code:  %d %d %d %d\n", gpu->C[0], gpu->C[1], gpu->C[2], gpu->C[3]);
       

	uint32_t output_cmp[2*RESOLUTION][2*RESOLUTION];
	int cpu_time = 0;
	start_time = timer_get_usec();
	for (int i = 0; i < 2*RESOLUTION; i++) {
		float y = -1.0f + (pun.f * (float) i);
		for (int j=0; j<2*RESOLUTION; j++) {
		    float x = -1.0f + (pun.f * (float)  j);
		    //output_x.f = x;
		    //if ( i % 10==0 && j % 10==0) {
		    	//trace("%d, %d: %x, %x\n", i, j, output_y.i, output_x.i);
		    //}
		    float u = 0.0;
		    float v = 0.0;
		    float u2 = u*u;
		    float v2 = v*v;
		    int k;
		    for (k = 1; k<MAX_ITERS && (u2+v2 < 4.0); k++) {
		    	v = 2 * u * v + y;
			u = u2 - v2 + x;
			u2 = u*u;
			v2 = v*v;
		    }
		    if (k >= MAX_ITERS) {
		    	output_cmp[i][j] = 1;
		    } else {
		    	output_cmp[i][j] = 0;
		    }
		}
	}
	end_time = timer_get_usec();
	cpu_time = end_time - start_time;

 
	for (int i = 0; i < 2*RESOLUTION; i++) {
	    for (int j=0; j < 2*RESOLUTION; j++) {
		if (gpu->output[i][j] == 55)  {
		    printk("Iteration %d, %d: x = %x.\n", i, j, gpu->output[i][j]);
		}
	    	//if(gpu->output[i][j] != output_cmp[i][j]) {
	        	//printk("Iteration %d, %d: %d != %d. Answer is INCORRECT\n", i, j, gpu->output[i][j], output_cmp[i][j]);
	    	//} else if (i % RESOLUTION/4 == 0 && j % RESOLUTION/4 == 0) {
	        	//printk("Iteration %d, %d: %d = %d. Answer is CORRECT\n", i, j, gpu->output[i][j], output_cmp[i][j]);
	    	//}
	    }
	}
	printk("Time taken on CPU: %d us\n", cpu_time);
	printk("Time taken on GPU: %d us\n", gpu_time);
	
	unsigned char CPU_OUT[2*RESOLUTION][2*RESOLUTION];
	for (int i=0; i<2*RESOLUTION; i++) {
	    for (int j=0; j<2*RESOLUTION; j++) {
	    	if (output_cmp[i][j] > 0) {
		    CPU_OUT[i][j] = 255;
		} else {
		    CPU_OUT[i][j] = 0;
		}
	    }
	}
	printk("%d\n", CPU_OUT[0][0]);
	kmalloc_init(8*FAT32_HEAP_MB);
  	pi_sd_init();

  	printk("Reading the MBR.\n");
	mbr_t *mbr = mbr_read();

  	printk("Loading the first partition.\n");
  	mbr_partition_ent_t partition;
  	memcpy(&partition, mbr->part_tab1, sizeof(mbr_partition_ent_t));
  	assert(mbr_part_is_fat32(partition.part_type));

 	printk("Loading the FAT.\n");
  	fat32_fs_t fs = fat32_mk(&partition);

  	printk("Loading the root directory.\n");
  	pi_dirent_t root = fat32_get_root(&fs);
	/*
	FILE *fp = fopen("output.pgm", "wb");
	fprintf(fp, "P5\n%d %d\n255\n", 2*RESOLUTION, 2*RESOLUTION);
	fwrite(CPU_OUT, sizeof(CPU_OUT), 1, fp);
	fclose(fp);
	*/

	printk("Creating hello.txt\n");
	char *hello_name = "OUTPUT.PGM";
	fat32_delete(&fs, &root, hello_name);
	fat32_create(&fs, &root, hello_name, 0);
  	int size;
	char *data = buildPGM(&size, 2*RESOLUTION, 2*RESOLUTION, CPU_OUT);
  	pi_file_t hello = (pi_file_t) {
    		.data = data,
    		.n_data = size,
    		.n_alloc = size,
  	};

  	fat32_write(&fs, &root, hello_name, &hello);
	gpu_release(gpu);
}

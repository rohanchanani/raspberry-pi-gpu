#include "vector-increment.h"

void test_increment(void)
{
    int i;
    volatile struct incrementGPU *gpu;
    vec_increment_init(&gpu);

    for (i = 0; i < MAX_INC; i++)
    {
        gpu->A[i] = 32 + i;
        gpu->B[i] = 64 + i;
    }

    printk("Testing increment on GPU...\n");
    printk("Memory before increment: %x %x %x %x\n", gpu->A[0], gpu->A[1], gpu->A[2], gpu->A[3]);

    int start_time = timer_get_usec();
    int iret_increment = vec_increment_exec(gpu);
    int end_time = timer_get_usec();
    int gpu_increment_time = end_time - start_time;

    printk("Memory after increment: %x %x %x %x\n", gpu->A[0], gpu->A[1], gpu->A[2], gpu->A[3]);

    for (i = 0; i < MAX_INC; i++)
    {
        if (gpu->A[i] != (32 + i) + (64 + i))
        {
            printk("Increment Iteration %d: %d + %d = %d. INCORRECT\n", i, gpu->A[i], gpu->B[i], gpu->A[i]);
        }
        else if (i % 64 == 0)
        {
            printk("Increment Iteration %d: %d + %d = %d. CORRECT\n", i, gpu->A[i], gpu->B[i], gpu->A[i]);
        }
    }

    // CPU benchmarks
    int cpu_increment_time = 0;

    start_time = timer_get_usec();
    for (i = 0; i < MAX_INC; i++)
        gpu->A[i] = gpu->A[i] + gpu->B[i];
    end_time = timer_get_usec();
    cpu_increment_time = end_time - start_time;

    printk("\nCPU Increment Time: %d us\n", cpu_increment_time);
    printk("GPU Increment Time: %d us\n", gpu_increment_time);

    vec_increment_release(gpu);
}

void notmain(void)
{
    printk("Testing increment on GPU...\n");
    test_increment();
}
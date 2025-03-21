#include "vector-multiply.h"
#include "parallel-add.h"

void test_add_mul(void)
{
    int i, j;
    volatile struct mulGPU *gpu;
    volatile struct addGPU *add_gpu;

    vec_mul_init(&gpu, N);
    vec_add_init(&add_gpu);

    for (i = 0; i < N; i++)
    {
        gpu->A[i] = 32 + i;
        gpu->B[i] = 64 + i;
        gpu->C[i] = 0;
    }

    for (i = 0; i < N; i++)
    {
        add_gpu->A[i] = 32 + i;
        add_gpu->B[i] = 64 + i;
        add_gpu->C[i] = 0;
    }
    // Test multiplication
    printk("Testing multiplication on GPU...\n");
    printk("Memory before multiplication: %x %x %x %x\n", gpu->C[0], gpu->C[1], gpu->C[2], gpu->C[3]);

    int start_time = timer_get_usec();
    int iret_mul = vec_mul_exec(gpu);
    int end_time = timer_get_usec();
    int gpu_mul_time = end_time - start_time;

    printk("Memory after multiplication:  %d %d %d %d\n", gpu->C[0], gpu->C[1], gpu->C[2], gpu->C[3]);

    for (i = 0; i < N; i++)
    {
        if (gpu->C[i] != (32 + i) * (64 + i))
        {
            printk("Mul Iteration %d: %d * %d = %d. INCORRECT\n", i, gpu->A[i], gpu->B[i], gpu->C[i]);
        }
        else if (i % 64 == 0)
        {
            printk("Mul Iteration %d: %d * %d = %d. CORRECT\n", i, gpu->A[i], gpu->B[i], gpu->C[i]);
        }
    }

    // Test addition
    printk("\nTesting addition on GPU...\n");
    printk("Memory before addition: %x %x %x %x\n", add_gpu->C[0], add_gpu->C[1], add_gpu->C[2], add_gpu->C[3]);

    start_time = timer_get_usec();
    int iret_add = vec_add_exec(add_gpu);
    end_time = timer_get_usec();
    int gpu_add_time = end_time - start_time;

    printk("Memory after addition:  %d %d %d %d\n", add_gpu->C[0], add_gpu->C[1], add_gpu->C[2], add_gpu->C[3]);

    for (i = 0; i < N; i++)
    {
        if (add_gpu->C[i] != (32 + i) + (64 + i))
        {
            printk("Add Iteration %d: %d + %d = %d. INCORRECT\n", i, add_gpu->A[i], add_gpu->B[i], add_gpu->C[i]);
        }
        else if (i % 64 == 0)
        {
            printk("Add Iteration %d: %d + %d = %d. CORRECT\n", i, add_gpu->A[i], add_gpu->B[i], add_gpu->C[i]);
        }
    }

    // CPU benchmarks
    int cpu_mul_time = 0;
    int cpu_add_time = 0;

    start_time = timer_get_usec();
    for (i = 0; i < N; i++)
        gpu->C[i] = gpu->A[i] * gpu->B[i];
    end_time = timer_get_usec();
    cpu_mul_time = end_time - start_time;

    start_time = timer_get_usec();
    for (i = 0; i < N; i++)
        add_gpu->C[i] = add_gpu->A[i] + add_gpu->B[i];
    end_time = timer_get_usec();
    cpu_add_time = end_time - start_time;

    printk("\nCPU Multiplication Time: %d us\n", cpu_mul_time);
    printk("GPU Multiplication Time: %d us\n", gpu_mul_time);
    printk("CPU Addition Time: %d us\n", cpu_add_time);
    printk("GPU Addition Time: %d us\n", gpu_add_time);

    vec_mul_release(gpu);
    vec_add_release(add_gpu);
}

void notmain(void)
{
    printk("Testing addition and multiplication on GPU...\n");
    
    test_add_mul();

    delay_ms(6000);
}
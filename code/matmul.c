#include "rpi.h"
#include "vector-multiply.h"
#include "parallel-add.h"
#include "vector-increment.h"

void test_add_mul(void)
{
    int i, j;
    volatile struct mulGPU *gpu;
    volatile struct addGPU *add_gpu;

    vec_mul_init(&gpu);
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

    for (i = 0; i < N; i++) {
        if (gpu->C[i] != (32 + i) * (64 + i)) {
            printk("Mul Iteration %d: %d * %d = %d. INCORRECT\n", i, gpu->A[i], gpu->B[i], gpu->C[i]);
        } else if (i % 64 == 0) {
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

    for (i = 0; i < N; i++) {
        if (add_gpu->C[i] != (32 + i) + (64 + i)) {
            printk("Add Iteration %d: %d + %d = %d. INCORRECT\n", i, add_gpu->A[i], add_gpu->B[i], add_gpu->C[i]);
        } else if (i % 64 == 0) {
            printk("Add Iteration %d: %d + %d = %d. CORRECT\n", i, add_gpu->A[i], add_gpu->B[i], add_gpu->C[i]);
        }
    }

    // CPU benchmarks
    int cpu_mul_time = 0;
    int cpu_add_time = 0;

    start_time = timer_get_usec();
    for (i = 0; i < N; i++) gpu->C[i] = gpu->A[i] * gpu->B[i];
    end_time = timer_get_usec();
    cpu_mul_time = end_time - start_time;

    start_time = timer_get_usec();
    for (i = 0; i < N; i++) add_gpu->C[i] = add_gpu->A[i] + add_gpu->B[i];
    end_time = timer_get_usec();
    cpu_add_time = end_time - start_time;

    printk("\nCPU Multiplication Time: %d us\n", cpu_mul_time);
    printk("GPU Multiplication Time: %d us\n", gpu_mul_time);
    printk("CPU Addition Time: %d us\n", cpu_add_time);
    printk("GPU Addition Time: %d us\n", gpu_add_time);

    vec_mul_release(gpu);
    vec_add_release(add_gpu);
}

void test_increment(void)
{
    int i;
    volatile struct incrementGPU *gpu;
    vec_increment_init(&gpu);

    for (i = 0; i < N; i++) {
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

    for (i = 0; i < N; i++) {
        if (gpu->A[i] != (32 + i) + (64 + i)) {
            printk("Increment Iteration %d: %d + %d = %d. INCORRECT\n", i, gpu->A[i], gpu->B[i], gpu->A[i]);
        } else if (i % 64 == 0) {
            printk("Increment Iteration %d: %d + %d = %d. CORRECT\n", i, gpu->A[i], gpu->B[i], gpu->A[i]);
        }
    }

    // CPU benchmarks
    int cpu_increment_time = 0;

    start_time = timer_get_usec();
    for (i = 0; i < N; i++) gpu->A[i] = gpu->A[i] + gpu->B[i];
    end_time = timer_get_usec();
    cpu_increment_time = end_time - start_time;

    printk("\nCPU Increment Time: %d us\n", cpu_increment_time);
    printk("GPU Increment Time: %d us\n", gpu_increment_time);

    vec_increment_release(gpu);

}

void test_matmul(void)
{
    int i, j, k;
    volatile struct mulGPU *mul_gpu;
    volatile struct addGPU *add_gpu;
    volatile int *A, *B, *C;

    kmalloc_init(1024);

    vec_mul_init(&mul_gpu);
    vec_add_init(&add_gpu);

    A = (volatile int *)kmalloc(N * N * sizeof(int));
    B = (volatile int *)kmalloc(N * N * sizeof(int));
    C = (volatile int *)kmalloc(N * N * sizeof(int));

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            A[i * N + j] = 32 + i + j;
            B[j * N + i] = 64 + i - j;
            C[i * N + j] = 0;
        }
    }

    printk("Testing matrix multiplication...\n");
    printk("Memory before matrix multiplication: %x %x %x %x\n",
           C[0], C[1], C[2], C[3]);

    int start_time = timer_get_usec();

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            for (k = 0; k < N; k++)
            {
                mul_gpu->A[k] = A[i * N + k];
                mul_gpu->B[k] = B[j * N + k];
                mul_gpu->C[k] = 0;
            }

            vec_mul_exec(mul_gpu);

            int dot_sum = 0;
            for (k = 0; k < N; k++)
            {
                dot_sum += mul_gpu->C[k];
            }

            C[i * N + j] = dot_sum;
        }
    }

    int end_time = timer_get_usec();
    int gpu_matmul_time = end_time - start_time;

    printk("Memory after matrix multiplication: %d %d %d %d\n",
           C[0], C[1], C[2], C[3]);

    

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            int expected = 0;
            for (k = 0; k < N; k++)
            {
                expected += A[i * N + k] * B[j * N + k];
            }
            if (C[i * N + j] != expected)
            {
                printk("MatMul Element (%d,%d): Got %d, Expected %d. INCORRECT\n",
                       i, j, C[i * N + j], expected);
            }
            else if (i % 64 == 0 && j % 64 == 0)
            {
                printk("MatMul Element (%d,%d): %d. CORRECT\n",
                       i, j, C[i * N + j]);
            }
        }
    }

    printk("\nFirst 10x10 section of matrix C from GPU:\n");
    for (i = 0; i < 10; i++) {
        for (j = 0; j < 10; j++) {
            printk("%d\t", C[i * N + j]);
        }
        printk("\n");
    }

    // CPU benchmark
    int cpu_matmul_time = 0;

    // Reset C to zeros
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            C[i * N + j] = 0;
        }
    }

    start_time = timer_get_usec();
    // CPU implementation of matrix multiplication with B transposed
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            for (k = 0; k < N; k++)
            {
                C[i * N + j] += A[i * N + k] * B[j * N + k];
            }
        }
    }
    end_time = timer_get_usec();
    cpu_matmul_time = end_time - start_time;

    printk("\nCPU Matrix Multiplication Time: %d us\n", cpu_matmul_time);
    printk("GPU Matrix Multiplication Time: %d us\n", gpu_matmul_time);

    printk("\nFirst 10x10 section of matrix C from CPU:\n");
    for (i = 0; i < 10; i++) {
        for (j = 0; j < 10; j++) {
            printk("%d\t", C[i * N + j]);
        }
        printk("\n");
    }

    vec_mul_release(mul_gpu);
    vec_add_release(add_gpu);
}

void notmain(void)
{
    // printk("Testing addition and multiplication on GPU...\n");
    // test_add_mul();
    printk("Testing matrix multiplication on GPU...\n");
    test_matmul();
    // printk("Testing increment on GPU...\n");
    // test_increment();
}

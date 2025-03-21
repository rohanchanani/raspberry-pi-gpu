#include "rpi.h"
#include "vector-multiply.h"
#include "parallel-add.h"
#include "vector-increment.h"



#define MATRIX_SIZE 64

void test_matmul(void)
{
    int i, j, k;
    volatile struct mulGPU *mul_gpu;
    volatile struct addGPU *add_gpu;
    volatile int *A, *B, *C;

    kmalloc_init(1024);

    vec_mul_init(&mul_gpu, MATRIX_SIZE);

    A = (volatile int *)kmalloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(int));
    B = (volatile int *)kmalloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(int));
    C = (volatile int *)kmalloc(MATRIX_SIZE * MATRIX_SIZE * sizeof(int));

    for (i = 0; i < MATRIX_SIZE; i++)
    {
        for (j = 0; j < MATRIX_SIZE; j++)
        {
            A[i * MATRIX_SIZE + j] = (i + 1) * (j + 1);
            B[j * MATRIX_SIZE + i] = (i + 1) + (j + 1) * 2;
            C[i * MATRIX_SIZE + j] = 0;
        }
    }

    printk("Testing matrix multiplication...\n");
    printk("Memory before matrix multiplication: %x %x %x %x\n",
           C[0], C[1], C[2], C[3]);

    int start_time = timer_get_usec();
    int gpu_matmul_time = 0;
    int end_time = 0;
    for (i = 0; i < MATRIX_SIZE; i++)
    {
        for (j = 0; j < MATRIX_SIZE; j++) 
        {
            for (k = 0; k < MATRIX_SIZE; k++)
            {
                mul_gpu->A[k] = A[i * MATRIX_SIZE + k];
                mul_gpu->B[k] = B[j * MATRIX_SIZE + k];
                mul_gpu->C[k] = 0;
            }

            start_time = timer_get_usec();

            vec_mul_exec(mul_gpu);

            end_time = timer_get_usec();
            gpu_matmul_time += end_time - start_time;

            start_time = timer_get_usec();

            int dot_sum = 0;

            for (k = 0; k < MATRIX_SIZE; k++)
            {
                dot_sum += mul_gpu->C[k];
            }

            end_time = timer_get_usec();
            gpu_matmul_time += end_time - start_time;

            C[i * MATRIX_SIZE + j] = dot_sum;
        }
    }

    

    printk("Memory after matrix multiplication: %d %d %d %d\n",
           C[0], C[1], C[2], C[3]);

    for (i = 0; i < MATRIX_SIZE; i++)
    {
        for (j = 0; j < MATRIX_SIZE; j++)
        {
            int expected = 0;
            for (k = 0; k < MATRIX_SIZE; k++)
            {
                expected += A[i * MATRIX_SIZE + k] * B[j * MATRIX_SIZE + k];
            }
            if (C[i * MATRIX_SIZE + j] != expected)
            {
                printk("matmul element (%d,%d): got %d, expected %d. INCORRECT\n",
                       i, j, C[i * MATRIX_SIZE + j], expected);
            }
            else if (i % 64 == 0 && j % 64 == 0)
            {
                printk("matmul element (%d,%d): %d. CORRECT\n",
                       i, j, C[i * MATRIX_SIZE + j]);
            }
        }
    }

    printk("\nFirst 10x10 section of matrix from GPU:\n");
    for (i = 0; i < 10; i++)
    {
        for (j = 0; j < 10; j++)
        {
            printk("%d\t", C[i * MATRIX_SIZE + j]);
        }
        printk("\n");
    }

    // CPU benchmark
    int cpu_matmul_time = 0;

    // Reset C to zeros
    for (i = 0; i < MATRIX_SIZE; i++)
    {
        for (j = 0; j < MATRIX_SIZE; j++)
        {
            C[i * MATRIX_SIZE + j] = 0;
        }
    }

    start_time = timer_get_usec();
    // CPU implementation of matrix multiplication with B transposed
    for (i = 0; i < MATRIX_SIZE; i++)
    {
        for (j = 0; j < MATRIX_SIZE; j++)
        {
            for (k = 0; k < MATRIX_SIZE; k++)
            {
                C[i * MATRIX_SIZE + j] += A[i * MATRIX_SIZE + k] * B[j * MATRIX_SIZE + k];
            }
        }
    }
    end_time = timer_get_usec();
    cpu_matmul_time = end_time - start_time;


    printk("\nFirst 10x10 section of matrix C from CPU:\n");
    for (i = 0; i < 10; i++)
    {
        for (j = 0; j < 10; j++)
        {
            printk("%d\t", C[i * MATRIX_SIZE + j]);
        }
        printk("\n");
    }

    vec_mul_release(mul_gpu);

    printk("\n\nCPU Matrix Multiplication Time: %d us\n", cpu_matmul_time);
    printk("GPU Matrix Multiplication Time: %d us\n", gpu_matmul_time);
}

void notmain(void)
{
    printk("Testing matrix multiplication on GPU...\n");
    test_matmul();
}

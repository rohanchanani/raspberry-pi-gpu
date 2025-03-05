#include "rpi.h"

// Define V3D registers according to the VideoCore® IV 3D Architecture Reference Guide.
#define V3D_BASE 0x20C00000
#define QPU_SCHEDULER_CONTROL (V3D_BASE + 0x418) // QPU Scheduler Control Register
#define QPU_PROGRAM_COUNTER (V3D_BASE + 0x430) // Program Counter Register
#define QPU_UNIFORM_ADDRESS (V3D_BASE + 0x434) // QPU Uniforms Address Register
#define QPU_UNIFORM_LENGTH (V3D_BASE + 0x438) // QPU Uniforms Register
#define QPU_CONTROL_STATUS (V3D_BASE + 0x43c) // QPU Uniforms Register

// Initialize the QPU by resetting the control list executor.
void gpu_init(void)
{
    kmalloc_init(64);
    PUT32(QPU_SCHEDULER_CONTROL, 0);
    dmb();
    delay_ms(1);
}

// Load a QPU kernel into GPU-accessible memory.
void gpu_load_kernel(const uint32_t *kernel, size_t size)
{
    uint32_t *gpu_mem = (uint32_t *)kmalloc_aligned(size, 4096);
    if (!gpu_mem)
    {
        printk("GPU: Failed to allocate memory for kernel\n");
        return;
    }
    memcpy(gpu_mem, kernel, size);
    dmb();
    // Write the kernel address into the control list executor address register.
    PUT32(QPU_PROGRAM_COUNTER, (uint32_t)gpu_mem);
}

// Set the uniforms pointer for the QPU program.
// According to the documentation, the uniforms pointer is stored in the second word
// of the control list (i.e. at V3D_CT0EA + 0x4).
void gpu_set_uniforms(const uint32_t *uniforms, size_t size)
{
    PUT32(QPU_UNIFORM_ADDRESS, (uint32_t)uniforms);
    PUT32(QPU_UNIFORM_LENGTH, size);
}

// Start QPU execution by writing to the control register.
void gpu_run(void)
{
    PUT32(QPU_CONTROL_STATUS, 1);
    dmb();
}

// Poll the control list executor status until the QPU task completes.
int gpu_wait(void)
{
    while ((GET32(QPU_CONTROL_STATUS) & 0x1) == 0)
    {
        asm volatile("nop");
    }
    return 0;
}

// Retrieve the result from the QPU.
uint32_t gpu_get_result(void)
{
    return GET32(QPU_RESULT_ADDRESS);
}

// Inline memory barrier.
static inline void gpu_inline_barrier(void)
{
    dsb();
}

// Perform a complete QPU task: initialize, load kernel and uniforms, run, wait, and return the result.
int gpu_run_task(const uint32_t *kernel, size_t kernel_size, const uint32_t *uniforms)
{
    gpu_init();
    gpu_load_kernel(kernel, kernel_size);
    gpu_set_uniforms(uniforms);
    gpu_run();
    gpu_wait();
    return gpu_get_result();
}

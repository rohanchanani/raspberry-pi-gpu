#include "rpi.h"

// Define V3D registers according to the VideoCore® IV 3D Architecture Reference Guide.
#define V3D_BASE 0x20C00000
#define QPU_SCHEDULER_CONTROL (V3D_BASE + 0x410) // QPU Scheduler Control Register, V3D_SQCNTL
#define QPU_PROGRAM_COUNTER (V3D_BASE + 0x430)   // Program Counter Register V3D_SRQPC
#define QPU_UNIFORM_ADDRESS (V3D_BASE + 0x434)   // QPU Uniforms Address Register, V3D_SRQUA
#define QPU_UNIFORM_LENGTH (V3D_BASE + 0x438)    // QPU Uniforms Register, V3D_SRQUL
#define QPU_CONTROL_STATUS (V3D_BASE + 0x43c)    // QPU Control Status Register, V3D_SRQCS

#define VPM_ALLOCATOR_CONTROL (V3D_BASE + 0x500) // VPM Allocator Control Register, V3D_VPACNTL
#define VPM_BASE_RESERVATION (V3D_BASE + 0x504)  // VPM Base Reservation Register, V3D_VPMBASE


// Initialize the QPU by resetting the control list executor.
void gpu_init(void)
{
    kmalloc_init(64);
    PUT32(QPU_SCHEDULER_CONTROL, 0);
    PUT32(QPU_SCHEDULER_CONTROL + 4, 0);

    int val =GET32(QPU_SCHEDULER_CONTROL);
    printk("QPU scheduler control: %x\n", val);
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
    printk("GPU: Loaded kernel at %x\n", (uint32_t)gpu_mem);
    memcpy(gpu_mem, kernel, size);
    // Write the kernel address into the control list executor address register.
    PUT32(QPU_PROGRAM_COUNTER, (uint32_t)gpu_mem);

    dmb();
}

// Set the uniforms pointer for the QPU program.
// According to the documentation, the uniforms pointer is stored in the second word
// of the control list (i.e. at V3D_CT0EA + 0x4).
void gpu_set_uniforms(const uint32_t *uniforms, size_t size)
{
    PUT32(QPU_UNIFORM_ADDRESS, (uint32_t)uniforms);
    PUT32(QPU_UNIFORM_LENGTH, size);
}

// Poll the control list executor status until the QPU task completes.
int gpu_wait(void)
{
    while ((GET32(QPU_CONTROL_STATUS) & 0x3f) == 0)
    {
        asm volatile("nop");
    }
    return 0;
}

// Retrieve the result from the QPU.
uint32_t gpu_get_result(void)
{
    #define GPU_MEMORY_BASE (0x20C00000)
    #define DMA_BASE (GPU_MEMORY_BASE + 0x000)
    uint32_t result = 0;
    printk("Starting memdump\n");
    for (int i = 0; i < 1000000; i+= 100) {
        uint32_t addr = DMA_BASE + (i * sizeof(uint32_t));
        uint32_t val = GET32(addr);
        if (val != 0xdeadbeef) {
            printk("DMA addr %x: %x\n", addr, val);
            break;  
        }
    }
    printk("Ending memdump\n");
    return result;
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
    gpu_set_uniforms(uniforms, sizeof(uniforms));
    gpu_load_kernel(kernel, kernel_size);

    gpu_wait();
    return gpu_get_result();
}

void gpu_run(uint32_t* code, uint32_t* uniforms, size_t code_size, size_t uniforms_size) {

}

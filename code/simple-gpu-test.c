#include "rpi.h"
#include "gpu.h"

// Simple QPU kernel that adds two numbers
// This is a very basic kernel that adds two uniform values and returns the result
static const uint32_t simple_add_kernel[] = {
    // Load uniform 0 into register r0
    0x15827d80, 0x10020827, // mov r0, uniform
    // Load uniform 1 into register r1
    0x15827d80, 0x10020867, // mov r1, uniform
    // Add r0 and r1, store in r2
    0x159c1fc0, 0xd0020827, // add r2, r0, r1
    // Store result to memory (VPM)
    0x159e7000, 0x10020c67, // mov vpm, r2
    // Program end
    0x159e7900, 0x100009e7, // thrend
    0x159e7900, 0x500009e7  // nop
};

// Test uniforms - two numbers to add
static const uint32_t test_uniforms[] = {
    42,   // First number
    58    // Second number
};

void notmain(void) {
    printk("Starting simple GPU test\n");


    
    // Initialize the GPU
    gpu_init();
    printk("GPU initialized\n");
    
    #define V3D_BASE (0x20C00000)
    #define QPU_CONTROL_STATUS (V3D_BASE + 0x43c)    // QPU Control Status Register, V3D_SRQCS
    #define QPU_PROGRAM_COUNTER (V3D_BASE + 0x430)    // QPU Program Counter Register, V3D_SRQPC

    // // Set the control status register to 1 to start the QPU
    // gpu_load_kernel(simple_add_kernel, sizeof(simple_add_kernel));
    // gpu_load_kernel(simple_add_kernel, sizeof(simple_add_kernel));
    // gpu_load_kernel(simple_add_kernel, sizeof(simple_add_kernel));
    // gpu_load_kernel(simple_add_kernel, sizeof(simple_add_kernel));

    // PUT32(QPU_CONTROL_STATUS, 0);

    PUT32(QPU_PROGRAM_COUNTER, 0xEECCEECC);
    int cs_val = GET32(QPU_CONTROL_STATUS);
    int pc_val = GET32(QPU_PROGRAM_COUNTER);
    printk("QPU control status: %x\n", cs_val);
    printk("QPU program counter: %x\n", pc_val);

    uint32_t result = gpu_get_result();
    printk("GPU result: %x\n", result);

    printk("Simple GPU test completed\n");
} 
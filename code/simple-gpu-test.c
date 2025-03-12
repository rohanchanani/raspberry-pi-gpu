#include "rpi.h"
#include "mailbox.h"

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

    unsigned gpu_fft_base_exec_direct(
        struct GPU_FFT_BASE * base,
        int num_qpus)
}
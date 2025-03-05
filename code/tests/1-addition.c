#include "gpu.h"
#include "rpi.h"

// Simple QPU addition kernel
static const uint32_t add_kernel[] = {
    0xE0021B67,  // mov r0, unif
    0xE0031B67,  // mov r1, unif
    0xE0001D27,  // fadd r0, r0, r1
    0xE0021BA7,  // mov unif, r0
    0x100009E7   // thrsw
};

void notmain(void) {
    printk("Starting addition test...\n");
    // Initialize GPU
    gpu_init();
    
    // Test values
    uint32_t a = 5;
    uint32_t b = 7;
    uint32_t expected = a + b;
    
    // Prepare uniforms
    uint32_t uniforms[] = {a, b};
    
    printk("Running addition task...\n");
    // Run the addition task
    uint32_t result = gpu_run_task(add_kernel, sizeof(add_kernel), uniforms);
    
    // Verify result
    if (result == expected) {
        printk("Test passed! %d + %d = %d\n", a, b, result);
    } else {
        printk("Test failed! Expected %d, got %d\n", expected, result);
    }
}

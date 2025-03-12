#ifndef __GPU_H__
#define __GPU_H__

#include <stdint.h>
#include <stddef.h>

// Initialize the QPU
void gpu_init(void);

// Load a QPU kernel into GPU-accessible memory
void gpu_load_kernel(const uint32_t *kernel, size_t size);

// Set the uniforms pointer for the QPU program
void gpu_set_uniforms(const uint32_t *uniforms, size_t size);

// Start QPU execution
void gpu_run(uint32_t* code, uint32_t* uniforms, size_t code_size, size_t uniforms_size);

// Wait for QPU task completion
int gpu_wait(void);

// Retrieve the result from the QPU
uint32_t gpu_get_result(void);

// Perform a complete QPU task
int gpu_run_task(const uint32_t *kernel, size_t kernel_size, const uint32_t *uniforms);

// Memory management
void* gpu_alloc(uint32_t size, uint32_t align);
void gpu_free(void* ptr);

// Utilities
void gpu_flush_cache(void);

#endif // __GPU_H__

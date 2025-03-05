#ifndef GPU_H
#define GPU_H

#include <stdint.h>
#include <stddef.h>

// GPU initialization
void gpu_init(void);

// Kernel loading
void gpu_load_kernel(const uint32_t *kernel, size_t size);

// Uniforms handling
void gpu_set_uniforms(const uint32_t *uniforms);

// Execution control
void gpu_run(void);
int gpu_wait(void);

// Result handling
uint32_t gpu_get_result(void);

// Complete task execution
int gpu_run_task(const uint32_t *kernel, size_t kernel_size, const uint32_t *uniforms);

// Memory management
void* gpu_alloc(uint32_t size, uint32_t align);
void gpu_free(void* ptr);

// Utilities
void gpu_flush_cache(void);

#endif // GPU_H

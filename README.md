# The wonderful world of the VideoCore IV

## GPU Architecture

The Raspberry Pi uses the VideoCore IV GPU, also made by Broadcom. This was surprisingly well documented, all in this document: [VideoCore IV 3D Architecture Reference Guide](./docs/VideoCore%20IV%203D%20Architecture%20Reference%20Guide.pdf). To best take advantage of the GPU, we need to understand how it works.

The GPU is made of 16 QPUs, which are 4-way SIMD processors, which also have 16-way virtual SIMD through multiplexing.

The GPU has two ways of dealing with memory: VPM (Vertex Pipeline Memory) and TMU (Texture Memory Unit). Since we are focusing on general purpose shaders, we will only use VPM (although there existing UNIX implementations that are slightly faster using TMU).

## QPU Instructions

The QPUs run their own instruction set, which is similar to the ARMv6 instruction set, but with some modifications to be parallelizable over 16 cores. It has its own 64-bit instructions, for which somebody conveniently made an assembler [VC4ASM](https://maazl.de/project/vc4asm/doc/index.html). We write our shaders in the assembly language for VC4ASM, and then compile it to an array of C instructions using the `vc4asm` tool, which we can then pass to the GPU in code.

## Mailbox

We use a mailbox interface to communicate with the GPU. This is a way for the CPU to communicate with the GPU. We can send 32-byte messages to the GPU, and it will send us back 32-byte responses. We use the mailbox to allocate memory on the GPU, access that memory, and enable the QPUs. For some reason, we weren't able to send the actual shaders to the GPU, we had to directly write to the GPU control registers. After clearing the V3D caches, We simply need to write the arguments (uniforms) and the address of the shader code to `V3D_SRQUA` and `V3D_SRQPC` to launch the shader (page 90-92 of the [manual](./docs/VideoCore%20IV%203D%20Architecture%20Reference%20Guide.pdf)).

## VPM Read/Write

We use the VPM to read and write to the GPU memory. We need to perform a DMA transfer to read and write to the VPM. Luckily, VC4ASM has a very nice set of [macros](https://maazl.de/project/vc4asm/doc/vc4.qinc.html#VPM) to make this easier. You can find more on page 53 of the [manual](./docs/VideoCore%20IV%203D%20Architecture%20Reference%20Guide.pdf).

## The `struct GPU`

We defined a `struct GPU` to help us manage the memory and uniforms for our shaders. This helps us keep track of the allocated memory and the uniforms for each shader. Here's a simple example of the `struct GPU` we used for the parallel-add shader:

```c
struct addGPU
{
    uint32_t A[N]; // Input array A
    uint32_t B[N]; // Input array B
    uint32_t C[N]; // Output array C
    uint32_t code[sizeof(addshader) / sizeof(uint32_t)]; // Shader code
    uint32_t unif[NUM_QPUS][5]; // Uniforms for each QPU
    uint32_t unif_ptr[NUM_QPUS]; // Pointers to the uniforms
    uint32_t mail[2]; // Mailbox for communication
    uint32_t handle;
};
```

## Shaders

### DMA `deadbeef`

The first shader we wrote was a simple DMA test inspired by the instructions in this [minimal example for linux](https://github.com/0xfaded/gpu-deadbeef), which simply writes 0xdeadbeef to a spot in memory passed in as a uniform.

### Vectorized Arithmetic

After this, we wrote parallel-add and multiply shaders, which let us parallelize across both SIMD and multiple QPUs. These were both written from scratch in QASM and process the entire array in parallel. Each QPU is assigned a portion of the array to process, and uses SIMD to process it 16 operations at a time. You can find this code [here](./code/parallel-add.qasm) and [here](./code/vector-multiply.qasm). 

### Matrix Multiplication

We also wrote a matrix multiplication program using these vectorized shaders, but it was slow due to the amount of overhead involved in transferring memory back and forth. We can easily speed this up by combining all of the operations into a single shader, which is what we did for the mandelbrot shader.

### Mandelbrot

The final shader we wrote was a mandelbrot shader, which uses the FAT32 filesystem to draw a fully GPU computed mandelbrot shader to the SD card. This achieved a 570x speedup over the CPU implementation, which is because the mandelbrot shader is a perfectly parallel operation that is a perfect fit for the GPU. You can find this code [here](./code/mandelbrot.qasm).


## Running the code

To run the code, we first need to compile the QASM code to a C file using [`vc4asm`](https://maazl.de/project/vc4asm/doc/index.html). Then, we can run the code using the standard Makefile. The computed shaders are all included in the code directory, but they can also be recomputed by using the [`run.sh`](./code/run.sh) script.

## Quirks

- In QASM, we can't perform an ALU operation on two registers in the same regfile. This means no `add ra1, ra1, ra2`.
- Similarly, we can't read from a memory location until 3 cycles after we write it. This requires a lot of `nop`s
- A branch instruction will execute the 3 instructions after it before branching. This caused us a lot of issues, but is also easily solved by `nop`s.

## Useful Links

- [VideoCore IV 3D Architecture Reference Guide](./docs/VideoCore%20IV%203D%20Architecture%20Reference%20Guide.pdf)
- [VC4ASM](https://maazl.de/project/vc4asm/doc/index.html)
- [Pete Warden's Blog](https://petewarden.com/2014/08/07/how-to-optimize-raspberry-pi-code-using-its-gpu/) and [code](https://github.com/jetpacapp/pi-gemm)
- [Macoy Madson's Blog](https://macoy.me/blog/programming/PiGPU)
- [gpu-deadbeef](https://github.com/0xfaded/gpu-deadbeef)

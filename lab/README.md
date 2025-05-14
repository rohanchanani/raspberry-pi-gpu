# The wonderful world of the VideoCore IV

## Introduction

The Raspberry Pi uses the VideoCore IV GPU, also made by Broadcom. This was surprisingly well documented, all in this document: [VideoCore IV 3D Architecture Reference Guide](./docs/VideoCore%20IV%203D%20Architecture%20Reference%20Guide.pdf). To best take advantage of the GPU, we need to understand how it works.

GPU consists of 16 QPUs (quadcore processing units). The QPUs are technically 4-way SIMD processors (hence quad), but the actual execution is effectively 16-way virtual SIMD by use of multiplexing, and each of the QPU registers are 16-wide vector registers. 

The GPU has two memory buffers: VPM (Vertex Pipeline Memory) and TMU (Texture Memory Unit). Since we are focusing on general purpose shaders, we will only use VPM (although there existing UNIX implementations that are slightly faster using TMU. It would be great if someone could accelerate one of the VPM implementations using the TMU).

## Guide to the Docs

<b>[VideoCore IV 3D Architecture Reference Guide](./docs/VideoCore%20IV%203D%20Architecture%20Reference%20Guide.pdf) </b> 

- Introduction p. 12 
- Architecture overview p. 13-25

- VPM and VCD p. 53-56 
Describes the VPM, which is the intermediate memory buffer in between physical memory and QPU registers. Memory flow is GPU registers <-> VPM <-> physical memory where the VPM/physical memory flow is done by DMA. 
- VPM/VCD Registers p. 57-59 
Describes how to configure the registers to set up loads and stores in the aforementioned memory flow. VC4ASM has very helpful macros for this, but sometimes you need more fine-grained control. 
- V3D Registers p. 82-84 
The only ones we really care about are the ones written to in the QPU_EXECUTE_DIRECT function in our mailbox, and we still aren't 100% sure on why we have to write to these specifically. We couldn't get the mailbox execute QPU function to work, so we did it through the registers directly. 
- QPU Scheduler registers p. 89-91 
The important ones are program address, uniforms address, uniforms length, and user program request control/status. All you provide is an address to start executing code and an array of uniforms, and the scheduler runs the program and updates status. 

<b>[VC4ASM Assembler for the QPU](https://maazl.de/project/vc4asm/doc/index.html) </b> 
[Build Instructions](https://maazl.de/project/vc4asm/doc/index.html#build): Make sure you have `.include "../share/vc4inc/vc4.qinc"` at the top of your qasm files. 
[Assembler](https://maazl.de/project/vc4asm/doc/index.html#vc4asm): To assemble a kernel titled `kernel.qasm`, run `vc4asm -c kernel.c -h kernel.h kernel.qasm`, add `#include "kernel.h"` to the file where you launch the kernel,  and add `kernel.c` to your `COMMON_SRC` in the Makefile. 
[Expressions and Operators](https://www.maazl.de/project/vc4asm/doc/expressions.html): The syntax is pretty similar to ARM assembly. Some key things to notice:
- Accumulators are registers `r0`...`r5`. It's generally a good idea to do arithmetic operations with these (adds, muls, fmuls, etc.). Note that r4 is read-only, so probably shouldn't be used. 
- Memory registers are `ra0`...`ra31` and `rb0`...`rb31`. From the Broadcom documentation (page 17-18):  
    <b>The QPU pipeline is constructed such that register files have an entire pipeline cycle to perform a read operation. As the QPU pipeline length from register file read to write-back is greater than four cycles, one cannot write data to a physical QPU register file in one instruction and then read that same data for use in the next instruction (no forwarding paths are provided). QPU code is expected to make heavy use of the six accumulator registers, which do not suffer the restriction that data written by an instruction cannot be read in the next instruction.</b>
In general, avoid using two registers from the same file (e.g. `ra1` and `ra2` or `rb7` and `rb25`) in the same instruction or in back-to-back instructions. Sometimes it'll will work without out a problem, sometimes the assembler will tell you you've written illegal code, and in the worst case it'll fail silently. You'll need to store data in the memory registers, so to be safe, always do a `mov r1, ra1` before operating on that data (assuming the data's in `ra1` and `r1` is an available accum register), and then do `mov r1, ra1` afterward to free up the accumulator for other use.
- There are several special purpose registers. 
  - The `unif` register holds the queue of uniforms, which you will provide when you launch the kernel. The workflow is straightforward - if you have  a 4-element unif array, say [1, 0x\<some address>, 12, 56], then you can do 
`mov ra1, unif; mov ra2, unif; mov ra3, unif; mov ra4, unif;` and `ra1` will be a 16-wide "uniform" vector with each value holding `4`,`ra2` will be a 16-wide "uniform" vector with each value holding `0x\<some address>`, etc. 
  - The `vr` and `vw` registers manage DMA loads/stores and VPM loads/stores (see below for more info). 
  - The `elem_num` register gives the index in the 16-wide vector - equivalently, it holds (0, 1, 2, ..., 15). 
  - Some others

[Directives](https://www.maazl.de/project/vc4asm/doc/directives.html): Useful for constants, custom macros, etc. 

[Instructions](https://www.maazl.de/project/vc4asm/doc/instructions.html): Pretty digestible instruction set. Some quirks/useful things:

- Condition codes - recall that the QPU is 16-way SIMD. So an instruction with a condition code will only execute in the SIMD lanes in which that condition code applies (e.g. `mov.ifn` will only execute in lanes with the `N` flag set). To set the flags, you simply add `.setf` to the instruction (e.g. `add.setf`).   


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

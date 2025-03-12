/*
 * bare_mbox.c - Bare-metal mailbox interface for Raspberry Pi GPU memory
 *
 * This implementation provides functions equivalent to those in the
 * provided mailbox.c file but does not rely on any operating system
 * services. Instead, it accesses the mailbox registers directly.
 *
 * Supported functions:
 *   - mailbox_write() / mailbox_read(): Basic mailbox I/O.
 *   - mailbox_call(): Sends a property message and waits for the response.
 *   - mem_alloc(): Allocate GPU memory.
 *   - mem_lock(): Lock allocated memory and return its bus address.
 *   - mem_unlock(): Unlock memory.
 *   - mem_free(): Release GPU memory.
 *   - qpu_enable(): Enable (or disable) the QPU.
 *   - execute_qpu(): Launch QPU code.
 *
 * References:
 *   - Raspberry Pi GPU firmware documentation (VideoCore IV property interface)
 *   - Bare-metal mailbox tutorials (e.g., Baking Pi)
 *
 * Adjust MAILBOX_BASE below according to your Raspberry Pi:
 *   Pi 1:  0x2000B880
 *   Pi 2/3: 0x3F00B880
 *   Pi 4:  0xFE00B880 (or similar)
 */

#include <stdint.h>
#include "rpi.h"

//
// Mailbox register definitions (volatile pointers)
//
#define MAILBOX_BASE 0x2000B880 // Change this value for your model
#define MAILBOX_READ (*(volatile uint32_t *)(MAILBOX_BASE + 0x0))
#define MAILBOX_STATUS (*(volatile uint32_t *)(MAILBOX_BASE + 0x18))
#define MAILBOX_WRITE (*(volatile uint32_t *)(MAILBOX_BASE + 0x20))

#define MAILBOX_FULL 0x80000000
#define MAILBOX_EMPTY 0x40000000

#define V3D_BASE 0x20C00000
#define V3D_SRQSC (V3D_BASE + 0x418)		   // QPU Scheduler Control Register
#define QPU_PROGRAM_COUNTER (V3D_BASE + 0x430) // Program Counter Register
#define QPU_UNIFORM_ADDRESS (V3D_BASE + 0x434) // QPU Uniforms Address Register
#define QPU_UNIFORM_LENGTH (V3D_BASE + 0x438)  // QPU Uniforms Register
#define QPU_CONTROL_STATUS (V3D_BASE + 0x43c)  // QPU Uniforms Register

#define V3D_L2CACTL (V3D_BASE + 0x020)
#define V3D_SLCACTL (V3D_BASE + 0x024)
#define V3D_SRQPC (V3D_BASE + 0x0430)
#define V3D_SRQUA (V3D_BASE + 0x0434)
#define V3D_SRQCS (V3D_BASE + 0x043c)
#define V3D_DBCFG (V3D_BASE + 0x0e00)
#define V3D_DBQITE (V3D_BASE + 0x0e2c)
#define V3D_DBQITC (V3D_BASE + 0x0e30)
//
// Basic mailbox I/O routines
//

// Write 'data' to the mailbox on the specified channel.
void mailbox_write(uint8_t channel, uint32_t data)
{
	// Wait until mailbox is not full.
	while (MAILBOX_STATUS & MAILBOX_FULL)
	{
	}
	// Lower 4 bits are used for channel.
	MAILBOX_WRITE = (data & ~0xF) | (channel & 0xF);
}

// Read from the mailbox on the specified channel.
uint32_t mailbox_read(uint8_t channel)
{
	uint32_t data;
	while (1)
	{
		data = MAILBOX_READ;
		// Check that the channel matches.
		if ((data & 0xF) == channel)
			return data & ~0xF;
	}
}

// Perform a mailbox property call.
// The message buffer 'msg' must be 16-byte aligned.
// Returns nonzero if the call succeeded.
int mbox_property(uint32_t *msg)
{
	// Check alignment.
	if ((uint32_t)msg & 0xF)
		return 0;
	mailbox_write(8, (uint32_t)msg);
	// Wait for the response.
	trace("sent call\n");
	//
	while (mailbox_read(8) != (uint32_t)msg)
	{
	}
	trace("received call\n");
	// The response code is in msg[1]; 0x80000000 indicates success.
	return (msg[1] == 0x80000000);
}

//
// Mailbox property calls for GPU memory and QPU control.
// All property messages are sent on mailbox channel 8.
//

uint32_t mem_alloc(uint32_t size, uint32_t align, uint32_t flags)
{

	uint32_t p[9] __attribute__((aligned(16))) =
		{
			9 * sizeof(uint32_t), // size
			0x00000000,			  // process request
			0x3000c,			  // (the tag id)
			3 * sizeof(uint32_t), // (size of the buffer)
			3 * sizeof(uint32_t), // (size of the data)
			size,				  // (num bytes)
			align,				  // (alignment)
			flags,				  // (MEM_FLAG_L1_NONALLOCATING)
			0					  // end tag
		};
	assert(mbox_property(p));
	return p[5];
}

uint32_t mem_free(uint32_t handle)
{

	uint32_t p[7] __attribute__((aligned(16))) =
		{
			7 * sizeof(uint32_t), // size
			0x00000000,			  // process request
			0x3000f,			  // (the tag id)
			1 * sizeof(uint32_t), // (size of the buffer)
			1 * sizeof(uint32_t), // (size of the data)
			handle,				  // (handle)
			0					  // end tag
		};
	assert(mbox_property(p));
	return p[5];
}

uint32_t mem_lock(uint32_t handle)
{

	uint32_t p[7] __attribute__((aligned(16))) =
		{
			7 * sizeof(uint32_t), // size
			0x00000000,			  // process request
			0x3000d,			  // (the tag id)
			1 * sizeof(uint32_t), // (size of the buffer)
			1 * sizeof(uint32_t), // (size of the data)
			handle,				  // (handle)
			0					  // end tag
		};
	assert(mbox_property(p));
	return p[5];
}

uint32_t mem_unlock(uint32_t handle)
{
	uint32_t p[7] __attribute__((aligned(16))) =
		{
			7 * sizeof(uint32_t), // size
			0x00000000,			  // process request
			0x3000e,			  // (the tag id)
			1 * sizeof(uint32_t), // (size of the buffer)
			1 * sizeof(uint32_t), // (size of the data)
			handle,				  // (handle)
			0					  // end tag
		};
	assert(mbox_property(p));
	return p[5];
}

uint32_t qpu_enable(uint32_t enable)
{

	uint32_t p[7] __attribute__((aligned(16))) =
		{
			7 * sizeof(uint32_t), // size
			0x00000000,			  // process request
			0x30012,			  // (the tag id)
			1 * sizeof(uint32_t), // (size of the buffer)
			1 * sizeof(uint32_t), // (size of the data)
			enable,				  // (enable QPU)
			0					  // end tag
		};
	assert(mbox_property(p));
	return p[5];
}

uint32_t execute_qpu(uint32_t num_qpus, uint32_t control, uint32_t noflush, uint32_t timeout)
{
	uint32_t p[10] __attribute__((aligned(16))) =
		{
			10 * sizeof(uint32_t), // size
			0x00000000,			   // process request
			0x30011,			   // (the tag id)
			4 * sizeof(uint32_t),  // (size of the buffer)
			4 * sizeof(uint32_t),  // (size of the data)
			num_qpus,
			control,
			noflush,
			timeout, // ms
			0		 // end tag
		};
	assert(mbox_property(p));
	return p[5];
}

unsigned gpu_fft_base_exec_direct(
	uint32_t code,
	uint32_t unifs,
	int num_qpus)
{

	unsigned q, t;

	PUT32(V3D_DBCFG, 0); // Disallow IRQ

	PUT32(V3D_DBQITE, 0);  // Disable IRQ
	PUT32(V3D_DBQITC, -1); // Resets IRQ flags

	PUT32(V3D_L2CACTL, 1 << 2); // Clear L2 cache
	PUT32(V3D_SLCACTL, -1);		// Clear other caches

	PUT32(V3D_SRQCS, (1 << 7) | (1 << 8) | (1 << 16)); // Reset error bit and counts

	for (q = 0; q < num_qpus; q++)
	{ // Launch shader(s)
		PUT32(V3D_SRQUA, (uint32_t)unifs);
		PUT32(V3D_SRQPC, (uint32_t)code);
	}

	// Busy wait polling
	for (;;)
	{
		if (((GET32(V3D_SRQCS) >> 16) & 0xff) == num_qpus)
		{
			printk("got value %x\n", GET32(V3D_SRQCS));
			break; // All done?
		}
	}

	return 0;
}
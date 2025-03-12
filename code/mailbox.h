#ifndef BARE_MBOX_H
#define BARE_MBOX_H

#include <stdint.h>
#include <stddef.h>

/* 
 * Bare-metal Mailbox Interface for Raspberry Pi
 *
 * This header defines the functions for interacting with the GPU's mailbox
 * property interface in a bare-metal environment.
 *
 * Functions:
 *   - mailbox_write(): Write a message to the mailbox.
 *   - mailbox_read():  Read a message from the mailbox.
 *   - mailbox_call():  Send a property message and wait for the response.
 *
 *   - mem_alloc():     Allocate GPU memory.
 *   - mem_free():      Free GPU memory.
 *   - mem_lock():      Lock allocated GPU memory and return its bus address.
 *   - mem_unlock():    Unlock GPU memory.
 *
 *   - qpu_enable():    Enable (or disable) the QPU.
 *   - execute_qpu():   Execute QPU code.
 *
 * All property messages are sent on mailbox channel 8.
 * 
 * Note: Ensure that the mailbox message buffers are 16-byte aligned.
 */

/* Basic mailbox I/O routines */
void mailbox_write(uint8_t channel, uint32_t data);
uint32_t mailbox_read(uint8_t channel);

/* Mailbox property call.
 * 'msg' must be 16-byte aligned.
 * Returns nonzero on success (i.e. if msg[1] == 0x80000000).
 */
int mbox_property(uint32_t *msg);

/* GPU memory allocation and management functions.
 *
 * mem_alloc:  Allocates 'size' bytes of GPU memory, with the given 'align'
 *             and 'flags'. Returns a nonzero handle on success.
 *
 * mem_free:   Releases the GPU memory associated with the given handle.
 *
 * mem_lock:   Locks the allocated GPU memory to obtain a bus address.
 *             Returns the bus address on success.
 *
 * mem_unlock: Unlocks the previously locked GPU memory.
 */
uint32_t mem_alloc(uint32_t size, uint32_t align, uint32_t flags);
uint32_t mem_free(uint32_t handle);
uint32_t mem_lock(uint32_t handle);
uint32_t mem_unlock(uint32_t handle);

/* QPU control functions.
 *
 * qpu_enable: Enable (or disable) the QPU. Pass 1 to enable, 0 to disable.
 *
 * execute_qpu: Launches QPU code. 
 *              Parameters:
 *                num_qpus - number of QPUs to execute
 *                control  - control flags for execution
 *                noflush  - set to nonzero to disable flush operations
 *                timeout  - timeout in milliseconds
 *              Returns a status code (typically the first response word).
 */
uint32_t qpu_enable(uint32_t enable);
uint32_t execute_qpu(uint32_t num_qpus, uint32_t control, uint32_t noflush, uint32_t timeout);

void clean_and_invalidate_cache();

#endif /* BARE_MBOX_H */


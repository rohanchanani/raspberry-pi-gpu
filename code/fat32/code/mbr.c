#include "rpi.h"
#include "pi-sd.h"
#include "mbr.h"

mbr_t *mbr_read() {
  // Be sure to call pi_sd_init() before calling this function!
  
  mbr_t* to_return = pi_sec_read(0, 1);
  // TODO: Read the MBR into a heap-allocated buffer.  Use `pi_sd_read` or
  // `pi_sec_read` to read 1 sector from LBA 0 into memory.

  // TODO: Verify that the MBR is valid. (see mbr_check)
  mbr_check(to_return);

  // TODO: Return the MBR.
  return to_return;
}

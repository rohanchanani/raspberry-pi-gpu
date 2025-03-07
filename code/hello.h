#ifndef hello_H
#define hello_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t hello[58];

#define smi_start (hello + 0)

#ifdef __cplusplus
}
#endif
#endif

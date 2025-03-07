#include "output.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
__declspec(align(8))
#elif defined(__GNUC__)
__attribute__((aligned(8)))
#endif
uint32_t output[6] = {
0x009e7000, 0x300009e7,
0x00000001, 0xe00209a7,
0x009e7000, 0x100009e7
};
#ifdef __HIGHC__
#pragma Align_to(8, output)
#ifdef __cplusplus
}
#endif
#endif

#include "simpleshader.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
__declspec(align(8))
#elif defined(__GNUC__)
__attribute__((aligned(8)))
#endif
uint32_t simpleshader[20] = {
0x00101a00, 0xe0021c67,
0xfaded070, 0xe0020c27,
0x159f2fc0, 0x100009e7,
0x80844000, 0xe0021c67,
0x15827d80, 0x10020827,
0x159e7000, 0x10021ca7,
0x159f2fc0, 0x100009e7,
0x009e7000, 0x300009e7,
0x009e7000, 0x100009e7,
0x009e7000, 0x100009e7
};
#ifdef __HIGHC__
#pragma Align_to(8, simpleshader)
#ifdef __cplusplus
}
#endif
#endif

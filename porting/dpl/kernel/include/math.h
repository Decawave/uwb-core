#include <linux/kernel.h>

#ifndef M_PI
#define M_PI 3.1415926535
#endif

#ifndef asinf
#define asinf(x) 1
#endif

#ifndef ceilf
#define ceilf(x) ((int32_t)x)
#endif

#ifndef sqrt
/* Use linux-kernel */
#define sqrt(_X) int_sqrt(_X)
#endif

//#include "softfloat/softfloat.h"
//#include <specialize.h>

#include "softfloat/softfloat.h"
#include <softfloat/specialize.h>

float64_t soft_nan(void) {
    float64_t a = {.v = defaultNaNF64UI };
    return a;
}
EXPORT_SYMBOL(soft_nan);

bool soft_isnan(float64_t a) {
    return (a.v == defaultNaNF64UI);
}
EXPORT_SYMBOL(soft_isnan);

float32_t soft_nan32(void) {
    float32_t a = {.v = defaultNaNF32UI };
    return a;
}
EXPORT_SYMBOL(soft_nan32);

bool soft_isnan32(float32_t a) {
    return (a.v == defaultNaNF32UI);
}
EXPORT_SYMBOL(soft_isnan32);

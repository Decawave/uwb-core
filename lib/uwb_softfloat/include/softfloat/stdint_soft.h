#ifndef STDINT_SOFT_H
#define STDINT_SOFT_H

#ifdef __KERNEL__

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/kernel.h>


typedef u8 uint_fast8_t;
typedef u8 uint_least8_t;
typedef s8 int_fast8_t;

typedef u16 uint_fast16_t;
typedef u16 uint_least16_t;
typedef s16 int_fast16_t;

typedef u32 uint_fast32_t;
typedef u32 uint_least32_t;
typedef s32 int_fast32_t;

typedef u64 uint_fast64_t;
typedef u64 uint_least64_t;
typedef s64 int_fast64_t;

#endif /* __KERNEL__ */

#ifdef MYNEWT

#include <stdint.h>
#include <stddef.h>

#endif /* MYNEWT */

#endif /* STDINT_SOFT_H */
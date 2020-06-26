#ifndef _STDLIB_H
#define _STDLIB_H

#include <linux/slab.h>
#include <linux/kernel.h>

#ifndef calloc
#define calloc(_X,_Y) kzalloc(_X*_Y, GFP_KERNEL)
#endif

#ifndef free
#define free(_X) kfree(_X)
#endif

#ifndef printf
#define printf(fmt, ...) printk("uwbcore:printf@%s:%d|"fmt, __func__, __LINE__, ##__VA_ARGS__)
#endif

#endif

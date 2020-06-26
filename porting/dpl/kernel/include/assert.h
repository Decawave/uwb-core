#ifndef ASSERT_H
#define ASSERT_H

#include <linux/bug.h>
#include <linux/kernel.h>

#ifndef assert
#define assert(x)                                                       \
do {    if (x) break;                                                   \
        printk(KERN_EMERG "### ASSERTION FAILED %s: %s: %d: %s\n",      \
               __FILE__, __func__, __LINE__, #x); dump_stack(); BUG();  \
} while (0)
#endif

#endif

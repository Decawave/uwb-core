#include "config/config.h"
#include <stdio.h>
#include <string.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kernel.h>

#ifndef INT8_MIN
#define INT8_MIN -127
#endif

#ifndef INT8_MAX
#define INT8_MAX 127
#endif

#ifndef UINT8_MAX
#define UINT8_MAX 255
#endif

#ifndef INT16_MIN
#define INT16_MIN -32767
#endif

#ifndef INT16_MAX
#define INT16_MAX 32767
#endif

#ifndef UINT16_MAX
#define UINT16_MAX 65535
#endif

int
conf_value_from_str(char *val_str, enum conf_type type, void *vp, int maxlen)
{
    int rc;
    long val;
    int64_t val64;

    if (!val_str) {
        goto err;
    }
    switch (type) {
    case CONF_INT8:
    case CONF_INT16:
    case CONF_INT32:
    case CONF_BOOL:
        rc = kstrtol(val_str, 0, &val);
        if (rc) {
            goto err;
        }
        if (type == CONF_BOOL) {
            if (val < 0 || val > 1) {
                goto err;
            }
            *(bool *)vp = val;
        } else if (type == CONF_INT8) {
            if (val < INT8_MIN || val > UINT8_MAX) {
                goto err;
            }
            *(int8_t *)vp = val;
        } else if (type == CONF_INT16) {
            if (val < INT16_MIN || val > UINT16_MAX) {
                goto err;
            }
            *(int16_t *)vp = val;
        } else if (type == CONF_INT32) {
            *(int32_t *)vp = val;
        }
        break;
    case CONF_INT64:
        rc = kstrtoll(val_str, 0, &val64);
        if (rc) {
            goto err;
        }
        *(int64_t *)vp = val64;
        break;
    case CONF_STRING:
        val = strlen(val_str);
        if (val + 1 > maxlen) {
            goto err;
        }
        strcpy(vp, val_str);
        break;
    default:
        goto err;
    }
    return 0;
err:
    return -1;
}

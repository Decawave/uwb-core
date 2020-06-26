#ifndef __CONTROL_H__
#define __CONTROL_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <time.h>

typedef void (*dwm_cb_fn_ptr)(char* input_string);
struct dwm_cb {
    dwm_cb_fn_ptr call_back;
};

void *init_ranging(void *arg);
void *start_ranging(void *arg);
void *stop_ranging(void *arg);
void *cb_register(dwm_cb_fn_ptr cb);

#ifdef __cplusplus
}
#endif

/* Required when Linking with AOSP framework */
#if defined(ANDROID_APK_BUILD)
#include <android/log.h>
#define LOG_TAG "control"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)

#else

#define LOG_TAG
#define LOGD(...)
#endif

#endif

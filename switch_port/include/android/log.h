#pragma once
#include <stdarg.h>

#define ANDROID_LOG_INFO 4
#define ANDROID_LOG_FATAL 7

static inline int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{
    (void)prio;
    (void)tag;
    (void)fmt;
    return 0;
}

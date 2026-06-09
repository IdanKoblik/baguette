#pragma once

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <time.h>

#define LOG(fmt, ...)                                                                     \
    do {                                                                                  \
        time_t _now = time(NULL);                                                         \
        struct tm _tm;                                                                    \
        localtime_r(&_now, &_tm);                                                         \
        char _buf[64];                                                                    \
        strftime(_buf, sizeof(_buf), "%Y-%m-%d %H:%M:%S", &_tm);                          \
        fprintf(stderr, "[%s %s:%d] " fmt "\n", _buf, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

// WTF
#define STR2(A) #A
#define STR(A) STR2(A)

#define INFO(fmt, ...)                                                             \
    do {                                                                           \
        syslog(LOG_INFO, "[" __FILE__ ":" STR(__LINE__) "]: " fmt, ##__VA_ARGS__); \
    } while (0)

#define ERROR(fmt, ...)                                                                            \
    do {                                                                                           \
        int _err = errno;                                                                          \
        syslog(LOG_ERR, "[" __FILE__ ":" STR(__LINE__) "]: " fmt " (errno=%d: %s)", ##__VA_ARGS__, \
               _err, strerror(_err));                                                              \
    } while (0)

#define WARN(fmt, ...)                                                                \
    do {                                                                              \
        syslog(LOG_WARNING, "[" __FILE__ ":" STR(__LINE__) "]: " fmt, ##__VA_ARGS__); \
    } while (0)

#define DEBUG(fmt, ...)                                                             \
    do {                                                                            \
        syslog(LOG_DEBUG, "[" __FILE__ ":" STR(__LINE__) "]: " fmt, ##__VA_ARGS__); \
    } while (0)

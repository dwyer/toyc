#ifndef LOG_H
#define LOG_H

#include <execinfo.h> // backtrace
#include <stdio.h>

#define LOG_LEVEL_ALL 0
#define LOG_LEVEL_VERBOSE 1
#define LOG_LEVEL_DEBUG 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_WARN 4
#define LOG_LEVEL_ERROR 5

#ifndef LOG_LEVEL
#   define LOG_LEVEL LOG_LEVEL_INFO
#endif

#define _LOG(tag, fmt, ...) fprintf(stderr, tag fmt "\n", ## __VA_ARGS__)

#if LOG_LEVEL <= LOG_LEVEL_VERBOSE
#   define LOGV(fmt, ...) _LOG("Verbose: ", fmt, ## __VA_ARGS__)
#else
#   define LOGV(fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_DEBUG
#   define LOGD(fmt, ...) _LOG("Debug: ", fmt, ## __VA_ARGS__)
#else
#   define LOGD(fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_INFO
#   define LOGI(fmt, ...) _LOG("Info: ", fmt, ## __VA_ARGS__)
#else
#   define LOGI(fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_WARN
#   define LOGW(fmt, ...) _LOG("Warning: ", fmt, ## __VA_ARGS__)
#else
#   define LOGW(fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_ERROR
#   define LOGE(fmt, ...) _LOG("Error: ", fmt, ## __VA_ARGS__)
#else
#   define LOGE(fmt, ...)
#endif

#define PANIC(fmt, ...) \
    do { \
        _LOG("Fatal: ", "%s:%d: " fmt, __FILE__, __LINE__, ## __VA_ARGS__); \
        void *_buf__##__LINE__[100]; \
        int _n__##__LINE__ = backtrace(_buf__##__LINE__, 100); \
        backtrace_symbols_fd(_buf__##__LINE__, _n__##__LINE__, 2); \
        exit(1); \
    } while (0)

#endif

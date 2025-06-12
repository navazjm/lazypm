//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// common.h - Common utility functions and macros.
//

#pragma once

#include "termbox2.h"
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <time.h>

//
// Utilites nicked from nob.h. Developers there are 10x smarter than me...
// github.com/tsoding/nob.h
//

#define LPM_UNUSED(value) (void)(value)

#define LPM_UNREACHABLE(message)                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        tb_shutdown();                                                                                                 \
        fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, message);                                      \
        abort();                                                                                                       \
    } while (0)

#ifndef LPM_ASSERT
#include <assert.h>
#define LPM_ASSERT assert
#endif // LPM_ASSERT

#ifndef LPM_MALLOC
#include <stdlib.h>
#define LPM_MALLOC malloc
#endif // LPM_MALLOC

#ifndef LPM_REALLOC
#include <stdlib.h>
#define LPM_REALLOC realloc
#endif // LPM_REALLOC

#ifndef LPM_FREE
#include <stdlib.h>
#define LPM_FREE free
#endif // LPM_FREE

#ifndef LPM_STRLEN
#include <string.h>
#define LPM_STRLEN strlen
#endif // LPM_STRLEN

#ifndef LPM_STRDUP
#include <string.h>
#define LPM_STRDUP strdup
#endif // LPM_STRDUP

#ifndef LPM_VASPRINTF
#include <stdio.h>
#define LPM_VASPRINTF vasprintf
#endif // LPM_VASPRINTF

// Initial capacity of a dynamic array
#ifndef LPM_DA_INIT_CAP
#define LPM_DA_INIT_CAP 256
#endif // LPM_DA_INIT_CAP

#define LPM_DA_RESERVE(da, expected_capacity)                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        if ((expected_capacity) > (da)->capacity)                                                                      \
        {                                                                                                              \
            if ((da)->capacity == 0)                                                                                   \
            {                                                                                                          \
                (da)->capacity = LPM_DA_INIT_CAP;                                                                      \
            }                                                                                                          \
            while ((expected_capacity) > (da)->capacity)                                                               \
            {                                                                                                          \
                (da)->capacity *= 2;                                                                                   \
            }                                                                                                          \
            (da)->items = LPM_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items));                             \
            LPM_ASSERT((da)->items != NULL && "Buy more RAM lol");                                                     \
        }                                                                                                              \
    } while (0)

// Append an item to a dynamic array
#define LPM_DA_APPEND(da, item)                                                                                        \
    do                                                                                                                 \
    {                                                                                                                  \
        LPM_DA_RESERVE((da), (da)->count + 1);                                                                         \
        (da)->items[(da)->count++] = (item);                                                                           \
    } while (0)

#define LPM_DA_FREE(da) LPM_FREE((da).items)

#define LPM_CLEANUP_RETURN(value)                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        result = (value);                                                                                              \
        goto cleanup;                                                                                                  \
    } while (0)

static inline char *lpm_strdup(const char *s)
{
    char *dup = LPM_STRDUP(s);
    if (!dup)
        LPM_ASSERT(0 && "lpm_strdup: Out of memory");
    return dup;
}

static inline int lpm_asprintf(char **strp, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = LPM_VASPRINTF(strp, fmt, args);
    va_end(args);
    if (ret == -1)
        LPM_ASSERT(0 && "lpm_asprintf: Out of memory");
    return ret;
}

//
// Color scheme
//

#define LPM_FG_COLOR TB_DEFAULT
#define LPM_FG_COLOR_DIM (TB_DEFAULT | TB_DIM)
#define LPM_FG_COLOR_BLACK TB_BLACK
#define LPM_FG_COLOR_BLACK_DIM (TB_BLACK | TB_DIM)
#define LPM_FG_COLOR_GREEN TB_GREEN
#define LPM_FG_COLOR_RED TB_RED
#define LPM_FG_COLOR_BLUE TB_BLUE
#define LPM_BG_COLOR 0
#define LPM_BG_COLOR_HIGHLIGHT TB_MAGENTA

//
// Exit Codes
//

typedef enum
{
    LPM_OK,
    LPM_ERROR,
    LPM_QUIT,
} LPM_Exit_Code;

//
// Logs
//

typedef enum
{
    LPM_LOG_LEVEL_INFO,
    LPM_LOG_LEVEL_WARNING,
    LPM_LOG_LEVEL_ERROR,

} LPM_Log_Level;

static inline void _lpm_log(LPM_Log_Level level, const char *file, int line, const char *fmt, ...)
{
    const char *xdg_state_home = getenv("XDG_STATE_HOME");
    const char *home = getenv("HOME");
    LPM_ASSERT(home != NULL && "no home dir found...");

    char path[512];
    int len = 0;

    if (xdg_state_home)
        len = snprintf(path, sizeof(path), "%s/lazypm", xdg_state_home);
    else
        len = snprintf(path, sizeof(path), "%s/.local/state/lazypm", home);

    if (mkdir(path, 0755) == -1 && errno != EEXIST)
        LPM_ASSERT(0 && "Failed to create lazypm directory");

    len += snprintf(path + len, sizeof(path) - len, "/logs");
    if (mkdir(path, 0755) == -1 && errno != EEXIST)
        LPM_ASSERT(0 && "Failed to create lazypm/log directory");

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    snprintf(path + len, sizeof(path) - len, "/lazypm-%04d-%02d-%02d.log", tm_info->tm_year + 1900, tm_info->tm_mon + 1,
             tm_info->tm_mday);

    FILE *fd = fopen(path, "a");
    LPM_ASSERT(fd != NULL && "failed to open log file...");

    char time_buf[16];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);
    fprintf(fd, "[%s ", time_buf);

    switch (level)
    {
    case LPM_LOG_LEVEL_INFO:
        fprintf(fd, "- INFO]\n");
        break;
    case LPM_LOG_LEVEL_WARNING:
        fprintf(fd, "- WARNING]\n");
        break;
    case LPM_LOG_LEVEL_ERROR:
        fprintf(fd, "- ERROR]\n");
        break;
    default:
        LPM_UNREACHABLE("lpm_log");
    }

    va_list args;
    va_start(args, fmt);
    fprintf(fd, "\tMessage : ");
    vfprintf(fd, fmt, args);
    va_end(args);
    fprintf(fd, "\tSource  : %s:%d\n\n", file, line);

    if (fclose(fd) == EOF)
        LPM_ASSERT(0 && "Failed to close log file");
}

#define LPM_LOG_INFO(fmt, ...) _lpm_log(LPM_LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LPM_LOG_WARNING(fmt, ...) _lpm_log(LPM_LOG_LEVEL_WARNING, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LPM_LOG_ERROR(fmt, ...) _lpm_log(LPM_LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

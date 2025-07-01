//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// common.h - Common utility functions and macros.
//

#pragma once

#include "external/termbox2.h"
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>

//
// Utilites nicked from nob.h. Developers there are 10x smarter than me...
// github.com/tsoding/nob.h
//

#define LPM_UNUSED(value) (void)(value)

#define LPM_UNREACHABLE(message)                                                                   \
    do                                                                                             \
    {                                                                                              \
        tb_shutdown();                                                                             \
        fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, message);                  \
        abort();                                                                                   \
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
#define _LPM_FREE free
#endif // LPM_FREE

#define LPM_FREE(ptr)                                                                              \
    do                                                                                             \
    {                                                                                              \
        if (ptr)                                                                                   \
        {                                                                                          \
            _LPM_FREE(ptr);                                                                        \
            (ptr) = NULL;                                                                          \
        }                                                                                          \
    } while (0)

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

#define LPM_DA_RESERVE(da, expected_capacity)                                                      \
    do                                                                                             \
    {                                                                                              \
        if ((expected_capacity) > (da)->capacity)                                                  \
        {                                                                                          \
            if ((da)->capacity == 0)                                                               \
            {                                                                                      \
                (da)->capacity = LPM_DA_INIT_CAP;                                                  \
            }                                                                                      \
            while ((expected_capacity) > (da)->capacity)                                           \
            {                                                                                      \
                (da)->capacity *= 2;                                                               \
            }                                                                                      \
            (da)->items = LPM_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items));         \
            LPM_ASSERT((da)->items != NULL && "Buy more RAM lol");                                 \
        }                                                                                          \
    } while (0)

// Append an item to a dynamic array
#define LPM_DA_APPEND(da, item)                                                                    \
    do                                                                                             \
    {                                                                                              \
        LPM_DA_RESERVE((da), (da)->count + 1);                                                     \
        (da)->items[(da)->count++] = (item);                                                       \
    } while (0)

#define LPM_DA_FREE(da) LPM_FREE((da).items)

#define LPM_CLEANUP_RETURN(value)                                                                  \
    do                                                                                             \
    {                                                                                              \
        result = (value);                                                                          \
        goto cleanup;                                                                              \
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

static inline int lpm_vasprintf(char **strp, const char *fmt, va_list args)
{
    int ret = LPM_VASPRINTF(strp, fmt, args);
    if (ret == -1)
        LPM_ASSERT(0 && "lmp_vasprintf: Out of memory");
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
#define LPM_BG_COLOR_HIGHLIGHT_FILTER TB_BLUE

//
// Exit Codes
//

typedef enum
{
    LPM_OK,
    LPM_ERROR,
    LPM_QUIT,
    LPM_ERROR_PIPE_OPEN,
    LPM_ERROR_PIPE_CLOSE,
    LPM_ERROR_FILE_READ,
    LPM_ERROR_COMMAND_FAIL,
} LPM_Exit_Code;

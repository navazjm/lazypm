//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// common.h - Common utility functions and macros.
//

#pragma once

#define LPM_FG_COLOR TB_DEFAULT
#define LPM_FG_COLOR_DIM (TB_DEFAULT | TB_DIM)
#define LPM_FG_COLOR_BLACK TB_BLACK
#define LPM_FG_COLOR_BLACK_DIM (TB_BLACK | TB_DIM)
#define LPM_FG_COLOR_GREEN TB_GREEN
#define LPM_FG_COLOR_RED TB_RED
#define LPM_FG_COLOR_BLUE TB_BLUE
#define LPM_BG_COLOR 0
#define LPM_BG_COLOR_HIGHLIGHT TB_MAGENTA

typedef enum
{
    LPM_OK,
    LPM_ERROR,
    LPM_QUIT,
} LPM_Exit_Code;

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

// Initial capacity of a dynamic array
#ifndef LPM_DA_INIT_CAP
#define LPM_DA_INIT_CAP 256
#endif // LPM_DA_INIT_CAP

#define lpm_da_reserve(da, expected_capacity)                                                                          \
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
#define lpm_da_append(da, item)                                                                                        \
    do                                                                                                                 \
    {                                                                                                                  \
        lpm_da_reserve((da), (da)->count + 1);                                                                         \
        (da)->items[(da)->count++] = (item);                                                                           \
    } while (0)

#define lpm_da_free(da) LPM_FREE((da).items)

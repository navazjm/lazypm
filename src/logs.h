//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// logs.h
//

#include "common.h"

typedef enum
{
    LPM_LOG_LEVEL_INFO,
    LPM_LOG_LEVEL_WARNING,
    LPM_LOG_LEVEL_ERROR,

} LPM_Log_Level;
char *lpm_log_level_str(LPM_Log_Level log_level);

void lpm_log_dump_session(void);
char *lpm_log_file_path(void);

void _lpm_log(LPM_Log_Level level, const char *file, int line, const char *fmt, ...);
#define LPM_LOG_INFO(fmt, ...) _lpm_log(LPM_LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LPM_LOG_WARNING(fmt, ...)                                                                  \
    _lpm_log(LPM_LOG_LEVEL_WARNING, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LPM_LOG_ERROR(fmt, ...)                                                                    \
    _lpm_log(LPM_LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

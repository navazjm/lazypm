//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// logs.c
//

#include "logs.h"

char *lpm_log_file_path()
{
    const char *xdg_state_home = getenv("XDG_STATE_HOME");
    const char *home = getenv("HOME");
    LPM_ASSERT(home != NULL && "no home dir found...");

    char *base_path;
    if (xdg_state_home)
        lpm_asprintf(&base_path, "%s/lazypm", xdg_state_home);
    else
        lpm_asprintf(&base_path, "%s/.local/state/lazypm", home);

    if (mkdir(base_path, 0755) == -1 && errno != EEXIST)
        LPM_ASSERT(0 && "Failed to create lazypm directory");

    char *log_dir;
    lpm_asprintf(&log_dir, "%s/logs", base_path);
    if (mkdir(log_dir, 0755) == -1 && errno != EEXIST)
        LPM_ASSERT(0 && "Failed to create lazypm/log directory");

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    char *final_path;
    lpm_asprintf(&final_path, "%s/lazypm-%04d-%02d-%02d.log", log_dir, tm_info->tm_year + 1900,
                 tm_info->tm_mon + 1, tm_info->tm_mday);

    LPM_FREE(base_path);
    LPM_FREE(log_dir);

    return final_path;
}

void _lpm_log(LPM_Log_Level level, const char *file, int line, const char *fmt, ...)
{
    char *path = lpm_log_file_path();
    FILE *fd = fopen(path, "a");
    LPM_ASSERT(fd != NULL && "failed to open log file...");
    LPM_FREE(path);

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
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

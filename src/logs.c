//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// logs.c
//

#include "logs.h"

char *lpm_log_level_str(LPM_Log_Level log_level)
{
    switch (log_level)
    {
    case LPM_LOG_LEVEL_INFO:
        return "[INFO]";
    case LPM_LOG_LEVEL_WARNING:
        return "[WARN]";
    case LPM_LOG_LEVEL_ERROR:
        return "[ERROR]";
    default:
        LPM_UNREACHABLE("lpm_log");
    }
}

static char *_log_buffer = {0};

void lpm_log_dump_session(void)
{
    if (_log_buffer)
    {
        fprintf(stderr, "%s --- Lazypm Logs ----------------------------------------\n",
                lpm_log_level_str(LPM_LOG_LEVEL_INFO));
        fprintf(stderr, "%s", _log_buffer);
        char *path = lpm_log_file_path();
        fprintf(stderr, "%s Full log: %s\n", lpm_log_level_str(LPM_LOG_LEVEL_INFO), path);
        LPM_FREE(path);
        fprintf(stderr, "%s --- End Logs -------------------------------------------\n",
                lpm_log_level_str(LPM_LOG_LEVEL_INFO));
    }
}

char *lpm_log_file_path(void)
{
    const char *home = getenv("HOME");
    LPM_ASSERT(home != NULL && "$HOME not found...");
    if (strncmp(home, "/root", 5) == 0)
    {
        const char *sudo_user = getenv("SUDO_USER");
        LPM_ASSERT(home != NULL && "$SUDO_USER not found...");
        // If running under sudo, try to get the real user's home
        if (sudo_user && strcmp(sudo_user, "root") != 0)
        {
            // Get the original user's home directory
            struct passwd *pw = getpwnam(sudo_user);
            if (pw)
                home = pw->pw_dir;
        }
    }

    char *base_path;
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

    const char *level_str = lpm_log_level_str(level);

    // Format the user message first
    char *user_message;
    va_list args;
    va_start(args, fmt);
    lpm_vasprintf(&user_message, fmt, args);
    va_end(args);
    // Build complete log entry
    char *log_entry;
    lpm_asprintf(&log_entry, "%-7s %s\n\tMessage : %s\n\tSource  : %s:%d\n", level_str, time_buf,
                 user_message, file, line);

    // Write to file
    fprintf(fd, "%s", log_entry);
    if (fclose(fd) == EOF)
        LPM_ASSERT(0 && "Failed to close log file");

    // Append to session log buffer
    if (_log_buffer == NULL)
    {
        _log_buffer = LPM_STRDUP(log_entry);
    }
    else
    {
        char *old_buffer = _log_buffer;
        lpm_asprintf(&_log_buffer, "%s%s", old_buffer, log_entry);
        LPM_FREE(old_buffer);
    }

    // Cleanup
    LPM_FREE(user_message);
    LPM_FREE(log_entry);
}

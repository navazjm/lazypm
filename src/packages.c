//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// packages.c - Track and display xbps packages
//

#include "packages.h"
#include "common.h"
#include "status.h"
#include <ctype.h>

void lpm_packages_teardown(LPM_Packages *pkgs)
{
    for (size_t i = 0; i < pkgs->count; ++i)
    {
        LPM_FREE(pkgs->items[i].status);
        pkgs->items[i].status = NULL;
        LPM_FREE(pkgs->items[i].name);
        pkgs->items[i].name = NULL;
        LPM_FREE(pkgs->items[i].description);
        pkgs->items[i].description = NULL;
    }
    LPM_FREE(pkgs->items);
    pkgs->items = NULL;
    pkgs->capacity = 0;
    pkgs->count = 0;
}

int lpm_packages_get(LPM_Packages *pkgs, const char *pkg_name)
{
    int result = LPM_OK;
    char *err_msg = NULL;
    LPM_Packages new_pkgs = {0};
    FILE *fp = NULL;
    char *line = NULL;
    size_t len = 0;

    // pkg_name NULL -> pass empty string to get all packages
    char *cmd;
    lpm_asprintf(&cmd, "xbps-query -Rs '%s'", pkg_name ? pkg_name : "");

    fp = popen(cmd, "r");
    if (fp == NULL)
    {
        LPM_LOG_ERROR("popen() failed for command: \"%s\"\n\tReason  : %s\n", cmd, strerror(errno));
        err_msg = "Failed to retrieve package(s).";
        LPM_CLEANUP_RETURN(LPM_ERROR);
    }

    while (getline(&line, &len, fp) != -1)
    {
        LPM_Package pkg = {0};
        char temp = line[3];
        line[3] = '\0';
        pkg.status = lpm_strdup(line);
        line[3] = temp;

        size_t i = 4;
        while (!isspace(line[i]))
        {
            i++;
        }
        temp = line[i];
        line[i] = '\0';
        pkg.name = lpm_strdup(line + 4);
        line[i] = temp;

        while (isspace(line[i]))
        {
            i++;
        }
        pkg.description = lpm_strdup(line + i);

        LPM_DA_APPEND(&new_pkgs, pkg);
    }

    if (ferror(fp))
    {
        LPM_LOG_ERROR("An error occurred while reading the output of: \"%s\"\n\tReason:%s\n", cmd, strerror(errno));

        err_msg = "Failed to parse query results.";
        LPM_CLEANUP_RETURN(LPM_ERROR);
    }

cleanup:
    LPM_FREE(cmd);
    LPM_FREE(line);
    if (fp && pclose(fp) == -1)
    {
        // since pclose failing isn't detrimental to our application, we can just silently
        // log an error if we don't already have an error message.

        LPM_LOG_WARNING("Failed to close command stream: \"%s\"\nReason: %s\n", cmd, strerror(errno));
        if (err_msg == NULL)
            err_msg = "Command succeeded but failed to close pipe stream.";
    }
    if (result == LPM_OK)
    {
        // clear previous packages and set new packages
        lpm_packages_teardown(pkgs);
        *pkgs = new_pkgs;
    }
    else
    {
        lpm_packages_teardown(&new_pkgs);
    }
    if (err_msg)
        lpm_status_msg_set_error(err_msg);

    return result;
}

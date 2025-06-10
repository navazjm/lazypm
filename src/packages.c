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
#include <errno.h>
#include <stdio.h>
#include <string.h>

void lpm_packages_teardown(LPM_Packages *pkgs)
{
    for (size_t i = 0; i < pkgs->count; ++i)
    {
        if (pkgs->items[i].status)
        {
            LPM_FREE(pkgs->items[i].status);
            pkgs->items[i].status = NULL;
        }
        if (pkgs->items[i].name)
        {
            LPM_FREE(pkgs->items[i].name);
            pkgs->items[i].name = NULL;
        }
        if (pkgs->items[i].description)
        {
            LPM_FREE(pkgs->items[i].description);
            pkgs->items[i].description = NULL;
        }
    }
    LPM_FREE(pkgs->items);
    pkgs->items = NULL;
    pkgs->capacity = 0;
    pkgs->count = 0;
}

// TODO: store fprintf into a log file
int lpm_packages_get(LPM_Packages *pkgs, const char *cmd)
{
    lpm_packages_teardown(pkgs);

    FILE *fp;
    char *line = NULL;
    size_t len = 0;

    fp = popen(cmd, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "[ERROR] popen() failed for command: \"%s\"\nReason: %s\n", cmd, strerror(errno));

        char *status_msg;
        asprintf(&status_msg, "Failed to get packages: %s", cmd);
        lpm_status_msg_set_error(status_msg);
        LPM_FREE(status_msg);
        return LPM_ERROR;
    }

    while (getline(&line, &len, fp) != -1)
    {
        LPM_Package pkg = {0};
        char temp = line[3];
        line[3] = '\0';
        pkg.status = strdup(line);
        line[3] = temp;

        size_t i = 4;
        while (!isspace(line[i]))
        {
            i++;
        }
        temp = line[i];
        line[i] = '\0';
        pkg.name = strdup(line + 4);
        line[i] = temp;

        while (isspace(line[i]))
        {
            i++;
        }
        pkg.description = strdup(line + i);

        lpm_da_append(pkgs, pkg);
    }

    free(line);

    int result = LPM_OK;
    if (ferror(fp))
    {
        fprintf(stderr, "[ERROR] An error occurred while reading the output of: \"%s\"\nReason: %s\n", cmd,
                strerror(errno));

        char *status_msg;
        asprintf(&status_msg, "Failed to get packages: %s", cmd);
        lpm_status_msg_set_error(status_msg);
        LPM_FREE(status_msg);
        result = LPM_ERROR;
    }

    if (pclose(fp) == -1)
    {
        fprintf(stderr, "[ERROR] Failed to close command stream: \"%s\"\nReason: %s\n", cmd, strerror(errno));

        char *status_msg;
        asprintf(&status_msg, "Failed to get packages: %s", cmd);
        lpm_status_msg_set_error(status_msg);
        LPM_FREE(status_msg);
        result = LPM_ERROR;
    }

    return result;
}

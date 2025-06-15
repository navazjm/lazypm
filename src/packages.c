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

typedef void (*LPM_Packages_Parse_Line_Callback)(char *line, void *data);

static uint8_t _lpm_packages_run_cmd(const char *cmd, LPM_Packages_Parse_Line_Callback callback,
                                     void *data)
{
    uint8_t result = LPM_OK;
    FILE *fp = NULL;
    char *line = NULL;
    size_t len = 0;

    fp = popen(cmd, "r");
    if (fp == NULL)
    {
        LPM_LOG_ERROR("popen() failed for command: \"%s\"\n\tReason  : %s\n", cmd, strerror(errno));
        LPM_CLEANUP_RETURN(LPM_ERROR_PIPE_OPEN);
    }

    while (getline(&line, &len, fp) != -1)
    {
        if (callback)
            callback(line, data);
    }

    if (ferror(fp))
    {
        LPM_LOG_ERROR("An error occurred while reading the output of: \"%s\"\n\tReason:%s\n", cmd,
                      strerror(errno));

        LPM_CLEANUP_RETURN(LPM_ERROR_FILE_READ);
    }

cleanup:
    LPM_FREE(line);
    if (fp)
    {
        int exit_status = pclose(fp);
        if (exit_status == -1)
        {
            LPM_LOG_WARNING("Failed to close command stream: \"%s\"\nReason: %s\n", cmd,
                            strerror(errno));
            if (result != LPM_ERROR_FILE_READ)
                result = LPM_ERROR_PIPE_CLOSE;
        }
        else if (WIFEXITED(exit_status))
        {
            int code = WEXITSTATUS(exit_status);
            if (code != 0)
            {
                LPM_LOG_ERROR("Command exited with non-zero status: %d\n", code);
                result = LPM_ERROR_COMMAND_FAIL; // define this constant
            }
        }
        else
        {
            LPM_LOG_ERROR("Command did not exit normally: \"%s\"\n", cmd);
            result = LPM_ERROR_COMMAND_FAIL;
        }
    }
    return result;
}

static void _lpm_packages_get_callback(char *line, void *data)
{
    LPM_Packages *pkgs = (LPM_Packages *)data;

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

    LPM_DA_APPEND(pkgs, pkg);
}

uint8_t lpm_packages_get(LPM_Packages *pkgs, const char *pkg_name)
{
    LPM_Packages new_pkgs = {0};

    // pkg_name NULL -> pass empty string to get all packages
    char *cmd;
    lpm_asprintf(&cmd, "xbps-query -Rs '%s'", pkg_name ? pkg_name : "");

    uint8_t result = _lpm_packages_run_cmd(cmd, _lpm_packages_get_callback, &new_pkgs);
    LPM_FREE(cmd);

    if (result == LPM_OK || result == LPM_ERROR_PIPE_CLOSE)
    {
        // clear previous packages and set new packages
        lpm_packages_teardown(pkgs);
        *pkgs = new_pkgs;
        if (result == LPM_ERROR_PIPE_CLOSE)
            lpm_status_msg_set_info("Query command succeeded but failed to close pipe stream.");
        return LPM_OK;
    }

    if (result == LPM_ERROR_PIPE_OPEN)
        lpm_status_msg_set_error("Failed to open pipe stream to query package(s).");
    else if (result == LPM_ERROR_FILE_READ)
        lpm_status_msg_set_error("Failed to parse query results.");
    else if (result == LPM_ERROR_COMMAND_FAIL)
        lpm_status_msg_set_error("Command failed to query package(s).");
    else
        LPM_UNREACHABLE("lpm_packages_get error checking");

    lpm_packages_teardown(&new_pkgs);
    return LPM_ERROR;
}

uint8_t lpm_packages_install(LPM_Package *pkg)
{
    if (pkg == NULL || pkg->name == NULL)
        return LPM_ERROR;

    char *cmd;
    lpm_asprintf(&cmd, "sudo xbps-install -Sy '%s' 2>&1", pkg->name);

    uint8_t result = _lpm_packages_run_cmd(cmd, NULL, NULL);
    LPM_FREE(cmd);

    if (result == LPM_ERROR_PIPE_OPEN)
        lpm_status_msg_set_error("Failed to open pipe stream to install package.");
    else if (result == LPM_ERROR_FILE_READ)
        lpm_status_msg_set_error("Failed to parse install results.");
    else if (result == LPM_ERROR_COMMAND_FAIL)
        lpm_status_msg_set_error("Command failed to install package.");
    else if (result == LPM_OK || result == LPM_ERROR_PIPE_CLOSE)
    {
        bool is_update = strcmp(pkg->status, LPM_PACKAGE_STATUS_INSTALLED) == 0;
        LPM_FREE(pkg->status);
        pkg->status = lpm_strdup(LPM_PACKAGE_STATUS_INSTALLED);

        if (result == LPM_OK)
        {
            char *status_msg;
            char *action = "installed";
            if (is_update)
                action = "updated";
            lpm_asprintf(&status_msg, "Package '%s' was %s successfully.", pkg->name, action);
            lpm_status_msg_set_success(status_msg);
            LPM_FREE(status_msg);
        }
        else
            lpm_status_msg_set_info("Install command succeeded but failed to close pipe stream.");
    }
    else
        LPM_UNREACHABLE("lpm_packages_install error checking");

    return result;
}

uint8_t lpm_packages_update_all()
{
    char *cmd = "sudo xbps-install -Syu 2>&1";
    uint8_t result = _lpm_packages_run_cmd(cmd, NULL, NULL);

    if (result == LPM_ERROR_PIPE_OPEN)
        lpm_status_msg_set_error("Failed to open pipe stream to update all packages.");
    else if (result == LPM_ERROR_FILE_READ)
        lpm_status_msg_set_error("Failed to parse update results.");
    else if (result == LPM_ERROR_COMMAND_FAIL)
        lpm_status_msg_set_error("Command failed to update all packages.");
    else if (result == LPM_OK)
        lpm_status_msg_set_success("Updated all packages successfully.");
    else if (result == LPM_ERROR_PIPE_CLOSE)
    {
        lpm_status_msg_set_info("Update command succeeded but failed to close pipe stream.");
        result = LPM_OK;
    }
    else
        LPM_UNREACHABLE("lpm_packages_update_all error checking");

    return result;
}

uint8_t lpm_packages_uninstall(LPM_Package *pkg)
{

    char *cmd;
    lpm_asprintf(&cmd, "sudo xbps-remove -yo '%s' 2>&1", pkg->name);
    uint8_t result = _lpm_packages_run_cmd(cmd, NULL, NULL);
    LPM_FREE(cmd);

    if (result == LPM_ERROR_PIPE_OPEN)
        lpm_status_msg_set_error("Failed to open pipe stream to uninstall package.");
    else if (result == LPM_ERROR_FILE_READ)
        lpm_status_msg_set_error("Failed to parse uninstall results.");
    else if (result == LPM_ERROR_COMMAND_FAIL)
        lpm_status_msg_set_error("Command failed to uninstall package.");
    else if (result == LPM_OK || result == LPM_ERROR_PIPE_CLOSE)
    {
        LPM_FREE(pkg->status);
        pkg->status = LPM_STRDUP(LPM_PACKAGE_STATUS_AVAILABLE);
        if (result == LPM_OK)
            lpm_status_msg_set_success("Uninstalled package successfully.");
        else
            lpm_status_msg_set_info("Uninstall command succeeded but failed to close pipe stream.");
        result = LPM_OK;
    }
    else
        LPM_UNREACHABLE("lpm_packages_uninstall error checking");

    return result;
}

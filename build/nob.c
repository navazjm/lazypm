// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more

#define NOB_IMPLEMENTATION
#define NOB_EXPERIMENTAL_DELETE_OLD

#include "nob.h"

#define BUILD_FOLDER "build/"
#define SRC_FOLDER "src/"

#define BUILD_FAILED_MSG                                                                           \
    nob_log(NOB_ERROR, "--- Build Failed --------------------------------------");

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    nob_log(NOB_INFO, "--- Build Lazypm ---------------------------------------");

    Nob_Cmd cmd = {0};
    bool install_lazypm = false;
    bool run_lazypm = false;

    while (argc > 1)
    {
        char *flag = argv[1];
        if (strcmp(flag, "--install") == 0 || strcmp(flag, "-i") == 0)
        {
            install_lazypm = true;
        }
        else if (strcmp(flag, "--run") == 0 || strcmp(flag, "-r") == 0)
        {
            run_lazypm = true;
        }
        else
        {
            nob_log(NOB_WARNING, "Unknown flag: \"%s\"", flag);
        }
        nob_shift_args(&argc, &argv);
    }
    nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra");

    Nob_File_Paths src_files = {0};
    if (!nob_read_entire_dir(SRC_FOLDER, &src_files))
    {
        BUILD_FAILED_MSG
        return 1;
    }

    for (size_t i = 0; i < src_files.count; ++i)
    {
        const char *temp_file_name = src_files.items[i];
        if (strcmp(temp_file_name, ".") == 0)
            continue;
        if (strcmp(temp_file_name, "..") == 0)
            continue;

        const char *temp_full_path = nob_temp_sprintf("%s%s", SRC_FOLDER, temp_file_name);
        if (nob_get_file_type(temp_full_path) == NOB_FILE_DIRECTORY)
            continue;

        int len = strlen(temp_file_name);
        const char *file_ext = &temp_file_name[len - 2];
        if (strcmp(file_ext, ".h") == 0)
            continue;
        nob_cmd_append(&cmd, temp_full_path);
    }

    nob_cmd_append(&cmd, "-o", BUILD_FOLDER "lazypm");
    if (!nob_cmd_run_sync_and_reset(&cmd))
    {
        BUILD_FAILED_MSG
        return 1;
    }

    nob_log(NOB_INFO, "--- Build Succeeded ------------------------------------");
    
    // reaching here, the actual build of lazypm has succeeded, but user may have passed
    // in extra flags to automate running other processes.

    // copy lazypm executable to /usr/local/bin
    if (install_lazypm)
    {
        nob_log(NOB_INFO, "--- Install Lazypm -------------------------------------");
        char *install_path = "/usr/local/bin";
        nob_cmd_append(&cmd, "sudo", "cp", BUILD_FOLDER "lazypm", install_path);
        if (!nob_cmd_run_sync_and_reset(&cmd))
        {
            nob_log(NOB_ERROR, "Failed to copy lazypm executable to \"%s\"", install_path);
            return 1;
        }
        nob_log(NOB_INFO, "--- Install Succeeded ----------------------------------");
    }

    if (run_lazypm)
    {
        nob_log(NOB_INFO, "--- Run Lazypm -----------------------------------------");
        nob_cmd_append(&cmd,"sudo", BUILD_FOLDER "lazypm");
        if (!nob_cmd_run_sync_and_reset(&cmd))
        {
            nob_log(NOB_ERROR, "Failed to run lazypm");
            return 1;
        }
        nob_log(NOB_INFO, "--- End Lazypm -----------------------------------------");
    }

    return 0;
}

// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more

#define NOB_IMPLEMENTATION
#define NOB_EXPERIMENTAL_DELETE_OLD

#include "nob.h"

#define BUILD_FOLDER "build/"
#define SRC_FOLDER "src/"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    nob_log(NOB_INFO, "--- Starting build ------------------------------------");

    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra");

    Nob_File_Paths src_files = {0};
    if (!nob_read_entire_dir(SRC_FOLDER, &src_files))
        return 1;

    for (size_t i = 0; i < src_files.count; ++i)
    {
        const char *temp_file_name = src_files.items[i];
        if (strcmp(temp_file_name, ".") == 0)
            continue;
        if (strcmp(temp_file_name, "..") == 0)
            continue;

        int len = strlen(temp_file_name);
        const char *file_ext = &temp_file_name[len - 2];
        if (strcmp(file_ext, ".h") == 0)
            continue;
        nob_cmd_append(&cmd, nob_temp_sprintf("%s%s", SRC_FOLDER, temp_file_name));
    }

    nob_cmd_append(&cmd, "-o", BUILD_FOLDER "lazypm");
    if (!nob_cmd_run_sync_and_reset(&cmd))
        return 1;

    nob_log(NOB_INFO, "--- Build complete ------------------------------------");
    return 0;
}

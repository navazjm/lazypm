// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more

#define NOB_IMPLEMENTATION
#define NOB_EXPERIMENTAL_DELETE_OLD

#include "nob.h"

#define BUILD_FOLDER "./build/"
#define SRC_FOLDER "./src/"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "cc", "-Wall", "-Wextra", "-o", BUILD_FOLDER "lazypm", SRC_FOLDER "lazypm.c");

    if (!nob_cmd_run_sync_and_reset(&cmd))
        return 1;

    nob_cmd_append(&cmd, "sudo", "cp", BUILD_FOLDER "./lazypm", "/usr/local/bin/");
    if (!nob_cmd_run_sync_and_reset(&cmd))
        return 1;

    return 0;
}

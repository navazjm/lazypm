//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// lazypm.c - Entry point into the lazypm tui application.
//

#define TB_IMPL

#include "tui.h"

int main(void)
{
    // until we implement feature to capture user's password, we will require users to
    // run `sudo lazypm`...
    if (getuid() != 0)
    {
        LPM_LOG_ERROR("lazypm requires root privileges to manage system packages.\n"
                      "\t\t  Please run with sudo: sudo lazypm");
        lpm_log_dump_session();
        return LPM_ERROR;
    }
    lpm_tui_crash_signals();
    LPM_TUI_Layout layout = {0};
    LPM_Packages pkgs = {0};
    LPM_Exit_Code result = lpm_tui_setup(&layout, &pkgs);
    if (result == LPM_OK)
        lpm_tui_run(&layout, &pkgs);
    lpm_tui_teardown(&layout, &pkgs);
    return result;
}

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
    lpm_tui_crash_signals();
    LPM_TUI_Layout layout = {0};
    LPM_Packages pkgs = {0};
    LPM_Exit_Code result = lpm_tui_setup(&layout, &pkgs);
    if (result == LPM_OK)
        lpm_tui_run(&layout, &pkgs);
    lpm_tui_teardown(&layout, &pkgs);
    return result;
}

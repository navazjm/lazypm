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
    int error = tb_init();
    if (error)
    {
        LPM_LOG_ERROR("Failed to initialized termbox2\n\tReason  : %s\n", tb_strerror(error));
        return error;
    }

    LPM_TUI_Layout layout = {0};
    lpm_tui_layout_setup(&layout);

    LPM_Packages pkgs = {0};
    error = lpm_packages_get(&pkgs, NULL);

    if (error == LPM_OK)
        lpm_tui_run(&layout, &pkgs);

    tb_shutdown();
    lpm_tui_layout_teardown(&layout);
    lpm_packages_teardown(&pkgs);

    return error;
}

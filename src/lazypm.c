//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// lazypm.c - Entry point into the lazypm tui application.
//

#define TB_IMPL
#define MIN_WIDTH 80
#define MIN_HEIGHT 15

#include "tui.h"

int main(void)
{
    int error = tb_init();
    if (error)
    {
        LPM_LOG_ERROR("Failed to initialized termbox2\n\tReason  : %s\n", tb_strerror(error));
        return error;
    }

    if (tb_width() < MIN_WIDTH)
    {
        LPM_LOG_ERROR("Terminal dimensions too small\n\tReason  : Width %d < %d\n", tb_width(),
                      MIN_WIDTH);
        tb_shutdown();
        return LPM_ERROR;
    }
    if (tb_height() < MIN_HEIGHT)
    {
        LPM_LOG_ERROR("Terminal dimensions too small\n\tReason  : Height %d < %d\n", tb_height(),
                      MIN_HEIGHT);
        tb_shutdown();
        return LPM_ERROR;
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

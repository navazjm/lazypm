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
    lpm_tui_crash_signals();

    int error = tb_init();
    if (error)
    {
        LPM_LOG_ERROR("Failed to initialized termbox2\n\tReason  : %s", tb_strerror(error));
        lpm_log_dump_session();
        return error;
    }

    if (tb_width() < MIN_WIDTH)
    {
        LPM_LOG_ERROR("Terminal dimensions too small\n\tReason  : Width %d < %d", tb_width(),
                      MIN_WIDTH);
        tb_shutdown();
        lpm_log_dump_session();
        return LPM_ERROR;
    }
    if (tb_height() < MIN_HEIGHT)
    {
        LPM_LOG_ERROR("Terminal dimensions too small\n\tReason  : Height %d < %d", tb_height(),
                      MIN_HEIGHT);
        tb_shutdown();
        lpm_log_dump_session();
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
    lpm_log_dump_session();

    return error;
}

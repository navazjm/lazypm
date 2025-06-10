//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// layout.c - Lazypm TUI layout
//

#include "layout.h"
#include "termbox2.h"

void lpm_layout_setup(LPM_Layout *layout)
{
    layout->min_xpos = 5;
    layout->max_xpos = tb_width() - layout->min_xpos;
    layout->min_ypos = 1;
    layout->max_ypos = tb_height() - layout->min_ypos;
    layout->header_xpos = layout->min_xpos;
    layout->header_ypos = layout->min_ypos;
    layout->filter_xpos = layout->min_xpos;
    layout->filter_ypos = 0;
    layout->packages_xpos = layout->min_xpos;
    layout->packages_ypos = 0;
    layout->packages_cursor_ypos = 0;
    layout->packages_page_index = 0;
    layout->packages_total_pages = 1;
    layout->packages_render_capacity = 0;
    layout->footer_xpos = layout->min_xpos;
    layout->footer_ypos = layout->max_ypos - 2;
}

void lpm_layout_teardown(LPM_Layout *layout)
{
    (void)layout;
}

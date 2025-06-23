//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// tui.h - Lazypm TUI
//

#pragma once

#include "common.h"
#include "packages.h"
#include "status.h"

typedef enum
{
    LPM_TUI_MODE_NORMAL,
    LPM_TUI_MODE_FILTER,
} LPM_TUI_Mode;

typedef struct
{
    uint8_t min_xpos; // Horizontal padding: leftmost column where layout begins
    uint8_t max_xpos; // Horizontal padding: rightmost column where layout ends
    uint8_t min_ypos; // Vertical padding: top row where layout begins
    uint8_t max_ypos; // Vertical padding: bottom row where layout ends

    uint8_t header_xpos; // X position of the header text (column)
    uint8_t header_ypos; // Y position of the header text (row)

    uint8_t packages_xpos;            // X position of the packages list (column)
    uint8_t packages_ypos;            // Y position of the packages list (row)
    uint8_t packages_cursor_ypos;     // Y position of the hovered("selected") package
    size_t packages_page_index;       // Current page of packages to render
    size_t packages_total_pages;      // Total pages of packages
    uint8_t packages_render_capacity; // Distance between packages_ypos and footer_ypos,
                                      // giving us largest number of packages we can render per page

    uint8_t footer_xpos; // X position of the footer text (column)
    uint8_t footer_ypos; // Y position of the footer text (row)
} LPM_TUI_Layout;

void lpm_tui_layout_setup(LPM_TUI_Layout *layout);
void lpm_tui_layout_teardown(LPM_TUI_Layout *layout);

void lpm_tui_run(LPM_TUI_Layout *layout, LPM_Packages *pkgs);
uint8_t lpm_tui_event_handler(struct tb_event *evt, LPM_TUI_Layout *layout, LPM_Packages *pkgs);
void lpm_tui_display(LPM_TUI_Layout *layout, LPM_Packages *pkgs);

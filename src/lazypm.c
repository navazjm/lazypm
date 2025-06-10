//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// lazypm.c - Entry point into the lazypm tui application.
//
// Roadmap:
// - [x] 0.1.0 Display packages
// - [ ] 0.2.0 status messages
// - [ ] 0.3.0 Filter
// - [ ] 0.4.0 Install
// - [ ] 0.5.0 Update
// - [ ] 0.6.0 Remove
// - [ ] 0.7.0 Show depedencies
//   - [ ] Modal to list depedencies
// - [ ] 0.8.0 Show ? (keybindings)
//   - [ ] Modal to list all keybindings
// - [ ] 0.9.0 Error messages.
//   - [ ] Log error messages to a file

#define TB_IMPL

#include "common.h"
#include "layout.h"
#include "termbox2.h"

// forward declarations
void lpm_run(LPM_Layout *layout, LPM_Packages *pkgs);
uint8_t lpm_event_handler(struct tb_event *evt, LPM_Layout *layout, LPM_Packages *pkgs);
void lpm_display(LPM_Layout *layout, LPM_Packages *pkgs);

int main(void)
{
    int error = tb_init();
    if (error)
    {
        fprintf(stderr, "[ERROR] Failed to initialized termbox2 with error code %d\n", error);
        return error;
    }

    LPM_Layout layout = {0};
    lpm_layout_setup(&layout);

    LPM_Packages pkgs = {0};
    error = lpm_packages_get(&pkgs, "xbps-query -Rs ''");

    if (error == LPM_OK)
        lpm_run(&layout, &pkgs);

    tb_shutdown();
    lpm_layout_teardown(&layout);
    lpm_packages_teardown(&pkgs);

    return error;
}

void lpm_run(LPM_Layout *layout, LPM_Packages *pkgs)
{
    int timeout_ms = 50; // how long to wait for an event to be triggered
    while (1)
    {
        lpm_display(layout, pkgs);
        tb_present();

        struct tb_event evt;
        int result = tb_peek_event(&evt, timeout_ms);
        if (result == TB_ERR_NO_EVENT)
            continue;
        if (result == TB_ERR_POLL && tb_last_errno() == EINTR)
            continue; // poll was interrupted, maybe by a SIGWINCH; try again

        if (lpm_event_handler(&evt, layout, pkgs) != LPM_OK)
            break;
    }
}

uint8_t lpm_event_handler(struct tb_event *evt, LPM_Layout *layout, LPM_Packages *pkgs)
{
    switch (evt->type)
    {
    case TB_EVENT_KEY:
        if (evt->key == TB_KEY_ESC || evt->key == TB_KEY_CTRL_C || evt->ch == 'q')
            return LPM_QUIT;

        size_t items_remaining = pkgs->count - layout->packages_page_index * layout->packages_render_capacity;
        size_t items_to_render =
            items_remaining < layout->packages_render_capacity ? items_remaining : layout->packages_render_capacity;

        if (evt->ch == 'H') // go to first page
        {
            layout->packages_page_index = 0;
        }
        else if (evt->ch == 'J') // go to last package of current page
        {
            layout->packages_cursor_ypos = items_to_render - 1;
        }
        else if (evt->ch == 'K') // go to first package of current page
        {
            layout->packages_cursor_ypos = 0;
        }
        else if (evt->ch == 'L') // go to last page
        {
            layout->packages_page_index = layout->packages_total_pages - 1;
        }
        else if (layout->packages_cursor_ypos < items_to_render - 1 &&
                 (evt->key == TB_KEY_ARROW_DOWN || evt->ch == 'j')) // go to next package, with bounds check
        {
            layout->packages_cursor_ypos++;
        }
        else if (layout->packages_cursor_ypos > 0 &&
                 (evt->key == TB_KEY_ARROW_UP || evt->ch == 'k')) // go to previous package, with bounds check
        {
            layout->packages_cursor_ypos--;
        }
        else if (layout->packages_page_index + 1 < layout->packages_total_pages &&
                 (evt->key == TB_KEY_ARROW_RIGHT || evt->ch == 'l')) // go to next page, with bounds check
        {
            layout->packages_page_index++;
        }
        else if (layout->packages_page_index + 1 > 1 &&
                 (evt->key == TB_KEY_ARROW_LEFT || evt->ch == 'h')) // go to previous page, with bounds check
        {
            layout->packages_page_index--;
        }
        break;
    case TB_EVENT_RESIZE:
        break;
    case TB_EVENT_MOUSE:
        break;
    default:
        break;
    }
    return LPM_OK;
}

// currently handles both updating and displaying the state of our app
// TODO: split out into an update and render functions
void lpm_display(LPM_Layout *layout, LPM_Packages *pkgs)
{
    tb_clear();

    //
    // header
    //

    const char *header = "  LAZYPM | (I)nstall | (R)emove | (D)ependencies | (U)pdate all | (F)ilter | ? help";
    tb_printf(layout->header_xpos, layout->header_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR_HIGHLIGHT, header);
    for (int i = layout->min_xpos + strlen(header); i < layout->max_xpos; ++i)
    {
        // highlight remaining cells of the header row
        tb_printf(i, layout->header_ypos, LPM_FG_COLOR, LPM_BG_COLOR_HIGHLIGHT, " ");
    }

    //
    // filter
    //

    layout->filter_xpos = layout->min_xpos;
    layout->filter_ypos = layout->header_ypos + 2;
    tb_printf(layout->filter_xpos, layout->filter_ypos, LPM_FG_COLOR, LPM_BG_COLOR, "Filter: ");
    layout->filter_xpos += strlen("Filter: ");

    //
    // packages
    //

    layout->packages_ypos = layout->filter_ypos + 2;
    uint8_t max_line_len = layout->max_xpos - layout->min_xpos;
    uint8_t longest_package_name_len = 0;
    layout->packages_render_capacity = layout->footer_ypos - layout->packages_ypos - 1;
    layout->packages_total_pages =
        (pkgs->count + layout->packages_render_capacity - 1) / layout->packages_render_capacity;

    size_t items_remaining = pkgs->count - layout->packages_page_index * layout->packages_render_capacity;
    size_t items_to_render =
        items_remaining < layout->packages_render_capacity ? items_remaining : layout->packages_render_capacity;

    if (layout->packages_cursor_ypos >= items_to_render)
        layout->packages_cursor_ypos = items_to_render - 1;

    for (size_t i = 0; i < items_to_render; ++i)
    {
        size_t idx = layout->packages_page_index * layout->packages_render_capacity + i;
        LPM_Package pkg = pkgs->items[idx];
        if (strlen(pkg.name) > longest_package_name_len)
            longest_package_name_len = strlen(pkg.name);
    }
    for (size_t i = 0; i < layout->packages_render_capacity; ++i)
    {
        if (i >= items_to_render)
            break;

        size_t idx = layout->packages_page_index * layout->packages_render_capacity + i;
        char *pkg_line;
        asprintf(&pkg_line, "%s %-*s %s", pkgs->items[idx].status, longest_package_name_len, pkgs->items[idx].name,
                 pkgs->items[idx].description);

        uint8_t pkg_line_len = strlen(pkg_line);
        if (pkg_line_len >= max_line_len)
        {
            pkg_line[max_line_len - 3] = '.';
            pkg_line[max_line_len - 2] = '.';
            pkg_line[max_line_len - 1] = '.';
            pkg_line[max_line_len] = '\0';
        }

        if (i == layout->packages_cursor_ypos)
        {
            tb_printf(layout->packages_xpos, layout->packages_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR_HIGHLIGHT, pkg_line);
            for (int j = layout->packages_xpos + pkg_line_len - 1; j < layout->max_xpos; ++j)
            {
                // just like the header row, highlight remaining cells of the hovered package row
                tb_printf(j, layout->packages_ypos, LPM_FG_COLOR, LPM_BG_COLOR_HIGHLIGHT, " ");
            }
        }
        else
        {
            tb_printf(layout->packages_xpos, layout->packages_ypos, LPM_FG_COLOR, LPM_BG_COLOR, pkg_line);
        }
        LPM_FREE(pkg_line);
        layout->packages_ypos++;
    }

    //
    // footer
    //

    tb_printf(layout->footer_xpos, layout->footer_ypos, LPM_FG_COLOR, LPM_BG_COLOR, "Page %zu of %zu",
              layout->packages_page_index + 1, layout->packages_total_pages);
}

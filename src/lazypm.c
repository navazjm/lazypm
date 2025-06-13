//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// lazypm.c - Entry point into the lazypm tui application.
//

#define TB_IMPL

#include "layout.h"
#include "status.h"

// forward declarations
void lpm_run(LPM_Layout *layout, LPM_Packages *pkgs);
uint8_t lpm_event_handler(struct tb_event *evt, LPM_Layout *layout, LPM_Packages *pkgs);
void lpm_display(LPM_Layout *layout, LPM_Packages *pkgs);

int main(void)
{
    int error = tb_init();
    if (error)
    {
        LPM_LOG_ERROR("Failed to initialized termbox2\n\tReason  : %s\n", tb_strerror(error));
        return error;
    }

    LPM_Layout layout = {0};
    lpm_layout_setup(&layout);

    LPM_Packages pkgs = {0};
    error = lpm_packages_get(&pkgs, NULL);

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

    size_t items_remaining =
        pkgs->count - layout->packages_page_index * layout->packages_render_capacity;
    size_t items_to_render = items_remaining < layout->packages_render_capacity
                                 ? items_remaining
                                 : layout->packages_render_capacity;
    size_t curr_selected_pkg_idx = layout->packages_page_index * layout->packages_render_capacity +
                                   layout->packages_cursor_ypos;

    switch (evt->type)
    {
    case TB_EVENT_KEY:
        if (evt->key == TB_KEY_ESC || evt->key == TB_KEY_CTRL_C || evt->ch == 'q')
            return LPM_QUIT;

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
                 (evt->key == TB_KEY_ARROW_DOWN ||
                  evt->ch == 'j')) // go to next package, with bounds check
        {
            layout->packages_cursor_ypos++;
        }
        else if (layout->packages_cursor_ypos > 0 &&
                 (evt->key == TB_KEY_ARROW_UP ||
                  evt->ch == 'k')) // go to previous package, with bounds check
        {
            layout->packages_cursor_ypos--;
        }
        else if (layout->packages_page_index + 1 < layout->packages_total_pages &&
                 (evt->key == TB_KEY_ARROW_RIGHT ||
                  evt->ch == 'l')) // go to next page, with bounds check
        {
            layout->packages_page_index++;
        }
        else if (layout->packages_page_index + 1 > 1 &&
                 (evt->key == TB_KEY_ARROW_LEFT ||
                  evt->ch == 'h')) // go to previous page, with bounds check
        {
            layout->packages_page_index--;
        }
        else if (evt->key == TB_KEY_ENTER)
        {
            LPM_Package pkg = pkgs->items[curr_selected_pkg_idx];
            char *status_msg;
            if (strcmp(pkg.status, LPM_PACKAGE_STATUS_AVAILABLE) == 0)
                lpm_asprintf(&status_msg, "Installing package '%s'... ", pkg.name);
            else
                lpm_asprintf(&status_msg, "Updating package '%s'... ", pkg.name);
            lpm_status_msg_set_info(status_msg);
            LPM_FREE(status_msg);
            lpm_packages_install(&pkg);
        }
        else if (evt->ch == 'u')
        {
            lpm_status_msg_set_info("Updating all installed packages. This may take a moment...");
            lpm_packages_udate_all();
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

    // temp buffer to concatenate text together for displaying
    char *temp = NULL;
    size_t temp_len = 0;

    //
    // header
    //

    const char *header_text = " LAZYPM ";
    tb_printf(layout->header_xpos, layout->header_ypos, LPM_FG_COLOR_BLACK_DIM,
              LPM_BG_COLOR_HIGHLIGHT, header_text);

    lpm_status_set_position(layout->header_xpos + LPM_STRLEN(header_text) + 1, layout->header_ypos);
    lpm_status_msg_display(false);

    //
    // packages
    //

    layout->packages_ypos = layout->header_ypos + 2;
    uint8_t max_line_len = layout->max_xpos - layout->min_xpos;
    uint8_t longest_package_name_len = 0;
    layout->packages_render_capacity = layout->footer_ypos - layout->packages_ypos - 1;
    layout->packages_total_pages =
        (pkgs->count + layout->packages_render_capacity - 1) / layout->packages_render_capacity;

    size_t items_remaining =
        pkgs->count - layout->packages_page_index * layout->packages_render_capacity;
    size_t items_to_render = items_remaining < layout->packages_render_capacity
                                 ? items_remaining
                                 : layout->packages_render_capacity;

    if (layout->packages_cursor_ypos >= items_to_render)
        layout->packages_cursor_ypos = items_to_render - 1;

    for (size_t i = 0; i < items_to_render; ++i)
    {
        size_t idx = layout->packages_page_index * layout->packages_render_capacity + i;
        LPM_Package pkg = pkgs->items[idx];
        if (LPM_STRLEN(pkg.name) > longest_package_name_len)
            longest_package_name_len = LPM_STRLEN(pkg.name);
    }
    for (size_t i = 0; i < layout->packages_render_capacity; ++i)
    {
        if (i >= items_to_render)
            break;

        size_t idx = layout->packages_page_index * layout->packages_render_capacity + i;
        lpm_asprintf(&temp, "%s %-*s %s", pkgs->items[idx].status, longest_package_name_len,
                     pkgs->items[idx].name, pkgs->items[idx].description);

        temp_len = LPM_STRLEN(temp);
        if (temp_len >= max_line_len)
        {
            temp[max_line_len - 3] = '.';
            temp[max_line_len - 2] = '.';
            temp[max_line_len - 1] = '.';
            temp[max_line_len] = '\0';
        }

        if (i == layout->packages_cursor_ypos)
        {
            tb_printf(layout->packages_xpos, layout->packages_ypos, LPM_FG_COLOR_BLACK_DIM,
                      LPM_BG_COLOR_HIGHLIGHT, temp);
            for (int j = layout->packages_xpos + temp_len - 1; j < layout->max_xpos; ++j)
            {
                // highlight remaining cells of the hovered package row
                tb_set_cell(j, layout->packages_ypos, ' ', LPM_FG_COLOR, LPM_BG_COLOR_HIGHLIGHT);
            }
        }
        else
        {
            tb_printf(layout->packages_xpos, layout->packages_ypos, LPM_FG_COLOR, LPM_BG_COLOR,
                      temp);
        }
        LPM_FREE(temp);
        temp = NULL;
        layout->packages_ypos++;
    }

    //
    // footer
    //

    if (temp)
        LPM_FREE(temp);
    lpm_asprintf(&temp, "Page %zu of %zu (%zu) | ", layout->packages_page_index + 1,
                 layout->packages_total_pages, pkgs->count);
    temp_len = LPM_STRLEN(temp);

    tb_printf(layout->footer_xpos, layout->footer_ypos, LPM_FG_COLOR_BLACK_DIM, LPM_BG_COLOR, temp);
    LPM_FREE(temp);
    temp = NULL;

    tb_printf(layout->footer_xpos + temp_len, layout->footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR,
              "/k");
    temp_len += LPM_STRLEN("/k") - 1;
    tb_printf(layout->footer_xpos + temp_len, layout->footer_ypos, LPM_FG_COLOR_BLACK_DIM,
              LPM_BG_COLOR, "up ");
    temp_len += LPM_STRLEN("up ") - 1;
    tb_printf(layout->footer_xpos + temp_len, layout->footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR,
              "/j");
    temp_len += LPM_STRLEN("/j") - 1;
    tb_printf(layout->footer_xpos + temp_len, layout->footer_ypos, LPM_FG_COLOR_BLACK_DIM,
              LPM_BG_COLOR, "down ");
    temp_len += LPM_STRLEN("down ") - 1;
    tb_printf(layout->footer_xpos + temp_len, layout->footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR,
              "/l");
    temp_len += LPM_STRLEN("/l") - 1;
    tb_printf(layout->footer_xpos + temp_len, layout->footer_ypos, LPM_FG_COLOR_BLACK_DIM,
              LPM_BG_COLOR, "next ");
    temp_len += LPM_STRLEN("next ") - 1;
    tb_printf(layout->footer_xpos + temp_len, layout->footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR,
              "/h");
    temp_len += LPM_STRLEN("/h") - 1;
    tb_printf(layout->footer_xpos + temp_len, layout->footer_ypos, LPM_FG_COLOR_BLACK_DIM,
              LPM_BG_COLOR, "previous");
    temp_len = 0;

    uint8_t footer_ypos = layout->footer_ypos + 2;
    size_t curr_selected_pkg_idx = layout->packages_page_index * layout->packages_render_capacity +
                                   layout->packages_cursor_ypos;

    tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR, "enter");
    temp_len += LPM_STRLEN("enter");
    if (strcmp(pkgs->items[curr_selected_pkg_idx].status, LPM_PACKAGE_STATUS_INSTALLED) == 0)
    {
        tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_BLACK_DIM, LPM_BG_COLOR,
                  " update ");
        temp_len += LPM_STRLEN(" update ") - 1;
        tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR, "x");
        temp_len += LPM_STRLEN("x");
        tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_BLACK_DIM, LPM_BG_COLOR,
                  " uinstall ");
        temp_len += LPM_STRLEN(" uinstall ") - 1;
    }
    else
    {
        // keybindings = "enter install ";
        tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_BLACK_DIM, LPM_BG_COLOR,
                  " install ");
        temp_len += LPM_STRLEN(" install ") - 1;
    }

    tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR, "u");
    temp_len += LPM_STRLEN("u");
    tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_BLACK_DIM, LPM_BG_COLOR,
              " update all ");
    temp_len += LPM_STRLEN(" update all ") - 1;
    tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR, "d");
    temp_len += LPM_STRLEN("d");
    tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_BLACK_DIM, LPM_BG_COLOR,
              " dependencies ");
    temp_len += LPM_STRLEN(" dependencies ") - 1;
    tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR, "/");
    temp_len += LPM_STRLEN("/");
    tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_BLACK_DIM, LPM_BG_COLOR,
              " filter ");
    temp_len += LPM_STRLEN(" filter ") - 1;
    tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR, "q");
    temp_len += LPM_STRLEN("q");
    tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_BLACK_DIM, LPM_BG_COLOR,
              " quit");
    temp_len = 0;
}

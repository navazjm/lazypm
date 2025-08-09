//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// tui.c - Lazypm TUI
//

#include "tui.h"

static LPM_TUI_Mode lpm_tui_mode = LPM_TUI_MODE_MAIN;
#define FILTER_TEXT_MAX_LEN 64
static char filter_text[FILTER_TEXT_MAX_LEN] = {0};
static bool filter_cursor = false;
static uint8_t filter_cursor_pos = 0;
static size_t filter_cursor_render_count = 0;

void lpm_tui_layout_setup(LPM_TUI_Layout *layout)
{
    layout->min_xpos = 5;
    layout->max_xpos = tb_width() - layout->min_xpos;
    layout->min_ypos = 1;
    layout->max_ypos = tb_height() - layout->min_ypos;
    layout->header_xpos = layout->min_xpos;
    layout->header_ypos = layout->min_ypos;
    layout->packages_xpos = layout->min_xpos;
    layout->packages_ypos = 0;
    layout->packages_cursor_ypos = 0;
    layout->packages_page_index = 0;
    layout->packages_total_pages = 1;
    layout->packages_render_capacity = 0;
    layout->footer_xpos = layout->min_xpos;
    layout->footer_ypos = layout->max_ypos - 4;
}

void lpm_tui_layout_teardown(LPM_TUI_Layout *layout)
{
    (void)layout;
}

LPM_Exit_Code lpm_tui_setup(LPM_TUI_Layout *layout, LPM_Packages *pkgs)
{
    int result = lpm_packages_update_xbps();
    if (result != LPM_OK && result != LPM_ERROR_PIPE_CLOSE)
        return result;

    result = tb_init();
    if (result)
    {
        LPM_LOG_ERROR("Failed to initialized termbox2\n\tReason  : %s", tb_strerror(result));
        return LPM_ERROR_TB_INIT;
    }

    if (tb_width() < MIN_WIDTH)
    {
        LPM_LOG_ERROR("Terminal dimensions too small\n\tReason  : Width %d < %d", tb_width(),
                      MIN_WIDTH);
        return LPM_ERROR;
    }
    if (tb_height() < MIN_HEIGHT)
    {
        LPM_LOG_ERROR("Terminal dimensions too small\n\tReason  : Height %d < %d", tb_height(),
                      MIN_HEIGHT);
        return LPM_ERROR;
    }

    lpm_tui_layout_setup(layout);
    result = lpm_packages_get(pkgs, NULL);
    return result;
}

void lpm_tui_teardown(LPM_TUI_Layout *layout, LPM_Packages *pkgs)
{
    tb_shutdown();
    lpm_tui_layout_teardown(layout);
    lpm_packages_teardown(pkgs);
    lpm_log_dump_session();
}

void lpm_tui_run(LPM_TUI_Layout *layout, LPM_Packages *pkgs)
{
    int timeout_ms = 50; // how long to wait for an event to be triggered
    while (1)
    {
        lpm_tui_display(layout, pkgs);
        tb_present();

        struct tb_event evt;
        int result = tb_peek_event(&evt, timeout_ms);
        if (result == TB_ERR_NO_EVENT)
            continue;
        if (result == TB_ERR_POLL && tb_last_errno() == EINTR)
            continue; // poll was interrupted, maybe by a SIGWINCH; try again

        if (lpm_tui_event_handler(&evt, layout, pkgs) != LPM_OK)
            break;
    }
}

LPM_Exit_Code lpm_tui_event_handler(struct tb_event *evt, LPM_TUI_Layout *layout,
                                    LPM_Packages *pkgs)
{
    if (lpm_tui_mode == LPM_TUI_MODE_FILTER)
    {
        size_t filter_text_len = strlen(filter_text);

        switch (evt->type)
        {
        case TB_EVENT_KEY:
            if (evt->key == TB_KEY_ESC || evt->key == TB_KEY_CTRL_C ||
                ((evt->key == TB_KEY_BACKSPACE || evt->key == TB_KEY_BACKSPACE2) &&
                 filter_text_len == 0))
            {
                lpm_tui_mode = LPM_TUI_MODE_MAIN;
            }
            else if (evt->key == TB_KEY_ENTER)
            {
                LPM_Packages new_pkgs = {0};
                uint8_t res = lpm_packages_get(&new_pkgs, filter_text);
                if (res == LPM_OK)
                {
                    // Transfer ownership of heap-allocated contents from new_pkgs to *pkgs
                    lpm_packages_teardown(pkgs);
                    *pkgs = new_pkgs;
                    if (filter_text_len > 0)
                    {
                        char *status_msg;
                        lpm_asprintf(&status_msg, "Showing results for '%s'", filter_text);
                        LPM_STATUS_MSG_SET(status_msg);
                        LPM_FREE(status_msg);
                    }
                    else
                        LPM_STATUS_MSG_SET(NULL);
                }
                else
                {
                    lpm_packages_teardown(&new_pkgs);
                }

                lpm_tui_mode = LPM_TUI_MODE_MAIN;
                layout->packages_cursor_ypos = 0;
            }
            else if (evt->key == TB_KEY_ARROW_LEFT && filter_cursor_pos > 0)
            {
                filter_cursor_pos--;
            }
            else if (evt->key == TB_KEY_ARROW_RIGHT && filter_cursor_pos < filter_text_len)
            {
                filter_cursor_pos++;
            }
            else if (evt->key == TB_KEY_CTRL_A)
            {
                filter_cursor_pos = 0;
            }
            else if (evt->key == TB_KEY_CTRL_E)
            {
                filter_cursor_pos = filter_text_len;
            }
            else if (evt->key == TB_KEY_CTRL_U && filter_cursor_pos > 0)
            {
                size_t tail_len = strlen(filter_text + filter_cursor_pos);
                // Move the tail to the beginning,includes null terminator
                memmove(filter_text, filter_text + filter_cursor_pos, tail_len + 1);
                // Clear the rest of the buffer after new end
                memset(filter_text + tail_len, 0, sizeof(filter_text) - tail_len);
                filter_cursor_pos = 0;
            }
            else if (evt->key == TB_KEY_BACKSPACE || evt->key == TB_KEY_BACKSPACE2)
            {
                memmove(&filter_text[filter_cursor_pos - 1], &filter_text[filter_cursor_pos],
                        filter_text_len - filter_cursor_pos + 1);
                filter_cursor_pos--;
            }
            else if (evt->key == TB_KEY_DELETE)
            {
                memmove(&filter_text[filter_cursor_pos + 1], &filter_text[filter_cursor_pos + 2],
                        filter_text_len - filter_cursor_pos);
            }
            else if (((evt->ch >= 'A' && evt->ch <= 'Z') || // A–Z
                      (evt->ch >= 'a' && evt->ch <= 'z') || // a–z
                      (evt->ch >= '0' && evt->ch <= '9') || // 0–9
                      evt->ch == '-' || evt->ch == '.' || evt->ch == ' ') &&
                     filter_text_len + 1 < FILTER_TEXT_MAX_LEN)
            {
                char c = (char)evt->ch;
                if (filter_cursor_pos != filter_text_len)
                {
                    // Inserting in the middle: shift text right, including null terminator
                    memmove(&filter_text[filter_cursor_pos + 1], &filter_text[filter_cursor_pos],
                            filter_text_len - filter_cursor_pos + 1);
                }
                filter_text[filter_cursor_pos++] = c;
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
    if (lpm_tui_mode == LPM_TUI_MODE_KEYBINDINGS)
    {
        switch (evt->type)
        {
        case TB_EVENT_KEY:
            if (evt->key == TB_KEY_ESC || evt->key == TB_KEY_CTRL_C || evt->ch == 'q')
            {
                lpm_tui_mode = LPM_TUI_MODE_MAIN;
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
        if (evt->key == TB_KEY_ESC || evt->key == TB_KEY_CTRL_C)
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
            LPM_Package *pkg = &pkgs->items[curr_selected_pkg_idx];
            char *status_msg;
            if (strcmp(pkg->status, LPM_PACKAGE_STATUS_AVAILABLE) == 0)
                lpm_asprintf(&status_msg, "Installing package '%s'... ", pkg->name);
            else
                lpm_asprintf(&status_msg, "Updating package '%s'... ", pkg->name);
            LPM_STATUS_MSG_SET_INFO(status_msg);
            LPM_FREE(status_msg);
            lpm_packages_install(pkg);
        }
        else if (evt->ch == 'u')
        {
            LPM_STATUS_MSG_SET_INFO("Updating all installed packages. This may take a moment...");
            lpm_packages_update_all();
        }
        else if (evt->ch == 'x')
        {
            LPM_Package *pkg = &pkgs->items[curr_selected_pkg_idx];
            if (strcmp(pkg->status, LPM_PACKAGE_STATUS_INSTALLED) == 0)
            {
                char *status_msg;
                lpm_asprintf(&status_msg, "Uninstalling package '%s'... ", pkg->name);
                LPM_STATUS_MSG_SET_INFO(status_msg);
                LPM_FREE(status_msg);
                lpm_packages_uninstall(pkg);
            }
        }
        else if (evt->ch == '/')
        {
            lpm_tui_mode = LPM_TUI_MODE_FILTER;
            filter_cursor_render_count = 0;
            filter_cursor_pos = 0;
            memset(filter_text, 0, sizeof(filter_text));
        }
        else if (evt->ch == '?')
        {
            lpm_tui_mode = LPM_TUI_MODE_KEYBINDINGS;
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

void lpm_tui_display(LPM_TUI_Layout *layout, LPM_Packages *pkgs)
{
    tb_clear();

    if (lpm_tui_mode == LPM_TUI_MODE_KEYBINDINGS)
    {
        lpm_tui_display_keybindings_screen(layout);
        return;
    }

    // temp buffer to concatenate text together for displaying
    char *temp = NULL;
    size_t temp_len = 0;

    //
    // header
    //

    char *header_text;
    if (lpm_tui_mode == LPM_TUI_MODE_MAIN)
    {
        header_text = " LAZYPM ";
        tb_printf(layout->header_xpos, layout->header_ypos, LPM_FG_COLOR_BLACK_DIM,
                  LPM_BG_COLOR_HIGHLIGHT, header_text);
        lpm_status_msg_set_position(layout->header_xpos + strlen(header_text) + 1,
                                    layout->header_ypos);
    }
    else if (lpm_tui_mode == LPM_TUI_MODE_FILTER)
    {
        header_text = " FILTER ";
        tb_printf(layout->header_xpos, layout->header_ypos, LPM_FG_COLOR_BLACK_DIM,
                  LPM_BG_COLOR_HIGHLIGHT_FILTER, header_text);
        tb_printf(layout->header_xpos + strlen(header_text) + 1, layout->header_ypos, LPM_FG_COLOR,
                  LPM_BG_COLOR, filter_text);
        if (filter_cursor)
        {
            tb_set_cell(layout->header_xpos + strlen(header_text) + 1 + filter_cursor_pos,
                        layout->header_ypos, ' ', LPM_FG_COLOR_BLACK_DIM,
                        LPM_BG_COLOR_HIGHLIGHT_FILTER);
        }
        filter_cursor = filter_cursor_render_count++ % 20 < 10;
        lpm_status_msg_set_position(layout->header_xpos + strlen(header_text) + 1 +
                                        strlen(filter_text) + 3,
                                    layout->header_ypos);
    }
    else
    {
        LPM_UNREACHABLE("lpm_display set header based on tui mode");
    }

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
        if (strlen(pkg.name) > longest_package_name_len)
            longest_package_name_len = strlen(pkg.name);
    }
    for (size_t i = 0; i < layout->packages_render_capacity; ++i)
    {
        if (i >= items_to_render)
            break;

        size_t idx = layout->packages_page_index * layout->packages_render_capacity + i;
        lpm_asprintf(&temp, "%s %-*s %s", pkgs->items[idx].status, longest_package_name_len,
                     pkgs->items[idx].name, pkgs->items[idx].description);

        temp_len = strlen(temp);
        if (temp_len >= max_line_len)
        {
            temp[max_line_len - 3] = '.';
            temp[max_line_len - 2] = '.';
            temp[max_line_len - 1] = '.';
            temp[max_line_len] = '\0';
        }

        if (lpm_tui_mode == LPM_TUI_MODE_MAIN && i == layout->packages_cursor_ypos)
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
        layout->packages_ypos++;
    }

    //
    // footer
    //

    if (temp)
        LPM_FREE(temp);
    lpm_asprintf(&temp, "Page %zu of %zu (%zu) | ", layout->packages_page_index + 1,
                 layout->packages_total_pages, pkgs->count);
    temp_len = strlen(temp);

    tb_printf(layout->footer_xpos, layout->footer_ypos, LPM_FG_COLOR_BLACK_DIM, LPM_BG_COLOR, temp);
    LPM_FREE(temp);

    tb_printf(layout->footer_xpos + temp_len, layout->footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR,
              "/k");
    temp_len += strlen("/k") - 1;
    tb_printf(layout->footer_xpos + temp_len, layout->footer_ypos, LPM_FG_COLOR_BLACK_DIM,
              LPM_BG_COLOR, "up ");
    temp_len += strlen("up ") - 1;
    tb_printf(layout->footer_xpos + temp_len, layout->footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR,
              "/j");
    temp_len += strlen("/j") - 1;
    tb_printf(layout->footer_xpos + temp_len, layout->footer_ypos, LPM_FG_COLOR_BLACK_DIM,
              LPM_BG_COLOR, "down ");
    temp_len += strlen("down ") - 1;
    tb_printf(layout->footer_xpos + temp_len, layout->footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR,
              "/l");
    temp_len += strlen("/l") - 1;
    tb_printf(layout->footer_xpos + temp_len, layout->footer_ypos, LPM_FG_COLOR_BLACK_DIM,
              LPM_BG_COLOR, "next ");
    temp_len += strlen("next ") - 1;
    tb_printf(layout->footer_xpos + temp_len, layout->footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR,
              "/h");
    temp_len += strlen("/h") - 1;
    tb_printf(layout->footer_xpos + temp_len, layout->footer_ypos, LPM_FG_COLOR_BLACK_DIM,
              LPM_BG_COLOR, "previous");
    temp_len = 0;

    uint8_t footer_ypos = layout->footer_ypos + 2;
    size_t curr_selected_pkg_idx = layout->packages_page_index * layout->packages_render_capacity +
                                   layout->packages_cursor_ypos;

    if (lpm_tui_mode == LPM_TUI_MODE_MAIN)
    {
        tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR,
                  "enter");
        temp_len += strlen("enter");
        if (strcmp(pkgs->items[curr_selected_pkg_idx].status, LPM_PACKAGE_STATUS_INSTALLED) == 0)
        {
            tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_BLACK_DIM,
                      LPM_BG_COLOR, " update ");
            temp_len += strlen(" update ") - 1;
            tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR,
                      "x");
            temp_len += strlen("x");
            tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_BLACK_DIM,
                      LPM_BG_COLOR, " uinstall ");
            temp_len += strlen(" uinstall ") - 1;
        }
        else
        {
            // keybindings = "enter install ";
            tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_BLACK_DIM,
                      LPM_BG_COLOR, " install ");
            temp_len += strlen(" install ") - 1;
        }

        // ---
        tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR, "/");
        temp_len += strlen("/");
        tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_BLACK_DIM, LPM_BG_COLOR,
                  " filter ");
        temp_len += strlen(" filter ") - 1;
        // ---
        tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR, "?");
        temp_len += strlen("?");
        tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_BLACK_DIM, LPM_BG_COLOR,
                  " more ");
        temp_len += strlen(" more ") - 1;
        // ---
        tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR,
                  "esc");
        temp_len += strlen("esc");
        tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_BLACK_DIM, LPM_BG_COLOR,
                  " quit");
        temp_len = 0;
    }
    else if (lpm_tui_mode == LPM_TUI_MODE_FILTER)
    {
        tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR,
                  "enter");
        temp_len += strlen("enter");
        tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_BLACK_DIM, LPM_BG_COLOR,
                  " query ");
        temp_len += strlen(" query ") - 1;
        tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR,
                  "esc");
        temp_len += strlen("esc");
        tb_printf(layout->footer_xpos + temp_len, footer_ypos, LPM_FG_COLOR_BLACK_DIM, LPM_BG_COLOR,
                  " cancel");
    }
    else
    {
        LPM_UNREACHABLE("lpm_display set footer based on tui mode");
    }
}

void lpm_tui_display_keybindings_screen(LPM_TUI_Layout *layout)
{
    // temp buffer to concatenate text together for displaying
    // char *temp = NULL;
    size_t temp_len = 0;

    char *header_text = " KEYBINDINGS ";
    tb_printf(layout->header_xpos, layout->header_ypos, LPM_FG_COLOR_BLACK_DIM,
              LPM_BG_COLOR_HIGHLIGHT_HELP, header_text);
    lpm_status_msg_set_position(layout->header_xpos + strlen(header_text) + 1, layout->header_ypos);

    layout->packages_ypos = layout->header_ypos + 2;

    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR,
              "escape (ctrl + c) : Exit current mode. If in LAZYPM mode, terminate program");

    // Lazypm (Main) Mode keybindings

    size_t longest_keybinding_strlen = strlen("enter");
    layout->packages_ypos++;
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR_BLACK_DIM,
              LPM_BG_COLOR_HIGHLIGHT, " LAZYPM ");
    layout->packages_ypos++;
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "h ()", ": Go to previous page");
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "l ()", ": Go to next page");
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "j ()", ": Go to next package");
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "k ()", ": Go to previous package");
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "H", ": Go to first page of packages");
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "L", ": Go to last page of packages");
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "J", ": Go to first package of current page");
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "K", ": Go to last package of current page");
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "enter", ": install or update selected package");
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "u", ": update all installed packages");
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "x", ": uninstall selected package if installed already");
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "/", ": enter filter mode");
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "?", ": view list of all keybindings");

    // Filter Mode keybindings

    longest_keybinding_strlen = strlen("backspace");
    layout->packages_ypos++;
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR_BLACK_DIM,
              LPM_BG_COLOR_HIGHLIGHT_FILTER, " FILTER ");
    layout->packages_ypos++;
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "enter", ": Query for packages with given input");
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "        ", ": Move cursor left one position");
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "        ", ": Move cursor right one position");
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "ctrl + a", ": Mover cursor to the beginning of input");
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "ctrl + e", ": Mover cursor to the end of input");
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "ctrl + u",
              ": Delete text from cursor position to the beginning of input");
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "backspace", ": Delete character left of cursor position");
    tb_printf(layout->packages_xpos, layout->packages_ypos++, LPM_FG_COLOR, LPM_BG_COLOR, "%-*s %s",
              longest_keybinding_strlen, "delete", ": Delete character right of cursor position");

    // Footer

    tb_printf(layout->footer_xpos + temp_len, layout->footer_ypos, LPM_FG_COLOR_DIM, LPM_BG_COLOR,
              "esc");
    temp_len += strlen("esc");
    tb_printf(layout->footer_xpos + temp_len, layout->footer_ypos, LPM_FG_COLOR_BLACK_DIM,
              LPM_BG_COLOR, " back");
}

void lpm_tui_crash_handler(int sig)
{
    tb_shutdown();
    lpm_log_dump_session();
    signal(sig, SIG_DFL);
    raise(sig);
}

void lpm_tui_crash_signals(void)
{
    signal(SIGSEGV, lpm_tui_crash_handler);
    signal(SIGABRT, lpm_tui_crash_handler);
    signal(SIGFPE, lpm_tui_crash_handler);
    signal(SIGILL, lpm_tui_crash_handler);
    signal(SIGBUS, lpm_tui_crash_handler);
    signal(SIGINT, lpm_tui_crash_handler); // User interruption
    signal(SIGTERM, lpm_tui_crash_handler);
    signal(SIGPIPE, lpm_tui_crash_handler); // Pipe-related (important for popen!)
    signal(SIGCHLD, SIG_DFL);               // Child process signals (may be useful)
}

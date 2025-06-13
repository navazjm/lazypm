//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// status.c - Set and Get LPM Status Messages
//

#include "status.h"
#include "common.h"

static char *_status_msg = NULL;
static struct timeval _status_start;
static LPM_Status_Msg_Type _status_type = LPM_STATUS_MSG_TYPE_INACTIVE;
static int _status_wait_ms = 5000;
static uint8_t _status_xpos = 0;
static uint8_t _status_ypos = 0;

void lpm_status_set_position(uint8_t xpos, uint8_t ypos)
{
    _status_xpos = xpos;
    _status_ypos = ypos;
}

static void _lpm_status_msg_set(LPM_Status_Msg_Type st, const char *msg)
{
    if (_status_msg)
        LPM_FREE(_status_msg);
    _status_msg = lpm_strdup(msg);
    gettimeofday(&_status_start, NULL);
    _status_type = st;
}

static void _lpm_status_msg_update(void)
{
    if (_status_type == LPM_STATUS_MSG_TYPE_INACTIVE)
        return;

    struct timeval now;
    gettimeofday(&now, NULL);

    long elapsed_ms =
        (now.tv_sec - _status_start.tv_sec) * 1000L + (now.tv_usec - _status_start.tv_usec) / 1000L;

    if (elapsed_ms >= _status_wait_ms)
    {
        LPM_FREE(_status_msg);
        _status_msg = NULL;
        _status_type = LPM_STATUS_MSG_TYPE_INACTIVE;
    }
}

static const char *_lpm_status_msg_get(void)
{
    return _status_msg;
}

void lpm_status_msg_display(bool flush)
{
    _lpm_status_msg_update();

    const char *status_msg = _lpm_status_msg_get();
    if (!status_msg)
        return;

    if (flush)
    {
        for (int x = _status_xpos; x < tb_width(); ++x)
        {
            tb_set_cell(x, _status_ypos, ' ', LPM_FG_COLOR, LPM_BG_COLOR);
        }
    }

    uint32_t fg_color = LPM_FG_COLOR; // LPM_Status_Msg_Type_Default
    if (_status_type == LPM_STATUS_MSG_TYPE_SUCCESS)
        fg_color = LPM_FG_COLOR_GREEN;
    if (_status_type == LPM_STATUS_MSG_TYPE_ERROR)
        fg_color = LPM_FG_COLOR_RED;
    if (_status_type == LPM_STATUS_MSG_TYPE_INFO)
        fg_color = LPM_FG_COLOR_BLUE;

    tb_printf(_status_xpos, _status_ypos, fg_color, LPM_BG_COLOR, status_msg);
}

void lpm_status_set_and_flush(LPM_Status_Msg_Type st, const char *msg)
{
    _lpm_status_msg_set(st, msg);
    lpm_status_msg_display(true);
    tb_present();
}

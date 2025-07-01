//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// status.c - Set and Get LPM Status Messages
//

#include "status.h"

#define STATUS_WAIT_TIME_MS 5000

typedef struct
{
    char *msg;
    struct timeval start_time;
    LPM_Status_Msg_Type type;
    uint8_t xpos;
    uint8_t ypos;
} LPM_Status_Msg;

// For now, we have a singleton-like global status. Eventually, would like to
// implement a queue like structure to store/display multiple status messages at
// any given time.
static LPM_Status_Msg _status = {
    .msg = NULL, .type = LPM_STATUS_MSG_TYPE_INACTIVE, .xpos = 0, .ypos = 0};

static void _lpm_status_msg_teardown(void)
{
    LPM_FREE(_status.msg);
    _status.type = LPM_STATUS_MSG_TYPE_INACTIVE;
    _status.xpos = 0;
    _status.ypos = 0;
}

static void _lpm_status_msg_set(LPM_Status_Msg_Type st, const char *msg)
{
    if (!msg)
    {
        _lpm_status_msg_teardown();
        return;
    }

    if (_status.msg)
        LPM_FREE(_status.msg);
    _status.msg = lpm_strdup(msg);
    gettimeofday(&_status.start_time, NULL);
    _status.type = st;
}

static void _lpm_status_msg_update(void)
{
    if (!_status.msg || _status.type == LPM_STATUS_MSG_TYPE_INACTIVE)
        return;

    struct timeval now;
    gettimeofday(&now, NULL);

    long elapsed_ms = (now.tv_sec - _status.start_time.tv_sec) * 1000L +
                      (now.tv_usec - _status.start_time.tv_usec) / 1000L;

    if (elapsed_ms >= STATUS_WAIT_TIME_MS)
    {
        _lpm_status_msg_teardown();
    }
}

void lpm_status_msg_display(bool flush)
{
    _lpm_status_msg_update();

    if (flush)
    {
        for (int x = _status.xpos; x < tb_width(); ++x)
        {
            tb_set_cell(x, _status.ypos, ' ', LPM_FG_COLOR, LPM_BG_COLOR);
        }
    }

    if (!_status.msg || _status.type == LPM_STATUS_MSG_TYPE_INACTIVE)
        return;

    uint32_t fg_color = LPM_FG_COLOR; // LPM_Status_Msg_Type_Default
    if (_status.type == LPM_STATUS_MSG_TYPE_SUCCESS)
        fg_color = LPM_FG_COLOR_GREEN;
    if (_status.type == LPM_STATUS_MSG_TYPE_ERROR)
        fg_color = LPM_FG_COLOR_RED;
    if (_status.type == LPM_STATUS_MSG_TYPE_INFO)
        fg_color = LPM_FG_COLOR_BLUE;

    tb_printf(_status.xpos, _status.ypos, fg_color, LPM_BG_COLOR, _status.msg);
}

void lpm_status_msg_set_and_display(LPM_Status_Msg_Type st, const char *msg)
{
    _lpm_status_msg_set(st, msg);
    lpm_status_msg_display(true);
    tb_present();
}

void lpm_status_msg_set_position(uint8_t xpos, uint8_t ypos)
{
    _status.xpos = xpos;
    _status.ypos = ypos;
}

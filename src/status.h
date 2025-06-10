//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// status.h - Set and Get LPM Status Messages
//

#pragma once

#include <stdint.h>

typedef enum
{
    LPM_STATUS_MSG_TYPE_INACTIVE,
    LPM_STATUS_MSG_TYPE_DEFAULT,
    LPM_STATUS_MSG_TYPE_SUCCESS,
    LPM_STATUS_MSG_TYPE_ERROR,
    LPM_STATUS_MSG_TYPE_INFO,
} LPM_Status_Msg_Type;

void _lpm_status_msg_set(LPM_Status_Msg_Type st, const char *msg);
#define lpm_status_msg_set(msg) _lpm_status_msg_set(LPM_STATUS_MSG_TYPE_DEFAULT, msg)
#define lpm_status_msg_set_success(msg) _lpm_status_msg_set(LPM_STATUS_MSG_TYPE_SUCCESS, msg)
#define lpm_status_msg_set_error(msg) _lpm_status_msg_set(LPM_STATUS_MSG_TYPE_ERROR, msg)
#define lpm_status_msg_set_info(msg) _lpm_status_msg_set(LPM_STATUS_MSG_TYPE_INFO, msg)

void lpm_status_msg_display(uint8_t xpos, uint8_t ypos);

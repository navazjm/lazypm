//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// status.h - Set and Get LPM Status Messages
//

#pragma once

#include "common.h"

typedef enum
{
    LPM_STATUS_MSG_TYPE_INACTIVE,
    LPM_STATUS_MSG_TYPE_DEFAULT,
    LPM_STATUS_MSG_TYPE_SUCCESS,
    LPM_STATUS_MSG_TYPE_ERROR,
    LPM_STATUS_MSG_TYPE_INFO,
} LPM_Status_Msg_Type;

void lpm_status_msg_set_position(uint8_t xpos, uint8_t ypos);
void lpm_status_msg_display(bool flush);

void lpm_status_msg_set_and_display(LPM_Status_Msg_Type st, const char *msg);
#define lpm_status_msg_set(msg) lpm_status_msg_set_and_display(LPM_STATUS_MSG_TYPE_DEFAULT, msg)
#define lpm_status_msg_set_success(msg)                                                            \
    lpm_status_msg_set_and_display(LPM_STATUS_MSG_TYPE_SUCCESS, msg)
#define lpm_status_msg_set_error(msg) lpm_status_msg_set_and_display(LPM_STATUS_MSG_TYPE_ERROR, msg)
#define lpm_status_msg_set_info(msg) lpm_status_msg_set_and_display(LPM_STATUS_MSG_TYPE_INFO, msg)

#ifndef BTSTACK_PROFILE_AVRCP_INTERNAL_H
#define BTSTACK_PROFILE_AVRCP_INTERNAL_H

#include "profiles/btstack_avrcp.h"

uint16_t btstack_avrcp_internal_get_cid(void);
void btstack_avrcp_internal_emit(
    btstack_avrcp_event_type_t type,
    const uint32_t *values,
    uint8_t value_count);

int btstack_avrcp_controller_setup(
    const btstack_avrcp_config_t *config);
void btstack_avrcp_controller_connected(uint16_t avrcp_cid);
void btstack_avrcp_controller_disconnected(void);
void btstack_avrcp_controller_copy_metadata(
    btstack_avrcp_event_t *event);
uint8_t btstack_avrcp_controller_get_play_status(void);
int btstack_avrcp_controller_control(
    btstack_avrcp_control_t control);
int btstack_avrcp_controller_set_repeat(bool enabled);
int btstack_avrcp_controller_set_shuffle(bool enabled);
int btstack_avrcp_controller_request_metadata(void);

int btstack_avrcp_target_setup(
    const btstack_avrcp_config_t *config);
void btstack_avrcp_target_connected(uint16_t avrcp_cid);
void btstack_avrcp_target_disconnected(void);

#endif

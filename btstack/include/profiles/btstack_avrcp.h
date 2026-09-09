#ifndef BTSTACK_PROFILE_AVRCP_H
#define BTSTACK_PROFILE_AVRCP_H

#include <stdbool.h>
#include <stdint.h>

#include "adapter/btstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BTSTACK_AVRCP_TEXT_MAX 64u

typedef enum {
    BTSTACK_AVRCP_STATE_IDLE = 0,
    BTSTACK_AVRCP_STATE_CONNECTING,
    BTSTACK_AVRCP_STATE_CONNECTED
} btstack_avrcp_state_t;

typedef enum {
    BTSTACK_AVRCP_CONTROL_PLAY = 0,
    BTSTACK_AVRCP_CONTROL_PAUSE,
    BTSTACK_AVRCP_CONTROL_STOP,
    BTSTACK_AVRCP_CONTROL_NEXT,
    BTSTACK_AVRCP_CONTROL_PREVIOUS,
    BTSTACK_AVRCP_CONTROL_FAST_FORWARD,
    BTSTACK_AVRCP_CONTROL_REWIND,
    BTSTACK_AVRCP_CONTROL_VOLUME_UP,
    BTSTACK_AVRCP_CONTROL_VOLUME_DOWN
} btstack_avrcp_control_t;

typedef enum {
    BTSTACK_AVRCP_EVENT_DISCONNECTED = 0,
    BTSTACK_AVRCP_EVENT_CONNECTING,
    BTSTACK_AVRCP_EVENT_CONNECTED,
    BTSTACK_AVRCP_EVENT_PLAY_STATUS,
    BTSTACK_AVRCP_EVENT_PLAY_MODE,
    BTSTACK_AVRCP_EVENT_TRACK_CHANGED,
    BTSTACK_AVRCP_EVENT_TRACK_STATUS,
    BTSTACK_AVRCP_EVENT_TRACK_INFO,
    BTSTACK_AVRCP_EVENT_ABSOLUTE_VOLUME
} btstack_avrcp_event_type_t;

typedef struct {
    btstack_avrcp_event_type_t type;
    btstack_bd_addr_t address;
    uint32_t values[3];
    uint8_t value_count;
    char title[BTSTACK_AVRCP_TEXT_MAX];
    char artist[BTSTACK_AVRCP_TEXT_MAX];
    char album[BTSTACK_AVRCP_TEXT_MAX];
} btstack_avrcp_event_t;

typedef void (*btstack_avrcp_event_callback_t)(
    const btstack_avrcp_event_t *event);

typedef struct {
    btstack_avrcp_event_callback_t event_callback;
    const char *controller_service_name;
    const char *target_service_name;
    const char *manufacturer;
    uint32_t controller_sdp_handle;
    uint32_t target_sdp_handle;
} btstack_avrcp_config_t;

int btstack_avrcp_setup(const btstack_avrcp_config_t *config);
int btstack_avrcp_connect(const btstack_bd_addr_t address);
int btstack_avrcp_disconnect(void);
bool btstack_avrcp_is_connected(void);
btstack_avrcp_state_t btstack_avrcp_get_state(void);
uint8_t btstack_avrcp_get_play_status(void);
int btstack_avrcp_control(btstack_avrcp_control_t control);
int btstack_avrcp_execute_control(uint32_t control);
int btstack_avrcp_set_repeat(bool enabled);
int btstack_avrcp_set_shuffle(bool enabled);
int btstack_avrcp_request_metadata(void);
int btstack_avrcp_set_event_callback(
    btstack_avrcp_event_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif

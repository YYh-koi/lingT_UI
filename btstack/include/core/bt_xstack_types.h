#ifndef BT_XSTACK_TYPES_H
#define BT_XSTACK_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BT_XSTACK_ADDRESS_LEN 6u
#define BT_XSTACK_NAME_MAX 64u
#define BT_XSTACK_PIN_MAX 15u
#define BT_XSTACK_TEXT_MAX 64u
#define BT_XSTACK_PAIRED_DEVICE_MAX 8u
#define BT_XSTACK_PROFILE_MASK_SUPPORTED 424u

typedef enum {
    BT_XSTACK_OK = 0,
    BT_XSTACK_INVALID_ARGUMENT = -1,
    BT_XSTACK_NOT_INITIALIZED = -2,
    BT_XSTACK_CONFIGURATION_CONFLICT = -3,
    BT_XSTACK_QUEUE_FULL = -4,
    BT_XSTACK_NOT_READY = -5,
    BT_XSTACK_INTERNAL_ERROR = -6
} bt_xstack_result_t;

typedef struct {
    uint8_t bytes[BT_XSTACK_ADDRESS_LEN];
} bt_xstack_addr_t;

typedef enum {
    BT_XSTACK_UUID_TYPE_16 = 0,
    BT_XSTACK_UUID_TYPE_128
} bt_xstack_uuid_type_t;

typedef struct {
    bt_xstack_uuid_type_t type;
    union {
        uint16_t uuid16;
        uint8_t uuid128[16];
    } value;
} bt_xstack_uuid_t;

typedef enum {
    BT_XSTACK_PROFILE_HFP_HF = 1,
    BT_XSTACK_PROFILE_A2DP_SINK = 2,
    BT_XSTACK_PROFILE_AVRCP = 3
} bt_xstack_profile_t;

typedef enum {
    BT_XSTACK_CONNECTION_DISCONNECTED = 0,
    BT_XSTACK_CONNECTION_CONNECTING,
    BT_XSTACK_CONNECTION_CONNECTED,
    BT_XSTACK_CONNECTION_DISCONNECTING
} bt_xstack_connection_state_t;

typedef enum {
    BT_XSTACK_POWER_OFF = 0,
    BT_XSTACK_POWER_ON = 1
} bt_xstack_power_state_t;

typedef enum {
    BT_XSTACK_HFP_STATE_IDLE = 1,
    BT_XSTACK_HFP_STATE_CONNECTING,
    BT_XSTACK_HFP_STATE_CONNECTED,
    BT_XSTACK_HFP_STATE_OUTGOING,
    BT_XSTACK_HFP_STATE_INCOMING,
    BT_XSTACK_HFP_STATE_ACTIVE,
    BT_XSTACK_HFP_STATE_HELD,
    BT_XSTACK_HFP_STATE_ACTIVE_WITH_INCOMING,
    BT_XSTACK_HFP_STATE_ACTIVE_WITH_HELD,
    BT_XSTACK_HFP_STATE_MULTIPARTY
} bt_xstack_hfp_state_t;

typedef enum {
    BT_XSTACK_A2DP_STATE_IDLE = 1,
    BT_XSTACK_A2DP_STATE_CONNECTING,
    BT_XSTACK_A2DP_STATE_CONNECTED,
    BT_XSTACK_A2DP_STATE_STREAM_OPEN,
    BT_XSTACK_A2DP_STATE_STREAMING
} bt_xstack_a2dp_state_t;

typedef enum {
    BT_XSTACK_AVRCP_STATE_IDLE = 1,
    BT_XSTACK_AVRCP_STATE_CONNECTING,
    BT_XSTACK_AVRCP_STATE_CONNECTED
} bt_xstack_avrcp_state_t;

typedef enum {
    BT_XSTACK_PLAY_STATE_STOPPED = 0,
    BT_XSTACK_PLAY_STATE_PLAYING,
    BT_XSTACK_PLAY_STATE_PAUSED,
    BT_XSTACK_PLAY_STATE_FORWARD_SEEK,
    BT_XSTACK_PLAY_STATE_REVERSE_SEEK,
    BT_XSTACK_PLAY_STATE_UNKNOWN = 0xff
} bt_xstack_play_state_t;

typedef enum {
    BT_XSTACK_HFP_MULTIPARTY_RELEASE_HELD = 0,
    BT_XSTACK_HFP_MULTIPARTY_HOLD_ACTIVE_ACCEPT_OTHER = 1,
    BT_XSTACK_HFP_MULTIPARTY_JOIN_CALLS = 2
} bt_xstack_hfp_multiparty_action_t;

typedef struct {
    bool valid;
    bt_xstack_addr_t address;
    uint32_t profile_mask;
    char name[BT_XSTACK_NAME_MAX];
} bt_xstack_paired_device_t;

typedef struct {
    bt_xstack_addr_t local_address;
    char local_name[BT_XSTACK_NAME_MAX];
    char pin[BT_XSTACK_PIN_MAX + 1u];
    uint32_t class_of_device;
    uint16_t profile_mask;
    uint8_t power_on;
    uint8_t discoverable;
    uint8_t connectable;
    uint8_t scanning;
    uint8_t ssp_mode;
    uint8_t tdl_mode;
    uint8_t microphone_muted;
    uint8_t speaker_volume;
    uint8_t hfp_config;
    uint8_t hfp_state;
    uint8_t hfp_audio_state;
    uint8_t hfp_voice_recognition;
    uint8_t a2dp_state;
    uint8_t a2dp_muted;
    uint8_t avrcp_state;
    uint8_t play_state;
    uint8_t repeat_mode;
    uint8_t shuffle_mode;
    uint8_t absolute_volume;
    bt_xstack_paired_device_t paired_devices[BT_XSTACK_PAIRED_DEVICE_MAX];
} bt_xstack_state_t;

typedef struct {
    uint32_t accepted_requests;
    uint32_t rejected_requests;
    uint32_t stack_commands;
    uint32_t stack_command_failures;
    uint32_t command_slots_in_use;
    uint32_t command_slots_max;
    uint32_t run_loop_ingress_depth;
    uint32_t run_loop_ingress_max;
    uint32_t run_loop_ingress_full;
    uint32_t critical_event_drops;
    uint32_t best_effort_event_drops;
} bt_xstack_diagnostics_t;

#ifdef __cplusplus
}
#endif

#endif

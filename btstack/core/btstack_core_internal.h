#ifndef BTSTACK_CORE_INTERNAL_H
#define BTSTACK_CORE_INTERNAL_H

#include <stdint.h>

#include "../include/core/bt_xstack_types.h"
#include "../include/core/bt_xstack_service.h"
#include "../include/core/bt_xstack_phone.h"
#include "../include/core/bt_xstack_audio.h"
#include "../include/core/bt_xstack_phonebook.h"
#include "../include/core/bt_xstack_le.h"
#include "../include/core/bt_xstack_gatt.h"
#include "../include/core/bt_xstack_cover_art.h"
#include "../include/profiles/btstack_a2dp_sink.h"
#include "../include/profiles/btstack_avrcp.h"
#include "../include/profiles/btstack_hfp_hf.h"
#include "../include/profiles/btstack_pbap_client.h"
#include "../include/profiles/btstack_gatt_client.h"
#include "../include/profiles/btstack_gatt_server.h"
#include "../include/profiles/btstack_bip_client.h"

#define BT_XSTACK_REQUEST_QUEUE_LEN 8u
#define BT_XSTACK_EVENT_QUEUE_LEN 16u
#define BT_XSTACK_CRITICAL_EVENT_QUEUE_LEN 16u
#define BT_XSTACK_BEST_EFFORT_EVENT_QUEUE_LEN 16u
#define BT_XSTACK_API_TASK_STACK_WORDS 2048u
#define BT_XSTACK_EVENT_TASK_STACK_WORDS 1024u
#define BT_XSTACK_COMMAND_SLOT_COUNT BT_XSTACK_REQUEST_QUEUE_LEN
#define BT_XSTACK_BACKEND_BUSY 1
#define BT_XSTACK_PBAP_CONTROL_EVENT_RESERVE 4u
#define BT_XSTACK_API_EVENT_BACKPRESSURE (-2)

typedef enum {
    BT_XSTACK_REQUEST_SET_POWER = 0,
    BT_XSTACK_REQUEST_SET_PROFILE_MASK,
    BT_XSTACK_REQUEST_SET_NAME,
    BT_XSTACK_REQUEST_SET_PIN,
    BT_XSTACK_REQUEST_SET_SSP,
    BT_XSTACK_REQUEST_PAIR_CONFIRM,
    BT_XSTACK_REQUEST_SET_DISCOVERABLE,
    BT_XSTACK_REQUEST_SET_CONNECTABLE,
    BT_XSTACK_REQUEST_SET_COD,
    BT_XSTACK_REQUEST_CLEAR_PEER_INDEX,
    BT_XSTACK_REQUEST_CLEAR_PEER_ADDRESS,
    BT_XSTACK_REQUEST_CLEAR_PEERS,
    BT_XSTACK_REQUEST_SET_TDL,
    BT_XSTACK_REQUEST_SCAN_START,
    BT_XSTACK_REQUEST_SCAN_STOP,
    BT_XSTACK_REQUEST_DISCONNECT_ALL,
    BT_XSTACK_REQUEST_CONNECT,
    BT_XSTACK_REQUEST_CONNECT_LAST,
    BT_XSTACK_REQUEST_DISCONNECT_PROFILE,
    BT_XSTACK_REQUEST_SET_HFP_CONFIG,
    BT_XSTACK_REQUEST_HFP_DIAL,
    BT_XSTACK_REQUEST_HFP_REDIAL,
    BT_XSTACK_REQUEST_HFP_DTMF,
    BT_XSTACK_REQUEST_HFP_CONTROL,
    BT_XSTACK_REQUEST_SET_MICROPHONE_MUTE,
    BT_XSTACK_REQUEST_SET_SPEAKER_VOLUME,
    BT_XSTACK_REQUEST_SET_A2DP_MUTE,
    BT_XSTACK_REQUEST_AVRCP_CONTROL,
    BT_XSTACK_REQUEST_SET_REPEAT,
    BT_XSTACK_REQUEST_SET_SHUFFLE,
    BT_XSTACK_REQUEST_PBAP_SYNC,
    BT_XSTACK_REQUEST_PBAP_CANCEL,
    BT_XSTACK_REQUEST_PBAP_AUTHENTICATE,
    BT_XSTACK_REQUEST_PBAP_EVENT_SPACE_AVAILABLE,
    BT_XSTACK_REQUEST_PBAP_PEER_DISCONNECTED,
    BT_XSTACK_REQUEST_COVER_ART_SET_MODE,
    BT_XSTACK_REQUEST_COVER_ART_GET_IMAGE,
    BT_XSTACK_REQUEST_LE_ADV_CONFIGURE,
    BT_XSTACK_REQUEST_LE_ADV_CONFIGURE_START,
    BT_XSTACK_REQUEST_LE_ADV_START,
    BT_XSTACK_REQUEST_LE_ADV_STOP,
    BT_XSTACK_REQUEST_LE_SCAN_START,
    BT_XSTACK_REQUEST_LE_SCAN_STOP,
    BT_XSTACK_REQUEST_LE_CONNECT,
    BT_XSTACK_REQUEST_LE_CONNECT_CANCEL,
    BT_XSTACK_REQUEST_LE_DISCONNECT,
    BT_XSTACK_REQUEST_LE_PAIR,
    BT_XSTACK_REQUEST_LE_PAIRING_CONFIRM,
    BT_XSTACK_REQUEST_LE_PASSKEY_REPLY,
    BT_XSTACK_REQUEST_LE_BOND_DELETE,
    BT_XSTACK_REQUEST_LE_BOND_DELETE_ALL,
    BT_XSTACK_REQUEST_GATT_SERVER_ADD_SERVICE,
    BT_XSTACK_REQUEST_GATT_SERVER_ADD_CHARACTERISTIC,
    BT_XSTACK_REQUEST_GATT_SERVER_ADD_DESCRIPTOR,
    BT_XSTACK_REQUEST_GATT_SERVER_BUILD,
    BT_XSTACK_REQUEST_GATT_SERVER_SET_VALUE,
    BT_XSTACK_REQUEST_GATT_SERVER_GET_VALUE,
    BT_XSTACK_REQUEST_GATT_SERVER_NOTIFY,
    BT_XSTACK_REQUEST_GATT_SERVER_INDICATE,
    BT_XSTACK_REQUEST_GATT_CLIENT_DISCOVER_SERVICES,
    BT_XSTACK_REQUEST_GATT_CLIENT_DISCOVER_CHARACTERISTICS,
    BT_XSTACK_REQUEST_GATT_CLIENT_DISCOVER_DESCRIPTORS,
    BT_XSTACK_REQUEST_GATT_CLIENT_READ,
    BT_XSTACK_REQUEST_GATT_CLIENT_WRITE,
    BT_XSTACK_REQUEST_GATT_CLIENT_SUBSCRIBE,
    BT_XSTACK_REQUEST_GATT_CLIENT_UNSUBSCRIBE
} bt_xstack_request_id_t;

typedef enum {
    BT_XSTACK_HFP_CONTROL_ANSWER = 0,
    BT_XSTACK_HFP_CONTROL_HANG_UP,
    BT_XSTACK_HFP_CONTROL_MULTIPARTY,
    BT_XSTACK_HFP_CONTROL_TOGGLE_AUDIO,
    BT_XSTACK_HFP_CONTROL_SET_AUDIO,
    BT_XSTACK_HFP_CONTROL_VOICE_RECOGNITION
} btstack_core_hfp_control_t;

typedef enum {
    BT_XSTACK_AVRCP_CONTROL_PLAY_PAUSE = 0,
    BT_XSTACK_AVRCP_CONTROL_PLAY,
    BT_XSTACK_AVRCP_CONTROL_PAUSE,
    BT_XSTACK_AVRCP_CONTROL_STOP,
    BT_XSTACK_AVRCP_CONTROL_NEXT,
    BT_XSTACK_AVRCP_CONTROL_PREVIOUS,
    BT_XSTACK_AVRCP_CONTROL_FAST_FORWARD,
    BT_XSTACK_AVRCP_CONTROL_REWIND,
    BT_XSTACK_AVRCP_CONTROL_VOLUME_UP,
    BT_XSTACK_AVRCP_CONTROL_VOLUME_DOWN
} btstack_core_avrcp_control_t;

typedef struct {
    bt_xstack_request_id_t id;
    uint32_t generation;
    union {
        struct {
    bt_xstack_profile_t profile;
    bt_xstack_addr_t address;
    uint32_t values[3];
    uint8_t value_count;
    uint8_t has_address;
    char text[BT_XSTACK_TEXT_MAX];
        };
        bt_xstack_le_adv_config_t le_adv;
        bt_xstack_le_scan_config_t le_scan;
        struct {
            bt_xstack_addr_t address;
            uint8_t address_type;
        } le_connect;
        struct {
            uint16_t parent_handle;
            bt_xstack_uuid_t uuid;
            bt_xstack_gatt_attribute_config_t config;
        } gatt_registration;
        struct {
            uint16_t handle;
            uint16_t end_handle;
            uint16_t value_len;
            uint8_t flags;
            bt_xstack_uuid_t uuid;
            bt_xstack_gatt_characteristic_t characteristic;
            uint8_t value[BT_XSTACK_GATT_VALUE_MAX];
        } gatt;
        struct {
            bt_xstack_cover_art_mode_t mode;
            bt_xstack_cover_art_image_descriptor_t descriptor;
        } cover_art;
    };
} btstack_core_request_t;

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
} btstack_core_service_snapshot_t;

typedef struct {
    uint8_t config;
    uint8_t state;
    uint8_t audio_state;
    uint8_t voice_recognition;
    uint8_t microphone_muted;
    uint8_t speaker_volume;
} btstack_core_hfp_snapshot_t;

typedef struct {
    uint8_t state;
    uint8_t muted;
} btstack_core_a2dp_snapshot_t;

typedef struct {
    uint8_t state;
    uint8_t play_state;
    uint8_t repeat_mode;
    uint8_t shuffle_mode;
    uint8_t absolute_volume;
} btstack_core_avrcp_snapshot_t;

typedef struct {
    uint32_t stack_commands;
    uint32_t stack_command_failures;
    uint32_t command_slots_in_use;
    uint32_t command_slots_max;
} btstack_core_backend_diagnostics_t;

typedef enum {
    BT_XSTACK_EVENT_DOMAIN_SERVICE = 0,
    BT_XSTACK_EVENT_DOMAIN_CONNECTION,
    BT_XSTACK_EVENT_DOMAIN_PHONE,
    BT_XSTACK_EVENT_DOMAIN_AUDIO,
    BT_XSTACK_EVENT_DOMAIN_PHONEBOOK,
    BT_XSTACK_EVENT_DOMAIN_LE,
    BT_XSTACK_EVENT_DOMAIN_GATT
} btstack_core_event_domain_t;

typedef struct {
    btstack_core_event_domain_t domain;
    union {
        bt_xstack_service_event_t service;
        bt_xstack_connection_event_t connection;
        bt_xstack_phone_event_t phone;
        bt_xstack_audio_event_t audio;
        bt_xstack_phonebook_event_t phonebook;
        bt_xstack_le_event_t le;
        bt_xstack_gatt_event_t gatt;
    } data;
} btstack_core_event_envelope_t;

typedef char bt_xstack_phonebook_event_must_fit_audio[
    (sizeof(bt_xstack_phonebook_event_t) <=
     sizeof(bt_xstack_audio_event_t)) ? 1 : -1];
typedef char bt_xstack_ble_event_must_fit_fixed_ring[
    (sizeof(btstack_core_event_envelope_t) <= BTSTACK_EVENT_DATA_MAX) ? 1 : -1];

int btstack_core_dispatcher_init(void);
int btstack_core_dispatch(
    const btstack_core_request_t *request);
int btstack_core_copy_state(bt_xstack_state_t *state);
void btstack_core_dispatcher_get_diagnostics(
    btstack_core_backend_diagnostics_t *diagnostics);
int btstack_event_mapper_ready(void);
void btstack_core_state_set_value(
    bt_xstack_request_id_t id,
    uint32_t value);
void btstack_profile_manager_state_set_value(
    bt_xstack_request_id_t id,
    uint32_t value);
void btstack_profile_manager_state_write_pbap(uint8_t state);
void btstack_core_state_sync_gap(void);
int btstack_core_state_get_last_peer(bt_xstack_addr_t *address);
void btstack_core_state_set_profile_peer(
    bt_xstack_profile_t profile,
    const bt_xstack_addr_t *address);
int btstack_core_state_get_profile_peer(
    bt_xstack_profile_t profile,
    bt_xstack_addr_t *address);
void btstack_core_state_mark_disconnect(
    bt_xstack_profile_t profile,
    bool requested);
bool btstack_core_state_take_disconnect_request(
    bt_xstack_profile_t profile);
int btstack_core_state_clear_peer_index(uint8_t index);
int btstack_core_state_clear_peer_address(
    const bt_xstack_addr_t *address);
void btstack_core_state_clear_peers(void);
int btstack_core_state_read_service(
    btstack_core_service_snapshot_t *snapshot);
int btstack_core_state_read_hfp(
    btstack_core_hfp_snapshot_t *snapshot);
int btstack_core_state_read_a2dp(
    btstack_core_a2dp_snapshot_t *snapshot);
int btstack_core_state_read_avrcp(
    btstack_core_avrcp_snapshot_t *snapshot);
int btstack_core_state_read_pbap(bt_xstack_phonebook_state_t *state);
void btstack_core_state_write_pbap(bt_xstack_phonebook_state_t state);
int btstack_core_state_get_peer_count(uint8_t *count);
int btstack_core_state_get_peer(
    uint8_t index,
    bt_xstack_paired_device_t *peer);
void btstack_core_event_post_microphone_mute(bool muted);
void btstack_core_event_set_scan_filter(const char *filter);
int btstack_core_event_mapper_init(void);
void btstack_core_event_mapper_post_connection(
    bt_xstack_profile_t profile,
    bt_xstack_connection_state_t state,
    const bt_xstack_addr_t *address,
    bool unexpected_loss);
void btstack_core_event_mapper_hfp_event(
    const btstack_hfp_hf_event_t *event);
uint8_t btstack_core_event_mapper_pbap_event(
    const btstack_pbap_client_event_t *event);
void btstack_core_event_mapper_a2dp_event(
    const btstack_a2dp_sink_event_t *event);
void btstack_core_event_mapper_avrcp_event(
    const btstack_avrcp_event_t *event);
void btstack_core_event_mapper_media_audio(
    const btstack_audio_sample_t *sample);
void btstack_core_event_mapper_call_audio(
    const uint8_t *data,
    uint16_t length);
void btstack_core_event_mapper_cover_art(const btstack_bip_event_t *event);
void btstack_core_event_mapper_gatt_client(const btstack_gatt_client_event_t *event);
void btstack_core_event_mapper_gatt_server(const btstack_gatt_server_event_t *event);
void btstack_core_event_mapper_post_le_state(bt_xstack_le_event_type_t type, int status);
void btstack_core_event_mapper_post_gatt_server_result(uint8_t operation,
    int status, uint16_t handle);
void btstack_core_event_mapper_post_gatt_server_value(uint16_t handle,
    int status, const uint8_t *value, uint16_t value_len);

void btstack_core_command_slot_released(void);
void btstack_core_request_completed(
    const btstack_core_request_t *request,
    int status);
void btstack_core_pbap_event_space_request_completed(void);
int btstack_core_post_event(
    const btstack_core_event_envelope_t *event);
void btstack_frontend_deliver_event(
    const btstack_core_event_envelope_t *event);
void btstack_frontend_deliver_cover_art(const bt_xstack_cover_art_event_t *event);
void btstack_core_media_audio_deliver(const uint8_t *data, uint32_t length);
void btstack_core_call_audio_deliver(const uint8_t *data, uint32_t length);

#endif

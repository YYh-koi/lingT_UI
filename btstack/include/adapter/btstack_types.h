#ifndef BTSTACK_TYPES_H
#define BTSTACK_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "btstack_config.h"

#ifndef BTSTACK_BD_ADDR_LEN
#define BTSTACK_BD_ADDR_LEN 6u
#endif

typedef enum {
    BTSTACK_STATUS_OK = 0,
    BTSTACK_STATUS_ERROR = -1,
    BTSTACK_STATUS_INVALID_ARGUMENT = -2,
    BTSTACK_STATUS_NOT_READY = -3,
    BTSTACK_STATUS_NOT_SUPPORTED = -4,
    BTSTACK_STATUS_NO_MEMORY = -5,
    BTSTACK_STATUS_BUSY = -6,
    BTSTACK_STATUS_TIMEOUT = -7,
    BTSTACK_STATUS_NOT_FOUND = -8,
    BTSTACK_STATUS_SECURITY_ERROR = -9,
    BTSTACK_STATUS_DISCONNECTED = -10
} btstack_status_t;

typedef enum {
    BTSTACK_TRANSPORT_CLASSIC = 0,
    BTSTACK_TRANSPORT_BLE = 1,
    BTSTACK_TRANSPORT_NONE = 0xff
} btstack_transport_t;

typedef enum {
    BTSTACK_EVENT_UNKNOWN = 0,
    BTSTACK_EVENT_INIT,
    BTSTACK_EVENT_IDLE,
    BTSTACK_EVENT_ADVERTISING,
    BTSTACK_EVENT_DIRECTED_ADVERTISING,
    BTSTACK_EVENT_ADVERTISING_TIMEOUT,
    BTSTACK_EVENT_CONNECTED,
    BTSTACK_EVENT_DISCONNECTED,
    BTSTACK_EVENT_CONNECT_FAILED,
    BTSTACK_EVENT_READY,
    BTSTACK_EVENT_RX,
    BTSTACK_EVENT_TX,
    BTSTACK_EVENT_HID_OUTPUT_REPORT,
    BTSTACK_EVENT_HID_FEATURE_REPORT,
    BTSTACK_EVENT_AUDIO_STARTED,
    BTSTACK_EVENT_AUDIO_STOPPED,
    BTSTACK_EVENT_AUDIO_SAMPLE,
    BTSTACK_EVENT_AUDIO_MEDIA_DATA,
    BTSTACK_EVENT_HFP_CALL,
    BTSTACK_EVENT_HFP_AUDIO,
    BTSTACK_EVENT_HFP_CONTROL
} btstack_event_t;

typedef struct {
    uint8_t report_type;
    uint8_t report_id;
    uint16_t report_len;
    const uint8_t *report;
} btstack_hid_report_payload_t;

typedef struct {
    btstack_transport_t transport;
    btstack_event_t event;
    int status;
    const uint8_t *data;
    uint16_t data_len;
    void *context;
} btstack_event_context_t;

typedef uint8_t btstack_bd_addr_t[BTSTACK_BD_ADDR_LEN];
typedef uint8_t btstack_address_type_t;

enum {
    BTSTACK_ADDRESS_TYPE_PUBLIC = 0u,
    BTSTACK_ADDRESS_TYPE_RANDOM = 1u,
    BTSTACK_ADDRESS_TYPE_PUBLIC_IDENTITY = 2u,
    BTSTACK_ADDRESS_TYPE_RANDOM_IDENTITY = 3u,
    BTSTACK_ADDRESS_TYPE_UNKNOWN = 0xffu
};

typedef struct {
    btstack_transport_t transport;
    btstack_address_type_t address_type;
    btstack_bd_addr_t address;
    uint8_t id;
} btstack_property_bond_peer_t;

typedef enum {
    BTSTACK_UUID_TYPE_16 = 0,
    BTSTACK_UUID_TYPE_128
} btstack_uuid_type_t;

typedef struct {
    btstack_uuid_type_t type;
    union {
        uint16_t uuid16;
        /* UUID128 bytes use canonical Bluetooth big-endian display order. */
        uint8_t uuid128[16];
    } value;
} btstack_uuid_t;

typedef enum {
    BTSTACK_AUDIO_FORMAT_UNKNOWN = 0,
    BTSTACK_AUDIO_FORMAT_PCM_S16,
    BTSTACK_AUDIO_FORMAT_SBC,
    BTSTACK_AUDIO_FORMAT_CVSD,
    BTSTACK_AUDIO_FORMAT_MSBC
} btstack_audio_format_t;

typedef struct {
    btstack_audio_format_t format;
    uint32_t sample_rate;
    uint8_t channel_count;
    uint8_t bits_per_sample;
    uint16_t frame_count;
    const uint8_t *data;
    uint16_t data_len;
} btstack_audio_sample_t;

typedef struct {
    btstack_audio_format_t format;
    uint16_t sequence_number;
    uint32_t timestamp;
    bool marker;
    const uint8_t *data;
    uint16_t data_len;
} btstack_media_packet_t;

typedef enum {
    BTSTACK_HFP_CALL_NONE = 0,
    BTSTACK_HFP_CALL_INCOMING,
    BTSTACK_HFP_CALL_OUTGOING,
    BTSTACK_HFP_CALL_ACTIVE,
    BTSTACK_HFP_CALL_HELD,
    BTSTACK_HFP_CALL_ENDED
} btstack_hfp_call_state_t;

typedef enum {
    BTSTACK_HFP_AUDIO_DISCONNECTED = 0,
    BTSTACK_HFP_AUDIO_CONNECTING,
    BTSTACK_HFP_AUDIO_CONNECTED,
    BTSTACK_HFP_AUDIO_DISCONNECTING
} btstack_hfp_audio_state_t;

typedef enum {
    BTSTACK_HFP_CONTROL_NONE = 0,
    BTSTACK_HFP_CONTROL_SERVICE_AVAILABLE,
    BTSTACK_HFP_CONTROL_SERVICE_UNAVAILABLE,
    BTSTACK_HFP_CONTROL_RING,
    BTSTACK_HFP_CONTROL_VOLUME,
    BTSTACK_HFP_CONTROL_NETWORK_OPERATOR,
    BTSTACK_HFP_CONTROL_SIGNAL_STRENGTH,
    BTSTACK_HFP_CONTROL_BATTERY_LEVEL
} btstack_hfp_control_event_t;

typedef struct {
    btstack_hfp_call_state_t call_state;
    btstack_hfp_audio_state_t audio_state;
    btstack_hfp_control_event_t control_event;
    int value;
    const char *text;
} btstack_hfp_event_t;

#endif

#ifndef BTSTACK_AT_INTERNAL_H
#define BTSTACK_AT_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>

#include "at/btstack_at.h"
#include "../core/btstack_core_internal.h"

#define BTSTACK_AT_TEXT_MAX 64u
#define BTSTACK_AT_ADDRESS_LEN 6u
#define BTSTACK_AT_PROFILE_MASK 424u
#define BTSTACK_AT_PEER_MAX 8u

typedef enum {
    BTSTACK_AT_FORM_EXECUTE = 0,
    BTSTACK_AT_FORM_QUERY,
    BTSTACK_AT_FORM_SET
} btstack_at_command_form_t;

typedef enum {
    BTSTACK_AT_COMMAND_AT = 0,
    BTSTACK_AT_COMMAND_PROFILE,
    BTSTACK_AT_COMMAND_VER,
    BTSTACK_AT_COMMAND_BTEN,
    BTSTACK_AT_COMMAND_ADDR,
    BTSTACK_AT_COMMAND_NAME,
    BTSTACK_AT_COMMAND_PIN,
    BTSTACK_AT_COMMAND_SSP,
    BTSTACK_AT_COMMAND_CFM,
    BTSTACK_AT_COMMAND_PAIR,
    BTSTACK_AT_COMMAND_PAGE,
    BTSTACK_AT_COMMAND_COD,
    BTSTACK_AT_COMMAND_PLIST,
    BTSTACK_AT_COMMAND_TDL,
    BTSTACK_AT_COMMAND_SCAN,
    BTSTACK_AT_COMMAND_DSCA,
    BTSTACK_AT_COMMAND_MICMUTE,
    BTSTACK_AT_COMMAND_SPKVOL,
    BTSTACK_AT_COMMAND_HFPCFG,
    BTSTACK_AT_COMMAND_HFPSTAT,
    BTSTACK_AT_COMMAND_HFPCONN,
    BTSTACK_AT_COMMAND_HFPDISC,
    BTSTACK_AT_COMMAND_HFPDIAL,
    BTSTACK_AT_COMMAND_HFPDTMF,
    BTSTACK_AT_COMMAND_HFPANSW,
    BTSTACK_AT_COMMAND_HFPCHUP,
    BTSTACK_AT_COMMAND_HFPMCAL,
    BTSTACK_AT_COMMAND_HFPADTS,
    BTSTACK_AT_COMMAND_HFPVR,
    BTSTACK_AT_COMMAND_A2DPSTAT,
    BTSTACK_AT_COMMAND_A2DPCONN,
    BTSTACK_AT_COMMAND_A2DPDISC,
    BTSTACK_AT_COMMAND_A2DPMUTE,
    BTSTACK_AT_COMMAND_PLAYPAUSE,
    BTSTACK_AT_COMMAND_PLAY,
    BTSTACK_AT_COMMAND_PAUSE,
    BTSTACK_AT_COMMAND_STOP,
    BTSTACK_AT_COMMAND_NEXT,
    BTSTACK_AT_COMMAND_PREV,
    BTSTACK_AT_COMMAND_FORWARD,
    BTSTACK_AT_COMMAND_BACKWARD,
    BTSTACK_AT_COMMAND_REPEAT,
    BTSTACK_AT_COMMAND_SHUFFLE,
    BTSTACK_AT_COMMAND_SABSUP,
    BTSTACK_AT_COMMAND_SABSDOWN,
    BTSTACK_AT_COMMAND_COVERMODE,
    BTSTACK_AT_COMMAND_COVERIMAGE,
    BTSTACK_AT_COMMAND_PBAPSYNC,
    BTSTACK_AT_COMMAND_PBAPCANCEL,
    BTSTACK_AT_COMMAND_PBAPAUTH,
    BTSTACK_AT_COMMAND_PBAPSTAT,
    BTSTACK_AT_COMMAND_LEADV,
    BTSTACK_AT_COMMAND_LESCAN,
    BTSTACK_AT_COMMAND_LECONN,
    BTSTACK_AT_COMMAND_LEDISC,
    BTSTACK_AT_COMMAND_LEPAIR,
    BTSTACK_AT_COMMAND_LECFM,
    BTSTACK_AT_COMMAND_LEPASS,
    BTSTACK_AT_COMMAND_GATTSADD,
    BTSTACK_AT_COMMAND_GATTSC,
    BTSTACK_AT_COMMAND_GATTSBUILD,
    BTSTACK_AT_COMMAND_GATTSVAL,
    BTSTACK_AT_COMMAND_GATTSNTF,
    BTSTACK_AT_COMMAND_GATTSIND,
    BTSTACK_AT_COMMAND_GATTCDISC,
    BTSTACK_AT_COMMAND_GATTCC,
    BTSTACK_AT_COMMAND_GATTCR,
    BTSTACK_AT_COMMAND_GATTCW,
    BTSTACK_AT_COMMAND_GATTCSUB,
    BTSTACK_AT_COMMAND_UNSUPPORTED
} btstack_at_command_id_t;

typedef struct {
    btstack_at_command_id_t id;
    btstack_at_command_form_t form;
    uint32_t values[3];
    uint8_t value_count;
    uint8_t address[BTSTACK_AT_ADDRESS_LEN];
    bool has_address;
    char text[BTSTACK_AT_TEXT_MAX];
    uint8_t text_length;
    char text2[BTSTACK_AT_TEXT_MAX];
    uint8_t text2_length;
    bt_xstack_le_adv_config_t le_adv;
    bt_xstack_le_scan_config_t le_scan;
    bt_xstack_uuid_t uuid;
    uint8_t security;
    uint8_t binary[BT_XSTACK_GATT_VALUE_MAX];
    uint16_t binary_length;
    bt_xstack_cover_art_image_descriptor_t cover_art_descriptor;
} btstack_at_command_t;

typedef struct {
    bool valid;
    uint8_t address[BTSTACK_AT_ADDRESS_LEN];
    uint32_t profile_mask;
    char name[BTSTACK_AT_TEXT_MAX];
} btstack_at_peer_t;

typedef struct {
    uint8_t local_address[BTSTACK_AT_ADDRESS_LEN];
    char local_name[BTSTACK_AT_TEXT_MAX];
    char pin[16];
    uint32_t class_of_device;
    uint16_t profile_mask;
    uint8_t power_on;
    uint8_t discoverable;
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
    uint8_t pbap_state;
    btstack_at_peer_t peers[BTSTACK_AT_PEER_MAX];
} btstack_at_state_t;

typedef struct {
    uint32_t generation;
    btstack_at_command_id_t command_id;
    int success;
    btstack_at_state_t state;
} btstack_at_result_t;

typedef enum {
    BTSTACK_AT_EVENT_DEVSTAT = 0,
    BTSTACK_AT_EVENT_PWRSTAT,
    BTSTACK_AT_EVENT_SCAN_RESULT,
    BTSTACK_AT_EVENT_SCAN_END,
    BTSTACK_AT_EVENT_PAIR_REQUEST,
    BTSTACK_AT_EVENT_PAIRED,
    BTSTACK_AT_EVENT_MICMUTED,
    BTSTACK_AT_EVENT_HFPSTAT,
    BTSTACK_AT_EVENT_HFPDEV,
    BTSTACK_AT_EVENT_HFPAUDIO,
    BTSTACK_AT_EVENT_HFPVR,
    BTSTACK_AT_EVENT_HFPSIG,
    BTSTACK_AT_EVENT_HFPNET,
    BTSTACK_AT_EVENT_HFPROAM,
    BTSTACK_AT_EVENT_HFPBATT,
    BTSTACK_AT_EVENT_HFPMANU,
    BTSTACK_AT_EVENT_HFPNUM,
    BTSTACK_AT_EVENT_HFPIBR,
    BTSTACK_AT_EVENT_HFPRING,
    BTSTACK_AT_EVENT_HFPLOST,
    BTSTACK_AT_EVENT_HFPMTPX,
    BTSTACK_AT_EVENT_A2DPSTAT,
    BTSTACK_AT_EVENT_A2DPDEV,
    BTSTACK_AT_EVENT_A2DPPCMCONFIG,
    BTSTACK_AT_EVENT_AVRCPSTAT,
    BTSTACK_AT_EVENT_A2DPLOST,
    BTSTACK_AT_EVENT_PLAYSTAT,
    BTSTACK_AT_EVENT_PLAYMODE,
    BTSTACK_AT_EVENT_TRACKSTAT,
    BTSTACK_AT_EVENT_TRACKINFO,
    BTSTACK_AT_EVENT_ABSVOL,
    BTSTACK_AT_EVENT_PBAPSTART,
    BTSTACK_AT_EVENT_PBAPCONTACT,
    BTSTACK_AT_EVENT_PBAPCALL,
    BTSTACK_AT_EVENT_PBAPCATEND,
    BTSTACK_AT_EVENT_PBAPDONE,
    BTSTACK_AT_EVENT_PBAPAUTHREQ,
    BTSTACK_AT_EVENT_PBAPERROR,
    BTSTACK_AT_EVENT_LE,
    BTSTACK_AT_EVENT_GATT
} btstack_at_event_id_t;
typedef struct {
    btstack_at_event_id_t id;
    union {
        struct {
            uint32_t values[3];
            uint8_t value_count;
        } numeric;
        struct {
            uint32_t values[3];
            uint8_t value_count;
            uint8_t address[BTSTACK_AT_ADDRESS_LEN];
            char text[BTSTACK_AT_TEXT_MAX];
        } peer;
        struct {
            uint32_t profile_mask;
            int32_t rssi;
            uint32_t class_of_device;
            uint8_t address_type;
            uint8_t address[BTSTACK_AT_ADDRESS_LEN];
            char name[BTSTACK_AT_TEXT_MAX];
        } scan;
        struct {
            char value[BTSTACK_AT_TEXT_MAX];
        } text;
        struct {
            char title[BTSTACK_AT_TEXT_MAX];
            char artist[BTSTACK_AT_TEXT_MAX];
            char album[BTSTACK_AT_TEXT_MAX];
        } metadata;
        bt_xstack_phonebook_event_t phonebook;
        bt_xstack_le_event_t le;
        bt_xstack_gatt_event_t gatt;
    } data;
} btstack_at_event_t;

typedef int (*btstack_at_backend_dispatch_t)(
    const btstack_at_command_t *command,
    uint32_t generation);

typedef enum {
    BTSTACK_AT_GATE_CONTINUE = 0,
    BTSTACK_AT_GATE_HANDLED = 1,
    BTSTACK_AT_GATE_REJECT = -1
} btstack_at_gate_result_t;

typedef int (*btstack_at_command_gate_t)(
    const btstack_at_command_t *command,
    void *context);

typedef void (*btstack_at_pcm_callback_t)(
    const uint8_t *data,
    uint32_t length);

int btstack_at_parse(
    const char *input,
    uint16_t length,
    btstack_at_command_t *command);

int btstack_at_format_simple(
    int success,
    char *output,
    uint16_t capacity,
    uint16_t *output_length);
int btstack_at_format_address_response(
    const char *name,
    const uint8_t address[BTSTACK_AT_ADDRESS_LEN],
    char *output,
    uint16_t capacity,
    uint16_t *output_length);
int btstack_at_format_uint_line(
    const char *name,
    uint32_t value,
    int append_ok,
    char *output,
    uint16_t capacity,
    uint16_t *output_length);
int btstack_at_format_text_response(
    const char *name,
    const char *value,
    char *output,
    uint16_t capacity,
    uint16_t *output_length);
int btstack_at_format_hex_response(
    const char *name,
    uint32_t value,
    uint8_t digits,
    char *output,
    uint16_t capacity,
    uint16_t *output_length);
int btstack_at_format_plist_record(
    uint8_t index,
    uint32_t profile_mask,
    const uint8_t address[BTSTACK_AT_ADDRESS_LEN],
    const char *name,
    char *output,
    uint16_t capacity,
    uint16_t *output_length);
int btstack_at_format_plist_end(
    char *output,
    uint16_t capacity,
    uint16_t *output_length);
int btstack_at_format_track_info(
    const char *title,
    const char *artist,
    const char *album,
    char *output,
    uint16_t capacity,
    uint16_t *output_length);
int btstack_at_format_event(
    const btstack_at_event_t *event,
    char *output,
    uint16_t capacity,
    uint16_t *output_length);

int btstack_at_backend_register(btstack_at_backend_dispatch_t dispatch);
int btstack_at_command_gate_register(
    btstack_at_command_gate_t gate,
    void *context);
int btstack_at_complete(const btstack_at_result_t *result);
int btstack_at_post_event(const btstack_at_event_t *event);
uint32_t btstack_at_event_overflow_count_get(void);
int btstack_at_core_adapter_init(void);
void btstack_at_event_rejected(const btstack_at_event_t *event);
void btstack_at_event_consumed(const btstack_at_event_t *event);
int btstack_at_event_drain_ready(void);
void btstack_at_stack_ingress_space_available(void);
int btstack_at_set_pcm_callback(btstack_at_pcm_callback_t callback);

#endif

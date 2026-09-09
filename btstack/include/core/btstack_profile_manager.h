#ifndef BTSTACK_PROFILE_MANAGER_H
#define BTSTACK_PROFILE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#include "api/bt_xstack_types.h"
#include "adapter/btstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BTSTACK_PROFILE_MANAGER_CLASSIC_HFP_HF      (1u << 0)
#define BTSTACK_PROFILE_MANAGER_CLASSIC_A2DP_SINK   (1u << 1)
#define BTSTACK_PROFILE_MANAGER_CLASSIC_AVRCP       (1u << 2)
#define BTSTACK_PROFILE_MANAGER_CLASSIC_PBAP_CLIENT (1u << 3)
#define BTSTACK_PROFILE_MANAGER_CLASSIC_ALL         0x0fu

#define BTSTACK_PROFILE_MANAGER_LE_GATT_CLIENT (1u << 0)
#define BTSTACK_PROFILE_MANAGER_LE_GATT_SERVER (1u << 1)
#define BTSTACK_PROFILE_MANAGER_LE_HOGP_DEVICE (1u << 2)
#define BTSTACK_PROFILE_MANAGER_LE_ALL         0x07u

typedef struct {
    uint32_t class_of_device;
    uint32_t a2dp_sdp_handle;
    uint32_t hfp_sdp_handle;
    uint32_t avrcp_controller_sdp_handle;
    uint32_t avrcp_target_sdp_handle;
    uint8_t hfp_rfcomm_channel;
    uint8_t hfp_microphone_gain;
    uint8_t hfp_speaker_gain;
} btstack_profile_manager_classic_config_t;

typedef struct {
    uint32_t struct_size;
    const char *device_name;
    const char *manufacturer;
    uint32_t classic_profile_mask;
    uint32_t le_profile_mask;
    btstack_profile_manager_classic_config_t classic;
} btstack_profile_manager_config_t;

typedef struct {
    uint8_t hfp_config;
    uint8_t hfp_state;
    uint8_t hfp_audio_state;
    uint8_t hfp_voice_recognition;
    uint8_t microphone_muted;
    uint8_t speaker_volume;
    uint8_t a2dp_state;
    uint8_t a2dp_muted;
    uint8_t avrcp_state;
    uint8_t play_state;
    uint8_t repeat_mode;
    uint8_t shuffle_mode;
    uint8_t absolute_volume;
    uint8_t pbap_state;
} btstack_profile_manager_classic_state_t;

typedef struct {
    btstack_bd_addr_t peer_address;
    uint16_t connection_handle;
    uint16_t mtu;
    uint8_t address_type;
    uint8_t role;
    bool connected;
    bool encrypted;
    bool bonded;
} btstack_profile_manager_le_state_t;

int btstack_profile_manager_init(
    const btstack_profile_manager_config_t *config);
int btstack_profile_manager_get_classic_state(
    btstack_profile_manager_classic_state_t *state);
int btstack_profile_manager_get_le_state(btstack_profile_manager_le_state_t *state);
void btstack_profile_manager_le_connected(uint16_t connection_handle, uint8_t role,
    uint8_t address_type, const btstack_bd_addr_t address);
void btstack_profile_manager_le_security_changed(bool encrypted, bool bonded);
void btstack_profile_manager_le_disconnected(uint16_t connection_handle);
int btstack_profile_manager_connect(
    bt_xstack_profile_t profile,
    const bt_xstack_addr_t *address);
int btstack_profile_manager_disconnect(bt_xstack_profile_t profile);
int btstack_profile_manager_disconnect_all(void);
bool btstack_profile_manager_hfp_event_delivered(
    uint8_t event_type,
    const uint8_t address[6]);

#ifdef __cplusplus
}
#endif

#endif

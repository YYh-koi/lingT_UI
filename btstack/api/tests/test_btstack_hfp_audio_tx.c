#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "btstack_defines.h"
#include "btstack_event.h"
#include "classic/hfp_hf.h"
#include "profiles/btstack_hfp_hf.h"

static btstack_packet_handler_t profile_handler;
static void (*upstream_direct_audio_handler)(
    const uint8_t *data, uint16_t length);
static btstack_packet_callback_registration_t *ordinary_hci_registration;
static uint8_t outgoing[258];
static unsigned int request_count;
static unsigned int ready_count;
static unsigned int downlink_count;
static uint8_t downlink[48];
static bool can_send;
static unsigned int upstream_direct_registration_count;

uint16_t little_endian_read_16(const uint8_t *buffer, int position)
{
    return (uint16_t)(buffer[position] |
        ((uint16_t)buffer[position + 1] << 8));
}
void reverse_bytes(const uint8_t *source, uint8_t *destination, int length)
{
    int index;
    for (index = 0; index < length; index++) {
        destination[index] = source[length - 1 - index];
    }
}

void btstack_platform_enter_critical(void) {}
void btstack_platform_exit_critical(void) {}

uint8_t hfp_hf_init(uint8_t rfcomm_channel_nr)
{ (void)rfcomm_channel_nr; return ERROR_CODE_SUCCESS; }
void hfp_hf_init_supported_features(uint32_t supported_features)
{ (void)supported_features; }
void hfp_hf_init_codecs(uint8_t codecs_nr, const uint8_t *codecs)
{ (void)codecs_nr; (void)codecs; }
void hfp_hf_register_packet_handler(btstack_packet_handler_t callback)
{ profile_handler = callback; }
uint8_t hfp_hf_set_default_microphone_gain(uint8_t gain)
{ (void)gain; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_set_default_speaker_gain(uint8_t gain)
{ (void)gain; return ERROR_CODE_SUCCESS; }
void hfp_hf_create_sdp_record(uint8_t *service, uint32_t handle,
    int channel, const char *name, uint16_t features,
    int wide_band_speech)
{ (void)service; (void)handle; (void)channel; (void)name;
  (void)features; (void)wide_band_speech; }
uint8_t sdp_register_service(uint8_t *record)
{ (void)record; return ERROR_CODE_SUCCESS; }
void btstack_event_register_hfp_callback(btstack_hfp_event_callback_t callback)
{ (void)callback; }
void btstack_event_register_hfp_audio_sample_callback(
    btstack_audio_sample_callback_t callback)
{ (void)callback; }
void hfp_hf_set_direct_audio_callback(
    void (*callback)(const uint8_t *data, uint16_t length))
{
    upstream_direct_audio_handler = callback;
    upstream_direct_registration_count++;
}
void hci_add_event_handler(
    btstack_packet_callback_registration_t *registration)
{ ordinary_hci_registration = registration; }
bool hci_can_send_sco_packet_now(void)
{ request_count++; return can_send; }
void hci_reserve_packet_buffer(void) {}
uint8_t *hci_get_outgoing_packet_buffer(void) { return outgoing; }
uint8_t hci_send_sco_packet_buffer(int size)
{
    assert(size == 51);
    return ERROR_CODE_SUCCESS;
}

uint8_t hfp_hf_enable_status_update_for_all_ag_indicators(uint16_t handle)
{ (void)handle; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_activate_call_waiting_notification(uint16_t handle)
{ (void)handle; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_activate_calling_line_notification(uint16_t handle)
{ (void)handle; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_query_operator_selection(uint16_t handle)
{ (void)handle; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_query_subscriber_number(uint16_t handle)
{ (void)handle; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_set_speaker_gain(hci_con_handle_t handle, int gain)
{ (void)handle; (void)gain; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_establish_service_level_connection(bd_addr_t address)
{ (void)address; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_release_service_level_connection(hci_con_handle_t handle)
{ (void)handle; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_establish_audio_connection(hci_con_handle_t handle)
{ (void)handle; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_release_audio_connection(hci_con_handle_t handle)
{ (void)handle; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_answer_incoming_call(hci_con_handle_t handle)
{ (void)handle; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_user_busy(hci_con_handle_t handle)
{ (void)handle; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_end_active_and_accept_other(hci_con_handle_t handle)
{ (void)handle; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_swap_calls(hci_con_handle_t handle)
{ (void)handle; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_terminate_call(hci_con_handle_t handle)
{ (void)handle; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_dial_number(hci_con_handle_t handle, char *number)
{ (void)handle; (void)number; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_redial_last_number(hci_con_handle_t handle)
{ (void)handle; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_activate_voice_recognition(hci_con_handle_t handle)
{ (void)handle; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_deactivate_voice_recognition(hci_con_handle_t handle)
{ (void)handle; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_set_microphone_gain(hci_con_handle_t handle, int gain)
{ (void)handle; (void)gain; return ERROR_CODE_SUCCESS; }
uint8_t hfp_hf_send_dtmf_code(hci_con_handle_t handle, char code)
{ (void)handle; (void)code; return ERROR_CODE_SUCCESS; }

static void ready(void) { ready_count++; }

static void downlink_callback(const uint8_t *data, uint16_t length)
{
    assert(length == sizeof(downlink));
    memcpy(downlink, data, length);
    downlink_count++;
}

int main(void)
{
    btstack_hfp_hf_config_t config;
    uint8_t established[21] = {0};
    uint8_t released[7] = {0};
    uint8_t progress[2] = {
        HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS, 0u };
    uint8_t payload[48];
    unsigned int index;

    memset(&config, 0, sizeof(config));
    config.service_name = "HF";
    config.rfcomm_channel = 1u;
    assert(btstack_hfp_hf_setup(&config) == BTSTACK_STATUS_OK);
    assert(profile_handler != NULL);
    assert(upstream_direct_audio_handler != NULL);
    assert(upstream_direct_registration_count == 1u);
    assert(ordinary_hci_registration != NULL);
    assert(ordinary_hci_registration->callback != NULL);
    assert(btstack_hfp_hf_set_audio_tx_ready_callback(ready) ==
        BTSTACK_STATUS_OK);
    assert(btstack_hfp_hf_set_direct_audio_callback(downlink_callback) ==
        BTSTACK_STATUS_OK);

    established[0] = HCI_EVENT_HFP_META;
    established[1] = 19u;
    established[2] = HFP_SUBEVENT_AUDIO_CONNECTION_ESTABLISHED;
    established[5] = ERROR_CODE_SUCCESS;
    established[6] = 0x34u;
    established[7] = 0x02u;
    established[19] = 48u;
    profile_handler(HCI_EVENT_PACKET, 0u, established,
        sizeof(established));
    assert(btstack_hfp_hf_get_audio_tx_frame_length() == 48u);

    for (index = 0u; index < sizeof(payload); index++) {
        payload[index] = (uint8_t)(index + 1u);
    }
    can_send = false;
    assert(btstack_hfp_hf_request_audio_can_send_now() ==
        BTSTACK_STATUS_OK);
    assert(request_count == 1u);
    assert(ready_count == 0u);
    can_send = true;
    ordinary_hci_registration->callback(
        HCI_EVENT_PACKET, 0u, progress, sizeof(progress));
    assert(ready_count == 1u);
    assert(btstack_hfp_hf_send_audio_frame(payload, sizeof(payload)) ==
        BTSTACK_STATUS_OK);
    assert(outgoing[0] == 0x34u);
    assert(outgoing[1] == 0x02u);
    assert(outgoing[2] == 48u);
    assert(memcmp(&outgoing[3], payload, sizeof(payload)) == 0);

    upstream_direct_audio_handler(payload, sizeof(payload));
    assert(downlink_count == 1u);
    assert(memcmp(downlink, payload, sizeof(payload)) == 0);
    assert(btstack_hfp_hf_set_direct_audio_callback(downlink_callback) ==
        BTSTACK_STATUS_OK);
    assert(upstream_direct_registration_count == 1u);

    released[0] = HCI_EVENT_HFP_META;
    released[1] = 5u;
    released[2] = HFP_SUBEVENT_AUDIO_CONNECTION_RELEASED;
    profile_handler(HCI_EVENT_PACKET, 0u, released, sizeof(released));
    assert(btstack_hfp_hf_get_audio_tx_frame_length() == 0u);
    assert(btstack_hfp_hf_request_audio_can_send_now() ==
        BTSTACK_STATUS_NOT_READY);
    return 0;
}

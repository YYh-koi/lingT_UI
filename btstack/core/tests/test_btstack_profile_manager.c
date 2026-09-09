#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "adapter/btstack_types.h"
#include "core/btstack_profile_manager.h"
#include "btstack_core_internal.h"
#include "profiles/btstack_a2dp_sink.h"
#include "profiles/btstack_avrcp.h"
#include "profiles/btstack_avrcp_cover_art.h"
#include "profiles/btstack_hfp_hf.h"
#include "profiles/btstack_pbap_client.h"
#include "profiles/btstack_gatt_client.h"
#include "profiles/btstack_gatt_server.h"

enum failure_stage {
    FAIL_NONE,
    FAIL_PBAP,
    FAIL_A2DP,
    FAIL_AVRCP,
    FAIL_HFP
};

static enum failure_stage failure;
static const char *calls[20];
static unsigned int call_count;
static btstack_hfp_hf_event_callback_t hfp_event_callback;
static btstack_a2dp_sink_event_callback_t a2dp_event_callback;
static btstack_avrcp_event_callback_t avrcp_event_callback;
static btstack_pbap_client_event_callback_t pbap_event_callback;
static unsigned int mapped_hfp_events;
static unsigned int mapped_a2dp_events;
static unsigned int mapped_avrcp_events;
static unsigned int mapped_pbap_events;
static unsigned int connection_events;
static unsigned int avrcp_connect_calls;
static unsigned int hfp_connect_calls;
static unsigned int a2dp_connect_calls;
static unsigned int hfp_disconnect_calls;
static unsigned int a2dp_disconnect_calls;
static unsigned int avrcp_disconnect_calls;
static unsigned int adapter_disconnect_calls;
static bool hfp_connected;
static bool a2dp_connected;
static bool avrcp_connected;
static btstack_bd_addr_t operation_address;
static unsigned int gatt_client_link_count;
static unsigned int gatt_server_link_count;
static void record(const char *name);

int btstack_gatt_client_init(void) { record("gatt-client"); return BTSTACK_STATUS_OK; }
int btstack_gatt_client_set_event_callback(btstack_gatt_client_event_callback_t callback) { assert(callback != NULL); return BTSTACK_STATUS_OK; }
void btstack_gatt_client_connection_changed(uint16_t handle, bool connected) { assert(handle == 0x0040u); (void)connected; gatt_client_link_count++; }
int btstack_gatt_server_init(void) { record("gatt-server"); return BTSTACK_STATUS_OK; }
int btstack_gatt_server_set_event_callback(btstack_gatt_server_event_callback_t callback) { assert(callback != NULL); return BTSTACK_STATUS_OK; }
void btstack_gatt_server_connection_changed(uint16_t handle, bool connected) { assert(handle == 0x0040u); (void)connected; gatt_server_link_count++; }

int btstack_core_state_get_last_peer(bt_xstack_addr_t *address)
{ (void)address; return -1; }
int btstack_core_state_read_hfp(btstack_core_hfp_snapshot_t *snapshot)
{ memset(snapshot, 0, sizeof(*snapshot)); snapshot->config = 3u; return 0; }
int btstack_core_state_read_avrcp(btstack_core_avrcp_snapshot_t *snapshot)
{ memset(snapshot, 0, sizeof(*snapshot)); return 0; }
void btstack_core_event_mapper_post_connection(
    bt_xstack_profile_t profile, bt_xstack_connection_state_t state,
    const bt_xstack_addr_t *address, bool unexpected_loss)
{
    (void)profile; (void)state; (void)address; (void)unexpected_loss;
    connection_events++;
}
void btstack_core_event_mapper_gatt_client(
    const btstack_gatt_client_event_t *event) { (void)event; }
void btstack_core_event_mapper_gatt_server(
    const btstack_gatt_server_event_t *event) { (void)event; }
void btstack_core_event_mapper_cover_art(const btstack_bip_event_t *event) { (void)event; }
void btstack_call_audio_tx_session_changed(bool connected, uint16_t length)
{ (void)connected; (void)length; }
const uint8_t *btstack_hfp_hf_get_peer_address(void) { return NULL; }
uint16_t btstack_hfp_hf_get_audio_tx_frame_length(void) { return 60u; }
int btstack_hfp_hf_control(btstack_hfp_hf_control_t control)
{ (void)control; return BTSTACK_STATUS_OK; }
btstack_pbap_client_state_t btstack_pbap_client_get_state(void)
{ return BTSTACK_PBAP_CLIENT_STATE_IDLE; }
bool btstack_hfp_hf_is_connected(void) { return hfp_connected; }
bool btstack_a2dp_sink_is_connected(void) { return a2dp_connected; }
bool btstack_avrcp_is_connected(void) { return avrcp_connected; }
btstack_avrcp_state_t btstack_avrcp_get_state(void)
{ return BTSTACK_AVRCP_STATE_IDLE; }
int btstack_hfp_hf_connect(const btstack_bd_addr_t address)
{
    memcpy(operation_address, address, sizeof(operation_address));
    hfp_connect_calls++;
    return BTSTACK_STATUS_OK;
}
int btstack_a2dp_sink_connect(const btstack_bd_addr_t address)
{
    memcpy(operation_address, address, sizeof(operation_address));
    a2dp_connect_calls++;
    return BTSTACK_STATUS_OK;
}
int btstack_avrcp_connect(const btstack_bd_addr_t address)
{
    memcpy(operation_address, address, sizeof(operation_address));
    avrcp_connect_calls++;
    return BTSTACK_STATUS_OK;
}
int btstack_hfp_hf_disconnect(void)
{ hfp_disconnect_calls++; return BTSTACK_STATUS_OK; }
int btstack_a2dp_sink_disconnect(void)
{ a2dp_disconnect_calls++; return BTSTACK_STATUS_OK; }
int btstack_avrcp_disconnect(void)
{ avrcp_disconnect_calls++; return BTSTACK_STATUS_OK; }
int btstack_adapter_disconnect(void)
{ adapter_disconnect_calls++; return BTSTACK_STATUS_OK; }
int btstack_avrcp_request_metadata(void) { return BTSTACK_STATUS_OK; }

void btstack_core_event_mapper_hfp_event(
    const btstack_hfp_hf_event_t *event)
{ assert(event != NULL); mapped_hfp_events++; }
uint8_t btstack_core_event_mapper_pbap_event(
    const btstack_pbap_client_event_t *event)
{ assert(event != NULL); mapped_pbap_events++; return 1u; }
void btstack_core_event_mapper_a2dp_event(
    const btstack_a2dp_sink_event_t *event)
{ assert(event != NULL); mapped_a2dp_events++; }
void btstack_core_event_mapper_avrcp_event(
    const btstack_avrcp_event_t *event)
{ assert(event != NULL); mapped_avrcp_events++; }
void btstack_core_event_mapper_media_audio(
    const btstack_audio_sample_t *sample) { (void)sample; }
void btstack_core_event_mapper_call_audio(
    const uint8_t *data, uint16_t length) { (void)data; (void)length; }
void btstack_call_audio_tx_init(void) {}

static void record(const char *name)
{
    assert(call_count < (sizeof(calls) / sizeof(calls[0])));
    calls[call_count++] = name;
}

void btstack_platform_enter_critical(void) {}
void btstack_platform_exit_critical(void) {}

void sdp_init(void) { record("sdp"); }
void rfcomm_init(void) { record("rfcomm"); }

int btstack_pbap_client_setup(void)
{
    record("pbap");
    return failure == FAIL_PBAP ? BTSTACK_STATUS_ERROR : BTSTACK_STATUS_OK;
}

int btstack_pbap_client_set_event_callback(
    btstack_pbap_client_event_callback_t callback)
{
    assert(callback != NULL);
    pbap_event_callback = callback;
    record("pbap-event");
    return BTSTACK_STATUS_OK;
}

int btstack_a2dp_sink_setup(const btstack_a2dp_sink_config_t *config)
{
    assert(config != NULL);
    assert(strcmp(config->service_name, "A2DP Sink") == 0);
    assert(strcmp(config->manufacturer, "Generalplus") == 0);
    assert(config->sdp_handle == 0x10001u);
    assert(config->event_callback != NULL);
    assert(config->audio_sample_callback != NULL);
    a2dp_event_callback = config->event_callback;
    record("a2dp");
    return failure == FAIL_A2DP ? BTSTACK_STATUS_ERROR : BTSTACK_STATUS_OK;
}

int btstack_avrcp_setup(const btstack_avrcp_config_t *config)
{
    assert(config != NULL);
    assert(strcmp(config->controller_service_name, "AVRCP Controller") == 0);
    assert(strcmp(config->target_service_name, "AVRCP Target") == 0);
    assert(strcmp(config->manufacturer, "Generalplus") == 0);
    assert(config->controller_sdp_handle == 0x10003u);
    assert(config->target_sdp_handle == 0x10004u);
    assert(config->event_callback != NULL);
    avrcp_event_callback = config->event_callback;
    record("avrcp");
    return failure == FAIL_AVRCP ? BTSTACK_STATUS_ERROR : BTSTACK_STATUS_OK;
}

int btstack_avrcp_cover_art_setup(btstack_bip_event_callback_t callback)
{
    assert(callback != NULL);
    record("cover-art");
    return BTSTACK_STATUS_OK;
}

int btstack_hfp_hf_setup(const btstack_hfp_hf_config_t *config)
{
    assert(config != NULL);
    assert(strcmp(config->service_name, "Hands-Free") == 0);
    assert(strcmp(config->manufacturer, "Generalplus") == 0);
    assert(config->sdp_handle == 0x10002u);
    assert(config->rfcomm_channel == 2u);
    assert(config->microphone_gain == 9u);
    assert(config->speaker_gain == 11u);
    assert(config->event_callback != NULL);
    hfp_event_callback = config->event_callback;
    record("hfp");
    return failure == FAIL_HFP ? BTSTACK_STATUS_ERROR : BTSTACK_STATUS_OK;
}

int btstack_hfp_hf_set_direct_audio_callback(
    btstack_hfp_direct_audio_callback_t callback)
{
    assert(callback != NULL);
    record("hfp-audio");
    return BTSTACK_STATUS_OK;
}

int btstack_adapter_set_name(const char *name)
{ assert(strcmp(name, "XStack") == 0); record("name"); return 0; }
int btstack_adapter_set_class_of_device(uint32_t value)
{ assert(value == 0x240404u); record("cod"); return 0; }
int btstack_adapter_set_ssp_mode(uint8_t value)
{ assert(value == 2u); record("ssp"); return 0; }
int btstack_adapter_set_connectable(bool value)
{ assert(value); record("connectable"); return 0; }
int btstack_adapter_set_discoverable(bool value)
{ assert(value); record("discoverable"); return 0; }

static btstack_profile_manager_config_t valid_config(void)
{
    btstack_profile_manager_config_t config;
    memset(&config, 0, sizeof(config));
    config.struct_size = sizeof(config);
    config.device_name = "XStack";
    config.manufacturer = "Generalplus";
    config.classic_profile_mask = BTSTACK_PROFILE_MANAGER_CLASSIC_ALL;
    config.classic.class_of_device = 0x240404u;
    config.classic.a2dp_sdp_handle = 0x10001u;
    config.classic.hfp_sdp_handle = 0x10002u;
    config.classic.avrcp_controller_sdp_handle = 0x10003u;
    config.classic.avrcp_target_sdp_handle = 0x10004u;
    config.classic.hfp_rfcomm_channel = 2u;
    config.classic.hfp_microphone_gain = 9u;
    config.classic.hfp_speaker_gain = 11u;
    return config;
}

static void assert_prefix(const char *const *expected, unsigned int count)
{
    unsigned int index;
    assert(call_count == count);
    for (index = 0u; index < count; index++) {
        assert(strcmp(calls[index], expected[index]) == 0);
    }
}

int main(int argc, char **argv)
{
    btstack_profile_manager_config_t config = valid_config();
    static const char *const success_order[] = {
        "sdp", "rfcomm", "pbap", "pbap-event",
        "a2dp", "avrcp", "cover-art", "hfp", "hfp-audio",
        "name", "cod", "ssp", "connectable", "discoverable"
    };

    assert(argc == 2);
    if (strcmp(argv[1], "null") == 0) {
        assert(btstack_profile_manager_init(NULL) ==
            BTSTACK_STATUS_INVALID_ARGUMENT);
        assert(call_count == 0u);
    } else if (strcmp(argv[1], "short") == 0) {
        config.struct_size--;
        assert(btstack_profile_manager_init(&config) ==
            BTSTACK_STATUS_INVALID_ARGUMENT);
        assert(call_count == 0u);
    } else if (strcmp(argv[1], "le") == 0) {
        btstack_profile_manager_le_state_t state;
        btstack_bd_addr_t peer = { 1u, 2u, 3u, 4u, 5u, 6u };
        config.classic_profile_mask = 0u;
        config.le_profile_mask = BTSTACK_PROFILE_MANAGER_LE_GATT_CLIENT |
            BTSTACK_PROFILE_MANAGER_LE_GATT_SERVER;
        assert(btstack_profile_manager_init(&config) == BTSTACK_STATUS_OK);
        assert(call_count == 2u);
        assert(strcmp(calls[0], "gatt-client") == 0);
        assert(strcmp(calls[1], "gatt-server") == 0);
        btstack_profile_manager_le_connected(0x0040u, 1u, 0u, peer);
        btstack_profile_manager_le_security_changed(true, true);
        assert(btstack_profile_manager_get_le_state(&state) == BTSTACK_STATUS_OK);
        assert(state.connected && state.encrypted && state.bonded);
        assert(state.connection_handle == 0x0040u);
        btstack_profile_manager_le_disconnected(0x0040u);
        assert(btstack_profile_manager_get_le_state(&state) == BTSTACK_STATUS_OK);
        assert(!state.connected && state.connection_handle == 0xffffu);
        assert(gatt_client_link_count == 2u && gatt_server_link_count == 2u);
    } else if (strcmp(argv[1], "le-invalid") == 0) {
        config.classic_profile_mask = 0u;
        config.le_profile_mask = 0x80u;
        assert(btstack_profile_manager_init(&config) == BTSTACK_STATUS_INVALID_ARGUMENT);
        config.le_profile_mask = BTSTACK_PROFILE_MANAGER_LE_HOGP_DEVICE;
        assert(btstack_profile_manager_init(&config) == BTSTACK_STATUS_NOT_SUPPORTED);
    } else if (strcmp(argv[1], "success") == 0) {
        assert(btstack_profile_manager_init(&config) == BTSTACK_STATUS_OK);
        assert(hfp_event_callback != NULL);
        assert(a2dp_event_callback != NULL);
        assert(avrcp_event_callback != NULL);
        assert(pbap_event_callback != NULL);
        assert_prefix(success_order,
            (unsigned int)(sizeof(success_order) / sizeof(success_order[0])));
    } else if (strcmp(argv[1], "idempotent") == 0) {
        assert(btstack_profile_manager_init(&config) == BTSTACK_STATUS_OK);
        assert(btstack_profile_manager_init(&config) == BTSTACK_STATUS_OK);
        assert_prefix(success_order,
            (unsigned int)(sizeof(success_order) / sizeof(success_order[0])));
        config.classic.hfp_rfcomm_channel++;
        assert(btstack_profile_manager_init(&config) ==
            BTSTACK_STATUS_INVALID_ARGUMENT);
    } else if (strcmp(argv[1], "events") == 0) {
        btstack_profile_manager_classic_state_t state;
        btstack_hfp_hf_event_t hfp_event;
        btstack_a2dp_sink_event_t a2dp_event;
        btstack_avrcp_event_t avrcp_event;
        btstack_pbap_client_event_t pbap_event;

        assert(btstack_profile_manager_init(&config) == BTSTACK_STATUS_OK);
        memset(&hfp_event, 0, sizeof(hfp_event));
        hfp_event.type = BTSTACK_HFP_HF_EVENT_CONNECTED;
        hfp_event.address[0] = 1u;
        hfp_event_callback(&hfp_event);
        hfp_event.type = BTSTACK_HFP_HF_EVENT_CALL_IDLE;
        hfp_event_callback(&hfp_event);

        memset(&a2dp_event, 0, sizeof(a2dp_event));
        a2dp_event.type = BTSTACK_A2DP_SINK_EVENT_CONNECTED;
        a2dp_event.address[0] = 2u;
        a2dp_event_callback(&a2dp_event);

        memset(&avrcp_event, 0, sizeof(avrcp_event));
        avrcp_event.type = BTSTACK_AVRCP_EVENT_CONNECTED;
        avrcp_event_callback(&avrcp_event);
        memset(&avrcp_event, 0, sizeof(avrcp_event));
        avrcp_event.type = BTSTACK_AVRCP_EVENT_PLAY_STATUS;
        avrcp_event.values[0] = 1u;
        avrcp_event.value_count = 1u;
        avrcp_event_callback(&avrcp_event);
        memset(&pbap_event, 0, sizeof(pbap_event));
        pbap_event.type = BTSTACK_PBAP_CLIENT_EVENT_PULLING;
        assert(pbap_event_callback(&pbap_event) == 1u);

        assert(btstack_profile_manager_get_classic_state(&state) ==
            BTSTACK_STATUS_OK);
        assert(state.hfp_state == BTSTACK_HFP_HF_STATE_CONNECTED);
        assert(state.a2dp_state == BT_XSTACK_A2DP_STATE_CONNECTED);
        assert(state.avrcp_state == BT_XSTACK_AVRCP_STATE_CONNECTED);
        assert(state.play_state == 1u);
        assert(state.pbap_state == BTSTACK_PBAP_CLIENT_STATE_PULLING);
        assert(mapped_hfp_events == 2u);
        assert(mapped_a2dp_events == 1u);
        assert(mapped_avrcp_events == 2u);
        assert(mapped_pbap_events == 0u);
        assert(connection_events == 4u);
        assert(avrcp_connect_calls == 1u);
        assert(btstack_profile_manager_hfp_event_delivered(
            BTSTACK_HFP_HF_EVENT_CONNECTION_LOST, hfp_event.address));
        assert(!btstack_profile_manager_hfp_event_delivered(
            BTSTACK_HFP_HF_EVENT_RING, hfp_event.address));
    } else if (strcmp(argv[1], "operations") == 0) {
        bt_xstack_addr_t address = {{1u, 2u, 3u, 4u, 5u, 6u}};

        assert(btstack_profile_manager_init(&config) == BTSTACK_STATUS_OK);
        assert(btstack_profile_manager_connect(
            BT_XSTACK_PROFILE_HFP_HF, &address) == BTSTACK_STATUS_OK);
        assert(hfp_connect_calls == 1u);
        assert(memcmp(operation_address, address.bytes,
            sizeof(operation_address)) == 0);
        assert(btstack_profile_manager_connect(
            BT_XSTACK_PROFILE_HFP_HF, NULL) == BTSTACK_STATUS_OK);
        assert(hfp_connect_calls == 2u);

        assert(btstack_profile_manager_connect(
            BT_XSTACK_PROFILE_A2DP_SINK, &address) == BTSTACK_STATUS_OK);
        assert(a2dp_connect_calls == 1u);
        assert(btstack_profile_manager_connect(
            BT_XSTACK_PROFILE_AVRCP, &address) == BTSTACK_STATUS_OK);
        assert(avrcp_connect_calls == 1u);

        hfp_connected = true;
        a2dp_connected = true;
        avrcp_connected = true;
        assert(btstack_profile_manager_disconnect_all() ==
            BTSTACK_STATUS_OK);
        assert(hfp_disconnect_calls == 1u);
        assert(a2dp_disconnect_calls == 1u);
        assert(avrcp_disconnect_calls == 1u);
        assert(adapter_disconnect_calls == 1u);
        assert(btstack_profile_manager_disconnect(
            (bt_xstack_profile_t)99) == BTSTACK_STATUS_INVALID_ARGUMENT);
    } else if (strcmp(argv[1], "linkage-disabled") == 0) {
        uint8_t address[6] = {1u, 0u, 0u, 0u, 0u, 0u};
        bt_xstack_addr_t peer = {{1u, 0u, 0u, 0u, 0u, 0u}};
        config.classic_profile_mask =
            BTSTACK_PROFILE_MANAGER_CLASSIC_HFP_HF;
        assert(btstack_profile_manager_init(&config) == BTSTACK_STATUS_OK);
        assert(btstack_profile_manager_connect(
            BT_XSTACK_PROFILE_A2DP_SINK, &peer) ==
            BTSTACK_STATUS_NOT_SUPPORTED);
        assert(a2dp_connect_calls == 0u);
        assert(!btstack_profile_manager_hfp_event_delivered(
            BTSTACK_HFP_HF_EVENT_CONNECTION_LOST, address));
    } else if (strcmp(argv[1], "pbap-fail") == 0) {
        static const char *const expected[] = { "sdp", "rfcomm", "pbap" };
        failure = FAIL_PBAP;
        assert(btstack_profile_manager_init(&config) == BTSTACK_STATUS_ERROR);
        assert_prefix(expected, 3u);
        assert(btstack_profile_manager_init(&config) ==
            BTSTACK_STATUS_NOT_READY);
    } else if (strcmp(argv[1], "a2dp-fail") == 0) {
        static const char *const expected[] = {
            "sdp", "rfcomm", "pbap", "pbap-event", "a2dp"
        };
        failure = FAIL_A2DP;
        assert(btstack_profile_manager_init(&config) == BTSTACK_STATUS_ERROR);
        assert_prefix(expected, 5u);
    } else if (strcmp(argv[1], "avrcp-fail") == 0) {
        static const char *const expected[] = {
            "sdp", "rfcomm", "pbap", "pbap-event", "a2dp", "avrcp"
        };
        failure = FAIL_AVRCP;
        assert(btstack_profile_manager_init(&config) == BTSTACK_STATUS_ERROR);
        assert_prefix(expected, 6u);
    } else if (strcmp(argv[1], "hfp-fail") == 0) {
        static const char *const expected[] = {
            "sdp", "rfcomm", "pbap", "pbap-event", "a2dp", "avrcp",
            "cover-art", "hfp"
        };
        failure = FAIL_HFP;
        assert(btstack_profile_manager_init(&config) == BTSTACK_STATUS_ERROR);
        assert_prefix(expected, 8u);
    } else {
        assert(!"unknown scenario");
    }
    return 0;
}

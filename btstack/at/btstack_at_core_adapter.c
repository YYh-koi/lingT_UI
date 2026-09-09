#include "btstack_at_internal.h"

#include <string.h>

#include "../core/btstack_runtime.h"
#include "core/btstack_profile_manager.h"
#include "port/btstack_platform.h"

static btstack_at_command_id_t btstack_at_pending_command;
static uint32_t btstack_at_pending_generation;
static uint8_t btstack_at_pbap_space_pending;
static uint8_t btstack_at_pbap_space_blocked;
static btstack_at_pcm_callback_t btstack_at_pcm_callback;
/* Event drain holds later LOST records in its existing ring until this
 * one cleanup has either been accepted and completed or can be retried. */
static uint8_t btstack_at_pbap_peer_disconnect_pending;
static uint8_t btstack_at_pbap_peer_disconnect_inflight;
static uint8_t btstack_at_pbap_peer_disconnect_dispatching;
static bt_xstack_addr_t btstack_at_pbap_peer_disconnect_address;

static void btstack_at_retry_pbap_peer_disconnect(void)
{
    btstack_core_request_t request;
    int dispatch = 0;
    int status;

    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_PBAP_PEER_DISCONNECTED;
    request.has_address = 1u;
    btstack_platform_enter_critical();
    if (btstack_at_pbap_peer_disconnect_pending &&
        !btstack_at_pbap_peer_disconnect_inflight &&
        !btstack_at_pbap_peer_disconnect_dispatching) {
        btstack_at_pbap_peer_disconnect_inflight = 1u;
        btstack_at_pbap_peer_disconnect_dispatching = 1u;
        request.address = btstack_at_pbap_peer_disconnect_address;
        dispatch = 1;
    }
    btstack_platform_exit_critical();
    if (!dispatch) return;

    status = btstack_core_dispatch(&request);
    btstack_platform_enter_critical();
    btstack_at_pbap_peer_disconnect_dispatching = 0u;
    if (status == BT_XSTACK_BACKEND_BUSY) {
        btstack_at_pbap_peer_disconnect_inflight = 0u;
    } else {
        btstack_at_pbap_peer_disconnect_pending = 0u;
    }
    btstack_platform_exit_critical();
}

static void btstack_at_queue_pbap_peer_disconnect(
    const uint8_t address[6])
{
    int queued = 0;

    btstack_platform_enter_critical();
    if (!btstack_at_pbap_peer_disconnect_pending &&
        !btstack_at_pbap_peer_disconnect_inflight &&
        !btstack_at_pbap_peer_disconnect_dispatching) {
        memcpy(btstack_at_pbap_peer_disconnect_address.bytes,
            address, 6u);
        btstack_at_pbap_peer_disconnect_pending = 1u;
        queued = 1;
    }
    btstack_platform_exit_critical();
    if (queued) btstack_at_retry_pbap_peer_disconnect();
}

int btstack_at_event_drain_ready(void)
{
    int ready;

    btstack_platform_enter_critical();
    ready = !btstack_at_pbap_peer_disconnect_pending &&
        !btstack_at_pbap_peer_disconnect_inflight &&
        !btstack_at_pbap_peer_disconnect_dispatching;
    btstack_platform_exit_critical();
    return ready;
}

void btstack_at_stack_ingress_space_available(void)
{
    btstack_at_retry_pbap_peer_disconnect();
}

static void btstack_at_copy_text(char *destination, uint16_t capacity,
    const char *source)
{
    uint16_t index = 0u;
    if ((destination == NULL) || (capacity == 0u)) return;
    if (source != NULL) {
        while ((source[index] != '\0') && ((index + 1u) < capacity)) {
            destination[index] = source[index];
            index++;
        }
    }
    destination[index] = '\0';
}

static void btstack_at_copy_address(uint8_t destination[6],
    const bt_xstack_addr_t *source)
{
    memcpy(destination, source->bytes, 6u);
}

static void btstack_at_fill_state(btstack_at_state_t *destination)
{
    bt_xstack_state_t source;
    bt_xstack_phonebook_state_t pbap_state;
    uint8_t index;

    memset(destination, 0, sizeof(*destination));
    if (btstack_core_copy_state(&source) != 0) return;
    btstack_at_copy_address(destination->local_address,
        &source.local_address);
    btstack_at_copy_text(destination->local_name,
        sizeof(destination->local_name), source.local_name);
    btstack_at_copy_text(destination->pin,
        sizeof(destination->pin), source.pin);
    destination->class_of_device = source.class_of_device;
    destination->profile_mask = source.profile_mask;
    destination->power_on = source.power_on;
    destination->discoverable = source.discoverable;
    destination->scanning = source.scanning;
    destination->ssp_mode = source.ssp_mode;
    destination->tdl_mode = source.tdl_mode;
    destination->microphone_muted = source.microphone_muted;
    destination->speaker_volume = source.speaker_volume;
    destination->hfp_config = source.hfp_config;
    destination->hfp_state = source.hfp_state;
    destination->hfp_audio_state = source.hfp_audio_state;
    destination->hfp_voice_recognition = source.hfp_voice_recognition;
    destination->a2dp_state = source.a2dp_state;
    destination->a2dp_muted = source.a2dp_muted;
    destination->avrcp_state = source.avrcp_state;
    destination->play_state = source.play_state;
    destination->repeat_mode = source.repeat_mode;
    destination->shuffle_mode = source.shuffle_mode;
    destination->absolute_volume = source.absolute_volume;
    if (btstack_core_state_read_pbap(&pbap_state) == 0) {
        destination->pbap_state = (uint8_t)pbap_state;
    }
    for (index = 0u; index < BTSTACK_AT_PEER_MAX; index++) {
        if (!source.paired_devices[index].valid) continue;
        destination->peers[index].valid = true;
        btstack_at_copy_address(destination->peers[index].address,
            &source.paired_devices[index].address);
        destination->peers[index].profile_mask =
            source.paired_devices[index].profile_mask;
        btstack_at_copy_text(destination->peers[index].name,
            sizeof(destination->peers[index].name),
            source.paired_devices[index].name);
    }
}

static void btstack_at_request_init(btstack_core_request_t *request,
    const btstack_at_command_t *command, uint32_t generation)
{
    memset(request, 0, sizeof(*request));
    request->generation = generation;
    request->values[0] = command->values[0];
    request->values[1] = command->values[1];
    request->values[2] = command->values[2];
    request->value_count = command->value_count;
    request->has_address = command->has_address ? 1u : 0u;
    memcpy(request->address.bytes, command->address, 6u);
    btstack_at_copy_text(request->text, sizeof(request->text), command->text);
}

static int btstack_at_map_request(const btstack_at_command_t *command,
    uint32_t generation, btstack_core_request_t *request, int *query)
{
    btstack_at_request_init(request, command, generation);
    *query = 0;
    switch (command->id) {
    case BTSTACK_AT_COMMAND_PROFILE:
        if (command->form == BTSTACK_AT_FORM_QUERY) { *query = 1; return 0; }
        request->id = BT_XSTACK_REQUEST_SET_PROFILE_MASK; break;
    case BTSTACK_AT_COMMAND_VER:
    case BTSTACK_AT_COMMAND_ADDR:
    case BTSTACK_AT_COMMAND_PBAPSTAT:
        *query = 1; return 0;
    case BTSTACK_AT_COMMAND_NAME:
        if (command->form == BTSTACK_AT_FORM_QUERY) { *query = 1; return 0; }
        request->id = BT_XSTACK_REQUEST_SET_NAME; break;
    case BTSTACK_AT_COMMAND_PIN:
        if (command->form == BTSTACK_AT_FORM_QUERY) { *query = 1; return 0; }
        request->id = BT_XSTACK_REQUEST_SET_PIN; break;
    case BTSTACK_AT_COMMAND_SSP:
        if (command->form == BTSTACK_AT_FORM_QUERY) { *query = 1; return 0; }
        request->id = BT_XSTACK_REQUEST_SET_SSP; break;
    case BTSTACK_AT_COMMAND_BTEN: request->id = BT_XSTACK_REQUEST_SET_POWER; break;
    case BTSTACK_AT_COMMAND_CFM: request->id = BT_XSTACK_REQUEST_PAIR_CONFIRM; break;
    case BTSTACK_AT_COMMAND_PAIR: request->id = BT_XSTACK_REQUEST_SET_DISCOVERABLE; break;
    case BTSTACK_AT_COMMAND_PAGE: request->id = BT_XSTACK_REQUEST_SET_CONNECTABLE; break;
    case BTSTACK_AT_COMMAND_COD:
        if (command->form == BTSTACK_AT_FORM_QUERY) { *query = 1; return 0; }
        request->id = BT_XSTACK_REQUEST_SET_COD; break;
    case BTSTACK_AT_COMMAND_PLIST:
        if (command->form == BTSTACK_AT_FORM_QUERY) { *query = 1; return 0; }
        request->id = command->has_address ?
            BT_XSTACK_REQUEST_CLEAR_PEER_ADDRESS :
            ((command->values[0] == 0u) ? BT_XSTACK_REQUEST_CLEAR_PEERS :
                BT_XSTACK_REQUEST_CLEAR_PEER_INDEX); break;
    case BTSTACK_AT_COMMAND_TDL:
        if (command->form == BTSTACK_AT_FORM_QUERY) { *query = 1; return 0; }
        request->id = BT_XSTACK_REQUEST_SET_TDL; break;
    case BTSTACK_AT_COMMAND_SCAN:
        request->id = command->values[0] ?
            BT_XSTACK_REQUEST_SCAN_START : BT_XSTACK_REQUEST_SCAN_STOP;
        if (command->values[0]) request->values[0] = command->values[1];
        break;
    case BTSTACK_AT_COMMAND_DSCA: request->id = BT_XSTACK_REQUEST_DISCONNECT_ALL; break;
    case BTSTACK_AT_COMMAND_MICMUTE: request->id = BT_XSTACK_REQUEST_SET_MICROPHONE_MUTE; break;
    case BTSTACK_AT_COMMAND_SPKVOL:
        if (command->form == BTSTACK_AT_FORM_QUERY) { *query = 1; return 0; }
        request->id = BT_XSTACK_REQUEST_SET_SPEAKER_VOLUME; break;
    case BTSTACK_AT_COMMAND_HFPCFG:
        if (command->form == BTSTACK_AT_FORM_QUERY) { *query = 1; return 0; }
        request->id = BT_XSTACK_REQUEST_SET_HFP_CONFIG; break;
    case BTSTACK_AT_COMMAND_HFPSTAT:
    case BTSTACK_AT_COMMAND_A2DPSTAT:
        *query = 1; return 0;
    case BTSTACK_AT_COMMAND_HFPCONN:
    case BTSTACK_AT_COMMAND_A2DPCONN:
        request->profile = (command->id == BTSTACK_AT_COMMAND_HFPCONN) ?
            BT_XSTACK_PROFILE_HFP_HF : BT_XSTACK_PROFILE_A2DP_SINK;
        request->id = command->has_address ? BT_XSTACK_REQUEST_CONNECT :
            BT_XSTACK_REQUEST_CONNECT_LAST; break;
    case BTSTACK_AT_COMMAND_HFPDISC:
    case BTSTACK_AT_COMMAND_A2DPDISC:
        request->id = BT_XSTACK_REQUEST_DISCONNECT_PROFILE;
        request->profile = (command->id == BTSTACK_AT_COMMAND_HFPDISC) ?
            BT_XSTACK_PROFILE_HFP_HF : BT_XSTACK_PROFILE_A2DP_SINK; break;
    case BTSTACK_AT_COMMAND_HFPDIAL:
        request->id = (command->form == BTSTACK_AT_FORM_SET) ?
            BT_XSTACK_REQUEST_HFP_DIAL : BT_XSTACK_REQUEST_HFP_REDIAL; break;
    case BTSTACK_AT_COMMAND_HFPDTMF: request->id = BT_XSTACK_REQUEST_HFP_DTMF; break;
    case BTSTACK_AT_COMMAND_HFPANSW:
        request->id = BT_XSTACK_REQUEST_HFP_CONTROL;
        request->values[0] = BT_XSTACK_HFP_CONTROL_ANSWER; break;
    case BTSTACK_AT_COMMAND_HFPCHUP:
        request->id = BT_XSTACK_REQUEST_HFP_CONTROL;
        request->values[0] = BT_XSTACK_HFP_CONTROL_HANG_UP; break;
    case BTSTACK_AT_COMMAND_HFPMCAL:
        request->id = BT_XSTACK_REQUEST_HFP_CONTROL;
        request->values[1] = command->values[0];
        request->values[0] = BT_XSTACK_HFP_CONTROL_MULTIPARTY;
        request->value_count = 2u; break;
    case BTSTACK_AT_COMMAND_HFPADTS:
        request->id = BT_XSTACK_REQUEST_HFP_CONTROL;
        request->values[0] = (command->form == BTSTACK_AT_FORM_EXECUTE) ?
            BT_XSTACK_HFP_CONTROL_TOGGLE_AUDIO : BT_XSTACK_HFP_CONTROL_SET_AUDIO;
        request->values[1] = (command->values[0] == 2u) ? 1u : 0u;
        request->value_count = (command->form == BTSTACK_AT_FORM_EXECUTE) ? 1u : 2u;
        break;
    case BTSTACK_AT_COMMAND_HFPVR:
        request->id = BT_XSTACK_REQUEST_HFP_CONTROL;
        request->values[1] = command->values[0];
        request->values[0] = BT_XSTACK_HFP_CONTROL_VOICE_RECOGNITION;
        request->value_count = 2u; break;
    case BTSTACK_AT_COMMAND_A2DPMUTE: request->id = BT_XSTACK_REQUEST_SET_A2DP_MUTE; break;
    case BTSTACK_AT_COMMAND_PLAYPAUSE: request->values[0] = BT_XSTACK_AVRCP_CONTROL_PLAY_PAUSE; goto avrcp_control;
    case BTSTACK_AT_COMMAND_PLAY: request->values[0] = BT_XSTACK_AVRCP_CONTROL_PLAY; goto avrcp_control;
    case BTSTACK_AT_COMMAND_PAUSE: request->values[0] = BT_XSTACK_AVRCP_CONTROL_PAUSE; goto avrcp_control;
    case BTSTACK_AT_COMMAND_STOP: request->values[0] = BT_XSTACK_AVRCP_CONTROL_STOP; goto avrcp_control;
    case BTSTACK_AT_COMMAND_NEXT: request->values[0] = BT_XSTACK_AVRCP_CONTROL_NEXT; goto avrcp_control;
    case BTSTACK_AT_COMMAND_PREV: request->values[0] = BT_XSTACK_AVRCP_CONTROL_PREVIOUS; goto avrcp_control;
    case BTSTACK_AT_COMMAND_FORWARD: request->values[0] = BT_XSTACK_AVRCP_CONTROL_FAST_FORWARD; goto avrcp_control;
    case BTSTACK_AT_COMMAND_BACKWARD: request->values[0] = BT_XSTACK_AVRCP_CONTROL_REWIND; goto avrcp_control;
    case BTSTACK_AT_COMMAND_SABSUP: request->values[0] = BT_XSTACK_AVRCP_CONTROL_VOLUME_UP; goto avrcp_control;
    case BTSTACK_AT_COMMAND_SABSDOWN: request->values[0] = BT_XSTACK_AVRCP_CONTROL_VOLUME_DOWN;
avrcp_control:
        request->id = BT_XSTACK_REQUEST_AVRCP_CONTROL;
        request->value_count = 1u; break;
    case BTSTACK_AT_COMMAND_REPEAT: request->id = BT_XSTACK_REQUEST_SET_REPEAT; break;
    case BTSTACK_AT_COMMAND_SHUFFLE: request->id = BT_XSTACK_REQUEST_SET_SHUFFLE; break;
    case BTSTACK_AT_COMMAND_COVERMODE:
        request->id = BT_XSTACK_REQUEST_COVER_ART_SET_MODE;
        request->cover_art.mode = (bt_xstack_cover_art_mode_t)command->values[0];
        break;
    case BTSTACK_AT_COMMAND_COVERIMAGE:
        request->id = BT_XSTACK_REQUEST_COVER_ART_GET_IMAGE;
        request->cover_art.descriptor = command->cover_art_descriptor;
        break;
    case BTSTACK_AT_COMMAND_PBAPSYNC: request->id = BT_XSTACK_REQUEST_PBAP_SYNC; break;
    case BTSTACK_AT_COMMAND_PBAPCANCEL: request->id = BT_XSTACK_REQUEST_PBAP_CANCEL; break;
    case BTSTACK_AT_COMMAND_PBAPAUTH: request->id = BT_XSTACK_REQUEST_PBAP_AUTHENTICATE; break;
    case BTSTACK_AT_COMMAND_LEADV:
        request->id = command->values[0] ?
            BT_XSTACK_REQUEST_LE_ADV_CONFIGURE_START :
            BT_XSTACK_REQUEST_LE_ADV_STOP;
        if (command->values[0]) request->le_adv = command->le_adv;
        break;
    case BTSTACK_AT_COMMAND_LESCAN:
        request->id = command->values[0] ?
            BT_XSTACK_REQUEST_LE_SCAN_START : BT_XSTACK_REQUEST_LE_SCAN_STOP;
        if (command->values[0]) request->le_scan = command->le_scan;
        break;
    case BTSTACK_AT_COMMAND_LECONN:
        request->id = BT_XSTACK_REQUEST_LE_CONNECT;
        request->le_connect.address_type = (uint8_t)command->values[0];
        memcpy(request->le_connect.address.bytes, command->address, 6u);
        break;
    case BTSTACK_AT_COMMAND_LEDISC:
        request->id = BT_XSTACK_REQUEST_LE_DISCONNECT;
        break;
    case BTSTACK_AT_COMMAND_LEPAIR:
        request->id = BT_XSTACK_REQUEST_LE_PAIR;
        break;
    case BTSTACK_AT_COMMAND_LECFM:
        request->id = BT_XSTACK_REQUEST_LE_PAIRING_CONFIRM;
        break;
    case BTSTACK_AT_COMMAND_LEPASS:
        request->id = BT_XSTACK_REQUEST_LE_PASSKEY_REPLY;
        break;
    case BTSTACK_AT_COMMAND_GATTSADD:
        request->id = BT_XSTACK_REQUEST_GATT_SERVER_ADD_SERVICE;
        request->gatt_registration.uuid = command->uuid;
        break;
    case BTSTACK_AT_COMMAND_GATTSC:
        request->id = BT_XSTACK_REQUEST_GATT_SERVER_ADD_CHARACTERISTIC;
        request->gatt_registration.parent_handle = (uint16_t)command->values[0];
        request->gatt_registration.config.uuid = command->uuid;
        request->gatt_registration.config.properties =
            (uint16_t)command->values[1];
        request->gatt_registration.config.max_len =
            (uint16_t)command->values[2];
        request->gatt_registration.config.read_security =
            (bt_xstack_gatt_security_t)command->security;
        request->gatt_registration.config.write_security =
            (bt_xstack_gatt_security_t)command->security;
        break;
    case BTSTACK_AT_COMMAND_GATTSBUILD:
        request->id = BT_XSTACK_REQUEST_GATT_SERVER_BUILD;
        break;
    case BTSTACK_AT_COMMAND_GATTSVAL:
    case BTSTACK_AT_COMMAND_GATTSNTF:
    case BTSTACK_AT_COMMAND_GATTSIND:
        request->id = (command->id == BTSTACK_AT_COMMAND_GATTSVAL) ?
            BT_XSTACK_REQUEST_GATT_SERVER_SET_VALUE :
            ((command->id == BTSTACK_AT_COMMAND_GATTSNTF) ?
                BT_XSTACK_REQUEST_GATT_SERVER_NOTIFY :
                BT_XSTACK_REQUEST_GATT_SERVER_INDICATE);
        request->gatt.handle = (uint16_t)command->values[0];
        request->gatt.value_len = command->binary_length;
        if (command->binary_length != 0u) {
            memcpy(request->gatt.value, command->binary,
                command->binary_length);
        }
        break;
    case BTSTACK_AT_COMMAND_GATTCDISC:
        request->id = BT_XSTACK_REQUEST_GATT_CLIENT_DISCOVER_SERVICES;
        request->gatt.flags = command->value_count ? 1u : 0u;
        request->gatt.uuid = command->uuid;
        break;
    case BTSTACK_AT_COMMAND_GATTCC:
        request->id = BT_XSTACK_REQUEST_GATT_CLIENT_DISCOVER_CHARACTERISTICS;
        request->gatt.handle = (uint16_t)command->values[0];
        request->gatt.end_handle = (uint16_t)command->values[1];
        request->gatt.flags = (command->value_count == 3u) ? 1u : 0u;
        request->gatt.uuid = command->uuid;
        break;
    case BTSTACK_AT_COMMAND_GATTCR:
        request->id = BT_XSTACK_REQUEST_GATT_CLIENT_READ;
        request->gatt.handle = (uint16_t)command->values[0];
        break;
    case BTSTACK_AT_COMMAND_GATTCW:
        request->id = BT_XSTACK_REQUEST_GATT_CLIENT_WRITE;
        request->gatt.handle = (uint16_t)command->values[0];
        request->gatt.flags = command->values[1] ? 0u : 1u;
        request->gatt.value_len = command->binary_length;
        if (command->binary_length != 0u) {
            memcpy(request->gatt.value, command->binary,
                command->binary_length);
        }
        break;
    case BTSTACK_AT_COMMAND_GATTCSUB:
        request->id = command->values[2] ?
            BT_XSTACK_REQUEST_GATT_CLIENT_SUBSCRIBE :
            BT_XSTACK_REQUEST_GATT_CLIENT_UNSUBSCRIBE;
        request->gatt.characteristic.start_handle =
            (uint16_t)command->values[0];
        request->gatt.characteristic.value_handle =
            (uint16_t)command->values[0];
        request->gatt.characteristic.end_handle =
            (uint16_t)command->values[1];
        request->gatt.flags = (uint8_t)command->values[2];
        break;
    default: return -1;
    }
    return 0;
}

static int btstack_at_core_dispatch(const btstack_at_command_t *command,
    uint32_t generation)
{
    btstack_core_request_t request;
    btstack_at_result_t result;
    int query;
    int status;

    if (btstack_at_map_request(command, generation, &request, &query) != 0) {
        return -1;
    }
    btstack_at_pending_command = command->id;
    btstack_at_pending_generation = generation;
    if (query) {
        memset(&result, 0, sizeof(result));
        result.generation = generation;
        result.command_id = command->id;
        result.success = 1;
        btstack_at_fill_state(&result.state);
        return btstack_at_complete(&result);
    }
    status = btstack_core_dispatch(&request);
    return (status == BT_XSTACK_BACKEND_BUSY) ? -1 : status;
}

static int btstack_at_core_gate(const btstack_at_command_t *command,
    void *context)
{
    (void)context;
    if ((command->id == BTSTACK_AT_COMMAND_BTEN) &&
        (command->values[0] != 0u) && !btstack_runtime_started()) {
        return (btstack_runtime_start() == 0) ?
            BTSTACK_AT_GATE_CONTINUE : BTSTACK_AT_GATE_REJECT;
    }
    if (!btstack_runtime_started() &&
        (command->id != BTSTACK_AT_COMMAND_VER) &&
        (command->id != BTSTACK_AT_COMMAND_ADDR) &&
        (command->id != BTSTACK_AT_COMMAND_PROFILE)) {
        return BTSTACK_AT_GATE_REJECT;
    }
    return BTSTACK_AT_GATE_CONTINUE;
}

int btstack_at_core_adapter_init(void)
{
    int status = btstack_at_backend_register(btstack_at_core_dispatch);
    if (status != BTSTACK_AT_STATUS_OK) return status;
    return btstack_at_command_gate_register(btstack_at_core_gate, NULL);
}

void btstack_core_command_slot_released(void)
{
    /* A failed post releases its slot synchronously. Retrying on this edge
     * would self-notify/spin; run-loop ingress dequeue is the retry edge. */
}

void btstack_core_request_completed(const btstack_core_request_t *request,
    int status)
{
    btstack_at_result_t result;
    if ((request != NULL) &&
        (request->id == BT_XSTACK_REQUEST_PBAP_PEER_DISCONNECTED)) {
        btstack_platform_enter_critical();
        btstack_at_pbap_peer_disconnect_inflight = 0u;
        btstack_platform_exit_critical();
        (void)btstack_platform_notify(
            BTSTACK_WORKER_AT,
            BTSTACK_WORKER_SIGNAL_STACK_INGRESS_SPACE);
        return;
    }
    if ((request == NULL) || (request->generation == 0u) ||
        (request->generation != btstack_at_pending_generation)) return;
    memset(&result, 0, sizeof(result));
    result.generation = request->generation;
    result.command_id = btstack_at_pending_command;
    result.success = (status == 0);
    btstack_at_fill_state(&result.state);
    (void)btstack_at_complete(&result);
}

void btstack_core_pbap_event_space_request_completed(void)
{
    btstack_platform_enter_critical();
    btstack_at_pbap_space_pending = 0u;
    btstack_platform_exit_critical();
}

void btstack_at_event_rejected(const btstack_at_event_t *event)
{
    if ((event != NULL) &&
        ((event->id == BTSTACK_AT_EVENT_PBAPCONTACT) ||
         (event->id == BTSTACK_AT_EVENT_PBAPCALL))) {
        /* Called by the AT event ring while its critical section is held. */
        btstack_at_pbap_space_blocked = 1u;
    }
}

void btstack_at_event_consumed(const btstack_at_event_t *event)
{
    btstack_core_request_t request;
    int request_space = 0;

    if (event == NULL) return;
    if ((event->id == BTSTACK_AT_EVENT_HFPLOST) &&
        btstack_profile_manager_hfp_event_delivered(
            (uint8_t)BTSTACK_HFP_HF_EVENT_CONNECTION_LOST,
            event->data.peer.address)) {
        btstack_at_queue_pbap_peer_disconnect(
            event->data.peer.address);
    }
    btstack_platform_enter_critical();
    if (btstack_at_pbap_space_blocked &&
        !btstack_at_pbap_space_pending) {
        btstack_at_pbap_space_blocked = 0u;
        btstack_at_pbap_space_pending = 1u;
        request_space = 1;
    }
    btstack_platform_exit_critical();
    if (!request_space) return;

    memset(&request, 0, sizeof(request));
    request.id = BT_XSTACK_REQUEST_PBAP_EVENT_SPACE_AVAILABLE;
    if (btstack_core_dispatch(&request) == BT_XSTACK_BACKEND_BUSY) {
        btstack_platform_enter_critical();
        btstack_at_pbap_space_pending = 0u;
        btstack_at_pbap_space_blocked = 1u;
        btstack_platform_exit_critical();
    }
}

static void btstack_at_map_service(const bt_xstack_service_event_t *source,
    btstack_at_event_t *destination)
{
    switch (source->id) {
    case BT_XSTACK_SERVICE_EVENT_DEVICE_STATE:
        destination->id = BTSTACK_AT_EVENT_DEVSTAT;
        memcpy(destination->data.numeric.values,
            source->values, sizeof(destination->data.numeric.values));
        destination->data.numeric.value_count = source->value_count;
        break;
    case BT_XSTACK_SERVICE_EVENT_POWER_STATE:
        destination->id = BTSTACK_AT_EVENT_PWRSTAT;
        memcpy(destination->data.numeric.values,
            source->values, sizeof(destination->data.numeric.values));
        destination->data.numeric.value_count = source->value_count;
        break;
    case BT_XSTACK_SERVICE_EVENT_SCAN_RESULT:
        destination->id = BTSTACK_AT_EVENT_SCAN_RESULT;
        destination->data.scan.profile_mask = source->values[0];
        destination->data.scan.rssi = source->rssi;
        destination->data.scan.address_type = 0u;
        destination->data.scan.class_of_device = source->class_of_device;
        btstack_at_copy_address(destination->data.scan.address,
            &source->address);
        btstack_at_copy_text(destination->data.scan.name,
            sizeof(destination->data.scan.name), source->name);
        break;
    case BT_XSTACK_SERVICE_EVENT_SCAN_COMPLETE:
        destination->id = BTSTACK_AT_EVENT_SCAN_END;
        break;
    case BT_XSTACK_SERVICE_EVENT_PAIRING_REQUEST:
        destination->id = BTSTACK_AT_EVENT_PAIR_REQUEST;
        memcpy(destination->data.peer.values,
            source->values, sizeof(destination->data.peer.values));
        destination->data.peer.value_count = source->value_count;
        btstack_at_copy_address(destination->data.peer.address,
            &source->address);
        btstack_at_copy_text(destination->data.peer.text,
            sizeof(destination->data.peer.text), source->name);
        break;
    case BT_XSTACK_SERVICE_EVENT_PAIRED:
        destination->id = BTSTACK_AT_EVENT_PAIRED;
        memcpy(destination->data.peer.values,
            source->values, sizeof(destination->data.peer.values));
        destination->data.peer.value_count = source->value_count;
        btstack_at_copy_address(destination->data.peer.address,
            &source->address);
        break;
    }
}

static void btstack_at_map_phone(const bt_xstack_phone_event_t *source,
    btstack_at_event_t *destination)
{
    static const btstack_at_event_id_t ids[] = {
        BTSTACK_AT_EVENT_HFPSTAT, BTSTACK_AT_EVENT_HFPDEV,
        BTSTACK_AT_EVENT_HFPAUDIO, BTSTACK_AT_EVENT_HFPVR,
        BTSTACK_AT_EVENT_HFPSIG, BTSTACK_AT_EVENT_HFPNET,
        BTSTACK_AT_EVENT_HFPROAM, BTSTACK_AT_EVENT_HFPBATT,
        BTSTACK_AT_EVENT_HFPNUM, BTSTACK_AT_EVENT_HFPIBR,
        BTSTACK_AT_EVENT_HFPRING, BTSTACK_AT_EVENT_HFPLOST,
        BTSTACK_AT_EVENT_HFPMTPX, BTSTACK_AT_EVENT_MICMUTED
    };
    destination->id = ids[source->id];
    switch (destination->id) {
    case BTSTACK_AT_EVENT_HFPSTAT:
    case BTSTACK_AT_EVENT_HFPDEV:
    case BTSTACK_AT_EVENT_HFPLOST:
    case BTSTACK_AT_EVENT_HFPMTPX:
        memcpy(destination->data.peer.values,
            source->values, sizeof(destination->data.peer.values));
        destination->data.peer.value_count = source->value_count;
        btstack_at_copy_address(destination->data.peer.address,
            &source->address);
        btstack_at_copy_text(destination->data.peer.text,
            sizeof(destination->data.peer.text), source->text);
        break;
    case BTSTACK_AT_EVENT_HFPNET:
    case BTSTACK_AT_EVENT_HFPMANU:
    case BTSTACK_AT_EVENT_HFPNUM:
        btstack_at_copy_text(destination->data.text.value,
            sizeof(destination->data.text.value), source->text);
        break;
    default:
        memcpy(destination->data.numeric.values,
            source->values, sizeof(destination->data.numeric.values));
        destination->data.numeric.value_count = source->value_count;
        break;
    }
}

static void btstack_at_map_audio(const bt_xstack_audio_event_t *source,
    btstack_at_event_t *destination)
{
    static const btstack_at_event_id_t ids[] = {
        BTSTACK_AT_EVENT_A2DPSTAT, BTSTACK_AT_EVENT_A2DPDEV,
        BTSTACK_AT_EVENT_A2DPLOST, BTSTACK_AT_EVENT_A2DPPCMCONFIG,
        BTSTACK_AT_EVENT_AVRCPSTAT, BTSTACK_AT_EVENT_PLAYSTAT,
        BTSTACK_AT_EVENT_PLAYMODE, BTSTACK_AT_EVENT_TRACKSTAT,
        BTSTACK_AT_EVENT_TRACKINFO, BTSTACK_AT_EVENT_ABSVOL
    };
    destination->id = ids[source->id];
    switch (destination->id) {
    case BTSTACK_AT_EVENT_A2DPDEV:
    case BTSTACK_AT_EVENT_A2DPLOST:
        btstack_at_copy_address(destination->data.peer.address,
            &source->address);
        break;
    case BTSTACK_AT_EVENT_TRACKINFO:
        btstack_at_copy_text(destination->data.metadata.title,
            sizeof(destination->data.metadata.title), source->title);
        btstack_at_copy_text(destination->data.metadata.artist,
            sizeof(destination->data.metadata.artist), source->artist);
        btstack_at_copy_text(destination->data.metadata.album,
            sizeof(destination->data.metadata.album), source->album);
        break;
    default:
        memcpy(destination->data.numeric.values,
            source->values, sizeof(destination->data.numeric.values));
        destination->data.numeric.value_count = source->value_count;
        break;
    }
}

int btstack_core_post_event(const btstack_core_event_envelope_t *source)
{
    btstack_at_event_t destination;
    if (source == NULL) return -1;
    memset(&destination, 0, sizeof(destination));
    switch (source->domain) {
    case BT_XSTACK_EVENT_DOMAIN_SERVICE:
        btstack_at_map_service(&source->data.service, &destination); break;
    case BT_XSTACK_EVENT_DOMAIN_PHONE:
        btstack_at_map_phone(&source->data.phone, &destination); break;
    case BT_XSTACK_EVENT_DOMAIN_AUDIO:
        btstack_at_map_audio(&source->data.audio, &destination); break;
    case BT_XSTACK_EVENT_DOMAIN_PHONEBOOK:
        destination.data.phonebook = source->data.phonebook;
        destination.id = (btstack_at_event_id_t)(BTSTACK_AT_EVENT_PBAPSTART +
            source->data.phonebook.id);
        break;
    case BT_XSTACK_EVENT_DOMAIN_LE:
        destination.id = BTSTACK_AT_EVENT_LE;
        destination.data.le = source->data.le;
        break;
    case BT_XSTACK_EVENT_DOMAIN_GATT:
        destination.id = BTSTACK_AT_EVENT_GATT;
        destination.data.gatt = source->data.gatt;
        break;
    case BT_XSTACK_EVENT_DOMAIN_CONNECTION:
    default:
        return 0;
    }
    return btstack_at_post_event(&destination);
}

void btstack_frontend_deliver_event(const btstack_core_event_envelope_t *event)
{
    (void)event;
}

void btstack_core_media_audio_deliver(const uint8_t *data, uint32_t length)
{
    if ((btstack_at_pcm_callback != NULL) && (data != NULL) && (length != 0u)) {
        btstack_at_pcm_callback(data, length);
    }
}

void btstack_core_call_audio_deliver(const uint8_t *data, uint32_t length)
{
    (void)data;
    (void)length;
}

int btstack_at_set_pcm_callback(btstack_at_pcm_callback_t callback)
{
    btstack_at_pcm_callback = callback;
    return BTSTACK_AT_STATUS_OK;
}

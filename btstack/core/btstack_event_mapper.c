#include "btstack_core_internal.h"
#include <string.h>

#include "adapter/btstack_adapter.h"
#include "adapter/btstack_types.h"
#include "profiles/btstack_a2dp_sink.h"
#include "profiles/btstack_avrcp.h"
#include "profiles/btstack_hfp_hf.h"
#include "profiles/btstack_pbap_client.h"
#include "classic/hfp_hf.h"
#include "port/btstack_platform.h"
#include "core/btstack_profile_manager.h"

static bt_xstack_state_t btstack_core_state;
static uint8_t btstack_core_scan_index;
static char btstack_core_scan_filter[BT_XSTACK_TEXT_MAX];

static void btstack_core_copy_address(
    bt_xstack_addr_t *destination,
    const uint8_t source[BT_XSTACK_ADDRESS_LEN])
{
    memcpy(destination->bytes, source, BT_XSTACK_ADDRESS_LEN);
}

static void btstack_core_copy_text(
    char *destination,
    uint16_t capacity,
    const char *source)
{
    uint16_t length;

    if ((destination == NULL) || (capacity == 0u)) {
        return;
    }
    if (source == NULL) {
        destination[0] = '\0';
        return;
    }
    length = (uint16_t)strlen(source);
    if (length >= capacity) {
        length = capacity - 1u;
    }
    memcpy(destination, source, length);
    destination[length] = '\0';
}

void btstack_core_state_sync_gap(void)
{
    btstack_adapter_state_t adapter_state;

    if (btstack_adapter_get_state(&adapter_state) != BTSTACK_STATUS_OK) {
        return;
    }
    btstack_platform_enter_critical();
    btstack_core_copy_address(
        &btstack_core_state.local_address,
        adapter_state.local_address);
    btstack_core_copy_text(
        btstack_core_state.local_name,
        sizeof(btstack_core_state.local_name),
        adapter_state.local_name);
    btstack_core_copy_text(
        btstack_core_state.pin,
        sizeof(btstack_core_state.pin),
        adapter_state.pin);
    btstack_core_state.class_of_device = adapter_state.class_of_device;
    btstack_core_state.ssp_mode = adapter_state.ssp_mode;
    btstack_core_state.power_on = adapter_state.stack_working ? 1u : 0u;
    btstack_core_state.connectable = adapter_state.connectable ? 1u : 0u;
    btstack_core_state.discoverable = adapter_state.discoverable ? 1u : 0u;
    btstack_core_state.scanning = adapter_state.scanning ? 1u : 0u;
    btstack_platform_exit_critical();
}

void btstack_core_state_set_value(
    bt_xstack_request_id_t id,
    uint32_t value)
{
    btstack_platform_enter_critical();
    switch (id) {
    case BT_XSTACK_REQUEST_SET_PROFILE_MASK:
        btstack_core_state.profile_mask = (uint16_t)value;
        break;
    case BT_XSTACK_REQUEST_SET_TDL:
        btstack_core_state.tdl_mode = (uint8_t)value;
        break;
    default:
        break;
    }
    btstack_platform_exit_critical();
    btstack_profile_manager_state_set_value(id, value);
}

int btstack_core_copy_state(bt_xstack_state_t *state)
{
    btstack_profile_manager_classic_state_t classic;
    if (state == NULL) {
        return -1;
    }
    btstack_platform_enter_critical();
    memcpy(state, &btstack_core_state, sizeof(*state));
    btstack_platform_exit_critical();
    if (btstack_profile_manager_get_classic_state(&classic) !=
        BTSTACK_STATUS_OK) return -1;
    state->hfp_config = classic.hfp_config;
    state->hfp_state = classic.hfp_state;
    state->hfp_audio_state = classic.hfp_audio_state;
    state->hfp_voice_recognition = classic.hfp_voice_recognition;
    state->microphone_muted = classic.microphone_muted;
    state->speaker_volume = classic.speaker_volume;
    state->a2dp_state = classic.a2dp_state;
    state->a2dp_muted = classic.a2dp_muted;
    state->avrcp_state = classic.avrcp_state;
    state->play_state = classic.play_state;
    state->repeat_mode = classic.repeat_mode;
    state->shuffle_mode = classic.shuffle_mode;
    state->absolute_volume = classic.absolute_volume;
    return 0;
}

int btstack_core_state_read_service(
    btstack_core_service_snapshot_t *snapshot)
{
    if (snapshot == NULL) {
        return -1;
    }
    btstack_platform_enter_critical();
    memcpy(&snapshot->local_address,
        &btstack_core_state.local_address,
        sizeof(snapshot->local_address));
    memcpy(snapshot->local_name,
        btstack_core_state.local_name,
        sizeof(snapshot->local_name));
    memcpy(snapshot->pin,
        btstack_core_state.pin,
        sizeof(snapshot->pin));
    snapshot->class_of_device = btstack_core_state.class_of_device;
    snapshot->profile_mask = btstack_core_state.profile_mask;
    snapshot->power_on = btstack_core_state.power_on;
    snapshot->discoverable = btstack_core_state.discoverable;
    snapshot->connectable = btstack_core_state.connectable;
    snapshot->scanning = btstack_core_state.scanning;
    snapshot->ssp_mode = btstack_core_state.ssp_mode;
    snapshot->tdl_mode = btstack_core_state.tdl_mode;
    btstack_platform_exit_critical();
    return 0;
}

int btstack_core_state_read_hfp(
    btstack_core_hfp_snapshot_t *snapshot)
{
    btstack_profile_manager_classic_state_t state;
    if (snapshot == NULL) {
        return -1;
    }
    if (btstack_profile_manager_get_classic_state(&state) !=
        BTSTACK_STATUS_OK) return -1;
    snapshot->config = state.hfp_config;
    snapshot->state = state.hfp_state;
    snapshot->audio_state = state.hfp_audio_state;
    snapshot->voice_recognition = state.hfp_voice_recognition;
    snapshot->microphone_muted = state.microphone_muted;
    snapshot->speaker_volume = state.speaker_volume;
    return 0;
}

int btstack_core_state_read_a2dp(
    btstack_core_a2dp_snapshot_t *snapshot)
{
    btstack_profile_manager_classic_state_t state;
    if (snapshot == NULL) {
        return -1;
    }
    if (btstack_profile_manager_get_classic_state(&state) !=
        BTSTACK_STATUS_OK) return -1;
    snapshot->state = state.a2dp_state;
    snapshot->muted = state.a2dp_muted;
    return 0;
}

int btstack_core_state_read_avrcp(
    btstack_core_avrcp_snapshot_t *snapshot)
{
    btstack_profile_manager_classic_state_t state;
    if (snapshot == NULL) {
        return -1;
    }
    if (btstack_profile_manager_get_classic_state(&state) !=
        BTSTACK_STATUS_OK) return -1;
    snapshot->state = state.avrcp_state;
    snapshot->play_state = state.play_state;
    snapshot->repeat_mode = state.repeat_mode;
    snapshot->shuffle_mode = state.shuffle_mode;
    snapshot->absolute_volume = state.absolute_volume;
    return 0;
}

int btstack_core_state_read_pbap(bt_xstack_phonebook_state_t *state)
{
    btstack_profile_manager_classic_state_t classic;
    if (state == NULL) {
        return -1;
    }
    if (btstack_profile_manager_get_classic_state(&classic) !=
        BTSTACK_STATUS_OK) return -1;
    *state = (bt_xstack_phonebook_state_t)classic.pbap_state;
    return 0;
}

void btstack_core_state_write_pbap(bt_xstack_phonebook_state_t state)
{
    btstack_profile_manager_state_write_pbap((uint8_t)state);
}

int btstack_core_state_get_peer_count(uint8_t *count)
{
    uint8_t index;
    uint8_t valid_count = 0u;

    if (count == NULL) {
        return -1;
    }
    btstack_platform_enter_critical();
    for (index = 0u; index < BT_XSTACK_PAIRED_DEVICE_MAX; index++) {
        if (btstack_core_state.paired_devices[index].valid) {
            valid_count++;
        }
    }
    btstack_platform_exit_critical();
    *count = valid_count;
    return 0;
}

int btstack_core_state_get_peer(
    uint8_t index,
    bt_xstack_paired_device_t *peer)
{
    if ((index >= BT_XSTACK_PAIRED_DEVICE_MAX) || (peer == NULL)) {
        return -1;
    }
    btstack_platform_enter_critical();
    memcpy(peer, &btstack_core_state.paired_devices[index],
        sizeof(*peer));
    btstack_platform_exit_critical();
    return 0;
}

int btstack_event_mapper_ready(void)
{
    int ready;

    btstack_platform_enter_critical();
    ready = btstack_core_state.power_on != 0u;
    btstack_platform_exit_critical();
    return ready;
}

int btstack_core_state_get_last_peer(bt_xstack_addr_t *address)
{
    int index;
    int result = -1;

    if (address == NULL) {
        return -1;
    }
    btstack_platform_enter_critical();
    for (index = (int)BT_XSTACK_PAIRED_DEVICE_MAX - 1;
         index >= 0;
         index--) {
        if (btstack_core_state.paired_devices[index].valid) {
            memcpy(address,
                &btstack_core_state.paired_devices[index].address,
                sizeof(*address));
            result = 0;
            break;
        }
    }
    btstack_platform_exit_critical();
    return result;
}

int btstack_core_state_clear_peer_index(uint8_t index)
{
    if ((index == 0u) || (index > BT_XSTACK_PAIRED_DEVICE_MAX)) {
        return -1;
    }
    btstack_platform_enter_critical();
    memset(&btstack_core_state.paired_devices[index - 1u], 0,
        sizeof(btstack_core_state.paired_devices[0]));
    btstack_platform_exit_critical();
    return 0;
}

int btstack_core_state_clear_peer_address(
    const bt_xstack_addr_t *address)
{
    uint8_t index;

    if (address == NULL) {
        return -1;
    }
    btstack_platform_enter_critical();
    for (index = 0u; index < BT_XSTACK_PAIRED_DEVICE_MAX; index++) {
        bt_xstack_paired_device_t *peer =
            &btstack_core_state.paired_devices[index];
        if (peer->valid &&
            (memcmp(&peer->address, address, sizeof(*address)) == 0)) {
            memset(peer, 0, sizeof(*peer));
            btstack_platform_exit_critical();
            return 0;
        }
    }
    btstack_platform_exit_critical();
    return -1;
}

void btstack_core_state_clear_peers(void)
{
    btstack_platform_enter_critical();
    memset(btstack_core_state.paired_devices, 0,
        sizeof(btstack_core_state.paired_devices));
    btstack_platform_exit_critical();
}

static void btstack_core_post_service(
    bt_xstack_service_event_id_t id,
    const bt_xstack_addr_t *address,
    uint32_t value)
{
    btstack_core_event_envelope_t envelope;

    memset(&envelope, 0, sizeof(envelope));
    envelope.domain = BT_XSTACK_EVENT_DOMAIN_SERVICE;
    envelope.data.service.id = id;
    envelope.data.service.values[0] = value;
    envelope.data.service.value_count = 1u;
    if (address != NULL) {
        memcpy(&envelope.data.service.address, address, sizeof(*address));
    }
    (void)btstack_core_post_event(&envelope);
}

static uint32_t btstack_core_device_state_value(void)
{
    uint32_t value = 0u;

    btstack_platform_enter_critical();
    if (btstack_core_state.power_on) value |= 1u;
    if (btstack_core_state.discoverable) value |= 2u;
    if (btstack_core_state.scanning) value |= 8u;
    btstack_platform_exit_critical();
    return value;
}

static void btstack_core_post_device_state(void)
{
    btstack_core_post_service(
        BT_XSTACK_SERVICE_EVENT_DEVICE_STATE,
        NULL,
        btstack_core_device_state_value());
}

static void btstack_core_add_peer(const uint8_t address[6])
{
    uint8_t index;
    uint8_t selected = BT_XSTACK_PAIRED_DEVICE_MAX;
    uint8_t count = 0u;
    bool disable_discoverable = false;

    btstack_platform_enter_critical();
    for (index = 0u; index < BT_XSTACK_PAIRED_DEVICE_MAX; index++) {
        bt_xstack_paired_device_t *peer =
            &btstack_core_state.paired_devices[index];
        if (peer->valid) {
            count++;
            if (memcmp(peer->address.bytes, address, 6u) == 0) {
                btstack_platform_exit_critical();
                return;
            }
        } else if (selected == BT_XSTACK_PAIRED_DEVICE_MAX) {
            selected = index;
        }
    }
    if ((btstack_core_state.tdl_mode != 0u) &&
        (count >= btstack_core_state.tdl_mode)) {
        disable_discoverable = true;
    } else {
        if (selected == BT_XSTACK_PAIRED_DEVICE_MAX) {
            if (btstack_core_state.tdl_mode != 0u) {
                btstack_platform_exit_critical();
                return;
            }
            for (index = 1u; index < BT_XSTACK_PAIRED_DEVICE_MAX; index++) {
                btstack_core_state.paired_devices[index - 1u] =
                    btstack_core_state.paired_devices[index];
            }
            selected = BT_XSTACK_PAIRED_DEVICE_MAX - 1u;
        }
        btstack_core_state.paired_devices[selected].valid = true;
        memcpy(btstack_core_state.paired_devices[selected].address.bytes,
            address, 6u);
        btstack_core_state.paired_devices[selected].profile_mask =
            BT_XSTACK_PROFILE_MASK_SUPPORTED;
        count++;
        if ((btstack_core_state.tdl_mode != 0u) &&
            (count >= btstack_core_state.tdl_mode)) {
            disable_discoverable = true;
        }
    }
    btstack_platform_exit_critical();

    if (disable_discoverable) {
        (void)btstack_adapter_set_discoverable(false);
        btstack_core_state_sync_gap();
        btstack_core_post_device_state();
    }
}

static void btstack_core_adapter_event(
    const btstack_adapter_event_t *event)
{
    btstack_core_event_envelope_t envelope;
    bt_xstack_state_t snapshot;
    char scan_filter[BT_XSTACK_TEXT_MAX];
    char event_name[BT_XSTACK_NAME_MAX];
    uint16_t name_length;
    uint8_t previous_power;

    if (event == NULL) return;
    memset(&envelope, 0, sizeof(envelope));
    envelope.domain = BT_XSTACK_EVENT_DOMAIN_SERVICE;
    switch (event->type) {
    case BTSTACK_ADAPTER_EVENT_STACK_STATE:
        (void)btstack_core_copy_state(&snapshot);
        previous_power = snapshot.power_on;
        btstack_core_state_sync_gap();
        (void)btstack_core_copy_state(&snapshot);
        if (previous_power != snapshot.power_on) {
            btstack_core_post_service(
                BT_XSTACK_SERVICE_EVENT_POWER_STATE,
                NULL,
                snapshot.power_on);
        }
        btstack_core_post_device_state();
        break;
    case BTSTACK_ADAPTER_EVENT_LOCAL_ADDRESS:
        btstack_platform_enter_critical();
        btstack_core_copy_address(
            &btstack_core_state.local_address,
            event->data.address.address);
        btstack_platform_exit_critical();
        break;
    case BTSTACK_ADAPTER_EVENT_ACL_CONNECTED:
    case BTSTACK_ADAPTER_EVENT_ACL_DISCONNECTED:
        btstack_core_state_sync_gap();
        break;
    case BTSTACK_ADAPTER_EVENT_SCAN_RESULT:
        btstack_platform_enter_critical();
        memcpy(scan_filter, btstack_core_scan_filter,
            sizeof(scan_filter));
        btstack_platform_exit_critical();
        name_length = event->data.scan_result.name_length;
        if (name_length >= sizeof(event_name)) {
            name_length = sizeof(event_name) - 1u;
        }
        memcpy(event_name, event->data.scan_result.name, name_length);
        event_name[name_length] = '\0';
        if ((scan_filter[0] != '\0') &&
            (strstr(event_name, scan_filter) == NULL)) {
            break;
        }
        envelope.data.service.id = BT_XSTACK_SERVICE_EVENT_SCAN_RESULT;
        envelope.data.service.values[0] = ++btstack_core_scan_index;
        envelope.data.service.value_count = 1u;
        envelope.data.service.class_of_device =
            event->data.scan_result.class_of_device;
        envelope.data.service.rssi = event->data.scan_result.rssi;
        btstack_core_copy_address(
            &envelope.data.service.address,
            event->data.scan_result.address);
        btstack_core_copy_text(
            envelope.data.service.name,
            sizeof(envelope.data.service.name),
            event_name);
        (void)btstack_core_post_event(&envelope);
        break;
    case BTSTACK_ADAPTER_EVENT_SCAN_COMPLETE:
        btstack_core_state_sync_gap();
        btstack_core_post_service(
            BT_XSTACK_SERVICE_EVENT_SCAN_COMPLETE, NULL, 0u);
        btstack_core_post_device_state();
        break;
    case BTSTACK_ADAPTER_EVENT_PAIRING_REQUEST:
        envelope.data.service.id =
            BT_XSTACK_SERVICE_EVENT_PAIRING_REQUEST;
        envelope.data.service.values[0] =
            event->data.pairing_request.numeric_value;
        envelope.data.service.value_count = 1u;
        btstack_core_copy_address(
            &envelope.data.service.address,
            event->data.pairing_request.address);
        (void)btstack_core_post_event(&envelope);
        break;
    case BTSTACK_ADAPTER_EVENT_PAIRING_COMPLETE:
        if (event->data.pairing_complete.status == 0) {
            btstack_core_add_peer(
                event->data.pairing_complete.address);
        }
        envelope.data.service.id = BT_XSTACK_SERVICE_EVENT_PAIRED;
        envelope.data.service.values[0] =
            (uint32_t)event->data.pairing_complete.status;
        envelope.data.service.value_count = 1u;
        btstack_core_copy_address(
            &envelope.data.service.address,
            event->data.pairing_complete.address);
        (void)btstack_core_post_event(&envelope);
        break;
    case BTSTACK_ADAPTER_EVENT_LE: {
        const btstack_adapter_le_event_t *le_event = &event->data.le;
        memset(&envelope, 0, sizeof(envelope));
        envelope.domain = BT_XSTACK_EVENT_DOMAIN_LE;
        envelope.data.le.status = le_event->status;
        switch (le_event->type) {
        case BTSTACK_ADAPTER_LE_EVENT_SCAN_RESULT:
            envelope.data.le.type = BT_XSTACK_LE_EVENT_SCAN_RESULT;
            btstack_core_copy_address(&envelope.data.le.data.scan.address,
                le_event->address);
            envelope.data.le.data.scan.rssi = le_event->rssi;
            envelope.data.le.data.scan.address_type = le_event->address_type;
            envelope.data.le.data.scan.advertising_event_type =
                le_event->advertising_event_type;
            envelope.data.le.data.scan.data_len = le_event->data_len;
            memcpy(envelope.data.le.data.scan.data, le_event->data,
                le_event->data_len);
            break;
        case BTSTACK_ADAPTER_LE_EVENT_CONNECTED:
            envelope.data.le.type = BT_XSTACK_LE_EVENT_CONNECTED;
            goto le_link;
        case BTSTACK_ADAPTER_LE_EVENT_DISCONNECTED:
            envelope.data.le.type = BT_XSTACK_LE_EVENT_DISCONNECTED;
            goto le_link;
        case BTSTACK_ADAPTER_LE_EVENT_CONNECTION_UPDATED:
            envelope.data.le.type = BT_XSTACK_LE_EVENT_CONNECTION_UPDATED;
le_link:
            btstack_core_copy_address(&envelope.data.le.data.link.address,
                le_event->address);
            envelope.data.le.data.link.connection_handle = le_event->connection_handle;
            envelope.data.le.data.link.interval = le_event->connection_interval;
            envelope.data.le.data.link.latency = le_event->connection_latency;
            envelope.data.le.data.link.supervision_timeout = le_event->supervision_timeout;
            envelope.data.le.data.link.address_type = le_event->address_type;
            envelope.data.le.data.link.reason = le_event->reason;
            envelope.data.le.data.link.role = (bt_xstack_le_role_t)le_event->role;
            break;
        case BTSTACK_ADAPTER_LE_EVENT_JUST_WORKS_REQUEST:
        case BTSTACK_ADAPTER_LE_EVENT_NUMERIC_COMPARISON:
            envelope.data.le.type = BT_XSTACK_LE_EVENT_PAIRING_REQUEST;
            envelope.data.le.data.security.passkey = le_event->passkey;
            break;
        case BTSTACK_ADAPTER_LE_EVENT_PASSKEY_DISPLAY:
            envelope.data.le.type = BT_XSTACK_LE_EVENT_PASSKEY_DISPLAY;
            envelope.data.le.data.security.passkey = le_event->passkey;
            break;
        case BTSTACK_ADAPTER_LE_EVENT_PASSKEY_INPUT:
            envelope.data.le.type = BT_XSTACK_LE_EVENT_PASSKEY_INPUT;
            break;
        case BTSTACK_ADAPTER_LE_EVENT_PAIRING_COMPLETE:
            envelope.data.le.type = BT_XSTACK_LE_EVENT_PAIRING_COMPLETE;
            envelope.data.le.data.security.bonded = le_event->status == BTSTACK_STATUS_OK;
            break;
        case BTSTACK_ADAPTER_LE_EVENT_IDENTITY_RESOLVED:
        case BTSTACK_ADAPTER_LE_EVENT_REENCRYPTION_COMPLETE:
            envelope.data.le.type = BT_XSTACK_LE_EVENT_ENCRYPTION_CHANGED;
            envelope.data.le.data.security.encrypted = le_event->status == BTSTACK_STATUS_OK;
            envelope.data.le.data.security.bonded = le_event->status == BTSTACK_STATUS_OK;
            envelope.data.le.data.security.bond_index = le_event->bond_index;
            break;
        default:
            return;
        }
        (void)btstack_core_post_event(&envelope);
        break;
    }
    default:
        break;
    }
}

static void btstack_core_gatt_uuid(bt_xstack_uuid_t *destination, uint16_t uuid16,
    const uint8_t uuid128[16])
{
    memset(destination, 0, sizeof(*destination));
    if (uuid16 != 0u) {
        destination->type = BT_XSTACK_UUID_TYPE_16;
        destination->value.uuid16 = uuid16;
    } else {
        destination->type = BT_XSTACK_UUID_TYPE_128;
        memcpy(destination->value.uuid128, uuid128, 16u);
    }
}

void btstack_core_event_mapper_gatt_client(const btstack_gatt_client_event_t *event)
{
    btstack_core_event_envelope_t envelope;
    if (event == NULL) return;
    memset(&envelope, 0, sizeof(envelope));
    envelope.domain = BT_XSTACK_EVENT_DOMAIN_GATT;
    envelope.data.gatt.status = event->status;
    switch (event->type) {
    case BTSTACK_GATT_CLIENT_EVENT_SERVICE:
        envelope.data.gatt.type = BT_XSTACK_GATT_EVENT_SERVICE;
        envelope.data.gatt.data.service.start_handle = event->data.service.start_group_handle;
        envelope.data.gatt.data.service.end_handle = event->data.service.end_group_handle;
        btstack_core_gatt_uuid(&envelope.data.gatt.data.service.uuid,
            event->data.service.uuid16, event->data.service.uuid128);
        break;
    case BTSTACK_GATT_CLIENT_EVENT_CHARACTERISTIC:
        envelope.data.gatt.type = BT_XSTACK_GATT_EVENT_CHARACTERISTIC;
        envelope.data.gatt.data.characteristic.start_handle = event->data.characteristic.start_handle;
        envelope.data.gatt.data.characteristic.value_handle = event->data.characteristic.value_handle;
        envelope.data.gatt.data.characteristic.end_handle = event->data.characteristic.end_handle;
        envelope.data.gatt.data.characteristic.properties = event->data.characteristic.properties;
        btstack_core_gatt_uuid(&envelope.data.gatt.data.characteristic.uuid,
            event->data.characteristic.uuid16, event->data.characteristic.uuid128);
        break;
    case BTSTACK_GATT_CLIENT_EVENT_DESCRIPTOR:
        envelope.data.gatt.type = BT_XSTACK_GATT_EVENT_DESCRIPTOR;
        envelope.data.gatt.data.descriptor.handle = event->data.descriptor.handle;
        btstack_core_gatt_uuid(&envelope.data.gatt.data.descriptor.uuid,
            event->data.descriptor.uuid16, event->data.descriptor.uuid128);
        break;
    case BTSTACK_GATT_CLIENT_EVENT_VALUE:
        envelope.data.gatt.type = BT_XSTACK_GATT_EVENT_VALUE;
        goto gatt_value;
    case BTSTACK_GATT_CLIENT_EVENT_NOTIFICATION:
        envelope.data.gatt.type = BT_XSTACK_GATT_EVENT_NOTIFICATION;
        goto gatt_value;
    case BTSTACK_GATT_CLIENT_EVENT_INDICATION:
        envelope.data.gatt.type = BT_XSTACK_GATT_EVENT_INDICATION;
gatt_value:
        envelope.data.gatt.data.value.connection_handle = event->connection_handle;
        envelope.data.gatt.data.value.value_handle = event->value_handle;
        envelope.data.gatt.data.value.value_len = event->value_len;
        envelope.data.gatt.data.value.truncated = event->truncated;
        memcpy(envelope.data.gatt.data.value.value, event->data.value, event->value_len);
        break;
    case BTSTACK_GATT_CLIENT_EVENT_MTU:
        envelope.data.gatt.type = BT_XSTACK_GATT_EVENT_MTU;
        envelope.data.gatt.data.value.connection_handle = event->connection_handle;
        envelope.data.gatt.data.value.mtu = event->mtu;
        break;
    case BTSTACK_GATT_CLIENT_EVENT_COMPLETE:
        envelope.data.gatt.type = BT_XSTACK_GATT_EVENT_CLIENT_COMPLETE;
        envelope.data.gatt.data.value.operation = (uint8_t)event->operation;
        break;
    default:
        return;
    }
    (void)btstack_core_post_event(&envelope);
}

void btstack_core_event_mapper_gatt_server(const btstack_gatt_server_event_t *event)
{
#if BTSTACK_LE && BTSTACK_GATT_SERVER
    btstack_core_event_envelope_t envelope;
    if (event == NULL) return;
    memset(&envelope, 0, sizeof(envelope));
    envelope.domain = BT_XSTACK_EVENT_DOMAIN_GATT;
    envelope.data.gatt.status = event->status;
    envelope.data.gatt.data.value.connection_handle = event->connection_handle;
    envelope.data.gatt.data.value.value_handle = event->attribute_handle;
    envelope.data.gatt.data.value.value_len = event->value_len;
    envelope.data.gatt.data.value.cccd_value = event->cccd_value;
    switch (event->type) {
    case BTSTACK_GATT_SERVER_EVENT_VALUE_WRITTEN:
        envelope.data.gatt.type = BT_XSTACK_GATT_EVENT_SERVER_WRITE;
        envelope.data.gatt.data.value.value_len = BT_XSTACK_GATT_VALUE_MAX;
        if (btstack_gatt_server_get_value(event->attribute_handle,
                envelope.data.gatt.data.value.value,
                &envelope.data.gatt.data.value.value_len) != BTSTACK_STATUS_OK) {
            envelope.data.gatt.data.value.value_len = 0u;
        }
        break;
    case BTSTACK_GATT_SERVER_EVENT_SUBSCRIPTION_CHANGED:
        envelope.data.gatt.type = BT_XSTACK_GATT_EVENT_SUBSCRIPTION_CHANGED; break;
    case BTSTACK_GATT_SERVER_EVENT_INDICATION_COMPLETE:
        envelope.data.gatt.type = BT_XSTACK_GATT_EVENT_INDICATION_COMPLETE; break;
    default: return;
    }
    (void)btstack_core_post_event(&envelope);
#else
    (void)event;
#endif
}

void btstack_core_event_mapper_post_le_state(bt_xstack_le_event_type_t type, int status)
{
    btstack_core_event_envelope_t envelope;
    memset(&envelope, 0, sizeof(envelope));
    envelope.domain = BT_XSTACK_EVENT_DOMAIN_LE;
    envelope.data.le.type = type;
    envelope.data.le.status = status;
    (void)btstack_core_post_event(&envelope);
}

void btstack_core_event_mapper_post_gatt_server_result(uint8_t operation,
    int status, uint16_t handle)
{
    btstack_core_event_envelope_t envelope;
    memset(&envelope, 0, sizeof(envelope));
    envelope.domain = BT_XSTACK_EVENT_DOMAIN_GATT;
    envelope.data.gatt.type = BT_XSTACK_GATT_EVENT_SERVER_COMPLETE;
    envelope.data.gatt.status = status;
    envelope.data.gatt.data.value.operation = operation;
    envelope.data.gatt.data.value.value_handle = handle;
    (void)btstack_core_post_event(&envelope);
}

void btstack_core_event_mapper_post_gatt_server_value(uint16_t handle,
    int status, const uint8_t *value, uint16_t value_len)
{
    btstack_core_event_envelope_t envelope;
    uint16_t copy_len = value_len;
    if (copy_len > BT_XSTACK_GATT_VALUE_MAX) {
        copy_len = BT_XSTACK_GATT_VALUE_MAX;
    }
    memset(&envelope, 0, sizeof(envelope));
    envelope.domain = BT_XSTACK_EVENT_DOMAIN_GATT;
    envelope.data.gatt.type = BT_XSTACK_GATT_EVENT_SERVER_VALUE;
    envelope.data.gatt.status = status;
    envelope.data.gatt.data.value.value_handle = handle;
    envelope.data.gatt.data.value.value_len = copy_len;
    envelope.data.gatt.data.value.truncated = (value_len > copy_len) ? 1u : 0u;
    if ((value != NULL) && (copy_len != 0u)) {
        memcpy(envelope.data.gatt.data.value.value, value, copy_len);
    }
    (void)btstack_core_post_event(&envelope);
}

void btstack_core_event_set_scan_filter(const char *filter)
{
    btstack_platform_enter_critical();
    btstack_core_copy_text(
        btstack_core_scan_filter,
        sizeof(btstack_core_scan_filter),
        filter);
    btstack_core_scan_index = 0u;
    btstack_platform_exit_critical();
}

void btstack_core_event_mapper_post_connection(
    bt_xstack_profile_t profile,
    bt_xstack_connection_state_t state,
    const bt_xstack_addr_t *address,
    bool unexpected_loss)
{
    btstack_core_event_envelope_t envelope;
    memset(&envelope, 0, sizeof(envelope));
    envelope.domain = BT_XSTACK_EVENT_DOMAIN_CONNECTION;
    envelope.data.connection.profile = profile;
    envelope.data.connection.state = state;
    envelope.data.connection.unexpected_loss = unexpected_loss;
    if (address != NULL) memcpy(&envelope.data.connection.address, address, sizeof(*address));
    (void)btstack_core_post_event(&envelope);
}

void btstack_core_event_mapper_hfp_event(const btstack_hfp_hf_event_t *event)
{
    btstack_core_event_envelope_t envelope;
    btstack_hfp_hf_state_t state;
    uint8_t is_state = 1u;
    if (event == NULL) return;
    memset(&envelope, 0, sizeof(envelope));
    envelope.domain = BT_XSTACK_EVENT_DOMAIN_PHONE;
    btstack_core_copy_address(&envelope.data.phone.address, event->address);
    memcpy(envelope.data.phone.values, event->values, sizeof(envelope.data.phone.values));
    envelope.data.phone.value_count =
        (event->value_count <= 2u) ? event->value_count : 2u;
    btstack_core_copy_text(envelope.data.phone.text,
        sizeof(envelope.data.phone.text), event->text);
    switch (event->type) {
    case BTSTACK_HFP_HF_EVENT_CONNECTING:
        state = BTSTACK_HFP_HF_STATE_CONNECTING; break;
    case BTSTACK_HFP_HF_EVENT_CONNECTED:
    case BTSTACK_HFP_HF_EVENT_CALL_IDLE:
        state = BTSTACK_HFP_HF_STATE_CONNECTED; break;
    case BTSTACK_HFP_HF_EVENT_CALL_OUTGOING:
        state = BTSTACK_HFP_HF_STATE_OUTGOING; break;
    case BTSTACK_HFP_HF_EVENT_CALL_INCOMING:
        state = BTSTACK_HFP_HF_STATE_INCOMING; break;
    case BTSTACK_HFP_HF_EVENT_CALL_ACTIVE:
        state = BTSTACK_HFP_HF_STATE_ACTIVE; break;
    case BTSTACK_HFP_HF_EVENT_CALL_HELD:
        state = BTSTACK_HFP_HF_STATE_HELD; break;
    case BTSTACK_HFP_HF_EVENT_CALL_ACTIVE_WITH_INCOMING:
        state = BTSTACK_HFP_HF_STATE_ACTIVE_WITH_INCOMING; break;
    case BTSTACK_HFP_HF_EVENT_CALL_ACTIVE_WITH_HELD:
        state = BTSTACK_HFP_HF_STATE_ACTIVE_WITH_HELD; break;
    case BTSTACK_HFP_HF_EVENT_CALL_MULTIPARTY:
        state = BTSTACK_HFP_HF_STATE_MULTIPARTY; break;
    case BTSTACK_HFP_HF_EVENT_CONNECTION_LOST:
    case BTSTACK_HFP_HF_EVENT_DISCONNECTED:
        state = BTSTACK_HFP_HF_STATE_IDLE; break;
    case BTSTACK_HFP_HF_EVENT_AUDIO:
        is_state = 0u;
        envelope.data.phone.values[0] =
            (event->audio_state == BTSTACK_HFP_AUDIO_CONNECTED) ? 1u : 0u;
        envelope.data.phone.id = BT_XSTACK_PHONE_EVENT_AUDIO;
        envelope.data.phone.value_count = 1u;
        break;
    case BTSTACK_HFP_HF_EVENT_VOICE_RECOGNITION:
        is_state = 0u;
        envelope.data.phone.id = BT_XSTACK_PHONE_EVENT_VOICE_RECOGNITION;
        break;
    case BTSTACK_HFP_HF_EVENT_SIGNAL: is_state = 0u; envelope.data.phone.id = BT_XSTACK_PHONE_EVENT_SIGNAL; break;
    case BTSTACK_HFP_HF_EVENT_NETWORK: is_state = 0u; envelope.data.phone.id = BT_XSTACK_PHONE_EVENT_NETWORK; break;
    case BTSTACK_HFP_HF_EVENT_ROAM: is_state = 0u; envelope.data.phone.id = BT_XSTACK_PHONE_EVENT_ROAM; break;
    case BTSTACK_HFP_HF_EVENT_BATTERY: is_state = 0u; envelope.data.phone.id = BT_XSTACK_PHONE_EVENT_BATTERY; break;
    case BTSTACK_HFP_HF_EVENT_NUMBER: is_state = 0u; envelope.data.phone.id = BT_XSTACK_PHONE_EVENT_NUMBER; break;
    case BTSTACK_HFP_HF_EVENT_IN_BAND_RING: is_state = 0u; envelope.data.phone.id = BT_XSTACK_PHONE_EVENT_IN_BAND_RING; break;
    case BTSTACK_HFP_HF_EVENT_RING:
        is_state = 0u;
        envelope.data.phone.id = BT_XSTACK_PHONE_EVENT_RING;
        break;
    case BTSTACK_HFP_HF_EVENT_MULTIPARTY: is_state = 0u; envelope.data.phone.id = BT_XSTACK_PHONE_EVENT_MULTIPARTY; break;
    default: return;
    }
    if (is_state) {
        envelope.data.phone.id = BT_XSTACK_PHONE_EVENT_STATE;
        envelope.data.phone.values[0] = (uint32_t)state;
        envelope.data.phone.value_count = 1u;
        (void)btstack_core_post_event(&envelope);
        if (event->type == BTSTACK_HFP_HF_EVENT_CONNECTED) {
            envelope.data.phone.id = BT_XSTACK_PHONE_EVENT_DEVICE;
            envelope.data.phone.value_count = 0u;
        } else if (event->type == BTSTACK_HFP_HF_EVENT_CONNECTION_LOST) {
            envelope.data.phone.id = BT_XSTACK_PHONE_EVENT_LOST;
            envelope.data.phone.value_count = 0u;
        } else {
            return;
        }
    }
    (void)btstack_core_post_event(&envelope);
}

static void btstack_core_copy_pbap_contact(
    bt_xstack_phonebook_contact_t *destination,
    const btstack_vcard_record_t *source)
{
    uint8_t index;
    uint8_t count = source->number_count;

    if (count > BT_XSTACK_PHONEBOOK_NUMBER_COUNT) {
        count = BT_XSTACK_PHONEBOOK_NUMBER_COUNT;
    }
    btstack_core_copy_text(destination->name,
        sizeof(destination->name), source->name);
    destination->number_count = count;
    destination->truncated = source->truncated;
    for (index = 0u; index < count; index++) {
        btstack_core_copy_text(destination->numbers[index].value,
            sizeof(destination->numbers[index].value),
            source->numbers[index].value);
        destination->numbers[index].type_flags =
            source->numbers[index].type_flags;
    }
}

static void btstack_core_copy_pbap_call(
    bt_xstack_phonebook_call_t *destination,
    const btstack_vcard_record_t *source)
{
    btstack_core_copy_text(destination->name,
        sizeof(destination->name), source->name);
    if (source->number_count != 0u) {
        btstack_core_copy_text(destination->number,
            sizeof(destination->number), source->numbers[0].value);
    }
    btstack_core_copy_text(destination->timestamp,
        sizeof(destination->timestamp), source->call_timestamp);
    destination->direction = (uint8_t)source->direction;
    destination->truncated = source->truncated;
}

uint8_t btstack_core_event_mapper_pbap_event(
    const btstack_pbap_client_event_t *event)
{
    btstack_core_event_envelope_t envelope;
    int post_result;

    if (event == NULL) {
        return 1u;
    }
    memset(&envelope, 0, sizeof(envelope));
    envelope.domain = BT_XSTACK_EVENT_DOMAIN_PHONEBOOK;
    btstack_core_copy_address(
        &envelope.data.phonebook.address, event->peer);
    envelope.data.phonebook.category =
        (bt_xstack_phonebook_category_t)event->category;
    envelope.data.phonebook.status =
        (bt_xstack_phonebook_status_t)event->status;
    envelope.data.phonebook.parser_error_count =
        event->parser_error_count;
    switch (event->type) {
    case BTSTACK_PBAP_CLIENT_EVENT_STARTED:
        envelope.data.phonebook.id = BT_XSTACK_PHONEBOOK_EVENT_STARTED;
        break;
    case BTSTACK_PBAP_CLIENT_EVENT_CONTACT:
        envelope.data.phonebook.id = BT_XSTACK_PHONEBOOK_EVENT_CONTACT;
        btstack_core_copy_pbap_contact(
            &envelope.data.phonebook.data.contact, &event->record);
        break;
    case BTSTACK_PBAP_CLIENT_EVENT_CALL:
        envelope.data.phonebook.id = BT_XSTACK_PHONEBOOK_EVENT_CALL;
        btstack_core_copy_pbap_call(
            &envelope.data.phonebook.data.call, &event->record);
        break;
    case BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED:
        envelope.data.phonebook.id =
            BT_XSTACK_PHONEBOOK_EVENT_CATEGORY_COMPLETED;
        break;
    case BTSTACK_PBAP_CLIENT_EVENT_COMPLETED:
        envelope.data.phonebook.id = BT_XSTACK_PHONEBOOK_EVENT_COMPLETED;
        break;
    case BTSTACK_PBAP_CLIENT_EVENT_AUTHENTICATION_REQUIRED:
        envelope.data.phonebook.id = BT_XSTACK_PHONEBOOK_EVENT_AUTH_REQUIRED;
        break;
    case BTSTACK_PBAP_CLIENT_EVENT_ERROR:
        envelope.data.phonebook.id = BT_XSTACK_PHONEBOOK_EVENT_ERROR;
        break;
    default:
        return 1u;
    }

    post_result = btstack_core_post_event(&envelope);
    if ((event->type == BTSTACK_PBAP_CLIENT_EVENT_CONTACT) ||
        (event->type == BTSTACK_PBAP_CLIENT_EVENT_CALL)) {
        if (post_result != 0) {
            return 0u;
        }
    }
    return 1u;
}

static uint8_t btstack_core_a2dp_state_value(btstack_a2dp_sink_state_t state)
{
    switch (state) {
    case BTSTACK_A2DP_SINK_STATE_IDLE: return 1u;
    case BTSTACK_A2DP_SINK_STATE_CONNECTING: return 2u;
    case BTSTACK_A2DP_SINK_STATE_CONNECTED: return 3u;
    case BTSTACK_A2DP_SINK_STATE_STREAM_OPEN:
    case BTSTACK_A2DP_SINK_STATE_SUSPENDED: return 4u;
    case BTSTACK_A2DP_SINK_STATE_STREAMING: return 5u;
    default: return 1u;
    }
}

static int btstack_core_a2dp_event_state(
    btstack_a2dp_sink_event_type_t type,
    btstack_a2dp_sink_state_t *state)
{
    if (state == NULL) return 0;
    switch (type) {
    case BTSTACK_A2DP_SINK_EVENT_CONNECTING:
        *state = BTSTACK_A2DP_SINK_STATE_CONNECTING; return 1;
    case BTSTACK_A2DP_SINK_EVENT_CONNECTED:
    case BTSTACK_A2DP_SINK_EVENT_STREAM_CLOSED:
        *state = BTSTACK_A2DP_SINK_STATE_CONNECTED; return 1;
    case BTSTACK_A2DP_SINK_EVENT_STREAM_OPEN:
        *state = BTSTACK_A2DP_SINK_STATE_STREAM_OPEN; return 1;
    case BTSTACK_A2DP_SINK_EVENT_STREAMING:
        *state = BTSTACK_A2DP_SINK_STATE_STREAMING; return 1;
    case BTSTACK_A2DP_SINK_EVENT_SUSPENDED:
        *state = BTSTACK_A2DP_SINK_STATE_SUSPENDED; return 1;
    case BTSTACK_A2DP_SINK_EVENT_CONNECTION_LOST:
    case BTSTACK_A2DP_SINK_EVENT_DISCONNECTED:
        *state = BTSTACK_A2DP_SINK_STATE_IDLE; return 1;
    default:
        return 0;
    }
}

void btstack_core_event_mapper_a2dp_event(
    const btstack_a2dp_sink_event_t *event)
{
    btstack_core_event_envelope_t envelope;
    btstack_a2dp_sink_state_t state;
    uint8_t value;
    if (event == NULL) return;
    memset(&envelope, 0, sizeof(envelope));
    envelope.domain = BT_XSTACK_EVENT_DOMAIN_AUDIO;
    btstack_core_copy_address(&envelope.data.audio.address, event->address);
    if (btstack_core_a2dp_event_state(event->type, &state)) {
        value = btstack_core_a2dp_state_value(state);
        envelope.data.audio.id = BT_XSTACK_AUDIO_EVENT_A2DP_STATE;
        envelope.data.audio.values[0] = value;
        envelope.data.audio.value_count = 1u;
        (void)btstack_core_post_event(&envelope);
        if (event->type == BTSTACK_A2DP_SINK_EVENT_CONNECTED) {
            envelope.data.audio.id = BT_XSTACK_AUDIO_EVENT_A2DP_DEVICE;
            envelope.data.audio.value_count = 0u;
        } else if (event->type ==
                   BTSTACK_A2DP_SINK_EVENT_CONNECTION_LOST) {
            envelope.data.audio.id = BT_XSTACK_AUDIO_EVENT_A2DP_LOST;
            envelope.data.audio.value_count = 0u;
        } else {
            return;
        }
        (void)btstack_core_post_event(&envelope);
        return;
    }
    switch (event->type) {
    case BTSTACK_A2DP_SINK_EVENT_PCM_CONFIG:
        envelope.data.audio.id = BT_XSTACK_AUDIO_EVENT_PCM_CONFIG;
        envelope.data.audio.values[0] = event->sample_rate;
        envelope.data.audio.values[1] = event->channel_count;
        envelope.data.audio.values[2] = event->bits_per_sample;
        envelope.data.audio.value_count = 3u;
        break;
    default: return;
    }
    (void)btstack_core_post_event(&envelope);
}

static uint8_t btstack_core_avrcp_state_value(btstack_avrcp_state_t state)
{
    if (state == BTSTACK_AVRCP_STATE_CONNECTING) return 2u;
    if (state == BTSTACK_AVRCP_STATE_CONNECTED) return 3u;
    return 1u;
}

void btstack_core_event_mapper_avrcp_event(
    const btstack_avrcp_event_t *event)
{
    btstack_core_event_envelope_t envelope;
    uint8_t value;
    if (event == NULL) return;
    memset(&envelope, 0, sizeof(envelope));
    envelope.domain = BT_XSTACK_EVENT_DOMAIN_AUDIO;
    btstack_core_copy_address(&envelope.data.audio.address, event->address);
    memcpy(envelope.data.audio.values, event->values, sizeof(envelope.data.audio.values));
    envelope.data.audio.value_count =
        (event->value_count <= 3u) ? event->value_count : 3u;
    if (event->type <= BTSTACK_AVRCP_EVENT_CONNECTED) {
        value = btstack_core_avrcp_state_value(
            (btstack_avrcp_state_t)event->type);
        envelope.data.audio.id = BT_XSTACK_AUDIO_EVENT_AVRCP_STATE;
        envelope.data.audio.values[0] = value;
        envelope.data.audio.value_count = 1u;
        (void)btstack_core_post_event(&envelope);
        return;
    }
    switch (event->type) {
    case BTSTACK_AVRCP_EVENT_PLAY_STATUS:
        envelope.data.audio.id = BT_XSTACK_AUDIO_EVENT_PLAY_STATE;
        break;
    case BTSTACK_AVRCP_EVENT_PLAY_MODE:
        envelope.data.audio.id = BT_XSTACK_AUDIO_EVENT_PLAY_MODE;
        break;
    case BTSTACK_AVRCP_EVENT_TRACK_CHANGED:
        return;
    case BTSTACK_AVRCP_EVENT_TRACK_STATUS:
        envelope.data.audio.id = BT_XSTACK_AUDIO_EVENT_TRACK_STATE;
        break;
    case BTSTACK_AVRCP_EVENT_TRACK_INFO:
        envelope.data.audio.id = BT_XSTACK_AUDIO_EVENT_TRACK_INFO;
        btstack_core_copy_text(envelope.data.audio.title,sizeof(envelope.data.audio.title),event->title);
        btstack_core_copy_text(envelope.data.audio.artist,sizeof(envelope.data.audio.artist),event->artist);
        btstack_core_copy_text(envelope.data.audio.album,sizeof(envelope.data.audio.album),event->album);
        break;
    case BTSTACK_AVRCP_EVENT_ABSOLUTE_VOLUME:
        envelope.data.audio.id = BT_XSTACK_AUDIO_EVENT_ABSOLUTE_VOLUME;
        break;
    default: return;
    }
    (void)btstack_core_post_event(&envelope);
}

void btstack_core_event_mapper_media_audio(
    const btstack_audio_sample_t *sample)
{
    if ((sample == NULL) ||
        (sample->format != BTSTACK_AUDIO_FORMAT_PCM_S16) ||
        (sample->data == NULL) || (sample->data_len == 0u)) {
        return;
    }
    btstack_core_media_audio_deliver(sample->data, sample->data_len);
}

void btstack_core_event_mapper_call_audio(
    const uint8_t *data,
    uint16_t length)
{
    if ((data == NULL) || (length == 0u)) {
        return;
    }
    btstack_core_call_audio_deliver(data, length);
}

void btstack_core_event_post_microphone_mute(bool muted)
{
    btstack_core_event_envelope_t envelope;
    memset(&envelope, 0, sizeof(envelope));
    envelope.domain = BT_XSTACK_EVENT_DOMAIN_PHONE;
    envelope.data.phone.id = BT_XSTACK_PHONE_EVENT_MICROPHONE_MUTE;
    envelope.data.phone.values[0] = muted ? 1u : 0u;
    envelope.data.phone.value_count = 1u;
    (void)btstack_core_post_event(&envelope);
}

void btstack_core_event_mapper_cover_art(const btstack_bip_event_t *event)
{
    bt_xstack_cover_art_event_t mapped;

    if (event == NULL) return;
    memset(&mapped, 0, sizeof(mapped));
    mapped.status = event->status;
    mapped.generation = event->generation;
    switch (event->type) {
    case BTSTACK_BIP_EVENT_STARTED:
        mapped.type = BT_XSTACK_COVER_ART_STARTED;
        memcpy(mapped.data.started.address, event->data.started.address, sizeof(mapped.data.started.address));
        memcpy(mapped.data.started.image_handle, event->data.started.image_handle, sizeof(mapped.data.started.image_handle));
        mapped.data.started.operation = (uint8_t)event->operation;
        break;
    case BTSTACK_BIP_EVENT_PROPERTY:
        mapped.type = BT_XSTACK_COVER_ART_PROPERTY;
        mapped.data.property = event->data.property;
        break;
    case BTSTACK_BIP_EVENT_PROPERTIES_COMPLETED:
        mapped.type = BT_XSTACK_COVER_ART_PROPERTIES_COMPLETED;
        mapped.data.properties_completed.property_count = event->data.properties_completed.property_count;
        break;
    case BTSTACK_BIP_EVENT_DATA:
        mapped.type = BT_XSTACK_COVER_ART_DATA;
        mapped.data.data.data = event->data.data.data;
        mapped.data.data.length = event->data.data.length;
        mapped.data.data.offset = event->data.data.offset;
        break;
    case BTSTACK_BIP_EVENT_COMPLETED:
        mapped.type = BT_XSTACK_COVER_ART_COMPLETED;
        mapped.data.completed.total_length = event->data.completed.total_length;
        break;
    case BTSTACK_BIP_EVENT_ERROR:
        mapped.type = BT_XSTACK_COVER_ART_ERROR;
        mapped.data.error.operation = (uint8_t)event->operation;
        mapped.data.error.native_status = event->native_status;
        break;
    default:
        return;
    }
    btstack_frontend_deliver_cover_art(&mapped);
}

int btstack_core_event_mapper_init(void)
{
    int status;
    memset(&btstack_core_state, 0, sizeof(btstack_core_state));
    btstack_core_state.profile_mask = BT_XSTACK_PROFILE_MASK_SUPPORTED;
    btstack_core_copy_text(btstack_core_state.pin,
        sizeof(btstack_core_state.pin), "0000");

    status = btstack_adapter_set_event_callback(
        btstack_core_adapter_event);
    if (status != BTSTACK_STATUS_OK) return status;
    return BTSTACK_STATUS_OK;
}

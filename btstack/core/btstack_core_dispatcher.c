#include "btstack_core_internal.h"

#include <string.h>

#include "adapter/btstack_adapter.h"
#include "adapter/btstack_types.h"
#include "btstack_run_loop.h"
#include "core/btstack_profile_manager.h"
#include "port/btstack_platform.h"
#include "profiles/btstack_avrcp.h"
#include "profiles/btstack_avrcp_cover_art.h"
#include "profiles/btstack_hfp_hf.h"
#include "profiles/btstack_pbap_client.h"
#include "profiles/btstack_gatt_client.h"
#include "profiles/btstack_gatt_server.h"

enum {
    BT_XSTACK_COMMAND_SLOT_FREE = 0,
    BT_XSTACK_COMMAND_SLOT_QUEUED,
    BT_XSTACK_COMMAND_SLOT_RUNNING
};

typedef struct {
    btstack_core_request_t request;
    btstack_context_callback_registration_t callback;
    uint8_t state;
} btstack_core_command_slot_t;

static btstack_core_command_slot_t
    btstack_core_command_slots[BT_XSTACK_COMMAND_SLOT_COUNT];
static uint32_t btstack_core_stack_command_count;
static uint32_t btstack_core_stack_command_failure_count;
static uint32_t btstack_core_command_slots_in_use;
static uint32_t btstack_core_command_slots_max;

static btstack_core_command_slot_t *
btstack_core_command_slot_acquire(void)
{
    uint8_t index;
    btstack_core_command_slot_t *slot = NULL;

    btstack_platform_enter_critical();
    for (index = 0u; index < BT_XSTACK_COMMAND_SLOT_COUNT; index++) {
        if (btstack_core_command_slots[index].state ==
            BT_XSTACK_COMMAND_SLOT_FREE) {
            slot = &btstack_core_command_slots[index];
            slot->state = BT_XSTACK_COMMAND_SLOT_QUEUED;
            btstack_core_command_slots_in_use++;
            if (btstack_core_command_slots_in_use >
                btstack_core_command_slots_max) {
                btstack_core_command_slots_max =
                    btstack_core_command_slots_in_use;
            }
            break;
        }
    }
    btstack_platform_exit_critical();
    return slot;
}

static void btstack_core_command_slot_release(
    btstack_core_command_slot_t *slot)
{
    if (slot == NULL) {
        return;
    }
    btstack_platform_enter_critical();
    slot->state = BT_XSTACK_COMMAND_SLOT_FREE;
    if (btstack_core_command_slots_in_use != 0u) {
        btstack_core_command_slots_in_use--;
    }
    btstack_platform_exit_critical();
    btstack_core_command_slot_released();
}

static void btstack_core_copy_native_address(
    uint8_t destination[6],
    const bt_xstack_addr_t *source)
{
    memcpy(destination, source->bytes, 6u);
}

static void btstack_core_native_uuid(btstack_uuid_t *destination,
    const bt_xstack_uuid_t *source)
{
    memset(destination, 0, sizeof(*destination));
    if (source->type == BT_XSTACK_UUID_TYPE_16) {
        destination->type = BTSTACK_UUID_TYPE_16;
        destination->value.uuid16 = source->value.uuid16;
    } else {
        destination->type = BTSTACK_UUID_TYPE_128;
        memcpy(destination->value.uuid128, source->value.uuid128, 16u);
    }
}

static int btstack_core_execute_local_request(
    const btstack_core_request_t *request,
    int *handled)
{
    bt_xstack_state_t state;

    if ((request == NULL) || (handled == NULL)) {
        return -1;
    }
    *handled = 1;
    switch (request->id) {
    case BT_XSTACK_REQUEST_SET_PROFILE_MASK:
    case BT_XSTACK_REQUEST_SET_TDL:
    case BT_XSTACK_REQUEST_SET_HFP_CONFIG:
    case BT_XSTACK_REQUEST_SET_A2DP_MUTE:
        btstack_core_state_set_value(
            request->id, request->values[0]);
        return 0;
    case BT_XSTACK_REQUEST_CLEAR_PEER_INDEX:
        return btstack_core_state_clear_peer_index(
            (uint8_t)request->values[0]);
    case BT_XSTACK_REQUEST_CLEAR_PEER_ADDRESS:
        return btstack_core_state_clear_peer_address(
            &request->address);
    case BT_XSTACK_REQUEST_CLEAR_PEERS:
        btstack_core_state_clear_peers();
        return 0;
    case BT_XSTACK_REQUEST_SET_MICROPHONE_MUTE:
        if ((btstack_core_copy_state(&state) != 0) ||
            (state.hfp_state < BT_XSTACK_HFP_STATE_ACTIVE) ||
            (state.hfp_state > BT_XSTACK_HFP_STATE_MULTIPARTY)) {
            return -1;
        }
        btstack_core_state_set_value(
            request->id, request->values[0]);
        btstack_core_event_post_microphone_mute(
            request->values[0] != 0u);
        return 0;
    default:
        *handled = 0;
        return 0;
    }
}

static int btstack_core_prepare_stack_request(btstack_core_request_t *prepared,
    const btstack_core_request_t *request)
{
    if ((prepared == NULL) || (request == NULL)) {
        return -1;
    }
    memcpy(prepared, request, sizeof(*prepared));
    return 0;
}

static int btstack_core_execute_stack_request(
    const btstack_core_request_t *request)
{
    uint8_t address[6];
    int status;
    uint16_t result_handle = 0u;
    btstack_profile_manager_le_state_t le_state;
    btstack_uuid_t native_uuid;

    switch (request->id) {
    case BT_XSTACK_REQUEST_PBAP_SYNC:
        btstack_core_copy_native_address(address, &request->address);
        status = btstack_pbap_client_sync(address,
            (btstack_pbap_client_category_mask_t)request->values[0]);
        btstack_core_state_write_pbap((bt_xstack_phonebook_state_t)
            btstack_pbap_client_get_state());
        return status;
    case BT_XSTACK_REQUEST_PBAP_CANCEL:
        status = btstack_pbap_client_cancel();
        btstack_core_state_write_pbap((bt_xstack_phonebook_state_t)
            btstack_pbap_client_get_state());
        return status;
    case BT_XSTACK_REQUEST_PBAP_AUTHENTICATE:
        status = btstack_pbap_client_authenticate(request->text);
        btstack_core_state_write_pbap((bt_xstack_phonebook_state_t)
            btstack_pbap_client_get_state());
        return status;
    case BT_XSTACK_REQUEST_PBAP_EVENT_SPACE_AVAILABLE:
        btstack_pbap_client_event_space_available();
        btstack_core_pbap_event_space_request_completed();
        return BTSTACK_STATUS_OK;
    case BT_XSTACK_REQUEST_PBAP_PEER_DISCONNECTED:
        btstack_core_copy_native_address(address, &request->address);
        btstack_pbap_client_peer_disconnected(address);
        btstack_core_state_write_pbap((bt_xstack_phonebook_state_t)
            btstack_pbap_client_get_state());
        return BTSTACK_STATUS_OK;
    case BT_XSTACK_REQUEST_COVER_ART_SET_MODE:
        return btstack_avrcp_cover_art_set_mode(request->cover_art.mode);
    case BT_XSTACK_REQUEST_COVER_ART_GET_IMAGE:
        return btstack_avrcp_cover_art_get_image(&request->cover_art.descriptor);
    case BT_XSTACK_REQUEST_SET_POWER:
        status = btstack_adapter_set_power(request->values[0] != 0u);
        if (status == BTSTACK_STATUS_OK) {
            btstack_core_state_sync_gap();
        }
        return status;
    case BT_XSTACK_REQUEST_SET_NAME:
        status = request->values[0] != 0u ?
            btstack_adapter_set_name_with_address_suffix(request->text) :
            btstack_adapter_set_name(request->text);
        if (status == BTSTACK_STATUS_OK) {
            btstack_core_state_sync_gap();
        }
        return status;
    case BT_XSTACK_REQUEST_SET_PIN:
        status = btstack_adapter_set_pin(request->text);
        if (status == BTSTACK_STATUS_OK) {
            btstack_core_state_sync_gap();
        }
        return status;
    case BT_XSTACK_REQUEST_SET_SSP:
        status = btstack_adapter_set_ssp_mode((uint8_t)request->values[0]);
        if (status == BTSTACK_STATUS_OK) {
            btstack_core_state_sync_gap();
        }
        return status;
    case BT_XSTACK_REQUEST_PAIR_CONFIRM:
        btstack_core_copy_native_address(address, &request->address);
        return btstack_adapter_pairing_confirm(address,
            request->values[0] != 0u);
    case BT_XSTACK_REQUEST_SET_DISCOVERABLE:
        status = btstack_adapter_set_discoverable(request->values[0] != 0u);
        if (status == BTSTACK_STATUS_OK) {
            btstack_core_state_sync_gap();
        }
        return status;
    case BT_XSTACK_REQUEST_SET_CONNECTABLE:
        status = btstack_adapter_set_connectable(request->values[0] != 0u);
        if (status == BTSTACK_STATUS_OK) {
            btstack_core_state_sync_gap();
        }
        return status;
    case BT_XSTACK_REQUEST_SET_COD:
        status = btstack_adapter_set_class_of_device(request->values[0]);
        if (status == BTSTACK_STATUS_OK) {
            btstack_core_state_sync_gap();
        }
        return status;
    case BT_XSTACK_REQUEST_SCAN_START:
        btstack_core_event_set_scan_filter(request->text);
        status = btstack_adapter_scan_start((uint8_t)request->values[0]);
        if (status == BTSTACK_STATUS_OK) {
            btstack_core_state_sync_gap();
        }
        return status;
    case BT_XSTACK_REQUEST_SCAN_STOP:
        status = btstack_adapter_scan_stop();
        if (status == BTSTACK_STATUS_OK) {
            btstack_core_state_sync_gap();
        }
        return status;
    case BT_XSTACK_REQUEST_DISCONNECT_ALL:
        return btstack_profile_manager_disconnect_all();
    case BT_XSTACK_REQUEST_CONNECT:
    case BT_XSTACK_REQUEST_CONNECT_LAST:
        return btstack_profile_manager_connect(request->profile,
            request->has_address ? &request->address : NULL);
    case BT_XSTACK_REQUEST_DISCONNECT_PROFILE:
        return btstack_profile_manager_disconnect(request->profile);
    case BT_XSTACK_REQUEST_HFP_DIAL:
        return btstack_hfp_hf_dial_number(request->text);
    case BT_XSTACK_REQUEST_HFP_REDIAL:
        return btstack_hfp_hf_redial();
    case BT_XSTACK_REQUEST_HFP_DTMF:
        return btstack_hfp_hf_send_dtmf(request->text[0]);
    case BT_XSTACK_REQUEST_HFP_CONTROL:
        return btstack_hfp_hf_execute_control(request->values[0],
            request->values[1]);
    case BT_XSTACK_REQUEST_SET_SPEAKER_VOLUME:
        status = btstack_hfp_hf_set_speaker_gain(
            (uint8_t)request->values[0]);
        if (status == BTSTACK_STATUS_OK) {
            btstack_core_state_set_value(
                request->id, request->values[0]);
        }
        return status;
    case BT_XSTACK_REQUEST_AVRCP_CONTROL:
        return btstack_avrcp_execute_control(request->values[0]);
    case BT_XSTACK_REQUEST_SET_REPEAT:
        status = btstack_avrcp_set_repeat(request->values[0] != 0u);
        if (status == BTSTACK_STATUS_OK) {
            btstack_core_state_set_value(
                request->id, request->values[0]);
        }
        return status;
    case BT_XSTACK_REQUEST_SET_SHUFFLE:
        status = btstack_avrcp_set_shuffle(request->values[0] != 0u);
        if (status == BTSTACK_STATUS_OK) {
            btstack_core_state_set_value(
                request->id, request->values[0]);
        }
        return status;
    case BT_XSTACK_REQUEST_LE_ADV_CONFIGURE:
    case BT_XSTACK_REQUEST_LE_ADV_CONFIGURE_START: {
        btstack_adapter_le_adv_config_t config;
        memset(&config, 0, sizeof(config));
        config.interval_min = request->le_adv.interval_min;
        config.interval_max = request->le_adv.interval_max;
        config.advertising_type = request->le_adv.advertising_type;
        config.own_address_type = request->le_adv.own_address_type;
        config.channel_map = request->le_adv.channel_map;
        config.filter_policy = request->le_adv.filter_policy;
        config.data_len = request->le_adv.data_len;
        config.scan_response_len = request->le_adv.scan_response_len;
        memcpy(config.data, request->le_adv.data, config.data_len);
        memcpy(config.scan_response, request->le_adv.scan_response,
            config.scan_response_len);
        status = btstack_adapter_le_adv_configure(&config);
        if ((status == BTSTACK_STATUS_OK) &&
            (request->id == BT_XSTACK_REQUEST_LE_ADV_CONFIGURE_START)) {
            status = btstack_adapter_le_adv_start();
            btstack_core_event_mapper_post_le_state(
                BT_XSTACK_LE_EVENT_ADV_STARTED, status);
        }
        return status;
    }
    case BT_XSTACK_REQUEST_LE_ADV_START:
        status = btstack_adapter_le_adv_start();
        btstack_core_event_mapper_post_le_state(BT_XSTACK_LE_EVENT_ADV_STARTED, status);
        return status;
    case BT_XSTACK_REQUEST_LE_ADV_STOP:
        status = btstack_adapter_le_adv_stop();
        btstack_core_event_mapper_post_le_state(BT_XSTACK_LE_EVENT_ADV_STOPPED, status);
        return status;
    case BT_XSTACK_REQUEST_LE_SCAN_START: {
        btstack_adapter_le_scan_config_t config;
        config.interval = request->le_scan.interval;
        config.window = request->le_scan.window;
        config.active = request->le_scan.active ? 1u : 0u;
        config.duplicate_filter = request->le_scan.duplicate_filter ? 1u : 0u;
        return btstack_adapter_le_scan_start(&config);
    }
    case BT_XSTACK_REQUEST_LE_SCAN_STOP:
        status = btstack_adapter_le_scan_stop();
        btstack_core_event_mapper_post_le_state(BT_XSTACK_LE_EVENT_SCAN_STOPPED, status);
        return status;
    case BT_XSTACK_REQUEST_LE_CONNECT:
        btstack_core_copy_native_address(address, &request->le_connect.address);
        status = btstack_adapter_le_connect(request->le_connect.address_type, address);
        btstack_core_event_mapper_post_le_state(BT_XSTACK_LE_EVENT_CONNECTING, status);
        return status;
    case BT_XSTACK_REQUEST_LE_CONNECT_CANCEL:
        return btstack_adapter_le_connect_cancel();
    case BT_XSTACK_REQUEST_LE_DISCONNECT:
        return btstack_adapter_le_disconnect();
    case BT_XSTACK_REQUEST_LE_PAIR:
        return btstack_adapter_le_pair();
    case BT_XSTACK_REQUEST_LE_PAIRING_CONFIRM:
        if (btstack_profile_manager_get_le_state(&le_state) != BTSTACK_STATUS_OK) {
            return BTSTACK_STATUS_NOT_READY;
        }
        return btstack_adapter_le_pairing_confirm(
            le_state.connection_handle, request->values[0] != 0u);
    case BT_XSTACK_REQUEST_LE_PASSKEY_REPLY:
        if (btstack_profile_manager_get_le_state(&le_state) != BTSTACK_STATUS_OK) {
            return BTSTACK_STATUS_NOT_READY;
        }
        return btstack_adapter_le_passkey_reply(
            le_state.connection_handle, request->values[0]);
    case BT_XSTACK_REQUEST_LE_BOND_DELETE:
        return btstack_adapter_le_bond_delete((uint8_t)request->values[0]);
    case BT_XSTACK_REQUEST_LE_BOND_DELETE_ALL:
        return btstack_adapter_le_bond_delete_all();
    case BT_XSTACK_REQUEST_GATT_SERVER_ADD_SERVICE:
        btstack_core_native_uuid(&native_uuid, &request->gatt_registration.uuid);
        status = btstack_gatt_server_add_service(&native_uuid, &result_handle);
        btstack_core_event_mapper_post_gatt_server_result((uint8_t)request->id,
            status, result_handle);
        return status;
    case BT_XSTACK_REQUEST_GATT_SERVER_ADD_CHARACTERISTIC: {
        btstack_gatt_server_characteristic_config_t config;
        btstack_core_native_uuid(&config.uuid, &request->gatt_registration.config.uuid);
        config.properties = request->gatt_registration.config.properties;
        config.max_len = request->gatt_registration.config.max_len;
        config.read_security = (btstack_gatt_security_t)
            request->gatt_registration.config.read_security;
        config.write_security = (btstack_gatt_security_t)
            request->gatt_registration.config.write_security;
        status = btstack_gatt_server_add_characteristic(
            request->gatt_registration.parent_handle, &config, &result_handle);
        btstack_core_event_mapper_post_gatt_server_result((uint8_t)request->id,
            status, result_handle);
        return status;
    }
    case BT_XSTACK_REQUEST_GATT_SERVER_ADD_DESCRIPTOR: {
        btstack_gatt_server_descriptor_config_t config;
        btstack_core_native_uuid(&config.uuid, &request->gatt_registration.config.uuid);
        config.properties = request->gatt_registration.config.properties;
        config.max_len = request->gatt_registration.config.max_len;
        config.read_security = (btstack_gatt_security_t)
            request->gatt_registration.config.read_security;
        config.write_security = (btstack_gatt_security_t)
            request->gatt_registration.config.write_security;
        status = btstack_gatt_server_add_descriptor(
            request->gatt_registration.parent_handle, &config, &result_handle);
        btstack_core_event_mapper_post_gatt_server_result((uint8_t)request->id,
            status, result_handle);
        return status;
    }
    case BT_XSTACK_REQUEST_GATT_SERVER_BUILD:
        status = btstack_gatt_server_build();
        btstack_core_event_mapper_post_gatt_server_result((uint8_t)request->id,
            status, 0u);
        return status;
    case BT_XSTACK_REQUEST_GATT_SERVER_SET_VALUE:
    case BT_XSTACK_REQUEST_GATT_SERVER_NOTIFY:
    case BT_XSTACK_REQUEST_GATT_SERVER_INDICATE:
        if (request->id == BT_XSTACK_REQUEST_GATT_SERVER_SET_VALUE) {
            status = btstack_gatt_server_set_value(request->gatt.handle,
                request->gatt.value, request->gatt.value_len);
        } else if (request->id == BT_XSTACK_REQUEST_GATT_SERVER_NOTIFY) {
            status = btstack_gatt_server_notify(request->gatt.handle,
                request->gatt.value, request->gatt.value_len);
        } else {
            status = btstack_gatt_server_indicate(request->gatt.handle,
                request->gatt.value, request->gatt.value_len);
        }
        btstack_core_event_mapper_post_gatt_server_result((uint8_t)request->id,
            status, request->gatt.handle);
        return status;
    case BT_XSTACK_REQUEST_GATT_SERVER_GET_VALUE: {
        uint8_t value[BT_XSTACK_GATT_VALUE_MAX];
        uint16_t value_len = sizeof(value);
        status = btstack_gatt_server_get_value(request->gatt.handle,
            value, &value_len);
        btstack_core_event_mapper_post_gatt_server_value(
            request->gatt.handle, status, value,
            (status == BTSTACK_STATUS_OK) ? value_len : 0u);
        return status;
    }
    case BT_XSTACK_REQUEST_GATT_CLIENT_DISCOVER_SERVICES:
        if (request->gatt.flags) btstack_core_native_uuid(&native_uuid, &request->gatt.uuid);
        return btstack_gatt_client_discover_services(
            request->gatt.flags ? &native_uuid : NULL);
    case BT_XSTACK_REQUEST_GATT_CLIENT_DISCOVER_CHARACTERISTICS:
        if (request->gatt.flags) btstack_core_native_uuid(&native_uuid, &request->gatt.uuid);
        return btstack_gatt_client_discover_characteristics(request->gatt.handle,
            request->gatt.end_handle, request->gatt.flags ? &native_uuid : NULL);
    case BT_XSTACK_REQUEST_GATT_CLIENT_DISCOVER_DESCRIPTORS: {
        gatt_client_characteristic_t characteristic;
        memset(&characteristic, 0, sizeof(characteristic));
        characteristic.start_handle = request->gatt.characteristic.start_handle;
        characteristic.value_handle = request->gatt.characteristic.value_handle;
        characteristic.end_handle = request->gatt.characteristic.end_handle;
        characteristic.properties = request->gatt.characteristic.properties;
        characteristic.uuid16 = request->gatt.characteristic.uuid.type ==
            BT_XSTACK_UUID_TYPE_16 ? request->gatt.characteristic.uuid.value.uuid16 : 0u;
        if (request->gatt.characteristic.uuid.type == BT_XSTACK_UUID_TYPE_128) {
            memcpy(characteristic.uuid128,
                request->gatt.characteristic.uuid.value.uuid128, 16u);
        }
        return btstack_gatt_client_discover_descriptors(&characteristic);
    }
    case BT_XSTACK_REQUEST_GATT_CLIENT_READ:
        return btstack_gatt_client_read(request->gatt.handle);
    case BT_XSTACK_REQUEST_GATT_CLIENT_WRITE:
        return btstack_gatt_client_write(request->gatt.handle, request->gatt.value,
            request->gatt.value_len, (request->gatt.flags & 1u) != 0u);
    case BT_XSTACK_REQUEST_GATT_CLIENT_SUBSCRIBE: {
        gatt_client_characteristic_t characteristic;
        memset(&characteristic, 0, sizeof(characteristic));
        characteristic.start_handle = request->gatt.characteristic.start_handle;
        characteristic.value_handle = request->gatt.characteristic.value_handle;
        characteristic.end_handle = request->gatt.characteristic.end_handle;
        characteristic.properties = request->gatt.characteristic.properties;
        return btstack_gatt_client_subscribe(&characteristic,
            (request->gatt.flags & 1u) != 0u, (request->gatt.flags & 2u) != 0u);
    }
    case BT_XSTACK_REQUEST_GATT_CLIENT_UNSUBSCRIBE:
        return btstack_gatt_client_unsubscribe();
    default:
        return -1;
    }
}

static void btstack_core_execute(void *context)
{
    btstack_core_command_slot_t *slot =
        (btstack_core_command_slot_t *)context;
    int status;

    if (slot == NULL) {
        return;
    }
    btstack_platform_enter_critical();
    slot->state = BT_XSTACK_COMMAND_SLOT_RUNNING;
    btstack_platform_exit_critical();
    status = btstack_core_execute_stack_request(&slot->request);
    btstack_platform_enter_critical();
    btstack_core_stack_command_count++;
    if (status != BTSTACK_STATUS_OK) {
        btstack_core_stack_command_failure_count++;
    }
    btstack_platform_exit_critical();
    btstack_core_request_completed(&slot->request, status);
    btstack_core_command_slot_release(slot);
}

void btstack_core_dispatcher_get_diagnostics(
    btstack_core_backend_diagnostics_t *diagnostics)
{
    if (diagnostics == NULL) {
        return;
    }
    btstack_platform_enter_critical();
    diagnostics->stack_commands = btstack_core_stack_command_count;
    diagnostics->stack_command_failures =
        btstack_core_stack_command_failure_count;
    diagnostics->command_slots_in_use =
        btstack_core_command_slots_in_use;
    diagnostics->command_slots_max = btstack_core_command_slots_max;
    btstack_platform_exit_critical();
}

int btstack_core_dispatch(
    const btstack_core_request_t *request)
{
    btstack_core_request_t prepared;
    btstack_core_command_slot_t *slot;
    int handled;

    if (request == NULL) {
        return -1;
    }
    handled = 0;
    {
        int local_status = btstack_core_execute_local_request(
            request, &handled);
        if (handled) {
            btstack_core_request_completed(request, local_status);
            return 0;
        }
    }
    if (btstack_core_prepare_stack_request(
            &prepared, request) != 0) {
        btstack_core_request_completed(request, -1);
        return 0;
    }
    slot = btstack_core_command_slot_acquire();
    if (slot == NULL) {
        return BT_XSTACK_BACKEND_BUSY;
    }
    memcpy(&slot->request, &prepared, sizeof(slot->request));
    slot->callback.callback = btstack_core_execute;
    slot->callback.context = slot;
    if (!btstack_platform_post_stack_callback(&slot->callback)) {
        btstack_core_command_slot_release(slot);
        return BT_XSTACK_BACKEND_BUSY;
    }
    return 0;
}

int btstack_core_dispatcher_init(void)
{
    memset(btstack_core_command_slots, 0,
        sizeof(btstack_core_command_slots));
    btstack_core_stack_command_count = 0u;
    btstack_core_stack_command_failure_count = 0u;
    btstack_core_command_slots_in_use = 0u;
    btstack_core_command_slots_max = 0u;
    return BTSTACK_STATUS_OK;
}

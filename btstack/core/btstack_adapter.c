#include "adapter/btstack_adapter.h"

#include <string.h>

#include "adapter/btstack_property.h"
#include "adapter/btstack_types.h"
#include "events/btstack_event.h"
#include "gap/btstack_gap.h"
#include "core/btstack_profile_manager.h"

static btstack_adapter_event_callback_t btstack_adapter_event_callback;

static void btstack_adapter_copy_address(
    btstack_bd_addr_t destination,
    const btstack_bd_addr_t source)
{
    memcpy(destination, source, sizeof(btstack_bd_addr_t));
}

static void btstack_adapter_map_le_event(btstack_adapter_le_event_t *destination,
    const btstack_gap_event_t *source)
{
    destination->status = source->status;
    btstack_adapter_copy_address(destination->address, source->address);
    destination->role = (btstack_adapter_le_role_t)source->le.role;
    destination->connection_handle = source->le.connection_handle;
    destination->connection_interval = source->le.connection_interval;
    destination->connection_latency = source->le.connection_latency;
    destination->supervision_timeout = source->le.supervision_timeout;
    destination->passkey = source->le.passkey;
    destination->bond_index = source->le.bond_index;
    destination->address_type = source->le.address_type;
    destination->advertising_event_type = source->le.advertising_event_type;
    destination->reason = source->le.reason;
    destination->rssi = source->le.rssi;
    destination->data_len = source->le.data_len;
    memcpy(destination->data, source->le.data, source->le.data_len);
}

static void btstack_adapter_gap_event(const btstack_gap_event_t *event)
{
    btstack_adapter_event_t mapped;

    if (event == NULL) return;
    switch (event->type) {
    case BTSTACK_GAP_EVENT_LE_CONNECTED:
        btstack_profile_manager_le_connected(event->le.connection_handle,
            (uint8_t)event->le.role, event->le.address_type, event->address);
        break;
    case BTSTACK_GAP_EVENT_LE_DISCONNECTED:
        btstack_profile_manager_le_disconnected(event->le.connection_handle);
        break;
    case BTSTACK_GAP_EVENT_LE_PAIRING_COMPLETE:
        if (event->status == BTSTACK_STATUS_OK) {
            btstack_profile_manager_le_security_changed(true, true);
        }
        break;
    case BTSTACK_GAP_EVENT_LE_REENCRYPTION_COMPLETE:
        btstack_profile_manager_le_security_changed(
            event->status == BTSTACK_STATUS_OK, event->status == BTSTACK_STATUS_OK);
        break;
    default:
        break;
    }
    if (btstack_adapter_event_callback == NULL) return;
    memset(&mapped, 0, sizeof(mapped));
    switch (event->type) {
    case BTSTACK_GAP_EVENT_STACK_STATE:
        mapped.type = BTSTACK_ADAPTER_EVENT_STACK_STATE;
        mapped.data.stack_state.working = event->stack_working != 0u;
        break;
    case BTSTACK_GAP_EVENT_LOCAL_ADDRESS:
        mapped.type = BTSTACK_ADAPTER_EVENT_LOCAL_ADDRESS;
        btstack_adapter_copy_address(
            mapped.data.address.address, event->address);
        break;
    case BTSTACK_GAP_EVENT_ACL_CONNECTED:
        mapped.type = BTSTACK_ADAPTER_EVENT_ACL_CONNECTED;
        btstack_adapter_copy_address(
            mapped.data.address.address, event->address);
        break;
    case BTSTACK_GAP_EVENT_ACL_DISCONNECTED:
        mapped.type = BTSTACK_ADAPTER_EVENT_ACL_DISCONNECTED;
        btstack_adapter_copy_address(
            mapped.data.address.address, event->address);
        break;
    case BTSTACK_GAP_EVENT_INQUIRY_RESULT:
        mapped.type = BTSTACK_ADAPTER_EVENT_SCAN_RESULT;
        btstack_adapter_copy_address(
            mapped.data.scan_result.address, event->address);
        mapped.data.scan_result.class_of_device = event->class_of_device;
        mapped.data.scan_result.rssi = event->rssi;
        mapped.data.scan_result.name_length = event->name_length;
        memcpy(mapped.data.scan_result.name, event->name,
            sizeof(mapped.data.scan_result.name));
        break;
    case BTSTACK_GAP_EVENT_INQUIRY_COMPLETE:
        mapped.type = BTSTACK_ADAPTER_EVENT_SCAN_COMPLETE;
        break;
    case BTSTACK_GAP_EVENT_PAIRING_REQUEST:
        mapped.type = BTSTACK_ADAPTER_EVENT_PAIRING_REQUEST;
        btstack_adapter_copy_address(
            mapped.data.pairing_request.address, event->address);
        mapped.data.pairing_request.numeric_value = event->numeric_value;
        break;
    case BTSTACK_GAP_EVENT_PAIRING_COMPLETE:
        mapped.type = BTSTACK_ADAPTER_EVENT_PAIRING_COMPLETE;
        btstack_adapter_copy_address(
            mapped.data.pairing_complete.address, event->address);
        mapped.data.pairing_complete.status = event->status;
        break;
    case BTSTACK_GAP_EVENT_LE_SCAN_RESULT:
        mapped.data.le.type = BTSTACK_ADAPTER_LE_EVENT_SCAN_RESULT;
        goto map_le_event;
    case BTSTACK_GAP_EVENT_LE_CONNECTED:
        mapped.data.le.type = BTSTACK_ADAPTER_LE_EVENT_CONNECTED;
        goto map_le_event;
    case BTSTACK_GAP_EVENT_LE_DISCONNECTED:
        mapped.data.le.type = BTSTACK_ADAPTER_LE_EVENT_DISCONNECTED;
        goto map_le_event;
    case BTSTACK_GAP_EVENT_LE_CONNECTION_UPDATED:
        mapped.data.le.type = BTSTACK_ADAPTER_LE_EVENT_CONNECTION_UPDATED;
        goto map_le_event;
    case BTSTACK_GAP_EVENT_LE_JUST_WORKS_REQUEST:
        mapped.data.le.type = BTSTACK_ADAPTER_LE_EVENT_JUST_WORKS_REQUEST;
        goto map_le_event;
    case BTSTACK_GAP_EVENT_LE_NUMERIC_COMPARISON:
        mapped.data.le.type = BTSTACK_ADAPTER_LE_EVENT_NUMERIC_COMPARISON;
        goto map_le_event;
    case BTSTACK_GAP_EVENT_LE_PASSKEY_DISPLAY:
        mapped.data.le.type = BTSTACK_ADAPTER_LE_EVENT_PASSKEY_DISPLAY;
        goto map_le_event;
    case BTSTACK_GAP_EVENT_LE_PASSKEY_INPUT:
        mapped.data.le.type = BTSTACK_ADAPTER_LE_EVENT_PASSKEY_INPUT;
        goto map_le_event;
    case BTSTACK_GAP_EVENT_LE_PAIRING_COMPLETE:
        mapped.data.le.type = BTSTACK_ADAPTER_LE_EVENT_PAIRING_COMPLETE;
        goto map_le_event;
    case BTSTACK_GAP_EVENT_LE_IDENTITY_RESOLVED:
        mapped.data.le.type = BTSTACK_ADAPTER_LE_EVENT_IDENTITY_RESOLVED;
        goto map_le_event;
    case BTSTACK_GAP_EVENT_LE_REENCRYPTION_COMPLETE:
        mapped.data.le.type = BTSTACK_ADAPTER_LE_EVENT_REENCRYPTION_COMPLETE;
map_le_event:
        mapped.type = BTSTACK_ADAPTER_EVENT_LE;
        btstack_adapter_map_le_event(&mapped.data.le, event);
        break;
    default:
        return;
    }
    btstack_adapter_event_callback(&mapped);
}

int btstack_adapter_setup(void)
{
    int status;

    status = btstack_property_init();
    if (status != BTSTACK_STATUS_OK) {
        return status;
    }

    status = btstack_event_init();
    if (status != BTSTACK_STATUS_OK) {
        return status;
    }

    status = btstack_gap_init();
    if (status != BTSTACK_STATUS_OK) {
        return status;
    }

    status = btstack_gap_set_event_callback(btstack_adapter_gap_event);
    if (status != BTSTACK_STATUS_OK) {
        return status;
    }

    status = btstack_gap_set_pin("0000");
    if (status != BTSTACK_STATUS_OK) {
        return status;
    }

    status = btstack_gap_set_ssp_mode(2u);
    if (status != BTSTACK_STATUS_OK) {
        return status;
    }

    status = btstack_gap_set_connectable(true);
    if (status != BTSTACK_STATUS_OK) {
        return status;
    }

    status = btstack_gap_set_discoverable(true);
    if (status != BTSTACK_STATUS_OK) {
        return status;
    }

#if BTSTACK_BONDING
    status = btstack_property_bond_init();
    if (status != BTSTACK_STATUS_OK) {
        return status;
    }
#endif

    return BTSTACK_STATUS_OK;
}

void btstack_adapter_deinit(void)
{
    btstack_gap_reset();
    btstack_event_reset();
    btstack_property_reset();
#if BTSTACK_BONDING
    btstack_property_bond_reset();
#endif
}

int btstack_adapter_set_event_callback(
    btstack_adapter_event_callback_t callback)
{
    btstack_adapter_event_callback = callback;
    return BTSTACK_STATUS_OK;
}

int btstack_adapter_set_power(bool enabled)
{
    return btstack_gap_set_power(enabled);
}

int btstack_adapter_set_name(const char *name)
{
    return btstack_gap_set_local_name(name);
}

int btstack_adapter_set_name_with_address_suffix(const char *name)
{
    static const char hex[] = "0123456789ABCDEF";
    btstack_bd_addr_t address;
    char suffixed[BTSTACK_LOCAL_NAME_MAX];
    size_t length;
    int status;

    if (name == NULL) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    length = strlen(name);
    if ((length + 5u) >= sizeof(suffixed)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    status = btstack_gap_get_local_address(address);
    if (status != BTSTACK_STATUS_OK) {
        return status;
    }
    memcpy(suffixed, name, length);
    suffixed[length++] = '-';
    suffixed[length++] = hex[address[4] >> 4];
    suffixed[length++] = hex[address[4] & 0x0fu];
    suffixed[length++] = hex[address[5] >> 4];
    suffixed[length++] = hex[address[5] & 0x0fu];
    suffixed[length] = '\0';
    return btstack_gap_set_local_name(suffixed);
}

int btstack_adapter_set_pin(const char *pin)
{
    return btstack_gap_set_pin(pin);
}

int btstack_adapter_set_ssp_mode(uint8_t mode)
{
    return btstack_gap_set_ssp_mode(mode);
}

int btstack_adapter_pairing_confirm(
    const btstack_bd_addr_t address,
    bool accept)
{
    return btstack_gap_pairing_confirm(address, accept);
}

int btstack_adapter_set_discoverable(bool enabled)
{
    return btstack_gap_set_discoverable(enabled);
}

int btstack_adapter_set_connectable(bool enabled)
{
    return btstack_gap_set_connectable(enabled);
}

int btstack_adapter_set_class_of_device(uint32_t class_of_device)
{
    return btstack_gap_set_class_of_device(class_of_device);
}

int btstack_adapter_scan_start(uint8_t duration)
{
    return btstack_gap_classic_scan_start(duration);
}

int btstack_adapter_scan_stop(void)
{
    return btstack_gap_classic_scan_stop();
}

int btstack_adapter_disconnect(void)
{
    return btstack_gap_disconnect();
}

int btstack_adapter_le_adv_configure(const btstack_adapter_le_adv_config_t *config)
{
    btstack_gap_le_adv_config_t gap_config;

    if (config == NULL) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    memset(&gap_config, 0, sizeof(gap_config));
    gap_config.interval_min = config->interval_min;
    gap_config.interval_max = config->interval_max;
    gap_config.advertising_type = config->advertising_type;
    gap_config.own_address_type = config->own_address_type;
    gap_config.channel_map = config->channel_map;
    gap_config.filter_policy = config->filter_policy;
    gap_config.data_len = config->data_len;
    gap_config.scan_response_len = config->scan_response_len;
    if ((gap_config.data_len > sizeof(gap_config.data)) ||
        (gap_config.scan_response_len > sizeof(gap_config.scan_response))) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    memcpy(gap_config.data, config->data, gap_config.data_len);
    memcpy(gap_config.scan_response, config->scan_response, gap_config.scan_response_len);
    return btstack_gap_le_adv_configure(&gap_config);
}

int btstack_adapter_le_adv_start(void) { return btstack_gap_le_adv_start(); }
int btstack_adapter_le_adv_stop(void) { return btstack_gap_le_adv_stop(); }

int btstack_adapter_le_scan_start(const btstack_adapter_le_scan_config_t *config)
{
    btstack_gap_le_scan_config_t gap_config;

    if (config == NULL) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    gap_config.interval = config->interval;
    gap_config.window = config->window;
    gap_config.active = config->active;
    gap_config.duplicate_filter = config->duplicate_filter;
    return btstack_gap_le_scan_start(&gap_config);
}

int btstack_adapter_le_scan_stop(void) { return btstack_gap_le_scan_stop(); }
int btstack_adapter_le_connect(uint8_t address_type, const btstack_bd_addr_t address)
{ return btstack_gap_le_connect(address_type, address); }
int btstack_adapter_le_connect_cancel(void) { return btstack_gap_le_connect_cancel(); }
int btstack_adapter_le_disconnect(void) { return btstack_gap_le_disconnect(); }
int btstack_adapter_le_pair(void) { return btstack_gap_le_pair(); }
int btstack_adapter_le_pairing_confirm(uint16_t connection_handle, bool accept)
{ return btstack_gap_le_pairing_confirm(connection_handle, accept); }
int btstack_adapter_le_passkey_reply(uint16_t connection_handle, uint32_t passkey)
{ return btstack_gap_le_passkey_reply(connection_handle, passkey); }
int btstack_adapter_le_bond_delete(uint8_t index) { return btstack_gap_le_bond_delete(index); }
int btstack_adapter_le_bond_delete_all(void) { return btstack_gap_le_bond_delete_all(); }

bool btstack_adapter_is_working(void)
{
    return btstack_gap_is_working();
}

int btstack_adapter_get_local_address(btstack_bd_addr_t address)
{
    return btstack_gap_get_local_address(address);
}

int btstack_adapter_get_state(btstack_adapter_state_t *state)
{
    btstack_gap_state_t gap_state;
    int status;

    if (state == NULL) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    status = btstack_gap_get_state(&gap_state);
    if (status != BTSTACK_STATUS_OK) {
        return status;
    }
    memset(state, 0, sizeof(*state));
    btstack_adapter_copy_address(
        state->local_address, gap_state.local_address);
    btstack_adapter_copy_address(
        state->peer_address, gap_state.peer_address);
    memcpy(state->local_name, gap_state.local_name,
        sizeof(state->local_name));
    memcpy(state->pin, gap_state.pin, sizeof(state->pin));
    state->class_of_device = gap_state.class_of_device;
    state->ssp_mode = gap_state.ssp_mode;
    state->stack_working = gap_state.stack_working;
    state->connectable = gap_state.connectable;
    state->discoverable = gap_state.discoverable;
    state->scanning = gap_state.scanning;
    state->acl_connected = gap_state.acl_connected;
    return BTSTACK_STATUS_OK;
}

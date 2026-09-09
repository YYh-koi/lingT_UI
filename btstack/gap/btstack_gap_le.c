#include "gap/btstack_gap.h"

#include <string.h>

#include "bluetooth.h"
#include "btstack_defines.h"
#include "btstack_event.h"
#include "ble/le_device_db.h"
#include "ble/sm.h"
#include "gap.h"
#include "hci.h"
#include "btstack_gap_internal.h"

#define BTSTACK_GAP_LE_INTERVAL_MIN 0x0004u
#define BTSTACK_GAP_LE_INTERVAL_MAX 0x4000u
#define BTSTACK_GAP_LE_SCAN_PHY_1M  0x01u

static btstack_gap_le_adv_config_t btstack_gap_le_adv_config;
static bool btstack_gap_le_adv_configured;
static bool btstack_gap_le_security_initialized;
static bool btstack_gap_le_pending_numeric_comparison;
static hci_con_handle_t btstack_gap_le_pending_security_handle = HCI_CON_HANDLE_INVALID;
static btstack_packet_callback_registration_t btstack_gap_le_sm_registration;

static bool btstack_gap_le_busy(const btstack_gap_context_t *context)
{
    return context->state.le_advertising || context->state.le_scanning ||
        context->state.le_connecting || context->state.le_connected;
}

static void btstack_gap_le_clear_link(btstack_gap_context_t *context)
{
    context->state.le_connecting = false;
    context->state.le_connected = false;
    context->state.le_role = BTSTACK_GAP_LE_ROLE_NONE;
    context->state.le_connection_handle = HCI_CON_HANDLE_INVALID;
    context->state.le_peer_address_type = 0u;
    (void)memset(context->state.le_peer_address, 0, sizeof(context->state.le_peer_address));
}

int btstack_gap_le_init(void)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();

    btstack_gap_le_adv_configured = false;
    (void)memset(&btstack_gap_le_adv_config, 0, sizeof(btstack_gap_le_adv_config));
    context->state.le_connection_handle = HCI_CON_HANDLE_INVALID;
    return BTSTACK_STATUS_OK;
}

void btstack_gap_le_reset(void)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();

    context->state.le_advertising = false;
    context->state.le_scanning = false;
    btstack_gap_le_clear_link(context);
    btstack_gap_le_adv_configured = false;
    (void)memset(&btstack_gap_le_adv_config, 0, sizeof(btstack_gap_le_adv_config));
}

void btstack_gap_le_stack_stopped(void)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();

    context->state.le_advertising = false;
    context->state.le_scanning = false;
    btstack_gap_le_clear_link(context);
}

int btstack_gap_le_adv_configure(const btstack_gap_le_adv_config_t *config)
{
    bd_addr_t direct_address = { 0u, 0u, 0u, 0u, 0u, 0u };

    if ((config == NULL) || (config->data_len > BTSTACK_ADV_DATA_MAX) ||
        (config->scan_response_len > BTSTACK_ADV_DATA_MAX) ||
        (config->interval_min < BTSTACK_GAP_LE_INTERVAL_MIN) ||
        (config->interval_max > BTSTACK_GAP_LE_INTERVAL_MAX) ||
        (config->interval_min > config->interval_max) ||
        (config->channel_map == 0u) || ((config->channel_map & 0xf8u) != 0u) ||
        (config->own_address_type > 1u)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (btstack_gap_le_busy(btstack_gap_internal_context())) {
        return BTSTACK_STATUS_BUSY;
    }
    btstack_gap_le_adv_config = *config;
    gap_random_address_set_mode(config->own_address_type == 0u ? GAP_RANDOM_ADDRESS_TYPE_OFF : GAP_RANDOM_ADDRESS_TYPE_STATIC);
    gap_advertisements_set_params(config->interval_min, config->interval_max, config->advertising_type,
        0u, direct_address, config->channel_map, config->filter_policy);
    gap_advertisements_set_data(btstack_gap_le_adv_config.data_len, btstack_gap_le_adv_config.data);
    gap_scan_response_set_data(btstack_gap_le_adv_config.scan_response_len, btstack_gap_le_adv_config.scan_response);
    btstack_gap_le_adv_configured = true;
    return BTSTACK_STATUS_OK;
}

int btstack_gap_le_adv_start(void)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();

    if (!context->initialized || !context->state.stack_working) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (!btstack_gap_le_adv_configured) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (context->state.le_advertising) {
        return BTSTACK_STATUS_OK;
    }
    if (context->state.le_scanning || context->state.le_connecting || context->state.le_connected) {
        return BTSTACK_STATUS_BUSY;
    }
    gap_advertisements_enable(1);
    context->state.le_advertising = true;
    return BTSTACK_STATUS_OK;
}

int btstack_gap_le_adv_stop(void)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();

    if (!context->initialized || !context->state.stack_working) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (!context->state.le_advertising) {
        return BTSTACK_STATUS_OK;
    }
    gap_advertisements_enable(0);
    context->state.le_advertising = false;
    return BTSTACK_STATUS_OK;
}

int btstack_gap_le_scan_start(const btstack_gap_le_scan_config_t *config)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();

    if ((config == NULL) || (config->interval < BTSTACK_GAP_LE_INTERVAL_MIN) ||
        (config->interval > BTSTACK_GAP_LE_INTERVAL_MAX) ||
        (config->window < BTSTACK_GAP_LE_INTERVAL_MIN) ||
        (config->window > config->interval) || (config->active > 1u) ||
        (config->duplicate_filter > 1u)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (!context->initialized || !context->state.stack_working) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (context->state.le_scanning) {
        return BTSTACK_STATUS_OK;
    }
    if (context->state.le_advertising || context->state.le_connecting || context->state.le_connected) {
        return BTSTACK_STATUS_BUSY;
    }
    gap_set_scan_params(config->active, config->interval, config->window, 0u);
    gap_set_scan_duplicate_filter(config->duplicate_filter != 0u);
    gap_set_scan_phys(BTSTACK_GAP_LE_SCAN_PHY_1M);
    gap_start_scan();
    context->state.le_scanning = true;
    return BTSTACK_STATUS_OK;
}

int btstack_gap_le_scan_stop(void)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();

    if (!context->initialized || !context->state.stack_working) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (!context->state.le_scanning) {
        return BTSTACK_STATUS_OK;
    }
    gap_stop_scan();
    context->state.le_scanning = false;
    return BTSTACK_STATUS_OK;
}

int btstack_gap_le_connect(uint8_t address_type, const btstack_bd_addr_t address)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();
    uint8_t status;

    if ((address == NULL) || (address_type > BD_ADDR_TYPE_LE_RANDOM)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (!context->initialized || !context->state.stack_working) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (btstack_gap_le_busy(context)) {
        return BTSTACK_STATUS_BUSY;
    }
    status = gap_connect(address, (bd_addr_type_t)address_type);
    if (status != ERROR_CODE_SUCCESS) {
        return btstack_gap_internal_status(status);
    }
    context->state.le_connecting = true;
    context->state.le_role = BTSTACK_GAP_LE_ROLE_CENTRAL;
    context->state.le_peer_address_type = address_type;
    btstack_gap_internal_copy_address(context->state.le_peer_address, address);
    return BTSTACK_STATUS_OK;
}

int btstack_gap_le_connect_cancel(void)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();
    int status;

    if (!context->state.le_connecting) {
        return BTSTACK_STATUS_NOT_READY;
    }
    status = btstack_gap_internal_status(gap_connect_cancel());
    if (status == BTSTACK_STATUS_OK) {
        btstack_gap_le_clear_link(context);
    }
    return status;
}

int btstack_gap_le_disconnect(void)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();

    if (!context->state.le_connected) {
        return BTSTACK_STATUS_NOT_READY;
    }
    return btstack_gap_internal_status(gap_disconnect(context->state.le_connection_handle));
}

static void btstack_gap_le_handle_scan_report(const uint8_t *packet, uint16_t size)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();
    btstack_gap_event_t event;
    uint8_t length;

    if (!context->state.le_scanning || (size < 12u)) {
        return;
    }
    length = gap_event_advertising_report_get_data_length(packet);
    if ((uint16_t)(12u + length) > size) {
        return;
    }
    if (length > BTSTACK_ADV_DATA_MAX) {
        length = BTSTACK_ADV_DATA_MAX;
    }
    (void)memset(&event, 0, sizeof(event));
    event.type = BTSTACK_GAP_EVENT_LE_SCAN_RESULT;
    gap_event_advertising_report_get_address(packet, event.address);
    event.le.address_type = gap_event_advertising_report_get_address_type(packet);
    event.le.advertising_event_type = gap_event_advertising_report_get_advertising_event_type(packet);
    event.le.rssi = (int8_t)gap_event_advertising_report_get_rssi(packet);
    event.le.data_len = length;
    (void)memcpy(event.le.data, gap_event_advertising_report_get_data(packet), length);
    btstack_gap_internal_emit(&event);
}

static void btstack_gap_le_handle_connected(const uint8_t *packet)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();
    btstack_gap_event_t event;
    btstack_bd_addr_t address;
    uint8_t status = hci_subevent_le_connection_complete_get_status(packet);
    hci_con_handle_t handle = hci_subevent_le_connection_complete_get_connection_handle(packet);

    if (status != ERROR_CODE_SUCCESS) {
        btstack_gap_le_clear_link(context);
        return;
    }
    if (context->state.le_connected) {
        (void)gap_disconnect(handle);
        return;
    }
    hci_subevent_le_connection_complete_get_peer_address(packet, address);
    context->state.le_advertising = false;
    context->state.le_scanning = false;
    context->state.le_connecting = false;
    context->state.le_connected = true;
    context->state.le_connection_handle = handle;
    context->state.le_role = hci_subevent_le_connection_complete_get_role(packet) == HCI_ROLE_MASTER ?
        BTSTACK_GAP_LE_ROLE_CENTRAL : BTSTACK_GAP_LE_ROLE_PERIPHERAL;
    context->state.le_peer_address_type = hci_subevent_le_connection_complete_get_peer_address_type(packet);
    btstack_gap_internal_copy_address(context->state.le_peer_address, address);
    (void)memset(&event, 0, sizeof(event));
    event.type = BTSTACK_GAP_EVENT_LE_CONNECTED;
    event.status = status;
    btstack_gap_internal_copy_address(event.address, address);
    event.le.role = context->state.le_role;
    event.le.address_type = context->state.le_peer_address_type;
    event.le.connection_handle = handle;
    event.le.connection_interval = hci_subevent_le_connection_complete_get_conn_interval(packet);
    event.le.connection_latency = hci_subevent_le_connection_complete_get_conn_latency(packet);
    event.le.supervision_timeout = hci_subevent_le_connection_complete_get_supervision_timeout(packet);
    btstack_gap_internal_emit(&event);
}

static void btstack_gap_le_handle_update(const uint8_t *packet)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();
    btstack_gap_event_t event;

    if (!context->state.le_connected ||
        (hci_subevent_le_connection_update_complete_get_connection_handle(packet) != context->state.le_connection_handle)) {
        return;
    }
    (void)memset(&event, 0, sizeof(event));
    event.type = BTSTACK_GAP_EVENT_LE_CONNECTION_UPDATED;
    event.status = hci_subevent_le_connection_update_complete_get_status(packet);
    btstack_gap_internal_copy_address(event.address, context->state.le_peer_address);
    event.le.role = context->state.le_role;
    event.le.connection_handle = context->state.le_connection_handle;
    event.le.connection_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
    event.le.connection_latency = hci_subevent_le_connection_update_complete_get_conn_latency(packet);
    event.le.supervision_timeout = hci_subevent_le_connection_update_complete_get_supervision_timeout(packet);
    btstack_gap_internal_emit(&event);
}

static void btstack_gap_le_handle_disconnected(const uint8_t *packet)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();
    btstack_gap_event_t event;

    if (!context->state.le_connected ||
        (hci_event_disconnection_complete_get_connection_handle(packet) != context->state.le_connection_handle)) {
        return;
    }
    (void)memset(&event, 0, sizeof(event));
    event.type = BTSTACK_GAP_EVENT_LE_DISCONNECTED;
    event.status = hci_event_disconnection_complete_get_reason(packet);
    btstack_gap_internal_copy_address(event.address, context->state.le_peer_address);
    event.le.role = context->state.le_role;
    event.le.connection_handle = context->state.le_connection_handle;
    btstack_gap_le_clear_link(context);
    btstack_gap_internal_emit(&event);
}

int btstack_gap_le_handle_hci_event(const uint8_t *packet, uint16_t size)
{
    uint8_t subevent;

    if ((packet == NULL) || (size < 2u)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (hci_event_packet_get_type(packet) == GAP_EVENT_ADVERTISING_REPORT) {
        btstack_gap_le_handle_scan_report(packet, size);
        return BTSTACK_STATUS_OK;
    }
    if (hci_event_packet_get_type(packet) == HCI_EVENT_DISCONNECTION_COMPLETE) {
        btstack_gap_le_handle_disconnected(packet);
        return BTSTACK_STATUS_OK;
    }
    if ((hci_event_packet_get_type(packet) != HCI_EVENT_LE_META) || (size < 3u)) {
        return BTSTACK_STATUS_OK;
    }
    subevent = hci_event_le_meta_get_subevent_code(packet);
    if ((subevent == HCI_SUBEVENT_LE_CONNECTION_COMPLETE) && (size >= 21u)) {
        btstack_gap_le_handle_connected(packet);
    } else if ((subevent == HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE) && (size >= 12u)) {
        btstack_gap_le_handle_update(packet);
    }
    return BTSTACK_STATUS_OK;
}

static void btstack_gap_le_security_event_common(btstack_gap_event_t *event, btstack_gap_event_type_t type,
    hci_con_handle_t handle, uint8_t address_type, const btstack_bd_addr_t address)
{
    (void)memset(event, 0, sizeof(*event));
    event->type = type;
    event->le.connection_handle = handle;
    event->le.address_type = address_type;
    btstack_gap_internal_copy_address(event->address, address);
}

static void btstack_gap_le_sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    btstack_gap_event_t event;
    btstack_bd_addr_t address;
    hci_con_handle_t handle;
    uint8_t type;

    (void)channel;
    if ((packet_type != HCI_EVENT_PACKET) || (packet == NULL) || (size < 2u)) {
        return;
    }
    type = hci_event_packet_get_type(packet);
    switch (type) {
    case SM_EVENT_JUST_WORKS_REQUEST:
        handle = sm_event_just_works_request_get_handle(packet);
        sm_event_just_works_request_get_address(packet, address);
        btstack_gap_le_pending_security_handle = handle;
        btstack_gap_le_pending_numeric_comparison = false;
        btstack_gap_le_security_event_common(&event, BTSTACK_GAP_EVENT_LE_JUST_WORKS_REQUEST, handle,
            sm_event_just_works_request_get_addr_type(packet), address);
        btstack_gap_internal_emit(&event);
        break;
    case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
        handle = sm_event_numeric_comparison_request_get_handle(packet);
        sm_event_numeric_comparison_request_get_address(packet, address);
        btstack_gap_le_pending_security_handle = handle;
        btstack_gap_le_pending_numeric_comparison = true;
        btstack_gap_le_security_event_common(&event, BTSTACK_GAP_EVENT_LE_NUMERIC_COMPARISON, handle,
            sm_event_numeric_comparison_request_get_addr_type(packet), address);
        event.le.passkey = sm_event_numeric_comparison_request_get_passkey(packet);
        btstack_gap_internal_emit(&event);
        break;
    case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
        handle = sm_event_passkey_display_number_get_handle(packet);
        sm_event_passkey_display_number_get_address(packet, address);
        btstack_gap_le_security_event_common(&event, BTSTACK_GAP_EVENT_LE_PASSKEY_DISPLAY, handle,
            sm_event_passkey_display_number_get_addr_type(packet), address);
        event.le.passkey = sm_event_passkey_display_number_get_passkey(packet);
        btstack_gap_internal_emit(&event);
        break;
    case SM_EVENT_PASSKEY_INPUT_NUMBER:
        handle = sm_event_passkey_input_number_get_handle(packet);
        sm_event_passkey_input_number_get_address(packet, address);
        btstack_gap_le_pending_security_handle = handle;
        btstack_gap_le_pending_numeric_comparison = false;
        btstack_gap_le_security_event_common(&event, BTSTACK_GAP_EVENT_LE_PASSKEY_INPUT, handle,
            sm_event_passkey_input_number_get_addr_type(packet), address);
        btstack_gap_internal_emit(&event);
        break;
    case SM_EVENT_PAIRING_COMPLETE:
        handle = sm_event_pairing_complete_get_handle(packet);
        sm_event_pairing_complete_get_address(packet, address);
        btstack_gap_le_security_event_common(&event, BTSTACK_GAP_EVENT_LE_PAIRING_COMPLETE, handle,
            sm_event_pairing_complete_get_addr_type(packet), address);
        event.status = sm_event_pairing_complete_get_status(packet);
        event.le.reason = sm_event_pairing_complete_get_reason(packet);
        btstack_gap_le_pending_security_handle = HCI_CON_HANDLE_INVALID;
        btstack_gap_internal_emit(&event);
        break;
    case SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED:
        handle = sm_event_identity_resolving_succeeded_get_handle(packet);
        sm_event_identity_resolving_succeeded_get_identity_address(packet, address);
        btstack_gap_le_security_event_common(&event, BTSTACK_GAP_EVENT_LE_IDENTITY_RESOLVED, handle,
            sm_event_identity_resolving_succeeded_get_identity_addr_type(packet), address);
        event.le.bond_index = sm_event_identity_resolving_succeeded_get_index(packet);
        btstack_gap_internal_emit(&event);
        break;
    case SM_EVENT_REENCRYPTION_COMPLETE:
        handle = sm_event_reencryption_complete_get_handle(packet);
        sm_event_reencryption_complete_get_address(packet, address);
        btstack_gap_le_security_event_common(&event, BTSTACK_GAP_EVENT_LE_REENCRYPTION_COMPLETE, handle,
            sm_event_reencryption_complete_get_addr_type(packet), address);
        event.status = sm_event_reencryption_complete_get_status(packet);
        btstack_gap_internal_emit(&event);
        break;
    default:
        break;
    }
}

int btstack_gap_le_security_init(void)
{
    if (btstack_gap_le_security_initialized) {
        return BTSTACK_STATUS_OK;
    }
    le_device_db_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_BONDING);
    btstack_gap_le_sm_registration.callback = btstack_gap_le_sm_packet_handler;
    sm_add_event_handler(&btstack_gap_le_sm_registration);
    gap_random_address_set_mode(GAP_RANDOM_ADDRESS_RESOLVABLE);
    gap_random_address_set_update_period(15 * 60 * 1000);
    btstack_gap_le_security_initialized = true;
    return BTSTACK_STATUS_OK;
}

void btstack_gap_le_security_reset(void)
{
    if (btstack_gap_le_security_initialized) {
        sm_remove_event_handler(&btstack_gap_le_sm_registration);
    }
    btstack_gap_le_security_initialized = false;
    btstack_gap_le_pending_numeric_comparison = false;
    btstack_gap_le_pending_security_handle = HCI_CON_HANDLE_INVALID;
    le_device_db_init();
}

int btstack_gap_le_pair(void)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();

    if (!context->state.le_connected) {
        return BTSTACK_STATUS_NOT_READY;
    }
    sm_request_pairing(context->state.le_connection_handle);
    return BTSTACK_STATUS_OK;
}

int btstack_gap_le_pairing_confirm(uint16_t connection_handle, bool accept)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();

    if (!context->state.le_connected) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (connection_handle != context->state.le_connection_handle) {
        return BTSTACK_STATUS_NOT_FOUND;
    }
    if (!accept) {
        sm_bonding_decline(connection_handle);
    } else if ((btstack_gap_le_pending_security_handle == connection_handle) &&
        btstack_gap_le_pending_numeric_comparison) {
        sm_numeric_comparison_confirm(connection_handle);
    } else {
        sm_just_works_confirm(connection_handle);
    }
    btstack_gap_le_pending_security_handle = HCI_CON_HANDLE_INVALID;
    btstack_gap_le_pending_numeric_comparison = false;
    return BTSTACK_STATUS_OK;
}

int btstack_gap_le_passkey_reply(uint16_t connection_handle, uint32_t passkey)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();

    if (passkey > 999999u) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (!context->state.le_connected) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (connection_handle != context->state.le_connection_handle) {
        return BTSTACK_STATUS_NOT_FOUND;
    }
    sm_passkey_input(connection_handle, passkey);
    btstack_gap_le_pending_security_handle = HCI_CON_HANDLE_INVALID;
    return BTSTACK_STATUS_OK;
}

uint8_t btstack_gap_le_bond_count(void)
{
    int count = le_device_db_count();

    if (count < 0) {
        return 0u;
    }
    if (count > (int)BTSTACK_LE_BOND_MAX) {
        count = (int)BTSTACK_LE_BOND_MAX;
    }
    return (uint8_t)count;
}

int btstack_gap_le_bond_delete(uint8_t index)
{
    if ((index >= BTSTACK_LE_BOND_MAX) || (index >= (uint8_t)le_device_db_max_count())) {
        return BTSTACK_STATUS_NOT_FOUND;
    }
    le_device_db_remove(index);
    return BTSTACK_STATUS_OK;
}

int btstack_gap_le_bond_delete_all(void)
{
    int index;
    int max_count = le_device_db_max_count();

    for (index = 0; index < max_count; index++) {
        le_device_db_remove(index);
    }
    return BTSTACK_STATUS_OK;
}

#include "gap/btstack_gap.h"

#include <string.h>

#include "bluetooth.h"
#include "btstack_event.h"
#include "gap.h"
#include "hci.h"
#include "btstack_gap_internal.h"

#define BTSTACK_GAP_INQUIRY_DURATION_MIN 0x01u
#define BTSTACK_GAP_INQUIRY_DURATION_MAX 0x30u

static int btstack_gap_classic_security_apply_cached(void)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();
    uint8_t mode = context->state.ssp_mode;

    if (mode == 0u) {
        gap_ssp_set_enable(0);
        gap_ssp_set_auto_accept(0);
        return BTSTACK_STATUS_OK;
    }
    gap_ssp_set_enable(1);
    gap_ssp_set_authentication_requirement(SSP_IO_AUTHREQ_MITM_PROTECTION_NOT_REQUIRED_GENERAL_BONDING);
    gap_ssp_set_io_capability(mode == 1u ? SSP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT : SSP_IO_CAPABILITY_DISPLAY_YES_NO);
    gap_ssp_set_auto_accept((mode == 1u) || (mode == 2u));
    return BTSTACK_STATUS_OK;
}

int btstack_gap_classic_init(void)
{
    return BTSTACK_STATUS_OK;
}

void btstack_gap_classic_reset(void)
{
}

int btstack_gap_classic_apply_cached(void)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();

    gap_set_local_name(context->state.local_name);
    gap_set_class_of_device(context->state.class_of_device);
    gap_discoverable_control(context->state.discoverable ? 1u : 0u);
    gap_connectable_control(context->state.connectable ? 1u : 0u);
    return btstack_gap_classic_security_apply_cached();
}

void btstack_gap_classic_stack_stopped(void)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();

    context->state.scanning = false;
    context->state.acl_connected = false;
    context->acl_handle = HCI_CON_HANDLE_INVALID;
    context->pending_pair_valid = false;
}

int btstack_gap_set_local_name(const char *name)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();
    size_t length;

    if (name == NULL) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (!context->initialized) {
        return BTSTACK_STATUS_NOT_READY;
    }
    length = strlen(name);
    if (length >= BTSTACK_LOCAL_NAME_MAX) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    (void)memcpy(context->state.local_name, name, length + 1u);
    gap_set_local_name(context->state.local_name);
    return BTSTACK_STATUS_OK;
}

int btstack_gap_set_class_of_device(uint32_t class_of_device)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();

    if (!context->initialized) {
        return BTSTACK_STATUS_NOT_READY;
    }
    context->state.class_of_device = class_of_device;
    gap_set_class_of_device(class_of_device);
    return BTSTACK_STATUS_OK;
}

int btstack_gap_set_discoverable(bool enabled)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();

    if (!context->initialized) {
        return BTSTACK_STATUS_NOT_READY;
    }
    context->state.discoverable = enabled;
    gap_discoverable_control(enabled ? 1u : 0u);
    return BTSTACK_STATUS_OK;
}

int btstack_gap_set_connectable(bool enabled)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();

    if (!context->initialized) {
        return BTSTACK_STATUS_NOT_READY;
    }
    context->state.connectable = enabled;
    gap_connectable_control(enabled ? 1u : 0u);
    return BTSTACK_STATUS_OK;
}

int btstack_gap_disconnect(void)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();

    if (!context->initialized) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (!context->state.acl_connected) {
        return BTSTACK_STATUS_OK;
    }
    return btstack_gap_internal_status(gap_disconnect(context->acl_handle));
}

int btstack_gap_classic_scan_start(uint8_t duration)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();
    int status;

    if ((duration < BTSTACK_GAP_INQUIRY_DURATION_MIN) || (duration > BTSTACK_GAP_INQUIRY_DURATION_MAX)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (!context->initialized || !context->state.stack_working) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (context->state.scanning) {
        return BTSTACK_STATUS_BUSY;
    }
    status = btstack_gap_internal_status(gap_inquiry_start(duration));
    if (status == BTSTACK_STATUS_OK) {
        context->state.scanning = true;
    }
    return status;
}

int btstack_gap_classic_scan_stop(void)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();
    int status;

    if (!context->initialized || !context->state.stack_working) {
        return BTSTACK_STATUS_NOT_READY;
    }
    status = btstack_gap_internal_status(gap_inquiry_stop());
    if (status == BTSTACK_STATUS_OK) {
        context->state.scanning = false;
    }
    return status;
}

bool btstack_gap_classic_is_scanning(void)
{
    return btstack_gap_internal_context()->state.scanning;
}

int btstack_gap_set_pin(const char *pin)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();
    size_t length;

    if (pin == NULL) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (!context->initialized) {
        return BTSTACK_STATUS_NOT_READY;
    }
    length = strlen(pin);
    if ((length == 0u) || (length > BTSTACK_GAP_PIN_MAX)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    (void)memcpy(context->state.pin, pin, length + 1u);
    return BTSTACK_STATUS_OK;
}

int btstack_gap_set_ssp_mode(uint8_t mode)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();

    if (mode > 3u) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (!context->initialized) {
        return BTSTACK_STATUS_NOT_READY;
    }
    context->state.ssp_mode = mode;
    return btstack_gap_classic_security_apply_cached();
}

int btstack_gap_pairing_confirm(const btstack_bd_addr_t address, bool accept)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();
    int native_status;

    if (address == NULL) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (!context->initialized) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (!context->pending_pair_valid || !btstack_gap_internal_address_equal(address, context->pending_pair_address)) {
        return BTSTACK_STATUS_NOT_FOUND;
    }
    native_status = accept ? gap_ssp_confirmation_response(address) : gap_ssp_confirmation_negative(address);
    if (native_status == ERROR_CODE_SUCCESS) {
        context->pending_pair_valid = false;
    }
    return btstack_gap_internal_status(native_status);
}

static void btstack_gap_classic_handle_connected(const uint8_t *packet)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();
    btstack_gap_event_t event;
    bd_addr_t address;
    uint8_t status = hci_event_connection_complete_get_status(packet);

    if ((status != ERROR_CODE_SUCCESS) || (hci_event_connection_complete_get_link_type(packet) != HCI_LINK_TYPE_ACL)) {
        return;
    }
    hci_event_connection_complete_get_bd_addr(packet, address);
    context->acl_handle = hci_event_connection_complete_get_connection_handle(packet);
    btstack_gap_internal_copy_address(context->state.peer_address, address);
    context->state.acl_connected = true;
    (void)memset(&event, 0, sizeof(event));
    event.type = BTSTACK_GAP_EVENT_ACL_CONNECTED;
    event.status = status;
    btstack_gap_internal_copy_address(event.address, address);
    btstack_gap_internal_emit(&event);
}

static void btstack_gap_classic_handle_disconnected(const uint8_t *packet)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();
    btstack_gap_event_t event;
    hci_con_handle_t handle = hci_event_disconnection_complete_get_connection_handle(packet);

    if (!context->state.acl_connected || (handle != context->acl_handle)) {
        return;
    }
    (void)memset(&event, 0, sizeof(event));
    event.type = BTSTACK_GAP_EVENT_ACL_DISCONNECTED;
    event.status = hci_event_disconnection_complete_get_reason(packet);
    btstack_gap_internal_copy_address(event.address, context->state.peer_address);
    context->state.acl_connected = false;
    context->acl_handle = HCI_CON_HANDLE_INVALID;
    (void)memset(context->state.peer_address, 0, BTSTACK_BD_ADDR_LEN);
    btstack_gap_internal_emit(&event);
}

static void btstack_gap_classic_handle_inquiry_result(const uint8_t *packet)
{
    btstack_gap_event_t event;
    const uint8_t *name;
    uint8_t name_length;

    (void)memset(&event, 0, sizeof(event));
    event.type = BTSTACK_GAP_EVENT_INQUIRY_RESULT;
    gap_event_inquiry_result_get_bd_addr(packet, event.address);
    event.class_of_device = gap_event_inquiry_result_get_class_of_device(packet);
    event.rssi = gap_event_inquiry_result_get_rssi_available(packet) ? (int8_t)gap_event_inquiry_result_get_rssi(packet) : -127;
    if (gap_event_inquiry_result_get_name_available(packet)) {
        name = gap_event_inquiry_result_get_name(packet);
        name_length = gap_event_inquiry_result_get_name_len(packet);
        if (name_length >= BTSTACK_LOCAL_NAME_MAX) {
            name_length = BTSTACK_LOCAL_NAME_MAX - 1u;
        }
        (void)memcpy(event.name, name, name_length);
        event.name[name_length] = '\0';
        event.name_length = name_length;
    }
    btstack_gap_internal_emit(&event);
}

static void btstack_gap_classic_handle_inquiry_complete(const uint8_t *packet)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();
    btstack_gap_event_t event;

    context->state.scanning = false;
    (void)memset(&event, 0, sizeof(event));
    event.type = BTSTACK_GAP_EVENT_INQUIRY_COMPLETE;
    event.status = gap_event_inquiry_complete_get_status(packet);
    btstack_gap_internal_emit(&event);
}

static void btstack_gap_classic_handle_confirmation_request(const btstack_bd_addr_t address, uint32_t value)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();
    btstack_gap_event_t event;

    if (context->state.ssp_mode != 3u) {
        (void)gap_ssp_confirmation_response(address);
        return;
    }
    (void)memset(&event, 0, sizeof(event));
    context->pending_pair_valid = true;
    btstack_gap_internal_copy_address(context->pending_pair_address, address);
    event.type = BTSTACK_GAP_EVENT_PAIRING_REQUEST;
    event.numeric_value = value;
    btstack_gap_internal_copy_address(event.address, address);
    btstack_gap_internal_emit(&event);
}

int btstack_gap_classic_handle_hci_event(const uint8_t *packet, uint16_t size)
{
    btstack_gap_context_t *context = btstack_gap_internal_context();
    btstack_gap_event_t event;
    bd_addr_t address;

    if ((packet == NULL) || (size < 2u)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    switch (hci_event_packet_get_type(packet)) {
    case HCI_EVENT_CONNECTION_COMPLETE:
        btstack_gap_classic_handle_connected(packet);
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        btstack_gap_classic_handle_disconnected(packet);
        break;
    case HCI_EVENT_PIN_CODE_REQUEST:
        hci_event_pin_code_request_get_bd_addr(packet, address);
        (void)gap_pin_code_response(address, context->state.pin);
        break;
    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
        hci_event_user_confirmation_request_get_bd_addr(packet, address);
        btstack_gap_classic_handle_confirmation_request(address, hci_event_user_confirmation_request_get_numeric_value(packet));
        break;
    case HCI_EVENT_SIMPLE_PAIRING_COMPLETE:
        (void)memset(&event, 0, sizeof(event));
        event.type = BTSTACK_GAP_EVENT_PAIRING_COMPLETE;
        event.status = hci_event_simple_pairing_complete_get_status(packet);
        hci_event_simple_pairing_complete_get_bd_addr(packet, event.address);
        context->pending_pair_valid = false;
        btstack_gap_internal_emit(&event);
        break;
    case GAP_EVENT_INQUIRY_RESULT:
        btstack_gap_classic_handle_inquiry_result(packet);
        break;
    case GAP_EVENT_INQUIRY_COMPLETE:
        btstack_gap_classic_handle_inquiry_complete(packet);
        break;
    default:
        break;
    }
    return BTSTACK_STATUS_OK;
}

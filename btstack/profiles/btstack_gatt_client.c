#include "profiles/btstack_gatt_client.h"

#include <string.h>

#include "bluetooth.h"
#include "btstack_defines.h"
#include "btstack_event.h"

#if BTSTACK_LE && BTSTACK_GATT_CLIENT

static btstack_gatt_client_event_callback_t btstack_gatt_client_event_callback;
static btstack_gatt_client_operation_t btstack_gatt_client_operation;
static hci_con_handle_t btstack_gatt_client_connection_handle = HCI_CON_HANDLE_INVALID;
static gatt_client_characteristic_t btstack_gatt_client_characteristic;
static gatt_client_notification_t btstack_gatt_client_notification;
static uint8_t btstack_gatt_client_write_value[BTSTACK_GATT_VALUE_MAX];
static bool btstack_gatt_client_initialized;
static bool btstack_gatt_client_listening;

static int btstack_gatt_client_status(uint8_t status)
{
    if (status == ERROR_CODE_SUCCESS) {
        return BTSTACK_STATUS_OK;
    }
    if (status == ERROR_CODE_COMMAND_DISALLOWED) {
        return BTSTACK_STATUS_BUSY;
    }
    return BTSTACK_STATUS_ERROR;
}

static bool btstack_gatt_client_uuid_valid(const btstack_uuid_t *uuid)
{
    return (uuid == NULL) || (uuid->type == BTSTACK_UUID_TYPE_16) || (uuid->type == BTSTACK_UUID_TYPE_128);
}

static void btstack_gatt_client_emit(const btstack_gatt_client_event_t *event)
{
    if ((event != NULL) && (btstack_gatt_client_event_callback != NULL)) {
        btstack_gatt_client_event_callback(event);
    }
}

static void btstack_gatt_client_complete(int status)
{
    btstack_gatt_client_event_t event;
    btstack_gatt_client_operation_t operation = btstack_gatt_client_operation;

    btstack_gatt_client_operation = BTSTACK_GATT_CLIENT_OPERATION_NONE;
    (void)memset(&event, 0, sizeof(event));
    event.type = BTSTACK_GATT_CLIENT_EVENT_COMPLETE;
    event.operation = operation;
    event.status = status;
    event.connection_handle = btstack_gatt_client_connection_handle;
    btstack_gatt_client_emit(&event);
}

static void btstack_gatt_client_copy_value(btstack_gatt_client_event_t *event, const uint8_t *value,
    uint16_t value_len)
{
    event->value_len = value_len;
    if (event->value_len > BTSTACK_GATT_VALUE_MAX) {
        event->value_len = BTSTACK_GATT_VALUE_MAX;
        event->truncated = 1u;
    }
    if ((event->value_len != 0u) && (value != NULL)) {
        (void)memcpy(event->data.value, value, event->value_len);
    }
}

static void btstack_gatt_client_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    btstack_gatt_client_event_t event;
    uint8_t type;

    (void)channel;
    if ((packet_type != HCI_EVENT_PACKET) || (packet == NULL) || (size < 2u)) {
        return;
    }
    type = hci_event_packet_get_type(packet);
    (void)memset(&event, 0, sizeof(event));
    event.operation = btstack_gatt_client_operation;
    event.connection_handle = btstack_gatt_client_connection_handle;
    switch (type) {
    case GATT_EVENT_SERVICE_QUERY_RESULT:
        event.type = BTSTACK_GATT_CLIENT_EVENT_SERVICE;
        gatt_event_service_query_result_get_service(packet, &event.data.service);
        btstack_gatt_client_emit(&event);
        break;
    case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
        event.type = BTSTACK_GATT_CLIENT_EVENT_CHARACTERISTIC;
        gatt_event_characteristic_query_result_get_characteristic(packet, &event.data.characteristic);
        btstack_gatt_client_emit(&event);
        break;
    case GATT_EVENT_ALL_CHARACTERISTIC_DESCRIPTORS_QUERY_RESULT:
        event.type = BTSTACK_GATT_CLIENT_EVENT_DESCRIPTOR;
        gatt_event_all_characteristic_descriptors_query_result_get_characteristic_descriptor(packet, &event.data.descriptor);
        btstack_gatt_client_emit(&event);
        break;
    case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT:
        event.type = BTSTACK_GATT_CLIENT_EVENT_VALUE;
        event.value_handle = gatt_event_characteristic_value_query_result_get_value_handle(packet);
        btstack_gatt_client_copy_value(&event, gatt_event_characteristic_value_query_result_get_value(packet),
            gatt_event_characteristic_value_query_result_get_value_length(packet));
        btstack_gatt_client_emit(&event);
        break;
    case GATT_EVENT_NOTIFICATION:
        event.type = BTSTACK_GATT_CLIENT_EVENT_NOTIFICATION;
        event.operation = BTSTACK_GATT_CLIENT_OPERATION_NONE;
        event.connection_handle = gatt_event_notification_get_handle(packet);
        event.value_handle = gatt_event_notification_get_value_handle(packet);
        btstack_gatt_client_copy_value(&event, gatt_event_notification_get_value(packet),
            gatt_event_notification_get_value_length(packet));
        btstack_gatt_client_emit(&event);
        break;
    case GATT_EVENT_INDICATION:
        event.type = BTSTACK_GATT_CLIENT_EVENT_INDICATION;
        event.operation = BTSTACK_GATT_CLIENT_OPERATION_NONE;
        event.connection_handle = gatt_event_indication_get_handle(packet);
        event.value_handle = gatt_event_indication_get_value_handle(packet);
        btstack_gatt_client_copy_value(&event, gatt_event_indication_get_value(packet),
            gatt_event_indication_get_value_length(packet));
        btstack_gatt_client_emit(&event);
        break;
    case GATT_EVENT_MTU:
        event.type = BTSTACK_GATT_CLIENT_EVENT_MTU;
        event.operation = BTSTACK_GATT_CLIENT_OPERATION_NONE;
        event.connection_handle = gatt_event_mtu_get_handle(packet);
        event.mtu = gatt_event_mtu_get_MTU(packet);
        btstack_gatt_client_emit(&event);
        break;
    case GATT_EVENT_QUERY_COMPLETE:
        if (btstack_gatt_client_operation == BTSTACK_GATT_CLIENT_OPERATION_NONE) {
            return;
        }
        if ((btstack_gatt_client_operation == BTSTACK_GATT_CLIENT_OPERATION_SUBSCRIBE) &&
            (gatt_event_query_complete_get_att_status(packet) == ERROR_CODE_SUCCESS)) {
            gatt_client_listen_for_characteristic_value_updates(&btstack_gatt_client_notification,
                btstack_gatt_client_packet_handler, btstack_gatt_client_connection_handle,
                &btstack_gatt_client_characteristic);
            btstack_gatt_client_listening = true;
        }
        if ((btstack_gatt_client_operation == BTSTACK_GATT_CLIENT_OPERATION_UNSUBSCRIBE) &&
            (gatt_event_query_complete_get_att_status(packet) == ERROR_CODE_SUCCESS)) {
            gatt_client_stop_listening_for_characteristic_value_updates(
                &btstack_gatt_client_notification);
            btstack_gatt_client_listening = false;
        }
        btstack_gatt_client_complete(btstack_gatt_client_status(gatt_event_query_complete_get_att_status(packet)));
        break;
    default:
        break;
    }
}

int btstack_gatt_client_init(void)
{
    if (btstack_gatt_client_initialized) {
        return BTSTACK_STATUS_OK;
    }
    (void)memset(&btstack_gatt_client_characteristic, 0, sizeof(btstack_gatt_client_characteristic));
    (void)memset(&btstack_gatt_client_notification, 0, sizeof(btstack_gatt_client_notification));
    btstack_gatt_client_operation = BTSTACK_GATT_CLIENT_OPERATION_NONE;
    btstack_gatt_client_connection_handle = HCI_CON_HANDLE_INVALID;
    gatt_client_init();
    btstack_gatt_client_initialized = true;
    return BTSTACK_STATUS_OK;
}

void btstack_gatt_client_reset(void)
{
    if (btstack_gatt_client_listening) {
        gatt_client_stop_listening_for_characteristic_value_updates(&btstack_gatt_client_notification);
    }
    btstack_gatt_client_listening = false;
    btstack_gatt_client_operation = BTSTACK_GATT_CLIENT_OPERATION_NONE;
    btstack_gatt_client_connection_handle = HCI_CON_HANDLE_INVALID;
    (void)memset(&btstack_gatt_client_characteristic, 0, sizeof(btstack_gatt_client_characteristic));
}

int btstack_gatt_client_set_event_callback(btstack_gatt_client_event_callback_t callback)
{
    btstack_gatt_client_event_callback = callback;
    return BTSTACK_STATUS_OK;
}

void btstack_gatt_client_connection_changed(uint16_t connection_handle, bool connected)
{
    if (!connected) {
        if (btstack_gatt_client_listening) {
            gatt_client_stop_listening_for_characteristic_value_updates(&btstack_gatt_client_notification);
            btstack_gatt_client_listening = false;
        }
        if (btstack_gatt_client_operation != BTSTACK_GATT_CLIENT_OPERATION_NONE) {
            btstack_gatt_client_complete(BTSTACK_STATUS_DISCONNECTED);
        }
        btstack_gatt_client_connection_handle = HCI_CON_HANDLE_INVALID;
        return;
    }
    btstack_gatt_client_connection_handle = connection_handle;
}

btstack_gatt_client_operation_t btstack_gatt_client_get_operation(void)
{
    return btstack_gatt_client_operation;
}

static int btstack_gatt_client_begin(btstack_gatt_client_operation_t operation)
{
    if (!btstack_gatt_client_initialized || (btstack_gatt_client_connection_handle == HCI_CON_HANDLE_INVALID)) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (btstack_gatt_client_operation != BTSTACK_GATT_CLIENT_OPERATION_NONE) {
        return BTSTACK_STATUS_BUSY;
    }
    btstack_gatt_client_operation = operation;
    return BTSTACK_STATUS_OK;
}

static int btstack_gatt_client_started(uint8_t status)
{
    int mapped = btstack_gatt_client_status(status);

    if (mapped != BTSTACK_STATUS_OK) {
        btstack_gatt_client_operation = BTSTACK_GATT_CLIENT_OPERATION_NONE;
    }
    return mapped;
}

int btstack_gatt_client_discover_services(const btstack_uuid_t *uuid)
{
    uint8_t status;
    int begin;

    if (!btstack_gatt_client_uuid_valid(uuid)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    begin = btstack_gatt_client_begin(BTSTACK_GATT_CLIENT_OPERATION_DISCOVER_SERVICES);
    if (begin != BTSTACK_STATUS_OK) {
        return begin;
    }
    if (uuid == NULL) {
        status = gatt_client_discover_primary_services(btstack_gatt_client_packet_handler,
            btstack_gatt_client_connection_handle);
    } else if (uuid->type == BTSTACK_UUID_TYPE_16) {
        status = gatt_client_discover_primary_services_by_uuid16(btstack_gatt_client_packet_handler,
            btstack_gatt_client_connection_handle, uuid->value.uuid16);
    } else {
        status = gatt_client_discover_primary_services_by_uuid128(btstack_gatt_client_packet_handler,
            btstack_gatt_client_connection_handle, uuid->value.uuid128);
    }
    return btstack_gatt_client_started(status);
}

int btstack_gatt_client_discover_characteristics(uint16_t start_handle, uint16_t end_handle,
    const btstack_uuid_t *uuid)
{
    gatt_client_service_t service;
    uint8_t status;
    int begin;

    if ((start_handle == 0u) || (start_handle > end_handle) || !btstack_gatt_client_uuid_valid(uuid)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    begin = btstack_gatt_client_begin(BTSTACK_GATT_CLIENT_OPERATION_DISCOVER_CHARACTERISTICS);
    if (begin != BTSTACK_STATUS_OK) {
        return begin;
    }
    if (uuid == NULL) {
        (void)memset(&service, 0, sizeof(service));
        service.start_group_handle = start_handle;
        service.end_group_handle = end_handle;
        status = gatt_client_discover_characteristics_for_service(btstack_gatt_client_packet_handler,
            btstack_gatt_client_connection_handle, &service);
    } else if (uuid->type == BTSTACK_UUID_TYPE_16) {
        status = gatt_client_discover_characteristics_for_handle_range_by_uuid16(btstack_gatt_client_packet_handler,
            btstack_gatt_client_connection_handle, start_handle, end_handle, uuid->value.uuid16);
    } else {
        status = gatt_client_discover_characteristics_for_handle_range_by_uuid128(btstack_gatt_client_packet_handler,
            btstack_gatt_client_connection_handle, start_handle, end_handle, uuid->value.uuid128);
    }
    return btstack_gatt_client_started(status);
}

int btstack_gatt_client_discover_descriptors(const gatt_client_characteristic_t *characteristic)
{
    int begin;

    if ((characteristic == NULL) || (characteristic->value_handle == 0u)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    begin = btstack_gatt_client_begin(BTSTACK_GATT_CLIENT_OPERATION_DISCOVER_DESCRIPTORS);
    if (begin != BTSTACK_STATUS_OK) {
        return begin;
    }
    btstack_gatt_client_characteristic = *characteristic;
    return btstack_gatt_client_started(gatt_client_discover_characteristic_descriptors(
        btstack_gatt_client_packet_handler, btstack_gatt_client_connection_handle,
        &btstack_gatt_client_characteristic));
}

int btstack_gatt_client_read(uint16_t value_handle)
{
    int begin;

    if (value_handle == 0u) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    begin = btstack_gatt_client_begin(BTSTACK_GATT_CLIENT_OPERATION_READ);
    if (begin != BTSTACK_STATUS_OK) {
        return begin;
    }
    return btstack_gatt_client_started(gatt_client_read_value_of_characteristic_using_value_handle(
        btstack_gatt_client_packet_handler, btstack_gatt_client_connection_handle, value_handle));
}

int btstack_gatt_client_write(uint16_t value_handle, const uint8_t *value, uint16_t value_len,
    bool without_response)
{
    uint8_t status;
    int begin;

    if ((value_handle == 0u) || ((value == NULL) && (value_len != 0u)) ||
        (value_len > BTSTACK_GATT_VALUE_MAX)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    begin = btstack_gatt_client_begin(BTSTACK_GATT_CLIENT_OPERATION_WRITE);
    if (begin != BTSTACK_STATUS_OK) {
        return begin;
    }
    if (value_len != 0u) {
        (void)memcpy(btstack_gatt_client_write_value, value, value_len);
    }
    if (without_response) {
        status = gatt_client_write_value_of_characteristic_without_response(btstack_gatt_client_connection_handle,
            value_handle, value_len, btstack_gatt_client_write_value);
        if (status != ERROR_CODE_SUCCESS) {
            return btstack_gatt_client_started(status);
        }
        btstack_gatt_client_complete(BTSTACK_STATUS_OK);
        return BTSTACK_STATUS_OK;
    }
    status = gatt_client_write_value_of_characteristic(btstack_gatt_client_packet_handler,
        btstack_gatt_client_connection_handle, value_handle, value_len, btstack_gatt_client_write_value);
    return btstack_gatt_client_started(status);
}

int btstack_gatt_client_subscribe(const gatt_client_characteristic_t *characteristic,
    bool notifications, bool indications)
{
    uint16_t configuration = 0u;
    int begin;

    if ((characteristic == NULL) || (!notifications && !indications)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    begin = btstack_gatt_client_begin(BTSTACK_GATT_CLIENT_OPERATION_SUBSCRIBE);
    if (begin != BTSTACK_STATUS_OK) {
        return begin;
    }
    if (notifications) {
        configuration |= GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
    }
    if (indications) {
        configuration |= GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_INDICATION;
    }
    btstack_gatt_client_characteristic = *characteristic;
    return btstack_gatt_client_started(gatt_client_write_client_characteristic_configuration(
        btstack_gatt_client_packet_handler, btstack_gatt_client_connection_handle,
        &btstack_gatt_client_characteristic, configuration));
}

int btstack_gatt_client_unsubscribe(void)
{
    int begin;

    if (!btstack_gatt_client_listening) {
        return BTSTACK_STATUS_OK;
    }
    begin = btstack_gatt_client_begin(BTSTACK_GATT_CLIENT_OPERATION_UNSUBSCRIBE);
    if (begin != BTSTACK_STATUS_OK) return begin;
    return btstack_gatt_client_started(
        gatt_client_write_client_characteristic_configuration(
            btstack_gatt_client_packet_handler, btstack_gatt_client_connection_handle,
            &btstack_gatt_client_characteristic, 0u));
}

#else

int btstack_gatt_client_init(void) { return BTSTACK_STATUS_NOT_SUPPORTED; }
void btstack_gatt_client_reset(void) { }
int btstack_gatt_client_set_event_callback(btstack_gatt_client_event_callback_t callback)
{ (void)callback; return BTSTACK_STATUS_NOT_SUPPORTED; }
void btstack_gatt_client_connection_changed(uint16_t connection_handle, bool connected)
{ (void)connection_handle; (void)connected; }
btstack_gatt_client_operation_t btstack_gatt_client_get_operation(void)
{ return BTSTACK_GATT_CLIENT_OPERATION_NONE; }
int btstack_gatt_client_discover_services(const btstack_uuid_t *uuid)
{ (void)uuid; return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_gatt_client_discover_characteristics(uint16_t start_handle, uint16_t end_handle,
    const btstack_uuid_t *uuid)
{ (void)start_handle; (void)end_handle; (void)uuid; return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_gatt_client_discover_descriptors(const gatt_client_characteristic_t *characteristic)
{ (void)characteristic; return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_gatt_client_read(uint16_t value_handle)
{ (void)value_handle; return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_gatt_client_write(uint16_t value_handle, const uint8_t *value, uint16_t value_len,
    bool without_response)
{ (void)value_handle; (void)value; (void)value_len; (void)without_response; return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_gatt_client_subscribe(const gatt_client_characteristic_t *characteristic,
    bool notifications, bool indications)
{ (void)characteristic; (void)notifications; (void)indications; return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_gatt_client_unsubscribe(void) { return BTSTACK_STATUS_NOT_SUPPORTED; }

#endif

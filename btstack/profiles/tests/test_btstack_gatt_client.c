#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "profiles/btstack_gatt_client.h"

static unsigned init_count;
static unsigned discover_count;
static unsigned read_count;
static unsigned write_count;
static unsigned write_command_count;
static unsigned listen_count;
static unsigned stop_listen_count;
static unsigned cccd_write_count;
static uint16_t last_cccd_configuration;
static btstack_packet_handler_t operation_handler;
static btstack_packet_handler_t notification_handler;
static btstack_gatt_client_event_t last_event;
static unsigned event_count;

void gatt_client_init(void) { init_count++; }
uint8_t gatt_client_discover_primary_services(btstack_packet_handler_t callback, hci_con_handle_t handle) { (void)handle; operation_handler = callback; discover_count++; return 0u; }
uint8_t gatt_client_discover_primary_services_by_uuid16(btstack_packet_handler_t callback, hci_con_handle_t handle, uint16_t uuid) { (void)uuid; return gatt_client_discover_primary_services(callback, handle); }
uint8_t gatt_client_discover_primary_services_by_uuid128(btstack_packet_handler_t callback, hci_con_handle_t handle, const uint8_t *uuid) { (void)uuid; return gatt_client_discover_primary_services(callback, handle); }
uint8_t gatt_client_discover_characteristics_for_handle_range_by_uuid16(btstack_packet_handler_t callback, hci_con_handle_t handle, uint16_t start, uint16_t end, uint16_t uuid) { (void)handle; (void)start; (void)end; (void)uuid; operation_handler = callback; return 0u; }
uint8_t gatt_client_discover_characteristics_for_service(btstack_packet_handler_t callback, hci_con_handle_t handle, gatt_client_service_t *service) { (void)handle; (void)service; operation_handler = callback; return 0u; }
uint8_t gatt_client_discover_characteristics_for_handle_range_by_uuid128(btstack_packet_handler_t callback, hci_con_handle_t handle, uint16_t start, uint16_t end, const uint8_t *uuid) { (void)uuid; return gatt_client_discover_characteristics_for_handle_range_by_uuid16(callback, handle, start, end, 0u); }
uint8_t gatt_client_discover_characteristic_descriptors(btstack_packet_handler_t callback, hci_con_handle_t handle, gatt_client_characteristic_t *characteristic) { (void)handle; (void)characteristic; operation_handler = callback; return 0u; }
uint8_t gatt_client_read_value_of_characteristic_using_value_handle(btstack_packet_handler_t callback, hci_con_handle_t handle, uint16_t value_handle) { (void)handle; (void)value_handle; operation_handler = callback; read_count++; return 0u; }
uint8_t gatt_client_write_value_of_characteristic(btstack_packet_handler_t callback, hci_con_handle_t handle, uint16_t value_handle, uint16_t len, uint8_t *value) { (void)handle; (void)value_handle; (void)len; (void)value; operation_handler = callback; write_count++; return 0u; }
uint8_t gatt_client_write_value_of_characteristic_without_response(hci_con_handle_t handle, uint16_t value_handle, uint16_t len, uint8_t *value) { (void)handle; (void)value_handle; (void)len; (void)value; write_command_count++; return 0u; }
uint8_t gatt_client_write_client_characteristic_configuration(btstack_packet_handler_t callback, hci_con_handle_t handle, gatt_client_characteristic_t *characteristic, uint16_t configuration) { (void)handle; (void)characteristic; operation_handler = callback; cccd_write_count++; last_cccd_configuration = configuration; return 0u; }
void gatt_client_listen_for_characteristic_value_updates(gatt_client_notification_t *registration, btstack_packet_handler_t callback, hci_con_handle_t handle, gatt_client_characteristic_t *characteristic) { (void)registration; (void)handle; (void)characteristic; notification_handler = callback; listen_count++; }
void gatt_client_stop_listening_for_characteristic_value_updates(gatt_client_notification_t *registration) { (void)registration; stop_listen_count++; }

uint16_t little_endian_read_16(const uint8_t *buffer, int position) { return (uint16_t)(buffer[position] | ((uint16_t)buffer[position + 1] << 8)); }
void reverse_128(const uint8_t *src, uint8_t *dst) { memcpy(dst, src, 16u); }
void gatt_client_deserialize_service(const uint8_t *packet, int offset, gatt_client_service_t *service) { (void)packet; (void)offset; memset(service, 0, sizeof(*service)); }
void gatt_client_deserialize_characteristic(const uint8_t *packet, int offset, gatt_client_characteristic_t *characteristic) { (void)packet; (void)offset; memset(characteristic, 0, sizeof(*characteristic)); }
void gatt_client_deserialize_characteristic_descriptor(const uint8_t *packet, int offset, gatt_client_characteristic_descriptor_t *descriptor) { (void)packet; (void)offset; memset(descriptor, 0, sizeof(*descriptor)); }

static void on_event(const btstack_gatt_client_event_t *event)
{
    assert(btstack_gatt_client_get_operation() == BTSTACK_GATT_CLIENT_OPERATION_NONE ||
        event->type != BTSTACK_GATT_CLIENT_EVENT_COMPLETE);
    last_event = *event;
    event_count++;
}

static void emit_query_complete(uint8_t status)
{
    uint8_t packet[9] = { GATT_EVENT_QUERY_COMPLETE, 7u, 0x40u, 0u, 0u, 0u, 0u, 0u, status };
    operation_handler(HCI_EVENT_PACKET, 0u, packet, sizeof(packet));
}

int main(void)
{
    btstack_uuid_t uuid = { BTSTACK_UUID_TYPE_16, { .uuid16 = 0x180fu } };
    gatt_client_characteristic_t characteristic;
    uint8_t value[BTSTACK_GATT_VALUE_MAX + 1u];

    memset(&characteristic, 0, sizeof(characteristic));
    characteristic.value_handle = 0x0025u;
    assert(btstack_gatt_client_init() == BTSTACK_STATUS_OK);
    assert(btstack_gatt_client_init() == BTSTACK_STATUS_OK);
    assert(init_count == 1u);
    assert(btstack_gatt_client_set_event_callback(on_event) == BTSTACK_STATUS_OK);
    assert(btstack_gatt_client_discover_services(&uuid) == BTSTACK_STATUS_NOT_READY);
    btstack_gatt_client_connection_changed(0x0040u, true);
    assert(btstack_gatt_client_discover_services(&uuid) == BTSTACK_STATUS_OK);
    assert(discover_count == 1u);
    assert(btstack_gatt_client_read(0x0025u) == BTSTACK_STATUS_BUSY);
    emit_query_complete(0u);
    assert(last_event.type == BTSTACK_GATT_CLIENT_EVENT_COMPLETE);
    assert(last_event.status == BTSTACK_STATUS_OK);
    assert(btstack_gatt_client_read(0x0025u) == BTSTACK_STATUS_OK);
    assert(read_count == 1u);
    btstack_gatt_client_connection_changed(0x0040u, false);
    assert(last_event.type == BTSTACK_GATT_CLIENT_EVENT_COMPLETE);
    assert(last_event.status == BTSTACK_STATUS_DISCONNECTED);
    btstack_gatt_client_connection_changed(0x0040u, true);
    assert(btstack_gatt_client_write(0x0025u, value, sizeof(value), false) == BTSTACK_STATUS_INVALID_ARGUMENT);
    assert(btstack_gatt_client_write(0x0025u, value, 4u, true) == BTSTACK_STATUS_OK);
    assert(write_command_count == 1u && btstack_gatt_client_get_operation() == BTSTACK_GATT_CLIENT_OPERATION_NONE);
    assert(btstack_gatt_client_subscribe(&characteristic, true, false) == BTSTACK_STATUS_OK);
    emit_query_complete(0u);
    assert(listen_count == 1u && notification_handler != NULL);
    assert(btstack_gatt_client_unsubscribe() == BTSTACK_STATUS_OK);
    assert(cccd_write_count == 2u && last_cccd_configuration == 0u);
    assert(stop_listen_count == 0u);
    emit_query_complete(0u);
    assert(stop_listen_count == 1u);
    assert(write_count == 0u);
    assert(event_count >= 4u);
    return 0;
}

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "profiles/btstack_hogp_device.h"
#include "profiles/btstack_gatt_server.h"

static uint16_t next_handle = 1u;
static uint16_t report_handles[2];
static unsigned service_count;
static unsigned characteristic_count;
static unsigned descriptor_count;
static unsigned notify_count;
static uint16_t last_notify_handle;

int btstack_gatt_server_add_service(const btstack_uuid_t *uuid, uint16_t *handle)
{
    assert(uuid != NULL && uuid->type == BTSTACK_UUID_TYPE_16);
    service_count++;
    if (handle != NULL) *handle = next_handle++;
    return BTSTACK_STATUS_OK;
}

int btstack_gatt_server_add_characteristic(uint16_t service_handle,
    const btstack_gatt_server_characteristic_config_t *config, uint16_t *handle)
{
    (void)service_handle;
    assert(config != NULL && config->uuid.type == BTSTACK_UUID_TYPE_16);
    characteristic_count++;
    if ((config->uuid.value.uuid16 == 0x2a4du) && (characteristic_count <= 4u)) {
        report_handles[characteristic_count - 3u] = next_handle;
    }
    if (handle != NULL) *handle = next_handle;
    next_handle += 2u;
    return BTSTACK_STATUS_OK;
}

int btstack_gatt_server_add_descriptor(uint16_t value_handle,
    const btstack_gatt_server_descriptor_config_t *config, uint16_t *handle)
{
    (void)value_handle;
    assert(config != NULL && config->uuid.value.uuid16 == 0x2908u);
    descriptor_count++;
    if (handle != NULL) *handle = next_handle;
    next_handle++;
    return BTSTACK_STATUS_OK;
}

int btstack_gatt_server_set_value(uint16_t handle, const uint8_t *value, uint16_t len)
{
    assert(handle != 0u && (value != NULL || len == 0u));
    return BTSTACK_STATUS_OK;
}

int btstack_gatt_server_notify(uint16_t handle, const uint8_t *value, uint16_t len)
{
    assert(value != NULL && len != 0u);
    notify_count++;
    last_notify_handle = handle;
    return BTSTACK_STATUS_OK;
}

int btstack_adapter_le_disconnect(void) { return BTSTACK_STATUS_OK; }

static void emit_subscription(uint16_t handle, uint16_t value)
{
    btstack_gatt_server_event_t event;
    memset(&event, 0, sizeof(event));
    event.type = BTSTACK_GATT_SERVER_EVENT_SUBSCRIPTION_CHANGED;
    event.attribute_handle = handle;
    event.cccd_value = value;
    btstack_hogp_device_handle_server_event(&event);
}

int main(void)
{
    static const uint8_t map[] = { 0x05u, 0x01u, 0x09u, 0x06u };
    static const uint8_t pnp[] = { 1u, 0x34u, 0x12u, 0x78u, 0x56u, 0u, 1u };
    static const btstack_hogp_report_info_t reports[] = { { 1u, 1u }, { 2u, 2u } };
    const btstack_hogp_device_config_t config = {
        map, sizeof(map), pnp, sizeof(pnp), reports, 2u
    };
    const uint8_t report[] = { 1u, 2u, 3u };

    assert(btstack_hogp_device_init() == BTSTACK_STATUS_OK);
    assert(btstack_hogp_device_setup(&config) == BTSTACK_STATUS_OK);
    assert(service_count == 3u);
    assert(characteristic_count == 6u);
    assert(descriptor_count == 2u);
    btstack_hogp_device_connection_changed(0x0040u, true, false);
    emit_subscription(report_handles[0], 1u);
    assert(!btstack_hogp_device_is_ready());
    btstack_hogp_device_security_changed(true);
    assert(btstack_hogp_device_is_ready());
    assert(btstack_hogp_device_send_report(1u, report, sizeof(report)) == BTSTACK_STATUS_OK);
    assert(notify_count == 1u && last_notify_handle == report_handles[0]);
    assert(btstack_hogp_device_send_report(2u, report, sizeof(report)) == BTSTACK_STATUS_NOT_FOUND);
    assert(btstack_hogp_device_disconnect() == BTSTACK_STATUS_OK);
    btstack_hogp_device_connection_changed(0x0040u, false, false);
    assert(!btstack_hogp_device_is_connected());
    assert(!btstack_hogp_device_is_ready());
    return 0;
}

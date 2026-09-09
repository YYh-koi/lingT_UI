#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "profiles/btstack_gatt_server.h"
#include "ble/att_server.h"

static unsigned att_init_count;
static unsigned notify_count;
static unsigned indicate_count;
static uint16_t last_handle;
static uint16_t last_len;
static att_write_callback_t captured_write;

uint16_t little_endian_read_16(const uint8_t *buffer, int position) { return (uint16_t)(buffer[position] | ((uint16_t)buffer[position + 1] << 8)); }

void att_db_util_init(void) { }
uint16_t att_db_util_add_service_uuid16(uint16_t uuid) { static uint16_t h = 1u; (void)uuid; return h++; }
uint16_t att_db_util_add_service_uuid128(const uint8_t *uuid) { (void)uuid; return 1u; }
uint16_t att_db_util_add_characteristic_uuid16(uint16_t uuid, uint16_t properties, uint8_t read, uint8_t write, uint8_t *data, uint16_t len) { static uint16_t h = 2u; (void)uuid; (void)properties; (void)read; (void)write; (void)data; (void)len; return h++ + 1u; }
uint16_t att_db_util_add_characteristic_uuid128(const uint8_t *uuid, uint16_t properties, uint8_t read, uint8_t write, uint8_t *data, uint16_t len) { (void)uuid; (void)properties; (void)read; (void)write; (void)data; (void)len; return 3u; }
uint16_t att_db_util_add_descriptor_uuid16(uint16_t uuid, uint16_t properties, uint8_t read, uint8_t write, uint8_t *data, uint16_t len) { static uint16_t h = 20u; (void)uuid; (void)properties; (void)read; (void)write; (void)data; (void)len; return h++; }
uint16_t att_db_util_add_descriptor_uuid128(const uint8_t *uuid, uint16_t properties, uint8_t read, uint8_t write, uint8_t *data, uint16_t len) { (void)uuid; (void)properties; (void)read; (void)write; (void)data; (void)len; return 20u; }
uint8_t *att_db_util_get_address(void) { static uint8_t db[2]; return db; }
uint16_t att_db_util_get_size(void) { return 2u; }
void att_server_init(const uint8_t *db, att_read_callback_t read, att_write_callback_t write) { assert(db != NULL && read != NULL && write != NULL); captured_write = write; att_init_count++; }
void att_server_register_packet_handler(btstack_packet_handler_t handler) { assert(handler != NULL); }
uint8_t att_server_notify(hci_con_handle_t connection, uint16_t handle, const uint8_t *value, uint16_t len) { (void)connection; (void)value; notify_count++; last_handle = handle; last_len = len; return 0u; }
uint8_t att_server_indicate(hci_con_handle_t connection, uint16_t handle, const uint8_t *value, uint16_t len) { (void)connection; (void)value; indicate_count++; last_handle = handle; last_len = len; return 0u; }

static btstack_uuid_t uuid16(uint16_t value)
{
    btstack_uuid_t uuid;
    memset(&uuid, 0, sizeof(uuid));
    uuid.type = BTSTACK_UUID_TYPE_16;
    uuid.value.uuid16 = value;
    return uuid;
}

static btstack_gatt_server_characteristic_config_t char_config(uint16_t id, uint16_t properties, uint16_t max_len)
{
    btstack_gatt_server_characteristic_config_t config;
    memset(&config, 0, sizeof(config));
    config.uuid = uuid16(id);
    config.properties = properties;
    config.max_len = max_len;
    return config;
}

static void test_counts_and_freeze(void)
{
    uint16_t service;
    uint16_t value_handle;
    unsigned i;

    assert(btstack_gatt_server_init() == BTSTACK_STATUS_OK);
    for (i = 0; i < BTSTACK_GATT_SERVICE_MAX; i++) {
        btstack_uuid_t uuid = uuid16((uint16_t)(0x1800u + i));
        assert(btstack_gatt_server_add_service(&uuid, &service) == BTSTACK_STATUS_OK);
    }
    assert(btstack_gatt_server_add_service(&(btstack_uuid_t){ BTSTACK_UUID_TYPE_16, { .uuid16 = 0x1900u } }, &service) == BTSTACK_STATUS_NO_MEMORY);
    assert(btstack_gatt_server_reset() == BTSTACK_STATUS_OK);
    assert(btstack_gatt_server_add_service(&(btstack_uuid_t){ BTSTACK_UUID_TYPE_16, { .uuid16 = 0x1800u } }, &service) == BTSTACK_STATUS_OK);
    for (i = 0; i < BTSTACK_GATT_CHARACTERISTIC_MAX; i++) {
        btstack_gatt_server_characteristic_config_t config = char_config((uint16_t)(0x2a00u + i), BTSTACK_GATT_PROPERTY_READ, 1u);
        assert(btstack_gatt_server_add_characteristic(service, &config, &value_handle) == BTSTACK_STATUS_OK);
    }
    assert(btstack_gatt_server_add_characteristic(service, &(btstack_gatt_server_characteristic_config_t){ .uuid = { BTSTACK_UUID_TYPE_16, { .uuid16 = 0x2affu } }, .properties = BTSTACK_GATT_PROPERTY_READ, .max_len = 1u }, &value_handle) == BTSTACK_STATUS_NO_MEMORY);
    assert(btstack_gatt_server_build() == BTSTACK_STATUS_OK);
    assert(att_init_count == 1u);
    assert(btstack_gatt_server_add_service(&(btstack_uuid_t){ BTSTACK_UUID_TYPE_16, { .uuid16 = 0x1810u } }, &service) == BTSTACK_STATUS_NOT_READY);
}

static void test_value_pool_and_send(void)
{
    uint16_t service;
    uint16_t handles[4];
    uint8_t value[244];
    uint16_t len;
    unsigned i;

    memset(value, 0x5au, sizeof(value));
    assert(btstack_gatt_server_reset() == BTSTACK_STATUS_OK);
    assert(btstack_gatt_server_add_service(&(btstack_uuid_t){ BTSTACK_UUID_TYPE_16, { .uuid16 = 0x180fu } }, &service) == BTSTACK_STATUS_OK);
    for (i = 0; i < 4u; i++) {
        btstack_gatt_server_characteristic_config_t config = char_config((uint16_t)(0x2a19u + i), BTSTACK_GATT_PROPERTY_READ | BTSTACK_GATT_PROPERTY_NOTIFY | BTSTACK_GATT_PROPERTY_INDICATE, 244u);
        assert(btstack_gatt_server_add_characteristic(service, &config, &handles[i]) == BTSTACK_STATUS_OK);
    }
    assert(btstack_gatt_server_add_descriptor(handles[3], &(btstack_gatt_server_descriptor_config_t){ .uuid = { BTSTACK_UUID_TYPE_16, { .uuid16 = 0x2901u } }, .properties = BTSTACK_GATT_PROPERTY_READ, .max_len = 48u }, NULL) == BTSTACK_STATUS_OK);
    assert(btstack_gatt_server_add_descriptor(handles[3], &(btstack_gatt_server_descriptor_config_t){ .uuid = { BTSTACK_UUID_TYPE_16, { .uuid16 = 0x2904u } }, .properties = BTSTACK_GATT_PROPERTY_READ, .max_len = 1u }, NULL) == BTSTACK_STATUS_NO_MEMORY);
    assert(btstack_gatt_server_set_value(handles[0], value, sizeof(value)) == BTSTACK_STATUS_OK);
    len = sizeof(value);
    memset(value, 0, sizeof(value));
    assert(btstack_gatt_server_get_value(handles[0], value, &len) == BTSTACK_STATUS_OK);
    assert(len == 244u && value[0] == 0x5au);
    assert(btstack_gatt_server_build() == BTSTACK_STATUS_OK);
    btstack_gatt_server_connection_changed(0x0040u, true);
    assert(captured_write(0x0040u, handles[0] + 1u, ATT_TRANSACTION_MODE_NONE, 0u, (uint8_t[]){ 3u, 0u }, 2u) == 0);
    assert(btstack_gatt_server_notify(handles[0], value, 20u) == BTSTACK_STATUS_OK);
    assert(btstack_gatt_server_indicate(handles[0], value, 21u) == BTSTACK_STATUS_OK);
    assert(notify_count == 1u && indicate_count == 1u && last_handle == handles[0] && last_len == 21u);
}

int main(void)
{
    test_counts_and_freeze();
    test_value_pool_and_send();
    return 0;
}

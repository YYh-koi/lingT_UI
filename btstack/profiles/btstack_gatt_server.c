#include "profiles/btstack_gatt_server.h"

#include <stddef.h>
#include <string.h>

#include "ble/att_db_util.h"
#include "ble/att_server.h"
#include "bluetooth.h"
#include "btstack_defines.h"
#include "btstack_event.h"

#if BTSTACK_LE && BTSTACK_GATT_SERVER

typedef enum {
    BTSTACK_GATT_SERVER_ENTRY_SERVICE = 0,
    BTSTACK_GATT_SERVER_ENTRY_CHARACTERISTIC,
    BTSTACK_GATT_SERVER_ENTRY_DESCRIPTOR
} btstack_gatt_server_entry_type_t;

typedef struct {
    btstack_uuid_t uuid;
    uint16_t handle;
} btstack_gatt_server_service_t;

typedef struct {
    btstack_gatt_server_characteristic_config_t config;
    uint16_t service_handle;
    uint16_t value_handle;
    uint16_t cccd_handle;
    uint16_t cccd_value;
    uint16_t value_offset;
    uint16_t value_len;
} btstack_gatt_server_characteristic_t;

typedef struct {
    btstack_gatt_server_descriptor_config_t config;
    uint16_t value_handle;
    uint16_t descriptor_handle;
    uint16_t value_offset;
    uint16_t value_len;
} btstack_gatt_server_descriptor_t;

typedef struct {
    btstack_gatt_server_entry_type_t type;
    uint8_t index;
} btstack_gatt_server_entry_t;

static btstack_gatt_server_service_t btstack_gatt_server_services[BTSTACK_GATT_SERVICE_MAX];
static btstack_gatt_server_characteristic_t btstack_gatt_server_characteristics[BTSTACK_GATT_CHARACTERISTIC_MAX];
static btstack_gatt_server_descriptor_t btstack_gatt_server_descriptors[BTSTACK_GATT_DESCRIPTOR_MAX];
static btstack_gatt_server_entry_t btstack_gatt_server_entries[
    BTSTACK_GATT_SERVICE_MAX + BTSTACK_GATT_CHARACTERISTIC_MAX + BTSTACK_GATT_DESCRIPTOR_MAX];
static uint8_t btstack_gatt_server_value_pool[BTSTACK_GATT_VALUE_POOL_MAX];
static uint8_t btstack_gatt_server_service_count;
static uint8_t btstack_gatt_server_characteristic_count;
static uint8_t btstack_gatt_server_descriptor_count;
static uint8_t btstack_gatt_server_entry_count;
static uint16_t btstack_gatt_server_value_pool_used;
static uint16_t btstack_gatt_server_db_size = 3u;
static uint16_t btstack_gatt_server_next_handle = 1u;
static hci_con_handle_t btstack_gatt_server_connection_handle = HCI_CON_HANDLE_INVALID;
static btstack_gatt_server_event_callback_t btstack_gatt_server_event_callback;
static bool btstack_gatt_server_initialized;
static bool btstack_gatt_server_built;

static bool btstack_gatt_server_uuid_valid(const btstack_uuid_t *uuid)
{
    return (uuid != NULL) && ((uuid->type == BTSTACK_UUID_TYPE_16) || (uuid->type == BTSTACK_UUID_TYPE_128));
}

static uint16_t btstack_gatt_server_uuid_attribute_size(const btstack_uuid_t *uuid)
{
    return uuid->type == BTSTACK_UUID_TYPE_16 ? 8u : 22u;
}

static btstack_gatt_server_characteristic_t *btstack_gatt_server_find_characteristic(uint16_t handle)
{
    uint8_t i;

    for (i = 0u; i < btstack_gatt_server_characteristic_count; i++) {
        if (btstack_gatt_server_characteristics[i].value_handle == handle) {
            return &btstack_gatt_server_characteristics[i];
        }
    }
    return NULL;
}

static btstack_gatt_server_characteristic_t *btstack_gatt_server_find_cccd(uint16_t handle)
{
    uint8_t i;

    for (i = 0u; i < btstack_gatt_server_characteristic_count; i++) {
        if ((btstack_gatt_server_characteristics[i].cccd_handle != 0u) &&
            (btstack_gatt_server_characteristics[i].cccd_handle == handle)) {
            return &btstack_gatt_server_characteristics[i];
        }
    }
    return NULL;
}

static btstack_gatt_server_descriptor_t *btstack_gatt_server_find_descriptor(uint16_t handle)
{
    uint8_t i;

    for (i = 0u; i < btstack_gatt_server_descriptor_count; i++) {
        if (btstack_gatt_server_descriptors[i].descriptor_handle == handle) {
            return &btstack_gatt_server_descriptors[i];
        }
    }
    return NULL;
}

static bool btstack_gatt_server_has_service(uint16_t handle)
{
    uint8_t i;

    for (i = 0u; i < btstack_gatt_server_service_count; i++) {
        if (btstack_gatt_server_services[i].handle == handle) {
            return true;
        }
    }
    return false;
}

static uint16_t btstack_gatt_server_read_blob(const uint8_t *data, uint16_t data_len, uint16_t offset,
    uint8_t *buffer, uint16_t buffer_size)
{
    uint16_t copy_len;

    if (offset > data_len) {
        return 0u;
    }
    copy_len = data_len - offset;
    if (buffer == NULL) {
        return copy_len;
    }
    if (copy_len > buffer_size) {
        copy_len = buffer_size;
    }
    (void)memcpy(buffer, &data[offset], copy_len);
    return copy_len;
}

static uint16_t btstack_gatt_server_read_callback(hci_con_handle_t connection_handle, uint16_t attribute_handle,
    uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    btstack_gatt_server_characteristic_t *characteristic;
    btstack_gatt_server_descriptor_t *descriptor;
    uint8_t cccd[2];

    (void)connection_handle;
    characteristic = btstack_gatt_server_find_characteristic(attribute_handle);
    if (characteristic != NULL) {
        return btstack_gatt_server_read_blob(&btstack_gatt_server_value_pool[characteristic->value_offset],
            characteristic->value_len, offset, buffer, buffer_size);
    }
    characteristic = btstack_gatt_server_find_cccd(attribute_handle);
    if (characteristic != NULL) {
        cccd[0] = (uint8_t)characteristic->cccd_value;
        cccd[1] = (uint8_t)(characteristic->cccd_value >> 8);
        return btstack_gatt_server_read_blob(cccd, sizeof(cccd), offset, buffer, buffer_size);
    }
    descriptor = btstack_gatt_server_find_descriptor(attribute_handle);
    if (descriptor != NULL) {
        return btstack_gatt_server_read_blob(&btstack_gatt_server_value_pool[descriptor->value_offset],
            descriptor->value_len, offset, buffer, buffer_size);
    }
    return 0u;
}

static void btstack_gatt_server_emit(btstack_gatt_server_event_type_t type, uint16_t attribute_handle,
    uint16_t value_len, uint16_t cccd_value, int status)
{
    btstack_gatt_server_event_t event;

    if (btstack_gatt_server_event_callback == NULL) {
        return;
    }
    (void)memset(&event, 0, sizeof(event));
    event.type = type;
    event.connection_handle = btstack_gatt_server_connection_handle;
    event.attribute_handle = attribute_handle;
    event.value_len = value_len;
    event.cccd_value = cccd_value;
    event.status = status;
    btstack_gatt_server_event_callback(&event);
}

static int btstack_gatt_server_write_callback(hci_con_handle_t connection_handle, uint16_t attribute_handle,
    uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    btstack_gatt_server_characteristic_t *characteristic;
    btstack_gatt_server_descriptor_t *descriptor;
    uint16_t end;

    (void)connection_handle;
    if (transaction_mode == ATT_TRANSACTION_MODE_VALIDATE) {
        return 0;
    }
    if (transaction_mode != ATT_TRANSACTION_MODE_NONE) {
        return ATT_ERROR_WRITE_NOT_PERMITTED;
    }
    characteristic = btstack_gatt_server_find_cccd(attribute_handle);
    if (characteristic != NULL) {
        if ((offset != 0u) || (buffer_size != 2u) || (buffer == NULL)) {
            return ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
        }
        characteristic->cccd_value = (uint16_t)(buffer[0] | ((uint16_t)buffer[1] << 8));
        btstack_gatt_server_emit(BTSTACK_GATT_SERVER_EVENT_SUBSCRIPTION_CHANGED,
            characteristic->value_handle, 0u, characteristic->cccd_value, BTSTACK_STATUS_OK);
        return 0;
    }
    characteristic = btstack_gatt_server_find_characteristic(attribute_handle);
    if (characteristic != NULL) {
        if ((characteristic->config.properties &
            (BTSTACK_GATT_PROPERTY_WRITE | BTSTACK_GATT_PROPERTY_WRITE_WITHOUT_RESPONSE)) == 0u) {
            return ATT_ERROR_WRITE_NOT_PERMITTED;
        }
        end = offset + buffer_size;
        if ((end < offset) || (end > characteristic->config.max_len)) {
            return ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
        }
        if ((buffer_size != 0u) && (buffer == NULL)) {
            return ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
        }
        (void)memcpy(&btstack_gatt_server_value_pool[characteristic->value_offset + offset], buffer, buffer_size);
        if (end > characteristic->value_len) {
            characteristic->value_len = end;
        }
        btstack_gatt_server_emit(BTSTACK_GATT_SERVER_EVENT_VALUE_WRITTEN, attribute_handle,
            characteristic->value_len, 0u, BTSTACK_STATUS_OK);
        return 0;
    }
    descriptor = btstack_gatt_server_find_descriptor(attribute_handle);
    if (descriptor != NULL) {
        if ((descriptor->config.properties &
            (BTSTACK_GATT_PROPERTY_WRITE | BTSTACK_GATT_PROPERTY_WRITE_WITHOUT_RESPONSE)) == 0u) {
            return ATT_ERROR_WRITE_NOT_PERMITTED;
        }
        end = offset + buffer_size;
        if ((end < offset) || (end > descriptor->config.max_len)) {
            return ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
        }
        (void)memcpy(&btstack_gatt_server_value_pool[descriptor->value_offset + offset], buffer, buffer_size);
        if (end > descriptor->value_len) {
            descriptor->value_len = end;
        }
        btstack_gatt_server_emit(BTSTACK_GATT_SERVER_EVENT_VALUE_WRITTEN, attribute_handle,
            descriptor->value_len, 0u, BTSTACK_STATUS_OK);
        return 0;
    }
    return ATT_ERROR_WRITE_NOT_PERMITTED;
}

static void btstack_gatt_server_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    (void)channel;
    if ((packet_type == HCI_EVENT_PACKET) && (packet != NULL) && (size >= 8u) &&
        (hci_event_packet_get_type(packet) == ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE)) {
        btstack_gatt_server_emit(BTSTACK_GATT_SERVER_EVENT_INDICATION_COMPLETE,
            att_event_handle_value_indication_complete_get_attribute_handle(packet), 0u, 0u,
            att_event_handle_value_indication_complete_get_status(packet));
    }
}

int btstack_gatt_server_init(void)
{
    if (btstack_gatt_server_initialized) {
        return BTSTACK_STATUS_OK;
    }
    return btstack_gatt_server_reset();
}

int btstack_gatt_server_reset(void)
{
    (void)memset(btstack_gatt_server_services, 0, sizeof(btstack_gatt_server_services));
    (void)memset(btstack_gatt_server_characteristics, 0, sizeof(btstack_gatt_server_characteristics));
    (void)memset(btstack_gatt_server_descriptors, 0, sizeof(btstack_gatt_server_descriptors));
    (void)memset(btstack_gatt_server_entries, 0, sizeof(btstack_gatt_server_entries));
    (void)memset(btstack_gatt_server_value_pool, 0, sizeof(btstack_gatt_server_value_pool));
    btstack_gatt_server_service_count = 0u;
    btstack_gatt_server_characteristic_count = 0u;
    btstack_gatt_server_descriptor_count = 0u;
    btstack_gatt_server_entry_count = 0u;
    btstack_gatt_server_value_pool_used = 0u;
    btstack_gatt_server_db_size = 3u;
    btstack_gatt_server_next_handle = 1u;
    btstack_gatt_server_connection_handle = HCI_CON_HANDLE_INVALID;
    btstack_gatt_server_built = false;
    btstack_gatt_server_initialized = true;
    return BTSTACK_STATUS_OK;
}

int btstack_gatt_server_set_event_callback(btstack_gatt_server_event_callback_t callback)
{
    btstack_gatt_server_event_callback = callback;
    return BTSTACK_STATUS_OK;
}

int btstack_gatt_server_add_service(const btstack_uuid_t *uuid, uint16_t *service_handle)
{
    btstack_gatt_server_service_t *service;
    uint16_t required;

    if (!btstack_gatt_server_initialized || btstack_gatt_server_built) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (!btstack_gatt_server_uuid_valid(uuid)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (btstack_gatt_server_service_count >= BTSTACK_GATT_SERVICE_MAX) {
        return BTSTACK_STATUS_NO_MEMORY;
    }
    required = uuid->type == BTSTACK_UUID_TYPE_16 ? 10u : 24u;
    if ((uint32_t)btstack_gatt_server_db_size + required > BTSTACK_GATT_DB_MAX) {
        return BTSTACK_STATUS_NO_MEMORY;
    }
    service = &btstack_gatt_server_services[btstack_gatt_server_service_count];
    service->uuid = *uuid;
    service->handle = btstack_gatt_server_next_handle++;
    btstack_gatt_server_entries[btstack_gatt_server_entry_count].type = BTSTACK_GATT_SERVER_ENTRY_SERVICE;
    btstack_gatt_server_entries[btstack_gatt_server_entry_count++].index = btstack_gatt_server_service_count++;
    btstack_gatt_server_db_size += required;
    if (service_handle != NULL) {
        *service_handle = service->handle;
    }
    return BTSTACK_STATUS_OK;
}

int btstack_gatt_server_add_characteristic(uint16_t service_handle,
    const btstack_gatt_server_characteristic_config_t *config, uint16_t *value_handle)
{
    btstack_gatt_server_characteristic_t *characteristic;
    uint16_t db_required;

    if (!btstack_gatt_server_initialized || btstack_gatt_server_built) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if ((config == NULL) || !btstack_gatt_server_uuid_valid(&config->uuid) ||
        (config->max_len == 0u) || (config->max_len > BTSTACK_GATT_VALUE_MAX) ||
        !btstack_gatt_server_has_service(service_handle)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (btstack_gatt_server_characteristic_count >= BTSTACK_GATT_CHARACTERISTIC_MAX) {
        return BTSTACK_STATUS_NO_MEMORY;
    }
    db_required = config->uuid.type == BTSTACK_UUID_TYPE_16 ? 21u : 49u;
    if ((config->properties & (BTSTACK_GATT_PROPERTY_NOTIFY | BTSTACK_GATT_PROPERTY_INDICATE)) != 0u) {
        db_required += 10u;
    }
    if (((uint32_t)btstack_gatt_server_db_size + db_required > BTSTACK_GATT_DB_MAX) ||
        ((uint32_t)btstack_gatt_server_value_pool_used + config->max_len > BTSTACK_GATT_VALUE_POOL_MAX)) {
        return BTSTACK_STATUS_NO_MEMORY;
    }
    characteristic = &btstack_gatt_server_characteristics[btstack_gatt_server_characteristic_count];
    (void)memset(characteristic, 0, sizeof(*characteristic));
    characteristic->config = *config;
    characteristic->service_handle = service_handle;
    characteristic->value_handle = btstack_gatt_server_next_handle + 1u;
    btstack_gatt_server_next_handle += 2u;
    if ((config->properties & (BTSTACK_GATT_PROPERTY_NOTIFY | BTSTACK_GATT_PROPERTY_INDICATE)) != 0u) {
        characteristic->cccd_handle = btstack_gatt_server_next_handle++;
    }
    characteristic->value_offset = btstack_gatt_server_value_pool_used;
    btstack_gatt_server_value_pool_used += config->max_len;
    btstack_gatt_server_entries[btstack_gatt_server_entry_count].type = BTSTACK_GATT_SERVER_ENTRY_CHARACTERISTIC;
    btstack_gatt_server_entries[btstack_gatt_server_entry_count++].index = btstack_gatt_server_characteristic_count++;
    btstack_gatt_server_db_size += db_required;
    if (value_handle != NULL) {
        *value_handle = characteristic->value_handle;
    }
    return BTSTACK_STATUS_OK;
}

int btstack_gatt_server_add_descriptor(uint16_t value_handle,
    const btstack_gatt_server_descriptor_config_t *config, uint16_t *descriptor_handle)
{
    btstack_gatt_server_descriptor_t *descriptor;
    uint16_t required;

    if (!btstack_gatt_server_initialized || btstack_gatt_server_built) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if ((config == NULL) || !btstack_gatt_server_uuid_valid(&config->uuid) ||
        (config->max_len == 0u) || (config->max_len > BTSTACK_GATT_VALUE_MAX) ||
        (btstack_gatt_server_find_characteristic(value_handle) == NULL)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (btstack_gatt_server_descriptor_count >= BTSTACK_GATT_DESCRIPTOR_MAX) {
        return BTSTACK_STATUS_NO_MEMORY;
    }
    required = btstack_gatt_server_uuid_attribute_size(&config->uuid);
    if (((uint32_t)btstack_gatt_server_db_size + required > BTSTACK_GATT_DB_MAX) ||
        ((uint32_t)btstack_gatt_server_value_pool_used + config->max_len > BTSTACK_GATT_VALUE_POOL_MAX)) {
        return BTSTACK_STATUS_NO_MEMORY;
    }
    descriptor = &btstack_gatt_server_descriptors[btstack_gatt_server_descriptor_count];
    (void)memset(descriptor, 0, sizeof(*descriptor));
    descriptor->config = *config;
    descriptor->value_handle = value_handle;
    descriptor->descriptor_handle = btstack_gatt_server_next_handle++;
    descriptor->value_offset = btstack_gatt_server_value_pool_used;
    btstack_gatt_server_value_pool_used += config->max_len;
    btstack_gatt_server_entries[btstack_gatt_server_entry_count].type = BTSTACK_GATT_SERVER_ENTRY_DESCRIPTOR;
    btstack_gatt_server_entries[btstack_gatt_server_entry_count++].index = btstack_gatt_server_descriptor_count++;
    btstack_gatt_server_db_size += required;
    if (descriptor_handle != NULL) {
        *descriptor_handle = descriptor->descriptor_handle;
    }
    return BTSTACK_STATUS_OK;
}

int btstack_gatt_server_build(void)
{
    uint8_t i;

    if (!btstack_gatt_server_initialized || (btstack_gatt_server_service_count == 0u)) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (btstack_gatt_server_built) {
        return BTSTACK_STATUS_OK;
    }
    att_db_util_init();
    for (i = 0u; i < btstack_gatt_server_entry_count; i++) {
        btstack_gatt_server_entry_t *entry = &btstack_gatt_server_entries[i];
        if (entry->type == BTSTACK_GATT_SERVER_ENTRY_SERVICE) {
            btstack_gatt_server_service_t *service = &btstack_gatt_server_services[entry->index];
            if (service->uuid.type == BTSTACK_UUID_TYPE_16) {
                (void)att_db_util_add_service_uuid16(service->uuid.value.uuid16);
            } else {
                (void)att_db_util_add_service_uuid128(service->uuid.value.uuid128);
            }
        } else if (entry->type == BTSTACK_GATT_SERVER_ENTRY_CHARACTERISTIC) {
            btstack_gatt_server_characteristic_t *characteristic = &btstack_gatt_server_characteristics[entry->index];
            uint16_t properties = characteristic->config.properties | BTSTACK_GATT_PROPERTY_DYNAMIC;
            if (characteristic->config.uuid.type == BTSTACK_UUID_TYPE_16) {
                (void)att_db_util_add_characteristic_uuid16(characteristic->config.uuid.value.uuid16, properties,
                    (uint8_t)characteristic->config.read_security, (uint8_t)characteristic->config.write_security, NULL, 0u);
            } else {
                (void)att_db_util_add_characteristic_uuid128(characteristic->config.uuid.value.uuid128, properties,
                    (uint8_t)characteristic->config.read_security, (uint8_t)characteristic->config.write_security, NULL, 0u);
            }
        } else {
            btstack_gatt_server_descriptor_t *descriptor = &btstack_gatt_server_descriptors[entry->index];
            uint16_t properties = descriptor->config.properties | BTSTACK_GATT_PROPERTY_DYNAMIC;
            if (descriptor->config.uuid.type == BTSTACK_UUID_TYPE_16) {
                (void)att_db_util_add_descriptor_uuid16(descriptor->config.uuid.value.uuid16, properties,
                    (uint8_t)descriptor->config.read_security, (uint8_t)descriptor->config.write_security, NULL, 0u);
            } else {
                (void)att_db_util_add_descriptor_uuid128(descriptor->config.uuid.value.uuid128, properties,
                    (uint8_t)descriptor->config.read_security, (uint8_t)descriptor->config.write_security, NULL, 0u);
            }
        }
    }
    if (att_db_util_get_size() > BTSTACK_GATT_DB_MAX) {
        return BTSTACK_STATUS_NO_MEMORY;
    }
    att_server_init(att_db_util_get_address(), btstack_gatt_server_read_callback, btstack_gatt_server_write_callback);
    att_server_register_packet_handler(btstack_gatt_server_packet_handler);
    btstack_gatt_server_built = true;
    return BTSTACK_STATUS_OK;
}

int btstack_gatt_server_set_value(uint16_t attribute_handle, const uint8_t *value, uint16_t value_len)
{
    btstack_gatt_server_characteristic_t *characteristic = btstack_gatt_server_find_characteristic(attribute_handle);
    btstack_gatt_server_descriptor_t *descriptor = btstack_gatt_server_find_descriptor(attribute_handle);
    uint16_t offset;
    uint16_t max_len;
    uint16_t *stored_len;

    if ((value == NULL) && (value_len != 0u)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (characteristic != NULL) {
        offset = characteristic->value_offset;
        max_len = characteristic->config.max_len;
        stored_len = &characteristic->value_len;
    } else if (descriptor != NULL) {
        offset = descriptor->value_offset;
        max_len = descriptor->config.max_len;
        stored_len = &descriptor->value_len;
    } else {
        return BTSTACK_STATUS_NOT_FOUND;
    }
    if (value_len > max_len) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (value_len != 0u) {
        (void)memcpy(&btstack_gatt_server_value_pool[offset], value, value_len);
    }
    *stored_len = value_len;
    return BTSTACK_STATUS_OK;
}

int btstack_gatt_server_get_value(uint16_t attribute_handle, uint8_t *value, uint16_t *value_len)
{
    btstack_gatt_server_characteristic_t *characteristic = btstack_gatt_server_find_characteristic(attribute_handle);
    btstack_gatt_server_descriptor_t *descriptor = btstack_gatt_server_find_descriptor(attribute_handle);
    uint16_t offset;
    uint16_t stored_len;

    if (value_len == NULL) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (characteristic != NULL) {
        offset = characteristic->value_offset;
        stored_len = characteristic->value_len;
    } else if (descriptor != NULL) {
        offset = descriptor->value_offset;
        stored_len = descriptor->value_len;
    } else {
        return BTSTACK_STATUS_NOT_FOUND;
    }
    if ((value == NULL) && (stored_len != 0u)) {
        *value_len = stored_len;
        return BTSTACK_STATUS_OK;
    }
    if (*value_len < stored_len) {
        *value_len = stored_len;
        return BTSTACK_STATUS_NO_MEMORY;
    }
    if (stored_len != 0u) {
        (void)memcpy(value, &btstack_gatt_server_value_pool[offset], stored_len);
    }
    *value_len = stored_len;
    return BTSTACK_STATUS_OK;
}

void btstack_gatt_server_connection_changed(uint16_t connection_handle, bool connected)
{
    uint8_t i;

    btstack_gatt_server_connection_handle = connected ? connection_handle : HCI_CON_HANDLE_INVALID;
    if (!connected) {
        for (i = 0u; i < btstack_gatt_server_characteristic_count; i++) {
            btstack_gatt_server_characteristics[i].cccd_value = 0u;
        }
    }
}

static int btstack_gatt_server_send(uint16_t value_handle, const uint8_t *value, uint16_t value_len, bool indicate)
{
    btstack_gatt_server_characteristic_t *characteristic = btstack_gatt_server_find_characteristic(value_handle);
    uint16_t required_cccd = indicate ? 0x0002u : 0x0001u;
    uint16_t required_property = indicate ? BTSTACK_GATT_PROPERTY_INDICATE : BTSTACK_GATT_PROPERTY_NOTIFY;
    uint8_t status;

    if ((value == NULL) && (value_len != 0u)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (!btstack_gatt_server_built || (btstack_gatt_server_connection_handle == HCI_CON_HANDLE_INVALID)) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (characteristic == NULL) {
        return BTSTACK_STATUS_NOT_FOUND;
    }
    if (((characteristic->config.properties & required_property) == 0u) ||
        ((characteristic->cccd_value & required_cccd) == 0u)) {
        return BTSTACK_STATUS_NOT_READY;
    }
    if (value_len > characteristic->config.max_len) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    status = indicate ?
        att_server_indicate(btstack_gatt_server_connection_handle, value_handle, value, value_len) :
        att_server_notify(btstack_gatt_server_connection_handle, value_handle, value, value_len);
    return status == ERROR_CODE_SUCCESS ? BTSTACK_STATUS_OK : BTSTACK_STATUS_BUSY;
}

int btstack_gatt_server_notify(uint16_t value_handle, const uint8_t *value, uint16_t value_len)
{
    return btstack_gatt_server_send(value_handle, value, value_len, false);
}

int btstack_gatt_server_indicate(uint16_t value_handle, const uint8_t *value, uint16_t value_len)
{
    return btstack_gatt_server_send(value_handle, value, value_len, true);
}

#else

int btstack_gatt_server_init(void) { return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_gatt_server_reset(void) { return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_gatt_server_set_event_callback(btstack_gatt_server_event_callback_t callback)
{ (void)callback; return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_gatt_server_add_service(const btstack_uuid_t *uuid, uint16_t *service_handle)
{ (void)uuid; (void)service_handle; return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_gatt_server_add_characteristic(uint16_t service_handle,
    const btstack_gatt_server_characteristic_config_t *config, uint16_t *value_handle)
{ (void)service_handle; (void)config; (void)value_handle; return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_gatt_server_add_descriptor(uint16_t value_handle,
    const btstack_gatt_server_descriptor_config_t *config, uint16_t *descriptor_handle)
{ (void)value_handle; (void)config; (void)descriptor_handle; return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_gatt_server_build(void) { return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_gatt_server_set_value(uint16_t attribute_handle, const uint8_t *value, uint16_t value_len)
{ (void)attribute_handle; (void)value; (void)value_len; return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_gatt_server_get_value(uint16_t attribute_handle, uint8_t *value, uint16_t *value_len)
{ (void)attribute_handle; (void)value; (void)value_len; return BTSTACK_STATUS_NOT_SUPPORTED; }
void btstack_gatt_server_connection_changed(uint16_t connection_handle, bool connected)
{ (void)connection_handle; (void)connected; }
int btstack_gatt_server_notify(uint16_t value_handle, const uint8_t *value, uint16_t value_len)
{ (void)value_handle; (void)value; (void)value_len; return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_gatt_server_indicate(uint16_t value_handle, const uint8_t *value, uint16_t value_len)
{ (void)value_handle; (void)value; (void)value_len; return BTSTACK_STATUS_NOT_SUPPORTED; }

#endif

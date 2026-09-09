#include "profiles/btstack_hogp_device.h"

#include <string.h>

#include "adapter/btstack_adapter.h"
#include "bluetooth_gatt.h"
#include "profiles/btstack_gatt_server.h"

#define BTSTACK_HOGP_INVALID_HANDLE 0xffffu
#define BTSTACK_HOGP_INPUT_REPORT 1u

#if BTSTACK_LE && BTSTACK_HOGP_DEVICE
typedef struct {
    btstack_hogp_report_info_t info;
    uint16_t value_handle;
    bool subscribed;
} btstack_hogp_report_t;

static uint8_t btstack_hogp_report_map[BTSTACK_HOGP_DESCRIPTOR_MAX];
static uint16_t btstack_hogp_report_map_len;
static uint8_t btstack_hogp_pnp_id[BTSTACK_HID_PNP_ID_LEN];
static uint16_t btstack_hogp_pnp_id_len;
static btstack_hogp_report_t btstack_hogp_reports[BTSTACK_HOGP_REPORT_INFO_MAX];
static uint8_t btstack_hogp_report_count;
static bool btstack_hogp_initialized;
static bool btstack_hogp_setup_done;
static bool btstack_hogp_connected;
static bool btstack_hogp_encrypted;
static uint16_t btstack_hogp_connection_handle = BTSTACK_HOGP_INVALID_HANDLE;

static btstack_uuid_t btstack_hogp_uuid16(uint16_t value)
{
    btstack_uuid_t uuid;
    memset(&uuid, 0, sizeof(uuid));
    uuid.type = BTSTACK_UUID_TYPE_16;
    uuid.value.uuid16 = value;
    return uuid;
}

static btstack_gatt_server_characteristic_config_t btstack_hogp_characteristic(
    uint16_t uuid, uint16_t properties, uint16_t max_len)
{
    btstack_gatt_server_characteristic_config_t config;
    memset(&config, 0, sizeof(config));
    config.uuid = btstack_hogp_uuid16(uuid);
    config.properties = properties;
    config.max_len = max_len;
    config.read_security = BTSTACK_GATT_SECURITY_ENCRYPTED;
    config.write_security = BTSTACK_GATT_SECURITY_ENCRYPTED;
    return config;
}

static int btstack_hogp_add_characteristic(uint16_t service_handle, uint16_t uuid,
    uint16_t properties, uint16_t max_len, const uint8_t *value, uint16_t value_len,
    uint16_t *value_handle)
{
    btstack_gatt_server_characteristic_config_t config =
        btstack_hogp_characteristic(uuid, properties, max_len);
    int status = btstack_gatt_server_add_characteristic(service_handle, &config, value_handle);
    if ((status == BTSTACK_STATUS_OK) && (value_len != 0u)) {
        status = btstack_gatt_server_set_value(*value_handle, value, value_len);
    }
    return status;
}

static bool btstack_hogp_has_input_subscription(void)
{
    uint8_t i;
    for (i = 0u; i < btstack_hogp_report_count; i++) {
        if ((btstack_hogp_reports[i].info.report_type == BTSTACK_HOGP_INPUT_REPORT) &&
            btstack_hogp_reports[i].subscribed) return true;
    }
    return false;
}
#endif

int btstack_hogp_device_init(void)
{
#if BTSTACK_LE && BTSTACK_HOGP_DEVICE
    memset(btstack_hogp_report_map, 0, sizeof(btstack_hogp_report_map));
    memset(btstack_hogp_pnp_id, 0, sizeof(btstack_hogp_pnp_id));
    memset(btstack_hogp_reports, 0, sizeof(btstack_hogp_reports));
    btstack_hogp_report_map_len = 0u;
    btstack_hogp_pnp_id_len = 0u;
    btstack_hogp_report_count = 0u;
    btstack_hogp_setup_done = false;
    btstack_hogp_connected = false;
    btstack_hogp_encrypted = false;
    btstack_hogp_connection_handle = BTSTACK_HOGP_INVALID_HANDLE;
    btstack_hogp_initialized = true;
    return BTSTACK_STATUS_OK;
#else
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hogp_device_set_report_map(const uint8_t *report_map, uint16_t report_map_len)
{
#if BTSTACK_LE && BTSTACK_HOGP_DEVICE
    if ((report_map == NULL) || (report_map_len == 0u) ||
        (report_map_len > BTSTACK_HOGP_DESCRIPTOR_MAX)) return BTSTACK_STATUS_INVALID_ARGUMENT;
    memcpy(btstack_hogp_report_map, report_map, report_map_len);
    btstack_hogp_report_map_len = report_map_len;
    return BTSTACK_STATUS_OK;
#else
    (void)report_map; (void)report_map_len;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hogp_device_set_pnp_id(const uint8_t *pnp_id, uint16_t pnp_id_len)
{
#if BTSTACK_LE && BTSTACK_HOGP_DEVICE
    if ((pnp_id == NULL) || (pnp_id_len != BTSTACK_HID_PNP_ID_LEN)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    memcpy(btstack_hogp_pnp_id, pnp_id, pnp_id_len);
    btstack_hogp_pnp_id_len = pnp_id_len;
    return BTSTACK_STATUS_OK;
#else
    (void)pnp_id; (void)pnp_id_len;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hogp_device_set_report_info(const btstack_hogp_report_info_t *reports,
    uint8_t report_count)
{
#if BTSTACK_LE && BTSTACK_HOGP_DEVICE
    uint8_t i;
    if ((reports == NULL) || (report_count == 0u) ||
        (report_count > BTSTACK_HOGP_REPORT_INFO_MAX)) return BTSTACK_STATUS_INVALID_ARGUMENT;
    memset(btstack_hogp_reports, 0, sizeof(btstack_hogp_reports));
    for (i = 0u; i < report_count; i++) {
        if ((reports[i].report_type < 1u) || (reports[i].report_type > 3u)) {
            return BTSTACK_STATUS_INVALID_ARGUMENT;
        }
        btstack_hogp_reports[i].info = reports[i];
    }
    btstack_hogp_report_count = report_count;
    return BTSTACK_STATUS_OK;
#else
    (void)reports; (void)report_count;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hogp_device_setup(const btstack_hogp_device_config_t *config)
{
#if BTSTACK_LE && BTSTACK_HOGP_DEVICE
    static const uint8_t hid_information[4] = { 0x11u, 0x01u, 0u, 0x02u };
    static const uint8_t battery_level = 100u;
    uint16_t hid_service;
    uint16_t battery_service;
    uint16_t device_info_service;
    uint16_t value_handle;
    uint8_t i;
    int status;

    if (!btstack_hogp_initialized || (config == NULL) || (config->report_map == NULL) ||
        (config->report_map_len == 0u) || (config->reports == NULL) ||
        (config->report_count == 0u)) return BTSTACK_STATUS_INVALID_ARGUMENT;
    status = btstack_hogp_device_set_report_map(config->report_map, config->report_map_len);
    if (status != BTSTACK_STATUS_OK) return status;
    status = btstack_hogp_device_set_report_info(config->reports, config->report_count);
    if (status != BTSTACK_STATUS_OK) return status;
    if (config->pnp_id != NULL) {
        status = btstack_hogp_device_set_pnp_id(config->pnp_id, config->pnp_id_len);
        if (status != BTSTACK_STATUS_OK) return status;
    }

    status = btstack_gatt_server_add_service(
        &(btstack_uuid_t){ BTSTACK_UUID_TYPE_16, { .uuid16 = ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE } },
        &hid_service);
    if (status != BTSTACK_STATUS_OK) return status;
    status = btstack_hogp_add_characteristic(hid_service,
        ORG_BLUETOOTH_CHARACTERISTIC_HID_INFORMATION, BTSTACK_GATT_PROPERTY_READ,
        sizeof(hid_information), hid_information, sizeof(hid_information), &value_handle);
    if (status != BTSTACK_STATUS_OK) return status;
    status = btstack_hogp_add_characteristic(hid_service,
        ORG_BLUETOOTH_CHARACTERISTIC_REPORT_MAP, BTSTACK_GATT_PROPERTY_READ,
        BTSTACK_HOGP_DESCRIPTOR_MAX, btstack_hogp_report_map,
        btstack_hogp_report_map_len, &value_handle);
    if (status != BTSTACK_STATUS_OK) return status;
    for (i = 0u; i < btstack_hogp_report_count; i++) {
        uint16_t properties = BTSTACK_GATT_PROPERTY_READ;
        uint8_t reference[2];
        btstack_gatt_server_descriptor_config_t descriptor;
        if (btstack_hogp_reports[i].info.report_type == BTSTACK_HOGP_INPUT_REPORT) {
            properties |= BTSTACK_GATT_PROPERTY_NOTIFY;
        } else {
            properties |= BTSTACK_GATT_PROPERTY_WRITE | BTSTACK_GATT_PROPERTY_WRITE_WITHOUT_RESPONSE;
        }
        status = btstack_hogp_add_characteristic(hid_service,
            ORG_BLUETOOTH_CHARACTERISTIC_REPORT, properties, BTSTACK_HOGP_REPORT_MAX,
            NULL, 0u, &btstack_hogp_reports[i].value_handle);
        if (status != BTSTACK_STATUS_OK) return status;
        descriptor = btstack_hogp_characteristic(
            ORG_BLUETOOTH_DESCRIPTOR_REPORT_REFERENCE, BTSTACK_GATT_PROPERTY_READ, sizeof(reference));
        status = btstack_gatt_server_add_descriptor(btstack_hogp_reports[i].value_handle,
            &descriptor, &value_handle);
        if (status != BTSTACK_STATUS_OK) return status;
        reference[0] = btstack_hogp_reports[i].info.report_id;
        reference[1] = btstack_hogp_reports[i].info.report_type;
        status = btstack_gatt_server_set_value(value_handle, reference, sizeof(reference));
        if (status != BTSTACK_STATUS_OK) return status;
    }

    status = btstack_gatt_server_add_service(
        &(btstack_uuid_t){ BTSTACK_UUID_TYPE_16, { .uuid16 = ORG_BLUETOOTH_SERVICE_BATTERY_SERVICE } },
        &battery_service);
    if (status != BTSTACK_STATUS_OK) return status;
    status = btstack_hogp_add_characteristic(battery_service,
        ORG_BLUETOOTH_CHARACTERISTIC_BATTERY_LEVEL,
        BTSTACK_GATT_PROPERTY_READ | BTSTACK_GATT_PROPERTY_NOTIFY,
        1u, &battery_level, 1u, &value_handle);
    if (status != BTSTACK_STATUS_OK) return status;

    status = btstack_gatt_server_add_service(
        &(btstack_uuid_t){ BTSTACK_UUID_TYPE_16, { .uuid16 = ORG_BLUETOOTH_SERVICE_DEVICE_INFORMATION } },
        &device_info_service);
    if (status != BTSTACK_STATUS_OK) return status;
    if (btstack_hogp_pnp_id_len != 0u) {
        status = btstack_hogp_add_characteristic(device_info_service,
            ORG_BLUETOOTH_CHARACTERISTIC_PNP_ID, BTSTACK_GATT_PROPERTY_READ,
            BTSTACK_HID_PNP_ID_LEN, btstack_hogp_pnp_id,
            btstack_hogp_pnp_id_len, &value_handle);
        if (status != BTSTACK_STATUS_OK) return status;
    }
    btstack_hogp_setup_done = true;
    return BTSTACK_STATUS_OK;
#else
    (void)config;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

void btstack_hogp_device_handle_server_event(const btstack_gatt_server_event_t *event)
{
#if BTSTACK_LE && BTSTACK_HOGP_DEVICE
    uint8_t i;
    if ((event == NULL) ||
        (event->type != BTSTACK_GATT_SERVER_EVENT_SUBSCRIPTION_CHANGED)) return;
    for (i = 0u; i < btstack_hogp_report_count; i++) {
        if (btstack_hogp_reports[i].value_handle == event->attribute_handle) {
            btstack_hogp_reports[i].subscribed = (event->cccd_value & 1u) != 0u;
            return;
        }
    }
#else
    (void)event;
#endif
}

void btstack_hogp_device_connection_changed(uint16_t connection_handle, bool connected, bool encrypted)
{
#if BTSTACK_LE && BTSTACK_HOGP_DEVICE
    uint8_t i;
    btstack_hogp_connected = connected;
    btstack_hogp_encrypted = connected && encrypted;
    btstack_hogp_connection_handle = connected ? connection_handle : BTSTACK_HOGP_INVALID_HANDLE;
    if (!connected) {
        for (i = 0u; i < btstack_hogp_report_count; i++) {
            btstack_hogp_reports[i].subscribed = false;
        }
    }
#else
    (void)connection_handle; (void)connected; (void)encrypted;
#endif
}

void btstack_hogp_device_security_changed(bool encrypted)
{
#if BTSTACK_LE && BTSTACK_HOGP_DEVICE
    btstack_hogp_encrypted = btstack_hogp_connected && encrypted;
#else
    (void)encrypted;
#endif
}

bool btstack_hogp_device_is_connected(void)
{
#if BTSTACK_LE && BTSTACK_HOGP_DEVICE
    return btstack_hogp_connected;
#else
    return false;
#endif
}

bool btstack_hogp_device_is_ready(void)
{
#if BTSTACK_LE && BTSTACK_HOGP_DEVICE
    return btstack_hogp_setup_done && btstack_hogp_connected &&
        btstack_hogp_encrypted && btstack_hogp_has_input_subscription();
#else
    return false;
#endif
}

uint16_t btstack_hogp_device_get_connection_handle(void)
{
#if BTSTACK_LE && BTSTACK_HOGP_DEVICE
    return btstack_hogp_connection_handle;
#else
    return BTSTACK_HOGP_INVALID_HANDLE;
#endif
}

int btstack_hogp_device_disconnect(void)
{
#if BTSTACK_LE && BTSTACK_HOGP_DEVICE
    if (!btstack_hogp_connected) return BTSTACK_STATUS_NOT_READY;
    return btstack_adapter_le_disconnect();
#else
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hogp_device_send_report(uint8_t report_id, const uint8_t *report, uint16_t report_len)
{
    return btstack_hogp_device_send_raw_report(BTSTACK_HOGP_INPUT_REPORT,
        report_id, report, report_len);
}

int btstack_hogp_device_send_raw_report(uint8_t report_type, uint8_t report_id,
    const uint8_t *report, uint16_t report_len)
{
#if BTSTACK_LE && BTSTACK_HOGP_DEVICE
    uint8_t i;
    if ((report == NULL) || (report_len == 0u) ||
        (report_len > BTSTACK_HOGP_REPORT_MAX)) return BTSTACK_STATUS_INVALID_ARGUMENT;
    if (!btstack_hogp_device_is_ready()) return BTSTACK_STATUS_NOT_READY;
    if (report_type != BTSTACK_HOGP_INPUT_REPORT) return BTSTACK_STATUS_NOT_SUPPORTED;
    for (i = 0u; i < btstack_hogp_report_count; i++) {
        if ((btstack_hogp_reports[i].info.report_type == report_type) &&
            (btstack_hogp_reports[i].info.report_id == report_id)) {
            if (!btstack_hogp_reports[i].subscribed) return BTSTACK_STATUS_NOT_READY;
            return btstack_gatt_server_notify(btstack_hogp_reports[i].value_handle,
                report, report_len);
        }
    }
    return BTSTACK_STATUS_NOT_FOUND;
#else
    (void)report_type; (void)report_id; (void)report; (void)report_len;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

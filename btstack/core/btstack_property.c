#include "adapter/btstack_property.h"

#include <string.h>

#include "adapter/btstack_port_ops.h"
#include "port/btstack_platform.h"

#define BTSTACK_PROPERTY_DEFAULT_NAME "GP-BT-ATBM6165"
#define BTSTACK_PROPERTY_STORAGE_TAG  0u
#define BTSTACK_PROPERTY_ADDR_MAGIC   0x42544341u
#define BTSTACK_PROPERTY_ADDR_VERSION 1u
#define BTSTACK_PROPERTY_ADDR_LEN     12u

typedef struct {
    uint8_t address[BTSTACK_BD_ADDR_LEN];
    btstack_address_type_t address_type;
    bool valid;
} btstack_property_address_t;

static btstack_property_address_t btstack_property_classic_address;
static btstack_property_address_t btstack_property_ble_address;
static char btstack_property_classic_name[BTSTACK_LOCAL_NAME_MAX];
static char btstack_property_ble_name[BTSTACK_LOCAL_NAME_MAX];
static bool btstack_property_loaded;

static uint32_t btstack_property_read_u32(const uint8_t *buffer)
{
    return ((uint32_t)buffer[0]) |
           (((uint32_t)buffer[1]) << 8) |
           (((uint32_t)buffer[2]) << 16) |
           (((uint32_t)buffer[3]) << 24);
}

static void btstack_property_write_u32(uint8_t *buffer, uint32_t value)
{
    buffer[0] = (uint8_t)(value & 0xffu);
    buffer[1] = (uint8_t)((value >> 8) & 0xffu);
    buffer[2] = (uint8_t)((value >> 16) & 0xffu);
    buffer[3] = (uint8_t)((value >> 24) & 0xffu);
}

static bool btstack_property_address_valid(
    const uint8_t address[BTSTACK_BD_ADDR_LEN])
{
    uint8_t i;
    bool any_zero_diff = false;
    bool any_ff_diff = false;

    if (address == 0) {
        return false;
    }

    for (i = 0u; i < BTSTACK_BD_ADDR_LEN; i++) {
        if (address[i] != 0x00u) {
            any_zero_diff = true;
        }
        if (address[i] != 0xffu) {
            any_ff_diff = true;
        }
    }

    return any_zero_diff && any_ff_diff;
}

static bool btstack_property_address_type_valid(
    btstack_address_type_t address_type)
{
    return (address_type == BTSTACK_ADDRESS_TYPE_PUBLIC) ||
           (address_type == BTSTACK_ADDRESS_TYPE_RANDOM) ||
           (address_type == BTSTACK_ADDRESS_TYPE_PUBLIC_IDENTITY) ||
           (address_type == BTSTACK_ADDRESS_TYPE_RANDOM_IDENTITY);
}

static bool btstack_property_name_valid(const char *name)
{
    uint16_t i;
    bool any_ff_diff = false;

    if ((name == 0) || (name[0] == '\0')) {
        return false;
    }

    for (i = 0u; (i < BTSTACK_LOCAL_NAME_MAX) && (name[i] != '\0'); i++) {
        if (((uint8_t)name[i]) != 0xffu) {
            any_ff_diff = true;
        }
    }

    return any_ff_diff;
}

static void btstack_property_copy_name(char *dst, const char *name)
{
    uint16_t i;

    memset(dst, 0, BTSTACK_LOCAL_NAME_MAX);
    for (i = 0u; i < (BTSTACK_LOCAL_NAME_MAX - 1u); i++) {
        if (name[i] == '\0') {
            break;
        }
        dst[i] = name[i];
    }
}

static const btstack_port_ops_storage_ops_t *btstack_property_storage(void)
{
    const btstack_port_ops_t *ops = btstack_port_ops_get_ops();

    if ((ops == 0) || (ops->storage.read == 0)) {
        return 0;
    }
    return &ops->storage;
}

static int btstack_property_store_address(
    btstack_port_ops_storage_key_t key,
    const btstack_property_address_t *address)
{
    uint8_t record[BTSTACK_PROPERTY_ADDR_LEN];
    const btstack_port_ops_t *ops = btstack_port_ops_get_ops();

    if ((ops == 0) || (ops->storage.write == 0) || (address == 0) ||
        !address->valid) {
        return BTSTACK_STATUS_NOT_SUPPORTED;
    }

    memset(record, 0, sizeof(record));
    btstack_property_write_u32(record, BTSTACK_PROPERTY_ADDR_MAGIC);
    record[4] = BTSTACK_PROPERTY_ADDR_VERSION;
    record[5] = address->address_type;
    memcpy(&record[6], address->address, BTSTACK_BD_ADDR_LEN);

    return ops->storage.write(
        ops->storage.context,
        key,
        BTSTACK_PROPERTY_STORAGE_TAG,
        record,
        (uint16_t)sizeof(record));
}

static bool btstack_property_load_address(
    btstack_port_ops_storage_key_t key,
    btstack_property_address_t *address)
{
    uint8_t record[BTSTACK_PROPERTY_ADDR_LEN];
    uint16_t record_len = (uint16_t)sizeof(record);
    const btstack_port_ops_storage_ops_t *storage;

    storage = btstack_property_storage();
    if ((storage == 0) || (address == 0)) {
        return false;
    }

    if (storage->read(
            storage->context,
            key,
            BTSTACK_PROPERTY_STORAGE_TAG,
            record,
            &record_len) != BTSTACK_STATUS_OK) {
        return false;
    }

    if ((record_len != sizeof(record)) ||
        (btstack_property_read_u32(record) != BTSTACK_PROPERTY_ADDR_MAGIC) ||
        (record[4] != BTSTACK_PROPERTY_ADDR_VERSION) ||
        !btstack_property_address_type_valid(record[5]) ||
        !btstack_property_address_valid(&record[6])) {
        return false;
    }

    address->address_type = record[5];
    memcpy(address->address, &record[6], BTSTACK_BD_ADDR_LEN);
    address->valid = true;
    return true;
}

static int btstack_property_store_name(
    btstack_port_ops_storage_key_t key,
    const char *name)
{
    const btstack_port_ops_t *ops = btstack_port_ops_get_ops();

    if ((ops == 0) || (ops->storage.write == 0) ||
        !btstack_property_name_valid(name)) {
        return BTSTACK_STATUS_NOT_SUPPORTED;
    }

    return ops->storage.write(
        ops->storage.context,
        key,
        BTSTACK_PROPERTY_STORAGE_TAG,
        (const uint8_t *)name,
        (uint16_t)(strlen(name) + 1u));
}

static bool btstack_property_load_name(
    btstack_port_ops_storage_key_t key,
    char *name)
{
    uint8_t buffer[BTSTACK_LOCAL_NAME_MAX];
    uint16_t buffer_len = (uint16_t)sizeof(buffer);
    const btstack_port_ops_storage_ops_t *storage;
    uint16_t i;
    bool terminated = false;

    storage = btstack_property_storage();
    if ((storage == 0) || (name == 0)) {
        return false;
    }

    if (storage->read(
            storage->context,
            key,
            BTSTACK_PROPERTY_STORAGE_TAG,
            buffer,
            &buffer_len) != BTSTACK_STATUS_OK) {
        return false;
    }

    if ((buffer_len == 0u) || (buffer_len > BTSTACK_LOCAL_NAME_MAX)) {
        return false;
    }
    for (i = 0u; i < buffer_len; i++) {
        if (buffer[i] == 0u) {
            terminated = true;
            break;
        }
    }
    if (!terminated) {
        if (buffer_len >= BTSTACK_LOCAL_NAME_MAX) {
            return false;
        }
        buffer[buffer_len] = 0u;
    }
    if (!btstack_property_name_valid((const char *)buffer)) {
        return false;
    }

    btstack_property_copy_name(name, (const char *)buffer);
    return true;
}

static void btstack_property_try_random_address(
    btstack_property_address_t *address,
    btstack_address_type_t fallback_address_type)
{
    const btstack_port_ops_t *ops = btstack_port_ops_get_ops();
    btstack_address_type_t address_type = BTSTACK_ADDRESS_TYPE_UNKNOWN;
    uint8_t random_address[BTSTACK_BD_ADDR_LEN];

    if ((ops == 0) || (ops->controller.get_random_address == 0) ||
        (address == 0)) {
        return;
    }

    if (ops->controller.get_random_address(
            ops->controller.context,
            random_address,
            &address_type) != BTSTACK_STATUS_OK) {
        return;
    }

    if (!btstack_property_address_type_valid(address_type) ||
        !btstack_property_address_valid(random_address)) {
        return;
    }

    memcpy(address->address, random_address, BTSTACK_BD_ADDR_LEN);
    if (fallback_address_type == BTSTACK_ADDRESS_TYPE_PUBLIC) {
        address->address_type = BTSTACK_ADDRESS_TYPE_PUBLIC;
    } else {
        address->address_type = address_type;
    }
    address->valid = true;
}

int btstack_property_init(void)
{
    if (btstack_property_loaded) {
        return BTSTACK_STATUS_OK;
    }

    btstack_property_reset();
    btstack_property_load_address(
        BTSTACK_PORT_STORAGE_CLASSIC_ADDRESS,
        &btstack_property_classic_address);
    btstack_property_load_address(
        BTSTACK_PORT_STORAGE_BLE_ADDRESS,
        &btstack_property_ble_address);

    if (!btstack_property_classic_address.valid) {
        btstack_property_try_random_address(
            &btstack_property_classic_address,
            BTSTACK_ADDRESS_TYPE_PUBLIC);
    }
    if (!btstack_property_ble_address.valid) {
        btstack_property_try_random_address(
            &btstack_property_ble_address,
            BTSTACK_ADDRESS_TYPE_UNKNOWN);
    }

    if (btstack_property_classic_address.valid) {
        btstack_property_classic_address.address_type =
            BTSTACK_ADDRESS_TYPE_PUBLIC;
    }

    btstack_property_load_name(
        BTSTACK_PORT_STORAGE_CLASSIC_NAME,
        btstack_property_classic_name);
    btstack_property_load_name(
        BTSTACK_PORT_STORAGE_BLE_NAME,
        btstack_property_ble_name);

    btstack_property_loaded = true;
    return BTSTACK_STATUS_OK;
}

void btstack_property_reset(void)
{
    btstack_platform_enter_critical();
    memset(&btstack_property_classic_address, 0,
           sizeof(btstack_property_classic_address));
    memset(&btstack_property_ble_address, 0,
           sizeof(btstack_property_ble_address));
    btstack_property_classic_address.address_type =
        BTSTACK_ADDRESS_TYPE_PUBLIC;
    btstack_property_ble_address.address_type =
        BTSTACK_ADDRESS_TYPE_PUBLIC;
    btstack_property_copy_name(
        btstack_property_classic_name,
        BTSTACK_PROPERTY_DEFAULT_NAME);
    btstack_property_copy_name(
        btstack_property_ble_name,
        BTSTACK_PROPERTY_DEFAULT_NAME);
    btstack_property_loaded = false;
    btstack_platform_exit_critical();
}

bool btstack_property_get_local_address(
    uint8_t address[BTSTACK_BD_ADDR_LEN],
    btstack_address_type_t *address_type)
{
    if (!btstack_property_get_classic_address(address)) {
        return false;
    }
    if (address_type != 0) {
        *address_type = BTSTACK_ADDRESS_TYPE_PUBLIC;
    }
    return true;
}

bool btstack_property_get_classic_address(
    uint8_t address[BTSTACK_BD_ADDR_LEN])
{
    btstack_property_init();
    if (address == 0) {
        return false;
    }
    btstack_platform_enter_critical();
    if (!btstack_property_classic_address.valid) {
        btstack_platform_exit_critical();
        return false;
    }
    memcpy(address, btstack_property_classic_address.address,
           BTSTACK_BD_ADDR_LEN);
    btstack_platform_exit_critical();
    return true;
}

bool btstack_property_get_ble_address(
    uint8_t address[BTSTACK_BD_ADDR_LEN],
    btstack_address_type_t *address_type)
{
    btstack_property_init();
    if (address == 0) {
        return false;
    }
    btstack_platform_enter_critical();
    if (!btstack_property_ble_address.valid) {
        btstack_platform_exit_critical();
        return false;
    }
    memcpy(address, btstack_property_ble_address.address,
           BTSTACK_BD_ADDR_LEN);
    if (address_type != 0) {
        *address_type = btstack_property_ble_address.address_type;
    }
    btstack_platform_exit_critical();
    return true;
}

int btstack_property_set_local_address(
    const uint8_t address[BTSTACK_BD_ADDR_LEN],
    bool store)
{
    return btstack_property_set_classic_address(address, store);
}

int btstack_property_set_classic_address(
    const uint8_t address[BTSTACK_BD_ADDR_LEN],
    bool store)
{
    btstack_property_address_t next_address;
    int status;

    btstack_property_init();
    if (!btstack_property_address_valid(address)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    memset(&next_address, 0, sizeof(next_address));
    memcpy(next_address.address, address, BTSTACK_BD_ADDR_LEN);
    next_address.address_type = BTSTACK_ADDRESS_TYPE_PUBLIC;
    next_address.valid = true;

    if (store) {
        status = btstack_property_store_address(
            BTSTACK_PORT_STORAGE_CLASSIC_ADDRESS,
            &next_address);
        if (status != BTSTACK_STATUS_OK) {
            return status;
        }
    }

    btstack_platform_enter_critical();
    btstack_property_classic_address = next_address;
    btstack_platform_exit_critical();
    return BTSTACK_STATUS_OK;
}

int btstack_property_set_ble_address(
    const uint8_t address[BTSTACK_BD_ADDR_LEN],
    btstack_address_type_t address_type,
    bool store)
{
    btstack_property_address_t next_address;
    int status;

    btstack_property_init();
    if (!btstack_property_address_valid(address) ||
        !btstack_property_address_type_valid(address_type)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    memset(&next_address, 0, sizeof(next_address));
    memcpy(next_address.address, address, BTSTACK_BD_ADDR_LEN);
    next_address.address_type = address_type;
    next_address.valid = true;

    if (store) {
        status = btstack_property_store_address(
            BTSTACK_PORT_STORAGE_BLE_ADDRESS,
            &next_address);
        if (status != BTSTACK_STATUS_OK) {
            return status;
        }
    }

    btstack_platform_enter_critical();
    btstack_property_ble_address = next_address;
    btstack_platform_exit_critical();
    return BTSTACK_STATUS_OK;
}

const char *btstack_property_get_local_name(void)
{
    return btstack_property_get_classic_name();
}

const char *btstack_property_get_classic_name(void)
{
    btstack_property_init();
    return btstack_property_classic_name;
}

const char *btstack_property_get_ble_name(void)
{
    btstack_property_init();
    return btstack_property_ble_name;
}

int btstack_property_set_local_name(const char *name, bool store)
{
    char next_name[BTSTACK_LOCAL_NAME_MAX];
    int status;

    btstack_property_init();
    if (!btstack_property_name_valid(name)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    btstack_property_copy_name(next_name, name);
    if (store) {
        status = btstack_property_store_name(
            BTSTACK_PORT_STORAGE_CLASSIC_NAME,
            next_name);
        if (status != BTSTACK_STATUS_OK) {
            return status;
        }
        status = btstack_property_store_name(
            BTSTACK_PORT_STORAGE_BLE_NAME,
            next_name);
        if (status != BTSTACK_STATUS_OK) {
            return status;
        }
    }

    btstack_platform_enter_critical();
    btstack_property_copy_name(
        btstack_property_classic_name,
        next_name);
    btstack_property_copy_name(
        btstack_property_ble_name,
        next_name);
    btstack_platform_exit_critical();
    return BTSTACK_STATUS_OK;
}

int btstack_property_set_classic_name(const char *name, bool store)
{
    char next_name[BTSTACK_LOCAL_NAME_MAX];
    int status;

    btstack_property_init();
    if (!btstack_property_name_valid(name)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    btstack_property_copy_name(next_name, name);
    if (store) {
        status = btstack_property_store_name(
            BTSTACK_PORT_STORAGE_CLASSIC_NAME,
            next_name);
        if (status != BTSTACK_STATUS_OK) {
            return status;
        }
    }
    btstack_platform_enter_critical();
    btstack_property_copy_name(
        btstack_property_classic_name,
        next_name);
    btstack_platform_exit_critical();
    return BTSTACK_STATUS_OK;
}

int btstack_property_set_ble_name(const char *name, bool store)
{
    char next_name[BTSTACK_LOCAL_NAME_MAX];
    int status;

    btstack_property_init();
    if (!btstack_property_name_valid(name)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    btstack_property_copy_name(next_name, name);
    if (store) {
        status = btstack_property_store_name(
            BTSTACK_PORT_STORAGE_BLE_NAME,
            next_name);
        if (status != BTSTACK_STATUS_OK) {
            return status;
        }
    }
    btstack_platform_enter_critical();
    btstack_property_copy_name(
        btstack_property_ble_name,
        next_name);
    btstack_platform_exit_critical();
    return BTSTACK_STATUS_OK;
}

int btstack_property_apply_controller_address(void)
{
    const btstack_port_ops_t *ops = btstack_port_ops_get_ops();
    int status = BTSTACK_STATUS_NOT_SUPPORTED;

    btstack_property_init();
    if ((ops == 0) || (ops->controller.apply_controller_address == 0)) {
        return BTSTACK_STATUS_NOT_SUPPORTED;
    }

    if (btstack_property_classic_address.valid) {
        status = ops->controller.apply_controller_address(
            ops->controller.context,
            BTSTACK_TRANSPORT_CLASSIC,
            btstack_property_classic_address.address,
            btstack_property_classic_address.address_type);
        if (status != BTSTACK_STATUS_OK) {
            return status;
        }
    }

    if (btstack_property_ble_address.valid) {
        status = ops->controller.apply_controller_address(
            ops->controller.context,
            BTSTACK_TRANSPORT_BLE,
            btstack_property_ble_address.address,
            btstack_property_ble_address.address_type);
    }

    return status;
}

#if BTSTACK_BONDING

#define BTSTACK_BOND_MAGIC      0x42544344u
#define BTSTACK_BOND_VERSION    1u
#define BTSTACK_BOND_RECORD_LEN 14u
#define BTSTACK_BOND_ACTIVE_TAG 0xffu
#define BTSTACK_BOND_ACTIVE_LEN 6u

static uint8_t btstack_property_bond_classic_id;
static uint8_t btstack_property_bond_ble_id;
static bool btstack_property_bond_loaded;

static uint32_t btstack_property_bond_read_u32(const uint8_t *buffer)
{
    return ((uint32_t)buffer[0]) |
           (((uint32_t)buffer[1]) << 8) |
           (((uint32_t)buffer[2]) << 16) |
           (((uint32_t)buffer[3]) << 24);
}

static void btstack_property_bond_write_u32(uint8_t *buffer, uint32_t value)
{
    buffer[0] = (uint8_t)(value & 0xffu);
    buffer[1] = (uint8_t)((value >> 8) & 0xffu);
    buffer[2] = (uint8_t)((value >> 16) & 0xffu);
    buffer[3] = (uint8_t)((value >> 24) & 0xffu);
}

static bool btstack_property_bond_transport_valid(
    btstack_transport_t transport)
{
    return (transport == BTSTACK_TRANSPORT_CLASSIC) ||
           (transport == BTSTACK_TRANSPORT_BLE);
}

static bool btstack_property_bond_id_valid(uint8_t id)
{
    return id < (uint8_t)BTSTACK_BOND_ID_COUNT;
}

static bool btstack_property_bond_address_valid(
    const uint8_t address[BTSTACK_BD_ADDR_LEN])
{
    uint8_t i;
    bool any_zero_diff = false;
    bool any_ff_diff = false;

    if (address == 0) {
        return false;
    }

    for (i = 0u; i < BTSTACK_BD_ADDR_LEN; i++) {
        if (address[i] != 0x00u) {
            any_zero_diff = true;
        }
        if (address[i] != 0xffu) {
            any_ff_diff = true;
        }
    }

    return any_zero_diff && any_ff_diff;
}

static bool btstack_property_bond_address_type_valid(
    btstack_address_type_t address_type)
{
    return (address_type == BTSTACK_ADDRESS_TYPE_PUBLIC) ||
           (address_type == BTSTACK_ADDRESS_TYPE_RANDOM) ||
           (address_type == BTSTACK_ADDRESS_TYPE_PUBLIC_IDENTITY) ||
           (address_type == BTSTACK_ADDRESS_TYPE_RANDOM_IDENTITY);
}

static btstack_port_ops_storage_key_t btstack_property_bond_storage_key(
    btstack_transport_t transport)
{
    if (transport == BTSTACK_TRANSPORT_BLE) {
        return BTSTACK_PORT_STORAGE_BLE_BOND;
    }
    return BTSTACK_PORT_STORAGE_CLASSIC_BOND;
}

static uint8_t *btstack_property_bond_id_ptr(btstack_transport_t transport)
{
    if (transport == BTSTACK_TRANSPORT_BLE) {
        return &btstack_property_bond_ble_id;
    }
    return &btstack_property_bond_classic_id;
}

static const btstack_port_ops_storage_ops_t *btstack_property_bond_storage(void)
{
    const btstack_port_ops_t *ops = btstack_port_ops_get_ops();

    if (ops == 0) {
        return 0;
    }
    return &ops->storage;
}

static int btstack_property_bond_active_id(
    btstack_transport_t transport,
    uint8_t id)
{
    uint8_t record[BTSTACK_BOND_ACTIVE_LEN];
    const btstack_port_ops_storage_ops_t *storage;

    storage = btstack_property_bond_storage();
    if ((storage == 0) || (storage->write == 0)) {
        return BTSTACK_STATUS_NOT_SUPPORTED;
    }

    btstack_property_bond_write_u32(record, BTSTACK_BOND_MAGIC);
    record[4] = BTSTACK_BOND_VERSION;
    record[5] = id;
    return storage->write(
        storage->context,
        btstack_property_bond_storage_key(transport),
        BTSTACK_BOND_ACTIVE_TAG,
        record,
        (uint16_t)sizeof(record));
}

static void btstack_property_bond_load_active_id(
    btstack_transport_t transport)
{
    uint8_t record[BTSTACK_BOND_ACTIVE_LEN];
    uint16_t record_len = (uint16_t)sizeof(record);
    const btstack_port_ops_storage_ops_t *storage;
    uint8_t *id_ptr;

    storage = btstack_property_bond_storage();
    id_ptr = btstack_property_bond_id_ptr(transport);
    *id_ptr = 0u;

    if ((storage == 0) || (storage->read == 0)) {
        return;
    }

    if (storage->read(
            storage->context,
            btstack_property_bond_storage_key(transport),
            BTSTACK_BOND_ACTIVE_TAG,
            record,
            &record_len) != BTSTACK_STATUS_OK) {
        return;
    }

    if ((record_len == sizeof(record)) &&
        (btstack_property_bond_read_u32(record) == BTSTACK_BOND_MAGIC) &&
        (record[4] == BTSTACK_BOND_VERSION) &&
        btstack_property_bond_id_valid(record[5])) {
        *id_ptr = record[5];
    }
}

int btstack_property_bond_init(void)
{
    if (btstack_property_bond_loaded) {
        return BTSTACK_STATUS_OK;
    }

    btstack_property_bond_load_active_id(BTSTACK_TRANSPORT_CLASSIC);
    btstack_property_bond_load_active_id(BTSTACK_TRANSPORT_BLE);
    btstack_property_bond_loaded = true;
    return BTSTACK_STATUS_OK;
}

void btstack_property_bond_reset(void)
{
    btstack_property_bond_classic_id = 0u;
    btstack_property_bond_ble_id = 0u;
    btstack_property_bond_loaded = false;
}

int btstack_property_bond_select_id(
    btstack_transport_t transport,
    uint8_t id)
{
    int status;

    if (!btstack_property_bond_transport_valid(transport) ||
        !btstack_property_bond_id_valid(id)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    btstack_property_bond_init();
    status = btstack_property_bond_active_id(transport, id);
    if (status == BTSTACK_STATUS_OK) {
        *btstack_property_bond_id_ptr(transport) = id;
    }
    return status;
}

uint8_t btstack_property_bond_get_id(
    btstack_transport_t transport)
{
    btstack_property_bond_init();
    if (!btstack_property_bond_transport_valid(transport)) {
        return 0u;
    }
    return *btstack_property_bond_id_ptr(transport);
}

int btstack_property_bond_put_peer(
    const btstack_property_bond_peer_t *peer)
{
    uint8_t record[BTSTACK_BOND_RECORD_LEN];
    const btstack_port_ops_storage_ops_t *storage;

    if ((peer == 0) ||
        !btstack_property_bond_transport_valid(peer->transport) ||
        !btstack_property_bond_id_valid(peer->id) ||
        !btstack_property_bond_address_type_valid(peer->address_type) ||
        !btstack_property_bond_address_valid(peer->address)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    storage = btstack_property_bond_storage();
    if ((storage == 0) || (storage->write == 0)) {
        return BTSTACK_STATUS_NOT_SUPPORTED;
    }

    btstack_property_bond_write_u32(record, BTSTACK_BOND_MAGIC);
    record[4] = BTSTACK_BOND_VERSION;
    record[5] = (uint8_t)peer->transport;
    record[6] = peer->id;
    record[7] = peer->address_type;
    memcpy(&record[8], peer->address, BTSTACK_BD_ADDR_LEN);

    return storage->write(
        storage->context,
        btstack_property_bond_storage_key(peer->transport),
        peer->id,
        record,
        (uint16_t)sizeof(record));
}

bool btstack_property_bond_has_peer(
    btstack_transport_t transport,
    uint8_t id)
{
    btstack_property_bond_peer_t peer;

    return btstack_property_bond_get_peer(transport, id, &peer) ==
           BTSTACK_STATUS_OK;
}

int btstack_property_bond_get_peer(
    btstack_transport_t transport,
    uint8_t id,
    btstack_property_bond_peer_t *peer)
{
    uint8_t record[BTSTACK_BOND_RECORD_LEN];
    uint16_t record_len = (uint16_t)sizeof(record);
    const btstack_port_ops_storage_ops_t *storage;

    if (!btstack_property_bond_transport_valid(transport) ||
        !btstack_property_bond_id_valid(id) || (peer == 0)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    storage = btstack_property_bond_storage();
    if ((storage == 0) || (storage->read == 0)) {
        return BTSTACK_STATUS_NOT_SUPPORTED;
    }

    if (storage->read(
            storage->context,
            btstack_property_bond_storage_key(transport),
            id,
            record,
            &record_len) != BTSTACK_STATUS_OK) {
        return BTSTACK_STATUS_NOT_FOUND;
    }

    if ((record_len != sizeof(record)) ||
        (btstack_property_bond_read_u32(record) != BTSTACK_BOND_MAGIC) ||
        (record[4] != BTSTACK_BOND_VERSION) ||
        (record[5] != (uint8_t)transport) ||
        (record[6] != id) ||
        !btstack_property_bond_address_type_valid(record[7]) ||
        !btstack_property_bond_address_valid(&record[8])) {
        return BTSTACK_STATUS_NOT_FOUND;
    }

    peer->transport = transport;
    peer->id = id;
    peer->address_type = record[7];
    memcpy(peer->address, &record[8], BTSTACK_BD_ADDR_LEN);
    return BTSTACK_STATUS_OK;
}

int btstack_property_bond_get_current_peer(
    btstack_transport_t transport,
    btstack_property_bond_peer_t *peer)
{
    return btstack_property_bond_get_peer(
        transport,
        btstack_property_bond_get_id(transport),
        peer);
}

int btstack_property_bond_clear_peer(
    btstack_transport_t transport,
    uint8_t id)
{
    const btstack_port_ops_storage_ops_t *storage;

    if (!btstack_property_bond_transport_valid(transport) ||
        !btstack_property_bond_id_valid(id)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    storage = btstack_property_bond_storage();
    if ((storage == 0) || (storage->delete_value == 0)) {
        return BTSTACK_STATUS_NOT_SUPPORTED;
    }

    return storage->delete_value(
        storage->context,
        btstack_property_bond_storage_key(transport),
        id);
}

int btstack_property_bond_clear_transport(
    btstack_transport_t transport)
{
    uint8_t id;
    const btstack_port_ops_storage_ops_t *storage;
    int status = BTSTACK_STATUS_OK;
    int delete_status;

    if (!btstack_property_bond_transport_valid(transport)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    storage = btstack_property_bond_storage();
    if ((storage == 0) || (storage->delete_value == 0)) {
        return BTSTACK_STATUS_NOT_SUPPORTED;
    }

    for (id = 0u; id < (uint8_t)BTSTACK_BOND_ID_COUNT; id++) {
        delete_status = storage->delete_value(
            storage->context,
            btstack_property_bond_storage_key(transport),
            id);
        if ((delete_status != BTSTACK_STATUS_OK) &&
            (delete_status != BTSTACK_STATUS_NOT_FOUND)) {
            status = delete_status;
        }
    }
    delete_status = storage->delete_value(
        storage->context,
        btstack_property_bond_storage_key(transport),
        BTSTACK_BOND_ACTIVE_TAG);
    if ((delete_status != BTSTACK_STATUS_OK) &&
        (delete_status != BTSTACK_STATUS_NOT_FOUND)) {
        status = delete_status;
    }
    if (status == BTSTACK_STATUS_OK) {
        *btstack_property_bond_id_ptr(transport) = 0u;
    }
    return status;
}

#else

int btstack_property_bond_init(void)
{
    return BTSTACK_STATUS_NOT_SUPPORTED;
}

void btstack_property_bond_reset(void)
{
}

int btstack_property_bond_select_id(
    btstack_transport_t transport,
    uint8_t id)
{
    (void)transport;
    (void)id;
    return BTSTACK_STATUS_NOT_SUPPORTED;
}

uint8_t btstack_property_bond_get_id(
    btstack_transport_t transport)
{
    (void)transport;
    return 0u;
}

int btstack_property_bond_put_peer(
    const btstack_property_bond_peer_t *peer)
{
    (void)peer;
    return BTSTACK_STATUS_NOT_SUPPORTED;
}

bool btstack_property_bond_has_peer(
    btstack_transport_t transport,
    uint8_t id)
{
    (void)transport;
    (void)id;
    return false;
}

int btstack_property_bond_get_peer(
    btstack_transport_t transport,
    uint8_t id,
    btstack_property_bond_peer_t *peer)
{
    (void)transport;
    (void)id;
    (void)peer;
    return BTSTACK_STATUS_NOT_SUPPORTED;
}

int btstack_property_bond_get_current_peer(
    btstack_transport_t transport,
    btstack_property_bond_peer_t *peer)
{
    (void)transport;
    (void)peer;
    return BTSTACK_STATUS_NOT_SUPPORTED;
}

int btstack_property_bond_clear_peer(
    btstack_transport_t transport,
    uint8_t id)
{
    (void)transport;
    (void)id;
    return BTSTACK_STATUS_NOT_SUPPORTED;
}

int btstack_property_bond_clear_transport(
    btstack_transport_t transport)
{
    (void)transport;
    return BTSTACK_STATUS_NOT_SUPPORTED;
}

#endif


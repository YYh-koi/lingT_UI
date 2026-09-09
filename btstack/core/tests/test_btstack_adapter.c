#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "adapter/btstack_adapter.h"
#include "gap/btstack_gap.h"

static btstack_gap_event_callback_t gap_callback;
static btstack_adapter_event_t last_adapter_event;
static unsigned adapter_event_count;
static unsigned property_init_count;
static unsigned event_init_count;
static unsigned gap_init_count;
static unsigned reset_count;
static unsigned operation_count;
static char last_text[32];
static uint32_t last_value;
static bool last_bool;
static btstack_bd_addr_t last_address;
static btstack_gap_state_t gap_state;
static btstack_gap_le_adv_config_t last_le_adv;
static btstack_gap_le_scan_config_t last_le_scan;

void btstack_profile_manager_le_connected(uint16_t handle, uint8_t role,
    uint8_t address_type, const btstack_bd_addr_t address)
{ (void)handle; (void)role; (void)address_type; (void)address; }
void btstack_profile_manager_le_disconnected(uint16_t handle) { (void)handle; }
void btstack_profile_manager_le_security_changed(bool encrypted, bool bonded)
{ (void)encrypted; (void)bonded; }

int btstack_property_init(void) { property_init_count++; return 0; }
void btstack_property_reset(void) { reset_count++; }
int btstack_event_init(void) { event_init_count++; return 0; }
void btstack_event_reset(void) { reset_count++; }
int btstack_gap_init(void) { gap_init_count++; return 0; }
void btstack_gap_reset(void) { reset_count++; }

int btstack_gap_set_event_callback(btstack_gap_event_callback_t callback)
{
    gap_callback = callback;
    return 0;
}

int btstack_gap_set_power(bool enabled)
{
    operation_count++;
    last_bool = enabled;
    return 0;
}

int btstack_gap_set_local_name(const char *name)
{
    operation_count++;
    (void)strncpy(last_text, name, sizeof(last_text) - 1u);
    return 0;
}

int btstack_gap_set_pin(const char *pin)
{
    operation_count++;
    (void)strncpy(last_text, pin, sizeof(last_text) - 1u);
    return 0;
}

int btstack_gap_set_ssp_mode(uint8_t mode)
{
    operation_count++;
    last_value = mode;
    return 0;
}

int btstack_gap_pairing_confirm(
    const btstack_bd_addr_t address, bool accept)
{
    operation_count++;
    memcpy(last_address, address, sizeof(last_address));
    last_bool = accept;
    return 0;
}

int btstack_gap_set_discoverable(bool enabled)
{
    operation_count++;
    last_bool = enabled;
    return 0;
}

int btstack_gap_set_connectable(bool enabled)
{
    operation_count++;
    last_bool = enabled;
    return 0;
}

int btstack_gap_set_class_of_device(uint32_t class_of_device)
{
    operation_count++;
    last_value = class_of_device;
    return 0;
}

int btstack_gap_classic_scan_start(uint8_t duration)
{
    operation_count++;
    last_value = duration;
    return 0;
}

int btstack_gap_classic_scan_stop(void) { operation_count++; return 0; }
int btstack_gap_disconnect(void) { operation_count++; return 0; }
bool btstack_gap_is_working(void) { return true; }

int btstack_gap_get_local_address(btstack_bd_addr_t address)
{
    memcpy(address, gap_state.local_address, sizeof(btstack_bd_addr_t));
    return 0;
}

int btstack_gap_get_state(btstack_gap_state_t *state)
{
    *state = gap_state;
    return 0;
}

int btstack_gap_le_adv_configure(const btstack_gap_le_adv_config_t *config)
{ last_le_adv = *config; operation_count++; return 0; }
int btstack_gap_le_adv_start(void) { operation_count++; return 0; }
int btstack_gap_le_adv_stop(void) { operation_count++; return 0; }
int btstack_gap_le_scan_start(const btstack_gap_le_scan_config_t *config)
{ last_le_scan = *config; operation_count++; return 0; }
int btstack_gap_le_scan_stop(void) { operation_count++; return 0; }
int btstack_gap_le_connect(uint8_t address_type, const btstack_bd_addr_t address)
{ last_value = address_type; memcpy(last_address, address, sizeof(last_address)); operation_count++; return 0; }
int btstack_gap_le_connect_cancel(void) { operation_count++; return 0; }
int btstack_gap_le_disconnect(void) { operation_count++; return 0; }
int btstack_gap_le_pair(void) { operation_count++; return 0; }
int btstack_gap_le_pairing_confirm(uint16_t connection_handle, bool accept)
{ last_value = connection_handle; last_bool = accept; operation_count++; return 0; }
int btstack_gap_le_passkey_reply(uint16_t connection_handle, uint32_t passkey)
{ (void)connection_handle; last_value = passkey; operation_count++; return 0; }
int btstack_gap_le_bond_delete(uint8_t index) { last_value = index; operation_count++; return 0; }
int btstack_gap_le_bond_delete_all(void) { operation_count++; return 0; }

static void adapter_event(const btstack_adapter_event_t *event)
{
    last_adapter_event = *event;
    adapter_event_count++;
}

static void test_setup_and_operations(void)
{
    btstack_adapter_state_t state;
    btstack_bd_addr_t address = { 1u, 2u, 3u, 4u, 5u, 6u };
    btstack_bd_addr_t result;
    unsigned before;

    memcpy(gap_state.local_address, address, sizeof(address));
    gap_state.class_of_device = 0x240404u;
    gap_state.ssp_mode = 2u;
    gap_state.stack_working = true;
    gap_state.connectable = true;

    assert(btstack_adapter_setup() == 0);
    assert(property_init_count == 1u);
    assert(event_init_count == 1u);
    assert(gap_init_count == 1u);
    assert(gap_callback != NULL);
    assert(strcmp(last_text, "0000") == 0);
    assert(last_value == 2u);
    assert(last_bool);

    before = operation_count;
    assert(btstack_adapter_set_power(true) == 0);
    assert(btstack_adapter_set_name("Unit") == 0);
    assert(strcmp(last_text, "Unit") == 0);
    assert(btstack_adapter_set_name_with_address_suffix("Unit") == 0);
    assert(strcmp(last_text, "Unit-0506") == 0);
    assert(btstack_adapter_set_pin("1234") == 0);
    assert(strcmp(last_text, "1234") == 0);
    assert(btstack_adapter_set_ssp_mode(1u) == 0);
    assert(last_value == 1u);
    assert(btstack_adapter_pairing_confirm(address, true) == 0);
    assert(memcmp(last_address, address, sizeof(address)) == 0);
    assert(btstack_adapter_set_discoverable(false) == 0);
    assert(!last_bool);
    assert(btstack_adapter_set_connectable(true) == 0);
    assert(last_bool);
    assert(btstack_adapter_set_class_of_device(0x123456u) == 0);
    assert(last_value == 0x123456u);
    assert(btstack_adapter_scan_start(7u) == 0);
    assert(last_value == 7u);
    assert(btstack_adapter_scan_stop() == 0);
    assert(btstack_adapter_disconnect() == 0);
    assert(operation_count == before + 12u);
    assert(btstack_adapter_is_working());
    assert(btstack_adapter_get_local_address(result) == 0);
    assert(memcmp(result, address, sizeof(result)) == 0);
    assert(btstack_adapter_get_state(&state) == 0);
    assert(state.class_of_device == 0x240404u);
    assert(state.ssp_mode == 2u);
    assert(state.stack_working);
    assert(state.connectable);
}

static void test_event_mapping(void)
{
    btstack_gap_event_t event;
    btstack_bd_addr_t address = { 6u, 5u, 4u, 3u, 2u, 1u };

    assert(btstack_adapter_set_event_callback(adapter_event) == 0);

    memset(&event, 0, sizeof(event));
    event.type = BTSTACK_GAP_EVENT_STACK_STATE;
    event.stack_working = 1u;
    gap_callback(&event);
    assert(last_adapter_event.type == BTSTACK_ADAPTER_EVENT_STACK_STATE);
    assert(last_adapter_event.data.stack_state.working);

    memset(&event, 0, sizeof(event));
    event.type = BTSTACK_GAP_EVENT_INQUIRY_RESULT;
    memcpy(event.address, address, sizeof(address));
    event.class_of_device = 0x112233u;
    event.rssi = -42;
    event.name_length = 4u;
    memcpy(event.name, "Test", 4u);
    gap_callback(&event);
    assert(last_adapter_event.type == BTSTACK_ADAPTER_EVENT_SCAN_RESULT);
    assert(memcmp(last_adapter_event.data.scan_result.address,
        address, sizeof(address)) == 0);
    assert(last_adapter_event.data.scan_result.class_of_device == 0x112233u);
    assert(last_adapter_event.data.scan_result.rssi == -42);
    assert(last_adapter_event.data.scan_result.name_length == 4u);
    assert(memcmp(last_adapter_event.data.scan_result.name, "Test", 4u) == 0);

    memset(&event, 0, sizeof(event));
    event.type = BTSTACK_GAP_EVENT_PAIRING_REQUEST;
    memcpy(event.address, address, sizeof(address));
    event.numeric_value = 123456u;
    gap_callback(&event);
    assert(last_adapter_event.type == BTSTACK_ADAPTER_EVENT_PAIRING_REQUEST);
    assert(last_adapter_event.data.pairing_request.numeric_value == 123456u);

    memset(&event, 0, sizeof(event));
    event.type = BTSTACK_GAP_EVENT_PAIRING_COMPLETE;
    memcpy(event.address, address, sizeof(address));
    event.status = 9;
    gap_callback(&event);
    assert(last_adapter_event.type == BTSTACK_ADAPTER_EVENT_PAIRING_COMPLETE);
    assert(last_adapter_event.data.pairing_complete.status == 9);

    memset(&event, 0, sizeof(event));
    event.type = BTSTACK_GAP_EVENT_LE_SCAN_RESULT;
    memcpy(event.address, address, sizeof(address));
    event.le.address_type = 1u;
    event.le.rssi = -30;
    event.le.data_len = 2u;
    event.le.data[0] = 0xaau;
    event.le.data[1] = 0x55u;
    gap_callback(&event);
    assert(last_adapter_event.type == BTSTACK_ADAPTER_EVENT_LE);
    assert(last_adapter_event.data.le.type == BTSTACK_ADAPTER_LE_EVENT_SCAN_RESULT);
    assert(last_adapter_event.data.le.address_type == 1u);
    assert(last_adapter_event.data.le.rssi == -30);
    assert(last_adapter_event.data.le.data_len == 2u);
    assert(last_adapter_event.data.le.data[1] == 0x55u);
    assert(adapter_event_count == 5u);
}

static void test_le_operations(void)
{
    btstack_adapter_le_adv_config_t adv;
    btstack_adapter_le_scan_config_t scan = { 0x20u, 0x10u, 1u, 1u };
    btstack_bd_addr_t address = { 1u, 2u, 3u, 4u, 5u, 6u };

    memset(&adv, 0, sizeof(adv));
    adv.interval_min = 0x20u;
    adv.interval_max = 0x30u;
    adv.data_len = 2u;
    adv.data[0] = 1u;
    adv.data[1] = 2u;
    assert(btstack_adapter_le_adv_configure(&adv) == 0);
    assert(last_le_adv.interval_max == 0x30u && last_le_adv.data[1] == 2u);
    assert(btstack_adapter_le_scan_start(&scan) == 0);
    assert(last_le_scan.window == 0x10u);
    assert(btstack_adapter_le_connect(1u, address) == 0);
    assert(last_value == 1u && memcmp(last_address, address, sizeof(address)) == 0);
    assert(btstack_adapter_le_pairing_confirm(0x40u, true) == 0);
    assert(last_value == 0x40u && last_bool);
    assert(btstack_adapter_le_passkey_reply(0x40u, 123456u) == 0);
    assert(last_value == 123456u);
    assert(btstack_adapter_le_bond_delete(2u) == 0);
    assert(last_value == 2u);
}

int main(void)
{
    test_setup_and_operations();
    test_event_mapping();
    test_le_operations();
    btstack_adapter_deinit();
    assert(reset_count == 3u);
    return 0;
}

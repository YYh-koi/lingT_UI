#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "gap/btstack_gap.h"
#include "btstack_gap_internal.h"
#include "bluetooth.h"
#include "btstack_defines.h"
#include "gap.h"

static btstack_gap_context_t context;
static btstack_gap_event_t last_event;
static unsigned event_count;
static unsigned adv_enable_count;
static int adv_enabled;
static unsigned scan_start_count;
static unsigned scan_stop_count;
static unsigned connect_count;
static unsigned cancel_count;
static unsigned disconnect_count;
static uint8_t adv_data[31];
static uint8_t adv_data_len;

btstack_gap_context_t *btstack_gap_internal_context(void) { return &context; }
int btstack_gap_internal_status(int status) { return status == 0 ? BTSTACK_STATUS_OK : BTSTACK_STATUS_ERROR; }
void btstack_gap_internal_emit(const btstack_gap_event_t *event) { last_event = *event; event_count++; }
void btstack_gap_internal_copy_address(btstack_bd_addr_t dst, const btstack_bd_addr_t src) { memcpy(dst, src, 6u); }
bool btstack_gap_internal_address_equal(const btstack_bd_addr_t a, const btstack_bd_addr_t b) { return memcmp(a, b, 6u) == 0; }

uint16_t little_endian_read_16(const uint8_t *buffer, int position) { return (uint16_t)(buffer[position] | ((uint16_t)buffer[position + 1] << 8)); }
uint32_t little_endian_read_32(const uint8_t *buffer, int position) { return (uint32_t)little_endian_read_16(buffer, position) | ((uint32_t)little_endian_read_16(buffer, position + 2) << 16); }
void reverse_bytes(const uint8_t *src, uint8_t *dst, int len) { int i; for (i = 0; i < len; i++) dst[i] = src[len - 1 - i]; }

void gap_random_address_set_mode(gap_random_address_type_t mode) { (void)mode; }
void gap_random_address_set_update_period(int period_ms) { (void)period_ms; }
void gap_advertisements_set_params(uint16_t min, uint16_t max, uint8_t type, uint8_t direct_type, bd_addr_t direct, uint8_t channels, uint8_t policy) { (void)min; (void)max; (void)type; (void)direct_type; (void)direct; (void)channels; (void)policy; }
void gap_advertisements_set_data(uint8_t len, uint8_t *data) { adv_data_len = len; memcpy(adv_data, data, len); }
void gap_scan_response_set_data(uint8_t len, uint8_t *data) { (void)len; (void)data; }
void gap_advertisements_enable(int enabled) { adv_enable_count++; adv_enabled = enabled; }
void gap_set_scan_params(uint8_t active, uint16_t interval, uint16_t window, uint8_t policy) { (void)active; (void)interval; (void)window; (void)policy; }
void gap_set_scan_duplicate_filter(bool enabled) { (void)enabled; }
void gap_set_scan_phys(uint8_t phys) { assert(phys == 1u); }
void gap_start_scan(void) { scan_start_count++; }
void gap_stop_scan(void) { scan_stop_count++; }
uint8_t gap_connect(const bd_addr_t address, bd_addr_type_t type) { (void)address; (void)type; connect_count++; return 0u; }
uint8_t gap_connect_cancel(void) { cancel_count++; return 0u; }
uint8_t gap_disconnect(hci_con_handle_t handle) { (void)handle; disconnect_count++; return 0u; }
void sm_init(void) { }
void sm_add_event_handler(btstack_packet_callback_registration_t *handler) { (void)handler; }
void sm_remove_event_handler(btstack_packet_callback_registration_t *handler) { (void)handler; }
void sm_set_io_capabilities(io_capability_t capability) { (void)capability; }
void sm_set_authentication_requirements(uint8_t requirements) { (void)requirements; }
void sm_request_pairing(hci_con_handle_t handle) { (void)handle; }
void sm_just_works_confirm(hci_con_handle_t handle) { (void)handle; }
void sm_numeric_comparison_confirm(hci_con_handle_t handle) { (void)handle; }
void sm_bonding_decline(hci_con_handle_t handle) { (void)handle; }
void sm_passkey_input(hci_con_handle_t handle, uint32_t passkey) { (void)handle; (void)passkey; }
void le_device_db_init(void) { }
int le_device_db_count(void) { return 0; }
int le_device_db_max_count(void) { return BTSTACK_LE_BOND_MAX; }
void le_device_db_remove(int index) { (void)index; }

static void set_working(bool working)
{
    context.initialized = true;
    context.state.stack_working = working;
}

static void test_advertising_and_scan(void)
{
    btstack_gap_le_adv_config_t adv;
    btstack_gap_le_scan_config_t scan = { 0x20u, 0x10u, 1u, 1u };
    unsigned i;

    memset(&adv, 0, sizeof(adv));
    adv.interval_min = 0x20u;
    adv.interval_max = 0x40u;
    adv.channel_map = 7u;
    adv.data_len = 31u;
    for (i = 0; i < 31u; i++) adv.data[i] = (uint8_t)i;
    assert(btstack_gap_le_adv_configure(&adv) == BTSTACK_STATUS_OK);
    assert(adv_data_len == 31u && memcmp(adv_data, adv.data, 31u) == 0);
    adv.data_len = 32u;
    assert(btstack_gap_le_adv_configure(&adv) == BTSTACK_STATUS_INVALID_ARGUMENT);
    set_working(false);
    assert(btstack_gap_le_adv_start() == BTSTACK_STATUS_NOT_READY);
    set_working(true);
    assert(btstack_gap_le_adv_start() == BTSTACK_STATUS_OK);
    assert(adv_enabled == 1);
    assert(btstack_gap_le_scan_start(&scan) == BTSTACK_STATUS_BUSY);
    assert(btstack_gap_le_adv_stop() == BTSTACK_STATUS_OK);
    assert(btstack_gap_le_scan_start(&scan) == BTSTACK_STATUS_OK);
    assert(scan_start_count == 1u);
    assert(btstack_gap_le_adv_start() == BTSTACK_STATUS_BUSY);
    assert(btstack_gap_le_scan_stop() == BTSTACK_STATUS_OK);
    assert(scan_stop_count == 1u);
}

static void test_report_and_connection(void)
{
    uint8_t report[15] = { GAP_EVENT_ADVERTISING_REPORT, 13u, 0u, BD_ADDR_TYPE_LE_PUBLIC, 6u, 5u, 4u, 3u, 2u, 1u, (uint8_t)-40, 3u, 1u, 2u, 3u };
    uint8_t connected[21] = { HCI_EVENT_LE_META, 19u, HCI_SUBEVENT_LE_CONNECTION_COMPLETE, 0u, 0x40u, 0u, HCI_ROLE_MASTER, BD_ADDR_TYPE_LE_PUBLIC, 6u, 5u, 4u, 3u, 2u, 1u, 0x18u, 0u, 0u, 0u, 0x48u, 0u, 0u };
    btstack_bd_addr_t peer = { 1u, 2u, 3u, 4u, 5u, 6u };

    assert(btstack_gap_le_scan_start(&(btstack_gap_le_scan_config_t){ 0x20u, 0x10u, 0u, 0u }) == BTSTACK_STATUS_OK);
    assert(btstack_gap_le_handle_hci_event(report, sizeof(report)) == BTSTACK_STATUS_OK);
    assert(last_event.type == BTSTACK_GAP_EVENT_LE_SCAN_RESULT);
    assert(last_event.le.data_len == 3u && memcmp(last_event.le.data, "\x01\x02\x03", 3u) == 0);
    report[12] = 9u;
    assert(last_event.le.data[0] == 1u);
    assert(btstack_gap_le_scan_stop() == BTSTACK_STATUS_OK);
    assert(btstack_gap_le_connect(BD_ADDR_TYPE_LE_PUBLIC, peer) == BTSTACK_STATUS_OK);
    assert(connect_count == 1u);
    assert(btstack_gap_le_connect(BD_ADDR_TYPE_LE_PUBLIC, peer) == BTSTACK_STATUS_BUSY);
    assert(btstack_gap_le_handle_hci_event(connected, sizeof(connected)) == BTSTACK_STATUS_OK);
    assert(last_event.type == BTSTACK_GAP_EVENT_LE_CONNECTED);
    assert(last_event.le.connection_handle == 0x0040u);
    assert(last_event.le.role == BTSTACK_GAP_LE_ROLE_CENTRAL);
    assert(btstack_gap_le_disconnect() == BTSTACK_STATUS_OK);
    assert(disconnect_count == 1u);
}

static void test_cancel(void)
{
    btstack_bd_addr_t peer = { 6u, 5u, 4u, 3u, 2u, 1u };

    btstack_gap_le_reset();
    set_working(true);
    assert(btstack_gap_le_connect(BD_ADDR_TYPE_LE_PUBLIC, peer) == BTSTACK_STATUS_OK);
    assert(btstack_gap_le_connect_cancel() == BTSTACK_STATUS_OK);
    assert(cancel_count == 1u);
}

int main(void)
{
    memset(&context, 0, sizeof(context));
    assert(btstack_gap_le_init() == BTSTACK_STATUS_OK);
    test_advertising_and_scan();
    test_report_and_connection();
    test_cancel();
    assert(adv_enable_count >= 2u);
    assert(event_count >= 2u);
    return 0;
}

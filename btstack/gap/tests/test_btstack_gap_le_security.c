#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "gap/btstack_gap.h"
#include "btstack_gap_internal.h"
#include "ble/le_device_db.h"
#include "ble/sm.h"

static btstack_gap_context_t context;
static unsigned sm_init_count;
static unsigned sm_handler_count;
static unsigned sm_remove_count;
static unsigned request_pair_count;
static unsigned confirm_count;
static unsigned decline_count;
static unsigned passkey_count;
static hci_con_handle_t last_handle;
static uint32_t last_passkey;
static io_capability_t io_capability;
static uint8_t auth_req;
static gap_random_address_type_t privacy_mode;
static int bond_count;
static bool removed[BTSTACK_LE_BOND_MAX];

btstack_gap_context_t *btstack_gap_internal_context(void) { return &context; }
int btstack_gap_internal_status(int status) { return status == 0 ? BTSTACK_STATUS_OK : BTSTACK_STATUS_ERROR; }
void btstack_gap_internal_emit(const btstack_gap_event_t *event) { (void)event; }
void btstack_gap_internal_copy_address(btstack_bd_addr_t dst, const btstack_bd_addr_t src) { memcpy(dst, src, 6u); }
bool btstack_gap_internal_address_equal(const btstack_bd_addr_t a, const btstack_bd_addr_t b) { return memcmp(a, b, 6u) == 0; }

void sm_init(void) { sm_init_count++; }
void sm_add_event_handler(btstack_packet_callback_registration_t *handler) { assert(handler != NULL && handler->callback != NULL); sm_handler_count++; }
void sm_remove_event_handler(btstack_packet_callback_registration_t *handler) { assert(handler != NULL); sm_remove_count++; }
void sm_set_io_capabilities(io_capability_t capability) { io_capability = capability; }
void sm_set_authentication_requirements(uint8_t requirements) { auth_req = requirements; }
void sm_request_pairing(hci_con_handle_t handle) { request_pair_count++; last_handle = handle; }
void sm_just_works_confirm(hci_con_handle_t handle) { confirm_count++; last_handle = handle; }
void sm_numeric_comparison_confirm(hci_con_handle_t handle) { confirm_count++; last_handle = handle; }
void sm_bonding_decline(hci_con_handle_t handle) { decline_count++; last_handle = handle; }
void sm_passkey_input(hci_con_handle_t handle, uint32_t passkey) { passkey_count++; last_handle = handle; last_passkey = passkey; }
void gap_random_address_set_mode(gap_random_address_type_t mode) { privacy_mode = mode; }
void gap_random_address_set_update_period(int period_ms) { assert(period_ms > 0); }
void gap_advertisements_set_params(uint16_t a, uint16_t b, uint8_t c, uint8_t d, bd_addr_t e, uint8_t f, uint8_t g) { (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; }
void gap_advertisements_set_data(uint8_t len, uint8_t *data) { (void)len; (void)data; }
void gap_scan_response_set_data(uint8_t len, uint8_t *data) { (void)len; (void)data; }
void gap_advertisements_enable(int enabled) { (void)enabled; }
void gap_set_scan_params(uint8_t active, uint16_t interval, uint16_t window, uint8_t policy) { (void)active; (void)interval; (void)window; (void)policy; }
void gap_set_scan_duplicate_filter(bool enabled) { (void)enabled; }
void gap_set_scan_phys(uint8_t phys) { (void)phys; }
void gap_start_scan(void) { }
void gap_stop_scan(void) { }
uint8_t gap_connect(const bd_addr_t address, bd_addr_type_t type) { (void)address; (void)type; return 0u; }
uint8_t gap_connect_cancel(void) { return 0u; }
uint8_t gap_disconnect(hci_con_handle_t handle) { (void)handle; return 0u; }

uint16_t little_endian_read_16(const uint8_t *buffer, int position) { return (uint16_t)(buffer[position] | ((uint16_t)buffer[position + 1] << 8)); }
uint32_t little_endian_read_32(const uint8_t *buffer, int position) { return (uint32_t)little_endian_read_16(buffer, position) | ((uint32_t)little_endian_read_16(buffer, position + 2) << 16); }
void reverse_bytes(const uint8_t *src, uint8_t *dst, int len) { int i; for (i = 0; i < len; i++) dst[i] = src[len - 1 - i]; }

void le_device_db_init(void) { bond_count = 0; memset(removed, 0, sizeof(removed)); }
int le_device_db_count(void) { return bond_count; }
int le_device_db_max_count(void) { return BTSTACK_LE_BOND_MAX; }
void le_device_db_remove(int index) { assert(index >= 0 && index < (int)BTSTACK_LE_BOND_MAX); if (!removed[index]) { removed[index] = true; if (bond_count > 0) bond_count--; } }

static void test_init_and_pair(void)
{
    assert(btstack_gap_le_security_init() == BTSTACK_STATUS_OK);
    assert(btstack_gap_le_security_init() == BTSTACK_STATUS_OK);
    assert(sm_init_count == 1u && sm_handler_count == 1u);
    assert(io_capability == IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    assert((auth_req & SM_AUTHREQ_BONDING) != 0u);
    assert(privacy_mode == GAP_RANDOM_ADDRESS_RESOLVABLE);
    assert(btstack_gap_le_pair() == BTSTACK_STATUS_NOT_READY);
    context.state.le_connected = true;
    context.state.le_connection_handle = 0x0040u;
    assert(btstack_gap_le_pair() == BTSTACK_STATUS_OK);
    assert(request_pair_count == 1u && last_handle == 0x0040u);
    assert(btstack_gap_le_pairing_confirm(0x0041u, true) == BTSTACK_STATUS_NOT_FOUND);
    assert(btstack_gap_le_pairing_confirm(0x0040u, true) == BTSTACK_STATUS_OK);
    assert(confirm_count == 1u);
    assert(btstack_gap_le_pairing_confirm(0x0040u, false) == BTSTACK_STATUS_OK);
    assert(decline_count == 1u);
    assert(btstack_gap_le_passkey_reply(0x0040u, 1000000u) == BTSTACK_STATUS_INVALID_ARGUMENT);
    assert(btstack_gap_le_passkey_reply(0x0040u, 123456u) == BTSTACK_STATUS_OK);
    assert(passkey_count == 1u && last_passkey == 123456u);
}

static void test_ram_bonds(void)
{
    bond_count = BTSTACK_LE_BOND_MAX;
    assert(btstack_gap_le_bond_count() == BTSTACK_LE_BOND_MAX);
    assert(btstack_gap_le_bond_delete(BTSTACK_LE_BOND_MAX) == BTSTACK_STATUS_NOT_FOUND);
    assert(btstack_gap_le_bond_delete(1u) == BTSTACK_STATUS_OK);
    assert(btstack_gap_le_bond_count() == BTSTACK_LE_BOND_MAX - 1u);
    assert(btstack_gap_le_bond_delete_all() == BTSTACK_STATUS_OK);
    assert(btstack_gap_le_bond_count() == 0u);
    btstack_gap_le_security_reset();
    assert(sm_remove_count == 1u);
    assert(btstack_gap_le_security_init() == BTSTACK_STATUS_OK);
    assert(sm_init_count == 2u);
}

int main(void)
{
    memset(&context, 0, sizeof(context));
    test_init_and_pair();
    test_ram_bonds();
    return 0;
}

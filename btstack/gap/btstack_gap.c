#include "gap/btstack_gap.h"

#include <string.h>

#include "btstack_defines.h"
#include "btstack_event.h"
#include "gap.h"
#include "hci.h"
#include "btstack_gap_internal.h"

static btstack_gap_context_t btstack_gap_context;
static btstack_gap_event_callback_t btstack_gap_event_callback;
static btstack_packet_callback_registration_t btstack_gap_hci_callback_registration;
static bool btstack_gap_hci_callback_registered;

btstack_gap_context_t *btstack_gap_internal_context(void)
{
    return &btstack_gap_context;
}

void btstack_gap_internal_copy_address(btstack_bd_addr_t destination, const btstack_bd_addr_t source)
{
    (void)memcpy(destination, source, BTSTACK_BD_ADDR_LEN);
}

bool btstack_gap_internal_address_equal(const btstack_bd_addr_t left, const btstack_bd_addr_t right)
{
    return memcmp(left, right, BTSTACK_BD_ADDR_LEN) == 0;
}

int btstack_gap_internal_status(int native_status)
{
    if (native_status == ERROR_CODE_SUCCESS) {
        return BTSTACK_STATUS_OK;
    }
    if (native_status == ERROR_CODE_INVALID_HCI_COMMAND_PARAMETERS) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (native_status == ERROR_CODE_COMMAND_DISALLOWED) {
        return BTSTACK_STATUS_BUSY;
    }
    return BTSTACK_STATUS_ERROR;
}

void btstack_gap_internal_emit(const btstack_gap_event_t *event)
{
    if ((event != NULL) && (btstack_gap_event_callback != NULL)) {
        btstack_gap_event_callback(event);
    }
}

static void btstack_gap_emit_stack_state(bool working)
{
    btstack_gap_event_t event;

    (void)memset(&event, 0, sizeof(event));
    event.type = BTSTACK_GAP_EVENT_STACK_STATE;
    event.stack_working = working ? 1u : 0u;
    btstack_gap_internal_emit(&event);
}

static void btstack_gap_emit_local_address(void)
{
    btstack_gap_event_t event;

    (void)memset(&event, 0, sizeof(event));
    event.type = BTSTACK_GAP_EVENT_LOCAL_ADDRESS;
    btstack_gap_internal_copy_address(event.address, btstack_gap_context.state.local_address);
    btstack_gap_internal_emit(&event);
}

static void btstack_gap_handle_stack_state(const uint8_t *packet)
{
    bool working = btstack_event_state_get_state(packet) == HCI_STATE_WORKING;

    btstack_gap_context.state.stack_working = working;
    if (working) {
        gap_local_bd_addr(btstack_gap_context.state.local_address);
        (void)btstack_gap_classic_apply_cached();
        btstack_gap_emit_local_address();
    } else {
        btstack_gap_classic_stack_stopped();
#if BTSTACK_LE
        btstack_gap_le_stack_stopped();
#endif
    }
    btstack_gap_emit_stack_state(working);
}

static void btstack_gap_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    (void)channel;
    if ((packet_type != HCI_EVENT_PACKET) || (packet == NULL) || (size < 2u)) {
        return;
    }
    if (hci_event_packet_get_type(packet) == BTSTACK_EVENT_STATE) {
        btstack_gap_handle_stack_state(packet);
        return;
    }
#if BTSTACK_LE
    (void)btstack_gap_le_handle_hci_event(packet, size);
#endif
    (void)btstack_gap_classic_handle_hci_event(packet, size);
}

int btstack_gap_init(void)
{
    int status;

    if (btstack_gap_context.initialized) {
        return BTSTACK_STATUS_OK;
    }
    (void)memset(&btstack_gap_context, 0, sizeof(btstack_gap_context));
    btstack_gap_context.acl_handle = HCI_CON_HANDLE_INVALID;
    status = btstack_gap_classic_init();
    if (status != BTSTACK_STATUS_OK) {
        return status;
    }
#if BTSTACK_LE
    status = btstack_gap_le_init();
    if (status != BTSTACK_STATUS_OK) {
        return status;
    }
    status = btstack_gap_le_security_init();
    if (status != BTSTACK_STATUS_OK) {
        return status;
    }
#endif
    if (!btstack_gap_hci_callback_registered) {
        btstack_gap_hci_callback_registration.callback = btstack_gap_packet_handler;
        hci_add_event_handler(&btstack_gap_hci_callback_registration);
        btstack_gap_hci_callback_registered = true;
    }
    btstack_gap_context.initialized = true;
    return BTSTACK_STATUS_OK;
}

void btstack_gap_reset(void)
{
    if (btstack_gap_hci_callback_registered) {
        hci_remove_event_handler(&btstack_gap_hci_callback_registration);
        btstack_gap_hci_callback_registered = false;
    }
    btstack_gap_classic_reset();
#if BTSTACK_LE
    btstack_gap_le_security_reset();
    btstack_gap_le_reset();
#endif
    (void)memset(&btstack_gap_context, 0, sizeof(btstack_gap_context));
    btstack_gap_context.acl_handle = HCI_CON_HANDLE_INVALID;
    btstack_gap_event_callback = NULL;
}

int btstack_gap_set_event_callback(btstack_gap_event_callback_t callback)
{
    if (!btstack_gap_context.initialized) {
        return BTSTACK_STATUS_NOT_READY;
    }
    btstack_gap_event_callback = callback;
    return BTSTACK_STATUS_OK;
}

int btstack_gap_set_power(bool enabled)
{
    if (!btstack_gap_context.initialized) {
        return BTSTACK_STATUS_NOT_READY;
    }
    return btstack_gap_internal_status(hci_power_control(enabled ? HCI_POWER_ON : HCI_POWER_OFF));
}

bool btstack_gap_is_working(void)
{
    return btstack_gap_context.state.stack_working;
}

int btstack_gap_get_local_address(btstack_bd_addr_t address)
{
    if (address == NULL) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (!btstack_gap_context.initialized) {
        return BTSTACK_STATUS_NOT_READY;
    }
    btstack_gap_internal_copy_address(address, btstack_gap_context.state.local_address);
    return BTSTACK_STATUS_OK;
}

int btstack_gap_get_state(btstack_gap_state_t *state)
{
    if (state == NULL) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (!btstack_gap_context.initialized) {
        return BTSTACK_STATUS_NOT_READY;
    }
    *state = btstack_gap_context.state;
    return BTSTACK_STATUS_OK;
}

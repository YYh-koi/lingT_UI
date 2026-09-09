#ifndef BTSTACK_GAP_INTERNAL_H
#define BTSTACK_GAP_INTERNAL_H

#include "gap/btstack_gap.h"
#include "hci.h"

typedef struct {
    btstack_gap_state_t state;
    hci_con_handle_t acl_handle;
    btstack_bd_addr_t pending_pair_address;
    bool pending_pair_valid;
    bool initialized;
} btstack_gap_context_t;

btstack_gap_context_t *btstack_gap_internal_context(void);
int btstack_gap_internal_status(int native_status);
void btstack_gap_internal_emit(const btstack_gap_event_t *event);
bool btstack_gap_internal_address_equal(
    const btstack_bd_addr_t left,
    const btstack_bd_addr_t right);
void btstack_gap_internal_copy_address(
    btstack_bd_addr_t destination,
    const btstack_bd_addr_t source);

int btstack_gap_classic_init(void);
void btstack_gap_classic_reset(void);
int btstack_gap_classic_apply_cached(void);
void btstack_gap_classic_stack_stopped(void);
int btstack_gap_classic_handle_hci_event(const uint8_t *packet, uint16_t size);
void btstack_gap_le_stack_stopped(void);
int btstack_gap_le_handle_hci_event(const uint8_t *packet, uint16_t size);
void btstack_gap_le_security_reset(void);

#endif

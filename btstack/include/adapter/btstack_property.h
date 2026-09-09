#ifndef BTSTACK_PROPERTY_H
#define BTSTACK_PROPERTY_H

#include <stdbool.h>
#include <stdint.h>

#include "adapter/btstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

int btstack_property_init(void);
void btstack_property_reset(void);

bool btstack_property_get_local_address(
    uint8_t address[BTSTACK_BD_ADDR_LEN],
    btstack_address_type_t *address_type);
bool btstack_property_get_classic_address(
    uint8_t address[BTSTACK_BD_ADDR_LEN]);
bool btstack_property_get_ble_address(
    uint8_t address[BTSTACK_BD_ADDR_LEN],
    btstack_address_type_t *address_type);

int btstack_property_set_local_address(
    const uint8_t address[BTSTACK_BD_ADDR_LEN],
    bool store);
int btstack_property_set_classic_address(
    const uint8_t address[BTSTACK_BD_ADDR_LEN],
    bool store);
int btstack_property_set_ble_address(
    const uint8_t address[BTSTACK_BD_ADDR_LEN],
    btstack_address_type_t address_type,
    bool store);

const char *btstack_property_get_local_name(void);
const char *btstack_property_get_classic_name(void);
const char *btstack_property_get_ble_name(void);

int btstack_property_set_local_name(
    const char *name,
    bool store);
int btstack_property_set_classic_name(
    const char *name,
    bool store);
int btstack_property_set_ble_name(
    const char *name,
    bool store);

int btstack_property_apply_controller_address(void);

int btstack_property_bond_init(void);
void btstack_property_bond_reset(void);
int btstack_property_bond_select_id(
    btstack_transport_t transport,
    uint8_t id);
uint8_t btstack_property_bond_get_id(
    btstack_transport_t transport);
int btstack_property_bond_put_peer(
    const btstack_property_bond_peer_t *peer);
bool btstack_property_bond_has_peer(
    btstack_transport_t transport,
    uint8_t id);
int btstack_property_bond_get_peer(
    btstack_transport_t transport,
    uint8_t id,
    btstack_property_bond_peer_t *peer);
int btstack_property_bond_get_current_peer(
    btstack_transport_t transport,
    btstack_property_bond_peer_t *peer);
int btstack_property_bond_clear_peer(
    btstack_transport_t transport,
    uint8_t id);
int btstack_property_bond_clear_transport(
    btstack_transport_t transport);

#ifdef __cplusplus
}
#endif

#endif

#ifndef BTSTACK_HID_DEVICE_H
#define BTSTACK_HID_DEVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "adapter/btstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t sub_class;
    uint8_t country_code;
    bool virtual_cable;
    bool reconnect_initiate;
    bool sdp_record_disabled;
    bool battery_power;
    bool remote_wake;
    bool normally_connectable;
    bool boot_device;
    uint16_t version;
    uint16_t parser_version;
    uint16_t profile_version;
    uint16_t supervision_timeout;
    uint16_t language;
    uint16_t string_offset;
    const char *service_name;
    const char *service_description;
    const char *provider_name;
} btstack_hid_device_sdp_config_t;

typedef struct {
    const uint8_t *descriptor;
    uint16_t descriptor_len;
    const uint8_t *pnp_id;
    uint16_t pnp_id_len;
    const btstack_hid_device_sdp_config_t *sdp;
} btstack_hid_device_config_t;

/*
 * setup() and setters copy descriptor, PNP ID, and SDP string data into
 * bounded adapter-owned storage before returning.
 * Link operations remain NOT_SUPPORTED until a target profile backend binds
 * these APIs to the selected BTstack Classic HID implementation.
 */
int btstack_hid_device_setup(
    const btstack_hid_device_config_t *config);
bool btstack_hid_device_is_connected(void);
int btstack_hid_device_disconnect(void);
int btstack_hid_device_reconnect(
    const btstack_property_bond_peer_t *peer);

int btstack_hid_device_set_descriptor(
    const uint8_t *descriptor,
    uint16_t descriptor_len);
int btstack_hid_device_set_pnp_id(
    const uint8_t *pnp_id,
    uint16_t pnp_id_len);
int btstack_hid_device_set_sdp_config(
    const btstack_hid_device_sdp_config_t *config);

int btstack_hid_device_send_report(
    uint8_t report_id,
    const uint8_t *report,
    uint16_t report_len);
int btstack_hid_device_send_raw_report(
    uint8_t report_type,
    uint8_t report_id,
    const uint8_t *report,
    uint16_t report_len);

#ifdef __cplusplus
}
#endif

#endif

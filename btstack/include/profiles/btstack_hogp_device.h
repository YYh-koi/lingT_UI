#ifndef BTSTACK_HOGP_DEVICE_H
#define BTSTACK_HOGP_DEVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "adapter/btstack_types.h"
#include "profiles/btstack_gatt_server.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t report_id;
    uint8_t report_type;
} btstack_hogp_report_info_t;

typedef struct {
    const uint8_t *report_map;
    uint16_t report_map_len;
    const uint8_t *pnp_id;
    uint16_t pnp_id_len;
    const btstack_hogp_report_info_t *reports;
    uint8_t report_count;
} btstack_hogp_device_config_t;

int btstack_hogp_device_init(void);
int btstack_hogp_device_setup(const btstack_hogp_device_config_t *config);
void btstack_hogp_device_handle_server_event(const btstack_gatt_server_event_t *event);
void btstack_hogp_device_connection_changed(uint16_t connection_handle, bool connected, bool encrypted);
void btstack_hogp_device_security_changed(bool encrypted);
bool btstack_hogp_device_is_connected(void);
bool btstack_hogp_device_is_ready(void);
uint16_t btstack_hogp_device_get_connection_handle(void);
int btstack_hogp_device_disconnect(void);

int btstack_hogp_device_set_report_map(const uint8_t *report_map, uint16_t report_map_len);
int btstack_hogp_device_set_pnp_id(const uint8_t *pnp_id, uint16_t pnp_id_len);
int btstack_hogp_device_set_report_info(const btstack_hogp_report_info_t *reports, uint8_t report_count);

int btstack_hogp_device_send_report(uint8_t report_id, const uint8_t *report, uint16_t report_len);
int btstack_hogp_device_send_raw_report(uint8_t report_type, uint8_t report_id,
    const uint8_t *report, uint16_t report_len);

#ifdef __cplusplus
}
#endif

#endif

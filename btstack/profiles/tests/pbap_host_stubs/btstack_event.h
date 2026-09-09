#ifndef BTSTACK_EVENT_H
#define BTSTACK_EVENT_H
#include <stdint.h>
#include "btstack_defines.h"
static inline uint16_t pbap_stub_le16(const uint8_t *p){ return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static inline uint8_t hci_event_packet_get_type(const uint8_t *e){ return e[0]; }
static inline uint8_t hci_event_pbap_meta_get_subevent_code(const uint8_t *e){ return e[2]; }
static inline uint16_t pbap_subevent_connection_opened_get_pbap_cid(const uint8_t *e){ return pbap_stub_le16(e + 3); }
static inline uint8_t pbap_subevent_connection_opened_get_status(const uint8_t *e){ return e[5]; }
static inline void pbap_subevent_connection_opened_get_bd_addr(const uint8_t *e, bd_addr_t a){ unsigned i; for(i=0;i<6;i++) a[i]=e[11-i]; }
static inline uint16_t pbap_subevent_connection_closed_get_goep_cid(const uint8_t *e){ return pbap_stub_le16(e + 3); }
static inline uint16_t pbap_subevent_operation_completed_get_goep_cid(const uint8_t *e){ return pbap_stub_le16(e + 3); }
static inline uint8_t pbap_subevent_operation_completed_get_status(const uint8_t *e){ return e[5]; }
static inline uint16_t pbap_subevent_authentication_request_get_goep_cid(const uint8_t *e){ return pbap_stub_le16(e + 3); }
#endif

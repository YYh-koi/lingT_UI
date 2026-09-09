#ifndef PBAP_CLIENT_H
#define PBAP_CLIENT_H
#include <stdint.h>
#include "btstack_defines.h"
#ifndef ENABLE_CLASSIC
#error "native btstack_config.h must be included before generic adapter headers"
#endif
void pbap_client_init(void);
uint8_t pbap_connect(btstack_packet_handler_t handler, bd_addr_t addr, uint16_t *out_cid);
uint8_t pbap_authentication_password(uint16_t cid, const char *password);
uint8_t pbap_disconnect(uint16_t cid);
uint8_t pbap_pull_phonebook(uint16_t cid, const char *path);
uint8_t pbap_abort(uint16_t cid);
uint8_t pbap_set_flow_control_mode(uint16_t cid, int enable);
uint8_t pbap_next_packet(uint16_t cid);
#endif

#ifndef BTSTACK_AT_DISPATCH_TABLE_H
#define BTSTACK_AT_DISPATCH_TABLE_H

#include "btstack_at_internal.h"

typedef enum {
    BTSTACK_AT_DOMAIN_GENERAL = 0,
    BTSTACK_AT_DOMAIN_HFP,
    BTSTACK_AT_DOMAIN_A2DP_AVRCP
} btstack_at_command_domain_t;

int btstack_at_command_get_domain(
    btstack_at_command_id_t id,
    btstack_at_command_domain_t *domain);

#endif

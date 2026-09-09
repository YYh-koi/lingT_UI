#ifndef BTSTACK_AT_COMMAND_TABLE_H
#define BTSTACK_AT_COMMAND_TABLE_H

#include "btstack_at_internal.h"

#define BTSTACK_AT_FORM_MASK_EXECUTE \
    (1u << BTSTACK_AT_FORM_EXECUTE)
#define BTSTACK_AT_FORM_MASK_QUERY \
    (1u << BTSTACK_AT_FORM_QUERY)
#define BTSTACK_AT_FORM_MASK_SET \
    (1u << BTSTACK_AT_FORM_SET)

typedef enum {
    BTSTACK_AT_VALIDATE_NONE = 0,
    BTSTACK_AT_VALIDATE_UINT,
    BTSTACK_AT_VALIDATE_TEXT,
    BTSTACK_AT_VALIDATE_ADDRESS,
    BTSTACK_AT_VALIDATE_UINT_LIST,
    BTSTACK_AT_VALIDATE_DOMAIN_SPECIFIC
} btstack_at_validation_kind_t;

typedef struct {
    const char *name;
    uint8_t name_length;
    btstack_at_command_id_t id;
    uint8_t allowed_forms;
    btstack_at_validation_kind_t validation;
} btstack_at_command_descriptor_t;

const btstack_at_command_descriptor_t *btstack_at_command_find(
    const char *name,
    uint16_t name_length);

#endif

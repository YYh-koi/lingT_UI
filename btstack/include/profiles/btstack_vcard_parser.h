#ifndef BTSTACK_VCARD_PARSER_H
#define BTSTACK_VCARD_PARSER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BTSTACK_VCARD_NAME_MAX          48u
#define BTSTACK_VCARD_NUMBER_MAX        32u
#define BTSTACK_VCARD_NUMBER_COUNT       4u
#define BTSTACK_VCARD_TIMESTAMP_MAX     24u
#define BTSTACK_VCARD_LOGICAL_LINE_MAX 256u

#define BTSTACK_VCARD_TEL_HOME  (1u << 0)
#define BTSTACK_VCARD_TEL_WORK  (1u << 1)
#define BTSTACK_VCARD_TEL_CELL  (1u << 2)
#define BTSTACK_VCARD_TEL_FAX   (1u << 3)
#define BTSTACK_VCARD_TEL_VOICE (1u << 4)
#define BTSTACK_VCARD_TEL_OTHER (1u << 5)

typedef enum {
    BTSTACK_VCARD_DIRECTION_NONE = 0,
    BTSTACK_VCARD_DIRECTION_INCOMING,
    BTSTACK_VCARD_DIRECTION_OUTGOING,
    BTSTACK_VCARD_DIRECTION_MISSED
} btstack_vcard_direction_t;

typedef enum {
    BTSTACK_VCARD_PARSER_OK = 0,
    BTSTACK_VCARD_PARSER_PAUSED,
    BTSTACK_VCARD_PARSER_ERROR
} btstack_vcard_parser_result_t;

typedef struct {
    char value[BTSTACK_VCARD_NUMBER_MAX + 1u];
    uint8_t type_flags;
} btstack_vcard_number_t;

typedef struct {
    char name[BTSTACK_VCARD_NAME_MAX + 1u];
    btstack_vcard_number_t numbers[BTSTACK_VCARD_NUMBER_COUNT];
    uint8_t number_count;
    char call_timestamp[BTSTACK_VCARD_TIMESTAMP_MAX + 1u];
    btstack_vcard_direction_t direction;
    uint8_t truncated;
} btstack_vcard_record_t;

typedef uint8_t (*btstack_vcard_record_callback_t)(
    void *context,
    const btstack_vcard_record_t *record);

typedef struct {
    btstack_vcard_record_callback_t record_callback;
    void *callback_context;
    btstack_vcard_record_t record;
    char n_fallback[BTSTACK_VCARD_NAME_MAX + 1u];
    char logical_line[BTSTACK_VCARD_LOGICAL_LINE_MAX + 1u];
    uint32_t parser_error_count;
    uint16_t logical_line_length;
    uint8_t in_record;
    uint8_t record_invalid;
    uint8_t have_fn;
    uint8_t line_pending;
    uint8_t line_overflow;
    uint8_t saw_cr;
    uint8_t in_callback;
    uint8_t physical_last_byte;
    uint8_t physical_has_byte;
    uint8_t pending_qp_soft_break;
} btstack_vcard_parser_t;

void btstack_vcard_parser_init(
    btstack_vcard_parser_t *parser,
    btstack_vcard_record_callback_t record_callback,
    void *context);

btstack_vcard_parser_result_t btstack_vcard_parser_process(
    btstack_vcard_parser_t *parser,
    const uint8_t *bytes,
    size_t length,
    size_t *consumed_out);

btstack_vcard_parser_result_t btstack_vcard_parser_finish(
    btstack_vcard_parser_t *parser);

uint32_t btstack_vcard_parser_error_count(
    const btstack_vcard_parser_t *parser);

#ifdef __cplusplus
}
#endif

#endif

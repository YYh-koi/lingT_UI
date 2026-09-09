#ifndef BT_XSTACK_PHONEBOOK_H
#define BT_XSTACK_PHONEBOOK_H

#include "bt_xstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BT_XSTACK_PHONEBOOK_NAME_MAX       48u
#define BT_XSTACK_PHONEBOOK_NUMBER_MAX     32u
#define BT_XSTACK_PHONEBOOK_NUMBER_COUNT    4u
#define BT_XSTACK_PHONEBOOK_TIMESTAMP_MAX  24u
#define BT_XSTACK_PHONEBOOK_PASSWORD_MAX    32u

#define BT_XSTACK_PHONEBOOK_TEL_HOME  (1u << 0)
#define BT_XSTACK_PHONEBOOK_TEL_WORK  (1u << 1)
#define BT_XSTACK_PHONEBOOK_TEL_CELL  (1u << 2)
#define BT_XSTACK_PHONEBOOK_TEL_FAX   (1u << 3)
#define BT_XSTACK_PHONEBOOK_TEL_VOICE (1u << 4)
#define BT_XSTACK_PHONEBOOK_TEL_OTHER (1u << 5)

typedef enum {
    BT_XSTACK_PHONEBOOK_STATE_IDLE = 0,
    BT_XSTACK_PHONEBOOK_STATE_CONNECTING,
    BT_XSTACK_PHONEBOOK_STATE_AUTHENTICATING,
    BT_XSTACK_PHONEBOOK_STATE_PULLING,
    BT_XSTACK_PHONEBOOK_STATE_PAUSED,
    BT_XSTACK_PHONEBOOK_STATE_DISCONNECTING
} bt_xstack_phonebook_state_t;

typedef uint8_t bt_xstack_phonebook_category_mask_t;

enum {
    BT_XSTACK_PHONEBOOK_CONTACTS = (1u << 0),
    BT_XSTACK_PHONEBOOK_INCOMING = (1u << 1),
    BT_XSTACK_PHONEBOOK_OUTGOING = (1u << 2),
    BT_XSTACK_PHONEBOOK_MISSED = (1u << 3),
    BT_XSTACK_PHONEBOOK_ALL =
        BT_XSTACK_PHONEBOOK_CONTACTS |
        BT_XSTACK_PHONEBOOK_INCOMING |
        BT_XSTACK_PHONEBOOK_OUTGOING |
        BT_XSTACK_PHONEBOOK_MISSED
};

typedef enum {
    BT_XSTACK_PHONEBOOK_CATEGORY_CONTACTS = 0,
    BT_XSTACK_PHONEBOOK_CATEGORY_INCOMING,
    BT_XSTACK_PHONEBOOK_CATEGORY_OUTGOING,
    BT_XSTACK_PHONEBOOK_CATEGORY_MISSED
} bt_xstack_phonebook_category_t;

typedef enum {
    BT_XSTACK_PHONEBOOK_EVENT_STARTED = 0,
    BT_XSTACK_PHONEBOOK_EVENT_CONTACT,
    BT_XSTACK_PHONEBOOK_EVENT_CALL,
    BT_XSTACK_PHONEBOOK_EVENT_CATEGORY_COMPLETED,
    BT_XSTACK_PHONEBOOK_EVENT_COMPLETED,
    BT_XSTACK_PHONEBOOK_EVENT_AUTH_REQUIRED,
    BT_XSTACK_PHONEBOOK_EVENT_ERROR
} bt_xstack_phonebook_event_id_t;

typedef enum {
    BT_XSTACK_PHONEBOOK_STATUS_SUCCESS = 0,
    BT_XSTACK_PHONEBOOK_STATUS_PARTIAL,
    BT_XSTACK_PHONEBOOK_STATUS_UNSUPPORTED,
    BT_XSTACK_PHONEBOOK_STATUS_DENIED,
    BT_XSTACK_PHONEBOOK_STATUS_CONNECTION_FAILED,
    BT_XSTACK_PHONEBOOK_STATUS_PROTOCOL_ERROR,
    BT_XSTACK_PHONEBOOK_STATUS_CANCELED,
    BT_XSTACK_PHONEBOOK_STATUS_CONNECTION_LOST,
    BT_XSTACK_PHONEBOOK_STATUS_AUTH_TIMEOUT,
    BT_XSTACK_PHONEBOOK_STATUS_BUSY
} bt_xstack_phonebook_status_t;

typedef enum {
    BT_XSTACK_PHONEBOOK_DIRECTION_NONE = 0,
    BT_XSTACK_PHONEBOOK_DIRECTION_INCOMING,
    BT_XSTACK_PHONEBOOK_DIRECTION_OUTGOING,
    BT_XSTACK_PHONEBOOK_DIRECTION_MISSED
} bt_xstack_phonebook_direction_t;

typedef struct {
    char value[BT_XSTACK_PHONEBOOK_NUMBER_MAX + 1u];
    uint8_t type_flags;
} bt_xstack_phonebook_number_t;

typedef struct {
    char name[BT_XSTACK_PHONEBOOK_NAME_MAX + 1u];
    bt_xstack_phonebook_number_t numbers[BT_XSTACK_PHONEBOOK_NUMBER_COUNT];
    uint8_t number_count;
    uint8_t truncated;
} bt_xstack_phonebook_contact_t;

typedef struct {
    char name[BT_XSTACK_PHONEBOOK_NAME_MAX + 1u];
    char number[BT_XSTACK_PHONEBOOK_NUMBER_MAX + 1u];
    char timestamp[BT_XSTACK_PHONEBOOK_TIMESTAMP_MAX + 1u];
    uint8_t direction;
    uint8_t truncated;
} bt_xstack_phonebook_call_t;

typedef struct {
    bt_xstack_phonebook_event_id_t id;
    bt_xstack_addr_t address;
    bt_xstack_phonebook_category_t category;
    bt_xstack_phonebook_status_t status;
    uint32_t parser_error_count;
    union {
        bt_xstack_phonebook_contact_t contact;
        bt_xstack_phonebook_call_t call;
    } data;
} bt_xstack_phonebook_event_t;

typedef void (*bt_xstack_phonebook_callback_t)(
    const bt_xstack_phonebook_event_t *event,
    void *context);

bt_xstack_result_t bt_xstack_phonebook_sync(
    const bt_xstack_addr_t *address,
    bt_xstack_phonebook_category_mask_t categories);
bt_xstack_result_t bt_xstack_phonebook_cancel(void);
bt_xstack_result_t bt_xstack_phonebook_authenticate(const char *password);
bt_xstack_result_t bt_xstack_phonebook_get_state(
    bt_xstack_phonebook_state_t *state);

#ifdef __cplusplus
}
#endif

#endif

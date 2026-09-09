#ifndef BTSTACK_PROFILE_PBAP_CLIENT_H
#define BTSTACK_PROFILE_PBAP_CLIENT_H

#include <stdint.h>

#include "adapter/btstack_types.h"
#include "profiles/btstack_vcard_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BTSTACK_PBAP_CLIENT_PASSWORD_MAX 32u

typedef enum {
    BTSTACK_PBAP_CLIENT_STATE_IDLE = 0,
    BTSTACK_PBAP_CLIENT_STATE_CONNECTING,
    BTSTACK_PBAP_CLIENT_STATE_AUTHENTICATING,
    BTSTACK_PBAP_CLIENT_STATE_PULLING,
    BTSTACK_PBAP_CLIENT_STATE_PAUSED,
    BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING
} btstack_pbap_client_state_t;

typedef enum {
    BTSTACK_PBAP_CLIENT_CATEGORY_CONTACTS = 0,
    BTSTACK_PBAP_CLIENT_CATEGORY_INCOMING,
    BTSTACK_PBAP_CLIENT_CATEGORY_OUTGOING,
    BTSTACK_PBAP_CLIENT_CATEGORY_MISSED
} btstack_pbap_client_category_t;

typedef uint8_t btstack_pbap_client_category_mask_t;

#define BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS \
    ((btstack_pbap_client_category_mask_t) (1u << \
        BTSTACK_PBAP_CLIENT_CATEGORY_CONTACTS))
#define BTSTACK_PBAP_CLIENT_CATEGORY_MASK_INCOMING \
    ((btstack_pbap_client_category_mask_t) (1u << \
        BTSTACK_PBAP_CLIENT_CATEGORY_INCOMING))
#define BTSTACK_PBAP_CLIENT_CATEGORY_MASK_OUTGOING \
    ((btstack_pbap_client_category_mask_t) (1u << \
        BTSTACK_PBAP_CLIENT_CATEGORY_OUTGOING))
#define BTSTACK_PBAP_CLIENT_CATEGORY_MASK_MISSED \
    ((btstack_pbap_client_category_mask_t) (1u << \
        BTSTACK_PBAP_CLIENT_CATEGORY_MISSED))
#define BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL \
    ((btstack_pbap_client_category_mask_t) 0x0fu)

typedef enum {
    BTSTACK_PBAP_CLIENT_EVENT_DISCONNECTED = 0,
    BTSTACK_PBAP_CLIENT_EVENT_CONNECTING,
    BTSTACK_PBAP_CLIENT_EVENT_AUTHENTICATING,
    BTSTACK_PBAP_CLIENT_EVENT_PULLING,
    BTSTACK_PBAP_CLIENT_EVENT_PAUSED,
    BTSTACK_PBAP_CLIENT_EVENT_DISCONNECTING,
    BTSTACK_PBAP_CLIENT_EVENT_STARTED,
    BTSTACK_PBAP_CLIENT_EVENT_CONTACT,
    BTSTACK_PBAP_CLIENT_EVENT_CALL,
    BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED,
    BTSTACK_PBAP_CLIENT_EVENT_COMPLETED,
    BTSTACK_PBAP_CLIENT_EVENT_AUTHENTICATION_REQUIRED,
    BTSTACK_PBAP_CLIENT_EVENT_ERROR
} btstack_pbap_client_event_type_t;

typedef enum {
    BTSTACK_PBAP_CLIENT_STATUS_SUCCESS = 0,
    BTSTACK_PBAP_CLIENT_STATUS_PARTIAL,
    BTSTACK_PBAP_CLIENT_STATUS_UNSUPPORTED,
    BTSTACK_PBAP_CLIENT_STATUS_DENIED,
    BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_FAILED,
    BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR,
    BTSTACK_PBAP_CLIENT_STATUS_CANCELED,
    BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST,
    BTSTACK_PBAP_CLIENT_STATUS_AUTH_TIMEOUT,
    BTSTACK_PBAP_CLIENT_STATUS_BUSY
} btstack_pbap_client_status_t;

typedef struct {
    btstack_pbap_client_event_type_t type;
    btstack_bd_addr_t peer;
    btstack_pbap_client_category_t category;
    btstack_pbap_client_status_t status;
    btstack_vcard_record_t record;
    uint32_t parser_error_count;
} btstack_pbap_client_event_t;

/* Returning zero is only meaningful for CONTACT and CALL record events. */
typedef uint8_t (*btstack_pbap_client_event_callback_t)(
    const btstack_pbap_client_event_t *event);

int btstack_pbap_client_setup(void);
int btstack_pbap_client_set_event_callback(
    btstack_pbap_client_event_callback_t callback);
int btstack_pbap_client_sync(
    const btstack_bd_addr_t address,
    btstack_pbap_client_category_mask_t category_mask);
int btstack_pbap_client_cancel(void);
int btstack_pbap_client_authenticate(const char *password);
btstack_pbap_client_state_t btstack_pbap_client_get_state(void);
void btstack_pbap_client_event_space_available(void);
void btstack_pbap_client_peer_disconnected(const btstack_bd_addr_t address);

#ifdef __cplusplus
}
#endif

#endif

#ifndef BTSTACK_DEFINES_H
#define BTSTACK_DEFINES_H
#include <stdint.h>
typedef uint8_t bd_addr_t[6];
typedef void (*btstack_packet_handler_t)(uint8_t, uint16_t, uint8_t *, uint16_t);
typedef struct btstack_context_callback_registration {
    struct btstack_context_callback_registration *next;
    void (*callback)(void *context);
    void *context;
} btstack_context_callback_registration_t;
#define HCI_EVENT_PACKET 0x04u
#define PBAP_DATA_PACKET 0x0eu
#define HCI_EVENT_PBAP_META 0xeeu
#define PBAP_SUBEVENT_CONNECTION_OPENED 0x01u
#define PBAP_SUBEVENT_CONNECTION_CLOSED 0x02u
#define PBAP_SUBEVENT_OPERATION_COMPLETED 0x03u
#define PBAP_SUBEVENT_AUTHENTICATION_REQUEST 0x05u
#endif

#ifndef BTSTACK_AT_H
#define BTSTACK_AT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BTSTACK_AT_COMMAND_MAX
#define BTSTACK_AT_COMMAND_MAX 512u
#endif

#ifndef BTSTACK_AT_INPUT_QUEUE_LEN
#define BTSTACK_AT_INPUT_QUEUE_LEN 8u
#endif

#ifndef BTSTACK_AT_EVENT_QUEUE_LEN
#define BTSTACK_AT_EVENT_QUEUE_LEN 16u
#endif

#ifndef BTSTACK_AT_OUTPUT_MAX
#define BTSTACK_AT_OUTPUT_MAX 600u
#endif

#ifndef BTSTACK_AT_EXEC_TIMEOUT_MS
#define BTSTACK_AT_EXEC_TIMEOUT_MS 1000u
#endif

typedef enum {
    BTSTACK_AT_STATUS_OK = 0,
    BTSTACK_AT_STATUS_INVALID_ARGUMENT = -1,
    BTSTACK_AT_STATUS_NOT_INITIALIZED = -2,
    BTSTACK_AT_STATUS_ALREADY_STARTED = -3,
    BTSTACK_AT_STATUS_COMMAND_TOO_LONG = -4,
    BTSTACK_AT_STATUS_QUEUE_FULL = -5,
    BTSTACK_AT_STATUS_NOT_READY = -6
} btstack_at_status_t;

typedef void (*btstack_at_output_callback_t)(
    const char *data,
    uint16_t length,
    void *context);

typedef struct {
    btstack_at_output_callback_t output_callback;
    void *output_context;
    uint32_t task_priority;
    uint16_t task_stack_size;
} btstack_at_config_t;

/* Configure the dedicated AT worker and its unified response/event callback. */
int btstack_at_init(const btstack_at_config_t *config);

/* Initialize the fixed rings and start the AT worker. Idempotent after success. */
int btstack_at_start(void);

/*
 * Copy one complete AT command into the bounded input ring.
 * May be called from normal task context, but not from an ISR.
 */
int btstack_at_submit(const char *command, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif

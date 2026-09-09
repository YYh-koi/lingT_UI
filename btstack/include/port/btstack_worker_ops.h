#ifndef BTSTACK_WORKER_OPS_H
#define BTSTACK_WORKER_OPS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BTSTACK_WORKER_WAIT_FOREVER UINT32_MAX

#define BTSTACK_WORKER_SIGNAL_WORK          (1u << 0)
#define BTSTACK_WORKER_SIGNAL_COMMAND_SLOT  (1u << 1)
#define BTSTACK_WORKER_SIGNAL_REQUEST_SPACE (1u << 2)
#define BTSTACK_WORKER_SIGNAL_COMPLETION    (1u << 3)
#define BTSTACK_WORKER_SIGNAL_STACK_INGRESS_SPACE (1u << 4)

typedef enum {
    BTSTACK_WORKER_API = 0,
    BTSTACK_WORKER_EVENT,
    BTSTACK_WORKER_STACK,
    BTSTACK_WORKER_AT,
    BTSTACK_WORKER_COUNT
} btstack_worker_id_t;

typedef void (*btstack_worker_entry_t)(void *argument);

typedef struct {
    uint32_t priority;
    uint16_t stack_words;
} btstack_worker_config_t;

typedef struct {
    void *context;
    int (*validate_config)(
        void *context,
        btstack_worker_id_t id,
        const btstack_worker_config_t *config);
    int (*start)(
        void *context,
        btstack_worker_id_t id,
        btstack_worker_entry_t entry,
        void *argument,
        const btstack_worker_config_t *config);
    int (*notify)(
        void *context,
        btstack_worker_id_t id,
        uint32_t signals);
    uint32_t (*wait)(
        void *context,
        btstack_worker_id_t id,
        uint32_t signal_mask,
        uint32_t timeout_ms);
    int (*stop)(void *context, btstack_worker_id_t id);
} btstack_worker_ops_t;

#ifdef __cplusplus
}
#endif

#endif

#ifndef BTSTACK_RUN_LOOP_H
#define BTSTACK_RUN_LOOP_H
#include <stdint.h>
#include "btstack_defines.h"
typedef struct btstack_timer_source {
    struct btstack_timer_source *next;
    uint32_t timeout;
    void (*process)(struct btstack_timer_source *timer);
    void *context;
} btstack_timer_source_t;
void btstack_run_loop_set_timer_handler(btstack_timer_source_t *timer, void (*process)(btstack_timer_source_t *));
void btstack_run_loop_set_timer(btstack_timer_source_t *timer, uint32_t timeout_ms);
void btstack_run_loop_add_timer(btstack_timer_source_t *timer);
int btstack_run_loop_remove_timer(btstack_timer_source_t *timer);
void btstack_run_loop_execute_on_main_thread(btstack_context_callback_registration_t *registration);
#endif

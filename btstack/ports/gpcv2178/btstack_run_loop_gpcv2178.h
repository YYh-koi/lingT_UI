#ifndef BTSTACK_RUN_LOOP_GPCV2178_H
#define BTSTACK_RUN_LOOP_GPCV2178_H

#include <stdbool.h>
#include <stdint.h>

#include "btstack_run_loop.h"

#include "port/btstack_stack_ops.h"

typedef btstack_stack_queue_diagnostics_t
    btstack_run_loop_gpcv2178_diagnostics_t;

const btstack_run_loop_t *btstack_run_loop_gpcv2178_get_instance(void);
bool btstack_run_loop_gpcv2178_enqueue_callback(
    btstack_context_callback_registration_t *callback_registration);
void btstack_run_loop_gpcv2178_get_diagnostics(
    btstack_run_loop_gpcv2178_diagnostics_t *diagnostics);
void btstack_run_loop_gpcv2178_wake(void);
void btstack_run_loop_gpcv2178_wake_from_irq(void);

#endif

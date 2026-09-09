#ifndef BT_XSTACK_TEST_RUN_LOOP_GPCV2178_H
#define BT_XSTACK_TEST_RUN_LOOP_GPCV2178_H

#include <stdint.h>

typedef struct {
    uint32_t current_depth;
    uint32_t max_depth;
    uint32_t full_count;
} btstack_run_loop_gpcv2178_diagnostics_t;

void btstack_run_loop_gpcv2178_get_diagnostics(
    btstack_run_loop_gpcv2178_diagnostics_t *diagnostics);

#endif

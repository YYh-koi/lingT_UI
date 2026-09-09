#ifndef BTSTACK_RUNTIME_H
#define BTSTACK_RUNTIME_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned int current_depth;
    unsigned int max_depth;
    unsigned int full_count;
} btstack_runtime_diagnostics_t;

int btstack_runtime_init(void);
int btstack_runtime_start(void);
int btstack_runtime_started(void);
void btstack_runtime_get_diagnostics(
    btstack_runtime_diagnostics_t *diagnostics);

#ifdef __cplusplus
}
#endif

#endif

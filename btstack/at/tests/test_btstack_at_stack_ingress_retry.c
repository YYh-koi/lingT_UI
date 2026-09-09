#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "project.h"
#include "btstack_at_internal.h"
#include "btstack_run_loop.h"
#include "port/btstack_platform.h"
#include "core/btstack_profile_manager.h"

bool btstack_profile_manager_hfp_event_delivered(
    uint8_t event_type, const uint8_t address[6])
{ (void)event_type; return address != NULL; }

static int test_critical_depth;
static unsigned int test_notify_count;
static unsigned int test_cleanup_dispatch_count;
static unsigned int test_cleanup_execute_count;
static unsigned int test_ingress_execute_count;
static uint8_t test_cleanup_address[6];
static btstack_core_request_t test_cleanup_request;
static btstack_context_callback_registration_t test_cleanup_callback;

void test_task_enter_critical(void)
{
    assert(test_critical_depth == 0);
    test_critical_depth++;
}

void test_task_exit_critical(void)
{
    assert(test_critical_depth == 1);
    test_critical_depth--;
}

void btstack_platform_enter_critical(void)
{
    test_task_enter_critical();
}

void btstack_platform_exit_critical(void)
{
    test_task_exit_critical();
}

SemaphoreHandle_t xSemaphoreCreateBinary(void)
{
    return (SemaphoreHandle_t)(uintptr_t)1u;
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t semaphore)
{
    assert(semaphore != NULL);
    return pdTRUE;
}

BaseType_t xSemaphoreTake(SemaphoreHandle_t semaphore, TickType_t ticks)
{
    (void)ticks;
    assert(semaphore != NULL);
    return pdTRUE;
}

BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t semaphore,
    BaseType_t *higher_priority_task_woken)
{
    assert(semaphore != NULL);
    assert(higher_priority_task_woken != NULL);
    return pdTRUE;
}

TickType_t xTaskGetTickCount(void)
{
    return 0u;
}

void vTaskDelay(TickType_t ticks)
{
    (void)ticks;
}

void test_port_yield_from_isr(BaseType_t higher_priority_task_woken)
{
    (void)higher_priority_task_woken;
}

void btstack_run_loop_base_init(void) {}
void btstack_run_loop_base_add_timer(btstack_timer_source_t *timer)
{ (void)timer; }
bool btstack_run_loop_base_remove_timer(btstack_timer_source_t *timer)
{ (void)timer; return false; }
void btstack_run_loop_base_process_timers(uint32_t now)
{ (void)now; }
void btstack_run_loop_base_dump_timer(void) {}
int32_t btstack_run_loop_base_get_time_until_timeout(uint32_t now)
{ (void)now; return -1; }
void btstack_run_loop_base_add_data_source(btstack_data_source_t *data_source)
{ (void)data_source; }
bool btstack_run_loop_base_remove_data_source(btstack_data_source_t *data_source)
{ (void)data_source; return false; }
void btstack_run_loop_base_enable_data_source_callbacks(
    btstack_data_source_t *data_source, uint16_t callbacks)
{ (void)data_source; (void)callbacks; }
void btstack_run_loop_base_disable_data_source_callbacks(
    btstack_data_source_t *data_source, uint16_t callbacks)
{ (void)data_source; (void)callbacks; }
void btstack_run_loop_base_poll_data_sources(void) {}
void btstack_run_loop_base_execute_callbacks(void) {}

#include "../../ports/gpcv2178/btstack_run_loop_gpcv2178.c"

int btstack_platform_notify(btstack_worker_id_t id, uint32_t signals)
{
    assert(test_critical_depth == 0);
    assert(id == BTSTACK_WORKER_AT);
    assert(signals == BTSTACK_WORKER_SIGNAL_STACK_INGRESS_SPACE);
    test_notify_count++;
    /* Model the AT worker consuming this one blocking-wait signal. */
    btstack_at_stack_ingress_space_available();
    return 0;
}

bool btstack_platform_post_stack_callback(
    btstack_context_callback_registration_t *callback)
{
    return btstack_run_loop_gpcv2178_enqueue_callback(callback);
}

static void test_execute_cleanup(void *context)
{
    const btstack_core_request_t *request = context;

    assert(request == &test_cleanup_request);
    assert(request->id == BT_XSTACK_REQUEST_PBAP_PEER_DISCONNECTED);
    memcpy(test_cleanup_address, request->address.bytes, 6u);
    test_cleanup_execute_count++;
    btstack_core_request_completed(request, 0);
}

int btstack_core_dispatch(const btstack_core_request_t *request)
{
    assert(request != NULL);
    assert(request->id == BT_XSTACK_REQUEST_PBAP_PEER_DISCONNECTED);
    test_cleanup_dispatch_count++;
    test_cleanup_request = *request;
    test_cleanup_callback.callback = test_execute_cleanup;
    test_cleanup_callback.context = &test_cleanup_request;
    return btstack_platform_post_stack_callback(&test_cleanup_callback) ?
        0 : BT_XSTACK_BACKEND_BUSY;
}

int btstack_at_backend_register(btstack_at_backend_dispatch_t dispatch)
{ (void)dispatch; return BTSTACK_AT_STATUS_OK; }
int btstack_at_command_gate_register(btstack_at_command_gate_t gate,
    void *context)
{ (void)gate; (void)context; return BTSTACK_AT_STATUS_OK; }
int btstack_at_complete(const btstack_at_result_t *result)
{ (void)result; return BTSTACK_AT_STATUS_OK; }
int btstack_at_post_event(const btstack_at_event_t *event)
{ (void)event; return BTSTACK_AT_STATUS_OK; }
int btstack_runtime_started(void) { return 1; }
int btstack_runtime_start(void) { return 0; }
int btstack_core_copy_state(bt_xstack_state_t *state)
{ memset(state, 0, sizeof(*state)); return 0; }
int btstack_core_state_read_pbap(bt_xstack_phonebook_state_t *state)
{ *state = BT_XSTACK_PHONEBOOK_STATE_IDLE; return 0; }

static void test_ingress_callback(void *context)
{
    (void)context;
    test_ingress_execute_count++;
}

int main(void)
{
    static const uint8_t lost_address[6] = {
        0x31u, 0x32u, 0x33u, 0x34u, 0x35u, 0x36u
    };
    btstack_context_callback_registration_t ingress[16];
    btstack_context_callback_registration_t *callback;
    btstack_at_event_t lost;
    unsigned int index;

    memset(ingress, 0, sizeof(ingress));
    memset(&lost, 0, sizeof(lost));
    lost.id = BTSTACK_AT_EVENT_HFPLOST;
    memcpy(lost.data.peer.address, lost_address, 6u);

    btstack_run_loop_gpcv2178_init();
    for (index = 0u; index < 16u; index++) {
        ingress[index].callback = test_ingress_callback;
        assert(btstack_run_loop_gpcv2178_enqueue_callback(&ingress[index]));
    }

    /* HFPLOST text has already been output when this hook is called. */
    btstack_at_event_consumed(&lost);
    assert(test_cleanup_dispatch_count == 1u);
    assert(!btstack_at_event_drain_ready());

    /* No AT event or command-slot edge: a real dequeue is the only wake. */
    callback = btstack_run_loop_gpcv2178_dequeue_callback();
    assert(callback == &ingress[0]);
    assert(test_notify_count == 1u);
    assert(test_cleanup_dispatch_count == 2u);
    assert(!btstack_at_event_drain_ready());
    callback->callback(callback->context);

    for (index = 1u; index < 16u; index++) {
        callback = btstack_run_loop_gpcv2178_dequeue_callback();
        assert(callback == &ingress[index]);
        callback->callback(callback->context);
    }
    callback = btstack_run_loop_gpcv2178_dequeue_callback();
    assert(callback == &test_cleanup_callback);
    callback->callback(callback->context);

    assert(test_ingress_execute_count == 16u);
    assert(test_cleanup_execute_count == 1u);
    assert(test_cleanup_dispatch_count == 2u);
    assert(memcmp(test_cleanup_address, lost_address, 6u) == 0);
    assert(btstack_at_event_drain_ready());
    return 0;
}

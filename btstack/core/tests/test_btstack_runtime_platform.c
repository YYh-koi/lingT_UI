#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "adapter/btstack_adapter.h"
#include "adapter/btstack_types.h"
#include "api/bt_xstack.h"
#include "btstack_core_internal.h"
#include "btstack_gpcv2178_port.h"
#include "btstack_memory.h"
#include "core/btstack_profile_manager.h"
#include "btstack_runtime.h"
#include "hci.h"
#include "l2cap.h"
#include "port/btstack_platform.h"

enum failure_stage {
    FAIL_NONE,
    FAIL_EARLY,
    FAIL_AFTER_HCI,
    FAIL_ADAPTER,
    FAIL_PROFILE_MANAGER,
    FAIL_BACKEND,
    FAIL_STACK_START
};

static enum failure_stage failure;
static const char *calls[32];
static unsigned int call_count;
static unsigned int late_count;
static unsigned int stack_start_count;
static unsigned int run_loop_execute_count;
static int reenter_stack_start;
static int reentrant_start_status;
static int reenter_runtime_init;
static int reenter_api_init;
static int reenter_legacy_init;
static int reentrant_runtime_init_status;
static int reentrant_legacy_init_status;
static bt_xstack_result_t reentrant_api_init_status;
static bt_xstack_config_t interleaving_config;
static unsigned int diagnostics_count;
static CRITICAL_SECTION platform_critical;
static int platform_critical_initialized;
static HANDLE diagnostics_failure_thread;
static HANDLE diagnostics_failure_entered;
static HANDLE diagnostics_callback_completed;
static int diagnostics_failure_status;
static int diagnostics_interleaving;
static int late_overlapped_diagnostics;

static void record(const char *name)
{
    assert(call_count < (sizeof(calls) / sizeof(calls[0])));
    calls[call_count++] = name;
}

static void enter_critical(void *context)
{ (void)context; if (platform_critical_initialized) EnterCriticalSection(&platform_critical); }
static void exit_critical(void *context)
{ (void)context; if (platform_critical_initialized) LeaveCriticalSection(&platform_critical); }
static uint32_t time_ms(void *context) { (void)context; return 0u; }

static int worker_validate(void *context, btstack_worker_id_t id,
    const btstack_worker_config_t *config)
{
    (void)context;
    assert(config != NULL);
    if (id == BTSTACK_WORKER_STACK) {
        assert(config->priority == 0u);
        assert(config->stack_words == 0u);
    }
    return BTSTACK_STATUS_OK;
}

static int worker_start(void *context, btstack_worker_id_t id,
    btstack_worker_entry_t entry, void *argument,
    const btstack_worker_config_t *config)
{
    (void)context;
    assert(entry != NULL);
    assert(config != NULL);
    if (id != BTSTACK_WORKER_STACK) return BTSTACK_STATUS_OK;
    assert(argument == NULL);
    record("start(STACK)");
    stack_start_count++;
    if (failure == FAIL_STACK_START) return BTSTACK_STATUS_ERROR;
    if (reenter_stack_start) {
        reenter_stack_start = 0;
        reentrant_start_status = btstack_runtime_start();
    }
    entry(argument);
    return BTSTACK_STATUS_OK;
}

static int worker_notify(void *context, btstack_worker_id_t id,
    uint32_t signals)
{ (void)context; (void)id; (void)signals; return BTSTACK_STATUS_OK; }
static uint32_t worker_wait(void *context, btstack_worker_id_t id,
    uint32_t mask, uint32_t timeout_ms)
{ (void)context; (void)id; (void)mask; (void)timeout_ms; return 0u; }
static int worker_stop(void *context, btstack_worker_id_t id)
{ (void)context; (void)id; return BTSTACK_STATUS_OK; }

static void control_init(const void *config) { (void)config; }
static const btstack_control_t control = { .init = control_init };
static const btstack_run_loop_t run_loop = {0};
static const hci_transport_t transport = {0};
static const uint8_t transport_config;
static const btstack_profile_manager_config_t profile_manager_config = {0};

static int early_init(void *context)
{
    (void)context;
    record("early_init");
    if (reenter_runtime_init) {
        reenter_runtime_init = 0;
        reentrant_runtime_init_status = btstack_runtime_init();
    }
    if (reenter_api_init) {
        reenter_api_init = 0;
        reentrant_api_init_status = bt_xstack_init(&interleaving_config);
    }
    if (reenter_legacy_init) {
        reenter_legacy_init = 0;
        reentrant_legacy_init_status = btstack_gpcv2178_init();
    }
    return failure == FAIL_EARLY ? BTSTACK_STATUS_ERROR : BTSTACK_STATUS_OK;
}

static void late_deinit(void *context)
{
    (void)context;
    record("late_deinit");
    if (diagnostics_interleaving &&
        (WaitForSingleObject(diagnostics_callback_completed, 0u) !=
            WAIT_OBJECT_0)) late_overlapped_diagnostics = 1;
    late_count++;
}

static const btstack_run_loop_t *get_run_loop(void *context)
{ (void)context; return &run_loop; }
static const hci_transport_t *get_transport(void *context)
{ (void)context; return &transport; }
static const void *get_transport_config(void *context)
{ (void)context; return &transport_config; }
static const btstack_profile_manager_config_t *get_profile_manager_config(
    void *context)
{ (void)context; return &profile_manager_config; }

static int after_hci_init(void *context)
{
    (void)context;
    record("after_hci_init");
    return failure == FAIL_AFTER_HCI ?
        BTSTACK_STATUS_ERROR : BTSTACK_STATUS_OK;
}

static bool post_callback(void *context,
    btstack_context_callback_registration_t *callback)
{ (void)context; return callback != NULL; }

static void get_diagnostics(void *context,
    btstack_stack_queue_diagnostics_t *diagnostics)
{
    (void)context;
    diagnostics_count++;
    if (diagnostics_interleaving) {
        assert(SetEvent(diagnostics_failure_entered));
        assert(WaitForSingleObject(diagnostics_callback_completed, 0u) ==
            WAIT_TIMEOUT);
        Sleep(20u);
        assert(late_count == 0u);
        assert(SetEvent(diagnostics_callback_completed));
    }
    diagnostics->current_depth = 3u;
    diagnostics->max_depth = 5u;
    diagnostics->full_count = 7u;
}

static DWORD WINAPI diagnostics_start_failure_thread(void *context)
{
    (void)context;
    assert(WaitForSingleObject(diagnostics_failure_entered, 5000u) ==
        WAIT_OBJECT_0);
    diagnostics_failure_status = btstack_runtime_start();
    return 0u;
}

int btstack_core_dispatch(const btstack_core_request_t *request)
{ (void)request; return BTSTACK_STATUS_OK; }
int btstack_core_copy_state(bt_xstack_state_t *state)
{ (void)state; return BTSTACK_STATUS_ERROR; }
int btstack_core_state_read_service(btstack_core_service_snapshot_t *state)
{ (void)state; return BTSTACK_STATUS_ERROR; }
int btstack_core_state_read_hfp(btstack_core_hfp_snapshot_t *state)
{ (void)state; return BTSTACK_STATUS_ERROR; }
int btstack_core_state_read_a2dp(btstack_core_a2dp_snapshot_t *state)
{ (void)state; return BTSTACK_STATUS_ERROR; }
int btstack_core_state_read_avrcp(btstack_core_avrcp_snapshot_t *state)
{ (void)state; return BTSTACK_STATUS_ERROR; }
int btstack_core_state_read_pbap(bt_xstack_phonebook_state_t *state)
{ (void)state; return BTSTACK_STATUS_ERROR; }
void btstack_core_dispatcher_get_diagnostics(
    btstack_core_backend_diagnostics_t *diagnostics)
{ memset(diagnostics, 0, sizeof(*diagnostics)); }
int btstack_core_state_get_peer_count(uint8_t *count)
{ (void)count; return BTSTACK_STATUS_ERROR; }
int btstack_core_state_get_peer(uint8_t index,
    bt_xstack_paired_device_t *peer)
{ (void)index; (void)peer; return BTSTACK_STATUS_ERROR; }

static const btstack_platform_t platform = {
    .abi_version = BTSTACK_PLATFORM_ABI_VERSION,
    .struct_size = sizeof(btstack_platform_t),
    .name = "runtime-host",
    .system = {
        .enter_critical = enter_critical,
        .exit_critical = exit_critical,
        .time_ms = time_ms
    },
    .workers = {
        .validate_config = worker_validate,
        .start = worker_start,
        .notify = worker_notify,
        .wait = worker_wait,
        .stop = worker_stop
    },
    .controller = { .control = &control },
    .stack = {
        .early_init = early_init,
        .late_deinit = late_deinit,
        .get_run_loop = get_run_loop,
        .get_hci_transport = get_transport,
        .get_hci_transport_config = get_transport_config,
        .get_profile_manager_config = get_profile_manager_config,
        .after_hci_init = after_hci_init,
        .post_callback = post_callback,
        .get_diagnostics = get_diagnostics
    }
};

const btstack_platform_t *btstack_platform_default(void)
{
    record("activate");
    return &platform;
}

void btstack_run_loop_init(const btstack_run_loop_t *value)
{ assert(value == &run_loop); record("run_loop_init"); }
void btstack_run_loop_execute(void)
{ record("run_loop_execute"); run_loop_execute_count++; }
void btstack_memory_init(void) { record("memory_init"); }
void hci_init(const hci_transport_t *value, const void *config)
{ assert(value == &transport); assert(config == &transport_config); record("hci_init"); }
void hci_set_control(const btstack_control_t *value)
{ assert(value == &control); record("hci_set_control"); }
void l2cap_init(void) { record("l2cap_init"); }

int btstack_adapter_setup(void)
{
    record("adapter_setup");
    return failure == FAIL_ADAPTER ? BTSTACK_STATUS_ERROR : BTSTACK_STATUS_OK;
}

int btstack_core_event_mapper_init(void)
{
    record("event_mapper_init");
    return BTSTACK_STATUS_OK;
}

int btstack_profile_manager_init(
    const btstack_profile_manager_config_t *config)
{
    assert(config == &profile_manager_config);
    record("profile_manager_init");
    return failure == FAIL_PROFILE_MANAGER ?
        BTSTACK_STATUS_ERROR : BTSTACK_STATUS_OK;
}

bool btstack_profile_manager_hfp_event_delivered(
    uint8_t event_type,
    const uint8_t address[6])
{
    (void)event_type;
    (void)address;
    return false;
}

int btstack_core_dispatcher_init(void)
{
    record("core_backend_init");
    return failure == FAIL_BACKEND ? BTSTACK_STATUS_ERROR : BTSTACK_STATUS_OK;
}

static void activate_platform(void)
{
    assert(btstack_platform_activate(btstack_platform_default()) ==
        BTSTACK_PLATFORM_OK);
}

static void expect_calls(const char *const *expected, unsigned int count)
{
    unsigned int i;
    assert(call_count == count);
    for (i = 0u; i < count; i++) assert(strcmp(calls[i], expected[i]) == 0);
}

static void run_success(void)
{
    static const char *const expected_init[] = {
        "activate", "early_init", "run_loop_init", "memory_init",
        "hci_init", "hci_set_control", "after_hci_init", "l2cap_init",
        "adapter_setup", "event_mapper_init", "profile_manager_init",
        "core_backend_init"
    };
    btstack_runtime_diagnostics_t diagnostics;

    activate_platform();
    assert(btstack_runtime_init() == BTSTACK_STATUS_OK);
    expect_calls(expected_init,
        (unsigned int)(sizeof(expected_init) / sizeof(expected_init[0])));
    assert(!btstack_runtime_started());
    assert(btstack_runtime_start() == BTSTACK_STATUS_OK);
    assert(btstack_runtime_started());
    assert(stack_start_count == 1u);
    assert(run_loop_execute_count == 1u);
    assert(btstack_runtime_start() == BTSTACK_STATUS_OK);
    assert(stack_start_count == 1u);
    btstack_runtime_get_diagnostics(&diagnostics);
    assert(diagnostics.current_depth == 3u);
    assert(diagnostics.max_depth == 5u);
    assert(diagnostics.full_count == 7u);
    assert(late_count == 0u);
}

static void run_failure(enum failure_stage stage)
{
    unsigned int count_after_failure;
    failure = stage;
    activate_platform();
    assert(btstack_runtime_init() != BTSTACK_STATUS_OK);
    assert(!btstack_runtime_started());
    assert(btstack_runtime_start() != BTSTACK_STATUS_OK);
    assert(late_count == (stage == FAIL_EARLY ? 0u : 1u));
    count_after_failure = call_count;
    assert(btstack_runtime_init() != BTSTACK_STATUS_OK);
    assert(call_count == count_after_failure);
    {
        btstack_runtime_diagnostics_t diagnostics = {9u, 9u, 9u};
        btstack_runtime_get_diagnostics(&diagnostics);
        assert(diagnostics.current_depth == 0u);
        assert(diagnostics.max_depth == 0u);
        assert(diagnostics.full_count == 0u);
        assert(diagnostics_count == 0u);
    }
}

static void run_reentrant_runtime_init(void)
{
    activate_platform();
    reenter_runtime_init = 1;
    assert(btstack_runtime_init() == BTSTACK_STATUS_OK);
    assert(reentrant_runtime_init_status == BTSTACK_STATUS_BUSY);
}

static void run_legacy_to_api_interleaving(void)
{
    activate_platform();
    memset(&interleaving_config, 0, sizeof(interleaving_config));
    reenter_api_init = 1;
    assert(btstack_gpcv2178_init() == BTSTACK_STATUS_OK);
    assert(reentrant_api_init_status == BT_XSTACK_NOT_READY);
    assert(!bt_xstack_is_ready());
    assert(bt_xstack_init(&interleaving_config) == BT_XSTACK_OK);
    assert(bt_xstack_is_ready());
}

static void run_api_to_legacy_interleaving(void)
{
    memset(&interleaving_config, 0, sizeof(interleaving_config));
    reenter_legacy_init = 1;
    assert(bt_xstack_init(&interleaving_config) == BT_XSTACK_OK);
    assert(reentrant_legacy_init_status == BTSTACK_STATUS_BUSY);
    assert(bt_xstack_is_ready());
}

static void run_diagnostics_deinit_interleaving(void)
{
    btstack_runtime_diagnostics_t diagnostics;
    DWORD exit_code;

    InitializeCriticalSection(&platform_critical);
    platform_critical_initialized = 1;
    diagnostics_failure_entered = CreateEvent(NULL, TRUE, FALSE, NULL);
    diagnostics_callback_completed = CreateEvent(NULL, TRUE, FALSE, NULL);
    assert(diagnostics_failure_entered != NULL);
    assert(diagnostics_callback_completed != NULL);
    activate_platform();
    assert(btstack_runtime_init() == BTSTACK_STATUS_OK);
    failure = FAIL_STACK_START;
    diagnostics_interleaving = 1;
    diagnostics_failure_thread = CreateThread(NULL, 0u,
        diagnostics_start_failure_thread, NULL, 0u, NULL);
    assert(diagnostics_failure_thread != NULL);
    btstack_runtime_get_diagnostics(&diagnostics);
    assert(diagnostics.current_depth == 3u);
    assert(WaitForSingleObject(diagnostics_failure_thread, 5000u) ==
        WAIT_OBJECT_0);
    assert(GetExitCodeThread(diagnostics_failure_thread, &exit_code));
    assert(exit_code == 0u);
    assert(diagnostics_failure_status != BTSTACK_STATUS_OK);
    assert(late_count == 1u);
    assert(!late_overlapped_diagnostics);
    diagnostics.current_depth = 9u;
    diagnostics.max_depth = 9u;
    diagnostics.full_count = 9u;
    btstack_runtime_get_diagnostics(&diagnostics);
    assert(diagnostics.current_depth == 0u);
    assert(diagnostics.max_depth == 0u);
    assert(diagnostics.full_count == 0u);
    assert(diagnostics_count == 1u);
    CloseHandle(diagnostics_failure_thread);
    CloseHandle(diagnostics_failure_entered);
    CloseHandle(diagnostics_callback_completed);
    platform_critical_initialized = 0;
    DeleteCriticalSection(&platform_critical);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    if (strcmp(argv[1], "success") == 0) run_success();
    else if (strcmp(argv[1], "early-fail") == 0) run_failure(FAIL_EARLY);
    else if (strcmp(argv[1], "after-hci-fail") == 0) run_failure(FAIL_AFTER_HCI);
    else if (strcmp(argv[1], "adapter-fail") == 0) run_failure(FAIL_ADAPTER);
    else if (strcmp(argv[1], "profile-manager-fail") == 0) {
        run_failure(FAIL_PROFILE_MANAGER);
    }
    else if (strcmp(argv[1], "backend-fail") == 0) run_failure(FAIL_BACKEND);
    else if (strcmp(argv[1], "stack-start-fail") == 0) {
        failure = FAIL_STACK_START;
        activate_platform();
        assert(btstack_runtime_init() == BTSTACK_STATUS_OK);
        assert(btstack_runtime_start() != BTSTACK_STATUS_OK);
        assert(!btstack_runtime_started());
        assert(late_count == 1u);
        assert(btstack_runtime_start() != BTSTACK_STATUS_OK);
        assert(stack_start_count == 1u);
        assert(late_count == 1u);
    } else if (strcmp(argv[1], "reentrant-start") == 0) {
        activate_platform();
        assert(btstack_runtime_init() == BTSTACK_STATUS_OK);
        reenter_stack_start = 1;
        assert(btstack_runtime_start() == BTSTACK_STATUS_OK);
        assert(reentrant_start_status == BTSTACK_STATUS_BUSY);
        assert(btstack_runtime_started());
        assert(stack_start_count == 1u);
        assert(late_count == 0u);
    } else if (strcmp(argv[1], "reentrant-init") == 0) {
        run_reentrant_runtime_init();
    } else if (strcmp(argv[1], "legacy-to-api") == 0) {
        run_legacy_to_api_interleaving();
    } else if (strcmp(argv[1], "api-to-legacy") == 0) {
        run_api_to_legacy_interleaving();
    } else if (strcmp(argv[1], "diagnostics-deinit") == 0) {
        run_diagnostics_deinit_interleaving();
    } else return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

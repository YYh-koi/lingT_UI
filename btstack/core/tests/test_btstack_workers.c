#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "bt_xstack.h"
#include "btstack_core_internal.h"
#include "btstack_runtime.h"
#include "port/btstack_platform.h"
#include "core/btstack_profile_manager.h"

bool btstack_profile_manager_hfp_event_delivered(
    uint8_t event_type, const uint8_t address[6])
{ (void)event_type; return address != NULL; }

typedef struct {
    btstack_worker_entry_t entries[BTSTACK_WORKER_COUNT];
    void *arguments[BTSTACK_WORKER_COUNT];
    btstack_worker_config_t configs[BTSTACK_WORKER_COUNT];
    uint32_t notified[BTSTACK_WORKER_COUNT];
    btstack_worker_id_t start_order[BTSTACK_WORKER_COUNT];
    btstack_worker_id_t stop_order[BTSTACK_WORKER_COUNT];
    unsigned int start_count;
    unsigned int stop_count;
    unsigned int notify_count;
    unsigned int wait_count;
    int fail_start_id;
    int release_request_space;
    int critical_depth;
    int critical_violation;
} worker_fixture_t;

static worker_fixture_t fixture;
static int entry_active[BTSTACK_WORKER_COUNT];
static unsigned int backend_count;
static unsigned int service_callback_count;
static unsigned int audio_callback_count;
static unsigned int runtime_init_count;
static int runtime_started = 1;
static int runtime_start_in_progress;
static int runtime_reenter_enable;
static bt_xstack_result_t runtime_reentrant_enable_result;

static void invoke_worker(btstack_worker_id_t id);

static void enter_critical(void *context)
{
    worker_fixture_t *state = context;
    state->critical_depth++;
    if (state->critical_depth != 1) state->critical_violation = 1;
}

static void exit_critical(void *context)
{
    worker_fixture_t *state = context;
    if (state->critical_depth != 1) state->critical_violation = 1;
    state->critical_depth--;
}

static uint32_t time_ms(void *context)
{
    (void)context;
    return 0u;
}

static int worker_validate(void *context, btstack_worker_id_t id,
    const btstack_worker_config_t *config)
{
    (void)context;
    if ((config == NULL) || (id >= BTSTACK_WORKER_COUNT)) return -1;
    if ((id == BTSTACK_WORKER_API) &&
        (config->stack_words != BT_XSTACK_API_TASK_STACK_WORDS)) return -1;
    if ((id == BTSTACK_WORKER_EVENT) &&
        (config->stack_words != BT_XSTACK_EVENT_TASK_STACK_WORDS)) return -1;
    return 0;
}

static int worker_start(void *context, btstack_worker_id_t id,
    btstack_worker_entry_t entry, void *argument,
    const btstack_worker_config_t *config)
{
    worker_fixture_t *state = context;
    assert(state->critical_depth == 0);
    state->start_order[state->start_count++] = id;
    if ((int)id == state->fail_start_id) return -1;
    state->entries[id] = entry;
    state->arguments[id] = argument;
    state->configs[id] = *config;
    return 0;
}

static int worker_notify(void *context, btstack_worker_id_t id,
    uint32_t signals)
{
    worker_fixture_t *state = context;
    if (state->critical_depth != 0) state->critical_violation = 1;
    state->notified[id] |= signals;
    state->notify_count++;
    return 0;
}

static uint32_t worker_wait(void *context, btstack_worker_id_t id,
    uint32_t mask, uint32_t timeout_ms)
{
    worker_fixture_t *state = context;
    uint32_t signals;
    (void)timeout_ms;
    assert(state->critical_depth == 0);
    state->wait_count++;
    signals = state->notified[id] & mask;
    state->notified[id] &= ~signals;
    if (signals != 0u) return signals;
    if (state->release_request_space &&
        (id == BTSTACK_WORKER_EVENT) &&
        ((mask & BTSTACK_WORKER_SIGNAL_REQUEST_SPACE) != 0u)) {
        state->release_request_space = 0;
        invoke_worker(BTSTACK_WORKER_API);
        return BTSTACK_WORKER_SIGNAL_REQUEST_SPACE;
    }
    assert(entry_active[id]);
    ExitThread(0u);
    return 0u;
}

static int worker_stop(void *context, btstack_worker_id_t id)
{
    worker_fixture_t *state = context;
    state->stop_order[state->stop_count++] = id;
    state->entries[id] = NULL;
    return 0;
}

static void control_init(const void *config) { (void)config; }
static const btstack_control_t control = { .init = control_init };
static const btstack_run_loop_t run_loop = {0};
static const hci_transport_t transport = {0};
static const uint8_t transport_config;
static int early_init(void *context) { (void)context; return 0; }
static void late_deinit(void *context) { (void)context; }
static const btstack_run_loop_t *get_run_loop(void *context)
{ (void)context; return &run_loop; }
static const hci_transport_t *get_transport(void *context)
{ (void)context; return &transport; }
static const void *get_transport_config(void *context)
{ (void)context; return &transport_config; }
static bool post_callback(void *context,
    btstack_context_callback_registration_t *callback)
{ (void)context; (void)callback; return true; }

static const btstack_platform_t platform = {
    .abi_version = BTSTACK_PLATFORM_ABI_VERSION,
    .struct_size = sizeof(btstack_platform_t),
    .name = "worker-test",
    .system = {
        .context = &fixture,
        .enter_critical = enter_critical,
        .exit_critical = exit_critical,
        .time_ms = time_ms
    },
    .workers = {
        .context = &fixture,
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
        .post_callback = post_callback
    }
};

const btstack_platform_t *btstack_platform_default(void) { return &platform; }

int btstack_runtime_init(void) { runtime_init_count++; return 0; }
int btstack_runtime_start(void)
{
    if (runtime_start_in_progress) return BTSTACK_STATUS_BUSY;
    runtime_start_in_progress = 1;
    if (runtime_reenter_enable) {
        runtime_reenter_enable = 0;
        runtime_reentrant_enable_result = bt_xstack_enable();
    }
    runtime_started = 1;
    runtime_start_in_progress = 0;
    return BTSTACK_STATUS_OK;
}
int btstack_runtime_started(void) { return runtime_started; }
void btstack_runtime_get_diagnostics(btstack_runtime_diagnostics_t *value)
{ if (value != NULL) memset(value, 0, sizeof(*value)); }

int btstack_core_dispatch(const btstack_core_request_t *request)
{ (void)request; backend_count++; return 0; }
int btstack_core_copy_state(bt_xstack_state_t *state)
{ (void)state; return -1; }
int btstack_core_state_read_service(btstack_core_service_snapshot_t *state)
{ (void)state; return -1; }
int btstack_core_state_read_hfp(btstack_core_hfp_snapshot_t *state)
{ (void)state; return -1; }
int btstack_core_state_read_a2dp(btstack_core_a2dp_snapshot_t *state)
{ (void)state; return -1; }
int btstack_core_state_read_avrcp(btstack_core_avrcp_snapshot_t *state)
{ (void)state; return -1; }
int btstack_core_state_read_pbap(bt_xstack_phonebook_state_t *state)
{ (void)state; return -1; }
void btstack_core_dispatcher_get_diagnostics(
    btstack_core_backend_diagnostics_t *value)
{ memset(value, 0, sizeof(*value)); }
int btstack_core_state_get_peer_count(uint8_t *count)
{ (void)count; return -1; }
int btstack_core_state_get_peer(uint8_t index,
    bt_xstack_paired_device_t *peer)
{ (void)index; (void)peer; return -1; }

static void service_callback(const bt_xstack_service_event_t *event,
    void *context)
{ (void)event; (void)context; service_callback_count++; }
static void media_callback(const uint8_t *data, uint32_t length,
    void *context)
{ (void)data; (void)length; (void)context; audio_callback_count++; }

typedef struct {
    btstack_worker_id_t id;
} invocation_t;

static DWORD WINAPI worker_thread(void *context)
{
    invocation_t *invocation = context;
    btstack_worker_id_t id = invocation->id;

    entry_active[id] = 1;
    fixture.entries[id](fixture.arguments[id]);
    return 1u;
}

static void invoke_worker(btstack_worker_id_t id)
{
    invocation_t invocation;
    HANDLE thread;
    DWORD exit_code;

    assert(fixture.entries[id] != NULL);
    invocation.id = id;
    thread = CreateThread(NULL, 0u, worker_thread, &invocation, 0u, NULL);
    assert(thread != NULL);
    assert(WaitForSingleObject(thread, 5000u) == WAIT_OBJECT_0);
    assert(GetExitCodeThread(thread, &exit_code));
    assert(exit_code == 0u);
    CloseHandle(thread);
    entry_active[id] = 0;
}

static bt_xstack_config_t make_config(void)
{
    bt_xstack_config_t config;
    memset(&config, 0, sizeof(config));
    config.service_callback = service_callback;
    config.media_audio_callback = media_callback;
    return config;
}

static void run_scheduling_test(void)
{
    static const bt_xstack_addr_t address = {{1u,2u,3u,4u,5u,6u}};
    btstack_core_event_envelope_t event;
    bt_xstack_config_t config = make_config();
    uint8_t audio = 0x5au;

    fixture.fail_start_id = -1;
    assert(bt_xstack_init(&config) == BT_XSTACK_OK);
    assert(fixture.start_count == 2u);
    assert(fixture.start_order[0] == BTSTACK_WORKER_API);
    assert(fixture.start_order[1] == BTSTACK_WORKER_EVENT);
    assert(fixture.entries[BTSTACK_WORKER_STACK] == NULL);
    assert(bt_xstack_phonebook_sync(&address,
        BT_XSTACK_PHONEBOOK_CONTACTS) == BT_XSTACK_OK);
    assert(backend_count == 0u);
    assert((fixture.notified[BTSTACK_WORKER_API] &
        BTSTACK_WORKER_SIGNAL_WORK) != 0u);

    memset(&event, 0, sizeof(event));
    event.domain = BT_XSTACK_EVENT_DOMAIN_SERVICE;
    event.data.service.id = BT_XSTACK_SERVICE_EVENT_POWER_STATE;
    assert(btstack_core_post_event(&event) == 0);
    assert(service_callback_count == 0u);
    invoke_worker(BTSTACK_WORKER_API);
    assert(backend_count == 1u);
    assert(service_callback_count == 0u);
    invoke_worker(BTSTACK_WORKER_EVENT);
    assert(service_callback_count == 1u);

    btstack_core_media_audio_deliver(&audio, 1u);
    assert(audio_callback_count == 1u);
    assert(fixture.critical_depth == 0);
    assert(fixture.critical_violation == 0);
}

static void run_reliable_space_test(void)
{
    static const bt_xstack_addr_t address = {{1u,2u,3u,4u,5u,6u}};
    btstack_core_event_envelope_t event;
    bt_xstack_config_t config = make_config();
    unsigned int i;

    fixture.fail_start_id = -1;
    assert(bt_xstack_init(&config) == BT_XSTACK_OK);
    for (i = 0u; i < BT_XSTACK_REQUEST_QUEUE_LEN; i++) {
        assert(bt_xstack_phonebook_sync(&address,
            BT_XSTACK_PHONEBOOK_CONTACTS) == BT_XSTACK_OK);
    }
    memset(&event, 0, sizeof(event));
    event.domain = BT_XSTACK_EVENT_DOMAIN_PHONE;
    event.data.phone.id = BT_XSTACK_PHONE_EVENT_LOST;
    event.data.phone.address = address;
    assert(btstack_core_post_event(&event) == 0);
    fixture.release_request_space = 1;
    invoke_worker(BTSTACK_WORKER_EVENT);
    assert(fixture.wait_count != 0u);
    assert(backend_count >= BT_XSTACK_REQUEST_QUEUE_LEN);
    invoke_worker(BTSTACK_WORKER_API);
    assert(backend_count == BT_XSTACK_REQUEST_QUEUE_LEN + 2u);
    assert(fixture.critical_violation == 0);
}

static void run_partial_start_failure_test(void)
{
    bt_xstack_config_t config = make_config();
    fixture.fail_start_id = BTSTACK_WORKER_EVENT;
    assert(bt_xstack_init(&config) == BT_XSTACK_INTERNAL_ERROR);
    assert(fixture.start_count == 2u);
    assert(fixture.stop_count == 1u);
    assert(fixture.stop_order[0] == BTSTACK_WORKER_API);
    assert(!bt_xstack_is_ready());
    assert(runtime_init_count == 1u);
    fixture.fail_start_id = -1;
    assert(bt_xstack_init(&config) == BT_XSTACK_OK);
    assert(fixture.start_count == 4u);
    assert(fixture.start_order[2] == BTSTACK_WORKER_API);
    assert(fixture.start_order[3] == BTSTACK_WORKER_EVENT);
    assert(bt_xstack_is_ready());
    assert(runtime_init_count == 1u);
}

static void run_api_start_failure_test(void)
{
    bt_xstack_config_t config = make_config();
    fixture.fail_start_id = BTSTACK_WORKER_API;
    assert(bt_xstack_init(&config) == BT_XSTACK_INTERNAL_ERROR);
    assert(fixture.start_count == 1u);
    assert(fixture.stop_count == 0u);
    assert(!bt_xstack_is_ready());
    fixture.fail_start_id = -1;
    assert(bt_xstack_init(&config) == BT_XSTACK_OK);
    assert(fixture.start_count == 3u);
    assert(bt_xstack_is_ready());
    assert(runtime_init_count == 1u);
}

static void run_enable_reentrant_test(void)
{
    bt_xstack_config_t config = make_config();
    fixture.fail_start_id = -1;
    runtime_started = 0;
    runtime_reenter_enable = 1;
    assert(bt_xstack_init(&config) == BT_XSTACK_OK);
    assert(bt_xstack_enable() == BT_XSTACK_OK);
    assert(runtime_reentrant_enable_result == BT_XSTACK_NOT_READY);
    assert(bt_xstack_is_ready());
    assert(backend_count == 0u);
    invoke_worker(BTSTACK_WORKER_API);
    assert(backend_count == 1u);
    assert(bt_xstack_enable() == BT_XSTACK_OK);
    invoke_worker(BTSTACK_WORKER_API);
    assert(backend_count == 2u);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    memset(&fixture, 0, sizeof(fixture));
    fixture.fail_start_id = -1;
    if (strcmp(argv[1], "scheduling") == 0) run_scheduling_test();
    else if (strcmp(argv[1], "reliable-space") == 0)
        run_reliable_space_test();
    else if (strcmp(argv[1], "partial-start") == 0)
        run_partial_start_failure_test();
    else if (strcmp(argv[1], "api-start") == 0)
        run_api_start_failure_test();
    else if (strcmp(argv[1], "enable-reentrant") == 0)
        run_enable_reentrant_test();
    else return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

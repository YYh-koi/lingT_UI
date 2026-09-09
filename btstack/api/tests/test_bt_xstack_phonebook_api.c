#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

#include "bt_xstack.h"
#include "../../core/btstack_core_internal.h"
#include "../../core/btstack_runtime.h"
#include "FreeRTOS.h"
#include "task.h"
#include "btstack_run_loop_gpcv2178.h"
#include "port/btstack_platform.h"
#include "core/btstack_profile_manager.h"

bool btstack_profile_manager_hfp_event_delivered(
    uint8_t event_type, const uint8_t address[6])
{ (void)event_type; return address != NULL; }

typedef struct {
    void (*entry)(void *);
    void *context;
    HANDLE semaphore;
    HANDLE thread;
    DWORD thread_id;
} test_task_t;

static CRITICAL_SECTION platform_lock;
static __thread unsigned int critical_depth;
static test_task_t test_tasks[2];
static unsigned int test_task_count;
static volatile LONG api_pop_event_notify_count;
static volatile LONG sequence_number;
static volatile LONG phone_callback_count;
static volatile LONG lost_callback_count;
static volatile LONG completed_callback_count;
static volatile LONG scan_callback_count;
static volatile LONG completed_sequence;
static volatile LONG first_scan_sequence;
static uint32_t signal_values[BT_XSTACK_CRITICAL_EVENT_QUEUE_LEN];
static uint32_t scan_values[BT_XSTACK_BEST_EFFORT_EVENT_QUEUE_LEN];
static volatile LONG signal_value_count;
static volatile LONG backend_request_count;
static volatile LONG le_callback_count;
static volatile LONG gatt_callback_count;
static volatile LONG peer_disconnected_count;
static btstack_core_request_t backend_requests[64];
static int reenter_init_enabled;
static bt_xstack_result_t reenter_same_result;
static bt_xstack_result_t reenter_different_result;
static bt_xstack_config_t reenter_config;
static unsigned int cover_art_enabled_calls;
static bool cover_art_enabled;
static unsigned int cover_art_callback_count;
static const uint8_t *cover_art_data_pointer;
static uint16_t cover_art_data_length;
static void *cover_art_context;

static LONG next_sequence(void)
{
    return InterlockedIncrement(&sequence_number);
}

static int wait_for_at_least(volatile LONG *value, LONG expected)
{
    unsigned int retry;

    for (retry = 0u; retry < 5000u; retry++) {
        if (InterlockedCompareExchange(value, 0, 0) >= expected) {
            return 1;
        }
        Sleep(1u);
    }
    return 0;
}

void test_task_enter_critical(void)
{
    EnterCriticalSection(&platform_lock);
    critical_depth++;
}

void test_task_exit_critical(void)
{
    assert(critical_depth != 0u);
    critical_depth--;
    LeaveCriticalSection(&platform_lock);
}

void btstack_platform_enter_critical(void)
{
    test_task_enter_critical();
}

void btstack_platform_exit_critical(void)
{
    test_task_exit_critical();
}

const btstack_platform_t *btstack_platform_get(void)
{
    static const btstack_platform_t platform = {0};

    return &platform;
}

const btstack_platform_t *btstack_platform_default(void)
{
    return btstack_platform_get();
}

btstack_platform_status_t btstack_platform_activate(
    const btstack_platform_t *platform)
{
    (void)platform;
    return BTSTACK_PLATFORM_OK;
}

int btstack_platform_worker_validate(
    btstack_worker_id_t id,
    const btstack_worker_config_t *config)
{
    if ((config == NULL) || (id >= BTSTACK_WORKER_COUNT)) return -1;
    if ((id == BTSTACK_WORKER_API) &&
        (config->stack_words != BT_XSTACK_API_TASK_STACK_WORDS)) return -1;
    if ((id == BTSTACK_WORKER_EVENT) &&
        (config->stack_words != BT_XSTACK_EVENT_TASK_STACK_WORDS)) return -1;
    return 0;
}

int btstack_platform_worker_start(
    btstack_worker_id_t id,
    btstack_worker_entry_t entry,
    void *argument,
    const btstack_worker_config_t *config)
{
    assert(id < BTSTACK_WORKER_STACK);
    assert(btstack_platform_worker_validate(id, config) == 0);
    return test_xTaskCreateStatic(entry,
        (id == BTSTACK_WORKER_API) ? "api" : "event",
        config->stack_words, argument, config->priority,
        NULL, NULL) != NULL ? 0 : -1;
}

int btstack_platform_worker_stop(btstack_worker_id_t id)
{
    (void)id;
    return 0;
}

int btstack_platform_notify(btstack_worker_id_t id, uint32_t signals)
{
    (void)signals;
    assert(id < test_task_count);
    return (xTaskNotifyGive(&test_tasks[id]) == pdPASS) ? 0 : -1;
}

uint32_t btstack_platform_wait(
    btstack_worker_id_t id,
    uint32_t mask,
    uint32_t timeout_ms)
{
    (void)id;
    assert(timeout_ms == BTSTACK_WORKER_WAIT_FOREVER);
    (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    return mask;
}

TaskHandle_t test_xTaskCreateStatic(
    void (*entry)(void *),
    const char *name,
    uint16_t stack_words,
    void *context,
    UBaseType_t priority,
    StackType_t *stack,
    StaticTask_t *control)
{
    test_task_t *task;

    (void)name;
    (void)stack_words;
    (void)priority;
    (void)stack;
    (void)control;
    assert(test_task_count < 2u);
    task = &test_tasks[test_task_count++];
    memset(task, 0, sizeof(*task));
    task->entry = entry;
    task->context = context;
    task->semaphore = CreateSemaphore(NULL, 0, 0x7fffffff, NULL);
    assert(task->semaphore != NULL);
    return task;
}

static DWORD WINAPI test_task_thread(void *context)
{
    test_task_t *task = context;

    task->thread_id = GetCurrentThreadId();
    task->entry(task->context);
    return 0u;
}

static void start_task(unsigned int index)
{
    test_task_t *task = &test_tasks[index];

    assert(task->thread == NULL);
    task->thread = CreateThread(
        NULL, 0u, test_task_thread, task, 0u, NULL);
    assert(task->thread != NULL);
}

uint32_t ulTaskNotifyTake(BaseType_t clear_on_exit, TickType_t ticks_to_wait)
{
    DWORD current = GetCurrentThreadId();
    unsigned int index;
    test_task_t *task = NULL;
    DWORD wait_result;

    assert(ticks_to_wait == portMAX_DELAY);
    for (index = 0u; index < test_task_count; index++) {
        if (test_tasks[index].thread_id == current) {
            task = &test_tasks[index];
            break;
        }
    }
    assert(task != NULL);
    wait_result = WaitForSingleObject(task->semaphore, INFINITE);
    assert(wait_result == WAIT_OBJECT_0);
    if (clear_on_exit == pdTRUE) {
        while (WaitForSingleObject(task->semaphore, 0u) == WAIT_OBJECT_0) {
        }
    }
    return 1u;
}

BaseType_t xTaskNotifyGive(TaskHandle_t handle)
{
    test_task_t *task = handle;

    assert(task != NULL);
    assert(critical_depth == 0u);
    if ((test_task_count == 2u) &&
        (GetCurrentThreadId() == test_tasks[0].thread_id) &&
        (task == &test_tasks[1])) {
        InterlockedIncrement(&api_pop_event_notify_count);
    }
    assert(ReleaseSemaphore(task->semaphore, 1, NULL));
    return pdPASS;
}

int btstack_runtime_init(void)
{
    if (reenter_init_enabled) {
        bt_xstack_config_t different = reenter_config;

        reenter_init_enabled = 0;
        reenter_same_result = bt_xstack_init(&reenter_config);
        different.context = (void *)(uintptr_t)1u;
        reenter_different_result = bt_xstack_init(&different);
    }
    return 0;
}

int btstack_runtime_start(void)
{
    return 0;
}

int btstack_runtime_started(void)
{
    return 1;
}

void btstack_avrcp_cover_art_set_callback_enabled(bool enabled)
{
    assert(critical_depth == 0u);
    cover_art_enabled_calls++;
    cover_art_enabled = enabled;
}

void btstack_runtime_get_diagnostics(
    btstack_runtime_diagnostics_t *diagnostics)
{
    if (diagnostics != NULL) {
        memset(diagnostics, 0, sizeof(*diagnostics));
    }
}

int btstack_core_dispatch(const btstack_core_request_t *request)
{
    LONG index = InterlockedIncrement(&backend_request_count) - 1;

    assert(index >= 0);
    assert(index < (LONG)(sizeof(backend_requests) /
        sizeof(backend_requests[0])));
    memcpy(&backend_requests[index], request, sizeof(*request));
    if (request->id == BT_XSTACK_REQUEST_PBAP_PEER_DISCONNECTED) {
        InterlockedIncrement(&peer_disconnected_count);
    }
    if (request->id == BT_XSTACK_REQUEST_PBAP_EVENT_SPACE_AVAILABLE) {
        btstack_core_pbap_event_space_request_completed();
    }
    return 0;
}

int btstack_core_copy_state(bt_xstack_state_t *state)
{
    (void)state;
    return -1;
}

int btstack_core_state_read_service(
    btstack_core_service_snapshot_t *snapshot)
{
    (void)snapshot;
    return -1;
}

int btstack_core_state_read_hfp(btstack_core_hfp_snapshot_t *snapshot)
{
    (void)snapshot;
    return -1;
}

int btstack_core_state_read_a2dp(btstack_core_a2dp_snapshot_t *snapshot)
{
    (void)snapshot;
    return -1;
}

int btstack_core_state_read_avrcp(btstack_core_avrcp_snapshot_t *snapshot)
{
    (void)snapshot;
    return -1;
}

int btstack_core_state_read_pbap(bt_xstack_phonebook_state_t *state)
{
    (void)state;
    return -1;
}

void btstack_core_dispatcher_get_diagnostics(
    btstack_core_backend_diagnostics_t *diagnostics)
{
    memset(diagnostics, 0, sizeof(*diagnostics));
}

int btstack_core_state_get_peer_count(uint8_t *count)
{
    (void)count;
    return -1;
}

int btstack_core_state_get_peer(
    uint8_t index,
    bt_xstack_paired_device_t *peer)
{
    (void)index;
    (void)peer;
    return -1;
}

static void service_callback(
    const bt_xstack_service_event_t *event,
    void *context)
{
    LONG sequence = next_sequence();

    (void)context;
    if (event->id == BT_XSTACK_SERVICE_EVENT_SCAN_RESULT) {
        LONG index = InterlockedCompareExchange(
            &scan_callback_count, 0, 0);

        assert(index < (LONG)BT_XSTACK_BEST_EFFORT_EVENT_QUEUE_LEN);
        scan_values[index] = event->values[0];
        if (InterlockedIncrement(&scan_callback_count) == 1) {
            InterlockedExchange(&first_scan_sequence, sequence);
        }
    }
}

static void phone_callback(
    const bt_xstack_phone_event_t *event,
    void *context)
{
    (void)context;
    (void)next_sequence();
    InterlockedIncrement(&phone_callback_count);
    if (event->id == BT_XSTACK_PHONE_EVENT_SIGNAL) {
        LONG index = signal_value_count;

        assert(index < (LONG)BT_XSTACK_CRITICAL_EVENT_QUEUE_LEN);
        signal_values[index] = event->values[0];
        InterlockedIncrement(&signal_value_count);
    }
    if (event->id == BT_XSTACK_PHONE_EVENT_LOST) {
        InterlockedIncrement(&lost_callback_count);
    }
}

static void phonebook_callback(
    const bt_xstack_phonebook_event_t *event,
    void *context)
{
    LONG sequence = next_sequence();

    (void)context;
    if (event->id == BT_XSTACK_PHONEBOOK_EVENT_COMPLETED) {
        InterlockedIncrement(&completed_callback_count);
        InterlockedExchange(&completed_sequence, sequence);
    }
}

static void le_callback(const bt_xstack_le_event_t *event, void *context)
{
    (void)context;
    assert(event->type == BT_XSTACK_LE_EVENT_CONNECTED);
    InterlockedIncrement(&le_callback_count);
}

static void gatt_callback(const bt_xstack_gatt_event_t *event, void *context)
{
    (void)context;
    assert(event->type == BT_XSTACK_GATT_EVENT_NOTIFICATION);
    InterlockedIncrement(&gatt_callback_count);
}

static void cover_art_callback(const bt_xstack_cover_art_event_t *event, void *context)
{
    assert(event != NULL);
    assert(critical_depth == 0u);
    cover_art_callback_count++;
    cover_art_context = context;
    if (event->type == BT_XSTACK_COVER_ART_DATA) {
        cover_art_data_pointer = event->data.data.data;
        cover_art_data_length = event->data.data.length;
    }
}

static void assert_request_ring_full(const bt_xstack_addr_t *address)
{
    unsigned int index;
    bt_xstack_cover_art_image_descriptor_t descriptor = {
        BT_XSTACK_COVER_ART_ENCODING_JPEG, 64u, 64u,
        BT_XSTACK_COVER_ART_TRANSFORMATION_NONE
    };

    for (index = 0u; index < BT_XSTACK_REQUEST_QUEUE_LEN; index++) {
        assert(bt_xstack_phonebook_sync(
            address, BT_XSTACK_PHONEBOOK_CONTACTS) == BT_XSTACK_OK);
    }
    assert(bt_xstack_phonebook_sync(
        address, BT_XSTACK_PHONEBOOK_CONTACTS) == BT_XSTACK_QUEUE_FULL);
    assert(bt_xstack_cover_art_get_image(&descriptor) == BT_XSTACK_QUEUE_FULL);
}

static void assert_cover_art_api(void *context)
{
    static const uint8_t bytes[] = { 9u, 8u, 7u };
    bt_xstack_cover_art_event_t event;
    bt_xstack_cover_art_image_descriptor_t descriptor;
    LONG before = InterlockedCompareExchange(&backend_request_count, 0, 0);

    memset(&descriptor, 0, sizeof(descriptor));
    assert(bt_xstack_cover_art_set_mode((bt_xstack_cover_art_mode_t)99) == BT_XSTACK_INVALID_ARGUMENT);
    assert(bt_xstack_cover_art_get_image(NULL) == BT_XSTACK_INVALID_ARGUMENT);
    assert(bt_xstack_cover_art_get_image(&descriptor) == BT_XSTACK_INVALID_ARGUMENT);
    assert(InterlockedCompareExchange(&backend_request_count, 0, 0) == before);

    memset(&event, 0, sizeof(event));
    event.type = BT_XSTACK_COVER_ART_DATA;
    event.data.data.data = bytes;
    event.data.data.length = sizeof(bytes);
    btstack_frontend_deliver_cover_art(&event);
    assert(cover_art_callback_count == 1u);
    assert(cover_art_context == context);
    assert(cover_art_data_pointer == bytes && cover_art_data_length == sizeof(bytes));
}

static void assert_cover_art_requests(void)
{
    bt_xstack_cover_art_image_descriptor_t descriptor = {
        BT_XSTACK_COVER_ART_ENCODING_PNG, 320u, 240u,
        BT_XSTACK_COVER_ART_TRANSFORMATION_CROP
    };
    LONG before = InterlockedCompareExchange(&backend_request_count, 0, 0);

    assert(bt_xstack_cover_art_set_mode(BT_XSTACK_COVER_ART_MODE_IMAGE) == BT_XSTACK_OK);
    assert(bt_xstack_cover_art_get_image(&descriptor) == BT_XSTACK_OK);
    assert(wait_for_at_least(&backend_request_count, before + 2));
    assert(backend_requests[before].id == BT_XSTACK_REQUEST_COVER_ART_SET_MODE);
    assert(backend_requests[before].cover_art.mode == BT_XSTACK_COVER_ART_MODE_IMAGE);
    assert(backend_requests[before + 1].id == BT_XSTACK_REQUEST_COVER_ART_GET_IMAGE);
    assert(backend_requests[before + 1].cover_art.descriptor.encoding == BT_XSTACK_COVER_ART_ENCODING_PNG);
    assert(backend_requests[before + 1].cover_art.descriptor.width == 320u);
    assert(backend_requests[before + 1].cover_art.descriptor.transformation == BT_XSTACK_COVER_ART_TRANSFORMATION_CROP);
}

static void post_ordinary_phone_event(uint32_t value)
{
    btstack_core_event_envelope_t event;

    memset(&event, 0, sizeof(event));
    event.domain = BT_XSTACK_EVENT_DOMAIN_PHONE;
    event.data.phone.id = BT_XSTACK_PHONE_EVENT_SIGNAL;
    event.data.phone.values[0] = value;
    event.data.phone.value_count = 1u;
    assert(btstack_core_post_event(&event) == 0);
}

static void assert_event_ring_admission(void)
{
    btstack_core_event_envelope_t event;
    bt_xstack_diagnostics_t diagnostics;
    unsigned int index;

    memset(&event, 0, sizeof(event));
    event.domain = BT_XSTACK_EVENT_DOMAIN_SERVICE;
    event.data.service.id = BT_XSTACK_SERVICE_EVENT_SCAN_RESULT;
    for (index = 0u;
         index < BT_XSTACK_BEST_EFFORT_EVENT_QUEUE_LEN;
         index++) {
        event.data.service.values[0] = index;
        event.data.service.value_count = 1u;
        assert(btstack_core_post_event(&event) == 0);
    }
    assert(btstack_core_post_event(&event) == -1);

    for (index = 0u;
         index < (BT_XSTACK_CRITICAL_EVENT_QUEUE_LEN -
            BT_XSTACK_PBAP_CONTROL_EVENT_RESERVE);
         index++) {
        post_ordinary_phone_event(index);
    }

    memset(&event, 0, sizeof(event));
    event.domain = BT_XSTACK_EVENT_DOMAIN_PHONEBOOK;
    event.data.phonebook.id = BT_XSTACK_PHONEBOOK_EVENT_CONTACT;
    assert(btstack_core_post_event(&event) ==
        BT_XSTACK_API_EVENT_BACKPRESSURE);

    memset(&event, 0, sizeof(event));
    event.domain = BT_XSTACK_EVENT_DOMAIN_PHONE;
    event.data.phone.id = BT_XSTACK_PHONE_EVENT_SIGNAL;
    assert(btstack_core_post_event(&event) == -1);

    event.data.phone.id = BT_XSTACK_PHONE_EVENT_STATE;
    assert(btstack_core_post_event(&event) == 0);
    event.data.phone.id = BT_XSTACK_PHONE_EVENT_LOST;
    event.data.phone.address.bytes[0] = 0xa5u;
    assert(btstack_core_post_event(&event) == 0);

    memset(&event, 0, sizeof(event));
    event.domain = BT_XSTACK_EVENT_DOMAIN_SERVICE;
    event.data.service.id = BT_XSTACK_SERVICE_EVENT_POWER_STATE;
    assert(btstack_core_post_event(&event) == 0);

    memset(&event, 0, sizeof(event));
    event.domain = BT_XSTACK_EVENT_DOMAIN_PHONEBOOK;
    event.data.phonebook.id = BT_XSTACK_PHONEBOOK_EVENT_COMPLETED;
    assert(btstack_core_post_event(&event) == 0);
    event.data.phonebook.id = BT_XSTACK_PHONEBOOK_EVENT_ERROR;
    assert(btstack_core_post_event(&event) == -1);

    assert(bt_xstack_get_diagnostics(&diagnostics) == BT_XSTACK_OK);
    assert(diagnostics.critical_event_drops == 2u);
    assert(diagnostics.best_effort_event_drops == 1u);
}

static void assert_workers_drain_real_rings(void)
{
    LONG backend_index;
    unsigned int index;

    start_task(1u);
    assert(wait_for_at_least(&phone_callback_count, 1));
    assert(InterlockedCompareExchange(&backend_request_count, 0, 0) == 0);

    start_task(0u);
    assert(wait_for_at_least(&completed_callback_count, 1));
    assert(wait_for_at_least(
        &scan_callback_count,
        BT_XSTACK_BEST_EFFORT_EVENT_QUEUE_LEN));
    assert(wait_for_at_least(&peer_disconnected_count, 1));
    assert(wait_for_at_least(&api_pop_event_notify_count, 1));

    assert(lost_callback_count == 1);
    assert(first_scan_sequence > completed_sequence);
    assert(phone_callback_count ==
        (LONG)(BT_XSTACK_CRITICAL_EVENT_QUEUE_LEN -
            BT_XSTACK_PBAP_CONTROL_EVENT_RESERVE + 2u));
    assert(signal_value_count ==
        (LONG)(BT_XSTACK_CRITICAL_EVENT_QUEUE_LEN -
            BT_XSTACK_PBAP_CONTROL_EVENT_RESERVE));
    for (index = 0u;
         index < (BT_XSTACK_CRITICAL_EVENT_QUEUE_LEN -
            BT_XSTACK_PBAP_CONTROL_EVENT_RESERVE);
         index++) {
        assert(signal_values[index] == index);
    }
    for (index = 0u;
         index < BT_XSTACK_BEST_EFFORT_EVENT_QUEUE_LEN;
         index++) {
        assert(scan_values[index] == index);
    }

    backend_index = InterlockedCompareExchange(
        &backend_request_count, 0, 0);
    assert(backend_index > (LONG)BT_XSTACK_REQUEST_QUEUE_LEN);
    for (index = 0u; index < BT_XSTACK_REQUEST_QUEUE_LEN; index++) {
        assert(backend_requests[index].id == BT_XSTACK_REQUEST_PBAP_SYNC);
        if (index != 0u) {
            assert(backend_requests[index].generation ==
                backend_requests[index - 1u].generation + 1u);
        }
    }
    for (index = 0u; index < (unsigned int)backend_index; index++) {
        if (backend_requests[index].id ==
                BT_XSTACK_REQUEST_PBAP_PEER_DISCONNECTED) {
            assert(backend_requests[index].address.bytes[0] == 0xa5u);
            return;
        }
    }
    assert(0 && "PBAP peer-disconnected request was not dispatched");
}

static void assert_ble_api_requests_and_events(void)
{
    bt_xstack_le_adv_config_t adv;
    bt_xstack_gatt_attribute_config_t attribute;
    btstack_core_event_envelope_t event;
    LONG before = InterlockedCompareExchange(&backend_request_count, 0, 0);
    static const uint8_t value[] = { 1u, 2u, 3u };

    memset(&adv, 0, sizeof(adv));
    adv.interval_min = 160u;
    adv.interval_max = 240u;
    adv.channel_map = 7u;
    adv.data_len = 2u;
    adv.data[0] = 1u;
    adv.data[1] = 2u;
    assert(bt_xstack_le_adv_configure(NULL) == BT_XSTACK_INVALID_ARGUMENT);
    assert(bt_xstack_le_adv_configure(&adv) == BT_XSTACK_OK);

    memset(&attribute, 0, sizeof(attribute));
    attribute.uuid.type = BT_XSTACK_UUID_TYPE_16;
    attribute.uuid.value.uuid16 = 0x2a19u;
    attribute.properties = BT_XSTACK_GATT_PROPERTY_READ;
    attribute.max_len = 20u;
    assert(bt_xstack_gatt_server_add_characteristic(1u, &attribute) ==
        BT_XSTACK_OK);
    assert(bt_xstack_gatt_server_set_value(3u, value, sizeof(value)) ==
        BT_XSTACK_OK);
    assert(bt_xstack_gatt_server_get_value(3u) == BT_XSTACK_OK);

    assert(btstack_platform_notify(BTSTACK_WORKER_API,
        BTSTACK_WORKER_SIGNAL_WORK) == 0);
    assert(wait_for_at_least(&backend_request_count, before + 4));
    assert(backend_requests[before].id == BT_XSTACK_REQUEST_LE_ADV_CONFIGURE);
    assert(backend_requests[before].le_adv.data[1] == 2u);
    assert(backend_requests[before + 1].id ==
        BT_XSTACK_REQUEST_GATT_SERVER_ADD_CHARACTERISTIC);
    assert(backend_requests[before + 1].gatt_registration.config.uuid.value.uuid16 ==
        0x2a19u);
    assert(backend_requests[before + 2].id ==
        BT_XSTACK_REQUEST_GATT_SERVER_SET_VALUE);
    assert(backend_requests[before + 2].gatt.value[2] == 3u);
    assert(backend_requests[before + 3].id ==
        BT_XSTACK_REQUEST_GATT_SERVER_GET_VALUE);
    assert(backend_requests[before + 3].gatt.handle == 3u);

    memset(&event, 0, sizeof(event));
    event.domain = BT_XSTACK_EVENT_DOMAIN_LE;
    event.data.le.type = BT_XSTACK_LE_EVENT_CONNECTED;
    assert(btstack_core_post_event(&event) == 0);
    memset(&event, 0, sizeof(event));
    event.domain = BT_XSTACK_EVENT_DOMAIN_GATT;
    event.data.gatt.type = BT_XSTACK_GATT_EVENT_NOTIFICATION;
    assert(btstack_core_post_event(&event) == 0);
    assert(btstack_platform_notify(BTSTACK_WORKER_EVENT,
        BTSTACK_WORKER_SIGNAL_WORK) == 0);
    assert(wait_for_at_least(&le_callback_count, 1));
    assert(wait_for_at_least(&gatt_callback_count, 1));
}

int main(int argc, char **argv)
{
    static const bt_xstack_addr_t address = {
        {0x10u, 0x20u, 0x30u, 0x40u, 0x50u, 0x60u}
    };
    bt_xstack_config_t config;
    int callback_context;
    int with_cover_callback = argc == 1;

    (void)argv;

    InitializeCriticalSection(&platform_lock);
    memset(&config, 0, sizeof(config));
    config.service_callback = service_callback;
    config.phone_callback = phone_callback;
    config.phonebook_callback = phonebook_callback;
    config.le_callback = le_callback;
    config.gatt_callback = gatt_callback;
    config.cover_art_callback = with_cover_callback ? cover_art_callback : NULL;
    config.context = &callback_context;
    reenter_config = config;
    reenter_init_enabled = 1;

    assert(bt_xstack_init(&config) == BT_XSTACK_OK);
    assert(reenter_same_result == BT_XSTACK_NOT_READY);
    assert(reenter_different_result == BT_XSTACK_CONFIGURATION_CONFLICT);
    assert(bt_xstack_init(&config) == BT_XSTACK_OK);
    assert(test_task_count == 2u);
    assert(cover_art_enabled_calls == 1u && cover_art_enabled == (with_cover_callback != 0));
    if (!with_cover_callback) {
        bt_xstack_cover_art_event_t event;

        memset(&event, 0, sizeof(event));
        event.type = BT_XSTACK_COVER_ART_STARTED;
        btstack_frontend_deliver_cover_art(&event);
        assert(cover_art_callback_count == 0u);
        assert(bt_xstack_cover_art_register_callback(
            cover_art_callback, &callback_context) == BT_XSTACK_OK);
        assert(cover_art_enabled_calls == 2u && cover_art_enabled == 1);
        btstack_frontend_deliver_cover_art(&event);
        assert(cover_art_callback_count == 1u);
        assert(bt_xstack_cover_art_register_callback(NULL, NULL) == BT_XSTACK_OK);
        assert(cover_art_enabled_calls == 3u && cover_art_enabled == 0);
        return 0;
    }
    assert_cover_art_api(&callback_context);

    assert_request_ring_full(&address);
    assert_event_ring_admission();
    assert_workers_drain_real_rings();
    assert_cover_art_requests();
    assert_ble_api_requests_and_events();
    assert(critical_depth == 0u);
    return 0;
}

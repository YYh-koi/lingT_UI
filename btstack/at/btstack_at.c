#include "btstack_at_internal.h"

#include "../core/btstack_ring.h"
#include "port/btstack_platform.h"

#ifndef BTSTACK_AT_TASK_STACK_WORDS
#define BTSTACK_AT_TASK_STACK_WORDS 2048u
#endif

#ifndef BTSTACK_AT_TASK_PRIORITY
#define BTSTACK_AT_TASK_PRIORITY 2u
#endif

#ifndef BTSTACK_AT_FIRMWARE_VERSION
#define BTSTACK_AT_FIRMWARE_VERSION "GPCV2178_BTSTACK_1.0"
#endif

#ifndef BTSTACK_AT_STATE_EVENT_CACHE_MAX
#define BTSTACK_AT_STATE_EVENT_CACHE_MAX 128u
#endif

typedef struct {
    uint16_t length;
    char data[BTSTACK_AT_COMMAND_MAX];
} btstack_at_input_item_t;

static btstack_at_config_t btstack_at_config;
static int btstack_at_initialized;
static int btstack_at_started;
static btstack_at_backend_dispatch_t btstack_at_backend_dispatch;
static btstack_at_command_gate_t btstack_at_command_gate;
static void *btstack_at_command_gate_context;

static btstack_ring_t btstack_at_input_ring;
static btstack_ring_t btstack_at_completion_ring;
static btstack_ring_t btstack_at_event_ring;
static btstack_at_input_item_t
    btstack_at_input_ring_storage[BTSTACK_AT_INPUT_QUEUE_LEN];
static btstack_at_result_t btstack_at_completion_ring_storage[1u];
static btstack_at_event_t
    btstack_at_event_ring_storage[BTSTACK_AT_EVENT_QUEUE_LEN];
static char btstack_at_output[BTSTACK_AT_OUTPUT_MAX];
static uint32_t btstack_at_generation;
static uint32_t btstack_at_pending_generation;
static btstack_at_command_id_t btstack_at_pending_command;
static int btstack_at_pending_active;
static volatile uint32_t btstack_at_event_overflow_count;
static uint8_t btstack_at_last_state_event_valid[
    BTSTACK_AT_EVENT_ABSVOL + 1u];
static uint16_t btstack_at_last_state_event_length[
    BTSTACK_AT_EVENT_ABSVOL + 1u];
static char btstack_at_last_state_event[
    BTSTACK_AT_EVENT_ABSVOL + 1u][BTSTACK_AT_STATE_EVENT_CACHE_MAX];

static void btstack_at_emit(uint16_t length)
{
    if ((length != 0u) && (btstack_at_config.output_callback != NULL)) {
        btstack_at_config.output_callback(
            btstack_at_output,
            length,
            btstack_at_config.output_context);
    }
}

static void btstack_at_emit_simple(int success)
{
    uint16_t length = 0u;
    if (btstack_at_format_simple(
            success,
            btstack_at_output,
            sizeof(btstack_at_output),
            &length) == 0) {
        btstack_at_emit(length);
    }
}

static int btstack_at_event_is_state(btstack_at_event_id_t id)
{
    switch (id) {
    case BTSTACK_AT_EVENT_DEVSTAT:
    case BTSTACK_AT_EVENT_PWRSTAT:
    case BTSTACK_AT_EVENT_MICMUTED:
    case BTSTACK_AT_EVENT_HFPSTAT:
    case BTSTACK_AT_EVENT_HFPAUDIO:
    case BTSTACK_AT_EVENT_HFPVR:
    case BTSTACK_AT_EVENT_HFPSIG:
    case BTSTACK_AT_EVENT_HFPNET:
    case BTSTACK_AT_EVENT_HFPROAM:
    case BTSTACK_AT_EVENT_HFPBATT:
    case BTSTACK_AT_EVENT_HFPMANU:
    case BTSTACK_AT_EVENT_HFPNUM:
    case BTSTACK_AT_EVENT_HFPIBR:
    case BTSTACK_AT_EVENT_A2DPSTAT:
    case BTSTACK_AT_EVENT_AVRCPSTAT:
    case BTSTACK_AT_EVENT_PLAYSTAT:
    case BTSTACK_AT_EVENT_PLAYMODE:
    case BTSTACK_AT_EVENT_TRACKSTAT:
    case BTSTACK_AT_EVENT_ABSVOL:
        return 1;
    default:
        return 0;
    }
}

static int btstack_at_state_event_is_duplicate(
    btstack_at_event_id_t id,
    const char *data,
    uint16_t length)
{
    uint16_t index;

    if (!btstack_at_event_is_state(id) ||
        (length > BTSTACK_AT_STATE_EVENT_CACHE_MAX)) {
        return 0;
    }
    if (btstack_at_last_state_event_valid[id] &&
        (btstack_at_last_state_event_length[id] == length)) {
        for (index = 0u; index < length; index++) {
            if (btstack_at_last_state_event[id][index] != data[index]) {
                break;
            }
        }
        if (index == length) {
            return 1;
        }
    }
    for (index = 0u; index < length; index++) {
        btstack_at_last_state_event[id][index] = data[index];
    }
    btstack_at_last_state_event_length[id] = length;
    btstack_at_last_state_event_valid[id] = 1u;
    return 0;
}

static void btstack_at_emit_result(
    const btstack_at_command_t *command,
    const btstack_at_result_t *result)
{
    uint16_t length = 0u;
    uint8_t index;

    if (result->success == 0) {
        btstack_at_emit_simple(0);
        return;
    }

    switch (command->id) {
    case BTSTACK_AT_COMMAND_PROFILE:
        (void)btstack_at_format_uint_line(
            "PROFILE", result->state.profile_mask, 1,
            btstack_at_output, sizeof(btstack_at_output), &length);
        break;
    case BTSTACK_AT_COMMAND_VER:
        (void)btstack_at_format_text_response(
            "VER", BTSTACK_AT_FIRMWARE_VERSION,
            btstack_at_output, sizeof(btstack_at_output), &length);
        break;
    case BTSTACK_AT_COMMAND_ADDR:
        (void)btstack_at_format_address_response(
            "ADDR", result->state.local_address,
            btstack_at_output, sizeof(btstack_at_output), &length);
        break;
    case BTSTACK_AT_COMMAND_NAME:
        (void)btstack_at_format_text_response(
            "NAME", result->state.local_name,
            btstack_at_output, sizeof(btstack_at_output), &length);
        break;
    case BTSTACK_AT_COMMAND_PIN:
        (void)btstack_at_format_text_response(
            "PIN", result->state.pin,
            btstack_at_output, sizeof(btstack_at_output), &length);
        break;
    case BTSTACK_AT_COMMAND_SSP:
        (void)btstack_at_format_uint_line(
            "SSP", result->state.ssp_mode, 1,
            btstack_at_output, sizeof(btstack_at_output), &length);
        break;
    case BTSTACK_AT_COMMAND_COD:
        (void)btstack_at_format_hex_response(
            "COD", result->state.class_of_device, 6u,
            btstack_at_output, sizeof(btstack_at_output), &length);
        break;
    case BTSTACK_AT_COMMAND_TDL:
        (void)btstack_at_format_uint_line(
            "TDL", result->state.tdl_mode, 1,
            btstack_at_output, sizeof(btstack_at_output), &length);
        break;
    case BTSTACK_AT_COMMAND_SPKVOL:
        (void)btstack_at_format_uint_line(
            "SPKVOL", result->state.speaker_volume, 1,
            btstack_at_output, sizeof(btstack_at_output), &length);
        break;
    case BTSTACK_AT_COMMAND_HFPCFG:
        (void)btstack_at_format_uint_line(
            "HFPCFG", result->state.hfp_config, 1,
            btstack_at_output, sizeof(btstack_at_output), &length);
        break;
    case BTSTACK_AT_COMMAND_HFPSTAT:
        (void)btstack_at_format_uint_line(
            "HFPSTAT", result->state.hfp_state, 1,
            btstack_at_output, sizeof(btstack_at_output), &length);
        break;
    case BTSTACK_AT_COMMAND_A2DPSTAT:
        (void)btstack_at_format_uint_line(
            "A2DPSTAT", result->state.a2dp_state, 1,
            btstack_at_output, sizeof(btstack_at_output), &length);
        break;
    case BTSTACK_AT_COMMAND_A2DPMUTE:
        (void)btstack_at_format_uint_line(
            "A2DPMUTED", result->state.a2dp_muted, 1,
            btstack_at_output, sizeof(btstack_at_output), &length);
        break;
    case BTSTACK_AT_COMMAND_PBAPSTAT:
        (void)btstack_at_format_uint_line(
            "PBAPSTAT", result->state.pbap_state, 1,
            btstack_at_output, sizeof(btstack_at_output), &length);
        break;
    case BTSTACK_AT_COMMAND_PLIST:
        if (command->form != BTSTACK_AT_FORM_QUERY) {
            btstack_at_emit_simple(1);
            return;
        }
        for (index = 0u; index < BTSTACK_AT_PEER_MAX; index++) {
            if (result->state.peers[index].valid) {
                if (btstack_at_format_plist_record(
                        (uint8_t)(index + 1u),
                        result->state.peers[index].profile_mask,
                        result->state.peers[index].address,
                        result->state.peers[index].name,
                        btstack_at_output,
                        sizeof(btstack_at_output),
                        &length) == 0) {
                    btstack_at_emit(length);
                }
            }
        }
        if (btstack_at_format_plist_end(
                btstack_at_output,
                sizeof(btstack_at_output),
                &length) == 0) {
            btstack_at_emit(length);
        }
        btstack_at_emit_simple(1);
        return;
    default:
        btstack_at_emit_simple(1);
        return;
    }

    if (length != 0u) {
        btstack_at_emit(length);
    } else {
        btstack_at_emit_simple(0);
    }
}

static void btstack_at_emit_event(const btstack_at_event_t *event)
{
    uint16_t length = 0u;

    if (btstack_at_format_event(
            event,
            btstack_at_output,
            sizeof(btstack_at_output),
            &length) == 0 &&
        !btstack_at_state_event_is_duplicate(
            event->id,
            btstack_at_output,
            length)) {
        btstack_at_emit(length);
    }
}

static int btstack_at_ring_pop(btstack_ring_t *ring, void *item)
{
    bool popped;

    btstack_platform_enter_critical();
    popped = btstack_ring_pop(ring, item);
    btstack_platform_exit_critical();
    return popped ? 1 : 0;
}

static int btstack_at_drain_events(void)
{
    btstack_at_event_t event;
    uint16_t pending;
    uint16_t index;
    int consumed = 0;

    if (!btstack_at_event_drain_ready()) {
        return 0;
    }
    btstack_platform_enter_critical();
    pending = btstack_ring_count(&btstack_at_event_ring);
    btstack_platform_exit_critical();
    for (index = 0u; index < pending; index++) {
        if (!btstack_at_event_drain_ready()) {
            break;
        }
        if (!btstack_at_ring_pop(&btstack_at_event_ring, &event)) {
            break;
        }
        btstack_at_emit_event(&event);
        btstack_at_event_consumed(&event);
        consumed = 1;
    }
    return consumed;
}

static int btstack_at_wait_for_result(
    uint32_t generation,
    btstack_at_command_id_t command_id,
    btstack_at_result_t *result)
{
    uint32_t start = btstack_platform_time_ms();
    uint32_t elapsed;
    uint32_t remaining;

    for (;;) {
        elapsed = btstack_platform_time_ms() - start;
        if (elapsed >= BTSTACK_AT_EXEC_TIMEOUT_MS) {
            return -1;
        }
        while (btstack_at_ring_pop(&btstack_at_completion_ring, result)) {
            if ((result->generation == generation) &&
                (result->command_id == command_id)) {
                return 0;
            }
        }

        elapsed = btstack_platform_time_ms() - start;
        if (elapsed >= BTSTACK_AT_EXEC_TIMEOUT_MS) {
            return -1;
        }
        remaining = BTSTACK_AT_EXEC_TIMEOUT_MS - elapsed;
        (void)btstack_platform_wait(
            BTSTACK_WORKER_AT,
            BTSTACK_WORKER_SIGNAL_COMPLETION,
            remaining);
    }
}

static uint32_t btstack_at_begin_completion_wait(
    btstack_at_command_id_t command_id)
{
    btstack_at_result_t stale_result;
    uint32_t generation;

    btstack_platform_enter_critical();
    while (btstack_ring_pop(
            &btstack_at_completion_ring, &stale_result)) {
    }
    generation = ++btstack_at_generation;
    btstack_at_pending_generation = generation;
    btstack_at_pending_command = command_id;
    btstack_at_pending_active = 1;
    btstack_platform_exit_critical();
    return generation;
}

static void btstack_at_end_completion_wait(
    uint32_t generation,
    btstack_at_command_id_t command_id)
{
    btstack_at_result_t stale_result;

    btstack_platform_enter_critical();
    if (btstack_at_pending_active &&
        (btstack_at_pending_generation == generation) &&
        (btstack_at_pending_command == command_id)) {
        btstack_at_pending_active = 0;
        while (btstack_ring_pop(
                &btstack_at_completion_ring, &stale_result)) {
        }
    }
    btstack_platform_exit_critical();
}

static void btstack_at_process_item(const btstack_at_input_item_t *item)
{
    btstack_at_command_t command;
    btstack_at_result_t result;
    uint32_t generation;
    int dispatch_status;
    int gate_status;

    /* 1. 解析并严格校验一条完整 AT 命令。 */
    if (btstack_at_parse(item->data, item->length, &command) != 0) {
        btstack_at_emit_simple(0);
        return;
    }

    /* 2. 基础 AT 命令无需访问 BTstack，直接返回 OK。 */
    if (command.id == BTSTACK_AT_COMMAND_AT) {
        btstack_at_emit_simple(1);
        return;
    }

    if (btstack_at_command_gate != NULL) {
        gate_status = btstack_at_command_gate(
            &command,
            btstack_at_command_gate_context);
        if (gate_status == BTSTACK_AT_GATE_HANDLED) {
            btstack_at_emit_simple(1);
            return;
        }
        if (gate_status == BTSTACK_AT_GATE_REJECT) {
            btstack_at_emit_simple(0);
            return;
        }
    }

    /* 3. 拒绝未支持的命令或尚未注册的 BTstack 后端。 */
    if ((command.id == BTSTACK_AT_COMMAND_UNSUPPORTED) ||
        (btstack_at_backend_dispatch == NULL)) {
        btstack_at_emit_simple(0);
        return;
    }

    /* 4. 原子清理旧结果并发布新命令的完成匹配键。 */
    generation = btstack_at_begin_completion_wait(command.id);

    /* 5. 发布匹配键后再调度，以接收同步完成。 */
    dispatch_status = btstack_at_backend_dispatch(&command, generation);
    if (dispatch_status != 0) {
        btstack_at_end_completion_wait(generation, command.id);
        btstack_at_emit_simple(0);
        return;
    }

    /* 6. 等待与流水号和命令 ID 匹配的执行结果。 */
    if (btstack_at_wait_for_result(
            generation,
            command.id,
            &result) != 0) {
        btstack_at_end_completion_wait(generation, command.id);
        btstack_at_emit_simple(0);
        return;
    }
    btstack_at_end_completion_wait(generation, command.id);

    /* 7. 先输出本条命令的最终响应。 */
    btstack_at_emit_result(&command, &result);

    /* 8. 再输出命令执行期间积累的异步事件。 */
    /* The worker drains one bounded event batch after this item returns. */
}

static void btstack_at_worker(void *context)
{
    btstack_at_input_item_t item;
    uint32_t signals = 0u;
    int did_work;

    (void)context;
    for (;;) {
        if ((signals &
                BTSTACK_WORKER_SIGNAL_STACK_INGRESS_SPACE) != 0u) {
            btstack_at_stack_ingress_space_available();
        }
        signals = 0u;
        did_work = 0;
        if (btstack_at_ring_pop(&btstack_at_input_ring, &item)) {
            btstack_at_process_item(&item);
            did_work = 1;
        }
        if (btstack_at_drain_events()) {
            did_work = 1;
        }
        if (!did_work) {
            signals = btstack_platform_wait(
                BTSTACK_WORKER_AT,
                BTSTACK_WORKER_SIGNAL_WORK |
                    BTSTACK_WORKER_SIGNAL_STACK_INGRESS_SPACE,
                BTSTACK_WORKER_WAIT_FOREVER);
        }
    }
}

int btstack_at_init(const btstack_at_config_t *config)
{
    btstack_at_config_t candidate;

    if ((config == NULL) || (config->output_callback == NULL)) {
        return BTSTACK_AT_STATUS_INVALID_ARGUMENT;
    }
    candidate = *config;
    if (candidate.task_priority == 0u) {
        candidate.task_priority = BTSTACK_AT_TASK_PRIORITY;
    }
    if (candidate.task_stack_size == 0u) {
        candidate.task_stack_size = BTSTACK_AT_TASK_STACK_WORDS;
    }
    if (candidate.task_stack_size > BTSTACK_AT_TASK_STACK_WORDS) {
        return BTSTACK_AT_STATUS_INVALID_ARGUMENT;
    }
    if (btstack_at_started != 0) {
        return BTSTACK_AT_STATUS_ALREADY_STARTED;
    }
    if (btstack_at_initialized != 0) {
        if ((btstack_at_config.output_callback ==
                candidate.output_callback) &&
            (btstack_at_config.output_context == candidate.output_context) &&
            (btstack_at_config.task_priority == candidate.task_priority) &&
            (btstack_at_config.task_stack_size ==
                candidate.task_stack_size)) {
            return BTSTACK_AT_STATUS_OK;
        }
        return BTSTACK_AT_STATUS_INVALID_ARGUMENT;
    }

    btstack_at_config = candidate;
    btstack_at_initialized = 1;
    return BTSTACK_AT_STATUS_OK;
}

int btstack_at_start(void)
{
    btstack_worker_config_t worker_config;

    if (btstack_at_initialized == 0) {
        return BTSTACK_AT_STATUS_NOT_INITIALIZED;
    }
    if (btstack_at_started != 0) {
        return BTSTACK_AT_STATUS_OK;
    }

    worker_config.priority = btstack_at_config.task_priority;
    worker_config.stack_words = btstack_at_config.task_stack_size;
    if (btstack_platform_worker_validate(
            BTSTACK_WORKER_AT, &worker_config) != 0) {
        return BTSTACK_AT_STATUS_NOT_READY;
    }

    btstack_ring_init(
        &btstack_at_input_ring,
        btstack_at_input_ring_storage,
        sizeof(btstack_at_input_ring_storage[0]),
        BTSTACK_AT_INPUT_QUEUE_LEN);
    btstack_ring_init(
        &btstack_at_completion_ring,
        btstack_at_completion_ring_storage,
        sizeof(btstack_at_completion_ring_storage[0]),
        1u);
    btstack_ring_init(
        &btstack_at_event_ring,
        btstack_at_event_ring_storage,
        sizeof(btstack_at_event_ring_storage[0]),
        BTSTACK_AT_EVENT_QUEUE_LEN);

    if (btstack_platform_worker_start(
            BTSTACK_WORKER_AT,
            btstack_at_worker,
            NULL,
            &worker_config) != 0) {
        return BTSTACK_AT_STATUS_NOT_READY;
    }

    btstack_at_started = 1;
    return BTSTACK_AT_STATUS_OK;
}

int btstack_at_submit(const char *command, uint16_t length)
{
    btstack_at_input_item_t item;
    uint16_t index;
    bool pushed;

    if ((command == NULL) || (length == 0u)) {
        return BTSTACK_AT_STATUS_INVALID_ARGUMENT;
    }
    if (btstack_at_initialized == 0) {
        return BTSTACK_AT_STATUS_NOT_INITIALIZED;
    }
    if (btstack_at_started == 0) {
        return BTSTACK_AT_STATUS_NOT_READY;
    }
    if (length >= BTSTACK_AT_COMMAND_MAX) {
        return BTSTACK_AT_STATUS_COMMAND_TOO_LONG;
    }

    item.length = length;
    for (index = 0u; index < length; index++) {
        item.data[index] = command[index];
    }
    item.data[length] = '\0';

    btstack_platform_enter_critical();
    pushed = btstack_ring_push(&btstack_at_input_ring, &item);
    btstack_platform_exit_critical();
    if (!pushed) {
        return BTSTACK_AT_STATUS_QUEUE_FULL;
    }
    (void)btstack_platform_notify(
        BTSTACK_WORKER_AT, BTSTACK_WORKER_SIGNAL_WORK);
    return BTSTACK_AT_STATUS_OK;
}

int btstack_at_backend_register(btstack_at_backend_dispatch_t dispatch)
{
    if (dispatch == NULL) {
        return BTSTACK_AT_STATUS_INVALID_ARGUMENT;
    }
    btstack_at_backend_dispatch = dispatch;
    return BTSTACK_AT_STATUS_OK;
}

int btstack_at_command_gate_register(
    btstack_at_command_gate_t gate,
    void *context)
{
    if (gate == NULL) {
        return BTSTACK_AT_STATUS_INVALID_ARGUMENT;
    }
    if ((btstack_at_command_gate != NULL) &&
        ((btstack_at_command_gate != gate) ||
         (btstack_at_command_gate_context != context))) {
        return BTSTACK_AT_STATUS_ALREADY_STARTED;
    }
    btstack_at_command_gate = gate;
    btstack_at_command_gate_context = context;
    return BTSTACK_AT_STATUS_OK;
}

int btstack_at_complete(const btstack_at_result_t *result)
{
    bool pushed;
    int matches_pending;

    if ((result == NULL) || (btstack_at_started == 0)) {
        return BTSTACK_AT_STATUS_NOT_READY;
    }
    btstack_platform_enter_critical();
    matches_pending = btstack_at_pending_active &&
        (result->generation == btstack_at_pending_generation) &&
        (result->command_id == btstack_at_pending_command);
    pushed = matches_pending ?
        btstack_ring_push(&btstack_at_completion_ring, result) : false;
    btstack_platform_exit_critical();
    if (!matches_pending) {
        return BTSTACK_AT_STATUS_OK;
    }
    if (!pushed) {
        return BTSTACK_AT_STATUS_QUEUE_FULL;
    }
    (void)btstack_platform_notify(
        BTSTACK_WORKER_AT, BTSTACK_WORKER_SIGNAL_COMPLETION);
    return BTSTACK_AT_STATUS_OK;
}

int btstack_at_post_event(const btstack_at_event_t *event)
{
    bool pushed;

    if ((event == NULL) || (btstack_at_started == 0)) {
        return BTSTACK_AT_STATUS_NOT_READY;
    }
    btstack_platform_enter_critical();
    pushed = btstack_ring_push(&btstack_at_event_ring, event);
    if (!pushed) {
        btstack_at_event_overflow_count++;
        btstack_at_event_rejected(event);
    }
    btstack_platform_exit_critical();
    if (!pushed) {
        return BTSTACK_AT_STATUS_QUEUE_FULL;
    }
    (void)btstack_platform_notify(
        BTSTACK_WORKER_AT, BTSTACK_WORKER_SIGNAL_WORK);
    return BTSTACK_AT_STATUS_OK;
}

uint32_t btstack_at_event_overflow_count_get(void)
{
    uint32_t count;

    btstack_platform_enter_critical();
    count = btstack_at_event_overflow_count;
    btstack_platform_exit_critical();
    return count;
}

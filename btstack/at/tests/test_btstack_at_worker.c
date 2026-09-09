#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "btstack_at_internal.h"
#include "port/btstack_platform.h"

static btstack_worker_entry_t test_worker_entry;
static void *test_worker_argument;
static unsigned int test_validate_calls[BTSTACK_WORKER_COUNT];
static unsigned int test_start_calls[BTSTACK_WORKER_COUNT];
static unsigned int test_notify_calls[BTSTACK_WORKER_COUNT];
static uint32_t test_last_notify_signals;
static int test_start_should_fail = 1;
static int test_critical_depth;
static int test_notify_inside_critical;
static int test_worker_running;
static unsigned int test_work_wait_calls;
static jmp_buf test_worker_exit;

typedef enum {
    TEST_COMPLETION_NONE = 0,
    TEST_COMPLETION_SYNC,
    TEST_COMPLETION_DELAYED,
    TEST_COMPLETION_FULL,
    TEST_COMPLETION_AT_DEADLINE
} test_completion_mode_t;

static test_completion_mode_t test_completion_mode;
static uint32_t test_time_ms;
static uint32_t test_pending_generation;
static btstack_at_command_id_t test_pending_command;
static unsigned int test_completion_wait_calls;
static uint32_t test_completion_wait_timeouts[8];
static unsigned int test_spurious_completion_wakes;
static int test_delayed_completion_sent;
static int test_first_completion_status;
static int test_second_completion_status;
static int test_wrong_completion_status;
static int test_inject_wrong_command_before_sync;
static int test_inject_event_during_completion;
static int test_inject_stale_on_second_worker_critical_exit;
static unsigned int test_worker_critical_exit_count;
static btstack_at_result_t test_stale_result;

static char test_parsed_commands[64][8];
static unsigned int test_parsed_count;
static unsigned int test_output_count;
static int test_output_outside_worker;
static void *test_expected_output_context;
static char test_outputs[128][32];
static unsigned int test_event_consumed_count;
static btstack_at_event_id_t test_consumed_event_ids[64];
static uint32_t test_consumed_event_values[64];
static unsigned int test_event_rejected_count;
static btstack_at_event_id_t test_rejected_event_id;
static int test_rejected_outside_critical;
static int test_continuous_event_injection;
static int test_second_command_seen;
static int test_event_starvation_detected;
static unsigned int test_continuous_event_outputs;
static int test_event_drain_ready = 1;
static unsigned int test_stack_space_available_calls;
static unsigned int test_stack_space_wait_calls;
static int test_return_stack_space_signal;

void btstack_platform_enter_critical(void)
{
    test_critical_depth++;
}

void btstack_platform_exit_critical(void)
{
    int inject_stale = 0;

    assert(test_critical_depth > 0);
    test_critical_depth--;
    if ((test_critical_depth == 0) && (test_worker_running != 0) &&
        (test_inject_stale_on_second_worker_critical_exit != 0)) {
        test_worker_critical_exit_count++;
        if (test_worker_critical_exit_count == 2u) {
            test_inject_stale_on_second_worker_critical_exit = 0;
            inject_stale = 1;
        }
    }
    if (inject_stale != 0) {
        assert(btstack_at_complete(&test_stale_result) ==
            BTSTACK_AT_STATUS_OK);
    }
}

uint32_t btstack_platform_time_ms(void)
{
    return test_time_ms;
}

int btstack_platform_worker_validate(
    btstack_worker_id_t id,
    const btstack_worker_config_t *config)
{
    assert((unsigned int)id < (unsigned int)BTSTACK_WORKER_COUNT);
    assert(config != NULL);
    test_validate_calls[id]++;
    return 0;
}

int btstack_platform_worker_start(
    btstack_worker_id_t id,
    btstack_worker_entry_t entry,
    void *argument,
    const btstack_worker_config_t *config)
{
    assert((unsigned int)id < (unsigned int)BTSTACK_WORKER_COUNT);
    assert(entry != NULL);
    assert(config != NULL);
    test_start_calls[id]++;
    if (test_start_should_fail != 0) {
        return -1;
    }
    test_worker_entry = entry;
    test_worker_argument = argument;
    return 0;
}

int btstack_platform_notify(btstack_worker_id_t id, uint32_t signals)
{
    assert((unsigned int)id < (unsigned int)BTSTACK_WORKER_COUNT);
    test_notify_calls[id]++;
    test_last_notify_signals = signals;
    if (test_critical_depth != 0) {
        test_notify_inside_critical = 1;
    }
    return 0;
}

uint32_t btstack_platform_wait(
    btstack_worker_id_t id,
    uint32_t mask,
    uint32_t timeout_ms)
{
    assert(id == BTSTACK_WORKER_AT);
    if (mask == BTSTACK_WORKER_SIGNAL_COMPLETION) {
        btstack_at_result_t result;

        assert(timeout_ms != BTSTACK_WORKER_WAIT_FOREVER);
        assert(test_completion_wait_calls <
            (sizeof(test_completion_wait_timeouts) /
             sizeof(test_completion_wait_timeouts[0])));
        test_completion_wait_timeouts[test_completion_wait_calls++] =
            timeout_ms;
        if (test_spurious_completion_wakes != 0u) {
            test_time_ms += 400u;
            test_spurious_completion_wakes--;
            return 0u;
        }
        if ((test_completion_mode == TEST_COMPLETION_DELAYED) &&
            (test_delayed_completion_sent == 0)) {
            btstack_at_event_t event;

            test_time_ms += 400u;
            if (test_inject_event_during_completion != 0) {
                memset(&event, 0, sizeof(event));
                event.id = BTSTACK_AT_EVENT_HFPRING;
                event.data.numeric.values[0] = 77u;
                event.data.numeric.value_count = 1u;
                assert(btstack_at_post_event(&event) ==
                    BTSTACK_AT_STATUS_OK);
                test_inject_event_during_completion = 0;
            }
            memset(&result, 0, sizeof(result));
            result.generation = test_pending_generation;
            result.command_id = test_pending_command;
            result.success = 1;
            test_delayed_completion_sent = 1;
            assert(btstack_at_complete(&result) == BTSTACK_AT_STATUS_OK);
            return BTSTACK_WORKER_SIGNAL_COMPLETION;
        }
        if (test_completion_mode == TEST_COMPLETION_AT_DEADLINE) {
            test_time_ms += timeout_ms;
            memset(&result, 0, sizeof(result));
            result.generation = test_pending_generation;
            result.command_id = test_pending_command;
            result.success = 1;
            assert(btstack_at_complete(&result) == BTSTACK_AT_STATUS_OK);
            return BTSTACK_WORKER_SIGNAL_COMPLETION;
        }
        test_time_ms += timeout_ms;
        return 0u;
    }
    assert(mask == (BTSTACK_WORKER_SIGNAL_WORK |
        BTSTACK_WORKER_SIGNAL_STACK_INGRESS_SPACE));
    assert(timeout_ms == BTSTACK_WORKER_WAIT_FOREVER);
    test_work_wait_calls++;
    if (test_return_stack_space_signal) {
        test_return_stack_space_signal = 0;
        test_stack_space_wait_calls++;
        return BTSTACK_WORKER_SIGNAL_STACK_INGRESS_SPACE;
    }
    longjmp(test_worker_exit, 1);
    return 0u;
}

int btstack_platform_worker_stop(btstack_worker_id_t id)
{
    (void)id;
    return 0;
}

int btstack_at_parse(
    const char *input,
    uint16_t length,
    btstack_at_command_t *command)
{
    size_t copy_length = length;

    assert(test_worker_running != 0);
    assert(test_parsed_count <
        (sizeof(test_parsed_commands) / sizeof(test_parsed_commands[0])));
    if (copy_length >= sizeof(test_parsed_commands[0])) {
        copy_length = sizeof(test_parsed_commands[0]) - 1u;
    }
    memcpy(test_parsed_commands[test_parsed_count], input, copy_length);
    test_parsed_commands[test_parsed_count][copy_length] = '\0';
    test_parsed_count++;
    if ((length == 2u) && (memcmp(input, "C1", 2u) == 0) &&
        test_continuous_event_injection) {
        test_second_command_seen = 1;
        test_continuous_event_injection = 0;
    }
    memset(command, 0, sizeof(*command));
    if ((length == 2u) && (memcmp(input, "AT", 2u) == 0)) {
        command->id = BTSTACK_AT_COMMAND_AT;
    } else if ((length >= 1u) && (input[0] == 'C')) {
        command->id = BTSTACK_AT_COMMAND_AT;
    } else {
        command->id = BTSTACK_AT_COMMAND_PLAY;
    }
    command->form = BTSTACK_AT_FORM_EXECUTE;
    return 0;
}

int btstack_at_format_simple(
    int success,
    char *output,
    uint16_t capacity,
    uint16_t *output_length)
{
    static const char ok[] = "OK\r\n";
    static const char error[] = "ERROR\r\n";
    const char *text = success != 0 ? ok : error;
    uint16_t length = (uint16_t)strlen(text);

    assert(capacity >= length);
    memcpy(output, text, length);
    *output_length = length;
    return 0;
}

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#define UNUSED_FORMATTER(name, signature) \
    int name signature { return -1; }

UNUSED_FORMATTER(btstack_at_format_address_response,
    (const char *name, const uint8_t address[BTSTACK_AT_ADDRESS_LEN],
     char *output, uint16_t capacity, uint16_t *output_length))
UNUSED_FORMATTER(btstack_at_format_uint_line,
    (const char *name, uint32_t value, int append_ok,
     char *output, uint16_t capacity, uint16_t *output_length))
UNUSED_FORMATTER(btstack_at_format_text_response,
    (const char *name, const char *value,
     char *output, uint16_t capacity, uint16_t *output_length))
UNUSED_FORMATTER(btstack_at_format_hex_response,
    (const char *name, uint32_t value, uint8_t digits,
     char *output, uint16_t capacity, uint16_t *output_length))
UNUSED_FORMATTER(btstack_at_format_plist_record,
    (uint8_t index, uint32_t profile_mask,
     const uint8_t address[BTSTACK_AT_ADDRESS_LEN], const char *name,
     char *output, uint16_t capacity, uint16_t *output_length))
UNUSED_FORMATTER(btstack_at_format_plist_end,
    (char *output, uint16_t capacity, uint16_t *output_length))
UNUSED_FORMATTER(btstack_at_format_track_info,
    (const char *title, const char *artist, const char *album,
     char *output, uint16_t capacity, uint16_t *output_length))

int btstack_at_format_event(
    const btstack_at_event_t *event,
    char *output,
    uint16_t capacity,
    uint16_t *output_length)
{
    int length = snprintf(
        output,
        capacity,
        "EV%02u:%03lu\r\n",
        (unsigned int)event->id,
        (unsigned long)((event->id == BTSTACK_AT_EVENT_HFPLOST) ?
            event->data.peer.values[0] :
            event->data.numeric.values[0]));

    assert(length > 0);
    assert((unsigned int)length < capacity);
    *output_length = (uint16_t)length;
    return 0;
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

void btstack_at_event_consumed(const btstack_at_event_t *event)
{
    assert(test_event_consumed_count <
        (sizeof(test_consumed_event_ids) /
         sizeof(test_consumed_event_ids[0])));
    test_consumed_event_ids[test_event_consumed_count] = event->id;
    test_consumed_event_values[test_event_consumed_count] =
        (event->id == BTSTACK_AT_EVENT_HFPLOST) ?
            event->data.peer.values[0] :
            event->data.numeric.values[0];
    test_event_consumed_count++;
    if (event->id == BTSTACK_AT_EVENT_HFPLOST) {
        test_event_drain_ready = 0;
    }
}

int btstack_at_event_drain_ready(void)
{
    return test_event_drain_ready;
}

void btstack_at_stack_ingress_space_available(void)
{
    test_stack_space_available_calls++;
    test_event_drain_ready = 1;
}

void btstack_at_event_rejected(const btstack_at_event_t *event)
{
    assert(event != NULL);
    if (test_critical_depth == 0) {
        test_rejected_outside_critical = 1;
    }
    test_rejected_event_id = event->id;
    test_event_rejected_count++;
}

static void test_output_callback(
    const char *data,
    uint16_t length,
    void *context)
{
    assert(length != 0u);
    assert(test_output_count <
        (sizeof(test_outputs) / sizeof(test_outputs[0])));
    assert(length < sizeof(test_outputs[0]));
    if (test_worker_running == 0) {
        test_output_outside_worker = 1;
    }
    assert(context == test_expected_output_context);
    memcpy(test_outputs[test_output_count], data, length);
    test_outputs[test_output_count][length] = '\0';
    test_output_count++;
    if (test_continuous_event_injection && (length >= 2u) &&
        (data[0] == 'E') && (data[1] == 'V')) {
        btstack_at_event_t event;

        test_continuous_event_outputs++;
        if (!test_second_command_seen &&
            (test_continuous_event_outputs >= 32u)) {
            test_event_starvation_detected = 1;
            longjmp(test_worker_exit, 1);
        }
        memset(&event, 0, sizeof(event));
        event.id = BTSTACK_AT_EVENT_HFPRING;
        event.data.numeric.values[0] =
            500u + test_continuous_event_outputs;
        event.data.numeric.value_count = 1u;
        assert(btstack_at_post_event(&event) == BTSTACK_AT_STATUS_OK);
    }
}

static int test_backend_dispatch(
    const btstack_at_command_t *command,
    uint32_t generation)
{
    btstack_at_result_t result;

    assert(test_worker_running != 0);
    test_pending_generation = generation;
    test_pending_command = command->id;
    if ((test_completion_mode != TEST_COMPLETION_SYNC) &&
        (test_completion_mode != TEST_COMPLETION_FULL)) {
        return 0;
    }

    memset(&result, 0, sizeof(result));
    result.generation = generation;
    result.command_id = command->id;
    result.success = 1;
    if (test_inject_wrong_command_before_sync != 0) {
        result.command_id = BTSTACK_AT_COMMAND_PAUSE;
        test_wrong_completion_status = btstack_at_complete(&result);
        result.command_id = command->id;
        test_inject_wrong_command_before_sync = 0;
    }
    test_first_completion_status = btstack_at_complete(&result);
    if (test_completion_mode == TEST_COMPLETION_FULL) {
        test_second_completion_status = btstack_at_complete(&result);
    }
    return test_first_completion_status;
}

static void test_run_worker_until_idle(void)
{
    if (setjmp(test_worker_exit) == 0) {
        test_worker_running = 1;
        test_worker_entry(test_worker_argument);
        assert(0);
    }
    test_worker_running = 0;
}

static void test_submit_and_run(const char *command)
{
    assert(btstack_at_submit(command, (uint16_t)strlen(command)) ==
        BTSTACK_AT_STATUS_OK);
    test_run_worker_until_idle();
}

static void test_start_retry_and_bounded_input_fifo(void)
{
    btstack_at_config_t config;
    unsigned int index;
    unsigned int notifications_before_full;
    char command[8];
    int output_context;

    memset(&config, 0, sizeof(config));
    test_expected_output_context = &output_context;
    config.output_callback = test_output_callback;
    config.output_context = test_expected_output_context;
    config.task_priority = 0u;
    config.task_stack_size = 0u;

    assert(btstack_at_init(&config) == BTSTACK_AT_STATUS_OK);
    assert(btstack_at_init(&config) == BTSTACK_AT_STATUS_OK);

    assert(btstack_at_start() == BTSTACK_AT_STATUS_NOT_READY);
    assert(test_validate_calls[BTSTACK_WORKER_AT] == 1u);
    assert(test_start_calls[BTSTACK_WORKER_AT] == 1u);
    assert(btstack_at_submit("AT", 2u) == BTSTACK_AT_STATUS_NOT_READY);

    test_start_should_fail = 0;
    assert(btstack_at_start() == BTSTACK_AT_STATUS_OK);
    assert(test_validate_calls[BTSTACK_WORKER_AT] == 2u);
    assert(test_start_calls[BTSTACK_WORKER_AT] == 2u);
    assert(test_worker_entry != NULL);
    for (index = 0u; index < BTSTACK_WORKER_COUNT; index++) {
        if (index != (unsigned int)BTSTACK_WORKER_AT) {
            assert(test_validate_calls[index] == 0u);
            assert(test_start_calls[index] == 0u);
        }
    }

    assert(btstack_at_start() == BTSTACK_AT_STATUS_OK);
    assert(test_start_calls[BTSTACK_WORKER_AT] == 2u);

    for (index = 0u; index < BTSTACK_AT_INPUT_QUEUE_LEN; index++) {
        (void)snprintf(command, sizeof(command), "C%u", index);
        assert(btstack_at_submit(command, 2u) == BTSTACK_AT_STATUS_OK);
    }
    assert(test_notify_calls[BTSTACK_WORKER_AT] == BTSTACK_AT_INPUT_QUEUE_LEN);
    assert(test_last_notify_signals == BTSTACK_WORKER_SIGNAL_WORK);
    assert(test_notify_inside_critical == 0);

    notifications_before_full = test_notify_calls[BTSTACK_WORKER_AT];
    assert(btstack_at_submit("FULL", 4u) == BTSTACK_AT_STATUS_QUEUE_FULL);
    assert(test_notify_calls[BTSTACK_WORKER_AT] == notifications_before_full);

    test_run_worker_until_idle();

    assert(test_parsed_count == BTSTACK_AT_INPUT_QUEUE_LEN);
    assert(test_output_count == BTSTACK_AT_INPUT_QUEUE_LEN);
    assert(test_output_outside_worker == 0);
    for (index = 0u; index < BTSTACK_AT_INPUT_QUEUE_LEN; index++) {
        (void)snprintf(command, sizeof(command), "C%u", index);
        assert(strcmp(test_parsed_commands[index], command) == 0);
    }
}

static void test_completion_stale_window_does_not_steal_slot(void)
{
    unsigned int output_before;
    unsigned int notifications_before;

    assert(btstack_at_backend_register(test_backend_dispatch) ==
        BTSTACK_AT_STATUS_OK);

    test_completion_mode = TEST_COMPLETION_NONE;
    test_time_ms = 100u;
    output_before = test_output_count;
    test_submit_and_run("TIMEOUT");
    assert(test_output_count == output_before + 1u);
    assert(strcmp(test_outputs[output_before], "ERROR\r\n") == 0);
    assert(test_completion_wait_calls == 1u);
    assert(test_completion_wait_timeouts[0] == BTSTACK_AT_EXEC_TIMEOUT_MS);

    memset(&test_stale_result, 0, sizeof(test_stale_result));
    test_stale_result.generation = test_pending_generation;
    test_stale_result.command_id = test_pending_command;
    test_stale_result.success = 1;

    test_completion_mode = TEST_COMPLETION_SYNC;
    test_first_completion_status = -999;
    test_inject_stale_on_second_worker_critical_exit = 1;
    test_worker_critical_exit_count = 0u;
    notifications_before = test_notify_calls[BTSTACK_WORKER_AT];
    output_before = test_output_count;
    test_submit_and_run("WINDOW");

    assert(test_first_completion_status == BTSTACK_AT_STATUS_OK);
    assert(test_output_count == output_before + 1u);
    assert(strcmp(test_outputs[output_before], "OK\r\n") == 0);
    assert(test_notify_inside_critical == 0);
    assert(test_notify_calls[BTSTACK_WORKER_AT] == notifications_before + 2u);
    assert(test_last_notify_signals == BTSTACK_WORKER_SIGNAL_COMPLETION);
}

static void test_completion_delayed_and_wrap_safe_remaining_deadline(void)
{
    unsigned int output_before = test_output_count;

    test_completion_mode = TEST_COMPLETION_DELAYED;
    test_completion_wait_calls = 0u;
    test_spurious_completion_wakes = 1u;
    test_delayed_completion_sent = 0;
    test_time_ms = UINT32_MAX - 199u;
    test_submit_and_run("DELAY");

    assert(test_output_count == output_before + 1u);
    assert(strcmp(test_outputs[output_before], "OK\r\n") == 0);
    assert(test_completion_wait_calls == 2u);
    assert(test_completion_wait_timeouts[0] == 1000u);
    assert(test_completion_wait_timeouts[1] == 600u);
    assert((uint32_t)(test_time_ms - (UINT32_MAX - 199u)) == 800u);
}

static void test_completion_spurious_wakes_do_not_extend_timeout(void)
{
    unsigned int output_before = test_output_count;
    uint32_t start = UINT32_MAX - 599u;

    test_completion_mode = TEST_COMPLETION_NONE;
    test_completion_wait_calls = 0u;
    test_spurious_completion_wakes = 2u;
    test_time_ms = start;
    test_submit_and_run("TIMEOUT2");

    assert(test_output_count == output_before + 1u);
    assert(strcmp(test_outputs[output_before], "ERROR\r\n") == 0);
    assert(test_completion_wait_calls == 3u);
    assert(test_completion_wait_timeouts[0] == 1000u);
    assert(test_completion_wait_timeouts[1] == 600u);
    assert(test_completion_wait_timeouts[2] == 200u);
    assert((uint32_t)(test_time_ms - start) == 1000u);
}

static void test_completion_at_deadline_times_out(void)
{
    unsigned int output_before = test_output_count;

    test_completion_mode = TEST_COMPLETION_AT_DEADLINE;
    test_completion_wait_calls = 0u;
    test_spurious_completion_wakes = 0u;
    test_time_ms = 5000u;
    test_submit_and_run("DEADLINE");

    assert(test_output_count == output_before + 1u);
    assert(strcmp(test_outputs[output_before], "ERROR\r\n") == 0);
    assert(test_completion_wait_calls == 1u);
    assert(test_completion_wait_timeouts[0] == 1000u);
    assert(test_time_ms == 6000u);
}

static void test_stale_completion_while_idle_is_discarded(void)
{
    btstack_at_result_t stale;
    unsigned int notifications_before;
    unsigned int output_before = test_output_count;

    memset(&stale, 0, sizeof(stale));
    stale.generation = test_pending_generation;
    stale.command_id = test_pending_command;
    stale.success = 1;
    notifications_before = test_notify_calls[BTSTACK_WORKER_AT];
    assert(btstack_at_complete(&stale) == BTSTACK_AT_STATUS_OK);
    assert(test_notify_calls[BTSTACK_WORKER_AT] == notifications_before);

    test_completion_mode = TEST_COMPLETION_SYNC;
    test_first_completion_status = -999;
    test_submit_and_run("AFTERSTALE");
    assert(test_first_completion_status == BTSTACK_AT_STATUS_OK);
    assert(test_output_count == output_before + 1u);
    assert(strcmp(test_outputs[output_before], "OK\r\n") == 0);
}

static void test_wrong_command_completion_does_not_steal_slot(void)
{
    unsigned int notifications_before;
    unsigned int output_before = test_output_count;

    test_completion_mode = TEST_COMPLETION_SYNC;
    test_inject_wrong_command_before_sync = 1;
    test_wrong_completion_status = -999;
    test_first_completion_status = -999;
    notifications_before = test_notify_calls[BTSTACK_WORKER_AT];
    test_submit_and_run("WRONGCMD");

    assert(test_wrong_completion_status == BTSTACK_AT_STATUS_OK);
    assert(test_first_completion_status == BTSTACK_AT_STATUS_OK);
    assert(test_notify_calls[BTSTACK_WORKER_AT] == notifications_before + 2u);
    assert(test_output_count == output_before + 1u);
    assert(strcmp(test_outputs[output_before], "OK\r\n") == 0);
}

static void test_completion_ring_capacity_is_one(void)
{
    unsigned int notifications_before;
    unsigned int output_before = test_output_count;

    test_completion_mode = TEST_COMPLETION_FULL;
    test_first_completion_status = -999;
    test_second_completion_status = -999;
    notifications_before = test_notify_calls[BTSTACK_WORKER_AT];
    test_submit_and_run("FULLCOMP");

    assert(test_first_completion_status == BTSTACK_AT_STATUS_OK);
    assert(test_second_completion_status == BTSTACK_AT_STATUS_QUEUE_FULL);
    assert(test_notify_calls[BTSTACK_WORKER_AT] == notifications_before + 2u);
    assert(test_output_count == output_before + 1u);
    assert(strcmp(test_outputs[output_before], "OK\r\n") == 0);
    assert(test_notify_inside_critical == 0);
    assert(test_output_outside_worker == 0);
}

static btstack_at_event_t test_make_event(
    btstack_at_event_id_t id,
    uint32_t value)
{
    btstack_at_event_t event;

    memset(&event, 0, sizeof(event));
    event.id = id;
    event.data.numeric.values[0] = value;
    event.data.numeric.value_count = 1u;
    return event;
}

static void test_event_fifo_is_deferred_to_worker(void)
{
    btstack_at_event_t event;
    unsigned int consumed_before = test_event_consumed_count;
    unsigned int notifications_before =
        test_notify_calls[BTSTACK_WORKER_AT];
    unsigned int output_before = test_output_count;
    unsigned int waits_before = test_work_wait_calls;
    unsigned int index;

    for (index = 0u; index < 3u; index++) {
        event = test_make_event(BTSTACK_AT_EVENT_HFPRING, index + 1u);
        assert(btstack_at_post_event(&event) == BTSTACK_AT_STATUS_OK);
    }

    assert(test_output_count == output_before);
    assert(test_event_consumed_count == consumed_before);
    assert(test_notify_calls[BTSTACK_WORKER_AT] ==
        notifications_before + 3u);
    assert(test_last_notify_signals == BTSTACK_WORKER_SIGNAL_WORK);
    assert(test_notify_inside_critical == 0);

    test_run_worker_until_idle();

    assert(test_output_count == output_before + 3u);
    assert(strcmp(test_outputs[output_before], "EV18:001\r\n") == 0);
    assert(strcmp(test_outputs[output_before + 1u], "EV18:002\r\n") == 0);
    assert(strcmp(test_outputs[output_before + 2u], "EV18:003\r\n") == 0);
    assert(test_event_consumed_count == consumed_before + 3u);
    for (index = 0u; index < 3u; index++) {
        assert(test_consumed_event_ids[consumed_before + index] ==
            BTSTACK_AT_EVENT_HFPRING);
        assert(test_consumed_event_values[consumed_before + index] ==
            index + 1u);
    }
    assert(test_output_outside_worker == 0);
    assert(test_work_wait_calls == waits_before + 1u);
}

static void test_event_ring_drops_only_the_seventeenth_event(void)
{
    btstack_at_event_t event;
    uint32_t overflow_before = btstack_at_event_overflow_count_get();
    unsigned int rejected_before = test_event_rejected_count;
    unsigned int consumed_before = test_event_consumed_count;
    unsigned int notifications_before =
        test_notify_calls[BTSTACK_WORKER_AT];
    unsigned int output_before = test_output_count;
    unsigned int index;

    for (index = 0u; index < BTSTACK_AT_EVENT_QUEUE_LEN; index++) {
        event = test_make_event(BTSTACK_AT_EVENT_HFPRING, 100u + index);
        assert(btstack_at_post_event(&event) == BTSTACK_AT_STATUS_OK);
    }
    event = test_make_event(BTSTACK_AT_EVENT_HFPRING, 999u);
    assert(btstack_at_post_event(&event) == BTSTACK_AT_STATUS_QUEUE_FULL);
    assert(btstack_at_event_overflow_count_get() == overflow_before + 1u);
    assert(test_event_rejected_count == rejected_before + 1u);
    assert(test_rejected_event_id == BTSTACK_AT_EVENT_HFPRING);
    assert(test_rejected_outside_critical == 0);

    assert(test_output_count == output_before);
    assert(test_event_consumed_count == consumed_before);
    assert(test_notify_calls[BTSTACK_WORKER_AT] ==
        notifications_before + BTSTACK_AT_EVENT_QUEUE_LEN);
    assert(test_notify_inside_critical == 0);

    test_run_worker_until_idle();

    assert(test_output_count ==
        output_before + BTSTACK_AT_EVENT_QUEUE_LEN);
    assert(test_event_consumed_count ==
        consumed_before + BTSTACK_AT_EVENT_QUEUE_LEN);
    for (index = 0u; index < BTSTACK_AT_EVENT_QUEUE_LEN; index++) {
        assert(test_consumed_event_values[consumed_before + index] ==
            100u + index);
    }
    event = test_make_event(BTSTACK_AT_EVENT_HFPRING, 200u);
    assert(btstack_at_post_event(&event) == BTSTACK_AT_STATUS_OK);
    assert(btstack_at_event_overflow_count_get() == overflow_before + 1u);
    assert(test_event_rejected_count == rejected_before + 1u);
    test_run_worker_until_idle();
}

static void test_event_during_completion_follows_command_result(void)
{
    unsigned int consumed_before = test_event_consumed_count;
    unsigned int output_before = test_output_count;

    test_completion_mode = TEST_COMPLETION_DELAYED;
    test_completion_wait_calls = 0u;
    test_spurious_completion_wakes = 0u;
    test_delayed_completion_sent = 0;
    test_inject_event_during_completion = 1;
    test_time_ms = 7000u;
    test_submit_and_run("EVENTWAIT");

    assert(test_output_count == output_before + 2u);
    assert(strcmp(test_outputs[output_before], "OK\r\n") == 0);
    assert(strcmp(test_outputs[output_before + 1u], "EV18:077\r\n") == 0);
    assert(test_event_consumed_count == consumed_before + 1u);
    assert(test_consumed_event_values[consumed_before] == 77u);
    assert(test_output_outside_worker == 0);
}

static void test_state_event_dedup_is_preserved(void)
{
    btstack_at_event_t event;
    unsigned int consumed_before = test_event_consumed_count;
    unsigned int output_before = test_output_count;

    event = test_make_event(BTSTACK_AT_EVENT_PLAYSTAT, 3u);
    assert(btstack_at_post_event(&event) == BTSTACK_AT_STATUS_OK);
    assert(btstack_at_post_event(&event) == BTSTACK_AT_STATUS_OK);
    test_run_worker_until_idle();

    assert(test_output_count == output_before + 1u);
    assert(strcmp(test_outputs[output_before], "EV26:003\r\n") == 0);
    assert(test_event_consumed_count == consumed_before + 2u);

    event.data.numeric.values[0] = 4u;
    assert(btstack_at_post_event(&event) == BTSTACK_AT_STATUS_OK);
    test_run_worker_until_idle();
    assert(test_output_count == output_before + 2u);
    assert(strcmp(test_outputs[output_before + 1u], "EV26:004\r\n") == 0);
    assert(test_event_consumed_count == consumed_before + 3u);
}

static void test_continuous_event_producer_cannot_starve_next_command(void)
{
    btstack_at_event_t event = test_make_event(
        BTSTACK_AT_EVENT_HFPRING, 500u);
    unsigned int parsed_before = test_parsed_count;

    test_continuous_event_injection = 1;
    test_second_command_seen = 0;
    test_event_starvation_detected = 0;
    test_continuous_event_outputs = 0u;
    test_completion_mode = TEST_COMPLETION_SYNC;

    assert(btstack_at_submit("PLAY", 4u) == BTSTACK_AT_STATUS_OK);
    assert(btstack_at_submit("C1", 2u) == BTSTACK_AT_STATUS_OK);
    assert(btstack_at_post_event(&event) == BTSTACK_AT_STATUS_OK);
    test_run_worker_until_idle();

    assert(test_event_starvation_detected == 0);
    assert(test_second_command_seen != 0);
    assert(test_parsed_count == parsed_before + 2u);
    assert(test_continuous_event_outputs == 1u);
}

static void test_lost_events_pause_drain_without_busy_spin(void)
{
    btstack_at_event_t lost = test_make_event(
        BTSTACK_AT_EVENT_HFPLOST, 701u);
    unsigned int output_before = test_output_count;
    unsigned int consumed_before = test_event_consumed_count;
    unsigned int waits_before = test_work_wait_calls;

    test_event_drain_ready = 1;
    assert(btstack_at_post_event(&lost) == BTSTACK_AT_STATUS_OK);
    lost.data.peer.values[0] = 702u;
    assert(btstack_at_post_event(&lost) == BTSTACK_AT_STATUS_OK);
    lost.data.peer.values[0] = 703u;
    assert(btstack_at_post_event(&lost) == BTSTACK_AT_STATUS_OK);

    test_run_worker_until_idle();
    assert(test_output_count == output_before + 1u);
    assert(test_event_consumed_count == consumed_before + 1u);
    assert(test_work_wait_calls == waits_before + 1u);
    assert(test_stack_space_available_calls == 0u);

    test_return_stack_space_signal = 1;
    test_run_worker_until_idle();
    assert(test_stack_space_wait_calls == 1u);
    assert(test_stack_space_available_calls == 1u);
    assert(test_output_count == output_before + 2u);
    assert(test_event_consumed_count == consumed_before + 2u);

    test_return_stack_space_signal = 1;
    test_run_worker_until_idle();
    assert(test_stack_space_wait_calls == 2u);
    assert(test_stack_space_available_calls == 2u);
    assert(test_output_count == output_before + 3u);
    assert(test_event_consumed_count == consumed_before + 3u);
}

int main(void)
{
    test_start_retry_and_bounded_input_fifo();
    test_completion_stale_window_does_not_steal_slot();
    test_completion_delayed_and_wrap_safe_remaining_deadline();
    test_completion_spurious_wakes_do_not_extend_timeout();
    test_completion_at_deadline_times_out();
    test_stale_completion_while_idle_is_discarded();
    test_wrong_command_completion_does_not_steal_slot();
    test_completion_ring_capacity_is_one();
    test_event_fifo_is_deferred_to_worker();
    test_event_ring_drops_only_the_seventeenth_event();
    test_event_during_completion_follows_command_result();
    test_state_event_dedup_is_preserved();
    test_continuous_event_producer_cannot_starve_next_command();
    test_lost_events_pause_drain_without_busy_spin();
    return 0;
}

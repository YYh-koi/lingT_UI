#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "btstack_at_internal.h"
#include "port/btstack_platform.h"
#include "core/btstack_profile_manager.h"

bool btstack_profile_manager_hfp_event_delivered(
    uint8_t event_type, const uint8_t address[6])
{ (void)event_type; return address != NULL; }

static btstack_at_event_t test_events[BTSTACK_AT_EVENT_QUEUE_LEN];
static unsigned int test_event_count;
static unsigned int test_resume_dispatch_count;
static int test_first_resume_busy;
static int test_critical_depth;
static btstack_context_callback_registration_t *test_posted_callback;
static btstack_context_callback_registration_t test_cleanup_callback;
static btstack_core_request_t test_cleanup_request;
static unsigned int test_cleanup_dispatch_count;
static unsigned int test_cleanup_post_count;
static unsigned int test_peer_disconnected_count;
static unsigned int test_peer_cleanup_effect_count;
static uint8_t test_peer_disconnected_address[6];
static uint8_t test_peer_disconnected_addresses[4][6];
static int test_post_succeeds = 1;
static int test_pbap_active = 1;
static int test_in_stack_callback;
static unsigned int test_sequence;
static unsigned int test_last_output_sequence;
static unsigned int test_peer_disconnect_sequence;
static char test_last_output[BTSTACK_AT_OUTPUT_MAX];
static uint16_t test_last_output_length;
static unsigned int test_at_notify_count;
static uint32_t test_at_notify_signals;

void btstack_platform_enter_critical(void)
{
    test_critical_depth++;
}

void btstack_platform_exit_critical(void)
{
    assert(test_critical_depth > 0);
    test_critical_depth--;
}

bool btstack_platform_post_stack_callback(
    btstack_context_callback_registration_t *callback)
{
    assert(test_critical_depth == 0);
    assert(callback != NULL);
    test_cleanup_post_count++;
    if (!test_post_succeeds) {
        return false;
    }
    assert(test_posted_callback == NULL);
    test_posted_callback = callback;
    return true;
}

int btstack_platform_notify(btstack_worker_id_t id, uint32_t signals)
{
    assert(id == BTSTACK_WORKER_AT);
    assert(signals == BTSTACK_WORKER_SIGNAL_STACK_INGRESS_SPACE);
    test_at_notify_count++;
    test_at_notify_signals |= signals;
    return 0;
}

static void test_pbap_peer_disconnected(const uint8_t address[6])
{
    assert(test_in_stack_callback != 0);
    test_peer_disconnected_count++;
    memcpy(test_peer_disconnected_address, address,
        sizeof(test_peer_disconnected_address));
    assert(test_peer_disconnected_count <= 4u);
    memcpy(test_peer_disconnected_addresses[
        test_peer_disconnected_count - 1u], address, 6u);
    test_peer_disconnect_sequence = ++test_sequence;
    if (test_pbap_active) {
        test_peer_cleanup_effect_count++;
    }
}

static void test_execute_cleanup(void *context)
{
    const btstack_core_request_t *request = context;

    assert(request == &test_cleanup_request);
    assert(request->id == BT_XSTACK_REQUEST_PBAP_PEER_DISCONNECTED);
    assert(request->has_address != 0u);
    test_posted_callback = NULL;
    test_in_stack_callback = 1;
    test_pbap_peer_disconnected(request->address.bytes);
    btstack_core_request_completed(request, 0);
    test_in_stack_callback = 0;
}

static void test_run_posted_cleanup(void)
{
    btstack_context_callback_registration_t *callback =
        test_posted_callback;

    assert(callback != NULL);
    callback->callback(callback->context);
}

int btstack_at_post_event(const btstack_at_event_t *event)
{
    if (test_event_count == BTSTACK_AT_EVENT_QUEUE_LEN) {
        btstack_platform_enter_critical();
        btstack_at_event_rejected(event);
        btstack_platform_exit_critical();
        return BTSTACK_AT_STATUS_QUEUE_FULL;
    }
    test_events[test_event_count++] = *event;
    return BTSTACK_AT_STATUS_OK;
}

static void test_consume_one_event(void)
{
    btstack_at_event_t event;
    char output[BTSTACK_AT_OUTPUT_MAX];
    uint16_t output_length = 0u;
    unsigned int index;

    assert(test_event_count != 0u);
    assert(btstack_at_event_drain_ready());
    event = test_events[0];
    for (index = 1u; index < test_event_count; index++) {
        test_events[index - 1u] = test_events[index];
    }
    test_event_count--;
    assert(btstack_at_format_event(&event, output, sizeof(output),
        &output_length) == 0);
    assert(output_length < sizeof(test_last_output));
    memcpy(test_last_output, output, output_length);
    test_last_output[output_length] = '\0';
    test_last_output_length = output_length;
    test_last_output_sequence = ++test_sequence;
    btstack_at_event_consumed(&event);
}

int btstack_core_dispatch(const btstack_core_request_t *request)
{
    assert(request != NULL);
    if (request->id == BT_XSTACK_REQUEST_PBAP_EVENT_SPACE_AVAILABLE) {
        test_resume_dispatch_count++;
        if (test_first_resume_busy &&
            (test_resume_dispatch_count == 1u)) {
            return BT_XSTACK_BACKEND_BUSY;
        }
        return 0;
    }
    assert(request->id == BT_XSTACK_REQUEST_PBAP_PEER_DISCONNECTED);
    test_cleanup_dispatch_count++;
    test_cleanup_request = *request;
    test_cleanup_callback.callback = test_execute_cleanup;
    test_cleanup_callback.context = &test_cleanup_request;
    if (!btstack_platform_post_stack_callback(&test_cleanup_callback)) {
        return BT_XSTACK_BACKEND_BUSY;
    }
    return 0;
}

int btstack_at_backend_register(btstack_at_backend_dispatch_t dispatch)
{
    (void)dispatch;
    return BTSTACK_AT_STATUS_OK;
}

int btstack_at_command_gate_register(
    btstack_at_command_gate_t gate,
    void *context)
{
    (void)gate;
    (void)context;
    return BTSTACK_AT_STATUS_OK;
}

int btstack_at_complete(const btstack_at_result_t *result)
{
    (void)result;
    return BTSTACK_AT_STATUS_OK;
}

int btstack_runtime_started(void)
{
    return 1;
}

int btstack_runtime_start(void)
{
    return 0;
}

int btstack_core_copy_state(bt_xstack_state_t *state)
{
    memset(state, 0, sizeof(*state));
    return 0;
}

int btstack_core_state_read_pbap(bt_xstack_phonebook_state_t *state)
{
    *state = BT_XSTACK_PHONEBOOK_STATE_IDLE;
    return 0;
}

static btstack_core_event_envelope_t test_non_pbap_event(void)
{
    btstack_core_event_envelope_t event;

    memset(&event, 0, sizeof(event));
    event.domain = BT_XSTACK_EVENT_DOMAIN_SERVICE;
    event.data.service.id = BT_XSTACK_SERVICE_EVENT_SCAN_COMPLETE;
    return event;
}

static btstack_core_event_envelope_t test_pbap_contact_event(void)
{
    btstack_core_event_envelope_t event;

    memset(&event, 0, sizeof(event));
    event.domain = BT_XSTACK_EVENT_DOMAIN_PHONEBOOK;
    event.data.phonebook.id = BT_XSTACK_PHONEBOOK_EVENT_CONTACT;
    return event;
}

static btstack_core_event_envelope_t test_phone_event(
    bt_xstack_phone_event_id_t id,
    const uint8_t address[6])
{
    btstack_core_event_envelope_t event;

    memset(&event, 0, sizeof(event));
    event.domain = BT_XSTACK_EVENT_DOMAIN_PHONE;
    event.data.phone.id = id;
    memcpy(event.data.phone.address.bytes, address, 6u);
    return event;
}

static void test_fill_with_non_pbap_events_then_reject_contact(void)
{
    btstack_core_event_envelope_t event = test_non_pbap_event();
    btstack_core_event_envelope_t contact = test_pbap_contact_event();
    unsigned int index;

    for (index = 0u; index < BTSTACK_AT_EVENT_QUEUE_LEN; index++) {
        assert(btstack_core_post_event(&event) == BTSTACK_AT_STATUS_OK);
    }
    assert(btstack_core_post_event(&contact) ==
        BTSTACK_AT_STATUS_QUEUE_FULL);
}

static void test_space_from_any_event_resumes_pbap_once(void)
{
    test_fill_with_non_pbap_events_then_reject_contact();

    test_consume_one_event();
    assert(test_resume_dispatch_count == 1u);
    assert(test_critical_depth == 0);

    test_consume_one_event();
    assert(test_resume_dispatch_count == 1u);

    btstack_core_pbap_event_space_request_completed();
    test_consume_one_event();
    assert(test_resume_dispatch_count == 1u);
}

static void test_busy_resume_retries_on_next_space_edge(void)
{
    test_first_resume_busy = 1;
    test_fill_with_non_pbap_events_then_reject_contact();

    test_consume_one_event();
    assert(test_resume_dispatch_count == 1u);
    test_consume_one_event();
    assert(test_resume_dispatch_count == 2u);
    assert(test_critical_depth == 0);
    test_consume_one_event();
    assert(test_resume_dispatch_count == 2u);
}

static void test_hfp_lost_output_precedes_one_stack_cleanup(void)
{
    static const uint8_t address[6] = {
        0x10u, 0x20u, 0x30u, 0x40u, 0x50u, 0x60u
    };
    btstack_core_event_envelope_t lost = test_phone_event(
        BT_XSTACK_PHONE_EVENT_LOST, address);

    assert(btstack_core_post_event(&lost) == BTSTACK_AT_STATUS_OK);
    assert(test_cleanup_dispatch_count == 0u);
    assert(test_cleanup_post_count == 0u);
    assert(test_peer_disconnected_count == 0u);

    test_consume_one_event();
    assert(strcmp(test_last_output, "+HFPLOST=102030405060\r\n") == 0);
    assert(test_last_output_length == 23u);
    assert(test_cleanup_dispatch_count == 1u);
    assert(test_cleanup_post_count == 1u);
    assert(test_peer_disconnected_count == 0u);
    assert(test_posted_callback != NULL);
    assert(!btstack_at_event_drain_ready());

    test_run_posted_cleanup();
    assert(test_peer_disconnected_count == 1u);
    assert(test_peer_cleanup_effect_count == 1u);
    assert(memcmp(test_peer_disconnected_address, address, 6u) == 0);
    assert(test_last_output_sequence < test_peer_disconnect_sequence);
    assert(btstack_at_event_drain_ready());
    assert(test_at_notify_count == 1u);
    assert((test_at_notify_signals &
        BTSTACK_WORKER_SIGNAL_STACK_INGRESS_SPACE) != 0u);

    /* A later session may reuse the same address without another DEVICE. */
    assert(btstack_core_post_event(&lost) == BTSTACK_AT_STATUS_OK);
    test_consume_one_event();
    assert(test_cleanup_dispatch_count == 2u);
    assert(test_cleanup_post_count == 2u);
    assert(test_peer_disconnected_count == 1u);
    test_run_posted_cleanup();
    assert(test_peer_disconnected_count == 2u);
}

static void test_hfp_lost_post_failure_retries_on_stack_ingress_space(void)
{
    static const uint8_t address[6] = {
        0xa1u, 0xb2u, 0xc3u, 0xd4u, 0xe5u, 0xf6u
    };
    btstack_core_event_envelope_t lost = test_phone_event(
        BT_XSTACK_PHONE_EVENT_LOST, address);
    test_post_succeeds = 0;
    assert(btstack_core_post_event(&lost) == BTSTACK_AT_STATUS_OK);
    test_consume_one_event();
    assert(strcmp(test_last_output, "+HFPLOST=A1B2C3D4E5F6\r\n") == 0);
    assert(test_cleanup_dispatch_count == 1u);
    assert(test_cleanup_post_count == 1u);
    assert(test_posted_callback == NULL);
    assert(test_peer_disconnected_count == 0u);
    assert(!btstack_at_event_drain_ready());

    /* No later AT event and no command-slot release: only ingress space. */
    test_post_succeeds = 1;
    btstack_at_stack_ingress_space_available();
    assert(test_cleanup_dispatch_count == 2u);
    assert(test_cleanup_post_count == 2u);
    assert(test_posted_callback != NULL);
    test_run_posted_cleanup();
    assert(test_peer_disconnected_count == 1u);
    assert(memcmp(test_peer_disconnected_address, address, 6u) == 0);
}

static void test_hfp_lost_is_safe_without_active_pbap(void)
{
    static const uint8_t address[6] = {
        1u, 3u, 5u, 7u, 9u, 11u
    };
    btstack_core_event_envelope_t lost = test_phone_event(
        BT_XSTACK_PHONE_EVENT_LOST, address);

    test_pbap_active = 0;
    assert(btstack_core_post_event(&lost) == BTSTACK_AT_STATUS_OK);
    test_consume_one_event();
    test_run_posted_cleanup();
    assert(test_peer_disconnected_count == 1u);
    assert(test_peer_cleanup_effect_count == 0u);
}

static void test_three_lost_events_are_serialized_without_loss(void)
{
    static const uint8_t address_a[6] = {
        0x11u, 0x12u, 0x13u, 0x14u, 0x15u, 0x16u
    };
    static const uint8_t address_b[6] = {
        0x21u, 0x22u, 0x23u, 0x24u, 0x25u, 0x26u
    };
    btstack_core_event_envelope_t lost_a = test_phone_event(
        BT_XSTACK_PHONE_EVENT_LOST, address_a);
    btstack_core_event_envelope_t lost_b = test_phone_event(
        BT_XSTACK_PHONE_EVENT_LOST, address_b);
    btstack_core_event_envelope_t lost_a_again = test_phone_event(
        BT_XSTACK_PHONE_EVENT_LOST, address_a);

    assert(btstack_core_post_event(&lost_a) == BTSTACK_AT_STATUS_OK);
    assert(btstack_core_post_event(&lost_b) == BTSTACK_AT_STATUS_OK);
    assert(btstack_core_post_event(&lost_a_again) == BTSTACK_AT_STATUS_OK);
    assert(test_event_count == 3u);

    test_consume_one_event();
    assert(test_event_count == 2u);
    assert(!btstack_at_event_drain_ready());
    test_run_posted_cleanup();
    assert(test_peer_disconnected_count == 1u);

    test_consume_one_event();
    assert(test_event_count == 1u);
    test_run_posted_cleanup();
    assert(test_peer_disconnected_count == 2u);

    test_consume_one_event();
    assert(test_event_count == 0u);
    test_run_posted_cleanup();
    assert(test_peer_disconnected_count == 3u);
    assert(memcmp(test_peer_disconnected_addresses[0], address_a, 6u) == 0);
    assert(memcmp(test_peer_disconnected_addresses[1], address_b, 6u) == 0);
    assert(memcmp(test_peer_disconnected_addresses[2], address_a, 6u) == 0);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    if (strcmp(argv[1], "accepted") == 0) {
        test_space_from_any_event_resumes_pbap_once();
        return 0;
    }
    if (strcmp(argv[1], "busy") == 0) {
        test_busy_resume_retries_on_next_space_edge();
        return 0;
    }
    if (strcmp(argv[1], "hfp-lost") == 0) {
        test_hfp_lost_output_precedes_one_stack_cleanup();
        return 0;
    }
    if (strcmp(argv[1], "hfp-lost-retry") == 0) {
        test_hfp_lost_post_failure_retries_on_stack_ingress_space();
        return 0;
    }
    if (strcmp(argv[1], "hfp-lost-idle-pbap") == 0) {
        test_hfp_lost_is_safe_without_active_pbap();
        return 0;
    }
    if (strcmp(argv[1], "hfp-lost-three-serialized") == 0) {
        test_three_lost_events_are_serialized_without_loss();
        return 0;
    }
    return 1;
}

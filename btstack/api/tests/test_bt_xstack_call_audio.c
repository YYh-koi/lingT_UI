#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <process.h>
#include <windows.h>

#include "bt_xstack_audio.h"
#include "btstack_call_audio_tx.h"
#include "port/btstack_platform.h"
#include "profiles/btstack_hfp_hf.h"

#define FRAME_LENGTH 48u
#define FRAME_COUNT  42u
#define SENT_FRAME_MAX 80u

static btstack_hfp_audio_tx_ready_callback_t ready_callback;
static uint16_t mock_frame_length;
static unsigned int request_count;
static unsigned int send_count;
static uint8_t sent_payload[SENT_FRAME_MAX * FRAME_LENGTH];
static uint16_t sent_length[SENT_FRAME_MAX];
static int send_result;
static int replace_session_during_send;
static int reenter_ready_during_send;
static btstack_context_callback_registration_t *posted_pump;
static unsigned int post_count;
static int post_result = 1;
static CRITICAL_SECTION ring_critical;
static unsigned int frame_getter_count;

void btstack_platform_enter_critical(void)
{ EnterCriticalSection(&ring_critical); }
void btstack_platform_exit_critical(void)
{ LeaveCriticalSection(&ring_critical); }
bool btstack_platform_post_stack_callback(
    btstack_context_callback_registration_t *callback)
{
    post_count++;
    if (!post_result) return false;
    posted_pump = callback;
    return true;
}

static void run_posted_pump(void)
{
    btstack_context_callback_registration_t *pump = posted_pump;
    assert(pump != NULL);
    posted_pump = NULL;
    pump->callback(pump->context);
}

int btstack_hfp_hf_set_audio_tx_ready_callback(
    btstack_hfp_audio_tx_ready_callback_t callback)
{
    ready_callback = callback;
    return BTSTACK_STATUS_OK;
}

uint16_t btstack_hfp_hf_get_audio_tx_frame_length(void)
{
    frame_getter_count++;
    return mock_frame_length;
}

int btstack_hfp_hf_request_audio_can_send_now(void)
{
    request_count++;
    return BTSTACK_STATUS_OK;
}

int btstack_hfp_hf_send_audio_frame(const uint8_t *data, uint16_t length)
{
    assert(data != NULL);
    assert(length == FRAME_LENGTH);
    if (replace_session_during_send) {
        replace_session_during_send = 0;
        btstack_call_audio_tx_session_changed(false, 0u);
        mock_frame_length = FRAME_LENGTH;
        btstack_call_audio_tx_session_changed(true, FRAME_LENGTH);
    }
    if (send_result != BTSTACK_STATUS_OK) {
        return send_result;
    }
    assert(send_count < SENT_FRAME_MAX);
    memcpy(&sent_payload[send_count * FRAME_LENGTH], data, length);
    sent_length[send_count] = length;
    send_count++;
    if (reenter_ready_during_send) {
        reenter_ready_during_send = 0;
        ready_callback();
    }
    return BTSTACK_STATUS_OK;
}

static void reset_fixture(void)
{
    mock_frame_length = 0u;
    request_count = 0u;
    send_count = 0u;
    send_result = BTSTACK_STATUS_OK;
    replace_session_during_send = 0;
    reenter_ready_during_send = 0;
    posted_pump = NULL;
    post_count = 0u;
    post_result = 1;
    frame_getter_count = 0u;
    memset(sent_payload, 0, sizeof(sent_payload));
    memset(sent_length, 0, sizeof(sent_length));
    btstack_call_audio_tx_init();
    assert(ready_callback != NULL);
}

static void test_public_validation_and_not_ready(void)
{
    uint8_t data[256] = {0};

    reset_fixture();
    assert(bt_xstack_call_audio_write(NULL, 1u) ==
        BT_XSTACK_INVALID_ARGUMENT);
    assert(bt_xstack_call_audio_write(data, 0u) ==
        BT_XSTACK_INVALID_ARGUMENT);
    assert(bt_xstack_call_audio_write(data, 256u) ==
        BT_XSTACK_INVALID_ARGUMENT);
    assert(bt_xstack_call_audio_write(data, 65536u) ==
        BT_XSTACK_INVALID_ARGUMENT);
    assert(bt_xstack_call_audio_write(data, 65537u) ==
        BT_XSTACK_INVALID_ARGUMENT);
    assert(bt_xstack_call_audio_write(data, FRAME_LENGTH) ==
        BT_XSTACK_NOT_READY);
}

static void test_fifo_capacity_and_headers(void)
{
    uint8_t frames[FRAME_COUNT][FRAME_LENGTH];
    uint8_t extra[FRAME_LENGTH] = {0};
    unsigned int frame;
    unsigned int byte;

    reset_fixture();
    mock_frame_length = FRAME_LENGTH;
    btstack_call_audio_tx_session_changed(true, FRAME_LENGTH);
    for (frame = 0u; frame < FRAME_COUNT; frame++) {
        for (byte = 0u; byte < FRAME_LENGTH; byte++) {
            frames[frame][byte] = (uint8_t)(frame + byte);
        }
        assert(bt_xstack_call_audio_write(frames[frame], FRAME_LENGTH) ==
            BT_XSTACK_OK);
    }
    assert(request_count == 0u);
    assert(send_count == 0u);
    assert(post_count == 1u);
    assert(bt_xstack_call_audio_write(extra, FRAME_LENGTH) ==
        BT_XSTACK_QUEUE_FULL);

    for (frame = 0u; frame < FRAME_COUNT; frame++) {
        run_posted_pump();
        assert(request_count == frame + 1u);
        ready_callback();
    }
    assert(send_count == FRAME_COUNT);
    for (frame = 0u; frame < FRAME_COUNT; frame++) {
        assert(sent_length[frame] == FRAME_LENGTH);
        assert(memcmp(&sent_payload[frame * FRAME_LENGTH],
            frames[frame], FRAME_LENGTH) == 0);
    }
}

static void test_disconnect_clears_and_generation_guards_consume(void)
{
    uint8_t old_frame[FRAME_LENGTH];
    uint8_t new_frame[FRAME_LENGTH];

    memset(old_frame, 0x33, sizeof(old_frame));
    memset(new_frame, 0x77, sizeof(new_frame));
    reset_fixture();
    mock_frame_length = FRAME_LENGTH;
    btstack_call_audio_tx_session_changed(true, FRAME_LENGTH);
    assert(bt_xstack_call_audio_write(old_frame, FRAME_LENGTH) ==
        BT_XSTACK_OK);

    replace_session_during_send = 1;
    run_posted_pump();
    ready_callback();
    assert(send_count == 1u);
    assert(bt_xstack_call_audio_write(new_frame, FRAME_LENGTH) ==
        BT_XSTACK_OK);
    run_posted_pump();
    ready_callback();
    assert(send_count == 2u);
    assert(memcmp(&sent_payload[FRAME_LENGTH], new_frame,
        FRAME_LENGTH) == 0);

    assert(bt_xstack_call_audio_write(old_frame, FRAME_LENGTH) ==
        BT_XSTACK_OK);
    btstack_call_audio_tx_session_changed(false, 0u);
    assert(bt_xstack_call_audio_write(new_frame, FRAME_LENGTH) ==
        BT_XSTACK_NOT_READY);
    mock_frame_length = FRAME_LENGTH;
    btstack_call_audio_tx_session_changed(true, FRAME_LENGTH);
    assert(bt_xstack_call_audio_write(new_frame, FRAME_LENGTH) ==
        BT_XSTACK_OK);
    run_posted_pump();
    ready_callback();
    assert(send_count == 3u);
    assert(memcmp(&sent_payload[2u * FRAME_LENGTH], new_frame,
        FRAME_LENGTH) == 0);
}

static void test_post_failure_retains_data_and_reentrant_ready_is_guarded(void)
{
    uint8_t first[FRAME_LENGTH];
    uint8_t second[FRAME_LENGTH];

    memset(first, 0x41, sizeof(first));
    memset(second, 0x52, sizeof(second));
    reset_fixture();
    mock_frame_length = FRAME_LENGTH;
    btstack_call_audio_tx_session_changed(true, FRAME_LENGTH);
    post_result = 0;
    assert(bt_xstack_call_audio_write(first, FRAME_LENGTH) == BT_XSTACK_OK);
    assert(request_count == 0u);
    assert(posted_pump == NULL);
    post_result = 1;
    assert(bt_xstack_call_audio_write(second, FRAME_LENGTH) == BT_XSTACK_OK);
    run_posted_pump();
    reenter_ready_during_send = 1;
    ready_callback();
    assert(send_count == 1u);
    assert(memcmp(sent_payload, first, FRAME_LENGTH) == 0);
    run_posted_pump();
    ready_callback();
    assert(send_count == 2u);
    assert(memcmp(&sent_payload[FRAME_LENGTH], second, FRAME_LENGTH) == 0);
}

static void test_ring_wrap_preserves_fifo(void)
{
    uint8_t frame[FRAME_LENGTH];
    unsigned int index;
    unsigned int expected;

    reset_fixture();
    mock_frame_length = FRAME_LENGTH;
    btstack_call_audio_tx_session_changed(true, FRAME_LENGTH);
    for (index = 0u; index < 30u; index++) {
        memset(frame, (int)index, sizeof(frame));
        assert(bt_xstack_call_audio_write(frame, sizeof(frame)) ==
            BT_XSTACK_OK);
    }
    for (index = 0u; index < 20u; index++) {
        run_posted_pump();
        ready_callback();
    }
    for (index = 30u; index < 50u; index++) {
        memset(frame, (int)index, sizeof(frame));
        assert(bt_xstack_call_audio_write(frame, sizeof(frame)) ==
            BT_XSTACK_OK);
    }
    for (index = 20u; index < 50u; index++) {
        run_posted_pump();
        ready_callback();
    }
    assert(send_count == 50u);
    for (expected = 0u; expected < 50u; expected++) {
        for (index = 0u; index < FRAME_LENGTH; index++) {
            assert(sent_payload[expected * FRAME_LENGTH + index] ==
                (uint8_t)expected);
        }
    }
}

static void test_session_frame_length_linearizes_with_ring(void)
{
    uint8_t frame[60];

    memset(frame, 0x6a, sizeof(frame));
    reset_fixture();
    mock_frame_length = 48u;
    btstack_call_audio_tx_session_changed(true, 48u);
    frame_getter_count = 0u;

    /* Write linearizes first; the following session change clears it. */
    assert(bt_xstack_call_audio_write(frame, 48u) == BT_XSTACK_OK);
    assert(frame_getter_count == 0u);
    assert(posted_pump != NULL);
    mock_frame_length = 60u;
    btstack_call_audio_tx_session_changed(true, 60u);
    run_posted_pump();
    assert(request_count == 0u);

    /* Session change linearizes first; 48 bytes are incomplete for frame 60. */
    assert(bt_xstack_call_audio_write(frame, 48u) == BT_XSTACK_OK);
    assert(frame_getter_count == 0u);
    assert(posted_pump == NULL);
    assert(bt_xstack_call_audio_write(&frame[48], 12u) == BT_XSTACK_OK);
    assert(posted_pump != NULL);
    run_posted_pump();
    assert(request_count == 1u);
}

typedef struct {
    unsigned int id;
    int failed;
} writer_context_t;

static unsigned __stdcall concurrent_writer(void *argument)
{
    writer_context_t *context = (writer_context_t *)argument;
    uint8_t frame[FRAME_LENGTH];
    unsigned int index;

    for (index = 0u; index < 8u; index++) {
        memset(frame, (int)(context->id * 16u + index), sizeof(frame));
        if (bt_xstack_call_audio_write(frame, sizeof(frame)) !=
                BT_XSTACK_OK) {
            context->failed = 1;
        }
    }
    return 0u;
}

static void test_concurrent_writers_do_not_corrupt_frames(void)
{
    HANDLE threads[4];
    writer_context_t contexts[4];
    unsigned int seen[64] = {0};
    unsigned int index;
    unsigned int byte;
    unsigned int marker;

    reset_fixture();
    mock_frame_length = FRAME_LENGTH;
    btstack_call_audio_tx_session_changed(true, FRAME_LENGTH);
    for (index = 0u; index < 4u; index++) {
        contexts[index].id = index;
        contexts[index].failed = 0;
        threads[index] = (HANDLE)_beginthreadex(NULL, 0u,
            concurrent_writer, &contexts[index], 0u, NULL);
        assert(threads[index] != NULL);
    }
    assert(WaitForMultipleObjects(4u, threads, TRUE, INFINITE) ==
        WAIT_OBJECT_0);
    for (index = 0u; index < 4u; index++) {
        CloseHandle(threads[index]);
        assert(contexts[index].failed == 0);
    }
    assert(post_count == 1u);
    for (index = 0u; index < 32u; index++) {
        run_posted_pump();
        ready_callback();
    }
    assert(send_count == 32u);
    for (index = 0u; index < 32u; index++) {
        marker = sent_payload[index * FRAME_LENGTH];
        assert(marker < 56u);
        assert((marker & 0x0fu) < 8u);
        seen[marker]++;
        for (byte = 1u; byte < FRAME_LENGTH; byte++) {
            assert(sent_payload[index * FRAME_LENGTH + byte] == marker);
        }
    }
    for (index = 0u; index < 4u; index++) {
        for (byte = 0u; byte < 8u; byte++) {
            assert(seen[index * 16u + byte] == 1u);
        }
    }
}

int main(void)
{
    InitializeCriticalSection(&ring_critical);
    test_public_validation_and_not_ready();
    test_fifo_capacity_and_headers();
    test_disconnect_clears_and_generation_guards_consume();
    test_post_failure_retains_data_and_reentrant_ready_is_guarded();
    test_ring_wrap_preserves_fifo();
    test_session_frame_length_linearizes_with_ring();
    test_concurrent_writers_do_not_corrupt_frames();
    DeleteCriticalSection(&ring_critical);
    return 0;
}

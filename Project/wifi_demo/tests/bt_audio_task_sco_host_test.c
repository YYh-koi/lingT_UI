#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>

#include "../bt_audio_task.c"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
        exit(1); \
    } \
} while (0)

static uint8_t host_stream_token;
static jmp_buf host_task_exit;
static uint8_t host_task_running;
static uint8_t host_inject_stop_on_reset;
static uint8_t host_inject_stop_done;
static uint32_t host_notify_take_count;
static uint32_t host_injected_pcm_len;
static uint8_t host_injected_pcm[BT_AUDIO_SCO_START_BYTES];
static uint32_t host_dac_put_count;
static uint32_t host_dac_set_count;

typedef enum {
    HOST_TASK_NO_INJECTION = 0,
    HOST_TASK_INJECT_PCM_ON_SECOND_WAKE,
    HOST_TASK_CONFIG_STOP_RACE
} host_task_mode_t;

static host_task_mode_t host_task_mode;

static void write_pattern(uint32_t total, uint8_t seed);

StreamBufferHandle_t xStreamBufferCreateStatic(
    size_t capacity,
    size_t trigger_level,
    uint8_t *storage,
    StaticStreamBuffer_t *state)
{
    (void)capacity;
    (void)trigger_level;
    (void)storage;
    (void)state;
    return &host_stream_token;
}

size_t xStreamBufferSend(
    StreamBufferHandle_t stream,
    const void *data,
    size_t len,
    uint32_t wait_ticks)
{
    (void)stream;
    (void)data;
    (void)wait_ticks;
    return len;
}

size_t xStreamBufferReceive(
    StreamBufferHandle_t stream,
    void *data,
    size_t len,
    uint32_t wait_ticks)
{
    (void)stream;
    (void)data;
    (void)len;
    (void)wait_ticks;
    return 0u;
}

size_t xStreamBufferBytesAvailable(StreamBufferHandle_t stream)
{
    (void)stream;
    return 0u;
}

int xStreamBufferReset(StreamBufferHandle_t stream)
{
    (void)stream;
    if (host_inject_stop_on_reset && !host_inject_stop_done) {
        host_inject_stop_done = 1u;
        bt_audio_task_stop();
    }
    return 0;
}

uint32_t ulTaskNotifyTake(BaseType_t clear_on_exit, uint32_t wait_ticks)
{
    (void)clear_on_exit;
    (void)wait_ticks;
    host_notify_take_count++;
    if ((host_task_mode == HOST_TASK_INJECT_PCM_ON_SECOND_WAKE) &&
        (host_notify_take_count == 2u)) {
        write_pattern(host_injected_pcm_len, host_injected_pcm[0]);
    }
    if (host_task_running &&
        (((host_task_mode == HOST_TASK_NO_INJECTION) &&
          (host_notify_take_count >= 2u)) ||
         ((host_task_mode != HOST_TASK_NO_INJECTION) &&
          (host_notify_take_count >= 3u)))) {
        longjmp(host_task_exit, 1);
    }
    return 0u;
}

void vTaskNotifyGiveFromISR(
    TaskHandle_t task,
    BaseType_t *higher_priority_task_woken)
{
    (void)task;
    *higher_priority_task_woken = pdFALSE;
}

void xTaskNotifyGive(TaskHandle_t task)
{
    (void)task;
}

TaskHandle_t xTaskCreateStatic(
    void (*entry)(void *),
    const char *name,
    uint32_t stack_words,
    void *context,
    uint32_t priority,
    StackType_t *stack,
    StaticTask_t *state)
{
    (void)entry;
    (void)name;
    (void)stack_words;
    (void)context;
    (void)priority;
    (void)stack;
    (void)state;
    return &host_stream_token;
}

void drv_l1_dac_timer_stop(void) {}
void drv_l1_dac_dbf_free(void) {}
INT8U drv_l1_dac_dbf_dma_channel_get(void) { return 0u; }
void drv_l1_dac_mono_set(void) {}
void drv_l1_dac_stereo_set(void) {}
void drv_l1_dac_sample_rate_set(INT32U sample_rate) { (void)sample_rate; }
int drv_l1_dac_cha_dbf_put_callback(
    INT16S *buffer,
    uint32_t samples,
    void (*callback)(INT8U, INT8U))
{
    (void)buffer;
    (void)samples;
    (void)callback;
    host_dac_put_count++;
    return STATUS_OK;
}
int drv_l1_dac_cha_dbf_set(INT16S *buffer, uint32_t samples)
{
    (void)buffer;
    (void)samples;
    host_dac_set_count++;
    return STATUS_OK;
}
void ap_peripheral_speaker_ctrl(int enabled) { (void)enabled; }

static void reset_state(void)
{
    memset(bt_audio_sco_ring_storage, 0, sizeof(bt_audio_sco_ring_storage));
    bt_audio_sco_ring_read = 0u;
    bt_audio_sco_ring_write = 0u;
    bt_audio_sco_ring_count = 0u;
    bt_audio_sco_ring_max_count = 0u;
    bt_audio_sco_overflow_count = 0u;
    bt_audio_sco_dropped_oldest_bytes = 0u;
    bt_audio_source_mismatch_drops = 0u;
    bt_audio_underflow_count = 0u;
    bt_audio_silence_bytes = 0u;
    bt_audio_odd_tail_observations = 0u;
    bt_audio_config_generation = 0u;
    bt_audio_dma_pending = 0u;
    bt_audio_pending_source = BT_AUDIO_SOURCE_IDLE;
    bt_audio_active_source = BT_AUDIO_SOURCE_SCO;
    bt_audio_task_handle = &host_stream_token;
    bt_audio_pcm_stream = &host_stream_token;
    host_task_running = 0u;
    host_task_mode = HOST_TASK_NO_INJECTION;
    host_inject_stop_on_reset = 0u;
    host_inject_stop_done = 0u;
    host_notify_take_count = 0u;
    host_injected_pcm_len = 0u;
    host_dac_put_count = 0u;
    host_dac_set_count = 0u;
}

static void run_audio_task_until_test_halt(host_task_mode_t mode)
{
    host_task_mode = mode;
    host_notify_take_count = 0u;
    if (setjmp(host_task_exit) == 0) {
        host_task_running = 1u;
        bt_audio_task_entry(NULL);
        CHECK(0 && "audio task unexpectedly returned");
    }
    host_task_running = 0u;
}

static void prepare_pending_sco_configuration(void)
{
    bt_audio_active_source = BT_AUDIO_SOURCE_IDLE;
    bt_audio_pending_sample_rate = 8000u;
    bt_audio_pending_channel_count = 1u;
    bt_audio_pending_bits_per_sample = 16u;
    bt_audio_pending_source = BT_AUDIO_SOURCE_SCO;
    bt_audio_config_generation = 1u;
}

static void write_pattern(uint32_t total, uint8_t seed)
{
    uint8_t block[BT_AUDIO_SCO_MAX_WRITE_BYTES];
    uint32_t written = 0u;
    uint32_t i;

    while (written < total) {
        uint32_t block_len = total - written;
        if (block_len > sizeof(block)) {
            block_len = sizeof(block);
        }
        for (i = 0u; i < block_len; i++) {
            block[i] = (uint8_t)(seed + written + i);
        }
        bt_audio_task_write_call_pcm(block, block_len);
        written += block_len;
    }
}

static void test_wraparound_and_even_overflow(void)
{
    uint8_t newest[2] = { 0xa5u, 0x5au };
    uint8_t output[2048];
    size_t received;
    uint32_t i;

    reset_state();
    write_pattern(2047u, 0u);
    CHECK(bt_audio_sco_ring_count == 2047u);
    CHECK(bt_audio_sco_ring_write == 2047u);
    bt_audio_task_write_call_pcm(newest, sizeof(newest));
    CHECK(bt_audio_sco_overflow_count == 1u);
    CHECK(bt_audio_sco_dropped_oldest_bytes == 2u);
    CHECK(bt_audio_sco_ring_read == 2u);
    CHECK(bt_audio_sco_ring_write == 1u);
    CHECK(bt_audio_sco_ring_count == 2047u);

    received = bt_audio_task_read_sco(output, sizeof(output));
    CHECK(received == 2046u);
    for (i = 0u; i < 2045u; i++) {
        CHECK(output[i] == (uint8_t)(i + 2u));
    }
    CHECK(output[2045] == newest[0]);
    CHECK(bt_audio_sco_ring_count == 1u);

    reset_state();
    write_pattern(BT_AUDIO_SCO_RING_BYTES, 11u);
    CHECK(bt_audio_sco_ring_count == BT_AUDIO_SCO_RING_BYTES);
    bt_audio_task_write_call_pcm(newest, sizeof(newest));
    CHECK(bt_audio_sco_dropped_oldest_bytes == 2u);
    CHECK(bt_audio_sco_ring_count == BT_AUDIO_SCO_RING_BYTES);
}

static void test_odd_blocks_keep_byte_stream_phase(void)
{
    const uint8_t first[3] = { 1u, 2u, 3u };
    const uint8_t second[3] = { 4u, 5u, 6u };
    uint8_t output[6];

    reset_state();
    bt_audio_task_write_call_pcm(first, sizeof(first));
    CHECK(bt_audio_sco_ring_count == 3u);
    CHECK(bt_audio_odd_tail_observations == 1u);
    bt_audio_task_write_call_pcm(second, sizeof(second));
    CHECK(bt_audio_sco_ring_count == 6u);
    CHECK(bt_audio_odd_tail_observations == 1u);
    CHECK(bt_audio_task_read_sco(output, sizeof(output)) == sizeof(output));
    CHECK(memcmp(output, "\x01\x02\x03\x04\x05\x06", sizeof(output)) == 0);
}

static void test_start_boundary_and_double_dma_read(void)
{
    uint32_t i;

    reset_state();
    prepare_pending_sco_configuration();
    for (i = 0u; i < sizeof(host_injected_pcm); i++) {
        host_injected_pcm[i] = (uint8_t)(19u + i);
    }
    host_injected_pcm_len = 767u;
    run_audio_task_until_test_halt(HOST_TASK_INJECT_PCM_ON_SECOND_WAKE);
    CHECK(bt_audio_task_sco_bytes_available() == 767u);
    CHECK(host_dac_put_count == 0u);
    CHECK(host_dac_set_count == 0u);

    reset_state();
    prepare_pending_sco_configuration();
    for (i = 0u; i < sizeof(host_injected_pcm); i++) {
        host_injected_pcm[i] = (uint8_t)(31u + i);
    }
    host_injected_pcm_len = BT_AUDIO_SCO_START_BYTES;
    run_audio_task_until_test_halt(HOST_TASK_INJECT_PCM_ON_SECOND_WAKE);
    CHECK(host_dac_put_count == 1u);
    CHECK(host_dac_set_count == 1u);
    CHECK(bt_audio_sco_ring_count == 0u);
    CHECK(bt_audio_underflow_count == 0u);
}

static void test_underflow_preserves_odd_tail(void)
{
    const uint8_t input[5] = { 9u, 8u, 7u, 6u, 5u };
    INT16S dma[BT_AUDIO_SCO_DMA_BUFFER_BYTES / sizeof(INT16S)];
    const uint8_t *bytes = (const uint8_t *)dma;
    uint32_t i;

    reset_state();
    memset(dma, 0xcc, sizeof(dma));
    bt_audio_task_write_call_pcm(input, sizeof(input));
    bt_audio_task_fill_dma(dma, sizeof(dma), BT_AUDIO_SOURCE_SCO);
    CHECK(memcmp(bytes, input, 4u) == 0);
    for (i = 4u; i < sizeof(dma); i++) {
        CHECK(bytes[i] == 0u);
    }
    CHECK(bt_audio_sco_ring_count == 1u);
    CHECK(bt_audio_underflow_count == 1u);
    CHECK(bt_audio_silence_bytes == BT_AUDIO_SCO_DMA_BUFFER_BYTES - 4u);
}

static void test_last_command_wins_and_stale_pcm_is_rejected(void)
{
    uint8_t stale_pcm[4] = { 1u, 2u, 3u, 4u };
    uint32_t captured_generation;

    reset_state();
    prepare_pending_sco_configuration();
    captured_generation = bt_audio_config_generation;
    host_inject_stop_on_reset = 1u;
    run_audio_task_until_test_halt(HOST_TASK_CONFIG_STOP_RACE);
    CHECK(host_inject_stop_done == 1u);
    CHECK(bt_audio_pending_source == BT_AUDIO_SOURCE_IDLE);
    CHECK(bt_audio_active_source == BT_AUDIO_SOURCE_IDLE);
    CHECK(bt_audio_config_generation != captured_generation);
    CHECK(host_dac_put_count == 0u);
    bt_audio_task_write_call_pcm(stale_pcm, sizeof(stale_pcm));
    CHECK(bt_audio_sco_ring_count == 0u);
    CHECK(bt_audio_source_mismatch_drops == 1u);

    reset_state();
    CHECK(bt_audio_task_configure(44100u, 2u, 16u) == 0);
    captured_generation = bt_audio_config_generation;
    CHECK(bt_audio_task_configure(8000u, 1u, 16u) == 0);
    CHECK(bt_audio_config_generation == captured_generation + 1u);
    CHECK(bt_audio_pending_source == BT_AUDIO_SOURCE_SCO);
    run_audio_task_until_test_halt(HOST_TASK_NO_INJECTION);
    CHECK(bt_audio_active_source == BT_AUDIO_SOURCE_SCO);
}

int main(void)
{
    CHECK(BT_AUDIO_SCO_RING_BYTES == 2048u);
    CHECK(BT_AUDIO_SCO_DMA_BUFFER_BYTES == 384u);
    CHECK(BT_AUDIO_SCO_START_BYTES == 768u);
    test_wraparound_and_even_overflow();
    test_odd_blocks_keep_byte_stream_phase();
    test_start_boundary_and_double_dma_read();
    test_underflow_preserves_odd_tail();
    test_last_command_wins_and_stale_pcm_is_rejected();
    puts("Executable SCO ring/lifecycle tests passed");
    return 0;
}

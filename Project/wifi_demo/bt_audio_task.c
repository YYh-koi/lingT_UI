#include "application.h"
#include "bt_audio_task.h"

#include <string.h>

#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "task.h"
#include "drv_l1_dac.h"
#include "ap_peripheral_handling.h"

enum {
    BT_AUDIO_A2DP_DMA_BUFFER_BYTES = 4096,
    BT_AUDIO_SCO_DMA_BUFFER_BYTES = 384,
    BT_AUDIO_SCO_START_BYTES = 768,
    BT_AUDIO_SCO_RING_BYTES = 2048,
    BT_AUDIO_SCO_MAX_WRITE_BYTES = 255,
    BT_AUDIO_DMA_BUFFER_BYTES = BT_AUDIO_A2DP_DMA_BUFFER_BYTES,
    BT_AUDIO_DMA_BUFFER_SAMPLES = BT_AUDIO_DMA_BUFFER_BYTES / 2,
    /*
     * Keep enough decoded A2DP PCM queued to absorb Bluetooth/HCI bursts.
     * At 44.1 kHz stereo this is about 370 ms; the first two DMA buffers
     * consume 46 ms before the first refill interrupt arrives.
     */
    BT_AUDIO_PCM_BUFFER_BYTES = BT_AUDIO_DMA_BUFFER_BYTES * 16,
    BT_AUDIO_A2DP_START_BYTES = BT_AUDIO_DMA_BUFFER_BYTES * 6,
    BT_AUDIO_DIAG_LOG_INTERVAL = 32,
    BT_AUDIO_TASK_STACK_WORDS = 1024
};

static INT16S bt_audio_dma_buffers[2][BT_AUDIO_DMA_BUFFER_SAMPLES]
    __attribute__((aligned(32)));

static StaticStreamBuffer_t bt_audio_pcm_stream_state;
static uint8_t bt_audio_pcm_stream_storage[
    BT_AUDIO_PCM_BUFFER_BYTES + 1u];
static StreamBufferHandle_t bt_audio_pcm_stream;

static uint8_t bt_audio_sco_ring_storage[BT_AUDIO_SCO_RING_BYTES];
static uint16_t bt_audio_sco_ring_read;
static uint16_t bt_audio_sco_ring_write;
static uint16_t bt_audio_sco_ring_count;
static uint16_t bt_audio_sco_ring_max_count;

static StaticTask_t bt_audio_task_state;
static StackType_t bt_audio_task_stack[BT_AUDIO_TASK_STACK_WORDS];
static TaskHandle_t bt_audio_task_handle;

static volatile uint32_t bt_audio_pending_sample_rate;
static volatile uint32_t bt_audio_config_generation;
static volatile uint32_t bt_audio_dma_pending;
static volatile uint32_t bt_audio_drop_count;
static volatile uint32_t bt_audio_underflow_count;
static volatile uint32_t bt_audio_silence_bytes;
static volatile uint32_t bt_audio_sco_overflow_count;
static volatile uint32_t bt_audio_sco_dropped_oldest_bytes;
static volatile uint32_t bt_audio_source_mismatch_drops;
static volatile uint32_t bt_audio_odd_tail_observations;
static volatile uint8_t bt_audio_pending_channel_count;
static volatile uint8_t bt_audio_pending_bits_per_sample;
static volatile bt_audio_source_t bt_audio_pending_source;
static volatile bt_audio_source_t bt_audio_active_source;

static void bt_audio_task_reset_sco_ring(void)
{
    taskENTER_CRITICAL();
    bt_audio_sco_ring_read = 0u;
    bt_audio_sco_ring_write = 0u;
    bt_audio_sco_ring_count = 0u;
    taskEXIT_CRITICAL();
}

static uint32_t bt_audio_task_sco_bytes_available(void)
{
    uint32_t available_bytes;

    taskENTER_CRITICAL();
    available_bytes = bt_audio_sco_ring_count;
    taskEXIT_CRITICAL();
    return available_bytes;
}

static size_t bt_audio_task_read_sco(
    uint8_t *destination,
    uint32_t requested_bytes)
{
    uint32_t available_bytes;
    uint32_t first_bytes;

    taskENTER_CRITICAL();
    available_bytes = bt_audio_sco_ring_count;
    available_bytes &= ~1u;
    if (available_bytes > requested_bytes) {
        available_bytes = requested_bytes;
    }
    first_bytes = BT_AUDIO_SCO_RING_BYTES - bt_audio_sco_ring_read;
    if (first_bytes > available_bytes) {
        first_bytes = available_bytes;
    }
    if (first_bytes != 0u) {
        memcpy(destination,
            &bt_audio_sco_ring_storage[bt_audio_sco_ring_read],
            first_bytes);
    }
    if (available_bytes > first_bytes) {
        memcpy(destination + first_bytes,
            bt_audio_sco_ring_storage,
            available_bytes - first_bytes);
    }
    bt_audio_sco_ring_read = (uint16_t)(
        (bt_audio_sco_ring_read + available_bytes) %
        BT_AUDIO_SCO_RING_BYTES);
    bt_audio_sco_ring_count -= (uint16_t)available_bytes;
    taskEXIT_CRITICAL();
    return (size_t)available_bytes;
}

static void bt_audio_task_flush_pcm(void)
{
    if (bt_audio_pcm_stream != NULL) {
        (void)xStreamBufferReset(bt_audio_pcm_stream);
    }
}

static void bt_audio_task_stop_dac(void)
{
#if BT_AUDIO_DIAG_BYPASS_DAC_DMA
    ap_peripheral_speaker_ctrl(0);
    return;
#else
    ap_peripheral_speaker_ctrl(0);
    drv_l1_dac_timer_stop();
    drv_l1_dac_dbf_free();
#endif
}

static void bt_audio_task_fill_dma(
    INT16S *buffer,
    uint32_t buffer_bytes,
    bt_audio_source_t source)
{
    uint8_t *destination = (uint8_t *)buffer;
    size_t received;

    if (source == BT_AUDIO_SOURCE_SCO) {
        received = bt_audio_task_read_sco(destination, buffer_bytes);
    } else {
        received = xStreamBufferReceive(
            bt_audio_pcm_stream,
            destination,
            (size_t)buffer_bytes,
            0u);
    }

    if (received < (size_t)buffer_bytes) {
        uint32_t missing = buffer_bytes - (uint32_t)received;
        memset(destination + received, 0, missing);
        taskENTER_CRITICAL();
        bt_audio_underflow_count++;
        bt_audio_silence_bytes += missing;
        taskEXIT_CRITICAL();
        if ((bt_audio_underflow_count %
                BT_AUDIO_DIAG_LOG_INTERVAL) == 1u) {
            DBG_PRINT("bt audio: underflow=%u missing=%u\r\n",
                (unsigned int)bt_audio_underflow_count,
                (unsigned int)missing);
        }
    }
}

#if !BT_AUDIO_DIAG_BYPASS_DAC_DMA
static void bt_audio_dac_isr(INT8U channel, INT8U message)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    (void)message;
    if ((bt_audio_task_handle == NULL) ||
        (channel != (INT8U)drv_l1_dac_dbf_dma_channel_get())) {
        return;
    }
    bt_audio_dma_pending++;
    vTaskNotifyGiveFromISR(
        bt_audio_task_handle,
        &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
}
#endif

static int bt_audio_task_start_dac(
    uint32_t sample_rate,
    uint8_t channel_count,
    uint32_t dma_buffer_bytes,
    bt_audio_source_t source)
{
#if BT_AUDIO_DIAG_BYPASS_DAC_DMA
    DBG_PRINT(
        "bt audio diag: DAC DMA bypassed rate=%u channels=%u\r\n",
        (unsigned int)sample_rate,
        (unsigned int)channel_count);
    return 0;
#else
    if (channel_count == 1u) {
        drv_l1_dac_mono_set();
    } else {
        drv_l1_dac_stereo_set();
    }
    drv_l1_dac_sample_rate_set((INT32U)sample_rate);

    bt_audio_task_fill_dma(
        bt_audio_dma_buffers[0], dma_buffer_bytes, source);
    bt_audio_task_fill_dma(
        bt_audio_dma_buffers[1], dma_buffer_bytes, source);
    if (drv_l1_dac_cha_dbf_put_callback(
            bt_audio_dma_buffers[0],
            dma_buffer_bytes / sizeof(INT16S),
            bt_audio_dac_isr) != STATUS_OK) {
        DBG_PRINT("bt audio: DAC DMA start failed\r\n");
        return -1;
    }
    if (drv_l1_dac_cha_dbf_set(
            bt_audio_dma_buffers[1],
            dma_buffer_bytes / sizeof(INT16S)) != STATUS_OK) {
        DBG_PRINT("bt audio: DAC DMA preload failed\r\n");
        bt_audio_task_stop_dac();
        return -1;
    }
    ap_peripheral_speaker_ctrl(1);
    DBG_PRINT(
        "bt audio: DAC started rate=%u channels=%u\r\n",
        (unsigned int)sample_rate,
        (unsigned int)channel_count);
    return 0;
#endif
}

static void bt_audio_task_entry(void *context)
{
    uint32_t active_generation = 0u;
    uint32_t sample_rate = 0u;
    uint32_t dma_buffer_bytes = BT_AUDIO_DMA_BUFFER_BYTES;
    uint32_t available_bytes;
    uint32_t start_bytes;
    bt_audio_source_t source = BT_AUDIO_SOURCE_IDLE;
    uint8_t channel_count = 0u;
    uint8_t configured = 0u;
    uint8_t playing = 0u;
    uint8_t refill_index = 0u;

    (void)context;
    for (;;) {
        uint32_t generation;
        uint32_t dma_pending;
        uint32_t pending_sample_rate;
        bt_audio_source_t pending_source;
        uint8_t pending_channel_count;
        uint8_t commit_configuration;

        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        taskENTER_CRITICAL();
        generation = bt_audio_config_generation;
        dma_pending = bt_audio_dma_pending;
        bt_audio_dma_pending = 0u;
        taskEXIT_CRITICAL();

        if (generation != active_generation) {
            taskENTER_CRITICAL();
            bt_audio_active_source = BT_AUDIO_SOURCE_IDLE;
            pending_sample_rate = bt_audio_pending_sample_rate;
            pending_channel_count = bt_audio_pending_channel_count;
            pending_source = bt_audio_pending_source;
            taskEXIT_CRITICAL();
            if (playing) {
                bt_audio_task_stop_dac();
            }
            bt_audio_task_flush_pcm();
            bt_audio_task_reset_sco_ring();
            playing = 0u;
            configured = 0u;
            source = BT_AUDIO_SOURCE_IDLE;
            refill_index = 0u;

            taskENTER_CRITICAL();
            commit_configuration =
                (generation == bt_audio_config_generation) ? 1u : 0u;
            if (commit_configuration) {
                active_generation = generation;
                sample_rate = pending_sample_rate;
                channel_count = pending_channel_count;
                source = pending_source;
                bt_audio_active_source = source;
            }
            taskEXIT_CRITICAL();
            if (!commit_configuration) {
                continue;
            }

            if (source == BT_AUDIO_SOURCE_IDLE) {
                continue;
            }
            dma_buffer_bytes =
                (source == BT_AUDIO_SOURCE_SCO) ?
                BT_AUDIO_SCO_DMA_BUFFER_BYTES :
                BT_AUDIO_A2DP_DMA_BUFFER_BYTES;
            configured = 1u;
        }

        while ((dma_pending != 0u) && playing) {
            bt_audio_task_fill_dma(
                bt_audio_dma_buffers[refill_index],
                dma_buffer_bytes,
                source);
            (void)drv_l1_dac_cha_dbf_set(
                bt_audio_dma_buffers[refill_index],
                dma_buffer_bytes / sizeof(INT16S));
            refill_index ^= 1u;
            dma_pending--;
        }

        if (source == BT_AUDIO_SOURCE_SCO) {
            available_bytes = bt_audio_task_sco_bytes_available();
            start_bytes = BT_AUDIO_SCO_START_BYTES;
        } else if (source == BT_AUDIO_SOURCE_A2DP) {
            available_bytes = (uint32_t)xStreamBufferBytesAvailable(
                bt_audio_pcm_stream);
            start_bytes = BT_AUDIO_A2DP_START_BYTES;
        } else {
            available_bytes = 0u;
            start_bytes = dma_buffer_bytes * 2u;
        }
        if (configured && !playing &&
            (available_bytes >= start_bytes)) {
            if (bt_audio_task_start_dac(
                    sample_rate,
                    channel_count,
                    dma_buffer_bytes,
                    source) == 0) {
                playing = 1u;
                refill_index = 0u;
            }
        }
    }
}

int bt_audio_task_start(void)
{
    if (bt_audio_task_handle != NULL) {
        return 0;
    }
    bt_audio_pcm_stream = xStreamBufferCreateStatic(
        BT_AUDIO_PCM_BUFFER_BYTES,
        1u,
        bt_audio_pcm_stream_storage,
        &bt_audio_pcm_stream_state);
    if (bt_audio_pcm_stream == NULL) {
        return -1;
    }

    taskENTER_CRITICAL();
    bt_audio_pending_source = BT_AUDIO_SOURCE_IDLE;
    bt_audio_active_source = BT_AUDIO_SOURCE_IDLE;
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
    taskEXIT_CRITICAL();

    bt_audio_task_handle = xTaskCreateStatic(
        bt_audio_task_entry,
        "bt_audio",
        BT_AUDIO_TASK_STACK_WORDS,
        NULL,
        tskIDLE_PRIORITY + 5u,
        bt_audio_task_stack,
        &bt_audio_task_state);
    return (bt_audio_task_handle != NULL) ? 0 : -1;
}

void bt_audio_task_stop(void)
{
    if (bt_audio_task_handle == NULL) {
        return;
    }
    taskENTER_CRITICAL();
    bt_audio_active_source = BT_AUDIO_SOURCE_IDLE;
    bt_audio_pending_source = BT_AUDIO_SOURCE_IDLE;
    bt_audio_config_generation++;
    taskEXIT_CRITICAL();
    xTaskNotifyGive(bt_audio_task_handle);
}

int bt_audio_task_configure(
    uint32_t sample_rate,
    uint8_t channel_count,
    uint8_t bits_per_sample)
{
    if ((bt_audio_task_handle == NULL) ||
        ((sample_rate != 8000u) &&
         (sample_rate != 44100u) &&
         (sample_rate != 48000u)) ||
        ((sample_rate == 8000u) && (channel_count != 1u)) ||
        ((channel_count != 1u) && (channel_count != 2u)) ||
        (bits_per_sample != 16u)) {
        return -1;
    }

    taskENTER_CRITICAL();
    bt_audio_active_source = BT_AUDIO_SOURCE_IDLE;
    bt_audio_pending_sample_rate = sample_rate;
    bt_audio_pending_channel_count = channel_count;
    bt_audio_pending_bits_per_sample = bits_per_sample;
    bt_audio_pending_source = (sample_rate == 8000u) ?
        BT_AUDIO_SOURCE_SCO : BT_AUDIO_SOURCE_A2DP;
    bt_audio_config_generation++;
    taskEXIT_CRITICAL();
    xTaskNotifyGive(bt_audio_task_handle);
    return 0;
}

void bt_audio_task_write_pcm(
    const uint8_t *data,
    uint32_t len)
{
    bt_audio_source_t source;

    taskENTER_CRITICAL();
    source = bt_audio_active_source;
    taskEXIT_CRITICAL();
    if (source == BT_AUDIO_SOURCE_SCO) {
        bt_audio_task_write_call_pcm(data, len);
    } else if (source == BT_AUDIO_SOURCE_A2DP) {
        bt_audio_task_write_media_pcm(data, len);
    }
}

void bt_audio_task_write_media_pcm(
    const uint8_t *data,
    uint32_t len)
{
    size_t sent;
    uint8_t accept;

    if ((data == NULL) || (len == 0u) ||
        (bt_audio_task_handle == NULL) ||
        (bt_audio_pcm_stream == NULL)) {
        return;
    }
    taskENTER_CRITICAL();
    accept = (bt_audio_active_source == BT_AUDIO_SOURCE_A2DP) ? 1u : 0u;
    if (!accept) {
        bt_audio_source_mismatch_drops++;
    }
    taskEXIT_CRITICAL();
    if (!accept) {
        return;
    }
    sent = xStreamBufferSend(
        bt_audio_pcm_stream,
        data,
        (size_t)len,
        0u);
    if (sent < (size_t)len) {
        bt_audio_drop_count++;
        if ((bt_audio_drop_count %
                BT_AUDIO_DIAG_LOG_INTERVAL) == 1u) {
            DBG_PRINT("bt audio: drop=%u bytes=%u\r\n",
                (unsigned int)bt_audio_drop_count,
                (unsigned int)((size_t)len - sent));
        }
    }
    if (sent != 0u) {
        xTaskNotifyGive(bt_audio_task_handle);
    }
}

void bt_audio_task_write_call_pcm(
    const uint8_t *data,
    uint32_t len)
{
    uint32_t discard_bytes;
    uint32_t first_bytes;
    uint32_t free_bytes;

    if ((data == NULL) || (len == 0u) ||
        (len > BT_AUDIO_SCO_MAX_WRITE_BYTES) ||
        (bt_audio_task_handle == NULL)) {
        return;
    }

    taskENTER_CRITICAL();
    if (bt_audio_active_source != BT_AUDIO_SOURCE_SCO) {
        bt_audio_source_mismatch_drops++;
        taskEXIT_CRITICAL();
        return;
    }

    free_bytes = BT_AUDIO_SCO_RING_BYTES - bt_audio_sco_ring_count;
    if (len > free_bytes) {
        discard_bytes = len - free_bytes;
        discard_bytes = (discard_bytes + 1u) & ~1u;
        bt_audio_sco_ring_read = (uint16_t)((bt_audio_sco_ring_read + discard_bytes) % BT_AUDIO_SCO_RING_BYTES);
        bt_audio_sco_ring_count -= (uint16_t)discard_bytes;
        bt_audio_sco_overflow_count++;
        bt_audio_sco_dropped_oldest_bytes += discard_bytes;
    }

    first_bytes = BT_AUDIO_SCO_RING_BYTES - bt_audio_sco_ring_write;
    if (first_bytes > len) {
        first_bytes = len;
    }
    memcpy(&bt_audio_sco_ring_storage[bt_audio_sco_ring_write],
        data, first_bytes);
    if (len > first_bytes) {
        memcpy(bt_audio_sco_ring_storage,
            data + first_bytes, len - first_bytes);
    }
    bt_audio_sco_ring_write = (uint16_t)(
        (bt_audio_sco_ring_write + len) % BT_AUDIO_SCO_RING_BYTES);
    bt_audio_sco_ring_count += (uint16_t)len;
    if (bt_audio_sco_ring_count > bt_audio_sco_ring_max_count) {
        bt_audio_sco_ring_max_count = bt_audio_sco_ring_count;
    }
    if ((bt_audio_sco_ring_count & 1u) != 0u) {
        bt_audio_odd_tail_observations++;
    }
    taskEXIT_CRITICAL();
    xTaskNotifyGive(bt_audio_task_handle);
}

int bt_audio_task_get_diagnostics(
    bt_audio_task_diagnostics_t *diagnostics)
{
    if (diagnostics == NULL) {
        return -1;
    }

    taskENTER_CRITICAL();
    diagnostics->source = bt_audio_active_source;
    diagnostics->sco_bytes = bt_audio_sco_ring_count;
    diagnostics->sco_max_bytes = bt_audio_sco_ring_max_count;
    diagnostics->sco_overflows = bt_audio_sco_overflow_count;
    diagnostics->sco_dropped_oldest_bytes =
        bt_audio_sco_dropped_oldest_bytes;
    diagnostics->source_mismatch_drops =
        bt_audio_source_mismatch_drops;
    diagnostics->dma_underflows = bt_audio_underflow_count;
    diagnostics->silence_bytes = bt_audio_silence_bytes;
    diagnostics->odd_tail_observations =
        bt_audio_odd_tail_observations;
    taskEXIT_CRITICAL();
    return 0;
}

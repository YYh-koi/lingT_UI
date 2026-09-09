#include "btstack_a2dp_media.h"

#include "btstack_config.h"

#if BTSTACK_CLASSIC && BTSTACK_A2DP_SINK

#include <limits.h>
#include <stddef.h>
#include <string.h>

#include "btstack_util.h"
#include "classic/btstack_sbc.h"

enum {
    RTP_FIXED_HEADER_SIZE = 12,
    RTP_VERSION = 2,
    RTP_PADDING = 0x20,
    RTP_EXTENSION = 0x10,
    RTP_CSRC_COUNT = 0x0f,
    SBC_FRAGMENTED = 0xe0,
    SBC_FRAME_COUNT = 0x0f
};

typedef struct {
    btstack_audio_sample_callback_t callback;
    btstack_sbc_decoder_state_t decoder;
    uint32_t sample_rate;
    uint32_t malformed_packets;
    uint32_t unsupported_packets;
    uint8_t channel_count;
    uint8_t configured;
} btstack_a2dp_media_context_t;

static btstack_a2dp_media_context_t btstack_a2dp_media;

static void btstack_a2dp_media_handle_pcm(
    int16_t *data,
    int num_samples,
    int num_channels,
    int sample_rate,
    void *context)
{
    btstack_a2dp_media_context_t *media =
        (btstack_a2dp_media_context_t *)context;
    btstack_audio_sample_t sample;
    uint32_t data_len;

    if ((media == NULL) || !media->configured ||
        (media->callback == NULL) || (data == NULL) ||
        (num_samples <= 0) || (num_channels <= 0) ||
        (sample_rate <= 0)) {
        return;
    }
    if (((uint32_t)sample_rate != media->sample_rate) ||
        ((uint8_t)num_channels != media->channel_count)) {
        return;
    }

    data_len = (uint32_t)num_samples * num_channels * sizeof(int16_t);
    if (data_len > UINT16_MAX) {
        return;
    }

    memset(&sample, 0, sizeof(sample));
    sample.format = BTSTACK_AUDIO_FORMAT_PCM_S16;
    sample.sample_rate = (uint32_t)sample_rate;
    sample.channel_count = (uint8_t)num_channels;
    sample.bits_per_sample = 16u;
    sample.frame_count = (uint16_t)num_samples;
    sample.data = (const uint8_t *)data;
    sample.data_len = (uint16_t)data_len;
    media->callback(&sample);
}

void btstack_a2dp_media_init(
    btstack_audio_sample_callback_t callback)
{
    memset(&btstack_a2dp_media, 0, sizeof(btstack_a2dp_media));
    btstack_a2dp_media.callback = callback;
}

int btstack_a2dp_media_configure(
    uint32_t sample_rate,
    uint8_t channel_count)
{
    if (((sample_rate != 44100u) && (sample_rate != 48000u)) ||
        ((channel_count != 1u) && (channel_count != 2u))) {
        return -1;
    }

    btstack_a2dp_media.sample_rate = sample_rate;
    btstack_a2dp_media.channel_count = channel_count;
    btstack_sbc_decoder_init(
        &btstack_a2dp_media.decoder,
        SBC_MODE_STANDARD,
        btstack_a2dp_media_handle_pcm,
        &btstack_a2dp_media);
    btstack_a2dp_media.configured = 1u;
    return 0;
}

void btstack_a2dp_media_reset(void)
{
    btstack_a2dp_media.configured = 0u;
    btstack_a2dp_media.sample_rate = 0u;
    btstack_a2dp_media.channel_count = 0u;
}

void btstack_a2dp_media_process(
    const uint8_t *packet,
    uint16_t size)
{
    uint32_t offset;
    uint32_t payload_end;
    uint32_t extension_size;
    uint16_t extension_words;
    uint8_t padding_count;
    uint8_t sbc_header;

    if (!btstack_a2dp_media.configured) {
        return;
    }
    if ((packet == NULL) || (size < (RTP_FIXED_HEADER_SIZE + 1u))) {
        btstack_a2dp_media.malformed_packets++;
        return;
    }
    if ((packet[0] >> 6) != RTP_VERSION) {
        btstack_a2dp_media.malformed_packets++;
        return;
    }

    payload_end = size;
    if ((packet[0] & RTP_PADDING) != 0u) {
        padding_count = packet[size - 1u];
        if ((padding_count == 0u) ||
            (padding_count >= payload_end)) {
            btstack_a2dp_media.malformed_packets++;
            return;
        }
        payload_end -= padding_count;
    }

    offset = RTP_FIXED_HEADER_SIZE +
        (4u * (uint32_t)(packet[0] & RTP_CSRC_COUNT));
    if (offset >= payload_end) {
        btstack_a2dp_media.malformed_packets++;
        return;
    }

    if ((packet[0] & RTP_EXTENSION) != 0u) {
        if ((payload_end - offset) < 4u) {
            btstack_a2dp_media.malformed_packets++;
            return;
        }
        extension_words = big_endian_read_16(packet, (int)(offset + 2u));
        extension_size = 4u + (4u * (uint32_t)extension_words);
        if (extension_size > (payload_end - offset)) {
            btstack_a2dp_media.malformed_packets++;
            return;
        }
        offset += extension_size;
    }

    if (offset >= payload_end) {
        btstack_a2dp_media.malformed_packets++;
        return;
    }
    sbc_header = packet[offset++];
    if ((sbc_header & SBC_FRAGMENTED) != 0u) {
        btstack_a2dp_media.unsupported_packets++;
        return;
    }
    if ((sbc_header & SBC_FRAME_COUNT) == 0u) {
        btstack_a2dp_media.malformed_packets++;
        return;
    }
    if (offset >= payload_end) {
        btstack_a2dp_media.malformed_packets++;
        return;
    }

    btstack_sbc_decoder_process_data(
        &btstack_a2dp_media.decoder,
        0,
        packet + offset,
        (int)(payload_end - offset));
}
#else
void btstack_a2dp_media_init(
    btstack_audio_sample_callback_t callback)
{
    (void)callback;
}

int btstack_a2dp_media_configure(
    uint32_t sample_rate,
    uint8_t channel_count)
{
    (void)sample_rate;
    (void)channel_count;
    return -1;
}

void btstack_a2dp_media_reset(void)
{
}

void btstack_a2dp_media_process(
    const uint8_t *packet,
    uint16_t size)
{
    (void)packet;
    (void)size;
}
#endif

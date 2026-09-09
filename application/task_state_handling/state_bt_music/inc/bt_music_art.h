#ifndef BT_MUSIC_ART_H
#define BT_MUSIC_ART_H

#include <stdint.h>

#include "bt_xstack.h"

#define BT_MUSIC_ART_MAX_JPEG_SIZE (64u * 1024u)

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} bt_music_art_rect_t;

void bt_music_art_state_entry(void *previous_state);
void bt_music_art_cover_event(const bt_xstack_cover_art_event_t *event, void *context);
int bt_music_art_calculate_fit(uint16_t source_width, uint16_t source_height,
    uint16_t screen_width, uint16_t screen_height, bt_music_art_rect_t *rect);
int bt_music_art_parse_jpeg_size(const uint8_t *data, uint32_t length,
    uint16_t *width, uint16_t *height);

#endif

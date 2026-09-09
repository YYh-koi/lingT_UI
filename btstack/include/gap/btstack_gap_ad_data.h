#ifndef BTSTACK_GAP_AD_DATA_H
#define BTSTACK_GAP_AD_DATA_H

#include <stdbool.h>
#include <stdint.h>

#include "adapter/btstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BTSTACK_AD_TYPE_FLAGS                          0x01u
#define BTSTACK_AD_TYPE_UUID16_LIST_MORE_AVAILABLE     0x02u
#define BTSTACK_AD_TYPE_UUID16_LIST_COMPLETE           0x03u
#define BTSTACK_AD_TYPE_SHORTENED_LOCAL_NAME           0x08u
#define BTSTACK_AD_TYPE_COMPLETE_LOCAL_NAME            0x09u

typedef struct {
    uint8_t *buffer;
    uint16_t len;
    uint16_t max_len;
} btstack_gap_ad_data_builder_t;

int btstack_gap_ad_data_init(
    btstack_gap_ad_data_builder_t *builder,
    uint8_t *buffer,
    uint16_t buffer_len);
void btstack_gap_ad_data_reset(
    btstack_gap_ad_data_builder_t *builder);
int btstack_gap_ad_data_append_field(
    btstack_gap_ad_data_builder_t *builder,
    uint8_t data_type,
    const uint8_t *data,
    uint16_t data_len);
int btstack_gap_ad_data_append_uuid16_list(
    btstack_gap_ad_data_builder_t *builder,
    const uint16_t *uuid16_list,
    uint8_t uuid16_count,
    bool complete);
int btstack_gap_ad_data_append_local_name(
    btstack_gap_ad_data_builder_t *builder,
    const char *name,
    bool *complete_name);
int btstack_gap_ad_data_append_cached_fields(
    btstack_gap_ad_data_builder_t *builder,
    const btstack_gap_ad_data_builder_t *cached_fields);
const uint8_t *btstack_gap_ad_data_data(
    const btstack_gap_ad_data_builder_t *builder);
uint16_t btstack_gap_ad_data_len(
    const btstack_gap_ad_data_builder_t *builder);

#ifdef __cplusplus
}
#endif

#endif

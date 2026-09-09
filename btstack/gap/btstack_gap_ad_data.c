#include "gap/btstack_gap_ad_data.h"

#include <string.h>

int btstack_gap_ad_data_init(
    btstack_gap_ad_data_builder_t *builder,
    uint8_t *buffer,
    uint16_t buffer_len)
{
    if ((builder == 0) || (buffer == 0) || (buffer_len == 0u)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    builder->buffer = buffer;
    builder->len = 0u;
    builder->max_len = buffer_len;
    memset(builder->buffer, 0, builder->max_len);
    return BTSTACK_STATUS_OK;
}

void btstack_gap_ad_data_reset(btstack_gap_ad_data_builder_t *builder)
{
    if ((builder == 0) || (builder->buffer == 0)) {
        return;
    }
    builder->len = 0u;
    memset(builder->buffer, 0, builder->max_len);
}

int btstack_gap_ad_data_append_field(
    btstack_gap_ad_data_builder_t *builder,
    uint8_t data_type,
    const uint8_t *data,
    uint16_t data_len)
{
    if ((builder == 0) || (builder->buffer == 0) ||
        ((data == 0) && (data_len > 0u)) ||
        (data_len == 0u) ||
        (data_len > 254u)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    if ((uint32_t)builder->len + data_len + 2u > builder->max_len) {
        return BTSTACK_STATUS_NO_MEMORY;
    }

    builder->buffer[builder->len++] = (uint8_t)(data_len + 1u);
    builder->buffer[builder->len++] = data_type;
    if (data_len > 0u) {
        memcpy(&builder->buffer[builder->len], data, data_len);
        builder->len = (uint16_t)(builder->len + data_len);
    }

    return BTSTACK_STATUS_OK;
}

int btstack_gap_ad_data_append_uuid16_list(
    btstack_gap_ad_data_builder_t *builder,
    const uint16_t *uuid16_list,
    uint8_t uuid16_count,
    bool complete)
{
    uint8_t data[254];
    uint8_t i;
    uint8_t data_type;

    if ((uuid16_list == 0) || (uuid16_count == 0u) ||
        ((uint16_t)uuid16_count * 2u > sizeof(data))) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    for (i = 0u; i < uuid16_count; i++) {
        data[(uint16_t)i * 2u] = (uint8_t)(uuid16_list[i] & 0xffu);
        data[((uint16_t)i * 2u) + 1u] =
            (uint8_t)((uuid16_list[i] >> 8) & 0xffu);
    }

    data_type = complete ?
        BTSTACK_AD_TYPE_UUID16_LIST_COMPLETE :
        BTSTACK_AD_TYPE_UUID16_LIST_MORE_AVAILABLE;

    return btstack_gap_ad_data_append_field(
        builder,
        data_type,
        data,
        (uint16_t)uuid16_count * 2u);
}

int btstack_gap_ad_data_append_local_name(
    btstack_gap_ad_data_builder_t *builder,
    const char *name,
    bool *complete_name)
{
    size_t name_len;
    uint16_t copy_len;
    uint8_t data_type;

    if (complete_name != 0) {
        *complete_name = false;
    }
    if ((builder == 0) || (builder->buffer == 0) || (name == 0)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    name_len = strlen(name);
    if (name_len == 0u) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (builder->len + 2u >= builder->max_len) {
        return BTSTACK_STATUS_NO_MEMORY;
    }

    copy_len = (uint16_t)(builder->max_len - builder->len - 2u);
    if (copy_len > 254u) {
        copy_len = 254u;
    }
    if (copy_len > name_len) {
        copy_len = (uint16_t)name_len;
    }

    data_type = (copy_len == name_len) ?
        BTSTACK_AD_TYPE_COMPLETE_LOCAL_NAME :
        BTSTACK_AD_TYPE_SHORTENED_LOCAL_NAME;
    if (complete_name != 0) {
        *complete_name = (copy_len == name_len);
    }

    return btstack_gap_ad_data_append_field(
        builder,
        data_type,
        (const uint8_t *)name,
        copy_len);
}

int btstack_gap_ad_data_append_cached_fields(
    btstack_gap_ad_data_builder_t *builder,
    const btstack_gap_ad_data_builder_t *cached_fields)
{
    if ((builder == 0) || (builder->buffer == 0) ||
        (cached_fields == 0) || (cached_fields->buffer == 0)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (cached_fields->len > cached_fields->max_len) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if ((uint32_t)builder->len + cached_fields->len > builder->max_len) {
        return BTSTACK_STATUS_NO_MEMORY;
    }

    memcpy(&builder->buffer[builder->len],
           cached_fields->buffer,
           cached_fields->len);
    builder->len = (uint16_t)(builder->len + cached_fields->len);
    return BTSTACK_STATUS_OK;
}

const uint8_t *btstack_gap_ad_data_data(
    const btstack_gap_ad_data_builder_t *builder)
{
    if (builder == 0) {
        return 0;
    }
    return builder->buffer;
}

uint16_t btstack_gap_ad_data_len(
    const btstack_gap_ad_data_builder_t *builder)
{
    if (builder == 0) {
        return 0u;
    }
    return builder->len;
}

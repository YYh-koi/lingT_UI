#include "profiles/btstack_bip_client.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "btstack_debug.h"
#include "btstack_defines.h"
#include "btstack_event.h"
#include "classic/avrcp_cover_art_client.h"
#include "yxml.h"

typedef enum {
    BTSTACK_BIP_STATE_IDLE = 0,
    BTSTACK_BIP_STATE_CONNECTING,
    BTSTACK_BIP_STATE_CONNECTED,
    BTSTACK_BIP_STATE_GETTING_PROPERTIES,
    BTSTACK_BIP_STATE_GETTING_IMAGE,
    BTSTACK_BIP_STATE_GETTING_THUMBNAIL,
    BTSTACK_BIP_STATE_DISCONNECTING
} btstack_bip_state_t;

typedef struct {
    yxml_t parser;
    char parser_buffer[128];
    char element[24];
    char attribute[24];
    char value[64];
    uint8_t value_len;
    bt_xstack_cover_art_property_t property;
    uint8_t in_property;
    uint8_t encoding_present;
    uint8_t pixel_present;
    uint8_t failed;
    uint8_t completed;
    uint8_t root_seen;
    uint16_t property_count;
    uint32_t generation;
} btstack_bip_properties_parser_t;

static btstack_bip_event_callback_t btstack_bip_callback;
static btstack_bip_properties_parser_t btstack_bip_properties;

#if BTSTACK_CLASSIC && BTSTACK_AVRCP_CONTROLLER && BTSTACK_AVRCP_TARGET && BTSTACK_AVRCP_COVER_ART
typedef struct {
    btstack_bip_state_t state;
    btstack_bip_operation_t operation;
    btstack_bd_addr_t address;
    char image_handle[8];
    uint32_t generation;
    uint32_t data_offset;
    uint16_t cover_art_cid;
    int terminal_status;
    uint8_t terminal_native_status;
    uint8_t object_valid;
    uint8_t terminal_pending;
    avrcp_cover_art_client_t native_client;
    uint8_t ertm_buffer[2000];
    l2cap_ertm_config_t ertm_config;
    char descriptor[160];
} btstack_bip_client_context_t;

static btstack_bip_client_context_t btstack_bip_client;
#endif

#ifdef BTSTACK_BIP_CLIENT_TESTING
static void btstack_bip_emit_error(void)
{
    btstack_bip_event_t event;

    if (btstack_bip_callback == NULL) return;
    memset(&event, 0, sizeof(event));
    event.type = BTSTACK_BIP_EVENT_ERROR;
    event.operation = BTSTACK_BIP_OPERATION_IMAGE_PROPERTIES;
    event.status = BTSTACK_STATUS_ERROR;
    event.generation = btstack_bip_properties.generation;
    btstack_bip_callback(&event);
}
#endif

static int btstack_bip_copy_symbol(char *destination, size_t capacity, const char *source)
{
    size_t length = strlen(source);

    if (length >= capacity) return BTSTACK_STATUS_ERROR;
    memcpy(destination, source, length + 1u);
    return BTSTACK_STATUS_OK;
}

static int btstack_bip_parse_u32(const char *text, uint32_t limit, uint32_t *value, const char **end)
{
    uint32_t result = 0u;
    const char *cursor = text;

    if (*cursor < '0' || *cursor > '9') return BTSTACK_STATUS_ERROR;
    while (*cursor >= '0' && *cursor <= '9') {
        uint32_t digit = (uint32_t)(*cursor - '0');

        if (result > (limit - digit) / 10u) return BTSTACK_STATUS_ERROR;
        result = result * 10u + digit;
        cursor++;
    }
    *value = result;
    *end = cursor;
    return BTSTACK_STATUS_OK;
}

static int btstack_bip_parse_pixel(const char *text, bt_xstack_cover_art_property_t *property)
{
    uint32_t min_width;
    uint32_t min_height;
    uint32_t max_width;
    uint32_t max_height;
    const char *cursor;

    if (btstack_bip_parse_u32(text, UINT16_MAX, &min_width, &cursor) != BTSTACK_STATUS_OK || *cursor != '*') return BTSTACK_STATUS_ERROR;
    if (btstack_bip_parse_u32(cursor + 1, UINT16_MAX, &min_height, &cursor) != BTSTACK_STATUS_OK) return BTSTACK_STATUS_ERROR;
    max_width = min_width;
    max_height = min_height;
    if (*cursor == '-') {
        if (btstack_bip_parse_u32(cursor + 1, UINT16_MAX, &max_width, &cursor) != BTSTACK_STATUS_OK || *cursor != '*') return BTSTACK_STATUS_ERROR;
        if (btstack_bip_parse_u32(cursor + 1, UINT16_MAX, &max_height, &cursor) != BTSTACK_STATUS_OK) return BTSTACK_STATUS_ERROR;
    }
    if (*cursor != '\0' || min_width > max_width || min_height > max_height) return BTSTACK_STATUS_ERROR;
    property->min_width = (uint16_t)min_width;
    property->max_width = (uint16_t)max_width;
    property->min_height = (uint16_t)min_height;
    property->max_height = (uint16_t)max_height;
    return BTSTACK_STATUS_OK;
}

static int btstack_bip_parse_encoding(const char *text, bt_xstack_cover_art_encoding_t *encoding)
{
    if (strcmp(text, "JPEG") == 0) *encoding = BT_XSTACK_COVER_ART_ENCODING_JPEG;
    else if (strcmp(text, "PNG") == 0) *encoding = BT_XSTACK_COVER_ART_ENCODING_PNG;
    else if (strcmp(text, "GIF") == 0) *encoding = BT_XSTACK_COVER_ART_ENCODING_GIF;
    else if (strcmp(text, "BMP") == 0) *encoding = BT_XSTACK_COVER_ART_ENCODING_BMP;
    else if (strcmp(text, "WBMP") == 0) *encoding = BT_XSTACK_COVER_ART_ENCODING_WBMP;
    else return BTSTACK_STATUS_ERROR;
    return BTSTACK_STATUS_OK;
}

static int btstack_bip_parse_transformations(const char *text, uint8_t *transformations)
{
    const char *cursor = text;
    uint8_t result = 0u;

    while (*cursor != '\0') {
        const char *start;
        size_t length;

        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n') cursor++;
        if (*cursor == '\0') break;
        start = cursor;
        while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t' && *cursor != '\r' && *cursor != '\n') cursor++;
        length = (size_t)(cursor - start);
        if (length == 7u && memcmp(start, "stretch", 7u) == 0) result |= BT_XSTACK_COVER_ART_TRANSFORMATION_STRETCH;
        else if (length == 4u && memcmp(start, "fill", 4u) == 0) result |= BT_XSTACK_COVER_ART_TRANSFORMATION_FILL;
        else if (length == 4u && memcmp(start, "crop", 4u) == 0) result |= BT_XSTACK_COVER_ART_TRANSFORMATION_CROP;
        else return BTSTACK_STATUS_ERROR;
    }
    *transformations = result;
    return BTSTACK_STATUS_OK;
}

static int btstack_bip_apply_attribute(void)
{
    btstack_bip_properties_parser_t *properties = &btstack_bip_properties;

    if (strcmp(properties->attribute, "encoding") == 0) {
        if (btstack_bip_parse_encoding(properties->value, &properties->property.encoding) != BTSTACK_STATUS_OK) return BTSTACK_STATUS_ERROR;
        properties->encoding_present = 1u;
    } else if (strcmp(properties->attribute, "pixel") == 0) {
        if (btstack_bip_parse_pixel(properties->value, &properties->property) != BTSTACK_STATUS_OK) return BTSTACK_STATUS_ERROR;
        properties->pixel_present = 1u;
    } else if (strcmp(properties->attribute, "size") == 0) {
        uint32_t size;
        const char *end;

        if (btstack_bip_parse_u32(properties->value, UINT32_MAX, &size, &end) != BTSTACK_STATUS_OK || *end != '\0') return BTSTACK_STATUS_ERROR;
        properties->property.file_size = size;
        properties->property.file_size_known = 1u;
    } else if (strcmp(properties->attribute, "transformation") == 0) {
        if (btstack_bip_parse_transformations(properties->value, &properties->property.transformations) != BTSTACK_STATUS_OK) return BTSTACK_STATUS_ERROR;
    }
    return BTSTACK_STATUS_OK;
}

static int btstack_bip_emit_property(void)
{
    btstack_bip_event_t event;

    if (!btstack_bip_properties.encoding_present || !btstack_bip_properties.pixel_present) return BTSTACK_STATUS_ERROR;
    btstack_bip_properties.property_count++;
    if (btstack_bip_callback == NULL) return BTSTACK_STATUS_OK;
    memset(&event, 0, sizeof(event));
    event.type = BTSTACK_BIP_EVENT_PROPERTY;
    event.operation = BTSTACK_BIP_OPERATION_IMAGE_PROPERTIES;
    event.status = BTSTACK_STATUS_OK;
    event.generation = btstack_bip_properties.generation;
    event.data.property = btstack_bip_properties.property;
    btstack_bip_callback(&event);
    return BTSTACK_STATUS_OK;
}

static void btstack_bip_emit_properties_completed(void)
{
    btstack_bip_event_t event;

    if (btstack_bip_callback == NULL) return;
    memset(&event, 0, sizeof(event));
    event.type = BTSTACK_BIP_EVENT_PROPERTIES_COMPLETED;
    event.operation = BTSTACK_BIP_OPERATION_IMAGE_PROPERTIES;
    event.status = BTSTACK_STATUS_OK;
    event.generation = btstack_bip_properties.generation;
    event.data.properties_completed.property_count = btstack_bip_properties.property_count;
    btstack_bip_callback(&event);
}

static int btstack_bip_handle_token(yxml_ret_t token)
{
    btstack_bip_properties_parser_t *properties = &btstack_bip_properties;

    switch (token) {
    case YXML_ELEMSTART:
        if (btstack_bip_copy_symbol(properties->element, sizeof(properties->element), properties->parser.elem) != BTSTACK_STATUS_OK) return BTSTACK_STATUS_ERROR;
        if (strcmp(properties->element, "image-properties") == 0) properties->root_seen = 1u;
        if (strcmp(properties->element, "native") == 0 || strcmp(properties->element, "variant") == 0) {
            memset(&properties->property, 0, sizeof(properties->property));
            properties->property.native_image = strcmp(properties->element, "native") == 0 ? 1u : 0u;
            properties->encoding_present = 0u;
            properties->pixel_present = 0u;
            properties->in_property = 1u;
        }
        break;
    case YXML_ATTRSTART:
        if (!properties->in_property) break;
        if (btstack_bip_copy_symbol(properties->attribute, sizeof(properties->attribute), properties->parser.attr) != BTSTACK_STATUS_OK) return BTSTACK_STATUS_ERROR;
        properties->value_len = 0u;
        properties->value[0] = '\0';
        break;
    case YXML_ATTRVAL:
        if (properties->in_property) {
            size_t length = strlen(properties->parser.data);

            if ((size_t)properties->value_len + length >= sizeof(properties->value)) return BTSTACK_STATUS_ERROR;
            memcpy(&properties->value[properties->value_len], properties->parser.data, length);
            properties->value_len = (uint8_t)(properties->value_len + length);
            properties->value[properties->value_len] = '\0';
        }
        break;
    case YXML_ATTREND:
        if (properties->in_property && btstack_bip_apply_attribute() != BTSTACK_STATUS_OK) return BTSTACK_STATUS_ERROR;
        break;
    case YXML_ELEMEND:
        if (properties->in_property) {
            properties->in_property = 0u;
            if (btstack_bip_emit_property() != BTSTACK_STATUS_OK) return BTSTACK_STATUS_ERROR;
        }
        break;
    default:
        break;
    }
    return BTSTACK_STATUS_OK;
}

static void btstack_bip_properties_reset(uint32_t generation)
{
    memset(&btstack_bip_properties, 0, sizeof(btstack_bip_properties));
    btstack_bip_properties.generation = generation;
    yxml_init(&btstack_bip_properties.parser, btstack_bip_properties.parser_buffer, sizeof(btstack_bip_properties.parser_buffer));
}

static void btstack_bip_properties_feed(const uint8_t *data, uint16_t length, uint8_t final_chunk)
{
    uint16_t index;

    if (btstack_bip_properties.failed || btstack_bip_properties.completed) return;
    for (index = 0u; index < length; index++) {
        yxml_ret_t token = yxml_parse(&btstack_bip_properties.parser, data[index]);

        if (token < YXML_OK || btstack_bip_handle_token(token) != BTSTACK_STATUS_OK) {
            btstack_bip_properties.failed = 1u;
            return;
        }
    }
    if (final_chunk) {
        if (yxml_eof(&btstack_bip_properties.parser) != YXML_OK || !btstack_bip_properties.root_seen || btstack_bip_properties.in_property) {
            btstack_bip_properties.failed = 1u;
        } else {
            btstack_bip_properties.completed = 1u;
        }
    }
}

#if BTSTACK_CLASSIC && BTSTACK_AVRCP_CONTROLLER && BTSTACK_AVRCP_TARGET && BTSTACK_AVRCP_COVER_ART
static void btstack_bip_native_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

static void btstack_bip_reset_connection(void)
{
    btstack_bip_client.state = BTSTACK_BIP_STATE_IDLE;
    btstack_bip_client.operation = BTSTACK_BIP_OPERATION_NONE;
    btstack_bip_client.cover_art_cid = 0u;
    btstack_bip_client.data_offset = 0u;
    btstack_bip_client.terminal_pending = 0u;
    btstack_bip_client.terminal_status = BTSTACK_STATUS_OK;
    btstack_bip_client.terminal_native_status = 0u;
}

static void btstack_bip_emit_started(void)
{
    btstack_bip_event_t event;

    if (btstack_bip_callback == NULL) return;
    memset(&event, 0, sizeof(event));
    event.type = BTSTACK_BIP_EVENT_STARTED;
    event.operation = btstack_bip_client.operation;
    event.status = BTSTACK_STATUS_OK;
    event.generation = btstack_bip_client.generation;
    memcpy(event.data.started.address, btstack_bip_client.address, sizeof(btstack_bd_addr_t));
    memcpy(event.data.started.image_handle, btstack_bip_client.image_handle, sizeof(event.data.started.image_handle));
    btstack_bip_callback(&event);
}

static void btstack_bip_emit_connection_established(int status, uint8_t native_status)
{
    btstack_bip_event_t event;

    if (btstack_bip_callback == NULL) return;
    memset(&event, 0, sizeof(event));
    event.type = BTSTACK_BIP_EVENT_CONNECTION_ESTABLISHED;
    event.operation = BTSTACK_BIP_OPERATION_NONE;
    event.status = status;
    event.native_status = native_status;
    btstack_bip_callback(&event);
}

static void btstack_bip_finish_error(int status, uint8_t native_status)
{
    btstack_bip_event_t event;
    btstack_bip_operation_t operation = btstack_bip_client.operation;
    uint32_t generation = btstack_bip_client.generation;

    btstack_bip_reset_connection();
    if (btstack_bip_callback == NULL) return;
    memset(&event, 0, sizeof(event));
    event.type = BTSTACK_BIP_EVENT_ERROR;
    event.operation = operation;
    event.status = status;
    event.native_status = native_status;
    event.generation = generation;
    btstack_bip_callback(&event);
}

static void btstack_bip_finish_success(void)
{
    btstack_bip_event_t event;
    btstack_bip_operation_t operation = btstack_bip_client.operation;
    uint32_t generation = btstack_bip_client.generation;
    uint32_t total_length = btstack_bip_client.data_offset;

    btstack_bip_client.operation = BTSTACK_BIP_OPERATION_NONE;
    btstack_bip_client.state = BTSTACK_BIP_STATE_CONNECTED;
    btstack_bip_client.data_offset = 0u;
    if (btstack_bip_callback == NULL) return;
    memset(&event, 0, sizeof(event));
    event.type = BTSTACK_BIP_EVENT_COMPLETED;
    event.operation = operation;
    event.status = BTSTACK_STATUS_OK;
    event.generation = generation;
    event.data.completed.total_length = total_length;
    btstack_bip_callback(&event);
}

static void btstack_bip_start_cleanup(int status, uint8_t native_status)
{
    uint8_t disconnect_status;

    btstack_bip_client.terminal_status = status;
    btstack_bip_client.terminal_native_status = native_status;
    btstack_bip_client.terminal_pending = 1u;
    if (btstack_bip_client.cover_art_cid == 0u) {
        btstack_bip_finish_error(status, native_status);
        return;
    }
    btstack_bip_client.state = BTSTACK_BIP_STATE_DISCONNECTING;
    disconnect_status = avrcp_cover_art_client_disconnect(btstack_bip_client.cover_art_cid);
    if (disconnect_status != ERROR_CODE_SUCCESS && btstack_bip_client.state == BTSTACK_BIP_STATE_DISCONNECTING) {
        btstack_bip_finish_error(status, native_status != 0u ? native_status : disconnect_status);
    }
}

static const char *btstack_bip_encoding_name(bt_xstack_cover_art_encoding_t encoding)
{
    switch (encoding) {
    case BT_XSTACK_COVER_ART_ENCODING_JPEG: return "JPEG";
    case BT_XSTACK_COVER_ART_ENCODING_PNG: return "PNG";
    case BT_XSTACK_COVER_ART_ENCODING_GIF: return "GIF";
    case BT_XSTACK_COVER_ART_ENCODING_BMP: return "BMP";
    case BT_XSTACK_COVER_ART_ENCODING_WBMP: return "WBMP";
    default: return NULL;
    }
}

static const char *btstack_bip_transformation_name(uint8_t transformation)
{
    switch (transformation) {
    case BT_XSTACK_COVER_ART_TRANSFORMATION_NONE: return NULL;
    case BT_XSTACK_COVER_ART_TRANSFORMATION_STRETCH: return "stretch";
    case BT_XSTACK_COVER_ART_TRANSFORMATION_FILL: return "fill";
    case BT_XSTACK_COVER_ART_TRANSFORMATION_CROP: return "crop";
    default: return NULL;
    }
}

static int btstack_bip_build_descriptor(const bt_xstack_cover_art_image_descriptor_t *descriptor)
{
    const char *encoding;
    const char *transformation;
    int length;

    if (descriptor == NULL || descriptor->width == 0u || descriptor->height == 0u) return BTSTACK_STATUS_INVALID_ARGUMENT;
    encoding = btstack_bip_encoding_name(descriptor->encoding);
    if (encoding == NULL) return BTSTACK_STATUS_INVALID_ARGUMENT;
    transformation = btstack_bip_transformation_name(descriptor->transformation);
    if (descriptor->transformation != BT_XSTACK_COVER_ART_TRANSFORMATION_NONE && transformation == NULL) return BTSTACK_STATUS_INVALID_ARGUMENT;
    if (transformation == NULL) {
        length = snprintf(btstack_bip_client.descriptor, sizeof(btstack_bip_client.descriptor),
            "<image-descriptor version=\"1.0\"><image encoding=\"%s\" pixel=\"%u*%u\" /></image-descriptor>",
            encoding, (unsigned)descriptor->width, (unsigned)descriptor->height);
    } else {
        length = snprintf(btstack_bip_client.descriptor, sizeof(btstack_bip_client.descriptor),
            "<image-descriptor version=\"1.0\"><image encoding=\"%s\" pixel=\"%u*%u\" transformation=\"%s\" /></image-descriptor>",
            encoding, (unsigned)descriptor->width, (unsigned)descriptor->height, transformation);
    }
    if (length < 0 || (size_t)length >= sizeof(btstack_bip_client.descriptor)) return BTSTACK_STATUS_INVALID_ARGUMENT;
    return BTSTACK_STATUS_OK;
}

static uint8_t btstack_bip_native_get(void)
{
    switch (btstack_bip_client.operation) {
    case BTSTACK_BIP_OPERATION_LINKED_THUMBNAIL:
        btstack_bip_client.state = BTSTACK_BIP_STATE_GETTING_THUMBNAIL;
        return avrcp_cover_art_client_get_linked_thumbnail(btstack_bip_client.cover_art_cid, btstack_bip_client.image_handle);
    case BTSTACK_BIP_OPERATION_IMAGE_PROPERTIES:
        btstack_bip_client.state = BTSTACK_BIP_STATE_GETTING_PROPERTIES;
        btstack_bip_properties_reset(btstack_bip_client.generation);
        return avrcp_cover_art_client_get_image_properties(btstack_bip_client.cover_art_cid, btstack_bip_client.image_handle);
    case BTSTACK_BIP_OPERATION_IMAGE:
        btstack_bip_client.state = BTSTACK_BIP_STATE_GETTING_IMAGE;
        return avrcp_cover_art_client_get_image(btstack_bip_client.cover_art_cid, btstack_bip_client.image_handle, btstack_bip_client.descriptor);
    default:
        return ERROR_CODE_COMMAND_DISALLOWED;
    }
}

static void btstack_bip_start_native_get(void)
{
    btstack_bip_state_t expected_state;
    uint8_t status = btstack_bip_native_get();

    expected_state = btstack_bip_client.state;
    if (status != ERROR_CODE_SUCCESS && btstack_bip_client.state == expected_state) btstack_bip_start_cleanup(BTSTACK_STATUS_ERROR, status);
}

static int btstack_bip_start_operation(btstack_bip_operation_t operation)
{
    uint8_t status;
    uint32_t generation;

    if (!btstack_bip_client.object_valid) return BTSTACK_STATUS_NOT_READY;
    if (btstack_bip_client.operation != BTSTACK_BIP_OPERATION_NONE ||
        (btstack_bip_client.state != BTSTACK_BIP_STATE_IDLE && btstack_bip_client.state != BTSTACK_BIP_STATE_CONNECTED)) return BTSTACK_STATUS_BUSY;
    btstack_bip_client.operation = operation;
    btstack_bip_client.data_offset = 0u;
    generation = btstack_bip_client.generation;
    btstack_bip_emit_started();
    if (btstack_bip_client.operation != operation || btstack_bip_client.generation != generation) return BTSTACK_STATUS_OK;
    if (btstack_bip_client.state == BTSTACK_BIP_STATE_CONNECTED) {
        btstack_bip_start_native_get();
        return BTSTACK_STATUS_OK;
    }
    btstack_bip_client.state = BTSTACK_BIP_STATE_CONNECTING;
    status = avrcp_cover_art_client_connect(&btstack_bip_client.native_client, btstack_bip_native_packet_handler,
        btstack_bip_client.address, btstack_bip_client.ertm_buffer, sizeof(btstack_bip_client.ertm_buffer),
        &btstack_bip_client.ertm_config, &btstack_bip_client.cover_art_cid);
    if (status != ERROR_CODE_SUCCESS && btstack_bip_client.state == BTSTACK_BIP_STATE_CONNECTING) {
        btstack_bip_finish_error(BTSTACK_STATUS_ERROR, status);
    }
    return status == ERROR_CODE_SUCCESS ? BTSTACK_STATUS_OK : BTSTACK_STATUS_ERROR;
}

static void btstack_bip_handle_data(const uint8_t *data, uint16_t length)
{
    btstack_bip_event_t event;
    btstack_bip_operation_t operation = btstack_bip_client.operation;
    uint32_t generation = btstack_bip_client.generation;
    uint32_t offset = btstack_bip_client.data_offset;

    if (btstack_bip_client.state == BTSTACK_BIP_STATE_GETTING_PROPERTIES) {
        btstack_bip_properties_feed(data, length, 0u);
        if (btstack_bip_properties.failed) btstack_bip_start_cleanup(BTSTACK_STATUS_ERROR, 0u);
        return;
    }
    if (btstack_bip_client.state != BTSTACK_BIP_STATE_GETTING_IMAGE && btstack_bip_client.state != BTSTACK_BIP_STATE_GETTING_THUMBNAIL) return;
    if (btstack_bip_callback != NULL) {
        memset(&event, 0, sizeof(event));
        event.type = BTSTACK_BIP_EVENT_DATA;
        event.operation = operation;
        event.status = BTSTACK_STATUS_OK;
        event.generation = generation;
        event.data.data.data = data;
        event.data.data.length = length;
        event.data.data.offset = offset;
        btstack_bip_callback(&event);
    }
    if (btstack_bip_client.operation == operation && btstack_bip_client.generation == generation &&
        (btstack_bip_client.state == BTSTACK_BIP_STATE_GETTING_IMAGE || btstack_bip_client.state == BTSTACK_BIP_STATE_GETTING_THUMBNAIL)) {
        btstack_bip_client.data_offset += length;
    }
}

static void btstack_bip_handle_operation_complete(uint8_t status)
{
    if (status != ERROR_CODE_SUCCESS) {
        btstack_bip_start_cleanup(BTSTACK_STATUS_ERROR, status);
        return;
    }
    if (btstack_bip_client.state == BTSTACK_BIP_STATE_GETTING_PROPERTIES) {
        btstack_bip_properties_feed(NULL, 0u, 1u);
        if (btstack_bip_properties.failed) {
            btstack_bip_start_cleanup(BTSTACK_STATUS_ERROR, 0u);
        } else {
            btstack_bip_client.operation = BTSTACK_BIP_OPERATION_NONE;
            btstack_bip_client.state = BTSTACK_BIP_STATE_CONNECTED;
            btstack_bip_emit_properties_completed();
        }
        return;
    }
    if (btstack_bip_client.state == BTSTACK_BIP_STATE_GETTING_IMAGE || btstack_bip_client.state == BTSTACK_BIP_STATE_GETTING_THUMBNAIL) btstack_bip_finish_success();
}

static void btstack_bip_native_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    uint8_t subevent;
    uint16_t cid;

    (void)size;
    if (packet_type == BIP_DATA_PACKET) {
        if (channel == btstack_bip_client.cover_art_cid) btstack_bip_handle_data(packet, size);
        return;
    }
    if (packet_type != HCI_EVENT_PACKET || hci_event_packet_get_type(packet) != HCI_EVENT_AVRCP_META) return;
    subevent = hci_event_avrcp_meta_get_subevent_code(packet);
    switch (subevent) {
    case AVRCP_SUBEVENT_COVER_ART_CONNECTION_ESTABLISHED:
        if (btstack_bip_client.state != BTSTACK_BIP_STATE_CONNECTING) return;
        if (avrcp_subevent_cover_art_connection_established_get_status(packet) != ERROR_CODE_SUCCESS) {
            uint8_t status = avrcp_subevent_cover_art_connection_established_get_status(packet);

            if (btstack_bip_client.operation != BTSTACK_BIP_OPERATION_NONE) {
                btstack_bip_finish_error(BTSTACK_STATUS_ERROR, status);
            } else {
                btstack_bip_reset_connection();
                btstack_bip_emit_connection_established(BTSTACK_STATUS_ERROR, status);
            }
            return;
        }
        btstack_bip_client.cover_art_cid = avrcp_subevent_cover_art_connection_established_get_cover_art_cid(packet);
        btstack_bip_client.state = BTSTACK_BIP_STATE_CONNECTED;
        if (btstack_bip_client.operation != BTSTACK_BIP_OPERATION_NONE) btstack_bip_start_native_get();
        else btstack_bip_emit_connection_established(BTSTACK_STATUS_OK, 0u);
        break;
    case AVRCP_SUBEVENT_COVER_ART_OPERATION_COMPLETE:
        cid = avrcp_subevent_cover_art_operation_complete_get_cover_art_cid(packet);
        if (cid == btstack_bip_client.cover_art_cid) btstack_bip_handle_operation_complete(avrcp_subevent_cover_art_operation_complete_get_status(packet));
        break;
    case AVRCP_SUBEVENT_COVER_ART_CONNECTION_RELEASED:
        cid = avrcp_subevent_cover_art_connection_released_get_cover_art_cid(packet);
        if (cid != btstack_bip_client.cover_art_cid) return;
        if (btstack_bip_client.terminal_pending) {
            int status = btstack_bip_client.terminal_status;
            uint8_t native_status = btstack_bip_client.terminal_native_status;

            btstack_bip_finish_error(status, native_status);
        } else if (btstack_bip_client.operation != BTSTACK_BIP_OPERATION_NONE) {
            btstack_bip_finish_error(BTSTACK_STATUS_DISCONNECTED, 0u);
        } else {
            btstack_bip_reset_connection();
        }
        break;
    default:
        break;
    }
}

int btstack_bip_client_init(btstack_bip_event_callback_t callback)
{
    memset(&btstack_bip_client, 0, sizeof(btstack_bip_client));
    btstack_bip_client.ertm_config.ertm_mandatory = 1u;
    btstack_bip_client.ertm_config.max_transmit = 2u;
    btstack_bip_client.ertm_config.retransmission_timeout_ms = 2000u;
    btstack_bip_client.ertm_config.monitor_timeout_ms = 12000u;
    btstack_bip_client.ertm_config.local_mtu = 512u;
    btstack_bip_client.ertm_config.num_tx_buffers = 2u;
    btstack_bip_client.ertm_config.num_rx_buffers = 2u;
    btstack_bip_client.ertm_config.fcs_option = 1u;
    btstack_bip_callback = callback;
    avrcp_cover_art_client_init();
    return BTSTACK_STATUS_OK;
}

int btstack_bip_client_connect(const btstack_bd_addr_t address)
{
    uint8_t status;

    if (address == NULL) return BTSTACK_STATUS_INVALID_ARGUMENT;
    if (!avrcp_cover_art_client_is_available((uint8_t *)address)) return BTSTACK_STATUS_NOT_SUPPORTED;
    if (btstack_bip_client.state == BTSTACK_BIP_STATE_CONNECTED &&
        memcmp(address, btstack_bip_client.address, sizeof(btstack_bd_addr_t)) == 0) {
        btstack_bip_emit_connection_established(BTSTACK_STATUS_OK, 0u);
        return BTSTACK_STATUS_OK;
    }
    if (btstack_bip_client.state == BTSTACK_BIP_STATE_CONNECTING &&
        memcmp(address, btstack_bip_client.address, sizeof(btstack_bd_addr_t)) == 0) return BTSTACK_STATUS_OK;
    if (btstack_bip_client.state != BTSTACK_BIP_STATE_IDLE ||
        btstack_bip_client.operation != BTSTACK_BIP_OPERATION_NONE) return BTSTACK_STATUS_BUSY;
    memcpy(btstack_bip_client.address, address, sizeof(btstack_bd_addr_t));
    btstack_bip_client.object_valid = 0u;
    btstack_bip_client.state = BTSTACK_BIP_STATE_CONNECTING;
    status = avrcp_cover_art_client_connect(&btstack_bip_client.native_client, btstack_bip_native_packet_handler,
        btstack_bip_client.address, btstack_bip_client.ertm_buffer, sizeof(btstack_bip_client.ertm_buffer),
        &btstack_bip_client.ertm_config, &btstack_bip_client.cover_art_cid);
    if (status != ERROR_CODE_SUCCESS && btstack_bip_client.state == BTSTACK_BIP_STATE_CONNECTING) {
        btstack_bip_reset_connection();
        btstack_bip_emit_connection_established(BTSTACK_STATUS_ERROR, status);
    }
    return status == ERROR_CODE_SUCCESS ? BTSTACK_STATUS_OK : BTSTACK_STATUS_ERROR;
}

int btstack_bip_client_set_object(const btstack_bd_addr_t address, const char image_handle[8], uint32_t generation)
{
    unsigned index;

    if (address == NULL || image_handle == NULL || generation == 0u) return BTSTACK_STATUS_INVALID_ARGUMENT;
    if (btstack_bip_client.operation != BTSTACK_BIP_OPERATION_NONE) return BTSTACK_STATUS_BUSY;
    for (index = 0u; index < 7u; index++) if (image_handle[index] == '\0') return BTSTACK_STATUS_INVALID_ARGUMENT;
    if (image_handle[7] != '\0') return BTSTACK_STATUS_INVALID_ARGUMENT;
    memcpy(btstack_bip_client.address, address, sizeof(btstack_bd_addr_t));
    memcpy(btstack_bip_client.image_handle, image_handle, sizeof(btstack_bip_client.image_handle));
    btstack_bip_client.generation = generation;
    btstack_bip_client.object_valid = 1u;
    return BTSTACK_STATUS_OK;
}

int btstack_bip_client_get_linked_thumbnail(void)
{
    return btstack_bip_start_operation(BTSTACK_BIP_OPERATION_LINKED_THUMBNAIL);
}

int btstack_bip_client_get_image_properties(void)
{
    return btstack_bip_start_operation(BTSTACK_BIP_OPERATION_IMAGE_PROPERTIES);
}

int btstack_bip_client_get_image(const bt_xstack_cover_art_image_descriptor_t *descriptor)
{
    int status = btstack_bip_build_descriptor(descriptor);

    if (status != BTSTACK_STATUS_OK) return status;
    return btstack_bip_start_operation(BTSTACK_BIP_OPERATION_IMAGE);
}

int btstack_bip_client_cancel(void)
{
    if (btstack_bip_client.operation == BTSTACK_BIP_OPERATION_NONE) return BTSTACK_STATUS_NOT_READY;
    btstack_bip_start_cleanup(BTSTACK_STATUS_DISCONNECTED, 0u);
    return BTSTACK_STATUS_OK;
}

void btstack_bip_client_peer_disconnected(const btstack_bd_addr_t address)
{
    if (address == NULL || memcmp(address, btstack_bip_client.address, sizeof(btstack_bd_addr_t)) != 0) return;
    if (btstack_bip_client.operation != BTSTACK_BIP_OPERATION_NONE) btstack_bip_start_cleanup(BTSTACK_STATUS_DISCONNECTED, 0u);
    else if (btstack_bip_client.state == BTSTACK_BIP_STATE_CONNECTED) {
        btstack_bip_client.state = BTSTACK_BIP_STATE_DISCONNECTING;
        if (avrcp_cover_art_client_disconnect(btstack_bip_client.cover_art_cid) != ERROR_CODE_SUCCESS) btstack_bip_reset_connection();
    }
}
#else
int btstack_bip_client_init(btstack_bip_event_callback_t callback) { btstack_bip_callback = callback; return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_bip_client_connect(const btstack_bd_addr_t address) { (void)address; return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_bip_client_set_object(const btstack_bd_addr_t address, const char image_handle[8], uint32_t generation) { (void)address; (void)image_handle; (void)generation; return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_bip_client_get_linked_thumbnail(void) { return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_bip_client_get_image_properties(void) { return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_bip_client_get_image(const bt_xstack_cover_art_image_descriptor_t *descriptor) { (void)descriptor; return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_bip_client_cancel(void) { return BTSTACK_STATUS_NOT_SUPPORTED; }
void btstack_bip_client_peer_disconnected(const btstack_bd_addr_t address) { (void)address; }
#endif

#ifdef BTSTACK_BIP_CLIENT_TESTING
void btstack_bip_client_test_properties_reset(uint32_t generation)
{
    btstack_bip_properties_reset(generation);
}

int btstack_bip_client_test_properties_feed(const uint8_t *data, uint16_t length, uint8_t final_chunk)
{
    uint8_t was_failed;
    uint8_t was_completed;

    if (data == NULL && length != 0u) return BTSTACK_STATUS_INVALID_ARGUMENT;
    was_failed = btstack_bip_properties.failed;
    was_completed = btstack_bip_properties.completed;
    btstack_bip_properties_feed(data, length, final_chunk);
    if (!was_failed && btstack_bip_properties.failed) btstack_bip_emit_error();
    if (!was_completed && btstack_bip_properties.completed) btstack_bip_emit_properties_completed();
    return BTSTACK_STATUS_OK;
}
#endif

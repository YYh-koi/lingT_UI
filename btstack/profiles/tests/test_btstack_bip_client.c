#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "profiles/btstack_bip_client.h"
#include "btstack_defines.h"
#include "classic/avrcp_cover_art_client.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

static btstack_bip_event_t events[16];
static unsigned event_count;
static btstack_packet_handler_t native_handler;
static uint16_t native_cid = 0x1234u;
static unsigned connect_calls;
static unsigned thumbnail_calls;
static unsigned properties_calls;
static unsigned image_calls;
static unsigned disconnect_calls;
static uint8_t connect_result;
static uint8_t get_result;
static uint8_t disconnect_result;
static uint8_t synchronous_connect;
static uint8_t synchronous_get;
static uint8_t synchronous_disconnect;
static uint8_t cover_art_available;
static btstack_bd_addr_t connected_address;
static char requested_handle[8];
static char requested_descriptor[192];
static uint8_t reenter_get_image;
static uint8_t reenter_cancel_on_data;
static uint8_t reenter_after_error;
static int reentry_result;
static bt_xstack_cover_art_image_descriptor_t reentry_descriptor;

static void emit_connection(uint8_t status)
{
    uint8_t packet[14] = { HCI_EVENT_AVRCP_META, 12u, AVRCP_SUBEVENT_COVER_ART_CONNECTION_ESTABLISHED, 0u };

    packet[3] = status;
    packet[12] = (uint8_t)native_cid;
    packet[13] = (uint8_t)(native_cid >> 8);
    native_handler(HCI_EVENT_PACKET, 0u, packet, sizeof(packet));
}

static void emit_operation(uint16_t cid, uint8_t status)
{
    uint8_t packet[6] = { HCI_EVENT_AVRCP_META, 4u, AVRCP_SUBEVENT_COVER_ART_OPERATION_COMPLETE, 0u, 0u, 0u };

    packet[3] = (uint8_t)cid;
    packet[4] = (uint8_t)(cid >> 8);
    packet[5] = status;
    native_handler(HCI_EVENT_PACKET, 0u, packet, sizeof(packet));
}

static void emit_released(uint16_t cid)
{
    uint8_t packet[5] = { HCI_EVENT_AVRCP_META, 3u, AVRCP_SUBEVENT_COVER_ART_CONNECTION_RELEASED, 0u, 0u };

    packet[3] = (uint8_t)cid;
    packet[4] = (uint8_t)(cid >> 8);
    native_handler(HCI_EVENT_PACKET, 0u, packet, sizeof(packet));
}

void avrcp_cover_art_client_init(void) { }
bool avrcp_cover_art_client_is_available(bd_addr_t address)
{
    CHECK(address != NULL);
    return cover_art_available != 0u;
}

uint8_t avrcp_cover_art_client_connect(avrcp_cover_art_client_t *client, btstack_packet_handler_t handler,
    bd_addr_t address, uint8_t *ertm_buffer, uint32_t ertm_buffer_size,
    const l2cap_ertm_config_t *ertm_config, uint16_t *cid)
{
    (void)client;
    CHECK(ertm_buffer != NULL && ertm_buffer_size == 2000u && ertm_config != NULL);
    connect_calls++;
    native_handler = handler;
    memcpy(connected_address, address, sizeof(connected_address));
    *cid = native_cid;
    if (synchronous_connect && connect_result == ERROR_CODE_SUCCESS) emit_connection(ERROR_CODE_SUCCESS);
    return connect_result;
}

static uint8_t record_get(const char *handle)
{
    memcpy(requested_handle, handle, sizeof(requested_handle));
    if (synchronous_get && get_result == ERROR_CODE_SUCCESS) emit_operation(native_cid, ERROR_CODE_SUCCESS);
    return get_result;
}

uint8_t avrcp_cover_art_client_get_linked_thumbnail(uint16_t cid, const char *handle)
{
    CHECK(cid == native_cid);
    thumbnail_calls++;
    return record_get(handle);
}

uint8_t avrcp_cover_art_client_get_image_properties(uint16_t cid, const char *handle)
{
    CHECK(cid == native_cid);
    properties_calls++;
    return record_get(handle);
}

uint8_t avrcp_cover_art_client_get_image(uint16_t cid, const char *handle, const char *descriptor)
{
    CHECK(cid == native_cid);
    image_calls++;
    strncpy(requested_descriptor, descriptor, sizeof(requested_descriptor) - 1u);
    requested_descriptor[sizeof(requested_descriptor) - 1u] = '\0';
    return record_get(handle);
}

uint8_t avrcp_cover_art_client_disconnect(uint16_t cid)
{
    CHECK(cid == native_cid);
    disconnect_calls++;
    if (synchronous_disconnect && disconnect_result == ERROR_CODE_SUCCESS) emit_released(cid);
    return disconnect_result;
}

void avrcp_cover_art_client_deinit(void) { }

static void on_event(const btstack_bip_event_t *event)
{
    static const btstack_bd_addr_t retry_address = { 9u, 8u, 7u, 6u, 5u, 4u };

    CHECK(event_count < 16u);
    events[event_count++] = *event;
    if (reenter_get_image && event->type == BTSTACK_BIP_EVENT_PROPERTIES_COMPLETED) {
        reenter_get_image = 0u;
        reentry_result = btstack_bip_client_get_image(&reentry_descriptor);
    }
    if (reenter_cancel_on_data && event->type == BTSTACK_BIP_EVENT_DATA) {
        reenter_cancel_on_data = 0u;
        reentry_result = btstack_bip_client_cancel();
    }
    if (reenter_after_error && event->type == BTSTACK_BIP_EVENT_ERROR) {
        reenter_after_error = 0u;
        connect_result = ERROR_CODE_SUCCESS;
        reentry_result = btstack_bip_client_set_object(
            retry_address, "2222222", 99u);
        if (reentry_result == BTSTACK_STATUS_OK) {
            reentry_result = btstack_bip_client_get_linked_thumbnail();
        }
    }
}

static void reset_parser(void)
{
    event_count = 0u;
    memset(events, 0, sizeof(events));
    btstack_bip_client_test_properties_reset(7u);
}

static void reset_native(void)
{
    event_count = 0u;
    connect_calls = 0u;
    thumbnail_calls = 0u;
    properties_calls = 0u;
    image_calls = 0u;
    disconnect_calls = 0u;
    connect_result = ERROR_CODE_SUCCESS;
    get_result = ERROR_CODE_SUCCESS;
    disconnect_result = ERROR_CODE_SUCCESS;
    synchronous_connect = 0u;
    synchronous_get = 0u;
    synchronous_disconnect = 0u;
    cover_art_available = 1u;
    reenter_get_image = 0u;
    reenter_cancel_on_data = 0u;
    reenter_after_error = 0u;
    reentry_result = BTSTACK_STATUS_ERROR;
    memset(requested_handle, 0, sizeof(requested_handle));
    memset(requested_descriptor, 0, sizeof(requested_descriptor));
    CHECK(btstack_bip_client_init(on_event) == BTSTACK_STATUS_OK);
}

static void test_preconnect_requires_sdp_support(void)
{
    static const btstack_bd_addr_t address = { 2u, 3u, 4u, 5u, 6u, 7u };

    reset_native();
    cover_art_available = 0u;
    CHECK(btstack_bip_client_connect(address) == BTSTACK_STATUS_NOT_SUPPORTED);
    CHECK(connect_calls == 0u && event_count == 0u);

    cover_art_available = 1u;
    CHECK(btstack_bip_client_connect(address) == BTSTACK_STATUS_OK);
    CHECK(connect_calls == 1u && event_count == 0u);
    emit_connection(ERROR_CODE_SUCCESS);
    CHECK(event_count == 1u);
    CHECK(events[0].type == BTSTACK_BIP_EVENT_CONNECTION_ESTABLISHED);
    CHECK(events[0].status == BTSTACK_STATUS_OK);
    CHECK(events[0].operation == BTSTACK_BIP_OPERATION_NONE);
    CHECK(thumbnail_calls == 0u && properties_calls == 0u && image_calls == 0u);

    CHECK(btstack_bip_client_set_object(address, "0123456", 15u) == BTSTACK_STATUS_OK);
    CHECK(btstack_bip_client_get_linked_thumbnail() == BTSTACK_STATUS_OK);
    CHECK(connect_calls == 1u && thumbnail_calls == 1u);
}

static void feed(const char *text, size_t length, uint8_t final_chunk)
{
    CHECK(length <= UINT16_MAX);
    CHECK(btstack_bip_client_test_properties_feed((const uint8_t *)text, (uint16_t)length, final_chunk) == BTSTACK_STATUS_OK);
}

static void test_split_properties(void)
{
    static const char xml[] =
        "<image-properties version=\"1.0\" handle=\"0123456\">"
        "<native encoding=\"JPEG\" pixel=\"640*640\" size=\"182340\" />"
        "<variant encoding=\"JPEG\" pixel=\"200*200\" transformation=\"stretch fill\" />"
        "<variant encoding=\"PNG\" pixel=\"100*100-300*300\" />"
        "</image-properties>";
    size_t index;

    reset_parser();
    for (index = 0u; index < sizeof(xml) - 1u; index++) {
        feed(&xml[index], 1u, index + 1u == sizeof(xml) - 1u);
    }
    CHECK(event_count == 4u);
    CHECK(events[0].type == BTSTACK_BIP_EVENT_PROPERTY);
    CHECK(events[0].data.property.encoding == BT_XSTACK_COVER_ART_ENCODING_JPEG);
    CHECK(events[0].data.property.min_width == 640u && events[0].data.property.max_width == 640u);
    CHECK(events[0].data.property.min_height == 640u && events[0].data.property.max_height == 640u);
    CHECK(events[0].data.property.file_size_known == 1u && events[0].data.property.file_size == 182340u);
    CHECK(events[0].data.property.native_image == 1u);
    CHECK(events[1].data.property.transformations == (BT_XSTACK_COVER_ART_TRANSFORMATION_STRETCH | BT_XSTACK_COVER_ART_TRANSFORMATION_FILL));
    CHECK(events[2].data.property.encoding == BT_XSTACK_COVER_ART_ENCODING_PNG);
    CHECK(events[2].data.property.min_width == 100u && events[2].data.property.max_width == 300u);
    CHECK(events[2].data.property.min_height == 100u && events[2].data.property.max_height == 300u);
    CHECK(events[3].type == BTSTACK_BIP_EVENT_PROPERTIES_COMPLETED);
    CHECK(events[3].data.properties_completed.property_count == 3u);
    CHECK(events[3].generation == 7u);
}

static void expect_error(const char *xml)
{
    static const char ignored_after_error[] = "<native encoding=\"JPEG\" pixel=\"1*1\"/>";

    reset_parser();
    feed(xml, strlen(xml), 1u);
    CHECK(event_count == 1u);
    CHECK(events[0].type == BTSTACK_BIP_EVENT_ERROR);
    feed(ignored_after_error, sizeof(ignored_after_error) - 1u, 1u);
    CHECK(event_count == 1u);
}

static void test_native_session(void)
{
    static const btstack_bd_addr_t address = { 1u, 2u, 3u, 4u, 5u, 6u };
    static const uint8_t first[] = { 1u, 2u, 3u };
    static const uint8_t second[] = { 4u, 5u };
    bt_xstack_cover_art_image_descriptor_t descriptor = {
        BT_XSTACK_COVER_ART_ENCODING_JPEG, 320u, 240u,
        BT_XSTACK_COVER_ART_TRANSFORMATION_CROP
    };

    reset_native();
    CHECK(btstack_bip_client_set_object(address, "0123456", 9u) == BTSTACK_STATUS_OK);
    CHECK(btstack_bip_client_get_linked_thumbnail() == BTSTACK_STATUS_OK);
    CHECK(connect_calls == 1u && memcmp(address, connected_address, sizeof(address)) == 0);
    CHECK(btstack_bip_client_get_image_properties() == BTSTACK_STATUS_BUSY);
    emit_connection(ERROR_CODE_SUCCESS);
    CHECK(thumbnail_calls == 1u && strcmp(requested_handle, "0123456") == 0);
    native_handler(BIP_DATA_PACKET, native_cid, (uint8_t *)first, sizeof(first));
    native_handler(BIP_DATA_PACKET, native_cid, (uint8_t *)second, sizeof(second));
    CHECK(events[1].type == BTSTACK_BIP_EVENT_DATA && events[1].data.data.offset == 0u);
    CHECK(events[2].type == BTSTACK_BIP_EVENT_DATA && events[2].data.data.offset == 3u);
    emit_operation(native_cid, ERROR_CODE_SUCCESS);
    CHECK(events[3].type == BTSTACK_BIP_EVENT_COMPLETED && events[3].data.completed.total_length == 5u);

    CHECK(btstack_bip_client_get_image(&descriptor) == BTSTACK_STATUS_OK);
    CHECK(image_calls == 1u);
    CHECK(strcmp(requested_descriptor, "<image-descriptor version=\"1.0\"><image encoding=\"JPEG\" pixel=\"320*240\" transformation=\"crop\" /></image-descriptor>") == 0);
    emit_operation(native_cid, ERROR_CODE_SUCCESS);
}

static void test_properties_session(void)
{
    static const btstack_bd_addr_t address = { 7u, 8u, 9u, 10u, 11u, 12u };
    static const char xml[] = "<image-properties><native encoding=\"JPEG\" pixel=\"64*64\"/></image-properties>";

    reset_native();
    reentry_descriptor.encoding = BT_XSTACK_COVER_ART_ENCODING_JPEG;
    reentry_descriptor.width = 64u;
    reentry_descriptor.height = 64u;
    reentry_descriptor.transformation = BT_XSTACK_COVER_ART_TRANSFORMATION_NONE;
    reenter_get_image = 1u;
    CHECK(btstack_bip_client_set_object(address, "7654321", 10u) == BTSTACK_STATUS_OK);
    CHECK(btstack_bip_client_get_image_properties() == BTSTACK_STATUS_OK);
    emit_connection(ERROR_CODE_SUCCESS);
    CHECK(properties_calls == 1u);
    native_handler(BIP_DATA_PACKET, native_cid, (uint8_t *)xml, sizeof(xml) - 1u);
    emit_operation(native_cid, ERROR_CODE_SUCCESS);
    CHECK(events[event_count - 2u].type == BTSTACK_BIP_EVENT_PROPERTIES_COMPLETED);
    CHECK(events[event_count - 2u].data.properties_completed.property_count == 1u);
    CHECK(reentry_result == BTSTACK_STATUS_OK && image_calls == 1u);
    emit_operation(native_cid, ERROR_CODE_SUCCESS);
}

static void test_failures_and_synchronous_callbacks(void)
{
    static const btstack_bd_addr_t address = { 1u, 1u, 1u, 1u, 1u, 1u };

    reset_native();
    connect_result = ERROR_CODE_COMMAND_DISALLOWED;
    reenter_after_error = 1u;
    CHECK(btstack_bip_client_set_object(address, "1111111", 11u) == BTSTACK_STATUS_OK);
    CHECK(btstack_bip_client_get_linked_thumbnail() == BTSTACK_STATUS_ERROR);
    CHECK(events[1].type == BTSTACK_BIP_EVENT_ERROR);
    CHECK(reentry_result == BTSTACK_STATUS_OK && connect_calls == 2u);
    emit_connection(ERROR_CODE_SUCCESS);
    emit_operation(native_cid, ERROR_CODE_SUCCESS);

    reset_native();
    synchronous_connect = 1u;
    synchronous_get = 1u;
    CHECK(btstack_bip_client_set_object(address, "1111111", 12u) == BTSTACK_STATUS_OK);
    CHECK(btstack_bip_client_get_linked_thumbnail() == BTSTACK_STATUS_OK);
    CHECK(events[event_count - 1u].type == BTSTACK_BIP_EVENT_COMPLETED);
    synchronous_get = 0u;
    CHECK(btstack_bip_client_get_linked_thumbnail() == BTSTACK_STATUS_OK);
    emit_operation((uint16_t)(native_cid + 1u), ERROR_CODE_SUCCESS);
    CHECK(btstack_bip_client_cancel() == BTSTACK_STATUS_OK);
    emit_released((uint16_t)(native_cid + 1u));
    CHECK(event_count >= 3u);
    emit_released(native_cid);
    CHECK(events[event_count - 1u].type == BTSTACK_BIP_EVENT_ERROR);

    reset_native();
    get_result = ERROR_CODE_COMMAND_DISALLOWED;
    disconnect_result = ERROR_CODE_COMMAND_DISALLOWED;
    CHECK(btstack_bip_client_set_object(address, "1111111", 13u) == BTSTACK_STATUS_OK);
    CHECK(btstack_bip_client_get_linked_thumbnail() == BTSTACK_STATUS_OK);
    emit_connection(ERROR_CODE_SUCCESS);
    CHECK(disconnect_calls == 1u && events[event_count - 1u].type == BTSTACK_BIP_EVENT_ERROR);
}

static void test_data_callback_cancel_and_late_data(void)
{
    static const btstack_bd_addr_t address = { 2u, 2u, 2u, 2u, 2u, 2u };
    static uint8_t data[] = { 4u, 5u, 6u };
    unsigned before;

    reset_native();
    CHECK(btstack_bip_client_set_object(address, "3333333", 14u) ==
        BTSTACK_STATUS_OK);
    CHECK(btstack_bip_client_get_linked_thumbnail() == BTSTACK_STATUS_OK);
    emit_connection(ERROR_CODE_SUCCESS);
    reenter_cancel_on_data = 1u;
    native_handler(BIP_DATA_PACKET, native_cid, data, sizeof(data));
    CHECK(reentry_result == BTSTACK_STATUS_OK && disconnect_calls == 1u);
    before = event_count;
    native_handler(BIP_DATA_PACKET, native_cid, data, sizeof(data));
    CHECK(event_count == before);
    emit_released(native_cid);
    CHECK(events[event_count - 1u].type == BTSTACK_BIP_EVENT_ERROR);
    before = event_count;
    emit_operation(native_cid, ERROR_CODE_SUCCESS);
    emit_released(native_cid);
    CHECK(event_count == before);
}

int main(void)
{
    CHECK(btstack_bip_client_init(on_event) == BTSTACK_STATUS_OK);
    test_split_properties();
    expect_error("<image-properties><native encoding=\"TIFF\" pixel=\"1*1\"/></image-properties>");
    expect_error("<image-properties><native encoding=\"JPEG\" pixel=\"70000*1\"/></image-properties>");
    expect_error("<image-properties><variant encoding=\"PNG\" pixel=\"300*300-100*100\"/></image-properties>");
    expect_error("<image-properties><native encoding=\"JPEG\" pixel=\"1*1\" size=\"4294967296\"/></image-properties>");
    expect_error("<image-properties><native pixel=\"1*1\"/></image-properties>");
    expect_error("<image-properties><native encoding=\"JPEG\"/></image-properties>");
    expect_error("<image-properties><native encoding=\"JPEG\" pixel=\"1*1\"></variant></image-properties>");
    test_preconnect_requires_sdp_support();
    test_native_session();
    test_properties_session();
    test_failures_and_synchronous_callbacks();
    test_data_callback_cancel_and_late_data();
    puts("PASS: incremental BIP image-properties parser");
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "profiles/btstack_avrcp_cover_art.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

static btstack_bip_event_callback_t bip_callback;
static unsigned set_object_calls;
static unsigned thumbnail_calls;
static unsigned properties_calls;
static unsigned image_calls;
static unsigned cancel_calls;
static unsigned peer_disconnect_calls;
static unsigned connect_calls;
static unsigned metadata_calls;
static unsigned forwarded_events;
static unsigned forwarded_data_events;
static unsigned forwarded_completed_events;
static btstack_bd_addr_t last_address;
static char last_handle[8];
static uint32_t last_generation;
static int start_result = BTSTACK_STATUS_OK;
static uint8_t replace_pending_in_callback;

int btstack_bip_client_init(btstack_bip_event_callback_t callback) { bip_callback = callback; return BTSTACK_STATUS_OK; }
int btstack_bip_client_connect(const btstack_bd_addr_t address)
{
    CHECK(address != NULL);
    connect_calls++;
    memcpy(last_address, address, sizeof(last_address));
    return start_result;
}
int btstack_bip_client_set_object(const btstack_bd_addr_t address, const char image_handle[8], uint32_t generation)
{
    set_object_calls++;
    memcpy(last_address, address, sizeof(last_address));
    memcpy(last_handle, image_handle, sizeof(last_handle));
    last_generation = generation;
    return BTSTACK_STATUS_OK;
}
int btstack_bip_client_get_linked_thumbnail(void) { thumbnail_calls++; return start_result; }
int btstack_bip_client_get_image_properties(void) { properties_calls++; return start_result; }
int btstack_bip_client_get_image(const bt_xstack_cover_art_image_descriptor_t *descriptor) { CHECK(descriptor != NULL); image_calls++; return start_result; }
int btstack_bip_client_cancel(void) { cancel_calls++; return BTSTACK_STATUS_OK; }
void btstack_bip_client_peer_disconnected(const btstack_bd_addr_t address) { CHECK(address != NULL); peer_disconnect_calls++; }
int btstack_avrcp_controller_request_metadata(void) { metadata_calls++; return BTSTACK_STATUS_OK; }

static void on_event(const btstack_bip_event_t *event)
{
    CHECK(event != NULL);
    forwarded_events++;
    if (event->type == BTSTACK_BIP_EVENT_DATA) forwarded_data_events++;
    if (event->type == BTSTACK_BIP_EVENT_COMPLETED) forwarded_completed_events++;
    if (replace_pending_in_callback && event->type == BTSTACK_BIP_EVENT_ERROR) {
        replace_pending_in_callback = 0u;
        btstack_avrcp_cover_art_handle_received((const uint8_t *)"3333333", 7u);
    }
}

static void emit_terminal(btstack_bip_event_type_t type, uint32_t generation)
{
    btstack_bip_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.generation = generation;
    bip_callback(&event);
}

static void emit_data(uint32_t generation)
{
    btstack_bip_event_t event;
    static const uint8_t data[] = { 1u, 2u };

    memset(&event, 0, sizeof(event));
    event.type = BTSTACK_BIP_EVENT_DATA;
    event.generation = generation;
    event.data.data.data = data;
    event.data.data.length = sizeof(data);
    bip_callback(&event);
}

static void emit_connection_ready(int status)
{
    btstack_bip_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = BTSTACK_BIP_EVENT_CONNECTION_ESTABLISHED;
    event.status = status;
    bip_callback(&event);
}

static void reset_test(void)
{
    set_object_calls = 0u;
    thumbnail_calls = 0u;
    properties_calls = 0u;
    image_calls = 0u;
    cancel_calls = 0u;
    peer_disconnect_calls = 0u;
    connect_calls = 0u;
    metadata_calls = 0u;
    forwarded_events = 0u;
    forwarded_data_events = 0u;
    forwarded_completed_events = 0u;
    replace_pending_in_callback = 0u;
    start_result = BTSTACK_STATUS_OK;
    CHECK(btstack_avrcp_cover_art_setup(on_event) == BTSTACK_STATUS_OK);
    btstack_avrcp_cover_art_set_callback_enabled(true);
}

static void test_obex_precedes_cover_handle_request(void)
{
    static const btstack_bd_addr_t peer = { 3u, 4u, 5u, 6u, 7u, 8u };

    reset_test();
    btstack_avrcp_cover_art_connected(peer);
    CHECK(connect_calls == 1u);
    CHECK(metadata_calls == 0u);
    CHECK(forwarded_events == 0u);
    emit_connection_ready(BTSTACK_STATUS_OK);
    CHECK(metadata_calls == 1u);
    CHECK(forwarded_events == 0u);
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"1234567", 7u);
    CHECK(set_object_calls == 1u && thumbnail_calls == 1u);

    emit_connection_ready(BTSTACK_STATUS_OK);
    CHECK(metadata_calls == 1u);
}

static void test_terminal_callback_replaces_pending_handle(void)
{
    static const btstack_bd_addr_t peer = { 1u, 1u, 2u, 2u, 3u, 3u };
    uint32_t replacement_generation;

    reset_test();
    btstack_avrcp_cover_art_connected(peer);
    emit_connection_ready(BTSTACK_STATUS_OK);
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"1111111", 7u);
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"2222222", 7u);
    replace_pending_in_callback = 1u;
    emit_terminal(BTSTACK_BIP_EVENT_ERROR, last_generation);
    CHECK(connect_calls == 2u);
    emit_connection_ready(BTSTACK_STATUS_OK);
    CHECK(set_object_calls == 2u);
    CHECK(strcmp(last_handle, "3333333") == 0);
    replacement_generation = last_generation;
    emit_terminal(BTSTACK_BIP_EVENT_COMPLETED, replacement_generation);
    CHECK(set_object_calls == 2u);
    CHECK(strcmp(last_handle, "3333333") == 0);
}

static void test_active_handle_change_keeps_bip_connected(void)
{
    static const btstack_bd_addr_t peer = { 9u, 8u, 7u, 6u, 5u, 4u };
    uint32_t old_generation;

    reset_test();
    btstack_avrcp_cover_art_connected(peer);
    emit_connection_ready(BTSTACK_STATUS_OK);
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"1111111", 7u);
    old_generation = last_generation;

    btstack_avrcp_cover_art_track_changed();
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"2222222", 7u);
    CHECK(cancel_calls == 0u);
    emit_data(old_generation);
    emit_terminal(BTSTACK_BIP_EVENT_COMPLETED, old_generation);
    CHECK(forwarded_data_events == 0u);
    CHECK(forwarded_completed_events == 0u);
    CHECK(connect_calls == 1u);
    CHECK(set_object_calls == 2u);
    CHECK(strcmp(last_handle, "2222222") == 0);
}

static void test_idle_track_change_reuses_bip(void)
{
    static const btstack_bd_addr_t peer = { 4u, 5u, 6u, 7u, 8u, 9u };

    reset_test();
    btstack_avrcp_cover_art_connected(peer);
    emit_connection_ready(BTSTACK_STATUS_OK);
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"3333333", 7u);
    emit_terminal(BTSTACK_BIP_EVENT_COMPLETED, last_generation);

    btstack_avrcp_cover_art_track_changed();
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"4444444", 7u);
    CHECK(connect_calls == 1u);
    CHECK(set_object_calls == 2u);
    CHECK(strcmp(last_handle, "4444444") == 0);
}

static void test_latest_handle_wins_without_cancel(void)
{
    static const btstack_bd_addr_t peer = { 7u, 7u, 6u, 6u, 5u, 5u };
    uint32_t old_generation;

    reset_test();
    btstack_avrcp_cover_art_connected(peer);
    emit_connection_ready(BTSTACK_STATUS_OK);
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"1111111", 7u);
    old_generation = last_generation;
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"2222222", 7u);
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"3333333", 7u);
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"4444444", 7u);
    CHECK(cancel_calls == 0u);
    CHECK(set_object_calls == 1u);

    emit_terminal(BTSTACK_BIP_EVENT_COMPLETED, old_generation);
    CHECK(connect_calls == 1u);
    CHECK(set_object_calls == 2u);
    CHECK(strcmp(last_handle, "4444444") == 0);
}

static void test_unchanged_handle_is_not_fetched_again(void)
{
    static const btstack_bd_addr_t peer = { 5u, 5u, 4u, 4u, 3u, 3u };

    reset_test();
    btstack_avrcp_cover_art_connected(peer);
    emit_connection_ready(BTSTACK_STATUS_OK);
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"5555555", 7u);
    emit_terminal(BTSTACK_BIP_EVENT_COMPLETED, last_generation);
    btstack_avrcp_cover_art_track_changed();
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"5555555", 7u);
    CHECK(connect_calls == 1u);
    CHECK(set_object_calls == 1u);
}

static void test_modes_and_pending(void)
{
    static const btstack_bd_addr_t peer = { 1u, 2u, 3u, 4u, 5u, 6u };
    bt_xstack_cover_art_image_descriptor_t descriptor = {
        BT_XSTACK_COVER_ART_ENCODING_JPEG, 200u, 200u,
        BT_XSTACK_COVER_ART_TRANSFORMATION_NONE
    };

    reset_test();
    btstack_avrcp_cover_art_connected(peer);
    emit_connection_ready(BTSTACK_STATUS_OK);
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"0123456", 7u);
    CHECK(set_object_calls == 1u && thumbnail_calls == 1u && last_generation == 1u);
    CHECK(memcmp(last_address, peer, sizeof(peer)) == 0 && strcmp(last_handle, "0123456") == 0);
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"0123456", 7u);
    CHECK(set_object_calls == 1u);
    CHECK(btstack_avrcp_cover_art_set_mode(BT_XSTACK_COVER_ART_MODE_IMAGE) == BTSTACK_STATUS_OK);
    CHECK(cancel_calls == 0u);
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"1111111", 7u);
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"2222222", 7u);
    emit_terminal(BTSTACK_BIP_EVENT_COMPLETED, 1u);
    CHECK(connect_calls == 1u);
    CHECK(set_object_calls == 2u && properties_calls == 1u && strcmp(last_handle, "2222222") == 0);
    CHECK(forwarded_events == 0u);
    emit_data(1u);
    emit_terminal(BTSTACK_BIP_EVENT_COMPLETED, 1u);
    CHECK(forwarded_events == 0u);
    emit_terminal(BTSTACK_BIP_EVENT_PROPERTIES_COMPLETED, last_generation);
    CHECK(btstack_avrcp_cover_art_get_image(&descriptor) == BTSTACK_STATUS_OK);
    CHECK(image_calls == 1u);
}

static void test_invalid_disabled_disconnect_and_wrap(void)
{
    static const btstack_bd_addr_t peer = { 6u, 5u, 4u, 3u, 2u, 1u };

    reset_test();
    btstack_avrcp_cover_art_connected(peer);
    emit_connection_ready(BTSTACK_STATUS_OK);
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"", 0u);
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"123456", 6u);
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"12345678", 8u);
    CHECK(set_object_calls == 0u);
    btstack_avrcp_cover_art_set_callback_enabled(false);
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"7654321", 7u);
    CHECK(set_object_calls == 0u);
    btstack_avrcp_cover_art_set_callback_enabled(true);
    emit_connection_ready(BTSTACK_STATUS_OK);
    CHECK(set_object_calls == 1u);
    btstack_avrcp_cover_art_test_set_generation(UINT32_MAX);
    emit_terminal(BTSTACK_BIP_EVENT_COMPLETED, last_generation);
    btstack_avrcp_cover_art_handle_received((const uint8_t *)"1111111", 7u);
    CHECK(last_generation == 1u);
    btstack_avrcp_cover_art_disconnected(peer);
    CHECK(peer_disconnect_calls == 1u);
}

int main(void)
{
    test_obex_precedes_cover_handle_request();
    test_active_handle_change_keeps_bip_connected();
    test_idle_track_change_reuses_bip();
    test_latest_handle_wins_without_cancel();
    test_unchanged_handle_is_not_fetched_again();
    test_modes_and_pending();
    test_terminal_callback_replaces_pending_handle();
    test_invalid_disabled_disconnect_and_wrap();
    puts("PASS: AVRCP Cover Art automatic policy");
    return 0;
}

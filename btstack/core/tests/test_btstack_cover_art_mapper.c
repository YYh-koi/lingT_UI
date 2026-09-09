#include <assert.h>
#include <string.h>

#include "btstack_core_internal.h"
#include "adapter/btstack_adapter.h"
#include "core/btstack_profile_manager.h"

static bt_xstack_cover_art_event_t delivered;
static unsigned delivery_count;

void btstack_platform_enter_critical(void) { }
void btstack_platform_exit_critical(void) { }
int btstack_adapter_get_state(btstack_adapter_state_t *state) { memset(state, 0, sizeof(*state)); return BTSTACK_STATUS_OK; }
int btstack_adapter_set_discoverable(bool enabled) { (void)enabled; return BTSTACK_STATUS_OK; }
int btstack_adapter_set_event_callback(btstack_adapter_event_callback_t callback) { (void)callback; return BTSTACK_STATUS_OK; }
void btstack_profile_manager_state_set_value(bt_xstack_request_id_t id, uint32_t value) { (void)id; (void)value; }
void btstack_profile_manager_state_write_pbap(uint8_t state) { (void)state; }
int btstack_profile_manager_get_classic_state(btstack_profile_manager_classic_state_t *state) { memset(state, 0, sizeof(*state)); return BTSTACK_STATUS_OK; }
int btstack_core_post_event(const btstack_core_event_envelope_t *event) { (void)event; return 0; }
int btstack_gatt_server_get_value(uint16_t handle, uint8_t *value, uint16_t *value_len) { (void)handle; (void)value; (void)value_len; return BTSTACK_STATUS_OK; }
void btstack_core_media_audio_deliver(const uint8_t *data, uint32_t length) { (void)data; (void)length; }
void btstack_core_call_audio_deliver(const uint8_t *data, uint32_t length) { (void)data; (void)length; }

void btstack_frontend_deliver_cover_art(const bt_xstack_cover_art_event_t *event)
{
    assert(event != NULL);
    delivered = *event;
    delivery_count++;
}

static void map(btstack_bip_event_t *event, bt_xstack_cover_art_event_type_t expected)
{
    unsigned before = delivery_count;

    btstack_core_event_mapper_cover_art(event);
    assert(delivery_count == before + 1u);
    assert(delivered.type == expected);
    assert(delivered.status == event->status);
    assert(delivered.generation == event->generation);
}

int main(void)
{
    static const uint8_t bytes[] = { 1u, 2u, 3u };
    btstack_bip_event_t event;

    memset(&event, 0, sizeof(event));
    event.status = -3;
    event.generation = 7u;
    event.operation = BTSTACK_BIP_OPERATION_IMAGE;
    event.type = BTSTACK_BIP_EVENT_STARTED;
    event.data.started.address[0] = 9u;
    memcpy(event.data.started.image_handle, "0123456", 8u);
    map(&event, BT_XSTACK_COVER_ART_STARTED);
    assert(delivered.data.started.address[0] == 9u && delivered.data.started.operation == BTSTACK_BIP_OPERATION_IMAGE);

    event.type = BTSTACK_BIP_EVENT_PROPERTY;
    event.data.property.encoding = BT_XSTACK_COVER_ART_ENCODING_PNG;
    event.data.property.max_width = 320u;
    map(&event, BT_XSTACK_COVER_ART_PROPERTY);
    assert(delivered.data.property.encoding == BT_XSTACK_COVER_ART_ENCODING_PNG && delivered.data.property.max_width == 320u);

    event.type = BTSTACK_BIP_EVENT_PROPERTIES_COMPLETED;
    event.data.properties_completed.property_count = 2u;
    map(&event, BT_XSTACK_COVER_ART_PROPERTIES_COMPLETED);
    assert(delivered.data.properties_completed.property_count == 2u);

    event.type = BTSTACK_BIP_EVENT_DATA;
    event.data.data.data = bytes;
    event.data.data.length = sizeof(bytes);
    event.data.data.offset = 11u;
    map(&event, BT_XSTACK_COVER_ART_DATA);
    assert(delivered.data.data.data == bytes && delivered.data.data.length == sizeof(bytes) && delivered.data.data.offset == 11u);

    event.type = BTSTACK_BIP_EVENT_COMPLETED;
    event.data.completed.total_length = 14u;
    map(&event, BT_XSTACK_COVER_ART_COMPLETED);
    assert(delivered.data.completed.total_length == 14u);

    event.type = BTSTACK_BIP_EVENT_ERROR;
    event.native_status = 5u;
    map(&event, BT_XSTACK_COVER_ART_ERROR);
    assert(delivered.data.error.operation == BTSTACK_BIP_OPERATION_IMAGE && delivered.data.error.native_status == 5u);
    return 0;
}

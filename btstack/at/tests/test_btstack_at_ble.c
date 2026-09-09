#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "btstack_at_internal.h"
#include "core/btstack_profile_manager.h"
#include "port/btstack_platform.h"

static btstack_at_backend_dispatch_t test_dispatch;
static btstack_core_request_t test_request;
static unsigned int test_dispatch_count;
static btstack_at_event_t test_posted_event;
static unsigned int test_posted_event_count;

static uint16_t text_length(const char *text)
{
    uint16_t length = 0u;
    while (text[length] != '\0') length++;
    return length;
}

int btstack_at_backend_register(btstack_at_backend_dispatch_t dispatch)
{
    test_dispatch = dispatch;
    return BTSTACK_AT_STATUS_OK;
}

int btstack_at_command_gate_register(btstack_at_command_gate_t gate, void *context)
{
    (void)gate;
    (void)context;
    return BTSTACK_AT_STATUS_OK;
}

int btstack_core_dispatch(const btstack_core_request_t *request)
{
    assert(request != NULL);
    test_request = *request;
    test_dispatch_count++;
    return 0;
}

int btstack_core_copy_state(bt_xstack_state_t *state)
{
    memset(state, 0, sizeof(*state));
    return 0;
}

int btstack_core_state_read_pbap(bt_xstack_phonebook_state_t *state)
{
    *state = BT_XSTACK_PHONEBOOK_STATE_IDLE;
    return 0;
}

int btstack_runtime_started(void) { return 1; }
int btstack_runtime_start(void) { return 0; }
int btstack_at_complete(const btstack_at_result_t *result)
{
    (void)result;
    return 0;
}
int btstack_at_post_event(const btstack_at_event_t *event)
{
    assert(event != NULL);
    test_posted_event = *event;
    test_posted_event_count++;
    return 0;
}
bool btstack_profile_manager_hfp_event_delivered(
    uint8_t event_type, const uint8_t address[6])
{
    (void)event_type;
    (void)address;
    return true;
}
void btstack_platform_enter_critical(void) {}
void btstack_platform_exit_critical(void) {}
int btstack_platform_notify(btstack_worker_id_t id, uint32_t signals)
{
    (void)id;
    (void)signals;
    return 0;
}

static void clear_command(btstack_at_command_t *command,
    btstack_at_command_id_t id)
{
    memset(command, 0, sizeof(*command));
    command->id = id;
    command->form = BTSTACK_AT_FORM_SET;
}

int main(void)
{
    btstack_at_command_t command;
    btstack_at_event_t event;
    btstack_core_event_envelope_t envelope;
    char output[BTSTACK_AT_OUTPUT_MAX];
    uint16_t output_length;

    assert(btstack_at_core_adapter_init() == BTSTACK_AT_STATUS_OK);
    assert(test_dispatch != NULL);

    clear_command(&command, BTSTACK_AT_COMMAND_LEADV);
    command.values[0] = 1u;
    command.le_adv.interval_min = 160u;
    command.le_adv.interval_max = 240u;
    command.le_adv.channel_map = 7u;
    command.le_adv.data_len = 2u;
    command.le_adv.data[0] = 1u;
    command.le_adv.data[1] = 2u;
    assert(test_dispatch(&command, 1u) == 0);
    assert(test_request.id == BT_XSTACK_REQUEST_LE_ADV_CONFIGURE_START);
    assert(test_request.le_adv.interval_min == 160u);
    assert(test_request.le_adv.data[1] == 2u);

    clear_command(&command, BTSTACK_AT_COMMAND_LESCAN);
    command.values[0] = 1u;
    command.le_scan.interval = 16u;
    command.le_scan.window = 8u;
    command.le_scan.active = true;
    assert(test_dispatch(&command, 2u) == 0);
    assert(test_request.id == BT_XSTACK_REQUEST_LE_SCAN_START);
    assert(test_request.le_scan.window == 8u);

    clear_command(&command, BTSTACK_AT_COMMAND_LECONN);
    command.values[0] = 1u;
    command.has_address = true;
    command.address[5] = 0xa5u;
    assert(test_dispatch(&command, 3u) == 0);
    assert(test_request.id == BT_XSTACK_REQUEST_LE_CONNECT);
    assert(test_request.le_connect.address_type == 1u);
    assert(test_request.le_connect.address.bytes[5] == 0xa5u);

    clear_command(&command, BTSTACK_AT_COMMAND_GATTSC);
    command.values[0] = 4u;
    command.values[1] = BT_XSTACK_GATT_PROPERTY_READ |
        BT_XSTACK_GATT_PROPERTY_NOTIFY;
    command.values[2] = 20u;
    command.security = BT_XSTACK_GATT_SECURITY_ENCRYPTED;
    command.uuid.type = BT_XSTACK_UUID_TYPE_16;
    command.uuid.value.uuid16 = 0x2a19u;
    assert(test_dispatch(&command, 4u) == 0);
    assert(test_request.id == BT_XSTACK_REQUEST_GATT_SERVER_ADD_CHARACTERISTIC);
    assert(test_request.gatt_registration.parent_handle == 4u);
    assert(test_request.gatt_registration.config.max_len == 20u);
    assert(test_request.gatt_registration.config.read_security ==
        BT_XSTACK_GATT_SECURITY_ENCRYPTED);

    clear_command(&command, BTSTACK_AT_COMMAND_GATTSVAL);
    command.values[0] = 7u;
    command.binary_length = 3u;
    command.binary[0] = 1u;
    command.binary[1] = 2u;
    command.binary[2] = 3u;
    assert(test_dispatch(&command, 5u) == 0);
    assert(test_request.id == BT_XSTACK_REQUEST_GATT_SERVER_SET_VALUE);
    assert(test_request.gatt.handle == 7u);
    assert(test_request.gatt.value_len == 3u);
    assert(test_request.gatt.value[2] == 3u);

    clear_command(&command, BTSTACK_AT_COMMAND_GATTCSUB);
    command.values[0] = 9u;
    command.values[1] = 10u;
    command.values[2] = 3u;
    assert(test_dispatch(&command, 6u) == 0);
    assert(test_request.id == BT_XSTACK_REQUEST_GATT_CLIENT_SUBSCRIBE);
    assert(test_request.gatt.characteristic.value_handle == 9u);
    assert(test_request.gatt.characteristic.end_handle == 10u);
    assert(test_request.gatt.flags == 3u);

    command.values[2] = 0u;
    assert(test_dispatch(&command, 7u) == 0);
    assert(test_request.id == BT_XSTACK_REQUEST_GATT_CLIENT_UNSUBSCRIBE);

    clear_command(&command, BTSTACK_AT_COMMAND_COVERMODE);
    command.values[0] = BT_XSTACK_COVER_ART_MODE_IMAGE;
    assert(test_dispatch(&command, 8u) == 0);
    assert(test_request.id == BT_XSTACK_REQUEST_COVER_ART_SET_MODE);
    assert(test_request.cover_art.mode == BT_XSTACK_COVER_ART_MODE_IMAGE);

    clear_command(&command, BTSTACK_AT_COMMAND_COVERIMAGE);
    command.cover_art_descriptor.encoding = BT_XSTACK_COVER_ART_ENCODING_PNG;
    command.cover_art_descriptor.width = 200u;
    command.cover_art_descriptor.height = 200u;
    command.cover_art_descriptor.transformation =
        BT_XSTACK_COVER_ART_TRANSFORMATION_CROP;
    assert(test_dispatch(&command, 9u) == 0);
    memset(&command.cover_art_descriptor, 0,
        sizeof(command.cover_art_descriptor));
    assert(test_request.id == BT_XSTACK_REQUEST_COVER_ART_GET_IMAGE);
    assert(test_request.cover_art.descriptor.encoding ==
        BT_XSTACK_COVER_ART_ENCODING_PNG);
    assert(test_request.cover_art.descriptor.width == 200u);
    assert(test_request.cover_art.descriptor.height == 200u);
    assert(test_request.cover_art.descriptor.transformation ==
        BT_XSTACK_COVER_ART_TRANSFORMATION_CROP);

    assert(test_dispatch_count == 9u);

    memset(&event, 0, sizeof(event));
    event.id = BTSTACK_AT_EVENT_LE;
    event.data.le.type = BT_XSTACK_LE_EVENT_SCAN_RESULT;
    event.data.le.status = 0;
    event.data.le.data.scan.address_type = 1u;
    event.data.le.data.scan.address.bytes[0] = 1u;
    event.data.le.data.scan.address.bytes[1] = 2u;
    event.data.le.data.scan.address.bytes[2] = 3u;
    event.data.le.data.scan.address.bytes[3] = 4u;
    event.data.le.data.scan.address.bytes[4] = 5u;
    event.data.le.data.scan.address.bytes[5] = 6u;
    event.data.le.data.scan.rssi = -42;
    event.data.le.data.scan.data_len = 3u;
    event.data.le.data.scan.data[0] = 2u;
    event.data.le.data.scan.data[1] = 1u;
    event.data.le.data.scan.data[2] = 6u;
    assert(btstack_at_format_event(&event, output, sizeof(output),
        &output_length) == 0);
    assert(output_length == text_length(
        "+LESCAN=2,0,1,010203040506,-42,0,020106\r\n"));
    assert(memcmp(output,
        "+LESCAN=2,0,1,010203040506,-42,0,020106\r\n",
        output_length) == 0);

    memset(&event, 0, sizeof(event));
    event.id = BTSTACK_AT_EVENT_GATT;
    event.data.gatt.type = BT_XSTACK_GATT_EVENT_VALUE;
    event.data.gatt.data.value.value_handle = 7u;
    event.data.gatt.data.value.value_len = 2u;
    event.data.gatt.data.value.value[0] = 1u;
    event.data.gatt.data.value.value[1] = 2u;
    assert(btstack_at_format_event(&event, output, sizeof(output),
        &output_length) == 0);
    assert(output_length == text_length("+GATTC=3,0,7,2,0102,0\r\n"));
    assert(memcmp(output, "+GATTC=3,0,7,2,0102,0\r\n",
        output_length) == 0);

    memset(&envelope, 0, sizeof(envelope));
    envelope.domain = BT_XSTACK_EVENT_DOMAIN_LE;
    envelope.data.le.type = BT_XSTACK_LE_EVENT_CONNECTED;
    envelope.data.le.data.link.connection_handle = 12u;
    assert(btstack_core_post_event(&envelope) == 0);
    assert(test_posted_event_count == 1u);
    assert(test_posted_event.id == BTSTACK_AT_EVENT_LE);
    assert(test_posted_event.data.le.data.link.connection_handle == 12u);

    memset(&envelope, 0, sizeof(envelope));
    envelope.domain = BT_XSTACK_EVENT_DOMAIN_GATT;
    envelope.data.gatt.type = BT_XSTACK_GATT_EVENT_NOTIFICATION;
    envelope.data.gatt.data.value.value_handle = 17u;
    assert(btstack_core_post_event(&envelope) == 0);
    assert(test_posted_event_count == 2u);
    assert(test_posted_event.id == BTSTACK_AT_EVENT_GATT);
    assert(test_posted_event.data.gatt.data.value.value_handle == 17u);
    return 0;
}

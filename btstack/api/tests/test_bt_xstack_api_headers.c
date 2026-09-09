#include <stddef.h>

#include "bt_xstack.h"

static void service_event(
    const bt_xstack_service_event_t *event,
    void *context)
{
    (void)event;
    (void)context;
}

static void connection_event(
    const bt_xstack_connection_event_t *event,
    void *context)
{
    (void)event;
    (void)context;
}

static void phone_event(
    const bt_xstack_phone_event_t *event,
    void *context)
{
    (void)event;
    (void)context;
}

static void audio_event(
    const bt_xstack_audio_event_t *event,
    void *context)
{
    (void)event;
    (void)context;
}

static void phonebook_event(
    const bt_xstack_phonebook_event_t *event,
    void *context)
{
    (void)event;
    (void)context;
}

static void le_event(const bt_xstack_le_event_t *event, void *context)
{
    (void)event;
    (void)context;
}

static void gatt_event(const bt_xstack_gatt_event_t *event, void *context)
{
    (void)event;
    (void)context;
}

static void pcm_event(
    const uint8_t *data,
    uint32_t length,
    void *context)
{
    (void)data;
    (void)length;
    (void)context;
}

int main(void)
{
    bt_xstack_config_t config = {0};
    bt_xstack_state_t state;
    bt_xstack_phonebook_state_t phonebook_state;
    bt_xstack_phonebook_category_mask_t contacts = BT_XSTACK_PHONEBOOK_CONTACTS;
    bt_xstack_phonebook_category_mask_t incoming = BT_XSTACK_PHONEBOOK_INCOMING;
    bt_xstack_phonebook_category_mask_t outgoing = BT_XSTACK_PHONEBOOK_OUTGOING;
    bt_xstack_phonebook_category_mask_t missed = BT_XSTACK_PHONEBOOK_MISSED;
    bt_xstack_phonebook_category_mask_t combined =
        BT_XSTACK_PHONEBOOK_CONTACTS | BT_XSTACK_PHONEBOOK_MISSED;
    bt_xstack_phonebook_category_mask_t all = BT_XSTACK_PHONEBOOK_ALL;
    bt_xstack_phonebook_event_t pbap_event = {0};
    bt_xstack_le_adv_config_t adv = {0};
    bt_xstack_le_scan_config_t scan = {0};
    bt_xstack_gatt_attribute_config_t attribute;
    bt_xstack_gatt_characteristic_t characteristic;
    bt_xstack_uuid_t uuid = { BT_XSTACK_UUID_TYPE_16, { 0x180fu } };

    attribute.uuid = uuid;
    attribute.properties = BT_XSTACK_GATT_PROPERTY_READ;
    attribute.max_len = 1u;
    attribute.read_security = BT_XSTACK_GATT_SECURITY_NONE;
    attribute.write_security = BT_XSTACK_GATT_SECURITY_NONE;
    characteristic.start_handle = 1u;
    characteristic.value_handle = 2u;
    characteristic.end_handle = 3u;
    characteristic.properties = BT_XSTACK_GATT_PROPERTY_NOTIFY;
    characteristic.uuid = uuid;

    config.service_callback = service_event;
    config.connection_callback = connection_event;
    config.phone_callback = phone_event;
    config.audio_event_callback = audio_event;
    config.phonebook_callback = phonebook_event;
    config.le_callback = le_event;
    config.gatt_callback = gatt_event;
    config.media_audio_callback = pcm_event;
    config.call_audio_callback = NULL;
    config.context = 0;

    (void)bt_xstack_init(&config);
    (void)bt_xstack_get_state(&state);
    (void)contacts;
    (void)incoming;
    (void)outgoing;
    (void)missed;
    (void)combined;
    (void)bt_xstack_phonebook_sync(&pbap_event.address, all);
    (void)bt_xstack_phonebook_cancel();
    (void)bt_xstack_phonebook_authenticate("1234");
    (void)bt_xstack_phonebook_get_state(&phonebook_state);
    (void)bt_xstack_le_adv_configure(&adv);
    (void)bt_xstack_le_adv_start();
    (void)bt_xstack_le_scan_start(&scan);
    (void)bt_xstack_le_connect(0u, &pbap_event.address);
    (void)bt_xstack_le_pair();
    (void)bt_xstack_gatt_server_add_service(&uuid);
    (void)bt_xstack_gatt_server_add_characteristic(1u, &attribute);
    (void)bt_xstack_gatt_server_get_value(1u);
    (void)bt_xstack_gatt_client_discover_services(&uuid);
    (void)bt_xstack_gatt_client_subscribe(&characteristic, true, false);
    return 0;
}

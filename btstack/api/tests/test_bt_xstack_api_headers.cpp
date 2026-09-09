#include "bt_xstack.h"

static void on_service(
    const bt_xstack_service_event_t *event,
    void *context)
{
    (void)event;
    (void)context;
}

static void on_pcm(
    const uint8_t *data,
    uint32_t length,
    void *context)
{
    (void)data;
    (void)length;
    (void)context;
}

static void on_phonebook(
    const bt_xstack_phonebook_event_t *event,
    void *context)
{
    (void)event;
    (void)context;
}

static void on_le(const bt_xstack_le_event_t *event, void *context)
{
    (void)event;
    (void)context;
}

static void on_gatt(const bt_xstack_gatt_event_t *event, void *context)
{
    (void)event;
    (void)context;
}

int main()
{
    bt_xstack_config_t config;
    bt_xstack_phonebook_state_t phonebook_state =
        BT_XSTACK_PHONEBOOK_STATE_IDLE;
    bt_xstack_phonebook_category_mask_t contacts = BT_XSTACK_PHONEBOOK_CONTACTS;
    bt_xstack_phonebook_category_mask_t incoming = BT_XSTACK_PHONEBOOK_INCOMING;
    bt_xstack_phonebook_category_mask_t outgoing = BT_XSTACK_PHONEBOOK_OUTGOING;
    bt_xstack_phonebook_category_mask_t missed = BT_XSTACK_PHONEBOOK_MISSED;
    bt_xstack_phonebook_category_mask_t combined =
        BT_XSTACK_PHONEBOOK_INCOMING | BT_XSTACK_PHONEBOOK_OUTGOING;
    bt_xstack_phonebook_category_mask_t all = BT_XSTACK_PHONEBOOK_ALL;

    config.service_callback = nullptr;
    config.connection_callback = nullptr;
    config.phone_callback = nullptr;
    config.audio_event_callback = nullptr;
    config.phonebook_callback = nullptr;
    config.le_callback = nullptr;
    config.gatt_callback = nullptr;
    config.media_audio_callback = nullptr;
    config.call_audio_callback = nullptr;
    config.context = nullptr;
    config.task_priority = 0u;
    config.task_stack_words = 0u;

    config.service_callback = on_service;
    config.phonebook_callback = on_phonebook;
    config.le_callback = on_le;
    config.gatt_callback = on_gatt;
    config.media_audio_callback = on_pcm;
    (void)contacts;
    (void)incoming;
    (void)outgoing;
    (void)missed;
    (void)combined;
    (void)bt_xstack_phonebook_sync(nullptr, all);
    (void)bt_xstack_phonebook_get_state(&phonebook_state);
    return (config.service_callback != nullptr) ? 0 : 1;
}

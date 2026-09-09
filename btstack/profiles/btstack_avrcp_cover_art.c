#include "profiles/btstack_avrcp_cover_art.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "btstack_debug.h"
#include "btstack_avrcp_internal.h"

#if BTSTACK_CLASSIC && BTSTACK_AVRCP_CONTROLLER && BTSTACK_AVRCP_TARGET && BTSTACK_AVRCP_COVER_ART
typedef struct {
    btstack_bd_addr_t peer;
    char handle[8];
    char pending_handle[8];
    uint32_t generation;
    uint32_t active_generation;
    uint32_t pending_generation;
    bt_xstack_cover_art_mode_t mode;
    btstack_bip_event_callback_t callback;
    uint8_t connected;
    uint8_t callback_enabled;
    uint8_t handle_valid;
    uint8_t operation_active;
    uint8_t properties_ready;
    uint8_t pending_valid;
    uint8_t cancel_requested;
    uint8_t retry_used;
    uint8_t obex_ready;
    uint8_t active_stale;
} btstack_avrcp_cover_art_context_t;

static btstack_avrcp_cover_art_context_t btstack_avrcp_cover_art;

static uint32_t btstack_avrcp_cover_art_next_generation(void)
{
    btstack_avrcp_cover_art.generation++;
    if (btstack_avrcp_cover_art.generation == 0u) btstack_avrcp_cover_art.generation = 1u;
    return btstack_avrcp_cover_art.generation;
}

static int btstack_avrcp_cover_art_start(void)
{
    int status;
    uint32_t generation;

    if (!btstack_avrcp_cover_art.connected || !btstack_avrcp_cover_art.obex_ready ||
        !btstack_avrcp_cover_art.callback_enabled || !btstack_avrcp_cover_art.handle_valid ||
        btstack_avrcp_cover_art.operation_active) {
        return BTSTACK_STATUS_NOT_READY;
    }
    generation = btstack_avrcp_cover_art.generation;
    status = btstack_bip_client_set_object(btstack_avrcp_cover_art.peer, btstack_avrcp_cover_art.handle, generation);
    if (status != BTSTACK_STATUS_OK) {
        return status;
    }
    btstack_avrcp_cover_art.operation_active = 1u;
    btstack_avrcp_cover_art.active_generation = generation;
    btstack_avrcp_cover_art.active_stale = 0u;
    btstack_avrcp_cover_art.cancel_requested = 0u;
    btstack_avrcp_cover_art.properties_ready = 0u;
    if (btstack_avrcp_cover_art.mode == BT_XSTACK_COVER_ART_MODE_LINKED_THUMBNAIL) {
        status = btstack_bip_client_get_linked_thumbnail();
    } else {
        status = btstack_bip_client_get_image_properties();
    }
    if (status != BTSTACK_STATUS_OK && btstack_avrcp_cover_art.operation_active &&
        btstack_avrcp_cover_art.active_generation == generation) {
        btstack_avrcp_cover_art.operation_active = 0u;
    }
    return status;
}

static void btstack_avrcp_cover_art_queue_current(void)
{
    memcpy(btstack_avrcp_cover_art.pending_handle, btstack_avrcp_cover_art.handle, sizeof(btstack_avrcp_cover_art.pending_handle));
    btstack_avrcp_cover_art.pending_generation = btstack_avrcp_cover_art_next_generation();
    btstack_avrcp_cover_art.pending_valid = 1u;
}

static void btstack_avrcp_cover_art_start_pending(void)
{
    if (!btstack_avrcp_cover_art.pending_valid || btstack_avrcp_cover_art.operation_active ||
        !btstack_avrcp_cover_art.connected || !btstack_avrcp_cover_art.obex_ready ||
        !btstack_avrcp_cover_art.callback_enabled) return;
    memcpy(btstack_avrcp_cover_art.handle, btstack_avrcp_cover_art.pending_handle, sizeof(btstack_avrcp_cover_art.handle));
    btstack_avrcp_cover_art.generation = btstack_avrcp_cover_art.pending_generation;
    btstack_avrcp_cover_art.pending_valid = 0u;
    btstack_avrcp_cover_art.handle_valid = 1u;
    (void)btstack_avrcp_cover_art_start();
}

static void btstack_avrcp_cover_art_bip_event(const btstack_bip_event_t *event)
{
    uint8_t terminal = 0u;
    uint8_t start_pending = 0u;
    uint8_t reconnect = 0u;
    uint8_t active_was_stale;

    if (event == NULL) return;
    if (event->type == BTSTACK_BIP_EVENT_CONNECTION_ESTABLISHED) {
        if (event->status == BTSTACK_STATUS_OK && btstack_avrcp_cover_art.connected &&
            btstack_avrcp_cover_art.callback_enabled && !btstack_avrcp_cover_art.obex_ready) {
            btstack_avrcp_cover_art.obex_ready = 1u;
            if (btstack_avrcp_cover_art.pending_valid) {
                btstack_avrcp_cover_art_start_pending();
            } else {
                (void)btstack_avrcp_controller_request_metadata();
            }
        }
        return;
    }
    if (event->generation != btstack_avrcp_cover_art.active_generation) return;
    active_was_stale = btstack_avrcp_cover_art.active_stale;
    {
        switch (event->type) {
        case BTSTACK_BIP_EVENT_PROPERTIES_COMPLETED:
            btstack_avrcp_cover_art.operation_active = 0u;
            if (!active_was_stale) btstack_avrcp_cover_art.properties_ready = 1u;
            btstack_avrcp_cover_art.cancel_requested = 0u;
            terminal = 1u;
            break;
        case BTSTACK_BIP_EVENT_COMPLETED:
            btstack_avrcp_cover_art.operation_active = 0u;
            btstack_avrcp_cover_art.cancel_requested = 0u;
            terminal = 1u;
            break;
        case BTSTACK_BIP_EVENT_ERROR:
            btstack_avrcp_cover_art.operation_active = 0u;
            btstack_avrcp_cover_art.cancel_requested = 0u;
            btstack_avrcp_cover_art.obex_ready = 0u;
            terminal = 1u;
            if (!btstack_avrcp_cover_art.pending_valid && btstack_avrcp_cover_art.connected &&
                btstack_avrcp_cover_art.handle_valid && !active_was_stale &&
                event->status == BTSTACK_STATUS_ERROR && !btstack_avrcp_cover_art.retry_used) {
                btstack_avrcp_cover_art.retry_used = 1u;
                btstack_avrcp_cover_art_queue_current();
            }
            reconnect = btstack_avrcp_cover_art.connected && btstack_avrcp_cover_art.callback_enabled;
            break;
        default:
            break;
        }
        start_pending = terminal && btstack_avrcp_cover_art.pending_valid;
        if (terminal) btstack_avrcp_cover_art.active_stale = 0u;
    }
    if ((!active_was_stale || event->type == BTSTACK_BIP_EVENT_ERROR) && btstack_avrcp_cover_art.callback != NULL) {
        btstack_avrcp_cover_art.callback(event);
    }
    if (reconnect && btstack_avrcp_cover_art.connected && btstack_avrcp_cover_art.callback_enabled) {
        (void)btstack_bip_client_connect(btstack_avrcp_cover_art.peer);
    } else if (start_pending && !btstack_avrcp_cover_art.operation_active) {
        btstack_avrcp_cover_art_start_pending();
    }
}

int btstack_avrcp_cover_art_setup(btstack_bip_event_callback_t callback)
{
    memset(&btstack_avrcp_cover_art, 0, sizeof(btstack_avrcp_cover_art));
    btstack_avrcp_cover_art.mode = BT_XSTACK_COVER_ART_MODE_LINKED_THUMBNAIL;
    btstack_avrcp_cover_art.callback = callback;
    return btstack_bip_client_init(btstack_avrcp_cover_art_bip_event);
}

void btstack_avrcp_cover_art_set_callback_enabled(bool enabled)
{
    btstack_avrcp_cover_art.callback_enabled = enabled ? 1u : 0u;
    if (!enabled) {
        if (btstack_avrcp_cover_art.operation_active && !btstack_avrcp_cover_art.cancel_requested) {
            btstack_avrcp_cover_art.cancel_requested = 1u;
            (void)btstack_bip_client_cancel();
        }
        return;
    }
    if (btstack_avrcp_cover_art.connected && !btstack_avrcp_cover_art.obex_ready) {
        (void)btstack_bip_client_connect(btstack_avrcp_cover_art.peer);
    } else if (!btstack_avrcp_cover_art.operation_active) {
        (void)btstack_avrcp_cover_art_start();
    }
}

int btstack_avrcp_cover_art_set_mode(bt_xstack_cover_art_mode_t mode)
{
    if (mode != BT_XSTACK_COVER_ART_MODE_LINKED_THUMBNAIL && mode != BT_XSTACK_COVER_ART_MODE_IMAGE) return BTSTACK_STATUS_INVALID_ARGUMENT;
    if (btstack_avrcp_cover_art.mode == mode) return BTSTACK_STATUS_OK;
    btstack_avrcp_cover_art.mode = mode;
    btstack_avrcp_cover_art.properties_ready = 0u;
    btstack_avrcp_cover_art.retry_used = 0u;
    if (!btstack_avrcp_cover_art.handle_valid) return BTSTACK_STATUS_OK;
    if (btstack_avrcp_cover_art.operation_active) {
        btstack_avrcp_cover_art_queue_current();
        btstack_avrcp_cover_art.active_stale = 1u;
    } else {
        btstack_avrcp_cover_art_next_generation();
        (void)btstack_avrcp_cover_art_start();
    }
    return BTSTACK_STATUS_OK;
}

int btstack_avrcp_cover_art_get_image(const bt_xstack_cover_art_image_descriptor_t *descriptor)
{
    int status;
    uint32_t generation;

    if (descriptor == NULL) return BTSTACK_STATUS_INVALID_ARGUMENT;
    if (btstack_avrcp_cover_art.mode != BT_XSTACK_COVER_ART_MODE_IMAGE || !btstack_avrcp_cover_art.properties_ready ||
        btstack_avrcp_cover_art.operation_active) return BTSTACK_STATUS_NOT_READY;
    generation = btstack_avrcp_cover_art.generation;
    btstack_avrcp_cover_art.operation_active = 1u;
    btstack_avrcp_cover_art.active_generation = generation;
    btstack_avrcp_cover_art.properties_ready = 0u;
    status = btstack_bip_client_get_image(descriptor);
    if (status != BTSTACK_STATUS_OK && btstack_avrcp_cover_art.operation_active &&
        btstack_avrcp_cover_art.active_generation == generation) btstack_avrcp_cover_art.operation_active = 0u;
    return status;
}

void btstack_avrcp_cover_art_connected(const btstack_bd_addr_t address)
{
    if (address == NULL) return;
    memcpy(btstack_avrcp_cover_art.peer, address, sizeof(btstack_avrcp_cover_art.peer));
    btstack_avrcp_cover_art.connected = 1u;
    btstack_avrcp_cover_art.obex_ready = 0u;
    if (btstack_avrcp_cover_art.callback_enabled) (void)btstack_bip_client_connect(address);
}

void btstack_avrcp_cover_art_track_changed(void)
{
    btstack_avrcp_cover_art.pending_valid = 0u;
    btstack_avrcp_cover_art.properties_ready = 0u;
    btstack_avrcp_cover_art.retry_used = 0u;
}

void btstack_avrcp_cover_art_handle_received(const uint8_t *value, uint16_t length)
{
    char handle[8];
    uint16_t index;
    uint32_t generation;

    if (value == NULL || length != 7u) return;
    for (index = 0u; index < 7u; index++) if (value[index] == 0u) return;
    memcpy(handle, value, 7u);
    handle[7] = '\0';
    if (btstack_avrcp_cover_art.pending_valid &&
        memcmp(handle, btstack_avrcp_cover_art.pending_handle, sizeof(handle)) == 0) return;
    if (btstack_avrcp_cover_art.handle_valid && !btstack_avrcp_cover_art.pending_valid &&
        !btstack_avrcp_cover_art.active_stale &&
        memcmp(handle, btstack_avrcp_cover_art.handle, sizeof(handle)) == 0) return;
    generation = btstack_avrcp_cover_art_next_generation();
    btstack_avrcp_cover_art.retry_used = 0u;
    if (btstack_avrcp_cover_art.operation_active || btstack_avrcp_cover_art.pending_valid) {
        memcpy(btstack_avrcp_cover_art.pending_handle, handle, sizeof(handle));
        btstack_avrcp_cover_art.pending_generation = generation;
        btstack_avrcp_cover_art.pending_valid = 1u;
        if (btstack_avrcp_cover_art.operation_active) btstack_avrcp_cover_art.active_stale = 1u;
        return;
    }
    memcpy(btstack_avrcp_cover_art.handle, handle, sizeof(handle));
    btstack_avrcp_cover_art.handle_valid = 1u;
    btstack_avrcp_cover_art.properties_ready = 0u;
    (void)btstack_avrcp_cover_art_start();
}

void btstack_avrcp_cover_art_disconnected(const btstack_bd_addr_t address)
{
    if (address == NULL || !btstack_avrcp_cover_art.connected || memcmp(address, btstack_avrcp_cover_art.peer, sizeof(btstack_bd_addr_t)) != 0) return;
    btstack_avrcp_cover_art.connected = 0u;
    btstack_avrcp_cover_art.handle_valid = 0u;
    btstack_avrcp_cover_art.pending_valid = 0u;
    btstack_avrcp_cover_art.properties_ready = 0u;
    btstack_avrcp_cover_art.obex_ready = 0u;
    btstack_bip_client_peer_disconnected(address);
}

#ifdef BTSTACK_AVRCP_COVER_ART_TESTING
void btstack_avrcp_cover_art_test_set_generation(uint32_t generation)
{
    btstack_avrcp_cover_art.generation = generation;
}
#endif
#else
int btstack_avrcp_cover_art_setup(btstack_bip_event_callback_t callback) { (void)callback; return BTSTACK_STATUS_NOT_SUPPORTED; }
void btstack_avrcp_cover_art_set_callback_enabled(bool enabled) { (void)enabled; }
int btstack_avrcp_cover_art_set_mode(bt_xstack_cover_art_mode_t mode) { (void)mode; return BTSTACK_STATUS_NOT_SUPPORTED; }
int btstack_avrcp_cover_art_get_image(const bt_xstack_cover_art_image_descriptor_t *descriptor) { (void)descriptor; return BTSTACK_STATUS_NOT_SUPPORTED; }
void btstack_avrcp_cover_art_connected(const btstack_bd_addr_t address) { (void)address; }
void btstack_avrcp_cover_art_track_changed(void) { }
void btstack_avrcp_cover_art_handle_received(const uint8_t *value, uint16_t length) { (void)value; (void)length; }
void btstack_avrcp_cover_art_disconnected(const btstack_bd_addr_t address) { (void)address; }
#ifdef BTSTACK_AVRCP_COVER_ART_TESTING
void btstack_avrcp_cover_art_test_set_generation(uint32_t generation) { (void)generation; }
#endif
#endif

#include "profiles/btstack_hid_device.h"

#include <string.h>

#if BTSTACK_CLASSIC && BTSTACK_HID_DEVICE
static uint8_t btstack_hid_descriptor[BTSTACK_HID_DESCRIPTOR_MAX];
static uint16_t btstack_hid_descriptor_len;
static uint8_t btstack_hid_pnp_id[BTSTACK_HID_PNP_ID_LEN];
static uint16_t btstack_hid_pnp_id_len;
static btstack_hid_device_sdp_config_t btstack_hid_sdp_config;
static char btstack_hid_service_name[BTSTACK_HID_SDP_STRING_MAX];
static char
    btstack_hid_service_description[BTSTACK_HID_SDP_STRING_MAX];
static char btstack_hid_provider_name[BTSTACK_HID_SDP_STRING_MAX];
static bool btstack_hid_setup_done;
static bool btstack_hid_connected;

static void btstack_hid_copy_string(char *dst, const char *src)
{
    uint16_t i;

    memset(dst, 0, BTSTACK_HID_SDP_STRING_MAX);
    if (src == 0) {
        return;
    }
    for (i = 0u; i < (BTSTACK_HID_SDP_STRING_MAX - 1u); i++) {
        if (src[i] == '\0') {
            break;
        }
        dst[i] = src[i];
    }
}

static void btstack_hid_bind_sdp_strings(void)
{
    btstack_hid_sdp_config.service_name =
        btstack_hid_service_name;
    btstack_hid_sdp_config.service_description =
        btstack_hid_service_description;
    btstack_hid_sdp_config.provider_name =
        btstack_hid_provider_name;
}
#endif

int btstack_hid_device_set_descriptor(
    const uint8_t *descriptor,
    uint16_t descriptor_len)
{
#if BTSTACK_CLASSIC && BTSTACK_HID_DEVICE
    if ((descriptor == 0) || (descriptor_len == 0u) ||
        (descriptor_len > BTSTACK_HID_DESCRIPTOR_MAX)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    memcpy(btstack_hid_descriptor, descriptor, descriptor_len);
    btstack_hid_descriptor_len = descriptor_len;
    return BTSTACK_STATUS_OK;
#else
    (void)descriptor;
    (void)descriptor_len;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hid_device_set_pnp_id(
    const uint8_t *pnp_id,
    uint16_t pnp_id_len)
{
#if BTSTACK_CLASSIC && BTSTACK_HID_DEVICE
    if ((pnp_id == 0) || (pnp_id_len == 0u) ||
        (pnp_id_len > BTSTACK_HID_PNP_ID_LEN)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    memset(btstack_hid_pnp_id, 0, sizeof(btstack_hid_pnp_id));
    memcpy(btstack_hid_pnp_id, pnp_id, pnp_id_len);
    btstack_hid_pnp_id_len = pnp_id_len;
    return BTSTACK_STATUS_OK;
#else
    (void)pnp_id;
    (void)pnp_id_len;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hid_device_set_sdp_config(
    const btstack_hid_device_sdp_config_t *config)
{
#if BTSTACK_CLASSIC && BTSTACK_HID_DEVICE
    if (config == 0) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    btstack_hid_sdp_config = *config;
    btstack_hid_copy_string(
        btstack_hid_service_name,
        config->service_name);
    btstack_hid_copy_string(
        btstack_hid_service_description,
        config->service_description);
    btstack_hid_copy_string(
        btstack_hid_provider_name,
        config->provider_name);
    btstack_hid_bind_sdp_strings();
    return BTSTACK_STATUS_OK;
#else
    (void)config;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hid_device_setup(
    const btstack_hid_device_config_t *config)
{
#if BTSTACK_CLASSIC && BTSTACK_HID_DEVICE
    int status;

    if ((config == 0) || (config->descriptor == 0) ||
        (config->descriptor_len == 0u)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (config->descriptor_len > BTSTACK_HID_DESCRIPTOR_MAX) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if ((config->pnp_id == 0) != (config->pnp_id_len == 0u)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (config->pnp_id_len > BTSTACK_HID_PNP_ID_LEN) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    memset(btstack_hid_pnp_id, 0, sizeof(btstack_hid_pnp_id));
    btstack_hid_pnp_id_len = 0u;
    memset(&btstack_hid_sdp_config, 0,
           sizeof(btstack_hid_sdp_config));
    memset(btstack_hid_service_name, 0,
           sizeof(btstack_hid_service_name));
    memset(btstack_hid_service_description, 0,
           sizeof(btstack_hid_service_description));
    memset(btstack_hid_provider_name, 0,
           sizeof(btstack_hid_provider_name));

    status = btstack_hid_device_set_descriptor(
        config->descriptor,
        config->descriptor_len);
    if (status != BTSTACK_STATUS_OK) {
        return status;
    }
    if (config->pnp_id != 0) {
        status = btstack_hid_device_set_pnp_id(
            config->pnp_id,
            config->pnp_id_len);
        if (status != BTSTACK_STATUS_OK) {
            return status;
        }
    }
    if (config->sdp != 0) {
        status = btstack_hid_device_set_sdp_config(config->sdp);
        if (status != BTSTACK_STATUS_OK) {
            return status;
        }
    }

    btstack_hid_setup_done = true;
    btstack_hid_connected = false;
    return BTSTACK_STATUS_OK;
#else
    (void)config;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

bool btstack_hid_device_is_connected(void)
{
#if BTSTACK_CLASSIC && BTSTACK_HID_DEVICE
    return btstack_hid_connected;
#else
    return false;
#endif
}

int btstack_hid_device_disconnect(void)
{
#if BTSTACK_CLASSIC && BTSTACK_HID_DEVICE
    if (!btstack_hid_connected) {
        return BTSTACK_STATUS_NOT_READY;
    }
    return BTSTACK_STATUS_NOT_SUPPORTED;
#else
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hid_device_reconnect(
    const btstack_property_bond_peer_t *peer)
{
#if BTSTACK_CLASSIC && BTSTACK_HID_DEVICE
    if ((peer == 0) ||
        (peer->transport != BTSTACK_TRANSPORT_CLASSIC)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (!btstack_hid_setup_done) {
        return BTSTACK_STATUS_NOT_READY;
    }
    return BTSTACK_STATUS_NOT_SUPPORTED;
#else
    (void)peer;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

int btstack_hid_device_send_report(
    uint8_t report_id,
    const uint8_t *report,
    uint16_t report_len)
{
    return btstack_hid_device_send_raw_report(
        1u,
        report_id,
        report,
        report_len);
}

int btstack_hid_device_send_raw_report(
    uint8_t report_type,
    uint8_t report_id,
    const uint8_t *report,
    uint16_t report_len)
{
#if BTSTACK_CLASSIC && BTSTACK_HID_DEVICE
    (void)report_type;
    (void)report_id;

    if ((report == 0) || (report_len == 0u)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (!btstack_hid_setup_done || !btstack_hid_connected) {
        return BTSTACK_STATUS_NOT_READY;
    }
    return BTSTACK_STATUS_NOT_SUPPORTED;
#else
    (void)report_type;
    (void)report_id;
    (void)report;
    (void)report_len;
    return BTSTACK_STATUS_NOT_SUPPORTED;
#endif
}

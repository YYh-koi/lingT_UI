#include "btstack_hci_dump_gpcv2178.h"

#include "bluetooth.h"
#include "btstack_defines.h"
#include "gplib_print_string.h"
#include "project.h"

#define BTSTACK_HCI_DUMP_FULL_PACKET_MAX_LEN 258u

static char btstack_hci_dump_hex_text[
    (BTSTACK_HCI_DUMP_FULL_PACKET_MAX_LEN * 3u) + 1u];

static uint16_t btstack_hci_dump_read_le16(const uint8_t *packet)
{
    return (uint16_t)packet[0] | ((uint16_t)packet[1] << 8);
}

static const char *btstack_hci_dump_direction(uint8_t in)
{
    return (in != 0u) ? "RX" : "TX";
}

static void btstack_hci_dump_gpcv2178_log_full(const char *label,
                                                const uint8_t *packet,
                                                uint16_t len)
{
    static const char hex_digits[] = "0123456789ABCDEF";
    uint16_t i;

    if ((packet == NULL) || (len > BTSTACK_HCI_DUMP_FULL_PACKET_MAX_LEN)) {
        DBG_PRINT("%s%u: invalid\r\n", label, (unsigned int)len);
        return;
    }

    for (i = 0u; i < len; ++i) {
        uint16_t offset = (uint16_t)(i * 3u);
        btstack_hci_dump_hex_text[offset] = ' ';
        btstack_hci_dump_hex_text[offset + 1u] =
            hex_digits[(packet[i] >> 4) & 0x0fu];
        btstack_hci_dump_hex_text[offset + 2u] =
            hex_digits[packet[i] & 0x0fu];
    }
    btstack_hci_dump_hex_text[len * 3u] = '\0';

    DBG_PRINT("%s%u:%s\r\n", label, (unsigned int)len,
              btstack_hci_dump_hex_text);
}

static void btstack_hci_dump_gpcv2178_log_acl(uint8_t in,
                                               const uint8_t *packet,
                                               uint16_t len)
{
    const char *direction = btstack_hci_dump_direction(in);
    uint16_t header;
    uint16_t handle;
    uint16_t acl_len;
    uint8_t pb;
    uint8_t bc;

    if ((packet == NULL) || (len < 4u)) {
        DBG_PRINT("HCI %s ACL len=%u malformed\r\n", direction,
                  (unsigned int)len);
        return;
    }

    header = btstack_hci_dump_read_le16(packet);
    handle = header & 0x0fffu;
    pb = (uint8_t)((header >> 12) & 0x03u);
    bc = (uint8_t)((header >> 14) & 0x03u);
    acl_len = btstack_hci_dump_read_le16(&packet[2]);

    if ((len >= 8u) && (pb != 1u)) {
        uint16_t l2cap_len = btstack_hci_dump_read_le16(&packet[4]);
        uint16_t cid = btstack_hci_dump_read_le16(&packet[6]);
        DBG_PRINT("HCI %s ACL len=%u handle=0x%04X pb=%u bc=%u acl_len=%u l2cap_len=%u cid=0x%04X\r\n",
                  direction, (unsigned int)len, (unsigned int)handle,
                  (unsigned int)pb, (unsigned int)bc,
                  (unsigned int)acl_len, (unsigned int)l2cap_len,
                  (unsigned int)cid);
        return;
    }

    DBG_PRINT("HCI %s ACL len=%u handle=0x%04X pb=%u bc=%u acl_len=%u\r\n",
              direction, (unsigned int)len, (unsigned int)handle,
              (unsigned int)pb, (unsigned int)bc,
              (unsigned int)acl_len);
}

static void btstack_hci_dump_gpcv2178_log_sco(uint8_t in,
                                               const uint8_t *packet,
                                               uint16_t len)
{
    const char *direction = btstack_hci_dump_direction(in);
    uint16_t header;

    if ((packet == NULL) || (len < 3u)) {
        DBG_PRINT("HCI %s SCO len=%u malformed\r\n", direction,
                  (unsigned int)len);
        return;
    }

    header = btstack_hci_dump_read_le16(packet);
    DBG_PRINT("HCI %s SCO len=%u handle=0x%04X status=%u data_len=%u\r\n",
              direction, (unsigned int)len,
              (unsigned int)(header & 0x0fffu),
              (unsigned int)((header >> 12) & 0x03u),
              (unsigned int)packet[2]);
}

static void btstack_hci_dump_gpcv2178_log_iso(uint8_t in,
                                               const uint8_t *packet,
                                               uint16_t len)
{
    const char *direction = btstack_hci_dump_direction(in);
    uint16_t header;
    uint16_t iso_len;

    if ((packet == NULL) || (len < 4u)) {
        DBG_PRINT("HCI %s ISO len=%u malformed\r\n", direction,
                  (unsigned int)len);
        return;
    }

    header = btstack_hci_dump_read_le16(packet);
    iso_len = btstack_hci_dump_read_le16(&packet[2]) & 0x3fffu;
    DBG_PRINT("HCI %s ISO len=%u handle=0x%04X pb=%u ts=%u data_len=%u\r\n",
              direction, (unsigned int)len,
              (unsigned int)(header & 0x0fffu),
              (unsigned int)((header >> 12) & 0x03u),
              (unsigned int)((header >> 14) & 0x01u),
              (unsigned int)iso_len);
}

static void btstack_hci_dump_gpcv2178_log_packet(uint8_t packet_type,
                                                  uint8_t in,
                                                  uint8_t *packet,
                                                  uint16_t len)
{
    switch (packet_type) {
    case HCI_COMMAND_DATA_PACKET:
        if (in == 0u) {
            btstack_hci_dump_gpcv2178_log_full(
                "HCI TX CMD len=", packet, len);
        }
        break;
    case HCI_EVENT_PACKET:
        if (in != 0u) {
            btstack_hci_dump_gpcv2178_log_full(
                "HCI RX EVT len=", packet, len);
        }
        break;
    case HCI_ACL_DATA_PACKET:
        btstack_hci_dump_gpcv2178_log_acl(in, packet, len);
        break;
    case HCI_SCO_DATA_PACKET:
        btstack_hci_dump_gpcv2178_log_sco(in, packet, len);
        break;
    case HCI_ISO_DATA_PACKET:
        btstack_hci_dump_gpcv2178_log_iso(in, packet, len);
        break;
    default:
        break;
    }
}

static void btstack_hci_dump_gpcv2178_log_message(int log_level,
                                                   const char *format,
                                                   va_list argptr)
{
    (void)log_level;
    (void)format;
    (void)argptr;
}

static const hci_dump_t btstack_hci_dump_gpcv2178 = {
    NULL,
    &btstack_hci_dump_gpcv2178_log_packet,
    &btstack_hci_dump_gpcv2178_log_message
};

const hci_dump_t *btstack_hci_dump_gpcv2178_instance(void)
{
    return &btstack_hci_dump_gpcv2178;
}

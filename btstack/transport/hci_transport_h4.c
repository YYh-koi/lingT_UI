/*
 * Copyright (C) 2021 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BLUEKITCHEN
 * GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at
 * contact@bluekitchen-gmbh.com
 *
 */

#define BTSTACK_FILE__ "hci_transport_h4.c"

#include "btstack_config.h"

#include "hci_transport_h4.h"

#include "btstack_debug.h"
#include "btstack_util.h"
#include "hci.h"
#include "hci_transport.h"

#include <stdio.h>
#include <string.h>

typedef enum {
    H4_W4_PACKET_TYPE,
    H4_W4_HEADER,
    H4_W4_PAYLOAD
} hci_transport_h4_rx_state_t;

static const btstack_uart_t *btstack_uart;
static btstack_uart_config_t uart_config;

static void (*packet_handler)(uint8_t packet_type, uint8_t *packet, uint16_t size);

static uint8_t h4_incoming_type;
static uint8_t h4_incoming_packet_with_pre_buffer[HCI_INCOMING_PRE_BUFFER_SIZE + HCI_INCOMING_PACKET_BUFFER_SIZE];
static uint8_t *h4_incoming_packet = &h4_incoming_packet_with_pre_buffer[HCI_INCOMING_PRE_BUFFER_SIZE];
static uint16_t h4_header_size;
static uint16_t h4_payload_size;
static hci_transport_h4_rx_state_t h4_rx_state;

static uint8_t h4_outgoing_packet[1 + HCI_OUTGOING_PACKET_BUFFER_SIZE];
static uint8_t h4_tx_active;
static uint32_t h4_diag_tx_count;
static uint32_t h4_diag_rx_count;

static void hci_transport_h4_read_next(void);

static uint16_t hci_transport_h4_payload_size(uint8_t packet_type, const uint8_t *packet)
{
    switch (packet_type) {
        case HCI_EVENT_PACKET:
            return packet[1];
        case HCI_ACL_DATA_PACKET:
            return little_endian_read_16(packet, 2);
        case HCI_SCO_DATA_PACKET:
            return packet[2];
        case HCI_ISO_DATA_PACKET:
            return little_endian_read_16(packet, 2) & 0x3fffu;
        default:
            return 0;
    }
}

static uint16_t hci_transport_h4_header_size(uint8_t packet_type)
{
    switch (packet_type) {
        case HCI_EVENT_PACKET:
            return HCI_EVENT_HEADER_SIZE;
        case HCI_ACL_DATA_PACKET:
            return HCI_ACL_HEADER_SIZE;
        case HCI_SCO_DATA_PACKET:
            return HCI_SCO_HEADER_SIZE;
        case HCI_ISO_DATA_PACKET:
            return HCI_ISO_HEADER_SIZE;
        default:
            return 0;
    }
}

static void hci_transport_h4_reset_rx(void)
{
    h4_incoming_type = 0;
    h4_header_size = 0;
    h4_payload_size = 0;
    h4_rx_state = H4_W4_PACKET_TYPE;
}

static void hci_transport_h4_emit_transport_packet_sent(void)
{
    if (packet_handler == NULL) {
        return;
    }

    {
        uint8_t event[] = { HCI_EVENT_TRANSPORT_PACKET_SENT, 0 };
        packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
    }
}

static void hci_transport_h4_block_sent(void)
{
    h4_tx_active = 0;
    hci_transport_h4_emit_transport_packet_sent();
}

static void hci_transport_h4_packet_complete(void)
{
    uint16_t packet_size = h4_header_size + h4_payload_size;

    h4_diag_rx_count++;
    if ((h4_diag_rx_count <= 16u) || ((h4_diag_rx_count & 0x3fu) == 0u)) {
        BTSTACK_PRINTF("btstack h4: rx pkt #%u type=%u len=%u p=%02x %02x %02x %02x\r\n",
            (unsigned int)h4_diag_rx_count,
            (unsigned int)h4_incoming_type,
            (unsigned int)packet_size,
            (unsigned int)((packet_size > 0u) ? h4_incoming_packet[0] : 0u),
            (unsigned int)((packet_size > 1u) ? h4_incoming_packet[1] : 0u),
            (unsigned int)((packet_size > 2u) ? h4_incoming_packet[2] : 0u),
            (unsigned int)((packet_size > 3u) ? h4_incoming_packet[3] : 0u));
    }

    if (packet_handler != NULL) {
        packet_handler(h4_incoming_type, h4_incoming_packet, packet_size);
    }

    hci_transport_h4_reset_rx();
    hci_transport_h4_read_next();
}

static void hci_transport_h4_block_received(void)
{
    switch (h4_rx_state) {
        case H4_W4_PACKET_TYPE:
            h4_header_size = hci_transport_h4_header_size(h4_incoming_type);
            if (h4_header_size == 0u) {
                BTSTACK_PRINTF("btstack h4: drop unknown type=0x%02x\r\n", (unsigned int)h4_incoming_type);
                hci_transport_h4_reset_rx();
                hci_transport_h4_read_next();
                return;
            }
            h4_rx_state = H4_W4_HEADER;
            hci_transport_h4_read_next();
            break;

        case H4_W4_HEADER:
            h4_payload_size = hci_transport_h4_payload_size(h4_incoming_type, h4_incoming_packet);
            if ((h4_header_size + h4_payload_size) > HCI_INCOMING_PACKET_BUFFER_SIZE) {
                BTSTACK_PRINTF("btstack h4: drop oversize type=%u len=%u\r\n",
                    (unsigned int)h4_incoming_type,
                    (unsigned int)(h4_header_size + h4_payload_size));
                hci_transport_h4_reset_rx();
                hci_transport_h4_read_next();
                return;
            }
            if (h4_payload_size == 0u) {
                hci_transport_h4_packet_complete();
                return;
            }
            h4_rx_state = H4_W4_PAYLOAD;
            hci_transport_h4_read_next();
            break;

        case H4_W4_PAYLOAD:
            hci_transport_h4_packet_complete();
            break;

        default:
            hci_transport_h4_reset_rx();
            hci_transport_h4_read_next();
            break;
    }
}

static void hci_transport_h4_read_next(void)
{
    switch (h4_rx_state) {
        case H4_W4_PACKET_TYPE:
            btstack_uart->receive_block(&h4_incoming_type, 1);
            break;
        case H4_W4_HEADER:
            btstack_uart->receive_block(h4_incoming_packet, h4_header_size);
            break;
        case H4_W4_PAYLOAD:
            btstack_uart->receive_block(&h4_incoming_packet[h4_header_size], h4_payload_size);
            break;
        default:
            hci_transport_h4_reset_rx();
            btstack_uart->receive_block(&h4_incoming_type, 1);
            break;
    }
}

static void hci_transport_h4_init(const void *transport_config)
{
    h4_tx_active = 0;
    h4_diag_tx_count = 0;
    h4_diag_rx_count = 0;
    hci_transport_h4_reset_rx();

    if (transport_config == NULL) {
        log_error("hci_transport_h4: no config");
        return;
    }
    if (((const hci_transport_config_t *)transport_config)->type != HCI_TRANSPORT_CONFIG_UART) {
        log_error("hci_transport_h4: config is not UART");
        return;
    }

    {
        const hci_transport_config_uart_t *hci_transport_config_uart = (const hci_transport_config_uart_t *)transport_config;
        uart_config.baudrate = hci_transport_config_uart->baudrate_init;
        uart_config.flowcontrol = hci_transport_config_uart->flowcontrol;
        uart_config.parity = hci_transport_config_uart->parity;
        uart_config.device_name = hci_transport_config_uart->device_name;
    }

    btstack_uart->init(&uart_config);
    btstack_uart->set_block_received(&hci_transport_h4_block_received);
    btstack_uart->set_block_sent(&hci_transport_h4_block_sent);
}

static int hci_transport_h4_open(void)
{
    int res;

    BTSTACK_PRINTF("btstack h4: open baud=%u\r\n", (unsigned int)uart_config.baudrate);
    res = btstack_uart->open();
    if (res != 0) {
        return res;
    }

    hci_transport_h4_reset_rx();
    hci_transport_h4_read_next();
    return 0;
}

static int hci_transport_h4_close(void)
{
    return btstack_uart->close();
}

static void hci_transport_h4_register_packet_handler(void (*handler)(uint8_t packet_type, uint8_t *packet, uint16_t size))
{
    packet_handler = handler;
}

static int hci_transport_h4_can_send_packet_now(uint8_t packet_type)
{
    (void)packet_type;
    return h4_tx_active == 0u;
}

static int hci_transport_h4_send_packet(uint8_t packet_type, uint8_t *packet, int size)
{
    if (h4_tx_active) {
        return -1;
    }
    if ((packet == NULL) || (size < 0) || (((uint32_t)size + 1u) > sizeof(h4_outgoing_packet))) {
        return -1;
    }

    h4_outgoing_packet[0] = packet_type;
    memcpy(&h4_outgoing_packet[1], packet, (size_t)size);

    h4_diag_tx_count++;
    if ((h4_diag_tx_count <= 16u) || ((h4_diag_tx_count & 0x3fu) == 0u)) {
        BTSTACK_PRINTF("btstack h4: tx pkt #%u type=%u len=%u p=%02x %02x %02x %02x\r\n",
            (unsigned int)h4_diag_tx_count,
            (unsigned int)packet_type,
            (unsigned int)size,
            (unsigned int)((size > 0) ? packet[0] : 0u),
            (unsigned int)((size > 1) ? packet[1] : 0u),
            (unsigned int)((size > 2) ? packet[2] : 0u),
            (unsigned int)((size > 3) ? packet[3] : 0u));
    }

    h4_tx_active = 1;
    btstack_uart->send_block(h4_outgoing_packet, (uint16_t)(size + 1));
    return 0;
}

static int hci_transport_h4_set_baudrate(uint32_t baudrate)
{
    uart_config.baudrate = baudrate;
    return btstack_uart->set_baudrate(baudrate);
}

static const hci_transport_t hci_transport_h4 = {
    "H4",
    &hci_transport_h4_init,
    &hci_transport_h4_open,
    &hci_transport_h4_close,
    &hci_transport_h4_register_packet_handler,
    &hci_transport_h4_can_send_packet_now,
    &hci_transport_h4_send_packet,
    &hci_transport_h4_set_baudrate,
    NULL,
    NULL,
};

const hci_transport_t *hci_transport_h4_instance_for_uart(const btstack_uart_t *uart_driver)
{
    btstack_uart = uart_driver;
    return &hci_transport_h4;
}

const hci_transport_t *hci_transport_h4_instance(const btstack_uart_block_t *uart_driver)
{
    return hci_transport_h4_instance_for_uart((const btstack_uart_t *)uart_driver);
}

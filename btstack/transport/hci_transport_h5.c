/*
 * Copyright (C) 2016 BlueKitchen GmbH
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

#define BTSTACK_FILE__ "hci_transport_h5.c"

/*
 *  hci_transport_h5.c
 *
 *  HCI Transport API implementation for basic H5 protocol based on UART driver with SLIP support
 */

#include "btstack_config.h"

#ifdef ENABLE_H5

#include "hci_transport_h5.h"

#include "btstack_debug.h"
#include "hci.h"
#include "hci_transport.h"

#include <inttypes.h>
#include <stdio.h>

// assert pre-buffer for packet type is available
#if !defined(HCI_OUTGOING_PRE_BUFFER_SIZE) || (HCI_OUTGOING_PRE_BUFFER_SIZE < 4)
#error HCI_OUTGOING_PRE_BUFFER_SIZE not defined or smaller than 4. Please update hci.h
#endif

#ifndef BTSTACK_H5_LINK_PERIOD_MS
#define BTSTACK_H5_LINK_PERIOD_MS 250u
#endif

#ifndef BTSTACK_H5_LINK_RESEND_TIMEOUT_MIN_MS
#define BTSTACK_H5_LINK_RESEND_TIMEOUT_MIN_MS 0u
#endif

#ifndef BTSTACK_H5_SEND_EMPTY_CONFIG_RESPONSE
#define BTSTACK_H5_SEND_EMPTY_CONFIG_RESPONSE 0
#endif

#ifndef BTSTACK_H5_RESET_ON_ACTIVE_SYNC
#define BTSTACK_H5_RESET_ON_ACTIVE_SYNC 0
#endif

#ifndef BTSTACK_H5_DIAG_VERBOSE
#define BTSTACK_H5_DIAG_VERBOSE 0
#endif

#if BTSTACK_H5_DIAG_VERBOSE
#define H5_DIAG_PRINTF(...) BTSTACK_PRINTF(__VA_ARGS__)
#else
#define H5_DIAG_PRINTF(...) do { } while (0)
#endif

typedef enum {
    LINK_UNINITIALIZED,
    LINK_INITIALIZED,
    LINK_ACTIVE
} hci_transport_link_state_t;

typedef enum {
    HCI_TRANSPORT_LINK_SEND_SYNC                  = 1 <<  0,
    HCI_TRANSPORT_LINK_SEND_SYNC_RESPONSE         = 1 <<  1,
    HCI_TRANSPORT_LINK_SEND_CONFIG                = 1 <<  2,
    HCI_TRANSPORT_LINK_SEND_CONFIG_RESPONSE_EMPTY = 1 <<  3,
    HCI_TRANSPORT_LINK_SEND_CONFIG_RESPONSE       = 1 <<  4,
    HCI_TRANSPORT_LINK_SEND_SLEEP                 = 1 <<  5,
    HCI_TRANSPORT_LINK_SEND_WOKEN                 = 1 <<  6,
    HCI_TRANSPORT_LINK_SEND_WAKEUP                = 1 <<  7,
    HCI_TRANSPORT_LINK_SEND_QUEUED_PACKET         = 1 <<  8,
    HCI_TRANSPORT_LINK_SEND_ACK_PACKET            = 1 <<  9,
    HCI_TRANSPORT_LINK_ENTER_SLEEP                = 1 << 10,
    HCI_TRANSPORT_LINK_SET_BAUDRATE               = 1 << 11,

} hci_transport_link_actions_t;

// Configuration Field. GT vendor reference uses 0x14: sliding window = 4, no OOF flow control, support data integrity check
#define LINK_CONFIG_SLIDING_WINDOW_SIZE 4
#define LINK_CONFIG_OOF_FLOW_CONTROL 0
#define LINK_CONFIG_DATA_INTEGRITY_CHECK 1
#define LINK_CONFIG_VERSION_NR 0
#define LINK_CONFIG_FIELD (LINK_CONFIG_SLIDING_WINDOW_SIZE | (LINK_CONFIG_OOF_FLOW_CONTROL << 3) | (LINK_CONFIG_DATA_INTEGRITY_CHECK << 4) | (LINK_CONFIG_VERSION_NR << 5))

// periodic sending during link establishment
#define LINK_PERIOD_MS BTSTACK_H5_LINK_PERIOD_MS

// resend wakeup
#define LINK_WAKEUP_MS 50

// additional packet types
#define LINK_ACKNOWLEDGEMENT_TYPE 0x00
#define LINK_CONTROL_PACKET_TYPE 0x0f

// ---
static const uint8_t link_control_sync[] =   { 0x01, 0x7e};
static const uint8_t link_control_sync_response[] = { 0x02, 0x7d};
static const uint8_t link_control_config[] = { 0x03, 0xfc, LINK_CONFIG_FIELD};
static const uint8_t link_control_config_prefix_len  = 2;
static const uint8_t link_control_config_response_empty[] = { 0x04, 0x7b};
static const uint8_t link_control_config_response[] = { 0x04, 0x7b, LINK_CONFIG_FIELD};
static const uint8_t link_control_config_response_prefix_len  = 2;
static const uint8_t link_control_wakeup[] = { 0x05, 0xfa};
static const uint8_t link_control_woken[] =  { 0x06, 0xf9};
static const uint8_t link_control_sleep[] =  { 0x07, 0x78};

// max size of link control messages
#define LINK_CONTROL_MAX_LEN 3

// incoming pre-bufffer + 4 bytes H5 header + max(acl header + acl payload, event header + event data) + 2 bytes opt CRC
static uint8_t   hci_packet_with_pre_buffer[HCI_INCOMING_PRE_BUFFER_SIZE + 6 + HCI_INCOMING_PACKET_BUFFER_SIZE];

// outgoing slip encoded buffer. +4 to assert that DIC fits in buffer. +1 to assert that last SOF fits in buffer.
static int       slip_write_active;

// H5 Link State
static hci_transport_link_state_t link_state;
static btstack_timer_source_t link_timer;
static uint8_t  link_seq_nr;
static uint8_t  link_ack_nr;
static uint16_t link_resend_timeout_ms;
static uint8_t  link_peer_asleep;
static uint8_t  link_peer_supports_data_integrity_check;
static uint32_t link_new_baudrate;

// auto sleep-mode
static btstack_timer_source_t inactivity_timer;
static uint16_t link_inactivity_timeout_ms; // auto-sleep if set

// Outgoing packet
static uint8_t   hci_packet_type;
static uint16_t  hci_packet_size;
static uint8_t * hci_packet;

// restore 2 bytes temp overwritten by DIC
static uint8_t * hci_packet_restore_dic_address;
static uint16_t  hci_packet_restore_dic_data;

// hci packet handler
static  void (*packet_handler)(uint8_t packet_type, uint8_t *packet, uint16_t size);

static int hci_transport_link_actions;

// UART Driver + Config
static const btstack_uart_t * btstack_uart;
static btstack_uart_config_t uart_config;
static btstack_uart_sleep_mode_t btstack_uart_sleep_mode;
static int hci_transport_bcsp_mode;

static uint32_t hci_transport_h5_diag_sync_tx_count;
static uint32_t hci_transport_h5_diag_config_tx_count;
static uint32_t hci_transport_h5_diag_timeout_count;
static uint32_t hci_transport_h5_diag_rx_frame_count;
static uint32_t hci_transport_h5_diag_tx_packet_count;
#if BTSTACK_H5_DIAG_VERBOSE
static uint32_t hci_transport_h5_diag_rx_acl_sig_count;
static uint32_t hci_transport_h5_diag_rx_acl_data_count;
#endif

// Prototypes
static int  hci_transport_link_have_outgoing_packet(void);
static void hci_transport_h5_frame_sent(void);
static void hci_transport_h5_process_frame(uint16_t frame_size);
static void hci_transport_link_run(void);
static void hci_transport_link_send_queued_packet(void);
static void hci_transport_link_set_timer(uint16_t timeout_ms);
static void hci_transport_link_timeout_handler(btstack_timer_source_t * timer);
static void hci_transport_slip_init(void);

// -----------------------------
// CRC16-CCITT Calculation - compromise: use 32 byte table - 512 byte table would be faster, but that's too large

static const uint16_t crc16_ccitt_table[] ={
    0x0000, 0x1081, 0x2102, 0x3183,
    0x4204, 0x5285, 0x6306, 0x7387,
    0x8408, 0x9489, 0xa50a, 0xb58b,
    0xc60c, 0xd68d, 0xe70e, 0xf78f
};

static uint16_t crc16_ccitt_update (uint16_t crc, uint8_t ch){
    crc = (crc >> 4) ^ crc16_ccitt_table[(crc ^ ch) & 0x000f];
    crc = (crc >> 4) ^ crc16_ccitt_table[(crc ^ (ch >> 4)) & 0x000f];
    return crc;
}

static uint16_t btstack_reverse_bits_16(uint16_t value){
    int reverse = 0;
    int i;
    for (i = 0; i < 16; i++) { 
        reverse = reverse << 1;
        reverse |= value & 1;
        value = value >> 1;
    }
    return reverse;
}

static uint16_t crc16_calc_for_slip_frame(const uint8_t * data, uint16_t len){
    int i;
    uint16_t crc = 0xffff;
    for (i=0 ; i < len ; i++){
        crc = crc16_ccitt_update(crc, data[i]);
    }
    return btstack_reverse_bits_16(crc);
}

// -----------------------------
static void hci_transport_inactivity_timeout_handler(btstack_timer_source_t * ts){
    (void) ts;
    log_info("inactivity timeout. link state %d, peer asleep %u, actions 0x%02x, outgoing packet %u",
        link_state, link_peer_asleep, hci_transport_link_actions, hci_transport_link_have_outgoing_packet());
    if (hci_transport_link_have_outgoing_packet()) return;
    if (link_state != LINK_ACTIVE) return;
    if (hci_transport_link_actions) return;
    if (link_peer_asleep) return;
    hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_SLEEP;
    hci_transport_link_run();
}

static void hci_transport_inactivity_timer_set(void){
    if (!link_inactivity_timeout_ms) return;
    btstack_run_loop_set_timer_handler(&inactivity_timer, &hci_transport_inactivity_timeout_handler);
    btstack_run_loop_set_timer(&inactivity_timer, link_inactivity_timeout_ms);
    btstack_run_loop_remove_timer(&inactivity_timer);
    btstack_run_loop_add_timer(&inactivity_timer);
}

static void hci_transport_slip_init(void){
    btstack_uart->receive_frame(&hci_packet_with_pre_buffer[HCI_INCOMING_PRE_BUFFER_SIZE], 6 + HCI_INCOMING_PACKET_BUFFER_SIZE);
}

// H5 Three-Wire Implementation

static void hci_transport_link_calc_header(uint8_t * header,
    uint8_t  sequence_nr,
    uint8_t  acknowledgement_nr,
    uint8_t  data_integrity_check_present,
    uint8_t  reliable_packet,
    uint8_t  packet_type,
    uint16_t payload_length){

    // unreliable packets have seq_nr = 0
    if (reliable_packet == 0) {
        sequence_nr = 0;
    }

    header[0] = sequence_nr | (acknowledgement_nr << 3) | (data_integrity_check_present << 6) | (reliable_packet << 7);
    header[1] = packet_type | ((payload_length & 0x0f) << 4);
    header[2] = payload_length >> 4;
    header[3] = 0xff - (header[0] + header[1] + header[2]);
}

// Store DIC after packet, assuming 2 bytes in buffer - keep track of overwritten bytes - relevant for fragmented packets
static void hci_transport_slip_send_frame_with_dic(uint8_t * frame, uint16_t frame_size){
    int slip_outgoing_dic_present = frame[0] & 0x40;
    if (slip_outgoing_dic_present){
        // preserved data at DIC location
        hci_packet_restore_dic_address = &frame[frame_size];
        hci_packet_restore_dic_data = little_endian_read_16(hci_packet_restore_dic_address, 0);
        // calc and set DIC
        uint16_t data_integrity_check = crc16_calc_for_slip_frame(frame, frame_size);
        big_endian_store_16(frame, frame_size, data_integrity_check);
        frame_size += 2;
    }

    // set slip send active and go
    slip_write_active = 1;
    btstack_uart->send_frame(frame, frame_size);
}

static void hci_transport_link_send_queued_packet(void){
    uint8_t * buffer =      hci_packet      - 4;
    uint16_t  buffer_size = hci_packet_size + 4;

    // setup header
    int reliable = hci_packet_type == HCI_SCO_DATA_PACKET ? 0 : 1;
    hci_transport_link_calc_header(buffer, link_seq_nr, link_ack_nr, link_peer_supports_data_integrity_check, reliable, hci_packet_type, hci_packet_size);

    // send frame with dic
    hci_transport_h5_diag_tx_packet_count++;
    if ((hci_transport_h5_diag_tx_packet_count <= 8u) || ((hci_transport_h5_diag_tx_packet_count & 0x0fu) == 0u)){
        H5_DIAG_PRINTF("btstack h5: tx pkt #%u type=%u len=%u seq=%u ack=%u rel=%u dic=%u p=%02x %02x %02x %02x\r\n",
            (unsigned int)hci_transport_h5_diag_tx_packet_count,
            (unsigned int)hci_packet_type,
            (unsigned int)hci_packet_size,
            (unsigned int)link_seq_nr,
            (unsigned int)link_ack_nr,
            (unsigned int)reliable,
            (unsigned int)link_peer_supports_data_integrity_check,
            (unsigned int)((hci_packet_size > 0u) ? hci_packet[0] : 0u),
            (unsigned int)((hci_packet_size > 1u) ? hci_packet[1] : 0u),
            (unsigned int)((hci_packet_size > 2u) ? hci_packet[2] : 0u),
            (unsigned int)((hci_packet_size > 3u) ? hci_packet[3] : 0u));
    }
    log_debug("send queued packet: seq %u, ack %u, size %u, append dic %u", link_seq_nr, link_ack_nr, hci_packet_size, link_peer_supports_data_integrity_check);
    log_debug_hexdump(hci_packet, hci_packet_size);
    hci_transport_slip_send_frame_with_dic(buffer, buffer_size);

    // reset inactvitiy timer
    hci_transport_inactivity_timer_set();
}

static void hci_transport_link_send_control(const uint8_t * message, int message_len){
    uint8_t  buffer[4 + LINK_CONTROL_MAX_LEN + 2];
    uint16_t buffer_size = 4 + message_len;

    // setup header
    hci_transport_link_calc_header(buffer, 0, 0, link_peer_supports_data_integrity_check, 0, LINK_CONTROL_PACKET_TYPE, message_len);

    // setup payload
    memcpy(&buffer[4], message, message_len);

    // send frame with dic
    log_debug("send control: size %u, append dic %u", message_len, link_peer_supports_data_integrity_check);
    log_debug_hexdump(message, message_len);
    hci_transport_slip_send_frame_with_dic(buffer, buffer_size);
}

static void hci_transport_link_send_ack_packet(void){
    // Pure ACK package is without DIC as there is no payload either
    log_debug("send ack %u", link_ack_nr);
    uint8_t header[4];
    hci_transport_link_calc_header(header, 0, link_ack_nr, 0, 0, LINK_ACKNOWLEDGEMENT_TYPE, 0);
    hci_transport_slip_send_frame_with_dic(header, sizeof(header));
}

static void hci_transport_link_send_sync(void){
    hci_transport_h5_diag_sync_tx_count++;
    if ((hci_transport_h5_diag_sync_tx_count <= 8u) || ((hci_transport_h5_diag_sync_tx_count & 0x0fu) == 0u)){
        BTSTACK_PRINTF("btstack h5: tx sync #%u state=%d actions=0x%x\r\n",
            (unsigned int)hci_transport_h5_diag_sync_tx_count,
            (int)link_state,
            (unsigned int)hci_transport_link_actions);
    }
    log_debug("link send sync");
    hci_transport_link_send_control(link_control_sync, sizeof(link_control_sync));
}

static void hci_transport_link_send_sync_response(void){
    BTSTACK_PRINTF("btstack h5: tx sync response state=%d\r\n", (int)link_state);
    log_debug("link send sync response");
    hci_transport_link_send_control(link_control_sync_response, sizeof(link_control_sync_response));
}

static void hci_transport_link_send_config(void){
    hci_transport_h5_diag_config_tx_count++;
    if ((hci_transport_h5_diag_config_tx_count <= 8u) || ((hci_transport_h5_diag_config_tx_count & 0x0fu) == 0u)){
        BTSTACK_PRINTF("btstack h5: tx config #%u state=%d dic=%u\r\n",
            (unsigned int)hci_transport_h5_diag_config_tx_count,
            (int)link_state,
            (unsigned int)link_peer_supports_data_integrity_check);
    }
    log_debug("link send config");
    hci_transport_link_send_control(link_control_config, sizeof(link_control_config));
}

static void hci_transport_link_send_config_response(void){
    BTSTACK_PRINTF("btstack h5: tx config response state=%d\r\n", (int)link_state);
    log_debug("link send config response");
#if BTSTACK_H5_SEND_EMPTY_CONFIG_RESPONSE
    hci_transport_link_send_control(link_control_config_response_empty, sizeof(link_control_config_response_empty));
#else
    hci_transport_link_send_control(link_control_config_response, sizeof(link_control_config_response));
#endif
}

static void hci_transport_link_send_config_response_empty(void){
    BTSTACK_PRINTF("btstack h5: tx empty config response state=%d\r\n", (int)link_state);
    log_debug("link send config response empty");
    hci_transport_link_send_control(link_control_config_response_empty, sizeof(link_control_config_response_empty));
}

static void hci_transport_link_send_woken(void){
    log_debug("link send woken");
    hci_transport_link_send_control(link_control_woken, sizeof(link_control_woken));
}

static void hci_transport_link_send_wakeup(void){
    log_debug("link send wakeup");
    hci_transport_link_send_control(link_control_wakeup, sizeof(link_control_wakeup));
}

static void hci_transport_link_send_sleep(void){
    log_debug("link send sleep");
    hci_transport_link_send_control(link_control_sleep, sizeof(link_control_sleep));
}

static void hci_transport_link_run(void){
    // exit if outgoing active
    if (slip_write_active) return;

    // process queued requests
    if (hci_transport_link_actions & HCI_TRANSPORT_LINK_SEND_SYNC){
        hci_transport_link_actions &= ~HCI_TRANSPORT_LINK_SEND_SYNC;
        hci_transport_link_send_sync();
        return;
    }
    if (hci_transport_link_actions & HCI_TRANSPORT_LINK_SEND_SYNC_RESPONSE){
        hci_transport_link_actions &= ~HCI_TRANSPORT_LINK_SEND_SYNC_RESPONSE;
        hci_transport_link_send_sync_response();
        return;
    }
    if (hci_transport_link_actions & HCI_TRANSPORT_LINK_SEND_CONFIG){
        hci_transport_link_actions &= ~HCI_TRANSPORT_LINK_SEND_CONFIG;
        hci_transport_link_send_config();
        return;
    }
    if (hci_transport_link_actions & HCI_TRANSPORT_LINK_SEND_CONFIG_RESPONSE){
        hci_transport_link_actions &= ~HCI_TRANSPORT_LINK_SEND_CONFIG_RESPONSE;
        hci_transport_link_send_config_response();
        return;
    }
    if (hci_transport_link_actions & HCI_TRANSPORT_LINK_SEND_CONFIG_RESPONSE_EMPTY){
        hci_transport_link_actions &= ~HCI_TRANSPORT_LINK_SEND_CONFIG_RESPONSE_EMPTY;
        hci_transport_link_send_config_response_empty();
        return;
    }
    if (hci_transport_link_actions & HCI_TRANSPORT_LINK_SEND_WOKEN){
        hci_transport_link_actions &= ~HCI_TRANSPORT_LINK_SEND_WOKEN;
        hci_transport_link_send_woken();
        return;
    }
    if (hci_transport_link_actions & HCI_TRANSPORT_LINK_SEND_WAKEUP){
        hci_transport_link_actions &= ~HCI_TRANSPORT_LINK_SEND_WAKEUP;
        hci_transport_link_send_wakeup();
        return;
    }
    if (hci_transport_link_actions & HCI_TRANSPORT_LINK_SEND_QUEUED_PACKET){
        hci_transport_link_actions &= ~HCI_TRANSPORT_LINK_SEND_QUEUED_PACKET;
        // packet already contains ack, no need to send addtitional one
        hci_transport_link_actions &= ~HCI_TRANSPORT_LINK_SEND_ACK_PACKET;
        hci_transport_link_send_queued_packet();
        return;
    }
    if (hci_transport_link_actions & HCI_TRANSPORT_LINK_SEND_ACK_PACKET){
        hci_transport_link_actions &= ~HCI_TRANSPORT_LINK_SEND_ACK_PACKET;
        hci_transport_link_send_ack_packet();
        return;
    }
    if (hci_transport_link_actions & HCI_TRANSPORT_LINK_SEND_SLEEP){
        hci_transport_link_actions &= ~HCI_TRANSPORT_LINK_SEND_SLEEP;
        hci_transport_link_actions |=  HCI_TRANSPORT_LINK_ENTER_SLEEP;
        link_peer_asleep = 1;
        hci_transport_link_send_sleep();
        return;
    }
}

static void hci_transport_link_set_timer(uint16_t timeout_ms){
    btstack_run_loop_set_timer_handler(&link_timer, &hci_transport_link_timeout_handler);
    btstack_run_loop_set_timer(&link_timer, timeout_ms);
    btstack_run_loop_add_timer(&link_timer);
}

static void hci_transport_link_timeout_handler(btstack_timer_source_t * timer){
    (void)timer;
    hci_transport_h5_diag_timeout_count++;
    if ((hci_transport_h5_diag_timeout_count <= 8u) || ((hci_transport_h5_diag_timeout_count & 0x0fu) == 0u)){
        H5_DIAG_PRINTF("btstack h5: timeout #%u state=%d actions=0x%x outgoing=%u\r\n",
            (unsigned int)hci_transport_h5_diag_timeout_count,
            (int)link_state,
            (unsigned int)hci_transport_link_actions,
            (unsigned int)hci_transport_link_have_outgoing_packet());
    }
    switch (link_state){
        case LINK_UNINITIALIZED:
            hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_SYNC;
            hci_transport_link_set_timer(LINK_PERIOD_MS);
            break;            
        case LINK_INITIALIZED:
            hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_CONFIG;
            hci_transport_link_set_timer(LINK_PERIOD_MS);
            break;
        case LINK_ACTIVE:
            if (!hci_transport_link_have_outgoing_packet()){
                log_info("h5 timeout while active, but no outgoing packet");
                return;
            }
            if (link_peer_asleep){
                hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_WAKEUP;
                hci_transport_link_set_timer(LINK_WAKEUP_MS);
                break;
            }
            // resend packet
            hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_QUEUED_PACKET;
            hci_transport_link_set_timer(link_resend_timeout_ms);
            break;
        default:
            break;
    }

    hci_transport_link_run();
}

static void hci_transport_link_init(void){
    link_state = LINK_UNINITIALIZED;
    link_peer_asleep = 0;
    link_peer_supports_data_integrity_check = 0;
 
    // get started
    hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_SYNC;
    hci_transport_link_set_timer(LINK_PERIOD_MS);
    hci_transport_link_run();
}

static int hci_transport_link_inc_seq_nr(int seq_nr){
    return (seq_nr + 1) & 0x07;    
}

static int hci_transport_link_have_outgoing_packet(void){
    return hci_packet != 0;
}

static void hci_transport_link_clear_queue(void){
    btstack_run_loop_remove_timer(&link_timer);
    hci_packet = NULL;
}

static void hci_transport_h5_queue_packet(uint8_t packet_type, uint8_t *packet, int size){
    hci_packet = packet;
    hci_packet_type = packet_type;
    hci_packet_size = size;
}

static void hci_transport_h5_emit_sleep_state(int sleep_active){
    static int last_state = 0;
    if (sleep_active == last_state) return;
    last_state = sleep_active;
    
    log_info("emit_sleep_state: %u", sleep_active);
    uint8_t event[3];
    event[0] = HCI_EVENT_TRANSPORT_SLEEP_MODE;
    event[1] = sizeof(event) - 2;
    event[2] = sleep_active;
    packet_handler(HCI_EVENT_PACKET, &event[0], sizeof(event));        
}

#if BTSTACK_H5_DIAG_VERBOSE
static void hci_transport_h5_log_acl_signaling_packet(uint8_t *packet, uint16_t size){
    uint16_t acl_handle;
    uint16_t acl_len;
    uint16_t l2cap_len;
    uint16_t l2cap_cid;
    uint8_t sig_code;
    uint8_t sig_id;
    uint16_t sig_len;
    uint16_t psm;
    uint16_t source_cid;

    if (size < 8u) {
        return;
    }

    l2cap_cid = little_endian_read_16(packet, 6);
    if (l2cap_cid != 0x0001u) {
        return;
    }

    acl_handle = little_endian_read_16(packet, 0) & 0x0fffu;
    acl_len = little_endian_read_16(packet, 2);
    l2cap_len = little_endian_read_16(packet, 4);
    sig_code = (size > 8u) ? packet[8] : 0u;
    sig_id = (size > 9u) ? packet[9] : 0u;
    sig_len = (size >= 12u) ? little_endian_read_16(packet, 10) : 0u;
    psm = ((sig_code == 0x02u) && (size >= 16u)) ? little_endian_read_16(packet, 12) : 0u;
    source_cid = ((sig_code == 0x02u) && (size >= 16u)) ? little_endian_read_16(packet, 14) : 0u;

    hci_transport_h5_diag_rx_acl_sig_count++;
    H5_DIAG_PRINTF("btstack h5: rx acl-sig #%u handle=0x%04x acl_len=%u l2cap_len=%u code=%u id=%u sig_len=%u psm=0x%04x source_cid=0x%04x\r\n",
        (unsigned int)hci_transport_h5_diag_rx_acl_sig_count,
        (unsigned int)acl_handle,
        (unsigned int)acl_len,
        (unsigned int)l2cap_len,
        (unsigned int)sig_code,
        (unsigned int)sig_id,
        (unsigned int)sig_len,
        (unsigned int)psm,
        (unsigned int)source_cid);
}

static void hci_transport_h5_log_acl_data_packet(uint8_t *packet, uint16_t size){
    uint16_t acl_handle;
    uint16_t acl_len;
    uint16_t l2cap_len;
    uint16_t l2cap_cid;
    uint8_t *payload;
    uint16_t payload_len;

    if (size < 8u) {
        return;
    }

    acl_handle = little_endian_read_16(packet, 0) & 0x0fffu;
    acl_len = little_endian_read_16(packet, 2);
    l2cap_len = little_endian_read_16(packet, 4);
    l2cap_cid = little_endian_read_16(packet, 6);

    if (l2cap_cid < 0x0040u) {
        return;
    }

    hci_transport_h5_diag_rx_acl_data_count++;
    if (hci_transport_h5_diag_rx_acl_data_count > 32u) {
        return;
    }

    payload = &packet[8];
    payload_len = (size > 8u) ? (uint16_t)(size - 8u) : 0u;

    H5_DIAG_PRINTF("btstack h5: rx acl-data #%u handle=0x%04x acl_len=%u l2cap_len=%u cid=0x%04x p=%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
        (unsigned int)hci_transport_h5_diag_rx_acl_data_count,
        (unsigned int)acl_handle,
        (unsigned int)acl_len,
        (unsigned int)l2cap_len,
        (unsigned int)l2cap_cid,
        (unsigned int)((payload_len > 0u) ? payload[0] : 0u),
        (unsigned int)((payload_len > 1u) ? payload[1] : 0u),
        (unsigned int)((payload_len > 2u) ? payload[2] : 0u),
        (unsigned int)((payload_len > 3u) ? payload[3] : 0u),
        (unsigned int)((payload_len > 4u) ? payload[4] : 0u),
        (unsigned int)((payload_len > 5u) ? payload[5] : 0u),
        (unsigned int)((payload_len > 6u) ? payload[6] : 0u),
        (unsigned int)((payload_len > 7u) ? payload[7] : 0u));
}
#endif

static void hci_transport_h5_process_frame(uint16_t frame_size){

    if (frame_size < 4) return;

    uint8_t * slip_header  = &hci_packet_with_pre_buffer[HCI_INCOMING_PRE_BUFFER_SIZE];
    uint8_t * slip_payload = &hci_packet_with_pre_buffer[HCI_INCOMING_PRE_BUFFER_SIZE + 4];
    int       frame_size_without_header = frame_size - 4;

    uint8_t  seq_nr =  slip_header[0] & 0x07;
    uint8_t  ack_nr = (slip_header[0] >> 3)    & 0x07;
    uint8_t  data_integrity_check_present = (slip_header[0] & 0x40) != 0;
    uint8_t  reliable_packet  = (slip_header[0] & 0x80) != 0;
    uint8_t  link_packet_type = slip_header[1] & 0x0f;
    uint16_t link_payload_len = (slip_header[1] >> 4) | (slip_header[2] << 4);

    hci_transport_h5_diag_rx_frame_count++;
    if ((hci_transport_h5_diag_rx_frame_count <= 16u) || ((hci_transport_h5_diag_rx_frame_count & 0x0fu) == 0u)){
        H5_DIAG_PRINTF("btstack h5: rx frame #%u state=%d type=%u rel=%u len=%u p=%02x %02x %02x\r\n",
            (unsigned int)hci_transport_h5_diag_rx_frame_count,
            (int)link_state,
            (unsigned int)link_packet_type,
            (unsigned int)reliable_packet,
            (unsigned int)link_payload_len,
            (unsigned int)((frame_size_without_header > 0) ? slip_payload[0] : 0),
            (unsigned int)((frame_size_without_header > 1) ? slip_payload[1] : 0),
            (unsigned int)((frame_size_without_header > 2) ? slip_payload[2] : 0));
    }

    log_debug("process_frame, reliable %u, packet type %u, seq_nr %u, ack_nr %u , dic %u, payload 0x%04x bytes", reliable_packet, link_packet_type, seq_nr, ack_nr, data_integrity_check_present, frame_size_without_header);
    log_debug_hexdump(slip_header, 4);
    log_debug_hexdump(slip_payload, frame_size_without_header);

    // CSR 8811 does not seem to auto-detect H5 mode and sends data with even parity.
    // if this byte sequence is detected, just enable even parity
    const uint8_t sync_response_bcsp[] = {0x01, 0x7a, 0x06, 0x10};
    if (memcmp(sync_response_bcsp, slip_header, 4) == 0){
        log_info("detected BSCP SYNC sent with Even Parity -> discard frame and enable Even Parity");
        btstack_uart->set_parity(BTSTACK_UART_PARITY_EVEN);
        return;
    }

    // validate header checksum
    uint8_t header_checksum = slip_header[0] + slip_header[1] + slip_header[2] + slip_header[3];
    if (header_checksum != 0xff){
        BTSTACK_PRINTF("btstack h5: rx bad header checksum=0x%02x h=%02x %02x %02x %02x\r\n",
            (unsigned int)header_checksum,
            (unsigned int)slip_header[0],
            (unsigned int)slip_header[1],
            (unsigned int)slip_header[2],
            (unsigned int)slip_header[3]);
        log_info("header checksum 0x%02x (instead of 0xff)", header_checksum);
        return;
    }

    // validate payload length
    int data_integrity_len = data_integrity_check_present ? 2 : 0;
    uint16_t received_payload_len = frame_size_without_header - data_integrity_len;
    if (link_payload_len != received_payload_len){
        BTSTACK_PRINTF("btstack h5: rx bad len expected=%u got=%u dic=%u\r\n",
            (unsigned int)link_payload_len,
            (unsigned int)received_payload_len,
            (unsigned int)data_integrity_check_present);
        log_info("expected payload len %u but got %u", link_payload_len, received_payload_len);
        return;
    }

    // validate data integrity check
    if (data_integrity_check_present){
        uint16_t dic_packet = big_endian_read_16(slip_payload, received_payload_len);
        uint16_t dic_calculate = crc16_calc_for_slip_frame(slip_header, 4 + received_payload_len);
        if (dic_packet != dic_calculate){
            BTSTACK_PRINTF("btstack h5: rx bad dic expected=0x%04x got=0x%04x len=%u\r\n",
                (unsigned int)dic_calculate,
                (unsigned int)dic_packet,
                (unsigned int)received_payload_len);
            log_info("expected dic value 0x%04x but got 0x%04x", dic_calculate, dic_packet);
            return;
        }
    }

    switch (link_state){
        case LINK_UNINITIALIZED:
            if (link_packet_type != LINK_CONTROL_PACKET_TYPE) break;
            if (memcmp(slip_payload, link_control_sync, sizeof(link_control_sync)) == 0){
                log_debug("link received sync");
                hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_SYNC_RESPONSE;
                break;
            }
            if (memcmp(slip_payload, link_control_sync_response, sizeof(link_control_sync_response)) == 0){
                log_debug("link received sync response");
                link_state = LINK_INITIALIZED;
                btstack_run_loop_remove_timer(&link_timer);
                BTSTACK_PRINTF("btstack h5: link initialized\r\n");
                log_info("link initialized");
                //
                hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_CONFIG;
                hci_transport_link_set_timer(LINK_PERIOD_MS);
                break;
            }
            break;
        case LINK_INITIALIZED:
            if (link_packet_type != LINK_CONTROL_PACKET_TYPE) break;
            if (memcmp(slip_payload, link_control_sync, sizeof(link_control_sync)) == 0){
                log_debug("link received sync");
                hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_SYNC_RESPONSE;
                break;
            }
            if (memcmp(slip_payload, link_control_config, link_control_config_prefix_len) == 0){
                if (link_payload_len == link_control_config_prefix_len){
                    log_debug("link received config, no config field");
                    hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_CONFIG_RESPONSE_EMPTY;
                } else {
                    log_debug("link received config, 0x%02x", slip_payload[2]);
                    hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_CONFIG_RESPONSE;
                }
                break;
            }
            if (memcmp(slip_payload, link_control_config_response, link_control_config_response_prefix_len) == 0){
                uint8_t config = (link_payload_len > link_control_config_response_prefix_len) ? slip_payload[2] : 0;
                link_peer_supports_data_integrity_check = (config & 0x10) != 0;
                log_info("link received config response 0x%02x, data integrity check supported %u", config, link_peer_supports_data_integrity_check);
                link_state = LINK_ACTIVE;
                btstack_run_loop_remove_timer(&link_timer);
                BTSTACK_PRINTF("btstack h5: link activated dic=%u\r\n", (unsigned int)link_peer_supports_data_integrity_check);
                log_info("link activated");
                // 
                link_seq_nr = 0;
                link_ack_nr = 0;
                // notify upper stack that it can start
                uint8_t event[] = { HCI_EVENT_TRANSPORT_PACKET_SENT, 0};
                packet_handler(HCI_EVENT_PACKET, &event[0], sizeof(event));
                break;
            }
            break;
        case LINK_ACTIVE:

            // validate packet sequence nr in reliable packets (check for out of sequence error)
            if (reliable_packet){
                if (seq_nr != link_ack_nr){
                    log_info("expected seq nr %u, but received %u", link_ack_nr, seq_nr);
                    hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_ACK_PACKET;
                    break;
                }
                // ack packet right away
                link_ack_nr = hci_transport_link_inc_seq_nr(link_ack_nr);
                hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_ACK_PACKET;
            }

            // Process ACKs in reliable packet and explicit ack packets
            if (reliable_packet || link_packet_type == LINK_ACKNOWLEDGEMENT_TYPE){
                // our packet is good if the remote expects our seq nr + 1
                int next_seq_nr = hci_transport_link_inc_seq_nr(link_seq_nr);
                if (hci_transport_link_have_outgoing_packet() && next_seq_nr == ack_nr){
                    H5_DIAG_PRINTF("btstack h5: tx pkt acked seq=%u ack=%u type=%u\r\n",
                        (unsigned int)link_seq_nr,
                        (unsigned int)ack_nr,
                        (unsigned int)link_packet_type);
                    log_debug("outoing packet with seq %u ack'ed", link_seq_nr);
                    link_seq_nr = next_seq_nr;
                    hci_transport_link_clear_queue();

                    // notify upper stack that it can send again
                    uint8_t event[] = { HCI_EVENT_TRANSPORT_PACKET_SENT, 0};
                    packet_handler(HCI_EVENT_PACKET, &event[0], sizeof(event));
                }
            } 

            switch (link_packet_type){
                case LINK_CONTROL_PACKET_TYPE:
                    if (memcmp(slip_payload, link_control_config, link_control_config_prefix_len) == 0){
                        if (link_payload_len == link_control_config_prefix_len){
                            log_debug("link received config, no config field");
                            hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_CONFIG_RESPONSE_EMPTY;
                        } else {
                            log_debug("link received config, 0x%02x", slip_payload[2]);
                            hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_CONFIG_RESPONSE;
                        }
                        break;
                    }
                    if (memcmp(slip_payload, link_control_sync, sizeof(link_control_sync)) == 0){
                        log_debug("link received sync in ACTIVE STATE!");
#if BTSTACK_H5_RESET_ON_ACTIVE_SYNC
                        BTSTACK_PRINTF("btstack h5: peer sync in active, restart link\r\n");
                        btstack_run_loop_remove_timer(&link_timer);
                        hci_packet = NULL;
                        link_seq_nr = 0;
                        link_ack_nr = 0;
                        link_peer_asleep = 0;
                        link_state = LINK_UNINITIALIZED;
                        hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_SYNC_RESPONSE;
                        hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_SYNC;
                        hci_transport_link_set_timer(LINK_PERIOD_MS);
#endif
                        break;
                    }
                    if (memcmp(slip_payload, link_control_sleep, sizeof(link_control_sleep)) == 0){
                        if (btstack_uart_sleep_mode){
                            log_info("link: received sleep message. Enabling UART Sleep.");
                            btstack_uart->set_sleep(btstack_uart_sleep_mode);
                            hci_transport_h5_emit_sleep_state(1);
                        } else {
                            log_info("link: received sleep message. UART Sleep not supported");
                        }
                        link_peer_asleep = 1;
                        break;
                    }
                    if (memcmp(slip_payload, link_control_wakeup, sizeof(link_control_wakeup)) == 0){
                        log_info("link: received wakupe message -> send woken");
                        link_peer_asleep = 0;
                        hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_WOKEN;
                        break;
                    }
                    if (memcmp(slip_payload, link_control_woken, sizeof(link_control_woken)) == 0){
                        log_info("link: received woken message");
                        link_peer_asleep = 0;
                        // queued packet will be sent in hci_transport_link_run if needed
                        break;
                    }
                    break;
                case HCI_EVENT_PACKET:
                case HCI_ACL_DATA_PACKET:
                case HCI_SCO_DATA_PACKET:
                case HCI_ISO_DATA_PACKET:
                    // seems like peer is awake
                    link_peer_asleep = 0;
#if BTSTACK_H5_DIAG_VERBOSE
                    if (link_packet_type == HCI_ACL_DATA_PACKET){
                        hci_transport_h5_log_acl_signaling_packet(slip_payload, link_payload_len);
                        hci_transport_h5_log_acl_data_packet(slip_payload, link_payload_len);
                    }
#endif
                    // forward packet to stack
                    packet_handler(link_packet_type, slip_payload, link_payload_len);
                    // reset inactvitiy timer
                    hci_transport_inactivity_timer_set();
                    break;
                default:
                    // ignore unknown packet type
                    break;
            }
            break;
        default:
            break;
    }

    hci_transport_link_run();
}

// recommended time until resend: 3 * time of largest packet
static uint16_t hci_transport_link_calc_resend_timeout(uint32_t baudrate){
    uint32_t max_packet_size_in_bit = (HCI_INCOMING_PACKET_BUFFER_SIZE + 6) << 3;
    uint32_t t_max_x3_ms = max_packet_size_in_bit * 3000 / baudrate;

    // allow for BTstack logging and other delays
    t_max_x3_ms += 50;
#if BTSTACK_H5_LINK_RESEND_TIMEOUT_MIN_MS > 0
    if (t_max_x3_ms < BTSTACK_H5_LINK_RESEND_TIMEOUT_MIN_MS){
        t_max_x3_ms = BTSTACK_H5_LINK_RESEND_TIMEOUT_MIN_MS;
    }
#endif

    log_info("resend timeout for %"PRIu32" baud: %u ms", baudrate, (int) t_max_x3_ms);
    return t_max_x3_ms;
}

static void hci_transport_link_update_resend_timeout(uint32_t baudrate){
    link_resend_timeout_ms = hci_transport_link_calc_resend_timeout(baudrate);
}

/// H5 Interface

static void hci_transport_h5_frame_received(uint16_t frame_size){
    hci_transport_h5_process_frame(frame_size);
    hci_transport_slip_init();
}

static void hci_transport_h5_frame_sent(void){

    // restore DIC and clear flag
    if (hci_packet_restore_dic_address){
        little_endian_store_16(hci_packet_restore_dic_address, 0, hci_packet_restore_dic_data);
        hci_packet_restore_dic_address = NULL;
    }

    // done
    slip_write_active = 0;

    // baudrate change pending?
    if (hci_transport_link_actions & HCI_TRANSPORT_LINK_SET_BAUDRATE){
        hci_transport_link_actions &= ~HCI_TRANSPORT_LINK_SET_BAUDRATE;
        btstack_uart->set_baudrate(link_new_baudrate);
        hci_transport_link_update_resend_timeout(link_new_baudrate);
    }

    // enter sleep mode after sending sleep message
    if (hci_transport_link_actions & HCI_TRANSPORT_LINK_ENTER_SLEEP){
        hci_transport_link_actions &= ~HCI_TRANSPORT_LINK_ENTER_SLEEP;
        if (btstack_uart_sleep_mode){
            log_info("link: sent sleep message. Enabling UART Sleep.");
            btstack_uart->set_sleep(btstack_uart_sleep_mode);
        } else {
            log_info("link: sent sleep message. UART Sleep not supported");
        }
        hci_transport_h5_emit_sleep_state(1);
    }

    // SCO packets are sent as unreliable, so we're done now
    if (hci_packet_type == HCI_SCO_DATA_PACKET){
        hci_transport_link_clear_queue();
        // notify upper stack that it can send again
        uint8_t event[] = { HCI_EVENT_TRANSPORT_PACKET_SENT, 0};
        packet_handler(HCI_EVENT_PACKET, &event[0], sizeof(event));
    }

    hci_transport_link_run();
}

static void hci_transport_h5_init(const void * transport_config){
    hci_transport_h5_diag_sync_tx_count = 0;
    hci_transport_h5_diag_config_tx_count = 0;
    hci_transport_h5_diag_timeout_count = 0;
    hci_transport_h5_diag_rx_frame_count = 0;
    hci_transport_h5_diag_tx_packet_count = 0;
#if BTSTACK_H5_DIAG_VERBOSE
    hci_transport_h5_diag_rx_acl_sig_count = 0;
    hci_transport_h5_diag_rx_acl_data_count = 0;
#endif

    // check for hci_transport_config_uart_t
    if (!transport_config) {
        log_error("hci_transport_h5: no config!");
        return;
    }
    if (((hci_transport_config_t*)transport_config)->type != HCI_TRANSPORT_CONFIG_UART) {
        log_error("hci_transport_h5: config not of type != HCI_TRANSPORT_CONFIG_UART!");
        return;
    }

    // extract UART config from transport config
    hci_transport_config_uart_t * hci_transport_config_uart = (hci_transport_config_uart_t*) transport_config;
    uart_config.baudrate    = hci_transport_config_uart->baudrate_init;
    uart_config.flowcontrol = hci_transport_config_uart->flowcontrol;
    uart_config.parity      = hci_transport_config_uart->parity;
    uart_config.device_name = hci_transport_config_uart->device_name;

    // setup UART driver
    btstack_uart->init(&uart_config);
    btstack_uart->set_frame_received(&hci_transport_h5_frame_received);
    btstack_uart->set_frame_sent(&hci_transport_h5_frame_sent);
}

static int hci_transport_h5_open(void){
    int res;
    BTSTACK_PRINTF("btstack h5: open baud=%u\r\n", (unsigned int)uart_config.baudrate);
    res = btstack_uart->open();
    if (res){
        return res;
    }        
    
    // 
    if (hci_transport_bcsp_mode){
        log_info("enable even parity for BCSP mode");
        btstack_uart->set_parity(BTSTACK_UART_PARITY_EVEN);
    }

    // check if wake on RX can be used
    btstack_uart_sleep_mode = BTSTACK_UART_SLEEP_OFF;
    int supported_sleep_modes = 0;
    if (btstack_uart->get_supported_sleep_modes){
        supported_sleep_modes = btstack_uart->get_supported_sleep_modes();
    }
    if (supported_sleep_modes & BTSTACK_UART_SLEEP_MASK_RTS_LOW_WAKE_ON_RX_EDGE){
        log_info("using wake on RX");
        btstack_uart_sleep_mode = BTSTACK_UART_SLEEP_RTS_LOW_WAKE_ON_RX_EDGE;
    } else {
        log_info("UART driver does not provide compatible sleep mode");
    }

    // setup resend timeout
    hci_transport_link_update_resend_timeout(uart_config.baudrate);

    // init link management - already starts syncing
    hci_transport_link_init();

    // start receiving
    hci_transport_slip_init();

    return 0;
}

static int hci_transport_h5_close(void){
    return btstack_uart->close();
}

static void hci_transport_h5_register_packet_handler(void (*handler)(uint8_t packet_type, uint8_t *packet, uint16_t size)){
    packet_handler = handler;
}

static int hci_transport_h5_can_send_packet_now(uint8_t packet_type){
    (void) packet_type;
    int res = !hci_transport_link_have_outgoing_packet() && link_state == LINK_ACTIVE;
    // log_info("can_send_packet_now: %u", res);
    return res;
}

static int hci_transport_h5_send_packet(uint8_t packet_type, uint8_t *packet, int size){
    if (!hci_transport_h5_can_send_packet_now(packet_type)){
        log_error("hci_transport_h5_send_packet called but in state %d", link_state);
        return -1;
    }

    // store request
    hci_transport_h5_queue_packet(packet_type, packet, size);

    // send wakeup first
    if (link_peer_asleep){
        hci_transport_h5_emit_sleep_state(0);
        if (btstack_uart_sleep_mode){
            log_info("disable UART sleep");
            btstack_uart->set_sleep(BTSTACK_UART_SLEEP_OFF);
        }
        hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_WAKEUP;
        hci_transport_link_set_timer(LINK_WAKEUP_MS);
    } else {
        hci_transport_link_actions |= HCI_TRANSPORT_LINK_SEND_QUEUED_PACKET;
        hci_transport_link_set_timer(link_resend_timeout_ms);
    }
    hci_transport_link_run();
    return 0;
}

static int hci_transport_h5_set_baudrate(uint32_t baudrate){

    log_info("set_baudrate %"PRIu32", h5 actions %x", baudrate, hci_transport_link_actions);
    // Baudrate is changed after an HCI Baudrate Change Command, which usually causes an HCI Event Commmand Complete
    // Before changing the baudrate, the HCI Command Complete needs to get acknowledged
    if (hci_transport_link_actions & HCI_TRANSPORT_LINK_SEND_ACK_PACKET){
        hci_transport_link_actions |= HCI_TRANSPORT_LINK_SET_BAUDRATE;
        link_new_baudrate = baudrate;
        hci_transport_link_run();
        return 0;        
    }

    int res = btstack_uart->set_baudrate(baudrate);

    if (res) return res;
    hci_transport_link_update_resend_timeout(baudrate);
    return 0;
}

static void hci_transport_h5_reset_link(void){

    log_info("reset_link");

    // clear outgoing queue
    hci_transport_link_clear_queue();

    // init slip parser state machine
    hci_transport_slip_init();
    
    // init link management - already starts syncing
    hci_transport_link_init();
}

static const hci_transport_t hci_transport_h5 = {
    /* const char * name; */                                        "H5",
    /* void   (*init) (const void *transport_config); */            &hci_transport_h5_init,
    /* int    (*open)(void); */                                     &hci_transport_h5_open,
    /* int    (*close)(void); */                                    &hci_transport_h5_close,
    /* void   (*register_packet_handler)(void (*handler)(...); */   &hci_transport_h5_register_packet_handler,
    /* int    (*can_send_packet_now)(uint8_t packet_type); */       &hci_transport_h5_can_send_packet_now,
    /* int    (*send_packet)(...); */                               &hci_transport_h5_send_packet,
    /* int    (*set_baudrate)(uint32_t baudrate); */                &hci_transport_h5_set_baudrate,
    /* void   (*reset_link)(void); */                               &hci_transport_h5_reset_link,
    /* void   (*set_sco_config)(uint16_t voice_setting, int num_connections); */ NULL, 
};

// configure and return h5 singleton
const hci_transport_t * hci_transport_h5_instance(const btstack_uart_t * uart_driver) {
    btstack_uart = uart_driver;
    return &hci_transport_h5;
}

void hci_transport_h5_set_auto_sleep(uint16_t inactivity_timeout_ms){
    link_inactivity_timeout_ms = inactivity_timeout_ms;
}

void hci_transport_h5_enable_bcsp_mode(void){
    hci_transport_bcsp_mode = 1;
}

#else
typedef int _fix_empty_translation_unit_warnig;
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "btstack_run_loop.h"
#include "btstack_slip.h"
#include "btstack_uart_slip_wrapper.h"
#include "hci.h"
#include "hci_transport_h4.h"
#include "hci_transport_h5.h"

#define CHECK(condition)                                                      \
    do {                                                                      \
        if (!(condition)) {                                                   \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n",                 \
                    __FILE__, __LINE__, #condition);                          \
            return 1;                                                         \
        }                                                                     \
    } while (0)

typedef struct {
    unsigned init_count;
    unsigned open_count;
    unsigned close_count;
    unsigned send_block_count;
    unsigned send_frame_count;
    btstack_uart_config_t config;
    void (*block_received)(void);
    void (*block_sent)(void);
    void (*frame_received)(uint16_t);
    void (*frame_sent)(void);
    uint8_t *receive_block_buffer;
    uint16_t receive_block_len;
    uint8_t *receive_frame_buffer;
    uint16_t receive_frame_len;
    uint8_t sent[1100];
    uint16_t sent_len;
} mock_uart_state_t;

typedef struct {
    unsigned count;
    uint8_t type;
    uint16_t len;
    uint8_t payload[32];
} packet_state_t;

static mock_uart_state_t mock;
static packet_state_t received_packet;
static unsigned wrapper_frame_sent_count;
static unsigned wrapper_frame_received_count;
static uint16_t wrapper_frame_received_len;

static void reset_state(void)
{
    memset(&mock, 0, sizeof(mock));
    memset(&received_packet, 0, sizeof(received_packet));
    wrapper_frame_sent_count = 0;
    wrapper_frame_received_count = 0;
    wrapper_frame_received_len = 0;
}

static int mock_uart_init(const btstack_uart_config_t *config)
{
    mock.init_count++;
    mock.config = *config;
    return 0;
}

static int mock_uart_open(void)
{
    mock.open_count++;
    return 0;
}

static int mock_uart_close(void)
{
    mock.close_count++;
    return 0;
}

static void mock_set_block_received(void (*handler)(void))
{
    mock.block_received = handler;
}

static void mock_set_block_sent(void (*handler)(void))
{
    mock.block_sent = handler;
}

static int mock_set_u32(uint32_t value)
{
    (void) value;
    return 0;
}

static int mock_set_int(int value)
{
    (void) value;
    return 0;
}

static void mock_receive_block(uint8_t *buffer, uint16_t len)
{
    mock.receive_block_buffer = buffer;
    mock.receive_block_len = len;
}

static void mock_send_block(const uint8_t *buffer, uint16_t len)
{
    mock.send_block_count++;
    mock.sent_len = len;
    memcpy(mock.sent, buffer, len);
}

static int mock_sleep_modes(void)
{
    return 0;
}

static void mock_set_sleep(btstack_uart_sleep_mode_t mode)
{
    (void) mode;
}

static void mock_set_wakeup(void (*handler)(void))
{
    (void) handler;
}

static void mock_set_frame_received(void (*handler)(uint16_t))
{
    mock.frame_received = handler;
}

static void mock_set_frame_sent(void (*handler)(void))
{
    mock.frame_sent = handler;
}

static void mock_receive_frame(uint8_t *buffer, uint16_t len)
{
    mock.receive_frame_buffer = buffer;
    mock.receive_frame_len = len;
}

static void mock_send_frame(const uint8_t *buffer, uint16_t len)
{
    mock.send_frame_count++;
    mock.sent_len = len;
    memcpy(mock.sent, buffer, len);
}

static const btstack_uart_t mock_uart = {
    mock_uart_init,
    mock_uart_open,
    mock_uart_close,
    mock_set_block_received,
    mock_set_block_sent,
    mock_set_u32,
    mock_set_int,
    mock_set_int,
    mock_receive_block,
    mock_send_block,
    mock_sleep_modes,
    mock_set_sleep,
    mock_set_wakeup,
    mock_set_frame_received,
    mock_set_frame_sent,
    mock_receive_frame,
    mock_send_frame
};

static void packet_handler(uint8_t packet_type, uint8_t *packet, uint16_t size)
{
    received_packet.count++;
    received_packet.type = packet_type;
    received_packet.len = size;
    if (size > sizeof(received_packet.payload)) {
        size = sizeof(received_packet.payload);
    }
    memcpy(received_packet.payload, packet, size);
}

static int feed_block(const uint8_t *data, uint16_t len)
{
    CHECK(mock.block_received != NULL);
    CHECK(mock.receive_block_buffer != NULL);
    CHECK(mock.receive_block_len == len);
    memcpy(mock.receive_block_buffer, data, len);
    mock.block_received();
    return 0;
}

static int feed_frame(const uint8_t *data, uint16_t len)
{
    CHECK(mock.frame_received != NULL);
    CHECK(mock.receive_frame_buffer != NULL);
    CHECK(mock.receive_frame_len >= len);
    memcpy(mock.receive_frame_buffer, data, len);
    mock.frame_received(len);
    return 0;
}

static void wrapper_frame_sent(void)
{
    wrapper_frame_sent_count++;
}

static void wrapper_frame_received(uint16_t len)
{
    wrapper_frame_received_count++;
    wrapper_frame_received_len = len;
}

void btstack_run_loop_set_timer(btstack_timer_source_t *timer, uint32_t timeout_in_ms)
{
    (void) timer;
    (void) timeout_in_ms;
}

void btstack_run_loop_set_timer_handler(btstack_timer_source_t *timer,
                                        void (*handler)(btstack_timer_source_t *))
{
    (void) timer;
    (void) handler;
}

void btstack_run_loop_add_timer(btstack_timer_source_t *timer)
{
    (void) timer;
}

int btstack_run_loop_remove_timer(btstack_timer_source_t *timer)
{
    (void) timer;
    return 0;
}

uint32_t btstack_run_loop_get_time_ms(void)
{
    return 1;
}

static const hci_transport_config_uart_t transport_config = {
    HCI_TRANSPORT_CONFIG_UART, 115200u, 0u, 1, "mock", BTSTACK_UART_PARITY_OFF
};

static int test_h4(void)
{
    const uint8_t command[] = {0x01, 0x02, 0x03};
    const uint8_t event_type[] = {HCI_EVENT_PACKET};
    const uint8_t event_header[] = {0x0e, 0x01};
    const uint8_t event_payload[] = {0x00};
    const hci_transport_t *transport;

    reset_state();
    transport = hci_transport_h4_instance_for_uart(&mock_uart);
    transport->init(&transport_config);
    CHECK(mock.init_count == 1u);
    CHECK(mock.config.baudrate == 115200u);
    CHECK(mock.block_received != NULL);
    CHECK(mock.block_sent != NULL);
    transport->register_packet_handler(packet_handler);
    CHECK(transport->open() == 0);
    CHECK(mock.open_count == 1u);
    CHECK(mock.receive_block_len == 1u);

    CHECK(transport->send_packet(HCI_COMMAND_DATA_PACKET,
                                 (uint8_t *) command, sizeof(command)) == 0);
    CHECK(mock.send_block_count == 1u);
    CHECK(mock.sent_len == sizeof(command) + 1u);
    CHECK(mock.sent[0] == HCI_COMMAND_DATA_PACKET);
    CHECK(memcmp(&mock.sent[1], command, sizeof(command)) == 0);
    mock.block_sent();
    CHECK(received_packet.count == 1u);
    CHECK(received_packet.type == HCI_EVENT_PACKET);
    CHECK(received_packet.payload[0] == HCI_EVENT_TRANSPORT_PACKET_SENT);

    CHECK(feed_block(event_type, sizeof(event_type)) == 0);
    CHECK(feed_block(event_header, sizeof(event_header)) == 0);
    CHECK(feed_block(event_payload, sizeof(event_payload)) == 0);
    CHECK(received_packet.count == 2u);
    CHECK(received_packet.type == HCI_EVENT_PACKET);
    CHECK(received_packet.len == 3u);
    CHECK(memcmp(received_packet.payload, "\x0e\x01\x00", 3) == 0);
    CHECK(transport->close() == 0);
    CHECK(mock.close_count == 1u);
    return 0;
}

static int test_h5(void)
{
    static const uint8_t sync_response[] = {0x00, 0x2f, 0x00, 0xd0, 0x02, 0x7d};
    static const uint8_t config_response[] = {0x00, 0x2f, 0x00, 0xd0, 0x04, 0x7b};
    uint8_t command_with_prebuffer[7] = {0};
    uint8_t *command = &command_with_prebuffer[4];
    static const uint8_t tx_ack[] = {0x08, 0x00, 0x00, 0xf7};
    static const uint8_t event[] = {0x80, 0x24, 0x00, 0x5b, 0x0e, 0x00};
    const hci_transport_t *transport;

    reset_state();
    command[0] = 0x01;
    command[1] = 0x02;
    command[2] = 0x03;
    transport = hci_transport_h5_instance(&mock_uart);
    transport->init(&transport_config);
    CHECK(mock.init_count == 1u);
    CHECK(mock.frame_received != NULL);
    CHECK(mock.frame_sent != NULL);
    transport->register_packet_handler(packet_handler);
    CHECK(transport->open() == 0);
    CHECK(mock.open_count == 1u);
    CHECK(mock.send_frame_count == 1u);
    CHECK(mock.receive_frame_buffer != NULL);

    mock.frame_sent();
    CHECK(feed_frame(sync_response, sizeof(sync_response)) == 0);
    CHECK(mock.send_frame_count == 2u);
    mock.frame_sent();
    CHECK(feed_frame(config_response, sizeof(config_response)) == 0);
    CHECK(transport->can_send_packet_now(HCI_COMMAND_DATA_PACKET));

    CHECK(transport->send_packet(HCI_COMMAND_DATA_PACKET,
                                 command, 3) == 0);
    CHECK(mock.sent_len == 7u);
    CHECK((mock.sent[0] & 0x80u) != 0u);
    CHECK((mock.sent[1] & 0x0fu) == HCI_COMMAND_DATA_PACKET);
    CHECK(memcmp(&mock.sent[4], command, 3) == 0);
    mock.frame_sent();
    CHECK(feed_frame(tx_ack, sizeof(tx_ack)) == 0);
    CHECK(transport->can_send_packet_now(HCI_COMMAND_DATA_PACKET));

    CHECK(feed_frame(event, sizeof(event)) == 0);
    CHECK(received_packet.type == HCI_EVENT_PACKET);
    CHECK(received_packet.len == 2u);
    CHECK(memcmp(received_packet.payload, "\x0e\x00", 2) == 0);
    CHECK(mock.sent_len == 4u);
    CHECK((mock.sent[0] & 0x38u) == 0x08u);
    mock.frame_sent();
    CHECK(transport->close() == 0);
    CHECK(mock.close_count == 1u);
    return 0;
}

static int test_slip_wrapper(void)
{
    static const uint8_t frame[] = {0x11, 0x22};
    static const uint8_t encoded[] = {BTSTACK_SLIP_SOF, 0x33, 0x44, BTSTACK_SLIP_SOF};
    uint8_t decoded[8] = {0};
    const btstack_uart_config_t config = {
        115200u, 1, "mock", BTSTACK_UART_PARITY_OFF
    };
    const btstack_uart_t *wrapper;
    unsigned i;

    reset_state();
    wrapper = btstack_uart_slip_wrapper_instance(&mock_uart);
    CHECK(wrapper->init(&config) == 0);
    CHECK(wrapper->open() == 0);
    wrapper->set_frame_sent(wrapper_frame_sent);
    wrapper->send_frame(frame, sizeof(frame));
    CHECK(mock.send_block_count == 1u);
    CHECK(mock.sent_len == 4u);
    CHECK(mock.sent[0] == BTSTACK_SLIP_SOF);
    CHECK(mock.sent[3] == BTSTACK_SLIP_SOF);
    mock.block_sent();
    CHECK(wrapper_frame_sent_count == 1u);

    wrapper->set_frame_received(wrapper_frame_received);
    wrapper->receive_frame(decoded, sizeof(decoded));
    for (i = 0; i < sizeof(encoded); i++) {
        CHECK(feed_block(&encoded[i], 1u) == 0);
    }
    CHECK(wrapper_frame_received_count == 1u);
    CHECK(wrapper_frame_received_len == 2u);
    CHECK(memcmp(decoded, "\x33\x44", 2) == 0);
    CHECK(wrapper->close() == 0);
    CHECK(mock.close_count == 1u);
    return 0;
}

int main(void)
{
    if (test_h4() != 0) return 1;
    if (test_h5() != 0) return 1;
    if (test_slip_wrapper() != 0) return 1;
    return 0;
}

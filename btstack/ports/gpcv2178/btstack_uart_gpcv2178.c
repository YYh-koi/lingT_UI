#include "btstack_uart_gpcv2178.h"

#include "btstack_config.h"
#include "btstack_run_loop.h"
#include "btstack_run_loop_gpcv2178.h"
#include "btstack_slip.h"
#include "drv_l1_arm_cp15.h"
#include "drv_l1_dma.h"
#include "drv_l1_uart.h"
#include "gplib_print_string.h"
#include "project.h"

extern INT32U drv_l1_uart_get_ctrl(INT8U dev_idx);
extern void drv_l1_uart_set_ctrl(INT8U dev_idx, INT32U val);

static void btstack_uart_gpcv2178_rx_dma_isr(INT8U channel, INT8U status);
static void btstack_uart_h5_rx_dma_try_preload(void);

#ifndef BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT
#define BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT 4u
#endif

#ifndef BTSTACK_GPCV2178_UART_RX_DMA_IDLE_FLUSH
#define BTSTACK_GPCV2178_UART_RX_DMA_IDLE_FLUSH 1u
#endif

#ifndef BTSTACK_GPCV2178_UART_RX_DMA_IDLE_MS
#define BTSTACK_GPCV2178_UART_RX_DMA_IDLE_MS 2u
#endif

#ifndef BTSTACK_GPCV2178_UART_DIAG_VERBOSE
#define BTSTACK_GPCV2178_UART_DIAG_VERBOSE 0
#endif

#if BTSTACK_GPCV2178_UART_DIAG_VERBOSE
#define BTSTACK_UART_DIAG_PRINTF(...) DBG_PRINT(__VA_ARGS__)
#else
#define BTSTACK_UART_DIAG_PRINTF(...) do { } while (0)
#endif

static void (*btstack_uart_block_received)(void);
static void (*btstack_uart_block_sent)(void);
static void (*btstack_uart_frame_received)(uint16_t frame_size);
static void (*btstack_uart_frame_sent)(void);
static void btstack_uart_h5_rx_process_ready(void);
static void btstack_uart_h5_rx_idle_timer_schedule(void);
static uint8_t *btstack_uart_rx_target;
static uint16_t btstack_uart_rx_len;
static uint16_t btstack_uart_rx_pos;
static uint8_t btstack_uart_rx_processing;

static uint8_t btstack_uart_ring[BTSTACK_GPCV2178_UART_RX_RING_SIZE];
static volatile uint16_t btstack_uart_ring_read;
static volatile uint16_t btstack_uart_ring_write;
static volatile uint32_t btstack_uart_ring_overflow_count;
static uint32_t btstack_uart_ring_overflow_reported;
static btstack_data_source_t btstack_uart_data_source;
static uint8_t btstack_uart_data_source_registered;

static DMA_STRUCT btstack_uart_rx_dma;
static DMA_STRUCT btstack_uart_tx_dma;
static volatile uint8_t btstack_uart_rx_dma_active;
static volatile uint8_t btstack_uart_rx_dma_done;
static volatile INT8U btstack_uart_rx_dma_status;
static volatile uint8_t btstack_uart_tx_dma_active;
static volatile uint8_t btstack_uart_tx_dma_done;
static volatile INT8U btstack_uart_tx_dma_status;
static volatile uint32_t btstack_uart_dma_error_count;
static uint16_t btstack_uart_tx_dma_len;
static uint32_t btstack_uart_rx_request_count;
static uint32_t btstack_uart_rx_done_count;
static uint32_t btstack_uart_tx_request_count;
static uint32_t btstack_uart_tx_done_count;
static uint8_t btstack_uart_rx_dma_enabled = BTSTACK_GPCV2178_UART_RX_USE_DMA ? 1u : 0u;
static uint8_t btstack_uart_tx_dma_enabled = BTSTACK_GPCV2178_UART_TX_USE_DMA ? 1u : 0u;

static ALIGN64 uint8_t btstack_uart_h5_rx_dma_blocks[BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT][BTSTACK_GPCV2178_UART_RX_DMA_CHUNK_SIZE];
static uint8_t btstack_uart_h5_tx_slip_buffer[BTSTACK_GPCV2178_UART_TX_CHUNK_SIZE];
static volatile uint8_t btstack_uart_h5_rx_dma_channel_owned;
static volatile uint8_t btstack_uart_h5_rx_dma_running;
static volatile uint8_t btstack_uart_h5_rx_dma_reload_pending;
static volatile uint8_t btstack_uart_h5_rx_dma_active_block;
static volatile uint8_t btstack_uart_h5_rx_dma_queued_block;
static volatile uint8_t btstack_uart_h5_rx_dma_block_state[BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT];
static volatile uint8_t btstack_uart_h5_rx_ready_queue[BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT];
static volatile uint16_t btstack_uart_h5_rx_ready_len[BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT];
static volatile uint8_t btstack_uart_h5_rx_ready_read;
static volatile uint8_t btstack_uart_h5_rx_ready_write;
static volatile uint8_t btstack_uart_h5_rx_ready_count;
static volatile uint8_t btstack_uart_h5_rx_restart_pending;
static volatile uint32_t btstack_uart_h5_rx_block_count;
static volatile uint32_t btstack_uart_h5_rx_idle_flush_count;
static volatile uint32_t btstack_uart_h5_rx_byte_count;
static volatile uint32_t btstack_uart_h5_rx_drop_count;
static volatile INT32U btstack_uart_h5_rx_dma_last_remaining;
static volatile uint32_t btstack_uart_h5_rx_dma_last_progress_ms;
static btstack_timer_source_t btstack_uart_h5_rx_idle_timer;
static uint8_t btstack_uart_h5_rx_idle_timer_active;
#if BTSTACK_GPCV2178_UART_DIAG_VERBOSE
static btstack_timer_source_t btstack_uart_h5_rx_health_timer;
static uint8_t btstack_uart_h5_rx_health_timer_active;
#endif
static uint8_t btstack_uart_h5_rx_current_block;
static uint16_t btstack_uart_h5_rx_current_offset;
static uint16_t btstack_uart_h5_rx_current_len;
static uint8_t btstack_uart_h5_rx_processing;
static uint8_t btstack_uart_h5_rx_have_frame_buffer;
static uint32_t btstack_uart_h5_rx_frame_count;
static uint32_t btstack_uart_h5_rx_receive_start;
static uint8_t btstack_uart_h5_tx_active;

static void btstack_uart_gpcv2178_set_rx_irq_enabled(uint8_t enable)
{
    INT32U ctrl = drv_l1_uart_get_ctrl(BTSTACK_GPCV2178_HCI_UART_DEV);
    if (enable) {
        ctrl |= C_UART_CTRL_RX_INT | C_UART_CTRL_RTIF_INT;
    } else {
        ctrl &= ~(C_UART_CTRL_RX_INT | C_UART_CTRL_RTIF_INT);
    }
    drv_l1_uart_set_ctrl(BTSTACK_GPCV2178_HCI_UART_DEV, ctrl);
}

static int btstack_uart_h5_rx_dma_find_free_block(void)
{
    uint8_t i;

    for (i = 0; i < BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT; i++) {
        if (btstack_uart_h5_rx_dma_block_state[i] == 0u) {
            return (int)i;
        }
    }
    return -1;
}

static int btstack_uart_h5_rx_dma_queue_block(uint8_t block, uint16_t len)
{
    if ((block >= BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT) || (len == 0u)) {
        if (block < BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT) {
            btstack_uart_h5_rx_dma_block_state[block] = 0u;
        }
        return 0;
    }

    if (btstack_uart_h5_rx_ready_count >= BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT) {
        btstack_uart_h5_rx_drop_count++;
        btstack_uart_h5_rx_dma_block_state[block] = 0u;
        return -1;
    }

    btstack_uart_h5_rx_dma_block_state[block] = 2u;
    btstack_uart_h5_rx_ready_queue[btstack_uart_h5_rx_ready_write] = block;
    btstack_uart_h5_rx_ready_len[btstack_uart_h5_rx_ready_write] = len;
    btstack_uart_h5_rx_ready_write++;
    if (btstack_uart_h5_rx_ready_write >= BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT) {
        btstack_uart_h5_rx_ready_write = 0;
    }
    btstack_uart_h5_rx_ready_count++;
    return 0;
}

static int btstack_uart_h5_rx_dma_pop_block(uint8_t *block, uint16_t *len)
{
    if (btstack_uart_h5_rx_ready_count == 0u) {
        return 0;
    }

    *block = btstack_uart_h5_rx_ready_queue[btstack_uart_h5_rx_ready_read];
    *len = btstack_uart_h5_rx_ready_len[btstack_uart_h5_rx_ready_read];
    btstack_uart_h5_rx_ready_read++;
    if (btstack_uart_h5_rx_ready_read >= BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT) {
        btstack_uart_h5_rx_ready_read = 0;
    }
    btstack_uart_h5_rx_ready_count--;
    return 1;
}

static void btstack_uart_h5_rx_dma_release_queued_block(void)
{
    uint8_t block = btstack_uart_h5_rx_dma_queued_block;

    if (block < BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT) {
        btstack_uart_h5_rx_dma_block_state[block] = 0u;
    }
    btstack_uart_h5_rx_dma_queued_block = 0xffu;
}

static void btstack_uart_h5_rx_dma_release_channel(void)
{
    if (btstack_uart_h5_rx_dma_channel_owned) {
        drv_l1_dma_transfer_double_buf_free(&btstack_uart_rx_dma);
        btstack_uart_h5_rx_dma_channel_owned = 0u;
    }
    btstack_uart_h5_rx_dma_running = 0u;
    btstack_uart_h5_rx_dma_reload_pending = 0u;
}

static int btstack_uart_h5_rx_dma_arm_new(void)
{
    int block;
    int status;

    if (btstack_uart_h5_rx_dma_channel_owned || btstack_uart_h5_rx_dma_running) {
        return 0;
    }

    block = btstack_uart_h5_rx_dma_find_free_block();
    if (block < 0) {
        btstack_uart_h5_rx_drop_count++;
        btstack_uart_h5_rx_restart_pending = 1;
        return -1;
    }

    memset(&btstack_uart_rx_dma, 0, sizeof(btstack_uart_rx_dma));
    btstack_uart_rx_dma.s_addr = (INT32U)P_UART0_BASE;
    btstack_uart_rx_dma.t_addr = (INT32U)&btstack_uart_h5_rx_dma_blocks[block][0];
    btstack_uart_rx_dma.count = BTSTACK_GPCV2178_UART_RX_DMA_CHUNK_SIZE;
    btstack_uart_rx_dma.width = DMA_DATA_WIDTH_1BYTE;
    btstack_uart_rx_dma.timeout = 0;
    btstack_uart_rx_dma.notify = NULL;
    btstack_uart_rx_dma.trigger = NULL;

    btstack_uart_h5_rx_dma_block_state[block] = 1u;
    btstack_uart_h5_rx_dma_active_block = (uint8_t)block;
    status = drv_l1_dma_transfer_double_buf_with_callback(&btstack_uart_rx_dma, NULL, btstack_uart_gpcv2178_rx_dma_isr);
    if (status != 0) {
        btstack_uart_h5_rx_dma_block_state[block] = 0u;
        btstack_uart_h5_rx_dma_active_block = 0xffu;
        btstack_uart_dma_error_count++;
        btstack_uart_h5_rx_restart_pending = 1;
        DBG_PRINT("btstack uart h5 rx dma arm fail %d\r\n", status);
        return status;
    }

    btstack_uart_h5_rx_dma_channel_owned = 1u;
    btstack_uart_h5_rx_dma_running = 1u;
    btstack_uart_h5_rx_dma_reload_pending = 1u;
    btstack_uart_h5_rx_restart_pending = 0u;
    btstack_uart_h5_rx_dma_last_remaining = BTSTACK_GPCV2178_UART_RX_DMA_CHUNK_SIZE;
    btstack_uart_h5_rx_dma_last_progress_ms = btstack_run_loop_get_time_ms();
    btstack_uart_h5_rx_dma_try_preload();
    return 0;
}

static void btstack_uart_h5_rx_dma_try_preload(void)
{
    int block;
    int status;

    if (!btstack_uart_h5_rx_dma_reload_pending ||
        !btstack_uart_h5_rx_dma_channel_owned ||
        !btstack_uart_h5_rx_dma_running ||
        (btstack_uart_h5_rx_dma_queued_block < BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT)) {
        return;
    }
    block = btstack_uart_h5_rx_dma_find_free_block();
    if (block < 0) {
        return;
    }

    btstack_uart_rx_dma.s_addr = (INT32U)P_UART0_BASE;
    btstack_uart_rx_dma.t_addr = (INT32U)&btstack_uart_h5_rx_dma_blocks[block][0];
    btstack_uart_rx_dma.count = BTSTACK_GPCV2178_UART_RX_DMA_CHUNK_SIZE;
    btstack_uart_rx_dma.width = DMA_DATA_WIDTH_1BYTE;
    btstack_uart_h5_rx_dma_block_state[block] = 1u;

    status = drv_l1_dma_transfer_double_buf_set(&btstack_uart_rx_dma);
    if (status != 0) {
        btstack_uart_h5_rx_dma_block_state[block] = 0u;
        btstack_uart_dma_error_count++;
        DBG_PRINT("btstack uart h5 rx dma preload fail ch=%u status=%d\r\n",
            (unsigned int)btstack_uart_rx_dma.channel,
            status);
        btstack_uart_h5_rx_dma_reload_pending = 0u;
        btstack_uart_h5_rx_restart_pending = 1u;
        return;
    }
    btstack_uart_h5_rx_dma_queued_block = (uint8_t)block;
    btstack_uart_h5_rx_dma_reload_pending = 0u;
}

static uint16_t btstack_uart_ring_next(uint16_t index)
{
    index++;
    if (index >= BTSTACK_GPCV2178_UART_RX_RING_SIZE) {
        index = 0;
    }
    return index;
}

static void btstack_uart_ring_put(uint8_t data)
{
    uint16_t next = btstack_uart_ring_next(btstack_uart_ring_write);
    if (next != btstack_uart_ring_read) {
        btstack_uart_ring[btstack_uart_ring_write] = data;
        btstack_uart_ring_write = next;
    } else {
        btstack_uart_ring_overflow_count++;
    }
}

static int btstack_uart_ring_get(uint8_t *data)
{
    if (btstack_uart_ring_read == btstack_uart_ring_write) {
        return 0;
    }
    *data = btstack_uart_ring[btstack_uart_ring_read];
    btstack_uart_ring_read = btstack_uart_ring_next(btstack_uart_ring_read);
    return 1;
}

static void btstack_uart_gpcv2178_rx_isr(INT8U dev_idx, INT32U status, INT8U data)
{
    if (dev_idx != BTSTACK_GPCV2178_HCI_UART_DEV) {
        return;
    }
    if (status & C_UART_STATUS_RXTO_INT) {
        btstack_run_loop_gpcv2178_wake_from_irq();
        return;
    }

    btstack_uart_ring_put(data);
    btstack_run_loop_gpcv2178_wake_from_irq();
}

static void btstack_uart_gpcv2178_rx_dma_isr(INT8U channel, INT8U status)
{
    uint8_t block;

    (void)channel;

    if (btstack_uart_h5_rx_dma_running) {
        block = btstack_uart_h5_rx_dma_active_block;
        if ((status == (INT8U)C_DMA_STATUS_DONE) && (block < BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT)) {
            btstack_uart_h5_rx_block_count++;
            btstack_uart_h5_rx_byte_count += BTSTACK_GPCV2178_UART_RX_DMA_CHUNK_SIZE;
            btstack_uart_h5_rx_dma_queue_block(block, BTSTACK_GPCV2178_UART_RX_DMA_CHUNK_SIZE);
            if (btstack_uart_h5_rx_dma_queued_block < BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT) {
                btstack_uart_h5_rx_dma_active_block = btstack_uart_h5_rx_dma_queued_block;
                btstack_uart_h5_rx_dma_queued_block = 0xffu;
                btstack_uart_h5_rx_dma_reload_pending = 1u;
                btstack_uart_h5_rx_dma_last_remaining = BTSTACK_GPCV2178_UART_RX_DMA_CHUNK_SIZE;
                btstack_uart_h5_rx_dma_last_progress_ms = btstack_run_loop_get_time_ms();
            } else {
                btstack_uart_h5_rx_dma_active_block = 0xffu;
                btstack_uart_h5_rx_dma_running = 0u;
                btstack_uart_h5_rx_dma_reload_pending = 0u;
                btstack_uart_h5_rx_restart_pending = 1u;
            }
        } else {
            btstack_uart_dma_error_count++;
            if (block < BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT) {
                btstack_uart_h5_rx_dma_block_state[block] = 0u;
            }
            btstack_uart_h5_rx_dma_release_queued_block();
            btstack_uart_h5_rx_dma_active_block = 0xffu;
            btstack_uart_h5_rx_dma_running = 0u;
            btstack_uart_h5_rx_dma_reload_pending = 0u;
            btstack_uart_h5_rx_restart_pending = 1u;
        }
        btstack_run_loop_gpcv2178_wake_from_irq();
        return;
    }

    btstack_uart_rx_dma_status = status;
    btstack_uart_rx_dma_active = 0;
    btstack_uart_rx_dma_done = 1;
    btstack_run_loop_gpcv2178_wake_from_irq();
}

static void btstack_uart_gpcv2178_tx_dma_isr(INT8U channel, INT8U status)
{
    (void)channel;
    btstack_uart_tx_dma_status = status;
    btstack_uart_tx_dma_active = 0;
    btstack_uart_tx_dma_done = 1;
    btstack_run_loop_gpcv2178_wake_from_irq();
}

static int btstack_uart_gpcv2178_start_rx_dma(uint8_t *buffer, uint16_t len)
{
    int status;

    if ((buffer == NULL) || (len == 0u)) {
        return -1;
    }
    if (btstack_uart_rx_dma_active) {
        return -1;
    }

    memset(&btstack_uart_rx_dma, 0, sizeof(btstack_uart_rx_dma));
    btstack_uart_rx_dma.s_addr = (INT32U)P_UART0_BASE;
    btstack_uart_rx_dma.t_addr = (INT32U)buffer;
    btstack_uart_rx_dma.count = len;
    btstack_uart_rx_dma.width = DMA_DATA_WIDTH_1BYTE;
    btstack_uart_rx_dma.timeout = 0;
    btstack_uart_rx_dma.notify = NULL;
    btstack_uart_rx_dma.trigger = NULL;

    btstack_uart_rx_dma_done = 0;
    btstack_uart_rx_dma_status = (INT8U)C_DMA_STATUS_WAITING;
    btstack_uart_rx_dma_active = 1;
    status = drv_l1_dma_transfer_with_callback(&btstack_uart_rx_dma, btstack_uart_gpcv2178_rx_dma_isr);
    if (status != 0) {
        btstack_uart_rx_dma_active = 0;
        btstack_uart_rx_dma_done = 0;
        btstack_uart_dma_error_count++;
        return status;
    }

    return 0;
}

static int btstack_uart_gpcv2178_start_tx_dma(const uint8_t *buffer, uint16_t len)
{
    int status;

    if ((buffer == NULL) || (len == 0u)) {
        return -1;
    }
    if (btstack_uart_tx_dma_active) {
        return -1;
    }

    memset(&btstack_uart_tx_dma, 0, sizeof(btstack_uart_tx_dma));
    btstack_uart_tx_dma.s_addr = (INT32U)buffer;
    btstack_uart_tx_dma.t_addr = (INT32U)P_UART0_BASE;
    btstack_uart_tx_dma.count = len;
    btstack_uart_tx_dma.width = DMA_DATA_WIDTH_1BYTE;
    btstack_uart_tx_dma.timeout = 0;
    btstack_uart_tx_dma.notify = NULL;
    btstack_uart_tx_dma.trigger = NULL;

    btstack_uart_tx_dma_done = 0;
    btstack_uart_tx_dma_status = (INT8U)C_DMA_STATUS_WAITING;
    btstack_uart_tx_dma_len = len;
    btstack_uart_tx_dma_active = 1;
    status = drv_l1_dma_transfer_with_callback(&btstack_uart_tx_dma, btstack_uart_gpcv2178_tx_dma_isr);
    if (status != 0) {
        btstack_uart_tx_dma_active = 0;
        btstack_uart_tx_dma_done = 0;
        btstack_uart_tx_dma_len = 0;
        btstack_uart_dma_error_count++;
        return status;
    }

    return 0;
}

static void btstack_uart_gpcv2178_complete_rx_dma(void)
{
    uint8_t *rx_target;
    uint16_t rx_len;

    if (!btstack_uart_rx_dma_done) {
        return;
    }

    btstack_uart_rx_dma_done = 0;

    rx_target = btstack_uart_rx_target;
    rx_len = btstack_uart_rx_len;
    btstack_uart_rx_target = NULL;
    btstack_uart_rx_len = 0;
    btstack_uart_rx_pos = 0;

    if (btstack_uart_rx_dma_status != (INT8U)C_DMA_STATUS_DONE) {
        btstack_uart_dma_error_count++;
        DBG_PRINT("btstack uart rx dma status %d\r\n", (int)btstack_uart_rx_dma_status);
        return;
    }

    if ((rx_target != NULL) && (rx_len != 0u)) {
        drv_l1_cp15_invalidate_dcache_range((INT32U)rx_target, rx_len);
    }

    btstack_uart_rx_done_count++;
    if ((btstack_uart_rx_done_count <= 16u) ||
        ((btstack_uart_tx_request_count >= 13u) && (btstack_uart_rx_done_count <= 128u)) ||
        ((btstack_uart_rx_done_count & 0x3fu) == 0u)) {
        BTSTACK_UART_DIAG_PRINTF("btstack uart rx dma done #%u len=%u b0=%02x\r\n",
            (unsigned int)btstack_uart_rx_done_count,
            (unsigned int)rx_len,
            (unsigned int)((rx_target != NULL && rx_len != 0u) ? rx_target[0] : 0u));
    }

    if (btstack_uart_block_received != NULL) {
        btstack_uart_block_received();
    }
}

static void btstack_uart_gpcv2178_complete_tx_dma(void)
{
    if (!btstack_uart_tx_dma_done) {
        return;
    }

    btstack_uart_tx_dma_done = 0;

    if (btstack_uart_tx_dma_status != (INT8U)C_DMA_STATUS_DONE) {
        btstack_uart_dma_error_count++;
        DBG_PRINT("btstack uart tx dma status %d\r\n", (int)btstack_uart_tx_dma_status);
        return;
    }

    btstack_uart_tx_done_count++;
    if ((btstack_uart_tx_done_count <= 16u) || ((btstack_uart_tx_done_count & 0x3fu) == 0u)) {
        BTSTACK_UART_DIAG_PRINTF("btstack uart tx dma done #%u len=%u\r\n",
            (unsigned int)btstack_uart_tx_done_count,
            (unsigned int)btstack_uart_tx_dma_len);
    }

    if (btstack_uart_block_sent != NULL) {
        btstack_uart_block_sent();
    }
}

static void btstack_uart_h5_rx_release_current_block(void)
{
    if (btstack_uart_h5_rx_current_block < BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT) {
        btstack_uart_h5_rx_dma_block_state[btstack_uart_h5_rx_current_block] = 0u;
    }
    btstack_uart_h5_rx_current_block = 0xffu;
    btstack_uart_h5_rx_current_offset = 0;
    btstack_uart_h5_rx_current_len = 0;
}

static void btstack_uart_h5_rx_idle_timer_handler(btstack_timer_source_t *timer)
{
    (void)timer;

    btstack_uart_h5_rx_idle_timer_active = 0;
    if (!btstack_uart_rx_dma_enabled) {
        return;
    }

    btstack_uart_h5_rx_process_ready();
    btstack_uart_h5_rx_idle_timer_schedule();
}

static void btstack_uart_h5_rx_idle_timer_schedule(void)
{
    if (!btstack_uart_rx_dma_enabled) {
        return;
    }
    if (!btstack_uart_h5_rx_dma_running) {
        return;
    }
    if (!btstack_uart_h5_rx_have_frame_buffer) {
        return;
    }
    if (btstack_uart_h5_rx_idle_timer_active) {
        return;
    }

    btstack_run_loop_set_timer(&btstack_uart_h5_rx_idle_timer, BTSTACK_GPCV2178_UART_RX_DMA_IDLE_MS);
    btstack_run_loop_add_timer(&btstack_uart_h5_rx_idle_timer);
    btstack_uart_h5_rx_idle_timer_active = 1;
}

#if BTSTACK_GPCV2178_UART_DIAG_VERBOSE
static void btstack_uart_h5_rx_health_timer_schedule(void)
{
    if (!btstack_uart_rx_dma_enabled ||
        btstack_uart_h5_rx_health_timer_active) {
        return;
    }

    btstack_run_loop_set_timer(
        &btstack_uart_h5_rx_health_timer,
        BTSTACK_GPCV2178_UART_HEALTH_PERIOD_MS);
    btstack_run_loop_add_timer(&btstack_uart_h5_rx_health_timer);
    btstack_uart_h5_rx_health_timer_active = 1u;
}

static void btstack_uart_h5_rx_health_timer_handler(
    btstack_timer_source_t *timer)
{
    INT32U remaining = 0xffffffffu;
    INT32S dbf_status = -1;
    INT32U uart_status;
    INT32U rx_error;

    (void)timer;
    btstack_uart_h5_rx_health_timer_active = 0u;
    if (btstack_uart_h5_rx_dma_running &&
        (dma_get_tx_count(
            btstack_uart_rx_dma.channel, &remaining) != 0)) {
        remaining = 0xffffffffu;
    }
    if (btstack_uart_h5_rx_dma_channel_owned) {
        dbf_status = drv_l1_dma_dbf_status_get(btstack_uart_rx_dma.channel);
    }
    uart_status = R_UART0_STATUS;
    rx_error = R_UART0_RX_STATUS;
    BTSTACK_UART_DIAG_PRINTF(
        "btstack uart rx health bytes=%u frames=%u dma_rem=%u dma_ch=%u owned=%u running=%u block=%u queued=%u dbf=%d preload=%u ready=%u cur=%u/%u full_irq=%u idle_flush=%u drop=%u error=%u uart=0x%08x rxerr=0x%08x\r\n",
        (unsigned int)btstack_uart_h5_rx_byte_count,
        (unsigned int)btstack_uart_h5_rx_frame_count,
        (unsigned int)remaining,
        (unsigned int)btstack_uart_rx_dma.channel,
        (unsigned int)btstack_uart_h5_rx_dma_channel_owned,
        (unsigned int)btstack_uart_h5_rx_dma_running,
        (unsigned int)btstack_uart_h5_rx_dma_active_block,
        (unsigned int)btstack_uart_h5_rx_dma_queued_block,
        (int)dbf_status,
        (unsigned int)btstack_uart_h5_rx_dma_reload_pending,
        (unsigned int)btstack_uart_h5_rx_ready_count,
        (unsigned int)btstack_uart_h5_rx_current_offset,
        (unsigned int)btstack_uart_h5_rx_current_len,
        (unsigned int)btstack_uart_h5_rx_block_count,
        (unsigned int)btstack_uart_h5_rx_idle_flush_count,
        (unsigned int)btstack_uart_h5_rx_drop_count,
        (unsigned int)btstack_uart_dma_error_count,
        (unsigned int)uart_status,
        (unsigned int)rx_error);
    btstack_uart_h5_rx_health_timer_schedule();
}
#endif

static void btstack_uart_h5_rx_flush_idle_dma(void)
{
    INT32U remaining = 0;
    uint32_t now;
    uint16_t received;
    uint8_t block;

    if (!BTSTACK_GPCV2178_UART_RX_DMA_IDLE_FLUSH) {
        return;
    }
    if (!btstack_uart_h5_rx_dma_running) {
        return;
    }
    if (dma_get_tx_count(btstack_uart_rx_dma.channel, &remaining) != 0) {
        return;
    }

    now = btstack_run_loop_get_time_ms();
    if (remaining != btstack_uart_h5_rx_dma_last_remaining) {
        btstack_uart_h5_rx_dma_last_remaining = remaining;
        btstack_uart_h5_rx_dma_last_progress_ms = now;
        return;
    }

    if (remaining >= BTSTACK_GPCV2178_UART_RX_DMA_CHUNK_SIZE) {
        btstack_uart_h5_rx_dma_last_progress_ms = now;
        return;
    }
    if ((now - btstack_uart_h5_rx_dma_last_progress_ms) < BTSTACK_GPCV2178_UART_RX_DMA_IDLE_MS) {
        return;
    }

    received = (uint16_t)(BTSTACK_GPCV2178_UART_RX_DMA_CHUNK_SIZE - remaining);
    if (received == 0u) {
        return;
    }

    block = btstack_uart_h5_rx_dma_active_block;
    if (block >= BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT) {
        return;
    }

    btstack_uart_h5_rx_dma_release_channel();
    btstack_uart_h5_rx_dma_active_block = 0xffu;
    btstack_uart_h5_rx_dma_release_queued_block();
    btstack_uart_h5_rx_restart_pending = 0u;
    btstack_uart_h5_rx_byte_count += received;
    btstack_uart_h5_rx_idle_flush_count++;
    btstack_uart_h5_rx_dma_queue_block(block, received);
    btstack_uart_h5_rx_dma_arm_new();

    if ((btstack_uart_h5_rx_idle_flush_count <= 16u) ||
        ((btstack_uart_h5_rx_idle_flush_count & 0x3fu) == 0u)) {
        BTSTACK_UART_DIAG_PRINTF("btstack uart h5 rx idle flush #%u len=%u\r\n",
            (unsigned int)btstack_uart_h5_rx_idle_flush_count,
            (unsigned int)received);
    }
}

static void btstack_uart_h5_rx_process_ready(void)
{
    uint8_t block;
    uint16_t len;
    uint8_t data;
    uint16_t frame_size;

    if (!btstack_uart_rx_dma_enabled) {
        return;
    }
    if (btstack_uart_h5_rx_processing) {
        return;
    }

    btstack_uart_h5_rx_processing = 1;

    btstack_uart_h5_rx_flush_idle_dma();
    btstack_uart_h5_rx_dma_try_preload();
    if (btstack_uart_h5_rx_restart_pending && !btstack_uart_h5_rx_dma_running) {
        btstack_uart_h5_rx_restart_pending = 0u;
        btstack_uart_h5_rx_dma_release_channel();
        btstack_uart_h5_rx_dma_release_queued_block();
        btstack_uart_h5_rx_dma_arm_new();
    }

    while (btstack_uart_h5_rx_have_frame_buffer) {
        if (btstack_uart_h5_rx_current_block >= BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT) {
            if (!btstack_uart_h5_rx_dma_pop_block(&block, &len)) {
                break;
            }
            btstack_uart_h5_rx_current_block = block;
            btstack_uart_h5_rx_current_offset = 0;
            btstack_uart_h5_rx_current_len = len;
            drv_l1_cp15_invalidate_dcache_range((INT32U)&btstack_uart_h5_rx_dma_blocks[block][0], len);
        }

        while (btstack_uart_h5_rx_current_offset < btstack_uart_h5_rx_current_len) {
            data = btstack_uart_h5_rx_dma_blocks[btstack_uart_h5_rx_current_block][btstack_uart_h5_rx_current_offset++];
            if ((btstack_uart_h5_rx_receive_start == 0u) && (data != BTSTACK_SLIP_SOF)) {
                btstack_uart_h5_rx_receive_start = btstack_run_loop_get_time_ms();
            }

            btstack_slip_decoder_process(data);
            frame_size = btstack_slip_decoder_frame_size();
            if (frame_size != 0u) {
                btstack_uart_h5_rx_frame_count++;
                btstack_uart_h5_rx_receive_start = 0;
                if (btstack_uart_h5_rx_current_offset == btstack_uart_h5_rx_current_len) {
                    btstack_uart_h5_rx_release_current_block();
                    btstack_uart_h5_rx_dma_try_preload();
                }
                if ((btstack_uart_h5_rx_frame_count <= 16u) ||
                    ((btstack_uart_h5_rx_frame_count & 0x3fu) == 0u)) {
                    BTSTACK_UART_DIAG_PRINTF("btstack uart h5 rx frame #%u len=%u dma_ch=%u running=%u ready=%u drop=%u error=%u\r\n",
                        (unsigned int)btstack_uart_h5_rx_frame_count,
                        (unsigned int)frame_size,
                        (unsigned int)btstack_uart_rx_dma.channel,
                        (unsigned int)btstack_uart_h5_rx_dma_running,
                        (unsigned int)btstack_uart_h5_rx_ready_count,
                        (unsigned int)btstack_uart_h5_rx_drop_count,
                        (unsigned int)btstack_uart_dma_error_count);
                }
                btstack_uart_h5_rx_have_frame_buffer = 0;
                if (btstack_uart_frame_received != NULL) {
                    btstack_uart_frame_received(frame_size);
                }
                btstack_uart_h5_rx_processing = 0;
                btstack_uart_h5_rx_idle_timer_schedule();
                return;
            }
        }

        if (btstack_uart_h5_rx_current_block < BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT) {
            btstack_uart_h5_rx_release_current_block();
            btstack_uart_h5_rx_dma_try_preload();
        }
    }

    btstack_uart_h5_rx_dma_try_preload();

    if ((btstack_uart_h5_rx_drop_count != 0u) && ((btstack_uart_h5_rx_drop_count & 0x0fu) == 1u)) {
        DBG_PRINT("btstack uart h5 rx drop %u\r\n", (unsigned int)btstack_uart_h5_rx_drop_count);
    }

    btstack_uart_h5_rx_processing = 0;
    btstack_uart_h5_rx_idle_timer_schedule();
}

static void btstack_uart_gpcv2178_try_complete_receive(void)
{
    uint8_t data;

    if (btstack_uart_rx_dma_enabled) {
        if (btstack_uart_rx_dma_active || btstack_uart_rx_dma_done) {
            btstack_uart_gpcv2178_complete_rx_dma();
        } else {
            btstack_uart_h5_rx_process_ready();
        }
        return;
    }

    if (btstack_uart_rx_processing) {
        return;
    }

    btstack_uart_rx_processing = 1;

    while (btstack_uart_rx_target != NULL) {
        while ((btstack_uart_rx_pos < btstack_uart_rx_len) && btstack_uart_ring_get(&data)) {
            btstack_uart_rx_target[btstack_uart_rx_pos++] = data;
        }
        if (btstack_uart_rx_pos != btstack_uart_rx_len) {
            break;
        }
        btstack_uart_rx_target = NULL;
        btstack_uart_rx_len = 0;
        btstack_uart_rx_pos = 0;
        if (btstack_uart_block_received != NULL) {
            btstack_uart_block_received();
        }
    }

    btstack_uart_rx_processing = 0;
}

static void btstack_uart_gpcv2178_process(btstack_data_source_t *data_source, btstack_data_source_callback_type_t callback_type)
{
    (void)data_source;
    (void)callback_type;

    if (btstack_uart_ring_overflow_reported != btstack_uart_ring_overflow_count) {
        btstack_uart_ring_overflow_reported = btstack_uart_ring_overflow_count;
        DBG_PRINT("btstack uart rx ring overflow %u\r\n", (unsigned int)btstack_uart_ring_overflow_reported);
    }

    btstack_uart_gpcv2178_complete_tx_dma();
    btstack_uart_gpcv2178_try_complete_receive();
}

static int btstack_uart_gpcv2178_init(const btstack_uart_config_t *uart_config)
{
    uint32_t baudrate = BTSTACK_GPCV2178_HCI_UART_BAUD_INIT;
    if ((uart_config != NULL) && (uart_config->baudrate != 0u)) {
        baudrate = uart_config->baudrate;
    }
    DBG_PRINT("btstack uart0 init baud=%u\r\n", (unsigned int)baudrate);
    drv_l1_uart0_init();
    drv_l1_uart0_buad_rate_set(baudrate);
    btstack_uart_gpcv2178_set_rx_irq_enabled(!btstack_uart_rx_dma_enabled);
    drv_l1_uart0_tx_enable();
    return 0;
}

static int btstack_uart_gpcv2178_open(void)
{
    DBG_PRINT("btstack uart0 open rx_dma=%u tx_dma=%u native_h5_dma=%u\r\n",
        (unsigned int)btstack_uart_rx_dma_enabled,
        (unsigned int)btstack_uart_tx_dma_enabled,
        (unsigned int)(btstack_uart_rx_dma_enabled ? 1u : 0u));
    btstack_uart_ring_read = 0;
    btstack_uart_ring_write = 0;
    btstack_uart_rx_target = NULL;
    btstack_uart_rx_len = 0;
    btstack_uart_rx_pos = 0;
    btstack_uart_rx_processing = 0;
    btstack_uart_ring_overflow_count = 0;
    btstack_uart_ring_overflow_reported = 0;
    btstack_uart_rx_dma_active = 0;
    btstack_uart_rx_dma_done = 0;
    btstack_uart_tx_dma_active = 0;
    btstack_uart_tx_dma_done = 0;
    btstack_uart_dma_error_count = 0;
    btstack_uart_tx_dma_len = 0;
    btstack_uart_rx_request_count = 0;
    btstack_uart_rx_done_count = 0;
    btstack_uart_tx_request_count = 0;
    btstack_uart_tx_done_count = 0;
    btstack_uart_h5_rx_dma_channel_owned = 0u;
    btstack_uart_h5_rx_dma_running = 0u;
    btstack_uart_h5_rx_dma_reload_pending = 0u;
    btstack_uart_h5_rx_dma_active_block = 0xffu;
    btstack_uart_h5_rx_dma_queued_block = 0xffu;
    memset((void *)btstack_uart_h5_rx_dma_block_state, 0, sizeof(btstack_uart_h5_rx_dma_block_state));
    btstack_uart_h5_rx_ready_read = 0;
    btstack_uart_h5_rx_ready_write = 0;
    btstack_uart_h5_rx_ready_count = 0;
    btstack_uart_h5_rx_restart_pending = 0;
    btstack_uart_h5_rx_block_count = 0;
    btstack_uart_h5_rx_idle_flush_count = 0;
    btstack_uart_h5_rx_byte_count = 0;
    btstack_uart_h5_rx_drop_count = 0;
    btstack_uart_h5_rx_dma_last_remaining = BTSTACK_GPCV2178_UART_RX_DMA_CHUNK_SIZE;
    btstack_uart_h5_rx_dma_last_progress_ms = 0;
    btstack_uart_h5_rx_idle_timer_active = 0;
    btstack_uart_h5_rx_current_block = 0xffu;
    btstack_uart_h5_rx_current_offset = 0;
    btstack_uart_h5_rx_current_len = 0;
    btstack_uart_h5_rx_processing = 0;
    btstack_uart_h5_rx_have_frame_buffer = 0;
    btstack_uart_h5_rx_frame_count = 0;
    btstack_uart_h5_rx_receive_start = 0;
    btstack_uart_h5_tx_active = 0;
    btstack_run_loop_set_timer_handler(&btstack_uart_h5_rx_idle_timer, btstack_uart_h5_rx_idle_timer_handler);
#if BTSTACK_GPCV2178_UART_DIAG_VERBOSE
    btstack_uart_h5_rx_health_timer_active = 0u;
    btstack_run_loop_set_timer_handler(
        &btstack_uart_h5_rx_health_timer,
        btstack_uart_h5_rx_health_timer_handler);
#endif

    btstack_run_loop_set_data_source_handler(&btstack_uart_data_source, &btstack_uart_gpcv2178_process);
    btstack_run_loop_enable_data_source_callbacks(&btstack_uart_data_source, DATA_SOURCE_CALLBACK_POLL);
    if (!btstack_uart_data_source_registered) {
        btstack_run_loop_add_data_source(&btstack_uart_data_source);
        btstack_uart_data_source_registered = 1;
    }

    if (btstack_uart_rx_dma_enabled) {
        drv_l1_uart0_rx_isr_set(NULL);
        btstack_uart_gpcv2178_set_rx_irq_enabled(0);
    } else {
        drv_l1_uart0_rx_isr_set(btstack_uart_gpcv2178_rx_isr);
        btstack_uart_gpcv2178_set_rx_irq_enabled(1);
    }
    drv_l1_uart0_tx_enable();
#if BTSTACK_GPCV2178_UART_DIAG_VERBOSE
    btstack_uart_h5_rx_health_timer_schedule();
#endif
    return 0;
}

static int btstack_uart_gpcv2178_close(void)
{
    btstack_uart_h5_rx_dma_release_channel();
    if (btstack_uart_h5_rx_dma_active_block < BTSTACK_GPCV2178_UART_RX_DMA_BLOCK_COUNT) {
        btstack_uart_h5_rx_dma_block_state[btstack_uart_h5_rx_dma_active_block] = 0u;
    }
    btstack_uart_h5_rx_dma_active_block = 0xffu;
    btstack_uart_h5_rx_dma_release_queued_block();
    btstack_uart_h5_rx_restart_pending = 0u;
    if (btstack_uart_h5_rx_idle_timer_active) {
        btstack_run_loop_remove_timer(&btstack_uart_h5_rx_idle_timer);
        btstack_uart_h5_rx_idle_timer_active = 0;
    }
#if BTSTACK_GPCV2178_UART_DIAG_VERBOSE
    if (btstack_uart_h5_rx_health_timer_active) {
        btstack_run_loop_remove_timer(&btstack_uart_h5_rx_health_timer);
        btstack_uart_h5_rx_health_timer_active = 0u;
    }
#endif
    if (btstack_uart_rx_dma_active) {
        user_dma_reset_channel(btstack_uart_rx_dma.channel);
        btstack_uart_rx_dma_active = 0;
        btstack_uart_rx_dma_done = 0;
    }
    if (btstack_uart_tx_dma_active) {
        user_dma_reset_channel(btstack_uart_tx_dma.channel);
        btstack_uart_tx_dma_active = 0;
        btstack_uart_tx_dma_done = 0;
    }
    drv_l1_uart0_rx_isr_set(NULL);
    btstack_uart_gpcv2178_set_rx_irq_enabled(0);
    drv_l1_uart0_tx_disable();
    if (btstack_uart_data_source_registered) {
        btstack_run_loop_remove_data_source(&btstack_uart_data_source);
        btstack_uart_data_source_registered = 0;
    }
    return 0;
}

static void btstack_uart_gpcv2178_set_block_received(void (*handler)(void))
{
    btstack_uart_block_received = handler;
}

static void btstack_uart_gpcv2178_set_block_sent(void (*handler)(void))
{
    btstack_uart_block_sent = handler;
}

static int btstack_uart_gpcv2178_set_baudrate(uint32_t baudrate)
{
    drv_l1_uart0_buad_rate_set(baudrate);
    return 0;
}

static int btstack_uart_gpcv2178_set_parity(int parity)
{
    if (parity == BTSTACK_UART_PARITY_EVEN) {
        return drv_l1_uart0_parity_chk_set(ENABLE, PARITY_EVEN);
    }
    if (parity == BTSTACK_UART_PARITY_ODD) {
        return drv_l1_uart0_parity_chk_set(ENABLE, PARITY_ODD);
    }
    return drv_l1_uart0_parity_chk_set(DISABLE, PARITY_DISABLE);
}

static int btstack_uart_gpcv2178_set_flowcontrol(int flowcontrol)
{
    return flowcontrol ? -1 : 0;
}

static void btstack_uart_gpcv2178_receive_block(uint8_t *buffer, uint16_t len)
{
    btstack_uart_rx_request_count++;
    if ((btstack_uart_rx_request_count <= 4u) || ((btstack_uart_rx_request_count & 0xffu) == 0u)) {
        BTSTACK_UART_DIAG_PRINTF("btstack uart rx req #%u len=%u dma=%u\r\n",
            (unsigned int)btstack_uart_rx_request_count,
            (unsigned int)len,
            (unsigned int)btstack_uart_rx_dma_enabled);
    }

    btstack_uart_rx_target = buffer;
    btstack_uart_rx_len = len;
    btstack_uart_rx_pos = 0;

    if (btstack_uart_rx_dma_enabled) {
        if (btstack_uart_gpcv2178_start_rx_dma(buffer, len) == 0) {
            return;
        }
        btstack_uart_rx_target = NULL;
        btstack_uart_rx_len = 0;
        btstack_uart_rx_pos = 0;
        DBG_PRINT("btstack uart rx dma start fail\r\n");
        return;
    }

    btstack_uart_gpcv2178_try_complete_receive();
}

static void btstack_uart_gpcv2178_send_block(const uint8_t *buffer, uint16_t len)
{
    uint16_t i;

    btstack_uart_tx_request_count++;
    if ((btstack_uart_tx_request_count <= 16u) || ((btstack_uart_tx_request_count & 0x3fu) == 0u)) {
        BTSTACK_UART_DIAG_PRINTF("btstack uart tx req #%u len=%u dma=%u b=%02x %02x %02x\r\n",
            (unsigned int)btstack_uart_tx_request_count,
            (unsigned int)len,
            (unsigned int)btstack_uart_tx_dma_enabled,
            (unsigned int)((buffer != NULL && len > 0u) ? buffer[0] : 0u),
            (unsigned int)((buffer != NULL && len > 1u) ? buffer[1] : 0u),
            (unsigned int)((buffer != NULL && len > 2u) ? buffer[2] : 0u));
    }
    if ((buffer != NULL) && (btstack_uart_tx_request_count <= 16u) && (len <= 16u)) {
        BTSTACK_UART_DIAG_PRINTF("btstack uart tx data #%u:", (unsigned int)btstack_uart_tx_request_count);
        for (i = 0; i < len; i++) {
            BTSTACK_UART_DIAG_PRINTF(" %02x", (unsigned int)buffer[i]);
        }
        BTSTACK_UART_DIAG_PRINTF("\r\n");
    }

    if ((btstack_uart_tx_dma_enabled) && (btstack_uart_gpcv2178_start_tx_dma(buffer, len) == 0)) {
        return;
    }

    for (i = 0; i < len; i++) {
        drv_l1_uart0_data_send(buffer[i], 1);
    }
    if (btstack_uart_block_sent != NULL) {
        btstack_uart_block_sent();
    }
}

static void btstack_uart_gpcv2178_h5_send_next_chunk(void);

static void btstack_uart_gpcv2178_h5_block_sent(void)
{
    if (!btstack_uart_h5_tx_active) {
        return;
    }

    if (btstack_slip_encoder_has_data()) {
        btstack_uart_gpcv2178_h5_send_next_chunk();
        return;
    }

    btstack_uart_h5_tx_active = 0;
    if (btstack_uart_frame_sent != NULL) {
        btstack_uart_frame_sent();
    }
}

static void btstack_uart_gpcv2178_h5_send_next_chunk(void)
{
    uint16_t pos = 0;

    while (btstack_slip_encoder_has_data() && (pos < sizeof(btstack_uart_h5_tx_slip_buffer))) {
        btstack_uart_h5_tx_slip_buffer[pos++] = btstack_slip_encoder_get_byte();
    }

    if (pos == 0u) {
        btstack_uart_gpcv2178_h5_block_sent();
        return;
    }

    btstack_uart_block_sent = btstack_uart_gpcv2178_h5_block_sent;
    btstack_uart_gpcv2178_send_block(btstack_uart_h5_tx_slip_buffer, pos);
}

static void btstack_uart_gpcv2178_set_frame_received(void (*handler)(uint16_t frame_size))
{
    btstack_uart_frame_received = handler;
}

static void btstack_uart_gpcv2178_set_frame_sent(void (*handler)(void))
{
    btstack_uart_frame_sent = handler;
}

static void btstack_uart_gpcv2178_receive_frame(uint8_t *buffer, uint16_t len)
{
    btstack_slip_decoder_init(buffer, len);
    btstack_uart_h5_rx_have_frame_buffer = 1;
    btstack_uart_h5_rx_receive_start = 0;

    if (btstack_uart_rx_dma_enabled &&
        !btstack_uart_h5_rx_dma_channel_owned &&
        !btstack_uart_h5_rx_dma_running) {
        btstack_uart_h5_rx_dma_arm_new();
    }
    btstack_uart_h5_rx_idle_timer_schedule();
}

static void btstack_uart_gpcv2178_send_frame(const uint8_t *buffer, uint16_t len)
{
    if (btstack_uart_h5_tx_active) {
        DBG_PRINT("btstack uart h5 tx busy\r\n");
        return;
    }

    btstack_uart_h5_tx_active = 1;
    btstack_slip_encoder_start(buffer, len);
    btstack_uart_gpcv2178_h5_send_next_chunk();
}

static int btstack_uart_gpcv2178_get_supported_sleep_modes(void)
{
    return 0;
}

static void btstack_uart_gpcv2178_set_sleep(btstack_uart_sleep_mode_t sleep_mode)
{
    (void)sleep_mode;
}

static void btstack_uart_gpcv2178_set_wakeup_handler(void (*handler)(void))
{
    (void)handler;
}

void btstack_uart_gpcv2178_enable_rx_dma_probe(uint8_t enable)
{
    btstack_uart_rx_dma_enabled = enable ? 1u : 0u;
}

static const btstack_uart_t btstack_uart_gpcv2178 = {
    &btstack_uart_gpcv2178_init,
    &btstack_uart_gpcv2178_open,
    &btstack_uart_gpcv2178_close,
    &btstack_uart_gpcv2178_set_block_received,
    &btstack_uart_gpcv2178_set_block_sent,
    &btstack_uart_gpcv2178_set_baudrate,
    &btstack_uart_gpcv2178_set_parity,
    &btstack_uart_gpcv2178_set_flowcontrol,
    &btstack_uart_gpcv2178_receive_block,
    &btstack_uart_gpcv2178_send_block,
    &btstack_uart_gpcv2178_get_supported_sleep_modes,
    &btstack_uart_gpcv2178_set_sleep,
    &btstack_uart_gpcv2178_set_wakeup_handler,
    &btstack_uart_gpcv2178_set_frame_received,
    &btstack_uart_gpcv2178_set_frame_sent,
    &btstack_uart_gpcv2178_receive_frame,
    &btstack_uart_gpcv2178_send_frame
};

const btstack_uart_t *btstack_uart_gpcv2178_instance(void)
{
    return &btstack_uart_gpcv2178;
}

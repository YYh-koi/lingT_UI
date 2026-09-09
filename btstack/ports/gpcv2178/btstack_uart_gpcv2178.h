#ifndef BTSTACK_UART_GPCV2178_H
#define BTSTACK_UART_GPCV2178_H

#include <stdint.h>
#include "btstack_uart.h"

const btstack_uart_t *btstack_uart_gpcv2178_instance(void);
void btstack_uart_gpcv2178_enable_rx_dma_probe(uint8_t enable);

#endif

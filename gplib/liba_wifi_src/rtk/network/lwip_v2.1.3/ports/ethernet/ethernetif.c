#include "drv_l1_arm_cp15.h"
#include "drv_l1_eth_mac.h"
#include "lwip/opt.h"
#include "lwip/stats.h"
#include "netif/etharp.h"
#include "ethernetif.h"
#include "gplib_print_string.h"

#define NON_CACHE_ADDR_GET(addr) (addr|0x80000000)
#define ETH_RXBUF_USE_COPY    0

#define ETHIF_THREAD_STACK_SIZE    (16384)

/* Define those to better describe your network interface. */
#define IFNAME0 'g'
#define IFNAME1 'p'

#define MAC_ADDR0   2
#define MAC_ADDR1   0
#define MAC_ADDR2   0
#define MAC_ADDR3   0
#define MAC_ADDR4   0
#define MAC_ADDR5   0

#define RX_OVERFLOW_FLAG 256

#if LWIP_STATS
/** Used to compute lwIP bandwidth. */
uint32_t lwip_tx_count = 0;
uint32_t lwip_rx_count = 0;
uint32_t lwip_tx_rate = 0;
uint32_t lwip_rx_rate = 0;
#endif

/* Queue to signal incoming packets */
osMessageQId s_rxQueue = NULL;
#define C_ETHIF_RX_QUEUE_SZ 3

/* MAC transmission mutex */
osSemaphoreId s_txSemaphore = NULL;

osThreadId ethernetif_input_id = 0;

/* Global Ethernet handle*/
gp_eth_handle eth_handle;

INT8U eth_tx_buf[NUM_TX_DESCS][ETH_TX_BUF_SIZE] __attribute__((aligned(64)));;
INT8U eth_rx_buf[NUM_RX_DESCS][ETH_RX_BUF_SIZE] __attribute__((aligned(64)));;

eth_dma_desc eth_tx_dma_desc[NUM_TX_DESCS] __attribute__((aligned(64)));;
eth_dma_desc eth_rx_dma_desc[NUM_RX_DESCS] __attribute__((aligned(64)));;

static void ethernetif_input( void const * argument );
extern void asmmemcpy(void* dest,void* src, int len);
void (*ethernetif_notify_link_changed) (struct netif *netif);

void ethif_rx_callback(void)
{
    uint32_t packet_cnt = drv_l1_eth_rx_packet_cnt_clear();

    osMessagePut(s_rxQueue, (uint32_t)&packet_cnt, osWaitForever);
}

void ethif_rx_overflow_callback(void)
{
    uint32_t flag = RX_OVERFLOW_FLAG;

    osMessagePut(s_rxQueue, (uint32_t)&flag, osWaitForever);
}

void ethif_tx_callback(void)
{
    eth_dma_desc *tx_desc;

    tx_desc = eth_handle.tx_desc;
    tx_desc->empty_flag = 1;
    tx_desc->pkt_size = 0;
    eth_handle.tx_desc = (eth_dma_desc *) eth_handle.tx_desc->next_desc;
    osSemaphoreRelease(s_txSemaphore);
}

static void gpmac_rx_buffer_allocate(void)
{
	uint32_t i = 0;
	struct pbuf *p = 0;
    eth_dma_desc *rx_desc;

	/* Set up the RX descriptors. */
	for (i = 0; i < NUM_RX_DESCS; i++) {
        if (eth_handle.rx_pbuf[i] == 0) {
            /* Allocate a new pbuf with the maximum size. */
            p = pbuf_alloc(PBUF_RAW, (uint16_t) ETH_RX_BUF_SIZE, PBUF_POOL);
            if (p == NULL) {
                LWIP_DEBUGF(NETIF_DEBUG, ("gpmac_rx_buffer_allocate: pbuf allocation failure\n"));
                break;
            }

            /* Make sure lwIP is well configured so one pbuf can contain the maximum packet size. */
            LWIP_ASSERT("gpmac_rx_buffer_allocate: pbuf size too small!", pbuf_clen(p) <= 1);

            /* Make sure that the payload buffer is properly aligned. */
            LWIP_ASSERT("gpmac_rx_buffer_allocate: unaligned p->payload buffer address",
                    (((uint32_t)p->payload & 0xFFFFFFFC) == (uint32_t)p->payload));

            rx_desc = (eth_dma_desc *) NON_CACHE_ADDR_GET((uint32_t)&eth_rx_dma_desc[i]);
            /* adjust payload address to caceh line alignment */
            p->payload = LWIP_MEM_ALIGN_CACHELINE(p->payload);
            rx_desc->pkt_addr = (uint32_t) p->payload;
            eth_handle.rx_pbuf[i] = (void *) p;

            /* invalidate the cache address of payload before starting rx dma */
            drv_l1_cp15_invalidate_dcache_range((uint32_t)p->payload, ETH_RX_BUF_SIZE);

            LWIP_DEBUGF(NETIF_DEBUG,
                    ("gpmac_rx_buffer_allocate: new pbuf allocated: %p [idx=%lu]\n",
                    p, i));
        }
	}
}

static void gpmac_rx_buffer_free(void)
{
    uint32_t i = 0;

    for (i = 0; i < NUM_RX_DESCS; i++) {
        if (eth_handle.rx_pbuf[i] != 0) {
            pbuf_free(eth_handle.rx_pbuf[i]);
        }
    }
}

/**
  * @brief In this function, the hardware should be initialized.
  * Called from ethernetif_init().
  *
  * @param netif the already initialized lwip network interface structure
  *        for this ethernetif
  */
static void low_level_init(struct netif *netif)
{
    uint32_t i;
    uint8_t macaddress[6]= { MAC_ADDR0, MAC_ADDR1, MAC_ADDR2, MAC_ADDR3, MAC_ADDR4, MAC_ADDR5 };

    eth_handle.auto_negotiation = ETH_AUTO_NEGOT_ENABLE;
    eth_handle.mac_addr = macaddress;
    eth_handle.speed = ETH_SPEED_100M;
    eth_handle.duplex_mode = ETH_FULL_DUPLEX;
    eth_handle.mac_mode = ETH_MAC_NORMAL;

    if (drv_l1_eth_init(&eth_handle) == 0)
    {
        /* Set netif link flag */
        netif->flags |= NETIF_FLAG_LINK_UP;
    } else {
        DBG_PRINT("driver l1 ethernet phy initial fail\r\n");
    }

    /* TX descriptor initial */
    drv_l1_eth_dma_tx_desc_init(&eth_handle, eth_tx_dma_desc, &eth_tx_buf[0][0], NUM_TX_DESCS);
    drv_l1_cp15_clean_dcache_range((INT32U)eth_tx_dma_desc, sizeof(eth_dma_desc)*NUM_TX_DESCS);

    /* TX descriptor intiial */
    for (i = 0; i < NUM_RX_DESCS; i++) {
        eth_handle.rx_pbuf[i] = 0;
    }

#if ETH_RXBUF_USE_COPY == 1
    drv_l1_eth_dma_rx_desc_init(&eth_handle, eth_rx_dma_desc, &eth_rx_buf[0][0], NUM_RX_DESCS);
    drv_l1_cp15_invalidate_dcache_range((uint32_t)eth_rx_buf, ETH_RX_BUF_SIZE*NUM_RX_DESCS);
#else
    gpmac_rx_buffer_allocate(); // use pbuf to rx buffer
    drv_l1_eth_dma_rx_desc_init(&eth_handle, eth_rx_dma_desc, 0, NUM_RX_DESCS);
#endif
    drv_l1_cp15_clean_dcache_range((uint32_t)eth_rx_dma_desc, sizeof(eth_dma_desc)*NUM_RX_DESCS);
    /* rx descriptor is accessed throuhg non-cache region, ivalidate rx descriptor avoid clean dirty data to memory in cache replacement */
    drv_l1_cp15_invalidate_dcache_range((uint32_t)eth_rx_dma_desc, sizeof(eth_dma_desc)*NUM_RX_DESCS);
    drv_l1_eth_callback_set(ethif_tx_callback, ethif_rx_callback, ethif_rx_overflow_callback);

    /* set netif MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    /* set netif MAC hardware address */
    netif->hwaddr[0] =  MAC_ADDR0;
    netif->hwaddr[1] =  MAC_ADDR1;
    netif->hwaddr[2] =  MAC_ADDR2;
    netif->hwaddr[3] =  MAC_ADDR3;
    netif->hwaddr[4] =  MAC_ADDR4;
    netif->hwaddr[5] =  MAC_ADDR5;

    /* set netif maximum transfer unit */
    netif->mtu = 1500;

    /* Accept broadcast address and ARP traffic */
    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

    /* create a queue used for ethernetif of frame reception */
    osMessageQDef_t s_rxQueue_def = {C_ETHIF_RX_QUEUE_SZ, sizeof(uint32_t), 0};
    s_rxQueue = osMessageCreate(&s_rxQueue_def, NULL);

    /* create a binary semaphore used for transmission mutex */
    osSemaphoreDef(TSEM);
    s_txSemaphore = osSemaphoreCreate(osSemaphore(TSEM) , 1 );
    osSemaphoreWait(s_txSemaphore, osWaitForever);

    /* create the task that handles the ETH_MAC */
    osThreadDef(ethernetif_input, osPriorityRealtime, 0, ETHIF_THREAD_STACK_SIZE);
    ethernetif_input_id = osThreadCreate(&os_thread_def_ethernetif_input, netif);

    /* Enable DMA reception */
    drv_l1_eth_rx_dma_enable();

    /* enable phy link change interrupt */
    drv_l1_eth_phy_link_change_int_en(1);
}

static void low_level_uninit(void)
{
    drv_l1_eth_uninit();

    gpmac_rx_buffer_free();

    if (s_rxQueue) {
        vQueueDelete(s_rxQueue);
        s_rxQueue = 0;
    }

    if (s_txSemaphore) {
        vSemaphoreDelete(s_txSemaphore);
        s_txSemaphore = 0;
    }

    if (ethernetif_input_id) {
        osThreadTerminate(ethernetif_input_id);
        ethernetif_input_id = 0;
    }
}

/**
  * @brief This function should do the actual transmission of the packet. The packet is
  * contained in the pbuf that is passed to the function. This pbuf
  * might be chained.
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
  * @return ERR_OK if the packet could be sent
  *         an err_t value if the packet couldn't be sent
  *
  * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
  *       strange results. You might consider waiting for space in the DMA queue
  *       to become available since the stack doesn't retry to send a packet
  *       dropped because of memory failure (except for the TCP timers).
  */
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    struct pbuf *q;
    uint8_t *buffer;
    eth_dma_desc *tx_desc;
    int i;

    buffer = (uint8_t *)eth_handle.tx_desc->pkt_addr;
    tx_desc = eth_handle.tx_desc;

    /* copy pbuf chain into tx buffer */
    for(q = p; q != NULL; q = q->next) {
        /* use optimized memcpy for arm, faster sextuple than original memcpy */
        asmmemcpy(buffer, q->payload, q->len);
		buffer += q->len;
	}

    tx_desc->pkt_size = p->tot_len;
    tx_desc->empty_flag = 0; // clear empty flag to represent a vaild data

    drv_l1_cp15_clean_dcache_range(tx_desc->pkt_addr, tx_desc->pkt_size);
    drv_l1_cp15_clean_dcache_range((INT32U)tx_desc, sizeof(eth_dma_desc));
    /* enable tx dma */

    for (i = 1; i <= 3; i++) {
        drv_l1_eth_tx_dma_enable();
        if (osSemaphoreWait(s_txSemaphore, 1/portTICK_RATE_MS) != osOK) {
            if (i == 3) {
                DBG_PRINT("Eth Tx is busy\r\n");
                return ERR_TIMEOUT;
            }
        } else {
            break;
        }
    }

#if LWIP_STATS
	lwip_tx_count += p->tot_len;
#endif
	LINK_STATS_INC(link.xmit);

    return ERR_OK;
}

/**
  * @brief Should allocate a pbuf and transfer the bytes of the incoming
  * packet from the interface into the pbuf.
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @return a pbuf filled with the received packet (including MAC header)
  *         NULL on memory error
  */
static struct pbuf * low_level_input(struct netif *netif)
{
	struct pbuf *p = 0;
	uint32_t length = 0;
	uint32_t index = 0;
	eth_dma_desc *rx_desc;

    rx_desc = (eth_dma_desc*) NON_CACHE_ADDR_GET((uint32_t)eth_handle.rx_desc);

	/* Check that a packet has been received if the empty_flag is 0 */
	if (rx_desc->empty_flag == 0) {
		/* Minus 4 byte FCS */
		length = (rx_desc->pkt_size & PKT_SIZE_MASK) - 4;
      #if ETH_RXBUF_USE_COPY == 1
        p = pbuf_alloc(PBUF_RAW, length, PBUF_POOL);
        asmmemcpy((uint8_t *)p->payload, (uint8_t *)rx_desc->pkt_addr, length);
        drv_l1_cp15_invalidate_dcache_range((uint32_t)rx_desc->pkt_addr, length);
      #else
		/* Fetch pre-allocated pbuf. */
		index = ((uint32_t)eth_handle.rx_desc - (uint32_t)eth_rx_dma_desc) / sizeof(eth_dma_desc);
		p = (struct pbuf *) eth_handle.rx_pbuf[index];
		/* Remove this pbuf from its desriptor. */
		eth_handle.rx_pbuf[index] = 0;
      #endif

		p->len = length;

		LWIP_DEBUGF(NETIF_DEBUG,
				("low_level_input: DMA buffer %p received, size=%lu [idx=%lu]\n",
				p, length, index));

        drv_l1_cp15_invalidate_dcache_range((uint32_t)p->payload, p->len);
		/* Set pbuf total packet size. */
		p->tot_len = length;
		LINK_STATS_INC(link.recv);

      #if ETH_RXBUF_USE_COPY == 0
		/* Fill empty descriptors with new pbufs. */
		gpmac_rx_buffer_allocate();
      #endif

		/* Set the empty flag ready for future receive. */
		rx_desc->empty_flag = 1; /* non cache region */
		eth_handle.rx_desc = (eth_dma_desc *) eth_handle.rx_desc->next_desc;

#if LWIP_STATS
        lwip_rx_count += length;
#endif
	} else {
        drv_l1_eth_disable_mac();
        DBG_PRINT("rx descriptor has the invaild empty flag\r\n");
	}

	return p;
}

/**
  * @brief This function is the ethernetif_input task, it is processed when a packet
  * is ready to be read from the interface. It uses the function low_level_input()
  * that should handle the actual reception of bytes from the network
  * interface. Then the type of the received packet is determined and
  * the appropriate input function is called.
  *
  * @param netif the lwip network interface structure for this ethernetif
  */
static void ethernetif_input( void const * argument )
{
    struct pbuf *p;
    struct netif *netif = (struct netif *) argument;
    struct eth_hdr *ethhdr;
    osEvent result;
    uint32_t packet_cnt;

    while(1) {
        /* wait for the RX notificication semphore */
        result = osMessageGet(s_rxQueue, osWaitForever);
        if (result.status != osEventMessage) {
            continue;
        }

        /* received packet count */
        packet_cnt = result.value.v;

        if (packet_cnt == RX_OVERFLOW_FLAG) { // rx overflow occurred
            R_ETH_FIFO_CR0 |= 0x6; // reset mmiirfif_fab and mmiirfif_sys
            R_ETH_FIFO_CR0 &= ~0x6;
            drv_l1_eth_rx_dma_enable();
            continue;
        }

        if (packet_cnt > 255) {
            DBG_PRINT("invaild received packet count vaule\r\n");
            continue;
        }

        while(packet_cnt != 0) {
            /* Move received packet into a new pbuf. */
            p = low_level_input(netif);
            if (p == NULL) {
                DBG_PRINT("pbuf is null in receive process\r\n");
                break;
            }

            /* Points to packet payload, which starts with an Ethernet header. */
            ethhdr = p->payload;

            switch (htons(ethhdr->type)) {
                case ETHTYPE_IP:
                case ETHTYPE_ARP:
#if PPPOE_SUPPORT
                case ETHTYPE_PPPOEDISC:
                case ETHTYPE_PPPOE:
#endif /* PPPOE_SUPPORT */
                /* Send packet to lwIP for processing. */
                    if (netif->input(p, netif) != ERR_OK) {
                        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
                        /* Free buffer. */
                        pbuf_free(p);
                    }
                    break;
                default:
                    /* Free buffer. */
                    pbuf_free(p);
                    break;
            }
            packet_cnt--;
        }
    }
}

/**
  * @brief Should be called at the beginning of the program to set up the
  * network interface. It calls the function low_level_init() to do the
  * actual setup of the hardware.
  *
  * This function should be passed as a parameter to netif_add().
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @return ERR_OK if the loopif is initialized
  *         ERR_MEM if private data couldn't be allocated
  *         any other err_t on error
  */
static err_t ethernetif_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;

    netif->output = etharp_output;
    netif->linkoutput = low_level_output;

    /* initialize the hardware */
    low_level_init(netif);

    return ERR_OK;
}

static err_t ethernetif_uninit(struct netif *netif)
{
    low_level_uninit();

    return ERR_OK;
}

/**
  * @brief  This function sets the netif link status.
  * @param  netif: the network interface
  * @retval None
  */
void ethernetif_set_link(void const *argument)
{
    struct link_struct *link_arg = (struct link_struct *)argument;
    ethernetif_notify_link_changed = link_arg->link_change;

    for(;;) {
        if (drv_l1_eth_phy_link_change() == 1) {
            drv_l1_eth_phy_link_status();
            if (drv_l1_eth_phy_link_status() == 1) {
                DBG_PRINT("link change detect in\r\n");
                netif_set_link_up(link_arg->netif);
            } else {
                DBG_PRINT("link change detect out\r\n");
                netif_set_link_down(link_arg->netif);
            }
        }
        vTaskDelay(1000);
    }
}

/**
  * @brief  Link callback function, this function is called on change of link status
  *         to update low level driver configuration.
* @param  netif: The network interface
  * @retval None
  */
void ethernetif_update_config(struct netif *netif)
{
    uint16_t val = 0;
    uint32_t t1;

    if(netif_is_link_up(netif))
    {
        drv_l1_eth_phy_link_change_int_en(0);

        if (eth_handle.auto_negotiation == ETH_AUTO_NEGOT_ENABLE) {
#if ETH_PHY_DEVICE != PHY_RTL8201F
            drv_l1_eth_phy_read(PHY_ADDR,PHY_BMCR,&val);
            /* if auto-negotiation bit were enabled then restart auto-negotiation bit
               if auto-negotiation bit is not enabled then start auto-negotiation */
            val |= 0x1200;
            drv_l1_eth_phy_write(PHY_ADDR,PHY_BMCR, val);
#endif
            t1 = xTaskGetTickCount();
            while(1) {
                if (drv_l1_eth_phy_read(PHY_ADDR,PHY_BMSR,&val) != 0) {
                    return;
                }

                if ((val & 0x20)) { //check auto-negotiation complete bit
                    break;
                }
                if ((xTaskGetTickCount() - t1)/portTICK_RATE_MS > PHY_AUTONEGO_COMPLETE_TIMEOUT) {
                    DBG_PRINT("Ethernet PHY wait auto negotiation timeout\r\n");
                    return;
                }
            }

            drv_l1_eth_phy_read(PHY_ADDR,PHY_BMCR,&val);

            if (val & 0x100) {
                DBG_PRINT("full duplex mode\r\n");
                eth_handle.duplex_mode = ETH_FULL_DUPLEX;
            } else {
                DBG_PRINT("half duplex mode\r\n");
                eth_handle.duplex_mode = ETH_HALF_DUPLEX;
            }

            if (val & 0x2000) {
                DBG_PRINT("100Mbps mode\r\n");
                eth_handle.speed = ETH_SPEED_100M;
            } else {
                DBG_PRINT("10Mbps mode\r\n");
                eth_handle.speed = ETH_SPEED_10M;
            }
        } else {
            val = 0;
            if (eth_handle.duplex_mode == ETH_FULL_DUPLEX) {
                val |= PHY_BMCR_FULL_DUPLEX;
            }
            if (eth_handle.speed == ETH_SPEED_100M) {
                val |= PHY_BMCR_100M;
            }

            drv_l1_eth_phy_write(PHY_ADDR,PHY_BMCR,val);
        }
        drv_l1_eth_config_mac(&eth_handle);
        drv_l1_eth_rx_dma_enable();
        /* enable phy link change interrupt */
        drv_l1_eth_phy_link_change_int_en(1);
    }
    else
    {
        /* Stop MAC interface */
        drv_l1_eth_disable_mac();
    }

    DBG_PRINT("Ethernet link status change\r\n");

    if (ethernetif_notify_link_changed) {
        ethernetif_notify_link_changed(netif);
    }
}

netif_init_fn eth_ethernetif_init()
{
    return &ethernetif_init;
}

netif_init_fn eth_ethernetif_uninit()
{
    return &ethernetif_uninit;
}


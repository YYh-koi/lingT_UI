/**
  ******************************************************************************
  * @file    netconf.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    07-October-2011
  * @brief   Network connection configuration
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "wifi/ethernetif.h"
#include "main.h"
#include "lwip_netconf.h"
#include <string.h>

/* Give default value if not defined */
#ifndef NET_IF_NUM
#ifdef CONFIG_CONCURRENT_MODE
#define NET_IF_NUM 2
#else
#define NET_IF_NUM 1
#endif
#endif

/*Static IP ADDRESS*/
#ifndef IP_ADDR0
#define IP_ADDR0   192
#define IP_ADDR1   168
#define IP_ADDR2   1
#define IP_ADDR3   80
#endif

/*NETMASK*/
#ifndef NETMASK_ADDR0
#define NETMASK_ADDR0   255
#define NETMASK_ADDR1   255
#define NETMASK_ADDR2   255
#define NETMASK_ADDR3   0
#endif

/*Gateway Address*/
#ifndef GW_ADDR0
#define GW_ADDR0   192
#define GW_ADDR1   168
#define GW_ADDR2   1
#define GW_ADDR3   1
#endif

/*Static IP ADDRESS*/
#ifndef AP_IP_ADDR0
#define AP_IP_ADDR0   192
#define AP_IP_ADDR1   168
#define AP_IP_ADDR2   43
#define AP_IP_ADDR3   1
#endif

/*NETMASK*/
#ifndef AP_NETMASK_ADDR0
#define AP_NETMASK_ADDR0   255
#define AP_NETMASK_ADDR1   255
#define AP_NETMASK_ADDR2   255
#define AP_NETMASK_ADDR3   0
#endif

/*Gateway Address*/
#ifndef AP_GW_ADDR0
#define AP_GW_ADDR0   192
#define AP_GW_ADDR1   168
#define AP_GW_ADDR2   43
#define AP_GW_ADDR3   1
#endif

/* Private define ------------------------------------------------------------*/
#define MAX_DHCP_TRIES 5

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

struct netif xnetif[NET_IF_NUM]; /* network interface structure */
/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Initializes the lwIP stack
  * @param  None
  * @retval None
  */
void LwIP_Init(void)
{
	ip_addr_t ipaddr;
	ip_addr_t netmask;
	ip_addr_t gw;
#ifdef USE_LCD
	uint8_t iptab[4];
	uint8_t iptxt[20];
#endif
	int8_t idx = 0;
	/* Create tcp_ip stack thread */
	tcpip_init( NULL, NULL );

#ifdef USE_LCD
	iptab[0] = IP_ADDR3;
	iptab[1] = IP_ADDR2;
	iptab[2] = IP_ADDR1;
	iptab[3] = IP_ADDR0;

	sprintf((char*)iptxt, "  %d.%d.%d.%d", iptab[3], iptab[2], iptab[1], iptab[0]);

	LCD_DisplayStringLine(Line8, (uint8_t*)"  Static IP address   ");
	LCD_DisplayStringLine(Line9, iptxt);
#endif

	/* - netif_add(struct netif *netif, struct ip_addr *ipaddr,
	        struct ip_addr *netmask, struct ip_addr *gw,
	        void *state, err_t (* init)(struct netif *netif),
	        err_t (* input)(struct pbuf *p, struct netif *netif))

	Adds your network interface to the netif_list. Allocate a struct
	netif and pass a pointer to this structure as the first argument.
	Give pointers to cleared ip_addr structures when using DHCP,
	or fill them with sane numbers otherwise. The state pointer may be NULL.

	The init function pointer must point to a initialization function for
	your ethernet netif interface. The following code illustrates it's use.*/
	//printf("NET_IF_NUM:%d\n\r",NET_IF_NUM);
	for(idx=NET_IF_NUM - 1;idx>=0;idx--){
		if(idx==0){
			IP4_ADDR(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
			IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
			IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
		}
		else{
			IP4_ADDR(&ipaddr, AP_IP_ADDR0, AP_IP_ADDR1, AP_IP_ADDR2, AP_IP_ADDR3);
			IP4_ADDR(&netmask, AP_NETMASK_ADDR0, AP_NETMASK_ADDR1 , AP_NETMASK_ADDR2, AP_NETMASK_ADDR3);
			IP4_ADDR(&gw, AP_GW_ADDR0, AP_GW_ADDR1, AP_GW_ADDR2, AP_GW_ADDR3);
		}
		xnetif[idx].name[0] = 'r';
		xnetif[idx].name[1] = '0'+idx;
		netif_add(&xnetif[idx], &ipaddr, &netmask, &gw, NULL, wifi_ethernetif_init(), &tcpip_input);
	}

	/*  Registers the default network interface. */
	netif_set_default(&xnetif[0]);

	/*  When the netif is fully configured this function must be called.*/
	for(idx = 0;idx < NET_IF_NUM;idx++)
		netif_set_up(&xnetif[idx]);
}

void LwIP_DeInit(void)
{
    int8_t idx = 0;

    for (idx = NET_IF_NUM - 1; idx >= 0; idx--){
        netif_remove(&xnetif[idx]);
        memset(&xnetif[idx], 0, sizeof(struct netif));
    }

    tcpip_deinit();
}

/**
  * @brief  LwIP_DHCP_Process_Handle
  * @param  None
  * @retval None
  */
void LwIP_DHCP(uint8_t idx, uint8_t dhcp_state)
{
	ip_addr_t ipaddr;
	ip_addr_t netmask;
	ip_addr_t gw;
	uint32_t IPaddress;
	uint8_t iptab[4];
	uint8_t DHCP_state;
#ifdef USE_LCD
	uint8_t iptxt[20];
#endif

	int mscnt = 0;
	struct netif *pnetif = NULL;

	DHCP_state = dhcp_state;

	if(idx > 1)
		idx = 1;
	pnetif = &xnetif[idx];
	if(DHCP_state == 0){
		pnetif->ip_addr.addr = 0;
		pnetif->netmask.addr = 0;
		pnetif->gw.addr = 0;
	}

	for (;;)
	{
		//printf("\n\r ========DHCP_state:%d============\n\r",DHCP_state);
		switch (DHCP_state)
		{
			case DHCP_START:
			{
				dhcp_start(pnetif);
				IPaddress = 0;
				DHCP_state = DHCP_WAIT_ADDRESS;
#ifdef USE_LCD
				LCD_DisplayStringLine(Line4, (uint8_t*)"     Looking for    ");
				LCD_DisplayStringLine(Line5, (uint8_t*)"     DHCP server    ");
				LCD_DisplayStringLine(Line6, (uint8_t*)"     please wait... ");
#endif
			}
			break;

		case DHCP_WAIT_ADDRESS:
		{
			/* Read the new IP address */
			IPaddress = pnetif->ip_addr.addr;

			if (IPaddress!=0)
			{
				DHCP_state = DHCP_ADDRESS_ASSIGNED;

				iptab[0] = (uint8_t)(IPaddress >> 24);
				iptab[1] = (uint8_t)(IPaddress >> 16);
				iptab[2] = (uint8_t)(IPaddress >> 8);
				iptab[3] = (uint8_t)(IPaddress);
#ifdef USE_LCD
				sprintf((char*)iptxt, "  %d.%d.%d.%d", iptab[3], iptab[2], iptab[1], iptab[0]);

				LCD_ClearLine(Line4);
				LCD_ClearLine(Line5);
				LCD_ClearLine(Line6);
				/* Display the IP address */
				LCD_DisplayStringLine(Line7, (uint8_t*)"IP address assigned ");
				LCD_DisplayStringLine(Line8, (uint8_t*)"  by a DHCP server  ");
				LCD_DisplayStringLine(Line9, iptxt);
#endif
				printf("\n\rIP address : %d.%d.%d.%d", iptab[3], iptab[2], iptab[1], iptab[0]);
				return;
			}
			else
			{
				/* DHCP timeout */
				struct dhcp *dhcp = netif_dhcp_data(pnetif);

				if (dhcp->tries > MAX_DHCP_TRIES)
				{
					DHCP_state = DHCP_TIMEOUT;

					/* Stop DHCP */
					dhcp_stop(pnetif);

					/* Static address used */
					IP4_ADDR(&ipaddr, IP_ADDR0 ,IP_ADDR1 , IP_ADDR2 , IP_ADDR3 );
					IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
					IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
					netif_set_addr(pnetif, &ipaddr , &netmask, &gw);

					iptab[0] = IP_ADDR3;
					iptab[1] = IP_ADDR2;
					iptab[2] = IP_ADDR1;
					iptab[3] = IP_ADDR0;
#ifdef USE_LCD
					LCD_DisplayStringLine(Line7, (uint8_t*)"    DHCP timeout    ");

					sprintf((char*)iptxt, "  %d.%d.%d.%d", iptab[3], iptab[2], iptab[1], iptab[0]);

					LCD_ClearLine(Line4);
					LCD_ClearLine(Line5);
					LCD_ClearLine(Line6);
					LCD_DisplayStringLine(Line8, (uint8_t*)"  Static IP address   ");
					LCD_DisplayStringLine(Line9, iptxt);
#endif
					printf("\n\rDHCP timeout");
					printf("\n\rStatic IP address : %d.%d.%d.%d", iptab[3], iptab[2], iptab[1], iptab[0]);

					return;
				}else
				{
					//sys_msleep(DHCP_FINE_TIMER_MSECS);
					vTaskDelay(DHCP_FINE_TIMER_MSECS);
					dhcp_fine_tmr();
					mscnt += DHCP_FINE_TIMER_MSECS;
					if (mscnt >= DHCP_COARSE_TIMER_SECS*1000)
					{
						dhcp_coarse_tmr();
						mscnt = 0;
					}
				}
			}
		}
		break;
		case DHCP_RELEASE_IP:
			printf("\n\rLwIP_DHCP: Release ip");
			dhcp_release(pnetif);
			return;
		case DHCP_STOP:
			printf("\n\rLwIP_DHCP: dhcp stop.");
			dhcp_stop(pnetif);
			return;
		default:
			break;
	}

	}
}

uint8_t* LwIP_GetMAC(struct netif *pnetif)
{
	return (uint8_t *) (pnetif->hwaddr);
}

uint8_t* LwIP_GetIP(struct netif *pnetif)
{
	return (uint8_t *) &(pnetif->ip_addr);
}

uint8_t* LwIP_GetGW(struct netif *pnetif)
{
	return (uint8_t *) &(pnetif->gw);
}

uint8_t* LwIP_GetMASK(struct netif *pnetif)
{
	return (uint8_t *) &(pnetif->netmask);
}

#if LWIP_DNS
void LwIP_GetDNS(struct ip_addr* dns)
{
	*dns = dns_getserver(0);
}

void LwIP_SetDNS(struct ip_addr* dns)
{
	dns_setserver(0, dns);
}
#endif
void LwIP_UseStaticIP(struct netif *pnetif)
{
	ip_addr_t ipaddr;
	ip_addr_t netmask;
	ip_addr_t gw;

	/* Static address used */
	if(pnetif->name[1] == '0'){
		IP4_ADDR(&ipaddr, IP_ADDR0 ,IP_ADDR1 , IP_ADDR2 , IP_ADDR3 );
		IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
		IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
	}else{
		IP4_ADDR(&ipaddr, AP_IP_ADDR0, AP_IP_ADDR1, AP_IP_ADDR2, AP_IP_ADDR3);
		IP4_ADDR(&netmask, AP_NETMASK_ADDR0, AP_NETMASK_ADDR1 , AP_NETMASK_ADDR2, AP_NETMASK_ADDR3);
		IP4_ADDR(&gw, AP_GW_ADDR0, AP_GW_ADDR1, AP_GW_ADDR2, AP_GW_ADDR3);
	}

	netif_set_addr(pnetif, &ipaddr , &netmask, &gw);
}

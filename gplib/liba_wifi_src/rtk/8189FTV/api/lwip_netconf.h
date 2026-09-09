/**
  ******************************************************************************
  * @file    netconf.h
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    07-October-2011
  * @brief   This file contains all the functions prototypes for the netconf.c
  *          file.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __NETCONF_H
#define __NETCONF_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "tcpip.h"
/* Includes ------------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
typedef enum
{
	DHCP_START=0,
	DHCP_WAIT_ADDRESS,
	DHCP_ADDRESS_ASSIGNED,
	DHCP_RELEASE_IP,
	DHCP_STOP,
	DHCP_TIMEOUT
} DHCP_State_TypeDef;

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void LwIP_Init(void);
void LwIP_DeInit(void);
void LwIP_DHCP(uint8_t idx, uint8_t dhcp_state);
unsigned char* LwIP_GetMAC(struct netif *pnetif);
unsigned char* LwIP_GetIP(struct netif *pnetif);
unsigned char* LwIP_GetGW(struct netif *pnetif);
uint8_t* LwIP_GetMASK(struct netif *pnetif);
#if LWIP_DNS
void LwIP_GetDNS(struct ip_addr* dns);
void LwIP_SetDNS(struct ip_addr* dns);
#endif
void LwIP_UseStaticIP(struct netif *pnetif);
#ifdef __cplusplus
}
#endif

#endif /* __NETCONF_H */

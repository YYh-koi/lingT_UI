/**
  ******************************************************************************
  * @file    main.h
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    07-October-2011
  * @brief   This file contains all the functions prototypes for the main.c
  *          file.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _WLAN_SETTING_H_
#define _WLAN_SETTING_H_

#ifdef __cplusplus
 extern "C" {
#endif

/* WLAN CONFIG ---------------------------------------------------------------*/
#define CONFIG_WLAN	1
#define WIFI_USE_SDIO

/* Includes ------------------------------------------------------------------*/
#include "lwip_netconf.h"

#if CONFIG_WLAN
#include "wifi/wifi_conf.h"
#include "wifi/wifi_ind.h"
#endif

#if defined(WIFI_USE_SDIO) && defined(RTK_WIFI_DEMO)
#include "wifi_io.h"
#endif

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/



#if CONFIG_WLAN
#include <autoconf.h>
#define STA_MODE_SSID   "wlan_sta_rtk"    /* Set SSID here */
#define AP_MODE_SSID    "wlan_ap_rtk"    /* Set SSID here */
#define AP_DEFAULT_CH   11
#define WLAN0_NAME      "wlan0"
#define WLAN1_NAME      "wlan1"
#define WPA_PASSPHRASE  "12345678"        /* Max 32 characters */
#define WEP40_KEY       "12345"           /* 5 ASCII characters */
#endif



/* MAC ADDRESS*/
#define MAC_ADDR0   02
#define MAC_ADDR1   00
#define MAC_ADDR2   00
#define MAC_ADDR3   00
#define MAC_ADDR4   00
#define MAC_ADDR5   00

/*Static IP ADDRESS*/
#define IP_ADDR0   192
#define IP_ADDR1   168
#define IP_ADDR2   1
#define IP_ADDR3   50

/*NETMASK*/
#define NETMASK_ADDR0   255
#define NETMASK_ADDR1   255
#define NETMASK_ADDR2   255
#define NETMASK_ADDR3   0

/*Gateway Address*/
#define GW_ADDR0   192
#define GW_ADDR1   168
#define GW_ADDR2   25
#define GW_ADDR3   1

/*Static IP ADDRESS*/
#define AP_IP_ADDR0   192
#define AP_IP_ADDR1   168
#define AP_IP_ADDR2   43
#define AP_IP_ADDR3   1

/*NETMASK*/
#define AP_NETMASK_ADDR0   255
#define AP_NETMASK_ADDR1   255
#define AP_NETMASK_ADDR2   255
#define AP_NETMASK_ADDR3   0

/*Gateway Address*/
#define AP_GW_ADDR0   192
#define AP_GW_ADDR1   168
#define AP_GW_ADDR2   43
#define AP_GW_ADDR3   1

/*Gereralplus Auth IP ADDRESS*/
#define GP_AUTH_IP_ADDR0   192
#define GP_AUTH_IP_ADDR1   168
#define GP_AUTH_IP_ADDR2   25
#define GP_AUTH_IP_ADDR3   1

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */


#ifdef __cplusplus
}
#endif

#endif /* _WLAN_SETTING_H_ */

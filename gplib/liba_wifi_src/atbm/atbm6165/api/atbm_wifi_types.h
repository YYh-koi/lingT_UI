/**************************************************************************************************************
 * AltoBeam public WiFi ABI types for customer applications.
 *************************************************************************************************************/

#ifndef ATBM_WIFI_TYPES_H
#define ATBM_WIFI_TYPES_H

typedef unsigned char ATBM_BOOL;
typedef unsigned char atbm_uint8;
typedef signed char atbm_int8;
typedef unsigned short atbm_uint16;
typedef signed short atbm_int16;
typedef unsigned int atbm_uint32;
typedef signed int atbm_int32;
typedef void atbm_void;
typedef atbm_int32 atbm_size_t;

#define ATBM_FALSE 0
#define ATBM_TRUE  1
#define ATBM_NULL  ((atbm_void *)0)

#define ATBM_WIFI_MAX_STA_IN_AP_MODE 8

struct atbm_net_device;
struct atbmwifi_common;
struct atbmwifi_vif;
struct netif;

#endif /* ATBM_WIFI_TYPES_H */

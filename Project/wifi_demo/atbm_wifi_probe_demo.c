#include "application.h"
#include "project.h"

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC) && (WIFI_VENDOR == WIFI_VENDOR_ATBM)

#include "atbm_wifi.h"

#define ATBM_AP_MODE_SSID       "GP-ATBM6165"
#define ATBM_AP_MODE_PASSWORD   "12345678"
#define ATBM_AP_MODE_CHANNEL    36

#define ATBM_P2P_DEVICE_NAME    "DIRECT-GP-ATBM6165"
#define ATBM_P2P_MODE_SSID      "GP-ATBM6165"
#define ATBM_P2P_MODE_PASSWORD  "12345678"
#define ATBM_P2P_MODE_CHANNEL   36

INT32S atbm_wifi_ap_mode_enable(void)
{
	static const atbm_wifi_ap_config_t config = {
		ATBM_AP_MODE_SSID,
		ATBM_AP_MODE_PASSWORD,
		ATBM_AP_MODE_CHANNEL,
		ATBM_WIFI_SECURITY_WPA2_PSK,
		0
	};
	int ret;

	ret = atbm_wifi_start_ap(&config);
	DBG_PRINT("ATBM WiFi AP start ret=%d ssid=%s channel=%d\r\n",
		ret,
		config.ssid,
		config.channel);
	if(ret != ATBM_WIFI_OK)
	{
		return STATUS_FAIL;
	}

	return STATUS_OK;
}

INT32S atbm_wifi_p2p_mode_enable(void)
{
	static const atbm_wifi_p2p_config_t config = {
		ATBM_P2P_DEVICE_NAME,
		ATBM_P2P_MODE_SSID,
		ATBM_P2P_MODE_PASSWORD,
		ATBM_P2P_MODE_CHANNEL
	};
	int ret;

	ret = atbm_wifi_start_p2p(&config);
	DBG_PRINT("ATBM WiFi P2P GO start ret=%d ssid=%s channel=%d\r\n",
		ret,
		config.ssid,
		config.channel);
	return (ret == ATBM_WIFI_OK) ? STATUS_OK : STATUS_FAIL;
}

#endif

#include <stddef.h>
#include <string.h>

#include "application.h"
#include "project.h"
#include "drv_l2.h"

#include "lwip/netif.h"
#include "atbm_lwip_port.h"
#include "dhcp/dhcps.h"

#include "atbm_hal.h"
#include "atbm_api.h"
#include "atbm_wifi_driver_api.h"
#include "atbm_wifi.h"
#include "atbm_os_mutex.h"

#ifndef ATBM_SDIO_BUS_BITS
#define ATBM_SDIO_BUS_BITS SDIO_WIFI_BIT
#endif

#define ATBM_WIFI_STA_IF_ID        0U
#define ATBM_WIFI_AP_IF_ID         1U
#define ATBM_WIFI_SCAN_BUFFER_SIZE 2732U
#define ATBM_WIFI_LOCK_FOREVER     0U

extern INT32S drvl1_sdio_config_bit_set(INT32U device_id, INT8U nrBit);
extern INT32S atbm_gp_sdio_init(INT32U device_id);
extern void drvl1_sdio_config_one_bit_intr_set(
	INT32U device_id,
	INT8U intr_type,
	INT8U intr_src);
extern int atbm_wifi_get_init_flag(void);
extern int atbm_wifi_p2p_go_stop(void);

typedef struct {
	atbm_mutex lock;
	ATBM_BOOL lock_ready;
	ATBM_BOOL sdio_ready;
	ATBM_BOOL lwip_ready;
	ATBM_BOOL driver_ready;
	ATBM_BOOL dhcps_ready;
	atbm_wifi_status_t status;
	atbm_wifi_event_callback_t callback;
	void *callback_user_data;
} atbm_wifi_interface_context_t;

static atbm_wifi_interface_context_t g_wifi = {
	.status = {
		ATBM_WIFI_MODE_NONE,
		ATBM_WIFI_STATE_STOPPED
	}
};

static unsigned int atbm_wifi_bounded_strlen(const char *text, unsigned int limit)
{
	unsigned int length;

	if(text == NULL)
	{
		return 0U;
	}

	for(length = 0U; length < limit; length++)
	{
		if(text[length] == '\0')
		{
			break;
		}
	}

	return length;
}

static int atbm_wifi_context_init(void)
{
	if(g_wifi.lock_ready == ATBM_FALSE)
	{
		if(atbm_os_mutexLockInit(&g_wifi.lock) != 0)
		{
			return ATBM_WIFI_ERR_NO_MEMORY;
		}
		g_wifi.lock_ready = ATBM_TRUE;
	}

	return ATBM_WIFI_OK;
}

static int atbm_wifi_lock(void)
{
	int ret = atbm_wifi_context_init();

	if(ret != ATBM_WIFI_OK)
	{
		return ret;
	}
	if(atbm_os_mutexLock(&g_wifi.lock, ATBM_WIFI_LOCK_FOREVER) != 0)
	{
		return ATBM_WIFI_ERR_INVALID_STATE;
	}

	return ATBM_WIFI_OK;
}

static void atbm_wifi_unlock(void)
{
	atbm_os_mutexUnLock(&g_wifi.lock);
}

static void atbm_wifi_emit(
	atbm_wifi_event_t event,
	const unsigned char *mac,
	int error)
{
	atbm_wifi_event_callback_t callback = g_wifi.callback;
	atbm_wifi_event_info_t info;

	if(callback == NULL)
	{
		return;
	}

	memset(&info, 0, sizeof(info));
	info.mode = g_wifi.status.mode;
	info.event = event;
	info.error = error;
	if(mac != NULL)
	{
		memcpy(info.mac, mac, ATBM_WIFI_MAC_ADDR_LEN);
	}

	callback(&info, g_wifi.callback_user_data);
}

static int atbm_wifi_validate_ssid(const char *ssid)
{
	unsigned int length = atbm_wifi_bounded_strlen(
		ssid,
		ATBM_WIFI_SSID_MAX_LEN + 1U);

	return (length >= 1U) && (length <= ATBM_WIFI_SSID_MAX_LEN);
}

static int atbm_wifi_validate_channel(int channel)
{
	return ((channel >= 1) && (channel <= 13)) ||
		((channel >= 36) && (channel <= 165));
}

static int atbm_wifi_validate_password(
	atbm_wifi_security_t security,
	const char *password)
{
	unsigned int length;

	if(security == ATBM_WIFI_SECURITY_OPEN)
	{
		return (password == NULL) || (password[0] == '\0');
	}
	if(password == NULL)
	{
		return 0;
	}

	length = atbm_wifi_bounded_strlen(
		password,
		ATBM_WIFI_PASSWORD_MAX_LEN + 1U);
	if(security == ATBM_WIFI_SECURITY_WEP)
	{
		return (length == 5U) || (length == 10U) ||
			(length == 13U) || (length == 26U);
	}

	return (length >= ATBM_WIFI_PASSWORD_MIN_LEN) &&
		(length <= ATBM_WIFI_PASSWORD_MAX_LEN);
}

static int atbm_wifi_validate_security(atbm_wifi_security_t security)
{
	return (security >= ATBM_WIFI_SECURITY_OPEN) &&
		(security <= ATBM_WIFI_SECURITY_WPA_WPA2_PSK);
}

static int atbm_wifi_map_security(
	atbm_wifi_security_t security,
	int *auth_mode,
	int *encryption)
{
	if((auth_mode == NULL) || (encryption == NULL) ||
		!atbm_wifi_validate_security(security))
	{
		return ATBM_WIFI_ERR_INVALID_ARG;
	}

	switch(security)
	{
	case ATBM_WIFI_SECURITY_OPEN:
		*auth_mode = WLAN_WPA_AUTH_DISABLED;
		*encryption = WLAN_ENCRYPT_NONE;
		break;
	case ATBM_WIFI_SECURITY_WEP:
		*auth_mode = WLAN_WPA_AUTH_DISABLED;
		*encryption = WLAN_ENCRYPT_WEP;
		break;
	case ATBM_WIFI_SECURITY_WPA_PSK:
		*auth_mode = WLAN_WPA_AUTH_PSK;
		*encryption = WLAN_ENCRYPT_TKIP;
		break;
	case ATBM_WIFI_SECURITY_WPA2_PSK:
		*auth_mode = WLAN_WPA2_AUTH_PSK;
		*encryption = WLAN_ENCRYPT_AES;
		break;
	case ATBM_WIFI_SECURITY_WPA_WPA2_PSK:
		*auth_mode = WLAN_MIX_AUTH_PSK;
		*encryption = WLAN_ENCRYPT_AES;
		break;
	default:
		return ATBM_WIFI_ERR_INVALID_ARG;
	}

	return ATBM_WIFI_OK;
}

static atbm_wifi_security_t atbm_wifi_map_scan_security(int security)
{
	switch(security)
	{
	case ATBM_KEY_WEP:
	case ATBM_KEY_WEP_SHARE:
		return ATBM_WIFI_SECURITY_WEP;
	case ATBM_KEY_WPA:
		return ATBM_WIFI_SECURITY_WPA_PSK;
	case ATBM_KEY_WPA2:
		return ATBM_WIFI_SECURITY_WPA2_PSK;
	case ATBM_KEY_MIX:
	case ATBM_KEY_SAE:
	case ATBM_KEY_SAE_COMPIT:
		return ATBM_WIFI_SECURITY_WPA_WPA2_PSK;
	case ATBM_KEY_NONE:
	default:
		return ATBM_WIFI_SECURITY_OPEN;
	}
}

static int atbm_wifi_raw_event_callback(
	int driver_mode,
	int event_id,
	void *parameter)
{
	atbm_wifi_event_t event;
	const unsigned char *mac = NULL;
	int deliver = 1;
	int is_ap_mode = atbmwifi_is_ap_mode(driver_mode);

	switch(event_id)
	{
	case ATBM_WIFI_SCANDONE_EVENT:
		event = ATBM_WIFI_EVENT_SCAN_DONE;
		break;
	case ATBM_WIFI_ASSOC_EVENT:
		if(is_ap_mode)
		{
			deliver = 0;
		}
		else
		{
			g_wifi.status.state = ATBM_WIFI_STATE_CONNECTED;
			event = ATBM_WIFI_EVENT_STA_CONNECTED;
		}
		break;
	case ATBM_WIFI_ASSOCRSP_TXOK_EVENT:
		event = ATBM_WIFI_EVENT_CLIENT_CONNECTED;
		mac = (const unsigned char *)parameter;
		break;
	case ATBM_WIFI_DEAUTH_EVENT:
	case ATBM_WIFI_DEASSOC_EVENT:
		if(is_ap_mode ||
			(g_wifi.status.mode == ATBM_WIFI_MODE_P2P_GO))
		{
			event = ATBM_WIFI_EVENT_CLIENT_DISCONNECTED;
			mac = (const unsigned char *)parameter;
		}
		else
		{
			g_wifi.status.state = ATBM_WIFI_STATE_STARTED;
			event = ATBM_WIFI_EVENT_STA_DISCONNECTED;
		}
		break;
	default:
		deliver = 0;
		break;
	}

	if(deliver)
	{
		atbm_wifi_emit(event, mac, ATBM_WIFI_OK);
	}

	return 0;
}

static int atbm_wifi_platform_init_locked(void)
{
	INT32S ret;

#if C_WIFI_ON_OFF_KEY
	gpio_write_io(WIFI_LDO_EN_PIN, DATA_HIGH);
	osDelay(50);
#endif

	if(g_wifi.sdio_ready == ATBM_FALSE)
	{
		drvl1_sdio_config_bit_set(
			SDIO_WIFI_DEVICE_ID,
			ATBM_SDIO_BUS_BITS);

#if (ATBM_SDIO_BUS_BITS == 1)
		drvl1_sdio_config_one_bit_intr_set(
			SDIO_WIFI_DEVICE_ID,
			EXTB,
			EXTB_IRQ_MUX3);
#endif

		ret = atbm_gp_sdio_init(SDIO_WIFI_DEVICE_ID);
		if(ret < 0)
		{
			return ATBM_WIFI_ERR_PLATFORM_INIT;
		}
		g_wifi.sdio_ready = ATBM_TRUE;
	}

	if(g_wifi.lwip_ready == ATBM_FALSE)
	{
		atbm_lwip_system_init();
		g_wifi.lwip_ready = ATBM_TRUE;
	}

	if(g_wifi.driver_ready == ATBM_FALSE)
	{
		ret = atbm_wifi_hw_init();
		if((ret < 0) || (atbm_wifi_get_init_flag() == 0))
		{
			return ATBM_WIFI_ERR_DRIVER_INIT;
		}
		g_wifi.driver_ready = ATBM_TRUE;
		atbmwifi_event_register_cb(atbm_wifi_raw_event_callback);
	}

	return ATBM_WIFI_OK;
}

int atbm_wifi_init(void)
{
	int ret;

	ret = atbm_wifi_lock();
	if(ret != ATBM_WIFI_OK)
	{
		return ret;
	}

	ret = atbm_wifi_platform_init_locked();
	if(ret == ATBM_WIFI_OK)
	{
		if(g_wifi.status.mode == ATBM_WIFI_MODE_NONE)
		{
			g_wifi.status.state = ATBM_WIFI_STATE_STOPPED;
		}
	}
	else
	{
		g_wifi.status.state = ATBM_WIFI_STATE_ERROR;
	}

	atbm_wifi_unlock();
	return ret;
}

static int atbm_wifi_network_server_start_locked(unsigned int if_id)
{
	struct netif *netif = atbm_wifi_netif_get_vif((atbm_uint8)if_id);

	if(netif == NULL)
	{
		return ATBM_WIFI_ERR_NETWORK_FAILED;
	}

	netif_set_default(netif);
	netif_set_link_up(netif);
	netif_set_up(netif);

	if(g_wifi.dhcps_ready == ATBM_FALSE)
	{
		dhcps_init(netif);
		g_wifi.dhcps_ready = ATBM_TRUE;
	}

	return ATBM_WIFI_OK;
}

static void atbm_wifi_network_server_stop_locked(void)
{
	if(g_wifi.dhcps_ready)
	{
		dhcps_deinit();
		g_wifi.dhcps_ready = ATBM_FALSE;
	}
}

static int atbm_wifi_active_if_id_locked(unsigned int *if_id)
{
	if(if_id == NULL)
	{
		return ATBM_WIFI_ERR_INVALID_ARG;
	}

	switch(g_wifi.status.mode)
	{
	case ATBM_WIFI_MODE_STA:
	case ATBM_WIFI_MODE_P2P_GO:
		*if_id = ATBM_WIFI_STA_IF_ID;
		return ATBM_WIFI_OK;
	case ATBM_WIFI_MODE_AP:
		*if_id = ATBM_WIFI_AP_IF_ID;
		return ATBM_WIFI_OK;
	case ATBM_WIFI_MODE_NONE:
	default:
		return ATBM_WIFI_ERR_NOT_READY;
	}
}

static int atbm_wifi_stop_current_locked(void)
{
	int ret = 0;

	switch(g_wifi.status.mode)
	{
	case ATBM_WIFI_MODE_STA:
		ret = atbm_wifi_sta_disjoin_ap();
		break;
	case ATBM_WIFI_MODE_AP:
		atbm_wifi_network_server_stop_locked();
		atbm_wifi_off(ATBM_WIFI_AP_IF_ID);
		break;
	case ATBM_WIFI_MODE_P2P_GO:
		atbm_wifi_network_server_stop_locked();
		ret = atbm_wifi_p2p_go_stop();
		if(atbm_wifi_p2p_stop() != 0)
		{
			ret = -1;
		}
		atbm_wifi_off(ATBM_WIFI_STA_IF_ID);
		break;
	case ATBM_WIFI_MODE_NONE:
	default:
		break;
	}

	g_wifi.status.mode = ATBM_WIFI_MODE_NONE;
	g_wifi.status.state = (ret == 0)
		? ATBM_WIFI_STATE_STOPPED
		: ATBM_WIFI_STATE_ERROR;

	return (ret == 0) ? ATBM_WIFI_OK : ATBM_WIFI_ERR_START_FAILED;
}

static int atbm_wifi_prepare_start_locked(atbm_wifi_interface_mode_t mode)
{
	int ret;

	if((g_wifi.sdio_ready == ATBM_FALSE) ||
		(g_wifi.lwip_ready == ATBM_FALSE) ||
		(g_wifi.driver_ready == ATBM_FALSE))
	{
		return ATBM_WIFI_ERR_NOT_READY;
	}

	if((g_wifi.status.mode == mode) &&
		((g_wifi.status.state == ATBM_WIFI_STATE_STARTED) ||
		 (g_wifi.status.state == ATBM_WIFI_STATE_CONNECTED)))
	{
		return 1;
	}

	if(g_wifi.status.mode != ATBM_WIFI_MODE_NONE)
	{
		ret = atbm_wifi_stop_current_locked();
		if(ret != ATBM_WIFI_OK)
		{
			return ret;
		}
	}

	g_wifi.status.mode = mode;
	g_wifi.status.state = ATBM_WIFI_STATE_STARTING;
	return ATBM_WIFI_OK;
}

int atbm_wifi_start_ap(const atbm_wifi_ap_config_t *config)
{
	int auth_mode;
	int encryption;
	int ret;
	const char *password;

	if((config == NULL) ||
		!atbm_wifi_validate_ssid(config->ssid) ||
		!atbm_wifi_validate_channel(config->channel) ||
		!atbm_wifi_validate_security(config->security) ||
		!atbm_wifi_validate_password(config->security, config->password))
	{
		return ATBM_WIFI_ERR_INVALID_ARG;
	}
	if(atbm_wifi_map_security(
		config->security,
		&auth_mode,
		&encryption) != ATBM_WIFI_OK)
	{
		return ATBM_WIFI_ERR_INVALID_ARG;
	}

	ret = atbm_wifi_lock();
	if(ret != ATBM_WIFI_OK)
	{
		return ret;
	}

	ret = atbm_wifi_prepare_start_locked(ATBM_WIFI_MODE_AP);
	if(ret == 1)
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_OK;
	}
	if(ret != ATBM_WIFI_OK)
	{
		atbm_wifi_unlock();
		atbm_wifi_emit(ATBM_WIFI_EVENT_ERROR, NULL, ret);
		return ret;
	}

	password = (config->password != NULL) ? config->password : "";
	if(atbm_wifi_on(ATBM_WIFI_AP_MODE) == NULL)
	{
		ret = ATBM_WIFI_ERR_START_FAILED;
		goto fail;
	}

	if(atbm_wifi_ap_create(
		(char *)config->ssid,
		auth_mode,
		encryption,
		(char *)password,
		config->channel,
		config->hidden ? ATBM_TRUE : ATBM_FALSE) < 0)
	{
		ret = ATBM_WIFI_ERR_START_FAILED;
		goto fail_off;
	}

	ret = atbm_wifi_network_server_start_locked(ATBM_WIFI_AP_IF_ID);
	if(ret != ATBM_WIFI_OK)
	{
		goto fail_off;
	}

	g_wifi.status.state = ATBM_WIFI_STATE_STARTED;
	atbm_wifi_unlock();
	atbm_wifi_emit(ATBM_WIFI_EVENT_STARTED, NULL, ATBM_WIFI_OK);
	return ATBM_WIFI_OK;

fail_off:
	atbm_wifi_network_server_stop_locked();
	atbm_wifi_off(ATBM_WIFI_AP_IF_ID);
fail:
	g_wifi.status.state = ATBM_WIFI_STATE_ERROR;
	atbm_wifi_unlock();
	atbm_wifi_emit(ATBM_WIFI_EVENT_ERROR, NULL, ret);
	return ret;
}

int atbm_wifi_stop_ap(void)
{
	int ret;

	ret = atbm_wifi_lock();
	if(ret != ATBM_WIFI_OK)
	{
		return ret;
	}
	if(g_wifi.status.mode == ATBM_WIFI_MODE_NONE)
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_OK;
	}
	if(g_wifi.status.mode != ATBM_WIFI_MODE_AP)
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_ERR_INVALID_STATE;
	}

	ret = atbm_wifi_stop_current_locked();
	atbm_wifi_unlock();
	if(ret == ATBM_WIFI_OK)
	{
		atbm_wifi_emit(ATBM_WIFI_EVENT_STOPPED, NULL, ATBM_WIFI_OK);
	}
	return ret;
}

int atbm_wifi_start_sta(const atbm_wifi_sta_config_t *config)
{
	int auth_mode;
	int encryption;
	int ret;
	const char *password;

	if((config == NULL) ||
		!atbm_wifi_validate_ssid(config->ssid) ||
		!atbm_wifi_validate_security(config->security) ||
		!atbm_wifi_validate_password(config->security, config->password))
	{
		return ATBM_WIFI_ERR_INVALID_ARG;
	}
	if(atbm_wifi_map_security(
		config->security,
		&auth_mode,
		&encryption) != ATBM_WIFI_OK)
	{
		return ATBM_WIFI_ERR_INVALID_ARG;
	}

	ret = atbm_wifi_lock();
	if(ret != ATBM_WIFI_OK)
	{
		return ret;
	}

	ret = atbm_wifi_prepare_start_locked(ATBM_WIFI_MODE_STA);
	if(ret == 1)
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_OK;
	}
	if(ret != ATBM_WIFI_OK)
	{
		atbm_wifi_unlock();
		atbm_wifi_emit(ATBM_WIFI_EVENT_ERROR, NULL, ret);
		return ret;
	}

	password = (config->password != NULL) ? config->password : "";
	if(atbm_wifi_on(ATBM_WIFI_STA_MODE) == NULL)
	{
		ret = ATBM_WIFI_ERR_START_FAILED;
		goto fail;
	}

	if(atbm_wifi_sta_join_ap(
		(char *)config->ssid,
		(char *)config->bssid,
		auth_mode,
		encryption,
		(char *)password) < 0)
	{
		atbm_wifi_off(ATBM_WIFI_STA_IF_ID);
		ret = ATBM_WIFI_ERR_START_FAILED;
		goto fail;
	}

	g_wifi.status.state = ATBM_WIFI_STATE_STARTED;
	atbm_wifi_unlock();
	atbm_wifi_emit(ATBM_WIFI_EVENT_STARTED, NULL, ATBM_WIFI_OK);
	return ATBM_WIFI_OK;

fail:
	g_wifi.status.state = ATBM_WIFI_STATE_ERROR;
	atbm_wifi_unlock();
	atbm_wifi_emit(ATBM_WIFI_EVENT_ERROR, NULL, ret);
	return ret;
}

int atbm_wifi_stop_sta(void)
{
	int ret;

	ret = atbm_wifi_lock();
	if(ret != ATBM_WIFI_OK)
	{
		return ret;
	}
	if(g_wifi.status.mode == ATBM_WIFI_MODE_NONE)
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_OK;
	}
	if(g_wifi.status.mode != ATBM_WIFI_MODE_STA)
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_ERR_INVALID_STATE;
	}

	ret = atbm_wifi_stop_current_locked();
	atbm_wifi_unlock();
	if(ret == ATBM_WIFI_OK)
	{
		atbm_wifi_emit(ATBM_WIFI_EVENT_STOPPED, NULL, ATBM_WIFI_OK);
	}
	return ret;
}

int atbm_wifi_start_p2p(const atbm_wifi_p2p_config_t *config)
{
	int ret;
	int p2p_configured = 0;
	int p2p_started = 0;
	int go_started = 0;

	if((config == NULL) ||
		!atbm_wifi_validate_ssid(config->device_name) ||
		!atbm_wifi_validate_ssid(config->ssid) ||
		!atbm_wifi_validate_channel(config->channel) ||
		!atbm_wifi_validate_password(
			ATBM_WIFI_SECURITY_WPA2_PSK,
			config->password))
	{
		return ATBM_WIFI_ERR_INVALID_ARG;
	}

	ret = atbm_wifi_lock();
	if(ret != ATBM_WIFI_OK)
	{
		return ret;
	}

	ret = atbm_wifi_prepare_start_locked(ATBM_WIFI_MODE_P2P_GO);
	if(ret == 1)
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_OK;
	}
	if(ret != ATBM_WIFI_OK)
	{
		atbm_wifi_unlock();
		atbm_wifi_emit(ATBM_WIFI_EVENT_ERROR, NULL, ret);
		return ret;
	}

	ret = atbm_wifi_p2p_user_config(
		(char *)config->device_name,
		(char *)config->ssid,
		(char *)config->password,
		config->channel);
	if(ret == 0)
	{
		p2p_configured = 1;
	}
	if(ret == 0)
	{
		ret = atbm_wifi_p2p_start();
		if(ret == 0)
		{
			p2p_started = 1;
		}
	}
	if(ret == 0)
	{
		ret = atbm_wifi_p2p_go_start();
		if(ret == 0)
		{
			go_started = 1;
		}
	}
	if(ret != 0)
	{
		ret = ATBM_WIFI_ERR_START_FAILED;
		goto fail;
	}

	ret = atbm_wifi_network_server_start_locked(ATBM_WIFI_STA_IF_ID);
	if(ret != ATBM_WIFI_OK)
	{
		goto fail;
	}

	g_wifi.status.state = ATBM_WIFI_STATE_STARTED;
	atbm_wifi_unlock();
	atbm_wifi_emit(ATBM_WIFI_EVENT_STARTED, NULL, ATBM_WIFI_OK);
	return ATBM_WIFI_OK;

fail:
	atbm_wifi_network_server_stop_locked();
	if(go_started)
	{
		atbm_wifi_p2p_go_stop();
	}
	if(p2p_started || p2p_configured)
	{
		atbm_wifi_p2p_stop();
	}
	atbm_wifi_off(ATBM_WIFI_STA_IF_ID);
	g_wifi.status.state = ATBM_WIFI_STATE_ERROR;
	atbm_wifi_unlock();
	atbm_wifi_emit(ATBM_WIFI_EVENT_ERROR, NULL, ret);
	return ret;
}

int atbm_wifi_stop_p2p(void)
{
	int ret;

	ret = atbm_wifi_lock();
	if(ret != ATBM_WIFI_OK)
	{
		return ret;
	}
	if(g_wifi.status.mode == ATBM_WIFI_MODE_NONE)
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_OK;
	}
	if(g_wifi.status.mode != ATBM_WIFI_MODE_P2P_GO)
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_ERR_INVALID_STATE;
	}

	ret = atbm_wifi_stop_current_locked();
	atbm_wifi_unlock();
	if(ret == ATBM_WIFI_OK)
	{
		atbm_wifi_emit(ATBM_WIFI_EVENT_STOPPED, NULL, ATBM_WIFI_OK);
	}
	return ret;
}

int atbm_wifi_scan(
	atbm_wifi_scan_result_t *results,
	unsigned int max_results,
	unsigned int *result_count)
{
	char *buffer;
	WLAN_SCAN_RESULT *raw_result;
	WLAN_BSS_INFO *raw_bss;
	unsigned int raw_capacity;
	unsigned int count;
	unsigned int index;
	unsigned int ssid_length;
	int ret;

	if((results == NULL) || (max_results == 0U) || (result_count == NULL))
	{
		return ATBM_WIFI_ERR_INVALID_ARG;
	}
	*result_count = 0U;

	ret = atbm_wifi_lock();
	if(ret != ATBM_WIFI_OK)
	{
		return ret;
	}
	if((g_wifi.driver_ready == ATBM_FALSE) ||
		(g_wifi.status.mode == ATBM_WIFI_MODE_NONE))
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_ERR_NOT_READY;
	}

	buffer = (char *)atbm_kzalloc(
		ATBM_WIFI_SCAN_BUFFER_SIZE,
		GFP_KERNEL);
	if(buffer == NULL)
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_ERR_NO_MEMORY;
	}

	ret = atbm_wifi_scan_network(buffer, ATBM_WIFI_SCAN_BUFFER_SIZE);
	if(ret < 0)
	{
		atbm_kfree(buffer);
		atbm_wifi_unlock();
		return ATBM_WIFI_ERR_START_FAILED;
	}

	raw_result = (WLAN_SCAN_RESULT *)buffer;
	raw_capacity = (ATBM_WIFI_SCAN_BUFFER_SIZE -
		(unsigned int)offsetof(WLAN_SCAN_RESULT, bss_info)) /
		(unsigned int)sizeof(WLAN_BSS_INFO);
	count = raw_result->count;
	if(count > raw_capacity)
	{
		count = raw_capacity;
	}
	if(count > max_results)
	{
		count = max_results;
	}

	raw_bss = raw_result->bss_info;
	for(index = 0U; index < count; index++)
	{
		memset(&results[index], 0, sizeof(results[index]));
		memcpy(
			results[index].bssid,
			raw_bss[index].BSSID,
			ATBM_WIFI_MAC_ADDR_LEN);
		ssid_length = raw_bss[index].SSID_len;
		if(ssid_length > ATBM_WIFI_SSID_MAX_LEN)
		{
			ssid_length = ATBM_WIFI_SSID_MAX_LEN;
		}
		memcpy(
			results[index].ssid,
			raw_bss[index].SSID,
			ssid_length);
		results[index].ssid[ssid_length] = '\0';
		results[index].channel = raw_bss[index].chanspec;
		results[index].rssi = raw_bss[index].RSSI;
		results[index].security =
			atbm_wifi_map_scan_security(raw_bss[index].security);
	}

	*result_count = count;
	atbm_kfree(buffer);
	atbm_wifi_unlock();
	atbm_wifi_emit(ATBM_WIFI_EVENT_SCAN_DONE, NULL, ATBM_WIFI_OK);
	return ATBM_WIFI_OK;
}

int atbm_wifi_get_status(atbm_wifi_status_t *status)
{
	int ret;

	if(status == NULL)
	{
		return ATBM_WIFI_ERR_INVALID_ARG;
	}

	ret = atbm_wifi_lock();
	if(ret != ATBM_WIFI_OK)
	{
		return ret;
	}
	*status = g_wifi.status;
	atbm_wifi_unlock();
	return ATBM_WIFI_OK;
}

int atbm_wifi_get_mac(unsigned char mac[ATBM_WIFI_MAC_ADDR_LEN])
{
	int ret;

	if(mac == NULL)
	{
		return ATBM_WIFI_ERR_INVALID_ARG;
	}

	ret = atbm_wifi_lock();
	if(ret != ATBM_WIFI_OK)
	{
		return ret;
	}
	if(g_wifi.driver_ready == ATBM_FALSE)
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_ERR_NOT_READY;
	}

	ret = atbm_wifi_get_mac_address(mac);
	atbm_wifi_unlock();
	return (ret == 0) ? ATBM_WIFI_OK : ATBM_WIFI_ERR_NOT_READY;
}

int atbm_wifi_is_connected(int *connected)
{
	int ret;

	if(connected == NULL)
	{
		return ATBM_WIFI_ERR_INVALID_ARG;
	}
	*connected = 0;

	ret = atbm_wifi_lock();
	if(ret != ATBM_WIFI_OK)
	{
		return ret;
	}
	if(g_wifi.driver_ready == ATBM_FALSE)
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_ERR_NOT_READY;
	}
	if(g_wifi.status.mode != ATBM_WIFI_MODE_STA)
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_ERR_INVALID_STATE;
	}

	*connected = atbm_wifi_isconnected(ATBM_WIFI_STA_IF_ID) ? 1 : 0;
	atbm_wifi_unlock();
	return ATBM_WIFI_OK;
}

int atbm_wifi_get_connection_info(atbm_wifi_connection_info_t *info)
{
	ATBM_WLAN_CONNECTION_INFO raw;
	unsigned int ssid_length;
	int ret;

	if(info == NULL)
	{
		return ATBM_WIFI_ERR_INVALID_ARG;
	}
	memset(info, 0, sizeof(*info));

	ret = atbm_wifi_lock();
	if(ret != ATBM_WIFI_OK)
	{
		return ret;
	}
	if((g_wifi.driver_ready == ATBM_FALSE) ||
		(g_wifi.status.mode != ATBM_WIFI_MODE_STA) ||
		!atbm_wifi_isconnected(ATBM_WIFI_STA_IF_ID))
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_ERR_NOT_READY;
	}

	memset(&raw, 0, sizeof(raw));
	ret = atbm_wifi_get_connected_info_vif(
		ATBM_WIFI_STA_IF_ID,
		&raw);
	if(ret != 0)
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_ERR_NOT_READY;
	}

	ssid_length = raw.Ssid_len;
	if(ssid_length > ATBM_WIFI_SSID_MAX_LEN)
	{
		ssid_length = ATBM_WIFI_SSID_MAX_LEN;
	}
	memcpy(info->ssid, raw.Ssid, ssid_length);
	info->ssid[ssid_length] = '\0';
	memcpy(info->bssid, raw.bssid, ATBM_WIFI_MAC_ADDR_LEN);
	info->channel = raw.channel;
	info->rssi = raw.Rssi;
	info->phy_rate = raw.Phy_rate;
	atbm_wifi_unlock();
	return ATBM_WIFI_OK;
}

int atbm_wifi_get_rssi(int *rssi)
{
	atbm_wifi_connection_info_t info;
	int ret;

	if(rssi == NULL)
	{
		return ATBM_WIFI_ERR_INVALID_ARG;
	}

	ret = atbm_wifi_get_connection_info(&info);
	if(ret == ATBM_WIFI_OK)
	{
		*rssi = info.rssi;
	}
	return ret;
}

int atbm_wifi_get_ip_info(atbm_wifi_ip_info_t *info)
{
	const ip4_addr_t *address;
	struct netif *netif;
	unsigned int if_id;
	int ret;

	if(info == NULL)
	{
		return ATBM_WIFI_ERR_INVALID_ARG;
	}
	memset(info, 0, sizeof(*info));

	ret = atbm_wifi_lock();
	if(ret != ATBM_WIFI_OK)
	{
		return ret;
	}
	if(g_wifi.driver_ready == ATBM_FALSE)
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_ERR_NOT_READY;
	}

	ret = atbm_wifi_active_if_id_locked(&if_id);
	if(ret != ATBM_WIFI_OK)
	{
		atbm_wifi_unlock();
		return ret;
	}
	netif = atbm_wifi_netif_get_vif((atbm_uint8)if_id);
	if(netif == NULL)
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_ERR_NOT_READY;
	}

	address = netif_ip4_addr(netif);
	info->ip[0] = ip4_addr1(address);
	info->ip[1] = ip4_addr2(address);
	info->ip[2] = ip4_addr3(address);
	info->ip[3] = ip4_addr4(address);
	address = netif_ip4_netmask(netif);
	info->netmask[0] = ip4_addr1(address);
	info->netmask[1] = ip4_addr2(address);
	info->netmask[2] = ip4_addr3(address);
	info->netmask[3] = ip4_addr4(address);
	address = netif_ip4_gw(netif);
	info->gateway[0] = ip4_addr1(address);
	info->gateway[1] = ip4_addr2(address);
	info->gateway[2] = ip4_addr3(address);
	info->gateway[3] = ip4_addr4(address);
	atbm_wifi_unlock();
	return ATBM_WIFI_OK;
}

int atbm_wifi_get_clients(atbm_wifi_client_info_t *clients,
	unsigned int max_clients,
	unsigned int *client_count)
{
	WLAN_MACLIST raw;
	unsigned int if_id;
	unsigned int index;
	int rssi;
	int ret;

	if((clients == NULL) || (max_clients == 0U) ||
		(client_count == NULL))
	{
		return ATBM_WIFI_ERR_INVALID_ARG;
	}
	*client_count = 0U;

	ret = atbm_wifi_lock();
	if(ret != ATBM_WIFI_OK)
	{
		return ret;
	}
	if((g_wifi.driver_ready == ATBM_FALSE) ||
		((g_wifi.status.mode != ATBM_WIFI_MODE_AP) &&
		 (g_wifi.status.mode != ATBM_WIFI_MODE_P2P_GO)))
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_ERR_INVALID_STATE;
	}

	ret = atbm_wifi_active_if_id_locked(&if_id);
	if(ret != ATBM_WIFI_OK)
	{
		atbm_wifi_unlock();
		return ret;
	}
	memset(&raw, 0, sizeof(raw));
	raw.count = (max_clients < ATBM_WIFI_MAX_STA_IN_AP_MODE)
		? max_clients
		: ATBM_WIFI_MAX_STA_IN_AP_MODE;
	ret = atbm_wifi_get_associated_client_list_vif(
		(atbm_uint8)if_id,
		&raw,
		sizeof(raw));
	if(ret != 0)
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_ERR_NOT_READY;
	}

	for(index = 0U; index < raw.count; index++)
	{
		memset(&clients[index], 0, sizeof(clients[index]));
		memcpy(clients[index].mac, raw.ea[index].mac,
			ATBM_WIFI_MAC_ADDR_LEN);
		rssi = 100;
		atbm_wifi_get_client_rssi_vif(
			(atbm_uint8)if_id,
			raw.ea[index].mac,
			&rssi);
		clients[index].rssi = rssi;
	}

	*client_count = raw.count;
	atbm_wifi_unlock();
	return ATBM_WIFI_OK;
}

int atbm_wifi_disconnect_client(
	const unsigned char mac[ATBM_WIFI_MAC_ADDR_LEN])
{
	unsigned int if_id;
	int ret;

	if(mac == NULL)
	{
		return ATBM_WIFI_ERR_INVALID_ARG;
	}

	ret = atbm_wifi_lock();
	if(ret != ATBM_WIFI_OK)
	{
		return ret;
	}
	if((g_wifi.driver_ready == ATBM_FALSE) ||
		((g_wifi.status.mode != ATBM_WIFI_MODE_AP) &&
		 (g_wifi.status.mode != ATBM_WIFI_MODE_P2P_GO)))
	{
		atbm_wifi_unlock();
		return ATBM_WIFI_ERR_INVALID_STATE;
	}

	ret = atbm_wifi_active_if_id_locked(&if_id);
	if(ret == ATBM_WIFI_OK)
	{
		ret = atbm_wifi_disconnect_client_vif(
			(atbm_uint8)if_id,
			mac);
	}
	atbm_wifi_unlock();
	return (ret == 0) ? ATBM_WIFI_OK : ATBM_WIFI_ERR_NOT_READY;
}

int atbm_wifi_register_event_callback(
	atbm_wifi_event_callback_t callback,
	void *user_data)
{
	int ret = atbm_wifi_lock();

	if(ret != ATBM_WIFI_OK)
	{
		return ret;
	}
	g_wifi.callback = callback;
	g_wifi.callback_user_data = user_data;
	if(g_wifi.driver_ready)
	{
		atbmwifi_event_register_cb(atbm_wifi_raw_event_callback);
	}
	atbm_wifi_unlock();
	return ATBM_WIFI_OK;
}

int atbm_wifi_deinit(void)
{
	int ret;
	int stop_ret;

	ret = atbm_wifi_lock();
	if(ret != ATBM_WIFI_OK)
	{
		return ret;
	}

	stop_ret = atbm_wifi_stop_current_locked();
	if(g_wifi.driver_ready)
	{
		if(atbm_wifi_hw_deinit() < 0)
		{
			stop_ret = ATBM_WIFI_ERR_DRIVER_INIT;
		}
		g_wifi.driver_ready = ATBM_FALSE;
	}
	g_wifi.status.mode = ATBM_WIFI_MODE_NONE;
	g_wifi.status.state = (stop_ret == ATBM_WIFI_OK)
		? ATBM_WIFI_STATE_STOPPED
		: ATBM_WIFI_STATE_ERROR;
	atbm_wifi_unlock();

#if C_WIFI_ON_OFF_KEY
	gpio_write_io(WIFI_LDO_EN_PIN, DATA_LOW);
#endif

	if(stop_ret == ATBM_WIFI_OK)
	{
		atbm_wifi_emit(ATBM_WIFI_EVENT_STOPPED, NULL, ATBM_WIFI_OK);
	}
	else
	{
		atbm_wifi_emit(ATBM_WIFI_EVENT_ERROR, NULL, stop_ret);
	}
	return stop_ret;
}

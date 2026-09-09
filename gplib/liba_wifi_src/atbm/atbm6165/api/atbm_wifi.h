#ifndef ATBM_WIFI_H
#define ATBM_WIFI_H

#ifdef __cplusplus
extern "C" {
#endif

#define ATBM_WIFI_SSID_MAX_LEN          32U
#define ATBM_WIFI_PASSWORD_MIN_LEN       8U
#define ATBM_WIFI_PASSWORD_MAX_LEN      63U
#define ATBM_WIFI_MAC_ADDR_LEN           6U

typedef enum {
	ATBM_WIFI_SECURITY_OPEN = 0,
	ATBM_WIFI_SECURITY_WEP,
	ATBM_WIFI_SECURITY_WPA_PSK,
	ATBM_WIFI_SECURITY_WPA2_PSK,
	ATBM_WIFI_SECURITY_WPA_WPA2_PSK
} atbm_wifi_security_t;

typedef enum {
	ATBM_WIFI_MODE_NONE = 0,
	ATBM_WIFI_MODE_STA,
	ATBM_WIFI_MODE_AP,
	ATBM_WIFI_MODE_P2P_GO
} atbm_wifi_interface_mode_t;

typedef enum {
	ATBM_WIFI_STATE_STOPPED = 0,
	ATBM_WIFI_STATE_STARTING,
	ATBM_WIFI_STATE_STARTED,
	ATBM_WIFI_STATE_CONNECTED,
	ATBM_WIFI_STATE_ERROR
} atbm_wifi_state_t;

typedef enum {
	ATBM_WIFI_OK = 0,
	ATBM_WIFI_ERR_INVALID_ARG = -1,
	ATBM_WIFI_ERR_INVALID_STATE = -2,
	ATBM_WIFI_ERR_PLATFORM_INIT = -3,
	ATBM_WIFI_ERR_DRIVER_INIT = -4,
	ATBM_WIFI_ERR_START_FAILED = -5,
	ATBM_WIFI_ERR_NETWORK_FAILED = -6,
	ATBM_WIFI_ERR_NO_MEMORY = -7,
	ATBM_WIFI_ERR_NOT_READY = -8
} atbm_wifi_error_t;

typedef struct {
	const char *ssid;
	const char *password;
	int channel;
	atbm_wifi_security_t security;
	int hidden;
} atbm_wifi_ap_config_t;

typedef struct {
	const char *ssid;
	const char *password;
	const unsigned char *bssid;
	atbm_wifi_security_t security;
} atbm_wifi_sta_config_t;

typedef struct {
	const char *device_name;
	const char *ssid;
	const char *password;
	int channel;
} atbm_wifi_p2p_config_t;

typedef struct {
	unsigned char bssid[ATBM_WIFI_MAC_ADDR_LEN];
	char ssid[ATBM_WIFI_SSID_MAX_LEN + 1U];
	int channel;
	int rssi;
	atbm_wifi_security_t security;
} atbm_wifi_scan_result_t;

typedef struct {
	atbm_wifi_interface_mode_t mode;
	atbm_wifi_state_t state;
} atbm_wifi_status_t;

typedef struct {
	char ssid[ATBM_WIFI_SSID_MAX_LEN + 1U];
	unsigned char bssid[ATBM_WIFI_MAC_ADDR_LEN];
	int channel;
	int rssi;
	int phy_rate;
} atbm_wifi_connection_info_t;

typedef struct {
	unsigned char ip[4];
	unsigned char netmask[4];
	unsigned char gateway[4];
} atbm_wifi_ip_info_t;

typedef struct {
	unsigned char mac[ATBM_WIFI_MAC_ADDR_LEN];
	int rssi;
} atbm_wifi_client_info_t;

typedef enum {
	ATBM_WIFI_EVENT_STARTED = 0,
	ATBM_WIFI_EVENT_STOPPED,
	ATBM_WIFI_EVENT_STA_CONNECTED,
	ATBM_WIFI_EVENT_STA_DISCONNECTED,
	ATBM_WIFI_EVENT_CLIENT_CONNECTED,
	ATBM_WIFI_EVENT_CLIENT_DISCONNECTED,
	ATBM_WIFI_EVENT_SCAN_DONE,
	ATBM_WIFI_EVENT_ERROR
} atbm_wifi_event_t;

typedef struct {
	atbm_wifi_interface_mode_t mode;
	atbm_wifi_event_t event;
	unsigned char mac[ATBM_WIFI_MAC_ADDR_LEN];
	int error;
} atbm_wifi_event_info_t;

typedef void (*atbm_wifi_event_callback_t)(
	const atbm_wifi_event_info_t *event,
	void *user_data);

int atbm_wifi_init(void);

int atbm_wifi_start_ap(const atbm_wifi_ap_config_t *config);
int atbm_wifi_stop_ap(void);

int atbm_wifi_start_sta(const atbm_wifi_sta_config_t *config);
int atbm_wifi_stop_sta(void);

int atbm_wifi_start_p2p(const atbm_wifi_p2p_config_t *config);
int atbm_wifi_stop_p2p(void);

int atbm_wifi_scan(atbm_wifi_scan_result_t *results,
	unsigned int max_results,
	unsigned int *result_count);

int atbm_wifi_get_status(atbm_wifi_status_t *status);
int atbm_wifi_get_mac(unsigned char mac[ATBM_WIFI_MAC_ADDR_LEN]);

int atbm_wifi_is_connected(int *connected);
int atbm_wifi_get_connection_info(atbm_wifi_connection_info_t *info);
int atbm_wifi_get_rssi(int *rssi);
int atbm_wifi_get_ip_info(atbm_wifi_ip_info_t *info);

int atbm_wifi_get_clients(atbm_wifi_client_info_t *clients,
	unsigned int max_clients,
	unsigned int *client_count);
int atbm_wifi_disconnect_client(
	const unsigned char mac[ATBM_WIFI_MAC_ADDR_LEN]);

int atbm_wifi_register_event_callback(
	atbm_wifi_event_callback_t callback,
	void *user_data);

int atbm_wifi_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* ATBM_WIFI_H */

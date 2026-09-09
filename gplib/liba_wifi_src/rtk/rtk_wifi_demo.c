
#define RTK_WIFI_DEMO 1

#if (RTK_WIFI_DEMO == 1)
#include "main.h"
#include "board_config.h"
#include "socket_cmd.h"
#include "gplib_print_string.h"
#include "dhcp/dhcps.h"
#include "ap_state_config.h"
#include "autoconf.h"

//#define NET_IF_NUM 2
#define CONFIG_INIT_NET                 1
#define CONFIG_WEP                      0
#define CONFIG_WPA                      0
#define CONFIG_WPA2                     0
#define CONFIG_PING_TEST                0
#define CONFIG_INTERACTIVE_MODE         1
#define CONFIG_POST_INIT                0
#define CONFIG_START_MP                 0
#define CONFIG_START_STA                0
#define CONFIG_START_AP                 1
#define CONFIG_START_STA_AP             0
#define CONFIG_DHCP_SERVER              1
#define CONFIG_DHCP_CLIENT              0
#define CONFIG_WEB_SERVER               0
#define CONFIG_AUTOCHL_SELECT           0 //auto channel select

extern struct netif  xnetif[NET_IF_NUM];
extern SDIO_BUS_OPS rtw_sdio_bus_ops;
extern char* rtw_initmac;
#endif//#if (RTK_WIFI_DEMO == 1)

unsigned int g_ap_sta_num = AP_STA_NUM;
int sdio_host_device_id = 1;
int g_wifi_init = 0;
int g_wifi_logservice = 0;

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
static INT8U  wifi_ssid[GPSOCK_WiFi_Name_Length];
static INT8U  wifi_ssid_len = 0;
static INT8U  wifi_pw[GPSOCK_WiFi_Passwd_Length];
static INT8U  wifi_pw_len	 = 0;
#endif

extern int wifi_autochl_select(unsigned char *channel_set, unsigned char channel_num);
int wifi_module_autochl_select(int low_chl, int hi_chl)
{
#if (WIFI_VENDOR == 1)
#elif (WIFI_VENDOR == 2)
	#if 0
    int autoChannel = -1;
	int i;
	unsigned char channel_set[11];
	
	for (i = 0; i < 11; i++) {
		channel_set[i] = i + 1;
	}
	autoChannel = wifi_autochl_select(channel_set, 11);

    return autoChannel;
	#else
    int autoChannel = -1;
	int i;
	unsigned char channel_set[11] = {0};
	
	for (i = 0; i <= (hi_chl-low_chl); i++) {
		channel_set[i] = low_chl+i;
	}
	autoChannel = wifi_autochl_select(channel_set, (hi_chl-low_chl+1));

    return autoChannel;	
	#endif
#endif
}

INT8U RTSP_wait_flag = 0;
extern INT32U RTSP_run_flag;
extern int lwip_clean_tcpPcbs();
INT32S rtk_wifi_module_enable(INT32U device_id, INT8U pinmux, char *ap_name, char *ap_passwd, INT32U channel, INT8U sta_mode)
{//8189FTV
    //int ret;


  /*    // josephhsieh@20240307
    while(RTSP_wait_flag == 1)
    {
        if(RTSP_run_flag == 0)
        {
            lwip_clean_tcpPcbs();
            RTSP_wait_flag = 0;
            break;
        }
        osDelay(100);
        DBG_PRINT("wait RTSP exit\r\n");
    }
	*/

	// wext_set_adaptivity(RTW_ADAPTIVITY_NORMAL);   // ©wÀW´ú¸Õ

    if(g_wifi_init == 0)
    {
        //sdio init
        sdio_host_device_id = device_id;

        if(rtw_sdio_bus_ops.bus_probe()<0){
            print_string("ERROR: sdio init fail\r\n");
            return -1;
        }

        /* Initilaize the LwIP stack */
        LwIP_Init();
        wifi_manager_init();

        print_string("\n\rwifi init sdio/lwip done");
        g_wifi_init = 1;
    }

    ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gw;
    struct netif * pnetif = &xnetif[0];

    IP4_ADDR(&ipaddr, GP_AUTH_IP_ADDR0, GP_AUTH_IP_ADDR1, GP_AUTH_IP_ADDR2, GP_AUTH_IP_ADDR3);
    IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
    IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
    netif_set_addr(pnetif, &ipaddr, &netmask,&gw);

	DBG_PRINT("GW:%d.%d.%d.%d\r\n", GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);

	wifi_off();
	vTaskDelay(20);

	if(sta_mode){
		if (wifi_on(RTW_MODE_STA) < 0)
		{
			print_string("\n\rERROR: Wifi on failed!");
			return -2;
		}
	}
	else{
		if (wifi_on(RTW_MODE_AP) < 0){
			print_string("\n\rERROR: Wifi on failed!");
			return -2;
		}
		//Add mac after orig ssid
		if(strlen(ap_name) < (30-3)){
			uint8_t *mac1 = LwIP_GetMAC(&xnetif[0]);
			char const* const ssidFmt ="%s%02x%02x%02x";
			char ssid[30];
			memset(&ssid, 0,sizeof(ssid));
			snprintf(ssid, 30, ssidFmt,ap_name, mac1[3], mac1[4], mac1[5]);
			ap_name = (char *)&ssid; //instead orig ssid
            #if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
			ap_state_wifi_ssid_set((INT8U*)&ssid, sizeof(ssid));
			#endif
		}

		#if CONFIG_AUTOCHL_SELECT
		int autoChannel = wifi_module_autochl_select(2,6); //(int low_chl, int hi_chl)
		print_string("autochl_select %d\r\n",autoChannel);
		channel = autoChannel? autoChannel:channel;
		#endif

		print_string("\n\rStarting AP ...");
	}

	if(sta_mode){
		INT8U *tmp;
		print_string("\n\rStarting STATION ...\r\n");
		print_string("ap_name: %s\r\n",ap_name);
		/*ret = */wifi_connect((char*)ap_name,
						RTW_SECURITY_WPA2_AES_PSK,
						(char*)ap_passwd,
						strlen(ap_name),
						strlen(ap_passwd),
						0,
						NULL);
		//if(ret == RTW_SUCCESS) {
			//struct netif *pxnetif = &xnetif[0];

			LwIP_DHCP(0, DHCP_START);
			tmp = LwIP_GetMAC(&xnetif[0]);
			print_string("STA  MAC => %02x:%02x:%02x:%02x:%02x:%02x\r\n", tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5]);
			tmp = LwIP_GetIP(&xnetif[0]);
			print_string("STA IP  => %d.%d.%d.%d\r\n", tmp[0], tmp[1], tmp[2], tmp[3]);
			tmp = LwIP_GetGW(&xnetif[0]);
			print_string("STA GW  => %d.%d.%d.%d\r\n", tmp[0], tmp[1], tmp[2], tmp[3]);
			tmp = LwIP_GetMASK(&xnetif[0]);
			print_string("STA MASK=> %d.%d.%d.%d\r\n", tmp[0], tmp[1], tmp[2], tmp[3]);

			//tmp = LwIP_GetBC(&xnetif[0]);
			//print_string("STA BC  => %d.%d.%d.%d\r\n", tmp[0], tmp[1], tmp[2], tmp[3]);
			//ip_addr_t ip_addr_tmp;
			//LwIP_GetDNS(&ip_addr_tmp);

			//tmp = (INT8U *)&ip_addr_tmp.addr;
			//print_string("STA DNS => %d.%d.%d.%d\r\n", tmp[0], tmp[1], tmp[2], tmp[3]);
		//}
	}
	else{
		int ap_passwd_len = strlen(ap_passwd);
		rtw_security_t ap_secu = RTW_SECURITY_OPEN;

		#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)
		ap_state_wifi_ssid_get    ((INT8U *)&wifi_ssid, &wifi_ssid_len);
		ap_state_wifi_password_get((INT8U *)&wifi_pw  , &wifi_pw_len);
		if(wifi_ssid_len)
		{
			ap_name = (char *)wifi_ssid;
			if(wifi_pw_len)
			{
				ap_passwd = (char *)wifi_pw;
				ap_passwd_len = wifi_pw_len;
			}
			else
			{
                ap_state_wifi_password_set((INT8U *)ap_passwd, strlen(ap_passwd));
			}
		}
		#endif

		if(ap_passwd_len > 0)
		{
			ap_secu = RTW_SECURITY_WPA2_AES_PSK;
		}

		if(wifi_start_ap(ap_name,
						ap_secu,
						ap_passwd,
						strlen(ap_name),
						ap_passwd_len,
						channel? channel:AP_DEFAULT_CH)<0)
		{
			print_string("%s wifi_start_ap failed!\r\n",__FUNCTION__);
		}
		else
		{
			int timeout = 20;
			while(1) {
				char essid[33];

				if(wext_get_ssid(WLAN0_NAME, (unsigned char *) essid) > 0) {
					if(strcmp((const char *) essid, (const char *)ap_name) == 0) {
						print_string("\n\r%s started\n", ap_name);
						break;
					}
				}

				if(timeout == 0) {
					print_string("\n\rERROR: Start AP timeout!");
					return -3;
				}

				vTaskDelay(1 * configTICK_RATE_HZ);
				timeout --;
			}
			print_string("\r%s wifi_start_ap end\n",__FUNCTION__);
		}


		/* Start DHCP Server */
		dhcps_init(pnetif);

		{
			rtw_wifi_setting_t setting;

			uint8_t *mac = LwIP_GetMAC(&xnetif[0]);
			uint8_t *ip = LwIP_GetIP(&xnetif[0]);

			wifi_get_setting(WLAN0_NAME,&setting);
			wifi_show_setting(WLAN0_NAME,&setting);
			print_string("\n\r  MAC => %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]) ;
			print_string("\n\r  IP  => %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	#if CONFIG_START_STA_AP
			mac = LwIP_GetMAC(&xnetif[1]);
			ip = LwIP_GetIP(&xnetif[1]);
			wifi_get_setting(WLAN1_NAME,&setting);
			wifi_show_setting(WLAN1_NAME,&setting);
			print_string("\n\r  MAC => %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]) ;
			print_string("\n\r  IP  => %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	#endif
			print_string("\n\r[MEM] After WLAN Init, available heap %d\n\r", xPortGetFreeHeapSize());
		}
	}

#if CONFIG_INTERACTIVE_MODE
    if(g_wifi_logservice == 0)
    {
        extern void log_service_init(void);

        //start_interactive_mode();
        log_service_init();
        g_wifi_logservice = 1;
    }
#endif
    return 0;
}

void wifi_terminal_thread(void const *param)
{
    wifi_off();
    osThreadTerminate(NULL);
}

INT32S rtk_wifi_module_disable(void)
{
    //osThreadDef(wifi_terminal_thread, osPriorityHigh, 0, 1024); //priority must >= sdio_irq_thread
    osThreadDef(wifi_terminal_thread, osPriorityIdle+8, 0, 2048); //priority must >= sdio_irq_thread
  	osThreadCreate(&os_thread_def_wifi_terminal_thread, NULL);

    //work-around
    RTSP_wait_flag = 1;
    print_string("%s wal_ap_mode_disable end\r\n",__FUNCTION__);
    return 0;
}

INT32S wifi_module_del_station(void)
{
    int client_number;
    struct {
        int    count;
        rtw_mac_t mac_list[3];
    } client_info;

    client_info.count = 3;
    wifi_get_associated_client_list(&client_info, sizeof(client_info));

    if(client_info.count == 0)
        print_string("\n\rClient Num: 0\n\r");
    else
    {
        for( client_number=0; client_number < client_info.count; client_number++ )
        {
            print_string("\n\r\tMAC => "MAC_FMT"",MAC_ARG(client_info.mac_list[client_number].octet));
            wext_del_station(WLAN0_NAME, client_info.mac_list[client_number].octet);
        }
    }
    return 0;
}

INT32S wifi_data_rate(void)
{
    char cmd[25];
    snprintf(cmd, 25, "dbg 0x7faa0000 0xa");
    wext_private_command(WLAN0_NAME, (char *)&cmd, 1);
    return 0;
}
/*  RTK:
 *  status:
 *  0: wifi disable
 *  1: wifi enable
 *  2: client connectted
 */
INT32S rtk_wifi_module_conn_status()
{
    INT32S status = 0;

    struct {
        int    count;
        rtw_mac_t mac_list[3];
    } client_info;

    client_info.count = 3;
    wifi_get_associated_client_list(&client_info, sizeof(client_info));

    if(rltk_wlan_running(WLAN0_IDX)) {
        status = 1;
        if(client_info.count)
        {
            status = 2;
        }
    }
    else
    {
        status = 0;
    }
    //print_string("@@@conn_status stanum %d status %d\r\n", client_info.count, status);
    return status;
}

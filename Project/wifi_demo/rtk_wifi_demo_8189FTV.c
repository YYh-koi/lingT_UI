#include "board_config.h"
#include "drv_l2.h"
#include "gp_stdlib.h"

// SDC0_MUX1 small EVM board big sd slot
// SDC1_MUX0 small EVM board TF card slot
// SDC0_MUX2 DVP board
// SDC0_MUX0 I2S board
// SDC1_MUX1 I2S board
// SDC1_MUX2 DVP board, conflict with JTAG, must use boot and plugoff JTAG

#define SDIO_WIFI_MODE          0
#define AP_MODE_SSID            "wlan_ap_gp"     // Set SSID of AP here, max length is 32 charaters
#define AP_MODE_PW              "12345678"
#define STA_MODE_SSID           "wlan_sta_gp"    // Set SSID of Station here, max length is 32 charaters
#define STA_MODE_PW             "12345678"
#define SET_AP_CHANNEL          0 //0: default channel 1
#define SET_RETRANSMISSION      0 //0: default re-transmission times(7)

extern  char wifi_ssid[];
extern  char wifi_password[];
extern INT32S rtk_wifi_module_enable(INT32U device_id, INT8U pinmux, char *ap_name, char *ap_passwd, INT32U channel, INT8U mode);
extern INT32S rtk_wifi_module_disable(void);

extern INT32S drvl1_sdio_config_bit_set(INT32U device_id, INT8U nrBit);

//global
extern int rtw_short_retry_lmt;
osThreadId g_cmd_taskHandle = 0;
static int g_sdio_init = 0;

#if defined(GPLIB_CONSOLE_EN) && (GPLIB_CONSOLE_EN == 1)
void RTK_Wifi_CmdTask(void const *param)
{

  print_string("%s ... \r\n",__func__);

  Cmd_Task((void *)param);
}
#endif

INT32S wal_ap_mode_enable(void)
{
    INT32S ret;
    #if C_WIFI_ON_OFF_KEY
    gpio_write_io(WIFI_LDO_EN_PIN, DATA_HIGH);
    osDelay(50);
    #endif
    if(g_sdio_init == 0){
    drvl1_sdio_config_bit_set(SDIO_WIFI_DEVICE_ID, SDIO_WIFI_BIT);

    #if (SDIO_WIFI_BIT == 1)
    drvl1_sdio_config_one_bit_intr_set(SDIO_WIFI_DEVICE_ID, EXTB, EXT_IRQ_MUX3);
    #endif
    //g_sdio_init = 1;
    }

    //rtw_short_retry_lmt = SET_RETRANSMISSION ? SET_RETRANSMISSION : rtw_short_retry_lmt;
    //set fixed wifi power level, set before wifi_on()
    //RTK suggest adjust level range [-4,4]
    //wifi_module_power_set(0);
    #if 1
    if( g_sdio_init != 0)
    drvl2_sdio_init(SDIO_WIFI_DEVICE_ID);
    g_sdio_init = 1;
    ret = rtk_wifi_module_enable(SDIO_WIFI_DEVICE_ID, 0, AP_MODE_SSID, AP_MODE_PW, SET_AP_CHANNEL, 0);
	#else
    ret = rtk_wifi_module_enable(SDIO_WIFI_DEVICE_ID, 0, STA_MODE_SSID, STA_MODE_PW, SET_AP_CHANNEL, 1);
	#endif

    DBG_PRINT("rtk_wifi_module_enable ret=%d\r\n", ret);
    //rtk_wifi_wifi_pwr_get();
    //set data rate, set after wifi_on()
    //wifi_module_data_rate(0xa);

    #if defined(GPLIB_CONSOLE_EN) && (GPLIB_CONSOLE_EN == 1)
    if(g_cmd_taskHandle == 0)
    {
        osThreadDef(RTK_Wifi_CmdTask, osPriorityLow, 0, 4096);
		void *mode=0;
		// mode
		// 0: ATW mode
		// 1: interative mode
        g_cmd_taskHandle = osThreadCreate(&os_thread_def_RTK_Wifi_CmdTask, mode);
    }
    #endif
    //get_mac_address();
    return STATUS_OK;
}

INT32S wal_ap_mode_disable(void)
{
    rtk_wifi_module_disable();
    osDelay(2000);
    gpio_write_io(WIFI_LDO_EN_PIN, DATA_LOW);
    return STATUS_OK;
}

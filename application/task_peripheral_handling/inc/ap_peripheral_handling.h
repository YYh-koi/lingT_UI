#ifndef __AP_PERIPHERAL_HANDLING_H__
#define __AP_PERIPHERAL_HANDLING_H__

// Send key events faster than human finger can do
#define CRAZY_KEY_TEST		0

typedef void (*KEYFUNC) (INT16U * tick_cnt_ptr);

typedef enum
{
	AP_PERIPHERAL_CALL_KEY_ANSWER = 0,
	AP_PERIPHERAL_CALL_KEY_HANGUP
} ap_peripheral_call_key_action_t;

typedef INT8U (*ap_peripheral_call_key_handler_t)(
	ap_peripheral_call_key_action_t action);

typedef enum
{
	AP_PERIPHERAL_MEDIA_KEY_PREVIOUS = 0, /* KEY1: previous track */
	AP_PERIPHERAL_MEDIA_KEY_PLAY_PAUSE,   /* KEY2: play / pause */
	AP_PERIPHERAL_MEDIA_KEY_NEXT          /* KEY3: next track */
} ap_peripheral_media_key_action_t;

typedef INT8U (*ap_peripheral_media_key_handler_t)(
	ap_peripheral_media_key_action_t action);

typedef struct
{
	KEYFUNC key_function;
	KEYFUNC fast_key_fun;
	INT16S	key_val_L;
	INT16S	key_val_H;
	INT16U	key_cnt;
} KEYSTATUS;

enum
{
	USBD_CONNECTED_EMPTY = 0,
	USBD_CONNECTED_TO_CHARGER,
	USBD_CONNECTED_TO_PC,
	USBD_CONNECTED_TO_HOST,
	USBD_CONNECTED_AND_HOST_JUST_REMOVED
};

extern INT8U	s_usbd_pin;

extern void ap_peripheral_mode_key_exe(INT16U *tick_cnt_ptr);
extern void ap_peripheral_ok_key_exe(INT16U *tick_cnt_ptr);
extern void ap_peripheral_pw_key_exe(INT16U *tick_cnt_ptr);
extern void ap_peripheral_sos_key_exe(INT16U *tick_cnt_ptr);
extern void ap_peripheral_menu_key_exe(INT16U *tick_cnt_ptr);
extern void ap_peripheral_prev_key_exe(INT16U *tick_cnt_ptr);
extern void ap_peripheral_next_key_exe(INT16U *tick_cnt_ptr);

extern void		ap_peripheral_init(void);
extern void		ap_peripheral_key_judge(void);
extern void		ap_peripheral_usbd_io_detection(void);
extern INT8U	ap_peripheral_get_usbd_io_status(void);
extern INT8U	ap_peripheral_get_usbd_connection_state(void);
extern void		ap_peripheral_set_usbd_connection_state(INT8U state);
extern void		ap_peripheral_key_register(INT8U type);
extern void		ap_peripheral_call_key_handler_register(
	ap_peripheral_call_key_handler_t handler);
extern void		ap_peripheral_media_key_handler_register(
	ap_peripheral_media_key_handler_t handler);
extern void		ap_peripheral_motion_detect_judge(void);
extern void		ap_peripheral_motion_detect_start(void);
extern void		ap_peripheral_motion_detect_stop(void);

extern void		ap_peripheral_auto_off_force_disable_set(INT8U auto_off_disable);

extern void		ap_peripheral_night_mode_set(INT8U type);

#if C_GSENSOR_FUNCTION == CUSTOM_ON
extern void		ap_peripheral_read_gsensor(void);
#endif

extern void		ap_peripheral_ad_key_judge(void);
extern void		ap_peripheral_config_store(void);


extern INT8U	back_a_car_status_get(void);
extern INT32S	ap_peripheral_back_detect(void);
extern void		back_a_car_status_set(INT8U val);
extern INT32U	ap_peripheral_get_host_io_status(void);

extern void		ap_peripheral_lcd_backlight_set(INT8U type);
extern INT32U	ap_peripheral_lcd_backlight_get(void);

#if C_SCREEN_SAVER
extern void		ap_peripheral_clr_screen_saver_timer(void);
#endif

extern INT32U	ap_peripheral_battery_level_get(void);
#if C_BATTERY_DETECT
extern void		ap_peripheral_battery_check_calculate(INT16U battery_value);
#endif

#if CRAZY_KEY_TEST
extern INT32U	crazy_key_enable;
extern void		ap_peripheral_crazy_key_test(void);
extern void		ap_peripheral_touch_panel_crazykey_test(void);
#endif

#if C_RTC_REBOOT
extern INT32S	ap_peripheral_rtc_alarm_set(void);
extern void		ap_peripheral_rtc_reboot_init(void);

#endif

extern void 	ap_peripheral_speaker_ctrl(INT8U on_or_off);
extern void		ap_peripheral_led_blink_set(INT8U state);
extern void     LED_service(void);

extern void     ap_peripheral_sys_osd_time_set(void);
extern void     ap_peripheral_osd_time_get(TIME_T *osd_time);

#if defined(HDMI_HPD_PIN) && (LCD_DRV_EN_HDMI==1)
extern void ap_peripheral_hdmi_hpd_detect(void);
#endif


#endif // __AP_PERIPHERAL_HANDLING_H__

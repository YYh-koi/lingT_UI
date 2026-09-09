#ifndef __AP_STATE_HANDLING_H__
#define __AP_STATE_HANDLING_H__

extern INT8U	ap_state_handling_connect_to_pc(INT32U prev_state);
extern void		ap_state_handling_disconnect_to_pc(void);
extern void		ap_state_handling_storage_id_set(INT8U stg_id);
extern INT8U	ap_state_handling_storage_id_get(void);
extern void		ap_state_handling_calendar_init(void);

#if C_WATER_MARK == CUSTOM_ON
extern DISPLAY_ICONSHOW    water_mark_icon;
extern DISPLAY_ICONSHOW    water_mark_gp420;
extern DISPLAY_ICONSHOW    water_mark_yuyv;
extern void ap_state_handling_prepare_water_mark_icon(DISPLAY_ICONSHOW *pIcon, INT8U draw_type);
#endif

extern INT8U	back_sensor_plug_status_get(void);
extern void		back_sensor_plug_status_set(INT8U in_out);

extern INT32S	ap_state_common_handling(INT32U msg_id);
extern void		ap_state_handling_power_off(INT8U sw_reset);
extern INT16U	present_state;

#if C_BATTERY_LOW_POWER_OFF
extern INT8U Low_Battery_Judge(void);
#endif

extern void		ap_state_handling_lcd_backlight_switch(INT8U enable);
extern INT32S	ap_state_handling_warn_str_icon_draw(INT16U str_idx, INT16U str_color);
extern void		ap_state_handling_warn_str_icon_exit(void);
extern void		ap_state_handling_show_insert_sdc(void);
extern void		ap_state_handling_show_no_sd(void);
extern void		ap_state_handling_show_busy(void);
extern void     ap_state_handling_show_protected(void);
extern void		ap_state_handling_show_sd_full(void);
extern void		ap_state_handling_show_sd_err(void);
extern void		ap_state_handling_show_battery_low(void);

#if C_BATTERY_DETECT
extern void		ap_state_handling_battery_icon_show(BAT_LVL bat_lvl);
extern void		ap_state_handling_charge_icon_show(INT8U charge_flag);
extern void		ap_state_handling_current_bat_lvl_show(void);
extern void		ap_state_handling_current_charge_icon_show(void);
#endif // #if C_BATTERY_DETECT

extern void		ap_state_handling_icon_show_cmd(INT8U cmd1, INT8U cmd2, INT8U cmd3, INT8U cmd4);
extern void		ap_state_handling_icon_clear_cmd(INT8U cmd1, INT8U cmd2, INT8U cmd3, INT8U cmd4);

extern void     ap_state_firmware_upgrade(void);
extern INT8U    ap_state_firmware_upgrade_check(void);

#endif // #ifndef __AP_STATE_HANDLING_H__

#ifndef __TASK_PERIPHERAL_HANDLING_H__
#define __TASK_PERIPHERAL_HANDLING_H__

#define PERI_TIME_INTERVAL_AD_DETECT		4									// 128 = 1s

#define PERI_COFING_STORE_INTERVAL	(5 * 128 / PERI_TIME_INTERVAL_AD_DETECT)	// 5s
#define PERI_TIME_BACKLIGHT_DELAY	(16)										// 128 = 1s
#define PERI_ADP_OUT_PWR_OFF_TIME	(7 * 128 / PERI_TIME_INTERVAL_AD_DETECT)	// 7s
#define PERI_USB_PHY_SUSPEND_TIME	(4 * 128 / PERI_TIME_INTERVAL_AD_DETECT)

#if C_SCREEN_SAVER
extern INT8U	screen_saver_enable;
#endif

extern void     peripheral_handling_task_init(void);
extern void		task_peripheral_handling_entry(void const *turnkey_args);
extern void		task_peripheral_handling_init(void);
extern INT32U	home_rec_mic_icon_status_get(void);

extern INT32S peripheral_task_msg_send(INT32U msg, void *para, INT32U bytes, INT32U prio);

#endif // __TASK_PERIPHERAL_HANDLING_H__

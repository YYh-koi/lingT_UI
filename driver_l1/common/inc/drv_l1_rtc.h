#ifndef __drv_l1_RTC_H__
#define __drv_l1_RTC_H__

#define RTC_RTCEN               (1 << 15)  /* RTC enable */

#define RTC_SCHSEL              (7 <<  0)  /* schedule time period selection */

#define RTC_SEC_BUSY            (1 << 15)  /* RTC second busy flag */
#define RTC_MIN_BUSY            (1 << 14)  /* RTC min busy flag */
#define RTC_HR_BUSY             (1 << 13)  /* RTC hour busy flag */

#define RTC_BUSY                -1

#define REALIBABLE              0x15

#define EXT_RTC_SEC_IEN         (1 <<  0)  /* second interrupt enbale */
#define EXT_RTC_ALARM_IEN       (1 <<  1)  /* alarm interrupt enbale */
#define EXT_RTC_WAKEUP_IEN      (1 <<  2)  /* wakeup interrupt enbale */

#define R_EXTRTC_CTRL		    0x00
#define R_EXTRTC_RRP		    0x01
#define R_EXTRTC_RELIABLE	    0x02
#define R_EXTRTC_LVR		    0x03

#define R_EXTRTC_LOAD_TIME0	    0x10
#define R_EXTRTC_LOAD_TIME1	    0x11
#define R_EXTRTC_LOAD_TIME2	    0x12
#define R_EXTRTC_LOAD_TIME3	    0x13
#define R_EXTRTC_LOAD_TIME4	    0x14
#define R_EXTRTC_LOAD_TIME5	    0x15

#define R_EXTRTC_ALARM0		    0x21
#define R_EXTRTC_ALARM1		    0x22
#define R_EXTRTC_ALARM2		    0x23
#define R_EXTRTC_ALARM3		    0x24
#define R_EXTRTC_ALARM4		    0x25

#define R_EXTRTC_TIME0		    0x30
#define R_EXTRTC_TIME1		    0x31
#define R_EXTRTC_TIME2		    0x32
#define R_EXTRTC_TIME3		    0x33
#define R_EXTRTC_TIME4		    0x34
#define R_EXTRTC_TIME5		    0x35

#define R_EXTRTC_INT_STATUS             0x40
#define R_EXTRTC_IO_CTRL0               0x48
#define R_EXTRTC_IO_MODE0               0x4A
#define R_EXTRTC_IO_MODE1               0x4B
#define R_EXTRTC_INT_ENABLE	            0x50
#define R_EXTRTC_WKU_IO_INT_EN          0x58
#define R_EXTRTC_WKU_IO_INT_STS         0x59


#define RTC_ALMEN               (1 << 10)  /* alarm function enable */
#define RTC_HMSEN               (1 <<  9)  /* H/M/S function enable */
#define RTC_SCHEN               (1 <<  8)  /* scheduler function enbale */

#define RTC_ALM_IEN             (1 << 10)  /* alarm interrupt enbale */
#define RTC_SCH_IEN             (1 <<  8)  /* scheduler interrupt enbale */
#define RTC_DAY_IEN             (1 <<  4)  /* hour interrupt enbale */
#define RTC_HR_IEN              (1 <<  3)  /* hour interrupt enbale */
#define RTC_MIN_IEN             (1 <<  2)  /* min interrupt enbale */
#define RTC_SEC_IEN             (1 <<  1)  /* alarm interrupt enbale */
#define RTC_HALF_SEC_IEN        (1 <<  0)  /* alarm interrupt enbale */

#define RTC_EN                  0xFFFFFFFF
#define RTC_DIS                 0

#define RTC_EN_MASK             0xFFFF

#define GPX_RTC_ALMOEN          (1 << 0)  /* alarm output singnal enable */
#define GPX_RTC_EN              (1 << 3)  /* RTC function enable */
#define GPX_RTC_VAEN            (1 << 4)  /* Voltage comparator enbale */
#define GPX_RTC_PWREN           (1 << 5)  /* set alarm output signal to high */

#define GPX_RTC_ALM_IEN         (1 <<  4)  /* alarm interrupt enbale */
#define GPX_RTC_DAY_IEN         (1 <<  5)  /* hour interrupt enbale */
#define GPX_RTC_HR_IEN          (1 <<  3)  /* hour interrupt enbale */
#define GPX_RTC_MIN_IEN         (1 <<  2)  /* min interrupt enbale */
#define GPX_RTC_SEC_IEN         (1 <<  1)  /* alarm interrupt enbale */
#define GPX_RTC_HALF_SEC_IEN    (1 <<  0)  /* alarm interrupt enbale */

typedef struct __rtc
{
	INT32U		rtc_sec;    /* seconds [0,59]  */
	INT32U		rtc_min;    /* minutes [0,59]  */
	INT32U		rtc_hour;   /* hours [0,23]  */
	INT32U      rtc_day;    /* gpy0200 day count,[ 0,4095] */
} t_rtc;

typedef enum
{
	RTC_ALM_INT_INDEX,
	RTC_SCH_INT_INDEX,
	RTC_HR_INT_INDEX,
	RTC_MIN_INT_INDEX,
	RTC_SEC_INT_INDEX,
	RTC_HSEC_INT_INDEX,
	RTC_DAY_INT_INDEX,
	EXT_RTC_SEC_INT_INDEX,
	EXT_RTC_ALARM_INT_INDEX,
	EXT_RTC_WAKEUP_INT_INDEX,
	EXT_RTC_IO_INT_INDEX,
	EXT_RTC_USB_INT_INDEX,
	RTC_INT_MAX
} RTC_INT_INDEX;

typedef enum
{
	RTC_SCH_16HZ,
	RTC_SCH_32HZ,
	RTC_SCH_64HZ,
	RTC_SCH_128HZ,
	RTC_SCH_256HZ,
	RTC_SCH_512HZ,
	RTC_SCH_1024HZ,
	RTC_SCH_2048HZ
} RTC_SCH_PERIOD;

extern void drv_l1_rtc_init(INT8U vic_rtc, INT8U vic_ertc);
extern void rtc_ext_to_int_set(void);
extern INT32S rtc_callback_clear(INT8U int_idx);
extern void rtc_time_get(t_rtc *rtc_time);
extern void rtc_alarm_get(t_rtc *rtc_time);
extern INT32S rtc_callback_set(INT8U int_idx, void (*user_isr)(void));
extern void rtc_alarm_set(t_rtc *rtc_time);
extern void rtc_time_set(t_rtc *rtc_time);
extern void rtc_day_get(t_rtc *rtc_time);
extern void rtc_int_set(INT16U mask, INT16U value);
extern void rtc_schedule_enable(INT8U freq);
extern void rtc_schedule_disable(void);
extern void RTC_KC_IRQHandler(void);
extern void rtc_alarm_enable(void);
extern void drv_l1_alarm_wakeup(INT32U alm_sec);

void ext_rtc_init(INT8U vic_num);
INT32S ext_rtc_set_reliable(INT8U byReliable) ;
INT32S ext_rtc_get_reliable(INT8U *pbyReliable) ;
INT32S ext_rtc_time_set(t_rtc *rtc_time) ;
INT32S ext_rtc_time_get(t_rtc *rtc_time) ;
INT32S ext_rtc_wakeup_int_enable(INT8U byEnable, void (*ext_rtc_wakeup_isr)(void)) ;
INT32S ext_rtc_alarm_enable(t_rtc *rtc_time, INT8U byEnable, void (*ext_rtc_alarm_isr)(void));
INT32S ext_rtc_chk_wakeup_status(void);
INT32S ext_rtc_sec_int_enable(INT8U byEnable, void (*ext_rtc_sec_isr)(void));
INT32S ext_rtc_read(INT8U addr, INT8U *data);
INT32S ext_rtc_write(INT8U addr, INT8U data);
INT32S ext_rtc_set_exio_wakeup(INT8U port, INT8U mode);
INT32S ext_rtc_io_int_enable(INT8U byEnable, void (*ext_rtc_io_isr)(void));
INT32S ext_rtc_set_usb_wakeup(INT8U io_conf);
INT32S ext_rtc_usb_int_enable(INT8U byEnable, void (*ext_rtc_usb_isr)(void));
#endif 		/* __drv_l1_RTC_H__ */

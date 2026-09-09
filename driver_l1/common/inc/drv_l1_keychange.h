#ifndef __DRV_L1_KEYCHANGE_H__
#define __DRV_L1_KEYCHANGE_H__

typedef enum
{
	KEYC0,
	KEYC1,
	KEYC2,
	KEYC3,
	KEYC4,
	KEYC5,
	KEYC6,
	KEYC7
}KEYC_ENUM;

#define INT_K0 0x00000002
#define INT_K1 0x00000008
#define INT_K2 0x00000020
#define INT_K3 0x00000080
#define INT_K4 0x00000200
#define INT_K5 0x00000800
#define INT_K6 0x00002000
#define INT_K7 0x00008000

#define KEYC_CH0 0x00000001
#define KEYC_CH1 0x00000002
#define KEYC_CH2 0x00000004
#define KEYC_CH3 0x00000008
#define KEYC_CH4 0x00000010
#define KEYC_CH5 0x00000020
#define KEYC_CH6 0x00000040
#define KEYC_CH7 0x00000080

extern void drv_l1_keychange_init(INT32U channel, INT32U double_edge, INT8U vic_num);
extern void drv_l1_keychange_for_wifi_init(INT32U channel, INT32U double_edge, INT8U vic_num);
extern void drv_l1_keychange_interrupt_flag_clear(INT8U channel);
extern void drv_l1_keychange_interrupt_enable(INT8U channel);
extern void drv_l1_keychange_interrupt_disable(INT8U channel);
extern void drv_l1_keychange_user_isr_clear(INT8U channel);
extern INT32U drv_l1_keychange_latch_update(void);
extern INT32U drv_l1_keychange_status_get(void);
extern void drv_l1_keychange_user_isr_set(INT8U channel,void (*user_isr)(void));

#endif		// __drv_l1_KEYCHANGE_H__

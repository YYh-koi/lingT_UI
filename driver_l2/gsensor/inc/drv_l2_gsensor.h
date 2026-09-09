#ifndef __DRV_L1_GSENSOR_H__
#define __DRV_L1_GSENSOR_H__

#define G_SENSOR_NONE		0
#define G_SENSOR_DA380		1
#define G_SENSOR_SC7A30E	2

#define P_SENSITIVE_LOW	     0x60
#define P_SENSITIVE_MID	     0x38
#define P_SENSITIVE_HIGH	 0x10

#define DMT_SENSITIVE_LOW	 0x60//0xB0
#define DMT_SENSITIVE_MID	 0x38//0x60
#define DMT_SENSITIVE_HIGH	 0x10//0x20

#define SC7A30E_XDATA			0x29
#define SC7A30E_YDATA			0x2B
#define SC7A30E_ZDATA			0x2D

#define SC7A30E_SENSITIVE_LOW	 	0x28//40
#define SC7A30E_SENSITIVE_MID	 	0x14//20
#define SC7A30E_SENSITIVE_HIGH	 	0x05//5
#define SC7A30E_SENSITIVE_OFF		0x7F
#define SC7A30E_SENSITIVE_LEVEL		DMT_SC7A30E_SENSITIVE_MID

//==========================================
//    DA380 Register
//==========================================
#define SOFT_RESET    0x00
#define CHIPID        0x01

#define ACC_X_LSB     0x02
#define ACC_X_MSB     0x03

#define ACC_Y_LSB     0x04
#define ACC_Y_MSB     0x05

#define ACC_Z_LSB     0x06
#define ACC_Z_MSB     0x07

#define MOTION_FLAG   0x09
#define NEWDATA_FLAG  0x0A

#define TAP_ACTIVE_STATUS 0x0B

#define ORIENT_STATUS 0x0C

#define RESOLUTION_RANGE 0x0F  // Resolution bit[3:2] -- 00:14bit
                               //                        01:12bit
                               //                        10:10bit
                               //                        11:8bit

                               // FS bit[1:0]         -- 00:+/-2g
                               //                        01:+/-4g
                               //                        10:+/-8g
                               //                        11:+/-16g
#define ODR_AXIS      0x10
#define MODE_BW       0x11
#define SWAP_POLARITY 0x12
#define INT_SET1      0x16
#define INT_SET2      0x17
#define INT_MAP1      0x19
#define INT_MAP2      0x1A
#define INT_CONFIG    0x20
#define INT_LATCH     0x21
#define FREEFALL_DUR  0x22
#define FREEFALL_THS  0x23
#define FREEFALL_HYST 0x24
#define ACTIVE_DUR    0x27
#define ACTIVE_THS    0x28
#define TAP_DUR       0x2A
#define TAP_THS       0x2B
#define ORIENT_HYST   0x2C
#define Z_BLOCK       0x2D
#define SELF_TEST     0x32
#define ENGINEERING_MODE   0x7f

//==========================================
//    SC7A30E Register
//==========================================
#define SC7A30E_SOFT_RESET    	0x00
#define SC7A30E_CHIPID        	0x0F

#define SC7A30E_CTRL_REG_1		0x20
#define SC7A30E_CTRL_REG_2		0x21
#define SC7A30E_CTRL_REG_3		0x22
#define SC7A30E_CTRL_REG_4		0x23
#define SC7A30E_CTRL_REG_5		0x24
#define SC7A30E_CTRL_REG_6		0x25
#define SC7A30E_REFERANCE		0x26
#define SC7A30E_OUT_X_L			0x28
#define SC7A30E_OUT_X_H			0x29
#define SC7A30E_OUT_Y_L			0x2A
#define SC7A30E_OUT_Y_H			0x2B
#define SC7A30E_OUT_Z_L			0x2C
#define SC7A30E_OUT_Z_H			0x2D

#define SC7A30E_INT1_CFG		0x30
#define SC7A30E_INT1_SOURCE		0x31
#define SC7A30E_INT1_THS		0x32
#define SC7A30E_INT1_DURATION	0x33

#define SC7A30E_CLICK_CFG		0x38
#define SC7A30E_CLICK_SRC		0x39
#define SC7A30E_CLICK_THS		0x3A
#define SC7A30E_TIME_LIMIT		0x3B
#define SC7A30E_TIME_LATENCY	0x3C
#define SC7A30E_IME_WINDOW		0x3D
#define SC7A30E_ACT_THS			0x3E
#define SC7A30E_ACT_DURATION	0x3F

//==========================================================

typedef enum
{
	SHAKE_L,
	SHAKE_R
} SHAKE;

typedef enum
{
	ORIENT_0,
	ORIENT_90,
	ORIENT_180,
	ORIENT_270
} ORIENT;

typedef struct
{
	INT32U Xacc;
	INT32U Yacc;
	INT32U Zacc;
} AXIS_DATA;

typedef struct
{
	AXIS_DATA Axis;
	INT32U Ori;
	INT32U SH;
} GSENSOR_DATA;

extern void		drv_l2_gsensor_sensitive_set(INT8U Gsensor);
extern void		drv_l2_gsensor_park_mode_init(INT8U level);
extern void		drv_l2_gsensor_int_flag_clear(void);
extern INT16U	drv_l2_gsensor_int_active_get(void);
extern INT8U	drv_l2_gsensor_power_on_status_get(void);
extern void		drv_l2_gsensor_gps_data_get(INT8U *pDATA);
extern void		drv_l2_gsensor_gps_data_set(void *pDST, void *pSRC);
extern void		drv_l2_gsensor_pw_off(void);
extern void		drv_l2_gsensor_init(INT8U dev, INT8U act, GPIO_ENUM gpio_scl, GPIO_ENUM gpio_sda, INT8U level);

#endif


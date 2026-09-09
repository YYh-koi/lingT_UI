#ifndef __DRV_L2_3A_H__
#define __DRV_L2_3A_H__

#define CDSP_AE_CTRL_EN		0x01
#define CDSP_AWB_CTRL_EN	0x02
#define CDSP_AF_CTRL_EN		0x04
#define CDSP_HIST_CTRL_EN	0x08
#define CDSP_LUM_STATUS		0x10
#define CDSP_LOW_LUM		0x20
#define CDSP_HIGH_LUM		0x40
#define CDSP_AWB_SET_GAIN	0x80
#define CDSP_SAT_SWITCH		0x100
#define CDSP_AE_UPDATE		0x200

#define C_SENSOR_SET_AE         0x123456d0
#define C_SENSOR_SET_AE2        0x123456d1
#define C_SENSOR_SET_AE_STOP    0x3456dd88
#define C_GP_DO_HIST_CST        0x12312345

#define C_GP_DO_AE          0x33445566
#define C_GP_DO_AE2         0x33445567
#define C_GP_DO_AE_STOP     0x88779965
#define C_GP_DO_AWB         0x44556677
#define C_GP_DO_AWB2        0x44556678
#define C_GP_DO_AWB_STOP    0x54687691
#define C_GP_DO_AF          0x33445789
#define C_GP_DO_AF_STOP     0x88709946

typedef struct
{
	// ev index
	INT32S ae_ev_step;
	INT32S sensor_ev_idx;
	INT32U get_lum;
	INT32U avg_lum;
	INT32U ae_target;
	INT32S digital_gain;
	INT32U awb_mode;
	INT32U ae_meter;
	AWB_RESULT awbret;
	INT16U rgain, bgain;
	INT32U edge_lvel;
	INT32S satcontr_idx;
	INT32U analog_gain;
	INT32U ae_time;
	INT32U is_night;
} ae_info_t;

int gp_3a_create(int arg, int num_sensor);
int gp_3a_del(void);

extern void gp_ae_info_get(ae_info_t *ae_ifo);
extern void gp_cdsp_ae_info_set_Again(INT32U time, INT32U analog_gain, INT32U ev_idx);
extern void gp_cdsp_task_priority(INT8 mode);
#endif // __DRV_L2_AEAWB_H__

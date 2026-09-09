/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2022 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park                       *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
#include "application.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
static drv_l2_sensor_ops_t* sensor_obj[MAX_SENSOR_NUM];
static drv_l2_sensor_para_t sensor_para = {0};
static sensor_frame_range_t gframeRangeArgs;
static SemaphoreHandle_t sem_clip  = NULL;
/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
**************************************************************************/
extern INT32U frame_width_after_clip;
/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#ifndef DBG_PRINT
#define DBG_PRINT   printf
#endif

/**
 * @brief   Register sensor operation function table
 * @param   sensor : operation function table
 * @return 	none
 */
void drv_l2_sensor_register_ops(drv_l2_sensor_ops_t* sensor, int i)
{
    if(sensor_obj[i] == 0) {
        sensor_obj[i] = sensor;
    }
	if(i == MAX_SENSOR_NUM) {
		DBG_PRINT("sensor_obj not find the empty when register ops\r\n");
	}
}
/**
 * @brief   Get sensor operation function table
 * @param   index : sensor index
 * @return 	sensor operation function table
 */
drv_l2_sensor_ops_t* drv_l2_sensor_get_ops(INT32U index)
{
	if(index > MAX_SENSOR_NUM) {
		DBG_PRINT("Index[%d] is wrong when get ops\r\n", index);
		return NULL;
	}
	return sensor_obj[index];
}

/**
 * @brief   Get sensor parameter function index
 * @param   none
 * @return 	sensor parameter function index
 */
drv_l2_sensor_para_t* drv_l2_sensor_get_para(void)
{
	return &sensor_para;
}

/**
 * @brief   Sensor driver layer 2 initialization function
 * @param   none
 * @return 	none
 */

void drv_l2_sensor_init(void)
{
	INT32U i;
	/* reset sensor_obj pointer to NULL */
	for(i=0; i<MAX_SENSOR_NUM; i++) {
		sensor_obj[i] = NULL;
	}

	/*== Front ==*/
	sensor_obj[0] =
#if (defined _SENSOR_H42_CDSP_MIPI) && (_SENSOR_H42_CDSP_MIPI == 1)
    (drv_l2_sensor_ops_t*)&h42_cdsp_mipi_ops;
#elif (defined _SENSOR_H62_CDSP_MIPI) && (_SENSOR_H62_CDSP_MIPI == 1)
    (drv_l2_sensor_ops_t*)&h62_cdsp_mipi_ops;
#elif (defined _SENSOR_GC1054_CDSP_MIPI) && (_SENSOR_GC1054_CDSP_MIPI == 1)
    (drv_l2_sensor_ops_t*)&gc1054_cdsp_mipi_ops;
#elif (defined _SENSOR_GC1064_CDSP_MIPI) && (_SENSOR_GC1064_CDSP_MIPI == 1)
    (drv_l2_sensor_ops_t*)&GC1064_cdsp_mipi_ops;
#elif (defined _SENSOR_GC2053_CDSP_MIPI) && (_SENSOR_GC2053_CDSP_MIPI == 1)
    (drv_l2_sensor_ops_t*)&gc2053_cdsp_mipi_ops;
#elif (defined _SENSOR_GC2083_CDSP_MIPI) && (_SENSOR_GC2083_CDSP_MIPI == 1)
    (drv_l2_sensor_ops_t*)&gc2083_cdsp_mipi_ops;
#elif (defined _SENSOR_TP9951_AHD_CSI_MIPI) && (_SENSOR_TP9951_AHD_CSI_MIPI == 1)
    (drv_l2_sensor_ops_t*)&tp9951_csi_mipi_ops;
#elif (defined _SENSOR_PR2000_AHD_CDSP_MIPI) && (_SENSOR_PR2000_AHD_CDSP_MIPI == 1)
    (drv_l2_sensor_ops_t*)&pr2000_cdsp_mipi_yuv_ops;
#elif (defined _SENSOR_PR2100_AHD_MIPI_DVP) && (_SENSOR_PR2100_AHD_MIPI_DVP == 1)
    (drv_l2_sensor_ops_t*)&pr2100_cdsp_mipi_ops;
#elif(defined _SENSOR_GC4653_CDSP_MIPI) && (_SENSOR_GC4653_CDSP_MIPI == 1)
	(drv_l2_sensor_ops_t*)&GC4653_cdsp_mipi_ops;
#elif(defined _SENSOR_GC5025_CDSP_MIPI) && (_SENSOR_GC5025_CDSP_MIPI == 1)
	(drv_l2_sensor_ops_t*)&gc5025_cdsp_mipi_ops;
#elif (defined _SENSOR_GC2145_YUV_CSI_MIPI) && (_SENSOR_GC2145_YUV_CSI_MIPI == 1)
	(drv_l2_sensor_ops_t*)&gc2145_csi_mipi_ops;
#elif (defined _SENSOR_PR2100_AHD_MIPI_CDSP_CSI) && (_SENSOR_PR2100_AHD_MIPI_CDSP_CSI == 1)
    (drv_l2_sensor_ops_t*)&pr2100_cdsp_mipi_ops;
#elif (defined _SENSOR_OV2740_CDSP_MIPI) && (_SENSOR_OV2740_CDSP_MIPI == 1)
   (drv_l2_sensor_ops_t*)&OV2740_cdsp_mipi_ops;
#elif (defined _SENSOR_USB_MJPEG_CAM_FRONT) && (_SENSOR_USB_MJPEG_CAM_FRONT== 1)
    (drv_l2_sensor_ops_t*)&usb_mjpeg_cam_ops_front;
#elif (defined _SENSOR_IMX307_CDSP_MIPI) && (_SENSOR_IMX307_CDSP_MIPI == 1)
   (drv_l2_sensor_ops_t*)&imx307_cdsp_mipi_ops;
#elif (defined _SENSOR_IMX230_CDSP_MIPI) && (_SENSOR_IMX230_CDSP_MIPI == 1)
   (drv_l2_sensor_ops_t*)&imx230_cdsp_mipi_ops;
#elif (defined _SENSOR_IMX376_CDSP_MIPI) && (_SENSOR_IMX376_CDSP_MIPI == 1)
   (drv_l2_sensor_ops_t*)&IMX376_cdsp_mipi_ops;
#else
	NULL;
#endif

	/*== Rear ==*/
	sensor_obj[1] =
#if (defined _SENSOR_RN6752_AHD_CSI_DVP) && (_SENSOR_RN6752_AHD_CSI_DVP == 1)
	(drv_l2_sensor_ops_t*)&rn6752_csi_dvp_ops;
#elif (defined _SENSOR_TP9950_AHD_CSI_DVP) && (_SENSOR_TP9950_AHD_CSI_DVP == 1)
	(drv_l2_sensor_ops_t*)&tp9950_csi_dvp_ops;
#elif (defined _SENSOR_TP9951_AHD_CSI_DVP) && (_SENSOR_TP9951_AHD_CSI_DVP == 1)
    (drv_l2_sensor_ops_t*)&tp9951_csi_dvp_ops;
#elif (defined _SENSOR_RN6752_AHD_CDSP_DVP) && (_SENSOR_RN6752_AHD_CDSP_DVP == 1)
	(drv_l2_sensor_ops_t*)&rn6752_cdsp_dvp_ops;
#elif (defined _SENSOR_PR2100_AHD_MIPI_DVP) && (_SENSOR_PR2100_AHD_MIPI_DVP == 1)
    (drv_l2_sensor_ops_t*)&pr2100_csi_dvp_ops;
#elif (defined _SENSOR_PR2100_AHD_MIPI_CDSP_CSI) && (_SENSOR_PR2100_AHD_MIPI_CDSP_CSI == 1)
    (drv_l2_sensor_ops_t*)&pr2100_csi_mipi_ops;
#elif (defined _SENSOR_PR2000_AHD_CSI_MIPI) && (_SENSOR_PR2000_AHD_CSI_MIPI == 1)
    (drv_l2_sensor_ops_t*)&pr2000_csi_mipi_ops;
#elif (defined _SENSOR_PR2000_AHD_CSI_DVP) && (_SENSOR_PR2000_AHD_CSI_DVP == 1)
    (drv_l2_sensor_ops_t*)&pr2000_dvp_csi_ops;
#elif (defined _SENSOR_USB_MJPEG_CAM_REAR) && (_SENSOR_USB_MJPEG_CAM_REAR == 1)
    (drv_l2_sensor_ops_t*)&usb_mjpeg_cam_ops_rear;
#elif (defined _SENSOR_GC0308_CSI_DVP) && (_SENSOR_GC0308_CSI_DVP == 1)
	(drv_l2_sensor_ops_t*)&gc0308_sensor_csi_ops;
#else
	NULL;
#endif

}

INT32U drv_l2_sensor_get_path(INT32U index)
{
	drv_l2_sensor_ops_t* pSensor_ops;
	char* p;

	pSensor_ops = sensor_obj[index];
	if(pSensor_ops)
	{
		p = strstr(pSensor_ops->name, "cdsp");
		if(p)
		{
			return VID_SRC_CDSP;
		}

		p = strstr(pSensor_ops->name, "csi0");
		if(p)
		{
			return VID_SRC_CSI0;
		}

		p = strstr(pSensor_ops->name, "csi1");
		if(p)
		{
			return VID_SRC_CSI1;
		}

		p = strstr(pSensor_ops->name, "usb");
		if(p)
		{
			return VID_SRC_USB;
		}
	}
	return VID_SRC_NONE;
}

INT16U drv_l2_user_sensor_width_get(INT32U index)
{
	drv_l2_sensor_ops_t* pSensor_opt;
	drv_l2_sensor_info_t* pSensor_info;


	pSensor_opt = drv_l2_sensor_get_ops(index);
	if(pSensor_opt)
	{
		pSensor_info = pSensor_opt->get_info(0);
		if(pSensor_info)
		{
			return pSensor_info->target_w;
		}
		return 640;
	}
	else{
		return 0;
	}
}

INT16U drv_l2_user_sensor_height_get(INT32U index)
{
	drv_l2_sensor_ops_t* pSensor_opt;
	drv_l2_sensor_info_t* pSensor_info;

	pSensor_opt = drv_l2_sensor_get_ops(index);
	if(pSensor_opt)
	{
		pSensor_info = pSensor_opt->get_info(0);
		if(pSensor_info)
		{
			return pSensor_info->target_h;
		}
		return 480;
	}
	else{
		return 0;
	}
}

INT16U drv_l2_user_csi_sensor_out_format_get(INT32U index)
{
	drv_l2_sensor_ops_t* pSensor_opt;
	drv_l2_sensor_info_t* pSensor_info;

	pSensor_opt = drv_l2_sensor_get_ops(index);
	if(pSensor_opt)
	{
		pSensor_info = pSensor_opt->get_info(0);
		if(pSensor_info)
		{
			if(CSI_OUT_GP420 == pSensor_info->output_format)
			{
				return VID_DATA_420;
			}
			else
			{
				return VID_DATA_422;
			}
		}
	}

	return VID_DATA_420;
}

/**
* @brief	Enable sensor and set output bufferA/bufferB
* @param	Isp_bufferA: output buffer A
* @param	Isp_bufferB: output buffer B
* @return	none
*/
void drv_l2_user_sensor_init(INT32U isp_bufferA, INT32U isp_bufferB)
{
	const drv_l2_sensor_ops_t *pSensor[MAX_SENSOR_NUM];
	INT8U axis;

	if(!sem_clip)
	{
		sem_clip = xSemaphoreCreateBinary();
	}
	// Enable 24MHz
	#if (defined _SENSOR_IMX307_CDSP_MIPI) && (_SENSOR_IMX307_CDSP_MIPI == 1)
	drv_l2_sensor_set_mclkout(MCLK_37M125, 0);
	#else
	drv_l2_sensor_set_mclkout(MCLK_24M, 0);
	#endif
	//Register sensor obj
	drv_l2_sensor_init();

	for(axis = 0; axis < MAX_SENSOR_NUM; axis++){
		pSensor[axis] = drv_l2_sensor_get_ops(axis);
		if(!pSensor[axis])
		{
			DBG_PRINT("no available %s sensor device\r\n", (axis == 0)?"front":"rear");
		}
		else{
			if(pSensor[axis]->init)
				pSensor[axis]->init();
			if(pSensor[axis]->stream_start)
				pSensor[axis]->stream_start(0, isp_bufferA, isp_bufferB);
		}
	}
}

/*+++ Frequence set 50/60Hz +++*/
void drv_l2_user_sensor_frequence(INT32U freq_num)
{
    static INT32U exp_freq = 0;
	INT32U temp;
	const drv_l2_sensor_ops_t *pSensor = drv_l2_sensor_get_ops(0);

	if(!pSensor || !pSensor->exposure_set_freq)
	{
		return;
	}

	switch(freq_num)
	{
		case 0:
			temp = 50;
			break;

		default:
			temp = 60;
			break;
	}

	if(temp == exp_freq)
        return;

	switch(freq_num)
	{
		case 0:
			exp_freq = 50;
			break;
		default:
			exp_freq = 60;
			break;
	}
	pSensor->exposure_set_freq(exp_freq);
}

/**
 * @brief   set csi input source path
 * @param   image_src[in]: 0: from dvp sensor(parallel sensor), 1: from mipi sensor
 * @return 	none
 */
void drv_l2_sensor_set_csi_source(INT32U image_src, INT8U csi_num, INT8U mipi_num)
{
    if(csi_num == 0) {//CSI0
        if(image_src == SENSOR_INTERFACE_DVP) {
            R_SYSTEM_CTRL0 &= ~(0x1<<0);//form parallel sensor interface
        } else {
            R_SYSTEM_CTRL0 |= (0x1<<0);	//form mipi interface
            if(mipi_num)
                R_SYSTEM_CTRL0 |= (0x1<<6);//from 0:mipiA or 1:mipiB
            else
                R_SYSTEM_CTRL0 &= ~(0x1<<6);//from 0:mipiA or 1:mipiB
        }
	} else {
        if(image_src == SENSOR_INTERFACE_DVP) {
            R_SYSTEM_CTRL2 &= ~(0x1<<0);//CSI2 0:parallel or 1:mipi
        } else {
            R_SYSTEM_CTRL2 |= (0x1<<0);//CSI2 0:parallel or 1:mipi
            if(mipi_num)
                R_SYSTEM_CTRL2 |= (0x1<<1);//from 0:mipiA or 1:mipiB
            else
                R_SYSTEM_CTRL2 &= ~(0x1<<1);//from 0:mipiA or 1:mipiB
        }
	}
}

/**
* @brief	select mclk out clock speed
* @param	mclk_sel:
* @return	speed
*/
INT32S drv_l2_sensor_set_mclkout(INT32U mclk_sel, INT8U csi_num)
{
	INT32U mclk_out;
    //drv_l1_clock_set_system_clk_en(CLK_EN1_DISPAY_OUT_REG, 1);
    //drv_l1_clock_set_system_clk_en(CLK_EN1_STN_SEN_AHBMUX, 1);
	//enable 48MHz
	R_SYSTEM_CTRL1 |= (1 << 11);

	// set mclk
	switch(mclk_sel)
	{
	case MCLK_NONE:
		R_CSI_TG_CTRL1 &= ~(1 << 7);
		R_SYSTEM_CTRL1 &= ~(1 << 8);
		return 0;

	case MCLK_12M:
        R_SYSTEM_CLK_CTRL |= 0x08;	//Enable Mclk (96Mhz)
		R_SYSTEM_CTRL1 |= (1 << 15) |(1 << 14) | (1 << 8);
		R_CSI_TG_CTRL1 |= (1 << 11);
		mclk_out = 12000000;
		break;

	case MCLK_13_5M:
        R_SYSTEM_CTRL1 &= ~(1 << 14);
		R_SYSTEM_CTRL1 |= (1 << 11) | (1 << 8);
        if((R_SYSTEM_PERI_CLK_CTRL & 0xF) == 0x6){ //ExternPLL = 387Mhz
            R_SYSTEM_TMDS_CTRL |= (1<<7);
            R_SYSTEM_TMDS_CTRL = ((R_SYSTEM_TMDS_CTRL & 0xFF80) | 0x001B);//TMDS clock = PLL clock out/(1+DIV)  DIV=7 //0x0303; //div 2+1 = 3
        }else{
            R_SYSTEM_PLLEN &= ~0x3F;
            R_SYSTEM_PLLEN |=0x18;
            drv_l1_csi_sensor_clock_select(0, DISABLE);
            drv_l1_clock_set_peripheral_clock_freq(SRC_C27M, 135*100*1000);
            drv_l1_clock_set_peripheral_clock_enable(SRC_C27M, ENABLE);
        }
		mclk_out = 13500000;
		break;

    case MCLK_18M:
    R_SYSTEM_CTRL1 &= ~(1 << 14);
		R_SYSTEM_CTRL1 |= (1 << 11) | (1 << 8);
        if((R_SYSTEM_PERI_CLK_CTRL & 0xF) == 0x6){ //ExternPLL = 387Mhz
            R_SYSTEM_TMDS_CTRL |= (1<<7);
            R_SYSTEM_TMDS_CTRL = ((R_SYSTEM_TMDS_CTRL & 0xFF80) | 0x0015);//TMDS clock = PLL clock out/(1+DIV)  DIV=7 //0x0303; //div 2+1 = 3
        }else{
            R_SYSTEM_PLLEN &= ~0x3F;
            R_SYSTEM_PLLEN |=0x18;
            drv_l1_csi_sensor_clock_select(0, DISABLE);
            drv_l1_clock_set_peripheral_clock_freq(SRC_C27M, 150*100*1000);
            drv_l1_clock_set_peripheral_clock_enable(SRC_C27M, ENABLE);
        }
		mclk_out = 18000000;
		break;


	case MCLK_27M:
        R_SYSTEM_CTRL1 &= ~(1 << 14);
		R_SYSTEM_CTRL1 |= (1 << 11) | (1 << 8);
        if((R_SYSTEM_PERI_CLK_CTRL & 0xF) == 0x6){ //ExternPLL = 387Mhz
            R_SYSTEM_TMDS_CTRL |= (1<<7);
            R_SYSTEM_TMDS_CTRL = ((R_SYSTEM_TMDS_CTRL & 0xFF80) | 0x000D);//TMDS clock = PLL clock out/(1+DIV)  DIV=7 //0x0303; //div 2+1 = 3

        }else{
        	R_SYSTEM_PLLEN &= ~0x3F;
            R_SYSTEM_PLLEN |=0x18;
            R_SYSTEM_TMDS_CTRL |= (1<<7);
            R_SYSTEM_PERI_CLK_CTRL &= ~(0x1 << 0);
            R_SYSTEM_TMDS_CTRL = ((R_SYSTEM_TMDS_CTRL & 0xFF80) | 0x000B);//TMDS clock = PLL clock out/(1+DIV)  DIV=7 //0x0303; //div 2+1 = 3
        }
		mclk_out = 27000000;
		break;

	case MCLK_24M:
		// 0xD000000C[14][11][8] set 1, from XCKGEN 24Mhz

		R_SYSTEM_CTRL1 |= (1 << 14) | (1 << 11) | (1 << 8);
		R_CSI_TG_CTRL1 |= (1 << 11);
		mclk_out = 24000000;
		break;
	case MCLK_37M125:
        if((R_SYSTEM_PERI_CLK_CTRL & 0xF) == 0x6){ //ExternPLL = 387Mhz
            R_SYSTEM_TMDS_CTRL |= (1<<7);
            R_SYSTEM_TMDS_CTRL = ((R_SYSTEM_TMDS_CTRL & 0xFF80) | 0x0007);//TMDS clock = PLL clock out/(1+DIV)  DIV=7 //0x0303; //div 2+1 = 3
        }else{
            R_SYSTEM_PERI_CLK_CTRL |= (0x1<<1); //Peripherals Clock source PLL : EXT PLL
            R_SYSTEM_EXTCLK_CTRL = 0x5900;//EXTPLLN:0x1E(297MHz)   // 0x4e00; //EXT_PLL = 72MHz
            R_SYSTEM_TMDS_CTRL = 0x0707;//TMDS clock = PLL clock out/(1+DIV)  DIV=7 //0x0303; //div 2+1 = 3
            R_SYSTEM_TMDS_CTRL |= (0x1<<7);
        }
		mclk_out = 37125000;
		break;
	case MCLK_48M:
		R_SYSTEM_CTRL1 &= ~(1 << 14);
		R_SYSTEM_CTRL1 |= (1 << 11) | (1 << 8);
		R_CSI_TG_CTRL1 |= (1 << 11);
		mclk_out = 48000000;
		break;

	default:
		return 0;
	}

	// Enable CSI MCLK out
	if ((R_CSI_TG_CTRL0 & 0x1) == 0) { //check if CSI is enabled
		R_CSI_TG_CTRL0 &= ~(0x01);
		R_CSI_TG_CTRL1 &= ~(1 << 7);
		R_CSI_TG_CTRL1 |= (1 << 14);
	}

	return mclk_out;
}

/**
* @brief	Enable MIPI clock and set MIPI clock divider
* @param	mipiclk_on : 1:enable
* @param	mipiclk_div : set MIPI_DIV
            MIPI clock = PLL out/(1+MIPI_DIV)
* @return	none
*/
void drv_l2_mipi_ctrl_set_clk(INT8U mipiclk_on, INT32U mipiclk_div)
{
    INT32U peri_clk, mipiclk;

    peri_clk = drv_l1_clock_get_peripheral_clock_source_freq();
    if(mipiclk_div>0)
        mipiclk = peri_clk/mipiclk_div;
    else
        mipiclk = peri_clk;
    DBG_PRINT("peri_clk/div =%d/%d = %d\r\n",peri_clk,mipiclk_div, mipiclk);

    if (mipiclk_on)// enable mipi clk
        drv_l1_clock_set_peripheral_clock_enable(SRC_MIPI, ENABLE);

     drv_l1_clock_set_peripheral_clock_freq(SRC_MIPI, mipiclk);
}


INT32U drv_l2_Isp_crop_wsize(INT32U clip_wsize)
{
	return clip_wsize;
}

static void drv_l2_sensor_frame_range_clip_isr()
{
	INT32U	Wfactor, Hfactor;
	//INT8U	SP_status, UVF_status;
    INT32U data_source;
	INT8U dn3r_en;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	frame_width_after_clip = gframeRangeArgs.clip_w;		//for MD

    data_source = drv_l1_CdspGetDataSource();
	dn3r_en = drv_l1_IspGet3dnr_en();
	drv_l1_CdspSetDataSource(1);							//SDRAM,Close MIPI0 Sensor Data
    drv_l1_IspSet3dnr_en(0);

	drv_l2_Isp_crop_wsize(gframeRangeArgs.clip_w);
	drv_l2_cdsp_extand_h_blank(gframeRangeArgs.frame_w);
	if(gframeRangeArgs.docropflag)
	{
		drv_l1_CdspSetCrop(gframeRangeArgs.point_x, gframeRangeArgs.point_y, gframeRangeArgs.clip_w, gframeRangeArgs.clip_h);
		drv_l1_CdspSetCropEn(gframeRangeArgs.docropflag);	// 1:Enable, 0:Disable
		gframeRangeArgs.frame_w = gframeRangeArgs.clip_w;
		gframeRangeArgs.frame_h = gframeRangeArgs.clip_h;
	}
	else
	{
		drv_l1_CdspSetCropEn(gframeRangeArgs.docropflag);	// 1:Enable, 0:Disable
	}

	// Scale Down
	if(gframeRangeArgs.scaledown_w == gframeRangeArgs.frame_w)
	{
		Wfactor = 0;
	}
	else
	{
		Wfactor = ((gframeRangeArgs.scaledown_w) << 16) / gframeRangeArgs.frame_w + 1;	// for 640x480
	}

	if(gframeRangeArgs.scaledown_h == gframeRangeArgs.frame_h)
	{
		Hfactor = 0;
	}
	else
	{
		Hfactor = ((((gframeRangeArgs.scaledown_h) << 16) / gframeRangeArgs.frame_h)+1);
	}

	drv_l1_CdspSetYuvHScale(Wfactor, Wfactor);
	drv_l1_CdspSetYuvVScale(Hfactor, Hfactor);

	if(Wfactor == 0)
	{
		drv_l1_CdspSetYuvHScaleEn(0, 1);
		DBG_PRINT("DISABLE YUV Hscale\r\n");
	}
	else
	{
		drv_l1_CdspSetYuvHScaleEn(gframeRangeArgs.doscaledownflag, 1);
	}

	if(Hfactor == 0)
	{
		drv_l1_CdspSetYuvVScaleEn(0, 1);
		DBG_PRINT("DISABLE YUV Vscale\r\n");
	}
	else
	{
		drv_l1_CdspSetYuvVScaleEn(gframeRangeArgs.doscaledownflag, 1);
	}

	if(gframeRangeArgs.docropflag)
	{
		drv_l1_CdspSetExtLine(gframeRangeArgs.clip_w, gframeRangeArgs.frame_w);
		drv_l2_CdspSetFifoSize(gframeRangeArgs.clip_w);

		drv_l2_cdsp_ReDoConfig(gframeRangeArgs.clip_w, gframeRangeArgs.clip_h);
	}
	else
	{
		drv_l1_CdspSetExtLine(gframeRangeArgs.frame_w, gframeRangeArgs.frame_w);
		drv_l2_CdspSetFifoSize(gframeRangeArgs.frame_w);
		drv_l2_cdsp_ReDoConfig(gframeRangeArgs.frame_w, gframeRangeArgs.frame_h);
	}
	drv_l1_CdspSetClampEn(1, gframeRangeArgs.scaledown_w);

	drv_l1_CdspSetYuvBuffA(gframeRangeArgs.scaledown_w, gframeRangeArgs.scaledown_h, gframeRangeArgs.frameAddrs);
	drv_l1_CdspSetYuvBuffB(gframeRangeArgs.scaledown_w, gframeRangeArgs.scaledown_h, gframeRangeArgs.frameAddrs);
#if C_MOTION_DETECTION == CUSTOM_ON
    if(motion_detect_status_get() == 1)
	{
        sensor_frame_clip_done(1);
    }
	else
	{
        sensor_frame_clip_done(0);
    }
#endif

	//cdsp sw reset
    drv_l1_CdspReset();
	drv_l1_IspSet3dnr_en(dn3r_en);
	R_CDSP_DO = data_source;
	xSemaphoreGiveFromISR(sem_clip, &xHigherPriorityTaskWoken);
}

INT32S drv_l2_sensor_frame_range_clip(sensor_frame_range_t *frameRange)
{
	INT16U	time_cnt = 1000;
	BaseType_t	err;

    if(!sem_clip)
	{
		sem_clip = xSemaphoreCreateBinary();
	}
	gp_memcpy((INT8S *) &gframeRangeArgs, (INT8S *) frameRange, sizeof(sensor_frame_range_t));
	while(drv_l2_cdsp_isr_once_set(drv_l2_sensor_frame_range_clip_isr) != STATUS_OK)
	{
		if(!--time_cnt)
		{
			break;
		}
		vTaskDelay(10);
	}
	if(time_cnt)
	{
		err = xSemaphoreTake(sem_clip, time_cnt);
		if(err == pdFALSE)
		{
            xSemaphoreGiveFromISR( sem_clip, &err);
			return STATUS_FAIL;
		}
		DBG_PRINT("==drv_l2_sensor_frame_range_clip OK\r\n");
		return STATUS_OK;
	}
	else
	{
		DBG_PRINT("==drv_l2_sensor_frame_range_clip fail\r\n");
		return STATUS_FAIL;
	}

}

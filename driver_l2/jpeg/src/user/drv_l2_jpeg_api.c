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
#include "drv_l2.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define	C_READ_SIZE			40*1024
#define PRE_JPEG_SCALE_EN	1

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#ifndef DISABLE
#define DISABLE     0
#endif

#ifndef ENABLE
#define ENABLE      1
#endif

#define MSG(args...)

//#define gp_malloc(size)                     pvPortMalloc(size)
//#define gp_malloc_align(size, align)        gp_malloc(size)
//#define gp_iram_malloc_align(size, align)   gp_malloc(size)
//#define gp_free(ptr)                        vPortFree((void *)ptr);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static INT32U gJpegYAddr, gJpegCbAddr, gJpegCrAddr;


/**
 * @brief   tool function, seek jpeg header within data buffer
 * @param   raw_data_addr:	vlc data buffer
 * @param   cblen:			vlc data buffer size
 * @return  offset
 */
static INT32S jpeg_api_seek_to_jpeg_header(INT32U raw_data_addr, INT32U cblen)
{
	INT8U *pdata;
	INT8S cnt;
	INT16U wdata;
	INT32U i;

	cnt = 0;
	//seek to data
	pdata = (INT8U*)raw_data_addr;
	wdata = *pdata++;
	wdata <<= 8;
	wdata |= *pdata++;

	for(i=0; i<cblen; i++)
	{
		if(wdata == 0xFFD8) {
			break;
		}

		wdata <<= 8;
		wdata |= *pdata++;
		cnt++;
	}

	if(i == cblen) {
		return -1;
	}

	return cnt;
}

#if 0 //unused function, keep it for reference
/**
 * @brief   tool function, check if vlc buffer contain EOF mark (0xFFD9)
 * @param   raw_data_addr:	vlc data buffer
 * @param   size:			vlc data buffer size
 * @return  0 TRUE/-1 FAIL
 */
static INT32S jpeg_api_check_end_of_jpeg(INT32U raw_data_addr, INT32U size)
{
	INT8U *pdata;
	INT16U wdata;
	INT32U i;

	pdata = (INT8U*)(raw_data_addr + size - 1);
	wdata = *pdata--;
	wdata |= (*pdata--) << 8;

	for(i=0; i < 1024; i++) {
		if(wdata == 0xFFD9) {
			break;
		}

		wdata >>= 8;
		wdata |= (*pdata--) << 8;
	}

	if(i == 1024) {
		return -1;
	}
	return 0;
}
#endif

/**
 * @brief   tool function, allocate buffers for jpeg decode
 * @return  0 SUCCESS/-1 FAIL
 */
static INT32S jpeg_api_fifo_mem_alloc(JpegDec_t *pJpegDec)
{
	INT16U cbcr_shift;
	INT32U y_size=0, cb_cr_size=0;

	if (pJpegDec->jpeg_yuv_mode == C_JPG_CTRL_YUV420) {
		MSG("C_JPG_CTRL_YUV420\r\n");
		cbcr_shift = 2;
	}
	else if (pJpegDec->jpeg_yuv_mode == C_JPG_CTRL_YUV411) {
		MSG("C_JPG_CTRL_YUV411\r\n");
		cbcr_shift = 2;
	}
	else if (pJpegDec->jpeg_yuv_mode == C_JPG_CTRL_YUV422) {
		MSG("C_JPG_CTRL_YUV422\r\n");
		cbcr_shift = 1;
	}
	else if (pJpegDec->jpeg_yuv_mode == C_JPG_CTRL_YUV422V) {
		MSG("C_JPG_CTRL_YUV422V\r\n");
		cbcr_shift = 1;
	}
	else if (pJpegDec->jpeg_yuv_mode == C_JPG_CTRL_YUV444) {
		MSG("C_JPG_CTRL_YUV444\r\n");
		cbcr_shift = 0;
	}
	else if (pJpegDec->jpeg_yuv_mode == C_JPG_CTRL_GRAYSCALE) {
		MSG("C_JPG_CTRL_GRAYSCALE\r\n");
		cbcr_shift = 32;
	}
	else if (pJpegDec->jpeg_yuv_mode == C_JPG_CTRL_YUV411V) {
		MSG("C_JPG_CTRL_YUV411V\r\n");
		cbcr_shift = 2;
	}
	else if (pJpegDec->jpeg_yuv_mode == C_JPG_CTRL_YUV420H2) {
		MSG("C_JPG_CTRL_YUV420H2\r\n");
		cbcr_shift = 1;
	}
	else if (pJpegDec->jpeg_yuv_mode == C_JPG_CTRL_YUV420V2) {
		MSG("C_JPG_CTRL_YUV420V2\r\n");
		cbcr_shift = 1;
	}
	else if (pJpegDec->jpeg_yuv_mode == C_JPG_CTRL_YUV411H2) {
		MSG("C_JPG_CTRL_YUV411H2\r\n");
		cbcr_shift = 1;
	}
	else if (pJpegDec->jpeg_yuv_mode == C_JPG_CTRL_YUV411V2) {
		MSG("C_JPG_CTRL_YUV411V2\r\n");
		cbcr_shift = 1;
	}
	else {
		return STATUS_FAIL;
	}

	if (pJpegDec->fifo_line) {
		switch(pJpegDec->jpeg_yuv_mode)
		{
		case C_JPG_CTRL_YUV444:
		case C_JPG_CTRL_YUV422:
		case C_JPG_CTRL_YUV422V:
		case C_JPG_CTRL_YUV420:
		case C_JPG_CTRL_YUV411:
			y_size = pJpegDec->jpeg_extend_w * pJpegDec->fifo_line * 2 * 2; //YUYV
			cb_cr_size = 0;
			break;

		case C_JPG_CTRL_GRAYSCALE:
		case C_JPG_CTRL_YUV411V:
		case C_JPG_CTRL_YUV420H2:
		case C_JPG_CTRL_YUV420V2:
		case C_JPG_CTRL_YUV411H2:
		case C_JPG_CTRL_YUV411V2:
			y_size = pJpegDec->jpeg_extend_w * pJpegDec->fifo_line*2;
			cb_cr_size = y_size >> cbcr_shift;
		}
	}
	else {
		y_size = pJpegDec->jpeg_extend_w * pJpegDec->jpeg_extend_h;
		cb_cr_size = y_size >> cbcr_shift;
	}

	// allocate internal first
	gJpegYAddr = (INT32U) gp_iram_malloc_align(y_size + cb_cr_size + cb_cr_size, 16);
	if (gJpegYAddr == 0) {
		gJpegYAddr = (INT32U) gp_malloc_align(y_size + cb_cr_size + cb_cr_size, 16);
		if (gJpegYAddr == 0) {
			return STATUS_FAIL;
		}
	}

	if(cb_cr_size) {
		gJpegCbAddr = gJpegYAddr + y_size;
		gJpegCrAddr = gJpegCbAddr + cb_cr_size;
	}

	return STATUS_OK;
}

/**
 * @brief   tool function, free global buffers
 * @return  0 SUCCESS/-1 FAIL
 */
static void jpeg_api_fifo_mem_free(void)
{
	if(gJpegYAddr) {
		gp_free((void*)gJpegYAddr);
	}

	gJpegYAddr = 0;
	gJpegCbAddr = 0;
	gJpegCrAddr = 0;
}

/**
 * @brief   tool function, set SCALER module for jpeg decode output
 * @return  none
 */
static void jpeg_api_set_scaler(JpegDec_t *pJpegDec, ScalerFormat_t *pScale, ScalerPara_t *pScalePara)
{
	if (pJpegDec->output_w == 0) {
	    pJpegDec->output_w = pJpegDec->output_buffer_w;
	}

	if (pJpegDec->output_h == 0) {
	    pJpegDec->output_h = pJpegDec->output_buffer_h;
	}

	switch(pJpegDec->jpeg_yuv_mode)
	{
	case C_JPG_CTRL_YUV444:
	case C_JPG_CTRL_YUV422:
	case C_JPG_CTRL_YUV420:
	case C_JPG_CTRL_YUV411:
	case C_JPG_CTRL_YUV422V:
		pScale->input_format = C_SCALER_CTRL_IN_YUYV;
		break;

	case C_JPG_CTRL_GRAYSCALE:
		pScale->input_format = C_SCALER_CTRL_IN_Y_ONLY;
		break;

	case C_JPG_CTRL_YUV411V:
		pScale->input_format = C_SCALER_CTRL_IN_YUV411V;
		break;

	case C_JPG_CTRL_YUV420H2:
		pScale->input_format = C_SCALER_CTRL_IN_YUV422V;
		break;

	case C_JPG_CTRL_YUV420V2:
		pScale->input_format = C_SCALER_CTRL_IN_YUV422;
		break;

	case C_JPG_CTRL_YUV411H2:
		pScale->input_format = C_SCALER_CTRL_IN_YUV422;
		break;

	case C_JPG_CTRL_YUV411V2:
		pScale->input_format = C_SCALER_CTRL_IN_YUV422V;
		break;

	default:
		while(1);
	}

	pScale->input_width = pJpegDec->jpeg_valid_w;
	pScale->input_height = pJpegDec->jpeg_valid_h;
	pScale->input_visible_width = pJpegDec->jpeg_valid_w;
	pScale->input_visible_height = pJpegDec->jpeg_valid_h;
	pScale->input_x_offset = 0;
	pScale->input_y_offset = 0;

	pScale->input_y_addr = gJpegYAddr;
	pScale->input_u_addr = gJpegCbAddr;
	pScale->input_v_addr = gJpegCrAddr;

	pScale->output_format = pJpegDec->output_format;
	pScale->output_width = pJpegDec->output_w;
	pScale->output_height = pJpegDec->output_h;
	pScale->output_buf_width = pJpegDec->output_buffer_w;
	pScale->output_buf_height = pJpegDec->output_buffer_h;
	pScale->output_x_offset = 0;

	pScale->output_y_addr = pJpegDec->output_addr;
	pScale->output_u_addr = 0;
	pScale->output_v_addr = 0;

	pScale->fifo_mode = pJpegDec->scaler_fifo;
	pScale->scale_mode = pJpegDec->scaler_mode;
	pScale->digizoom_m = 10;
	pScale->digizoom_n = 10;

	pScalePara->boundary_mode = 1;
	pScalePara->boundary_color = pJpegDec->output_boundary_color;
}

/**
 * @brief   tool function, set union mode pre scale down
 * @return  none
 */
static void jpeg_union_mode_pre_scale_down_set(JpegDec_t *pJpegDec)
{
#if PRE_JPEG_SCALE_EN == 1
	if((pJpegDec->output_w <= (pJpegDec->jpeg_valid_w>>2)) && (pJpegDec->output_h <= (pJpegDec->jpeg_valid_h>>2))) {
		MSG("PreScale = 1/4 in Union Mode\r\n");
		drv_l1_jpeg_decode_scale_down_set(pJpegDec->hwidx,ENUM_JPG_DIV4);	// Use 1/4 scale-down mode
		pJpegDec->jpeg_valid_w >>= 2;
		pJpegDec->jpeg_valid_h >>= 2;
		pJpegDec->jpeg_extend_w >>= 2;
		pJpegDec->jpeg_extend_h >>= 2;
	}
	else if((pJpegDec->output_w <= (pJpegDec->jpeg_valid_w>>1)) && (pJpegDec->output_h <= (pJpegDec->jpeg_valid_h>>1))) {
		MSG("PreScale = 1/2 in Union Mode\r\n");
		drv_l1_jpeg_decode_scale_down_set(pJpegDec->hwidx,ENUM_JPG_DIV2);	// Use 1/2 scale-down mode
		pJpegDec->jpeg_valid_w >>= 1;
		pJpegDec->jpeg_valid_h >>= 1;
		pJpegDec->jpeg_extend_w >>= 1;
		pJpegDec->jpeg_extend_h >>= 1;
	}
	else {
		drv_l1_jpeg_decode_scale_down_set(pJpegDec->hwidx,ENUM_JPG_NO_SCALE_DOWN);
	}
#else
	drv_l1_jpeg_decode_scale_down_set(pJpegDec->hwidx, ENUM_JPG_NO_SCALE_DOWN);
#endif
}

/**
 * @brief   tool function, set pre scale down
 * @return  none
 */
static void jpeg_pre_scale_down_set(JpegDec_t *pJpegDec)
{
#if PRE_JPEG_SCALE_EN
	if((pJpegDec->output_w <= (pJpegDec->jpeg_valid_w>>1)) && (pJpegDec->output_h <= (pJpegDec->jpeg_valid_h>>1))) {
		MSG("PreScale = 1/2\r\n");
		jpeg_decode_level2_scaledown_enable(pJpegDec->hwidx);	// Use 1/2 scale-down mode
		pJpegDec->jpeg_valid_w >>= 1;
		pJpegDec->jpeg_valid_h >>= 1;
		pJpegDec->jpeg_extend_w >>= 1;
		pJpegDec->jpeg_extend_h >>= 1;
	}
	else {
		drv_l1_jpeg_level2_scaledown_mode_disable(pJpegDec->hwidx);
	}
	drv_l1_jpeg_level2_scaledown_mode_disable(pJpegDec->hwidx);
#endif
}

/**
 * @brief   tool funciton, decode jpeg without scaler
 * @return  0 SUCCESS/-1 FAIL
 */
static INT32S jpeg_decode_without_scaler(JpegDec_t *pJpegDec)
{
	INT8U *p_vlc;
	INT32U fly_len, header_len;
	INT32S ret;
	INT32U div;

	MSG("%s\r\n", __func__);

	ret = jpeg_decode_output_set(pJpegDec->hwidx, pJpegDec->output_addr, DUMMY_BUFFER_ADDRS, DUMMY_BUFFER_ADDRS, C_JPG_FIFO_DISABLE);
	if(ret < 0) {
		MSG("jpeg_decode_output_set() failed\r\n");
		ret = STATUS_FAIL;
		goto __exit;
	}

	div = pJpegDec->jpeg_valid_w / pJpegDec->output_w;

	if (div == 2)
		drv_l1_jpeg_decode_scale_down_set(pJpegDec->hwidx, ENUM_JPG_DIV2);
	else if (div == 4)
		drv_l1_jpeg_decode_scale_down_set(pJpegDec->hwidx, ENUM_JPG_DIV4);
	else
		drv_l1_jpeg_decode_scale_down_set(pJpegDec->hwidx, ENUM_JPG_NO_SCALE_DOWN);

	p_vlc = jpeg_decode_image_vlc_addr_get(pJpegDec->hwidx);
	header_len = ((INT32U) p_vlc) - pJpegDec->raw_data_addr;
	fly_len = pJpegDec->raw_data_size - header_len;

	ret = drv_l1_jpeg_vlc_addr_set(pJpegDec->hwidx, (INT32U) p_vlc);
	if (ret < 0) {
		MSG("Calling to jpeg_vlc_addr_set() failed\r\n");
		ret = STATUS_FAIL;
		goto __exit;
	}

	ret = drv_l1_jpeg_vlc_maximum_length_set(pJpegDec->hwidx, fly_len);
	if (ret < 0) {
		MSG("Calling to jpeg_vlc_maximum_length_set() failed\r\n");
		ret = STATUS_FAIL;
		goto __exit;
	}

	if (pJpegDec->output_format == C_SCALER_CTRL_OUT_ARGB8888 || pJpegDec->output_format == C_SCALER_CTRL_OUT_YUV444) {
		if (pJpegDec->output_format == C_SCALER_CTRL_OUT_ARGB8888)
			drv_l1_jpeg_out_format_set(pJpegDec->hwidx, C_JPG_DECOUT_FMT_ARGB8888);
		else
			drv_l1_jpeg_out_format_set(pJpegDec->hwidx, C_JPG_DECOUT_FMT_AYUV8888);

		if (pJpegDec->input_alpha_addr) {
			drv_l1_jpeg_decode_alpha_set(pJpegDec->hwidx, 0, 0);
			jpeg_decode_output_set(pJpegDec->hwidx, pJpegDec->output_addr, pJpegDec->input_alpha_addr, 0x50000000, C_JPG_FIFO_DISABLE);
		}
		else {
			drv_l1_jpeg_decode_alpha_set(pJpegDec->hwidx, 1, 0x80);
		}
	}
	else if (pJpegDec->output_format == C_SCALER_CTRL_OUT_YUYV || pJpegDec->output_format == C_SCALER_CTRL_OUT_Y_ONLY)
		drv_l1_jpeg_out_format_set(pJpegDec->hwidx, C_JPG_DECOUT_FMT_YUYV);
	else
		drv_l1_jpeg_out_format_set(pJpegDec->hwidx, C_JPG_DECOUT_FMT_GP420);

	ret = drv_l1_jpeg_decompression_start(pJpegDec->hwidx, NULL);
	if (ret < 0) {
		MSG("Calling to jpeg_decompression_start() failed\r\n");
		ret = STATUS_FAIL;
		goto __exit;
	}

	while (1) {
		ret = drv_l1_jpeg_status_wait_idle(pJpegDec->hwidx, TRUE);
		if(ret & C_JPG_STATUS_DECODE_DONE) {
			break;
		}

		if(ret & (C_JPG_STATUS_STOP|C_JPG_STATUS_TIMEOUT|C_JPG_STATUS_INIT_ERR)) {
			ret = STATUS_FAIL;
			goto __exit;
		}
	}

	ret = STATUS_OK;
__exit:
	drv_l1_jpeg_stop(pJpegDec->hwidx);
	return ret;
}

/**
 * @brief   tool funciton, decode jpeg with scaler
 * @return  0 SUCCESS/-1 FAIL
 */
static INT32S jpeg_decode_and_scaler(JpegDec_t *pJpegDec)
{
	INT8U  *p_vlc;
	INT8U  scaler_done;
	INT32S jpeg_status, scaler_status;
	INT32S fly_len, header_len, ret;
	ScalerFormat_t scale;
	ScalerPara_t para;

	MSG("%s\r\n", __func__);

	ret = jpeg_api_fifo_mem_alloc(pJpegDec);
	if(ret < 0) {
		ret = STATUS_FAIL;
		goto __exit;
	}

	ret = jpeg_decode_output_set(pJpegDec->hwidx, gJpegYAddr, gJpegCbAddr, gJpegCrAddr, pJpegDec->jpeg_fifo);
	if(ret < 0) {
		ret = STATUS_FAIL;
		goto __exit;
	}

	p_vlc = jpeg_decode_image_vlc_addr_get(pJpegDec->hwidx);
	header_len = ((INT32U) p_vlc) - pJpegDec->raw_data_addr;
	fly_len = pJpegDec->raw_data_size - header_len;

	// Set maximum VLC length to prevent JPEG from hangging too long
	ret = jpeg_decode_vlc_maximum_length_set(pJpegDec->hwidx, fly_len);
	if(ret < 0) {
		ret = STATUS_FAIL;
		goto __exit;
	}

	// Now start JPEG decoding on the fly
	if(fly_len > C_READ_SIZE) {
		ret = ((INT32U)p_vlc) + C_READ_SIZE;
		ret &= ~0x3F;
		fly_len = ret - ((INT32U)p_vlc);	//64 align
	}

	ret = jpeg_decode_on_the_fly_start(pJpegDec->hwidx, p_vlc, fly_len);
	if(ret < 0) {
		ret = STATUS_FAIL;
		goto __exit;
	}

	// Setup Scaler
	scaler_done = 0;
	scaler_status = C_SCALER_STATUS_STOP;
	memset((void *)&scale, 0, sizeof(scale));
	memset((void *)&para, 0, sizeof(para));
	jpeg_api_set_scaler(pJpegDec, &scale, &para);

  	while(1) {
  		jpeg_status = jpeg_decode_status_query(pJpegDec->hwidx, 1);

		if(jpeg_status & C_JPG_STATUS_DECODE_DONE) {
			// Wait until scaler finish its job
			while(1) {
				if(scaler_status == C_SCALER_STATUS_STOP) {
					scaler_status = drv_l2_scaler_trigger(SCALER_0, ENABLE, &scale, &para);
				}
				else if(scaler_status & C_SCALER_STATUS_INPUT_EMPTY) {
					scaler_status = drv_l2_scaler_retrigger(SCALER_0, &scale);
				}
				else if(scaler_status & C_SCALER_STATUS_TIMEOUT) {
					MSG("Scale1Timeout.\r\n");
					break;
				}
				else if(scaler_status & C_SCALER_STATUS_INIT_ERR) {
					MSG("Scale1InitErr.\r\n");
					break;
				}
				else {
				 	MSG("Scale1StatusErr = 0x%x\r\n", scaler_status);
				  	break;
				}

				if(scaler_status == C_SCALER_STATUS_STOP) {
					break;
				}
			}
			break;
		}

  		if(jpeg_status & C_JPG_STATUS_INPUT_EMPTY) {
			p_vlc += fly_len;
			ret = ((INT32U)p_vlc) + C_READ_SIZE;
			ret &= ~0x0F;
			fly_len = ret - ((INT32U)p_vlc);

			// Now restart JPEG decoding on the fly
			ret = jpeg_decode_on_the_fly_start(pJpegDec->hwidx, p_vlc, fly_len);
		  	if(ret < 0) {
		  		ret = STATUS_FAIL;
		  		goto __exit;
		  	}
		}

		if(jpeg_status & C_JPG_STATUS_OUTPUT_FULL) {
			// Start scaler to handle the full output FIFO now
		  	if(scaler_done == 0) {
				// scale1 run
		  		if(scaler_status == C_SCALER_STATUS_STOP) {
					scaler_status = drv_l2_scaler_trigger(SCALER_0, ENABLE, &scale, &para);
				}
				else if(scaler_status & C_SCALER_STATUS_INPUT_EMPTY) {
					scaler_status = drv_l2_scaler_retrigger(SCALER_0, &scale);
				}
				else if(scaler_status & C_SCALER_STATUS_TIMEOUT) {
					MSG("Scale1Timeout\r\n");
					break;
				}
				else if(scaler_status & C_SCALER_STATUS_INIT_ERR) {
					MSG("Scale1InitErr\r\n");
					break;
		  		}
		  		else {
			  		MSG("Scale1StatusErr = 0x%x\r\n", scaler_status);
			  		break;
			  	}

				if(scaler_status == C_SCALER_STATUS_STOP) {
					scaler_done = 1;
				}
			}
	  		// Now restart JPEG to output to next FIFO
	  		if(jpeg_decode_output_restart(pJpegDec->hwidx))
	  		{
	  			MSG("Failed to call jpeg_decode_output_restart()\r\n");
	  			break;
	  		}
		}

		if(jpeg_status & C_JPG_STATUS_STOP) {
			MSG("JPEG is not started!\r\n");
			break;
		}

		if(jpeg_status & C_JPG_STATUS_TIMEOUT) {
			MSG("JPEG execution timeout!\r\n");
			break;
		}

		if(jpeg_status & C_JPG_STATUS_INIT_ERR) {
			MSG("JPEG init error!\r\n");
			break;
		}

		if(jpeg_status & C_JPG_STATUS_RST_VLC_DONE) {
			MSG("JPEG Restart marker number is incorrect!\r\n");
			break;
		}

		if(jpeg_status & C_JPG_STATUS_RST_MARKER_ERR) {
			MSG("JPEG Restart marker sequence error!\r\n");
			break;
		}
  	}

  	ret = STATUS_OK;
__exit:
	jpeg_decode_stop(pJpegDec->hwidx);
  	jpeg_api_fifo_mem_free();
	return ret;
}

/**
 * @brief   get jpeg width and height and format
 * @param	idx:			jpeg idx
 * @param	raw_data_addr:	input vlc data
 * @param	width:			pointer to store width
 * @param	height:			pointer to store height
 * @param	img_yuv_mode:	pointer to store image format
 * @return  0 SUCCESS/-1 FAIL
 */
INT32S jpeg_api_decode_get_size(JPEG_IDX idx, INT32U raw_data_addr, INT32U size, INT16U *width, INT16U *height, INT16U *img_yuv_mode)
{
	INT32S nRet;

	nRet = jpeg_api_seek_to_jpeg_header(raw_data_addr, size);
	if(nRet < 0) {
		nRet = STATUS_FAIL;
		goto Return;
	}

	size -= nRet;
	raw_data_addr += nRet;
	nRet = jpeg_decode_parse_header(idx, (INT8U *) raw_data_addr, size);
	if(nRet != JPEG_PARSE_OK) {
		nRet = STATUS_FAIL;
		goto Return;
	}

	*width = jpeg_decode_image_width_get(idx);
	*height = jpeg_decode_image_height_get(idx);
	*img_yuv_mode = jpeg_decode_image_yuv_mode_get(idx);
	nRet = STATUS_OK;
Return:
	return nRet;
}

/**
 * @brief   decode jpeg
 * @param	pJpegDec:		pointer to decode parameters
 * @return  0 SUCCESS/-1 FAIL
 */
INT32S jpeg_api_decode_output(JpegDec_t *pJpegDec)
{
	INT32S ret, div, scale;

	MSG("%s\r\n", __func__);

	ret = jpeg_api_seek_to_jpeg_header(pJpegDec->raw_data_addr, pJpegDec->raw_data_size);
	if(ret < 0) {
		return -1;
	}

	pJpegDec->raw_data_size -= ret;
	pJpegDec->raw_data_addr += ret;

	jpeg_decode_init(pJpegDec->hwidx);
	ret = jpeg_decode_parse_header(pJpegDec->hwidx, (INT8U *) pJpegDec->raw_data_addr, pJpegDec->raw_data_size);
	if(ret != JPEG_PARSE_OK) {
		jpeg_decode_stop(pJpegDec->hwidx);
		ret = STATUS_FAIL;
		goto __exit;
	}

	if(pJpegDec->scaler_mode == 0) {
		pJpegDec->scaler_mode = C_SCALER_BY_RATIO;
	}

	if(pJpegDec->fifo_line == 0) {
		pJpegDec->jpeg_fifo = C_JPG_FIFO_32LINE;
		pJpegDec->scaler_fifo = C_SCALER_CTRL_FIFO_32LINE;
		pJpegDec->fifo_line = 32;
	}

	pJpegDec->jpeg_yuv_mode = jpeg_decode_image_yuv_mode_get(pJpegDec->hwidx);
	pJpegDec->jpeg_valid_w = jpeg_decode_image_width_get(pJpegDec->hwidx);
	pJpegDec->jpeg_valid_h = jpeg_decode_image_height_get(pJpegDec->hwidx);
	pJpegDec->jpeg_extend_w = jpeg_decode_image_extended_width_get(pJpegDec->hwidx);
	pJpegDec->jpeg_extend_h = jpeg_decode_image_extended_height_get(pJpegDec->hwidx);

	MSG("jpeg valid = %dx%d\r\n", pJpegDec->jpeg_valid_w, pJpegDec->jpeg_valid_h);
	MSG("jpeg extend = %dx%d\r\n", pJpegDec->jpeg_extend_w, pJpegDec->jpeg_extend_h);
	switch(pJpegDec->output_format)
	{
	case C_SCALER_CTRL_OUT_YUYV:		MSG("output YUYV\r\n");		break;
	case C_SCALER_CTRL_OUT_GP420:		MSG("output GP420\r\n");	break;
	case C_SCALER_CTRL_OUT_ARGB8888:	MSG("output ARGB8888 with %s alpha\r\n", pJpegDec->input_alpha_addr ? "external" : "fixed");	break;
	case C_SCALER_CTRL_OUT_YUV444:		MSG("output AYUV8888 with %s alpha\r\n", pJpegDec->input_alpha_addr ? "external" : "fixed");	break;
	case C_SCALER_CTRL_OUT_Y_ONLY:		MSG("output YONLY\r\n");	break;
	}

	if(pJpegDec->output_w == 0) {
		pJpegDec->output_w = pJpegDec->jpeg_valid_w;
	}

	if(pJpegDec->output_buffer_w == 0) {
		pJpegDec->output_buffer_w = pJpegDec->output_w;
	}

	if(pJpegDec->output_h == 0) {
		pJpegDec->output_h = pJpegDec->jpeg_valid_h;
	}

	if(pJpegDec->output_buffer_h == 0) {
		pJpegDec->output_buffer_h = pJpegDec->output_h;
	}

	switch(pJpegDec->jpeg_yuv_mode)
	{
	case C_JPG_CTRL_YUV444:
	case C_JPG_CTRL_YUV422:
	case C_JPG_CTRL_YUV420:
	case C_JPG_CTRL_YUV411:
	case C_JPG_CTRL_YUV422V:
		// use union mode
		drv_l1_jpeg_using_union_mode_enable(pJpegDec->hwidx);

		jpeg_union_mode_pre_scale_down_set(pJpegDec);

		scale = 1;
		if((pJpegDec->output_w == pJpegDec->jpeg_valid_w && pJpegDec->output_h == pJpegDec->jpeg_valid_h) ||
			(pJpegDec->output_w == pJpegDec->jpeg_valid_w >> 1 && pJpegDec->output_h == pJpegDec->jpeg_valid_h >> 1) ||
			(pJpegDec->output_w == pJpegDec->jpeg_valid_w >> 2 && pJpegDec->output_h == pJpegDec->jpeg_valid_h >> 2)) {
			scale = 0;
			div = pJpegDec->jpeg_valid_w / pJpegDec->output_w;
			if ((pJpegDec->output_format == C_SCALER_CTRL_OUT_GP420) && (div == 4)) //GP420 not support div 4
				scale = 1;
		}
		if (!scale) {
			ret = jpeg_decode_without_scaler(pJpegDec);
		}
		else {
			ret = jpeg_decode_and_scaler(pJpegDec);
		}
		break;

	case C_JPG_CTRL_GRAYSCALE:
	case C_JPG_CTRL_YUV411V:
	case C_JPG_CTRL_YUV420H2:
	case C_JPG_CTRL_YUV420V2:
	case C_JPG_CTRL_YUV411H2:
	case C_JPG_CTRL_YUV411V2:
		// use YUV seperate mode and fifo mode
		drv_l1_jpeg_using_union_mode_disable(pJpegDec->hwidx);

		jpeg_pre_scale_down_set(pJpegDec);

		if(pJpegDec->output_format == C_SCALER_CTRL_OUT_Y_ONLY &&
			pJpegDec->output_w == pJpegDec->jpeg_valid_w && pJpegDec->output_h == pJpegDec->jpeg_valid_h)
			ret = jpeg_decode_without_scaler(pJpegDec);
		else
			ret = jpeg_decode_and_scaler(pJpegDec);
		break;
	}

__exit:
	return ret;
}

/**
 * @brief   encode jpeg
 * @param	pJpegEnc:		pointer to encode parameters
 * @return  0 SUCCESS/-1 FAIL
 */
INT32S jpeg_api_encode_one_frame(JpegEnc_t *pJpegEnc)
{
	INT32S ret;

	jpeg_encode_init(pJpegEnc->hwidx);
	if (pJpegEnc->quality_value)
		gplib_jpeg_default_quantization_table_load(pJpegEnc->hwidx, pJpegEnc->quality_value);	// Load default qunatization table(quality)
	else if (pJpegEnc->q_update.qY && pJpegEnc->q_update.qUV) {
		jpeg_header_quality_set(pJpegEnc->hwidx, pJpegEnc->header, pJpegEnc->q_update.qY, pJpegEnc->q_update.qUV);
	}
	gplib_jpeg_default_huffman_table_load(pJpegEnc->hwidx);								// Load default huffman table
	ret = jpeg_encode_input_size_set(pJpegEnc->hwidx, pJpegEnc->width, pJpegEnc->height);
	if(ret < 0) {
		goto __exit;
	}

	jpeg_encode_input_format_set(pJpegEnc->hwidx, pJpegEnc->yuv_fmt);						// C_JPEG_FORMAT_YUYV / C_JPEG_FORMAT_YUV_SEPARATE
	switch(pJpegEnc->yuv_sample_fmt)
	{
	case JPEG_YUV422:
		ret = jpeg_encode_yuv_sampling_mode_set(pJpegEnc->hwidx, C_JPG_CTRL_YUV422);
		break;

	case JPEG_YUV420:
		ret = drv_l1_jpeg_yuv_sampling_mode_set(pJpegEnc->hwidx, C_JPG_CTRL_GP420 | C_JPG_CTRL_YUV420);
		break;

	default:
		ret = STATUS_FAIL;
	}

	if(ret < 0) {
		goto __exit;
	}

	switch(pJpegEnc->scale_mode)
	{
	case JPEG_SCALE_VX2:
		drv_l1_jpeg_encode_scale_up_set(pJpegEnc->hwidx, ENUM_JPG_SCALE_VX2);
		break;
	case JPEG_SCALE_X2:
		drv_l1_jpeg_encode_scale_up_set(pJpegEnc->hwidx, ENUM_JPG_SCALE_X2);
		break;
	case JPEG_SCALE_YUYV_X4:
		drv_l1_jpeg_encode_scale_up_set(pJpegEnc->hwidx, ENUM_JPG_SCALE_X4);
		break;
	case JPEG_SCALE_GP420_X3D2:
		drv_l1_jpeg_encode_scale_up_set(pJpegEnc->hwidx, ENUM_JPG_SCALE_X1_5);
		break;
	case JPEG_SCALE_YUYV_X9D8:
		drv_l1_jpeg_encode_scale_up_set(pJpegEnc->hwidx, ENUM_JPG_SCALE_X9D8);
		break;
	default:
		break;
	}
	ret = jpeg_encode_output_addr_set(pJpegEnc->hwidx, (INT32U) pJpegEnc->output_buffer);
	if(ret < 0) {
		goto __exit;
	}

	cache_drain_range(pJpegEnc->output_buffer, pJpegEnc->output_buffer_len);

	ret = jpeg_encode_once_start(pJpegEnc->hwidx, pJpegEnc->input_buffer_y, pJpegEnc->input_buffer_u, pJpegEnc->input_buffer_v);
	if(ret < 0) {
		goto __exit;
	}

	while(1) {
		pJpegEnc->jpeg_status = jpeg_encode_status_query(pJpegEnc->hwidx, TRUE);
		if(pJpegEnc->jpeg_status & C_JPG_STATUS_ENCODE_DONE) {
			// Get encode length
			pJpegEnc->encode_size = jpeg_encode_vlc_cnt_get(pJpegEnc->hwidx);
			// MSG("Encode Size = %d\r\n", pJpegEnc->encode_size);
			drv_l1_cp15_invalidate_dcache_range(pJpegEnc->output_buffer, pJpegEnc->encode_size);
			break;
		}
		else if(pJpegEnc->jpeg_status & C_JPG_STATUS_ENCODING) {
			continue;
		}
		else {
			MSG("JPEG encode error!\r\n");
			ret = STATUS_FAIL;
			goto __exit;
		}
	}

	ret = STATUS_OK;
__exit:
	jpeg_encode_stop(pJpegEnc->hwidx);
	return ret;
}

/**
 * @brief   start encode jpeg in fifo mode and wait 1s fifo finish
 * @param	pJpegEnc:		pointer to encode parameters
 * @return  0 SUCCESS/-1 FAIL
 */
INT32S jpeg_api_encode_fifo_start(JpegEnc_t *pJpegEnc)
{
	INT32S ret;

	jpeg_encode_init(pJpegEnc->hwidx);
	gplib_jpeg_default_quantization_table_load(pJpegEnc->hwidx, pJpegEnc->quality_value);		// Load default qunatization table(quality=50)
	gplib_jpeg_default_huffman_table_load(pJpegEnc->hwidx);			        				// Load default huffman table
	ret = jpeg_encode_input_size_set(pJpegEnc->hwidx, pJpegEnc->width, pJpegEnc->height);
	if(ret < 0) {
		goto __exit;
	}

	jpeg_encode_input_format_set(pJpegEnc->hwidx, pJpegEnc->yuv_fmt);							// C_JPEG_FORMAT_YUYV / C_JPEG_FORMAT_YUV_SEPARATE
	switch(pJpegEnc->yuv_sample_fmt)
	{
	case JPEG_YUV422:
		ret = jpeg_encode_yuv_sampling_mode_set(pJpegEnc->hwidx, C_JPG_CTRL_YUV422);
		break;

	case JPEG_YUV420:
		ret = drv_l1_jpeg_yuv_sampling_mode_set(pJpegEnc->hwidx, C_JPG_CTRL_GP420 | C_JPG_CTRL_YUV420);
		break;

	default:
		ret = STATUS_FAIL;
	}

	if(ret < 0) {
		goto __exit;
	}

	ret = jpeg_encode_output_addr_set(pJpegEnc->hwidx, (INT32U)pJpegEnc->output_buffer);
	if(ret < 0) {
		goto __exit;
	}

	ret = jpeg_encode_maximum_length_set(pJpegEnc->hwidx, pJpegEnc->output_buffer_len);
	if(ret < 0) {
		goto __exit;
	}

	ret = jpeg_encode_on_the_fly_start(pJpegEnc->hwidx,
									   pJpegEnc->input_buffer_y,
									   pJpegEnc->input_buffer_u,
									   pJpegEnc->input_buffer_v,
									   pJpegEnc->input_y_len + pJpegEnc->input_uv_len + pJpegEnc->input_uv_len);
	if(ret < 0) {
		goto __exit;
	}

	/*if (pJpegEnc->wait_done == 0) {
		ret = STATUS_OK:
		goto __exit;
	}*/

	// wait jpeg block encode done
	pJpegEnc->jpeg_status = jpeg_encode_status_query(pJpegEnc->hwidx, TRUE);

	if(pJpegEnc->jpeg_status & (C_JPG_STATUS_TIMEOUT | C_JPG_STATUS_INIT_ERR |C_JPG_STATUS_RST_MARKER_ERR)) {
		ret = STATUS_FAIL;
		goto __exit;
	}

__exit:
	return ret;
}

/**
 * @brief   restart encode jpeg in fifo mode
 * @param	pJpegEnc:		pointer to encode parameters
 * @return  0 SUCCESS/-1 FAIL
 */
INT32S jpeg_api_encode_fifo_once(JpegEnc_t *pJpegEnc)
{
	INT32S ret = STATUS_OK;

	while(1) {
		if(pJpegEnc->jpeg_status & C_JPG_STATUS_ENCODE_DONE) {
			goto __exit;
		}
		else if(pJpegEnc->jpeg_status & C_JPG_STATUS_INPUT_EMPTY) {
			ret = jpeg_encode_on_the_fly_start(pJpegEnc->hwidx,
											   pJpegEnc->input_buffer_y,
											   pJpegEnc->input_buffer_u,
											   pJpegEnc->input_buffer_v,
											   pJpegEnc->input_y_len + pJpegEnc->input_uv_len + pJpegEnc->input_uv_len);
			if(ret < 0) {
				goto __exit;
			}

			// wait jpeg block encode done
			pJpegEnc->jpeg_status = jpeg_encode_status_query(pJpegEnc->hwidx, TRUE);
			goto __exit;
		}
		else if(pJpegEnc->jpeg_status & C_JPG_STATUS_STOP) {
			MSG("\r\njpeg encode is not started!\r\n");
			ret = STATUS_FAIL;
			goto __exit;
		}
		else if(pJpegEnc->jpeg_status & C_JPG_STATUS_TIMEOUT) {
			MSG("\r\njpeg encode execution timeout!\r\n");
			ret = STATUS_FAIL;
			goto __exit;
		}
		else if(pJpegEnc->jpeg_status & C_JPG_STATUS_INIT_ERR) {
			MSG("\r\njpeg encode init error!\r\n");
			ret = STATUS_FAIL;
			goto __exit;
		}
		else {
			MSG("\r\nJPEG status error = 0x%x!\r\n", pJpegEnc->jpeg_status);
			ret = STATUS_FAIL;
			goto __exit;
		}
	}

__exit:
	return ret;
}

/**
 * @brief   stop encode jpeg in fifo mode
 * @param	pJpegEnc:		pointer to encode parameters
 * @return  0 SUCCESS/-1 FAIL
 */
INT32S jpeg_api_encode_fifo_stop(JpegEnc_t *pJpegEnc)
{
	INT32S ret = STATUS_OK;

	while(1) {
		if(pJpegEnc->jpeg_status & C_JPG_STATUS_ENCODE_DONE) {
			// get jpeg encode size
			pJpegEnc->encode_size = jpeg_encode_vlc_cnt_get(pJpegEnc->hwidx);
			//MSG("Encode Size = %d\r\n", pJpegEnc->encode_size);
			drv_l1_cp15_invalidate_dcache_range(pJpegEnc->output_buffer, pJpegEnc->encode_size);
			goto __exit;
		}
		else if(pJpegEnc->jpeg_status & C_JPG_STATUS_INPUT_EMPTY) {
			ret = jpeg_encode_on_the_fly_start(pJpegEnc->hwidx,
											   pJpegEnc->input_buffer_y,
											   pJpegEnc->input_buffer_u,
											   pJpegEnc->input_buffer_v,
											   pJpegEnc->input_y_len + pJpegEnc->input_uv_len + pJpegEnc->input_uv_len);
			if(ret < 0) {
				goto __exit;
			}

			pJpegEnc->jpeg_status = jpeg_encode_status_query(pJpegEnc->hwidx, TRUE);	//wait jpeg block encode done
		}
		else if(pJpegEnc->jpeg_status & C_JPG_STATUS_STOP) {
			MSG("\r\njpeg encode is not started!\r\n");
			ret = STATUS_FAIL;
			goto __exit;
		}
		else if(pJpegEnc->jpeg_status & C_JPG_STATUS_TIMEOUT) {
			MSG("\r\njpeg encode execution timeout!\r\n");
			ret = STATUS_FAIL;
			goto __exit;
		}
		else if(pJpegEnc->jpeg_status & C_JPG_STATUS_INIT_ERR) {
			MSG("\r\njpeg encode init error!\r\n");
			ret = STATUS_FAIL;
			goto __exit;
		}
		else {
			MSG("\r\nJPEG status error = 0x%x!\r\n", pJpegEnc->jpeg_status);
			ret = STATUS_FAIL;
			goto __exit;
		}
	}

__exit:
	jpeg_encode_stop(pJpegEnc->hwidx);
	return ret;
}

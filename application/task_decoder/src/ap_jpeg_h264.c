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
#define C_USE_SCALER_NUMBER				SCALER_0

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                            E X T E R N A L                             *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static ScalerFormat_t	jpeg_src={0};
static ScalerPara_t		jpeg_para={0};

INT32U jpeg_y_size;

static INT32U jpeg_output_format;
/*
jpeg_output_ratio:
0=Fit to output_buffer_width and output_buffer_height,
1=Maintain ratio and fit to output_buffer_width or output_buffer_height,
2=Same as 1 but without scale up
*/
static INT8U jpeg_output_ratio;
static INT16U jpeg_output_buffer_width;
static INT16U jpeg_output_buffer_height;
static INT16U jpeg_output_image_width;
static INT16U jpeg_output_image_height;
static INT32U out_of_boundary_color;
static INT32U jpeg_output_buffer_pointer;
#if C_BROWSE_FULL_SCREEN == CUSTOM_OFF
static INT16U jpeg_output_image_x_offset;
#endif
// Control variables
static INT32U jpeg_fifo_line_num = 32;
static INT32U size_temp;

// JPEG variables
static INT16U jpeg_valid_width, jpeg_valid_height, jpeg_image_yuv_mode;
static INT32U jpeg_extend_width, jpeg_extend_height;
static INT32U jpeg_fifo_register = C_JPG_FIFO_32LINE;

static INT8U thumbnail_exist;
static INT16U thumbnail_width;
static INT16U thumbnail_height;
static INT16U thumbnail_yuv_mode;

// Buffer addresses
static INT32U jpeg_out_y, jpeg_out_cb, jpeg_out_cr;
static INT32U scaler_out_buffer;

/*
scaler_fifo_register:
C_SCALER_CTRL_FIFO_DISABLE
C_SCALER_CTRL_FIFO_32LINE
*/
static INT32U scaler_fifo_register = C_SCALER_CTRL_FIFO_32LINE;


extern INT8U	jpeg_picture_header[624];
extern INT32S vid_dec_get_total_time(void);
/**************************************************************************
 *                           F U N C T I O N S                            *
 **************************************************************************/
 /**
 * @brief   JPEG memory allocation for JPEG decode
 * @param   fifo: JPEG fifo line number
 * @return  None
 */
void jpeg_memory_allocate(INT32U fifo)
{
	INT32U jpeg_output_y_size=0;
	INT32U jpeg_output_cb_cr_size=0;
	INT16U cbcr_shift;

	if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV420) {
		cbcr_shift = 2;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV411) {
		cbcr_shift = 2;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV422) {
		cbcr_shift = 1;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV422V) {
		cbcr_shift = 1;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV444) {
		cbcr_shift = 0;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_GRAYSCALE) {
		cbcr_shift = 32;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV411V) {
		cbcr_shift = 2;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV420H2) {
		cbcr_shift = 1;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV420V2) {
		cbcr_shift = 1;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV411H2) {
		cbcr_shift = 1;
	} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV411V2) {
		cbcr_shift = 1;
	} else {
		jpeg_out_y = 0;
		return;
	}

	if (fifo) {

		switch(jpeg_image_yuv_mode)
		{
		case C_JPG_CTRL_YUV444:
		case C_JPG_CTRL_YUV411:
		case C_JPG_CTRL_YUV422V:
			jpeg_output_y_size = jpeg_extend_width*fifo*2; //Y only
			jpeg_output_cb_cr_size = jpeg_output_y_size >> cbcr_shift;	//U & V
			size_temp = jpeg_extend_width*fifo*2;
			break;


		case C_JPG_CTRL_YUV422:
		case C_JPG_CTRL_YUV420:

			jpeg_output_y_size = jpeg_extend_width*fifo*2*2; //YUYV
			jpeg_output_cb_cr_size = 0;
			size_temp = jpeg_extend_width*fifo*2;
			break;

		case C_JPG_CTRL_GRAYSCALE:
		case C_JPG_CTRL_YUV411V:
		case C_JPG_CTRL_YUV420H2:
		case C_JPG_CTRL_YUV420V2:
		case C_JPG_CTRL_YUV411H2:
		case C_JPG_CTRL_YUV411V2:
			jpeg_output_y_size = jpeg_extend_width*fifo*2;
			jpeg_output_cb_cr_size = jpeg_output_y_size >> cbcr_shift;
		}
	} else {
		jpeg_output_y_size = jpeg_extend_width * jpeg_extend_height;
		jpeg_output_cb_cr_size = jpeg_output_y_size >> cbcr_shift;
	}

  	jpeg_out_y = (INT32U) gp_malloc_align(jpeg_output_y_size, 32);
  	if (!jpeg_out_y) {
  		return;
  	}

  	cache_invalid_range(jpeg_out_y, jpeg_output_y_size);
	jpeg_y_size = jpeg_output_y_size;
  	if(jpeg_output_cb_cr_size) {
  		jpeg_out_cb = (INT32U) gp_malloc_align(jpeg_output_cb_cr_size, 8);
  		if (!jpeg_out_cb) {
  			gp_free((void *) jpeg_out_y);
  			jpeg_out_y = (INT32U) NULL;
  			return;
  		}
  		cache_invalid_range(jpeg_out_cb, jpeg_output_cb_cr_size);

  		jpeg_out_cr = (INT32U) gp_malloc_align(jpeg_output_cb_cr_size, 8);
  		if (!jpeg_out_cr) {
  			gp_free((void *) jpeg_out_cb);
  			jpeg_out_cb = (INT32U) NULL;
  			gp_free((void *) jpeg_out_y);
  			jpeg_out_y = (INT32U) NULL;
  			return;
  		}
  		cache_invalid_range(jpeg_out_cr, jpeg_output_cb_cr_size);
  	} else {
  		jpeg_out_cb = 0;
  		jpeg_out_cr = 0;
  	}
}

 /**
 * @brief   JPEG setup scaler parameters
 * @param   fifo: JPEG fifo line number
 * @return  None
 */
void jpeg_scaler_set_parameters(INT32U fifo)
{
	switch(jpeg_image_yuv_mode)
	{
		case C_JPG_CTRL_YUV444:
			jpeg_src.input_format = C_SCALER_CTRL_IN_YUV444;
			break;
		case C_JPG_CTRL_YUV411:
			jpeg_src.input_format = C_SCALER_CTRL_IN_YUV411;
			break;
		case C_JPG_CTRL_YUV422V:
			jpeg_src.input_format = C_SCALER_CTRL_IN_YUV422V;
			break;
		case C_JPG_CTRL_YUV422:
		case C_JPG_CTRL_YUV420:
			jpeg_src.input_format = C_SCALER_CTRL_IN_YUYV;	//union mode
			break;
		case C_JPG_CTRL_GRAYSCALE:
			jpeg_src.input_format = C_SCALER_CTRL_IN_Y_ONLY;
			break;
		case C_JPG_CTRL_YUV411V:
			jpeg_src.input_format = C_SCALER_CTRL_IN_YUV411V;
			break;
		case C_JPG_CTRL_YUV420H2:
			jpeg_src.input_format = C_SCALER_CTRL_IN_YUV422V;
			break;
		case C_JPG_CTRL_YUV420V2:
			jpeg_src.input_format = C_SCALER_CTRL_IN_YUV422;
			break;
		case C_JPG_CTRL_YUV411H2:
			jpeg_src.input_format = C_SCALER_CTRL_IN_YUV422;
			break;
		case C_JPG_CTRL_YUV411V2:
			jpeg_src.input_format = C_SCALER_CTRL_IN_YUV422V;
			break;
		default:
			return;
	}
	jpeg_src.input_width = jpeg_extend_width;
	jpeg_src.input_height = jpeg_extend_height;
	jpeg_src.input_visible_width = jpeg_valid_width;
	jpeg_src.input_visible_height = jpeg_valid_height;

	jpeg_src.input_x_offset = 0;
	jpeg_src.input_y_offset = 0;
	jpeg_src.input_y_addr = jpeg_out_y;
	jpeg_src.input_u_addr = jpeg_out_cb;
	jpeg_src.input_v_addr = jpeg_out_cr;

	jpeg_src.input_b_y_addr = jpeg_out_y+size_temp;
	jpeg_src.input_b_u_addr = jpeg_out_cb;
	jpeg_src.input_b_v_addr = jpeg_out_cr;
	jpeg_src.output_format = jpeg_output_format;

	jpeg_src.output_width = jpeg_output_image_width;
	jpeg_src.output_height = jpeg_output_image_height;
	jpeg_src.output_buf_width = jpeg_output_buffer_width;
	jpeg_src.output_buf_height = jpeg_output_buffer_height;
	jpeg_src.callback = NULL;
	jpeg_src.output_x_offset = 0;

	if (jpeg_output_buffer_pointer) {
	    scaler_out_buffer = jpeg_output_buffer_pointer;
	} else {
	    scaler_out_buffer = (INT32U) gp_malloc((jpeg_output_buffer_width*jpeg_output_buffer_height)<<1);
	    if (!scaler_out_buffer) {
		    return;
		}
	}

	jpeg_src.output_y_addr = scaler_out_buffer;
	jpeg_src.output_u_addr = 0;
	jpeg_src.output_v_addr = 0;
	jpeg_src.fifo_mode = fifo;
	jpeg_src.scale_mode = C_SCALER_BY_RATIO;
	jpeg_src.digizoom_m = 10;
	jpeg_src.digizoom_n = 10;
	jpeg_para.yuv_type = C_SCALER_CTRL_TYPE_YCBCR;
	jpeg_para.boundary_mode = 1;
	jpeg_para.boundary_color = out_of_boundary_color;
}

 /**
 * @brief   JPEG setup pre-scale down, scale to 1/2 or 1/4 only
 * @param   None
 * @return  None
 */
void
jpeg_pre_scaledown_set(
	INT32U jpeg_hw_id
)
{

	switch(jpeg_image_yuv_mode)
	{
	case C_JPG_CTRL_YUV444:
	case C_JPG_CTRL_YUV422V:
	case C_JPG_CTRL_YUV411:
		drv_l1_jpeg_using_union_mode_disable(jpeg_hw_id);
		break;

	case C_JPG_CTRL_YUV422:
	case C_JPG_CTRL_YUV420:
        drv_l1_jpeg_using_union_mode_enable(jpeg_hw_id);

        if(jpeg_image_yuv_mode == C_JPG_CTRL_YUV420 || jpeg_image_yuv_mode ==C_JPG_CTRL_YUV422){
            if((jpeg_output_image_width <= (jpeg_valid_width>>2)) &&
                (jpeg_output_image_height <= (jpeg_valid_height>>2))) {

                drv_l1_jpeg_decode_scale_down_set(jpeg_hw_id, ENUM_JPG_DIV4);// Use 1/4 scale-down mode
                jpeg_valid_width >>= 2;
                jpeg_valid_height >>= 2;
                jpeg_extend_width >>= 2;
                jpeg_extend_height >>= 2;
            } else if((jpeg_output_image_width <= (jpeg_valid_width>>1)) &&
                (jpeg_output_image_height <= (jpeg_valid_height>>1))) {
                drv_l1_jpeg_decode_scale_down_set(jpeg_hw_id, ENUM_JPG_DIV2);// Use 1/2 scale-down mode
                jpeg_valid_width >>= 1;
                jpeg_valid_height >>= 1;
                jpeg_extend_width >>= 1;
                jpeg_extend_height >>= 1;
            }
		}
		break;

	case C_JPG_CTRL_GRAYSCALE:
	case C_JPG_CTRL_YUV411V:
	case C_JPG_CTRL_YUV420H2:
	case C_JPG_CTRL_YUV420V2:
	case C_JPG_CTRL_YUV411H2:
	case C_JPG_CTRL_YUV411V2:
        drv_l1_jpeg_using_union_mode_disable(jpeg_hw_id);
		break;
	}

}

 /**
 * @brief   JPEG decode from DRAM buffer and scale
 * @param   img_decode_struct: image decode structure
 * @param   clip_xy: clip x & y position
 * @param   clip_size: [31:16] is clip width, [15:0] is clip height
 * @return  0: Success; others: Failed
 */
INT32S
jpeg_buffer_decode_and_scale(
	IMAGE_DECODE_STRUCT *img_decode_struct,
	INT32U clip_xy,
	INT32U clip_size
)
{
	INT16S file_handle;
	INT32S parse_status;
	INT8U *p_vlc;
	INT32U left_len;
  	INT32S jpeg_status;
  	INT32S scaler_status=0;
	INT8U scaler_done;
	INT32U jpeg_hw_id;
	#if C_BROWSE_FULL_SCREEN == CUSTOM_OFF
    static INT8U ratio_flag = 0;
	#endif
	// Read from file
	INT32U	total_jpeg_dec = 0;

	if (!img_decode_struct) {
		return STATUS_FAIL;
	}

	jpeg_hw_id = get_idle_jpeg(-1);

	file_handle = img_decode_struct->file_handle;
    jpeg_output_format = img_decode_struct->output_format;
    jpeg_output_ratio = img_decode_struct->output_ratio;
    jpeg_output_buffer_width = img_decode_struct->output_buffer_width;
    jpeg_output_buffer_height = img_decode_struct->output_buffer_height;
    jpeg_output_image_width = img_decode_struct->output_image_width;
    jpeg_output_image_height = img_decode_struct->output_image_height;
    out_of_boundary_color = img_decode_struct->out_of_boundary_color;
    jpeg_output_buffer_pointer = img_decode_struct->output_buffer_pointer;
    #if C_BROWSE_FULL_SCREEN == CUSTOM_OFF
    jpeg_output_image_x_offset = 0;
    #endif
	// Initiate software header parser and hardware engine
	jpeg_decode_init(jpeg_hw_id);

	parse_status = jpeg_decode_parse_header(jpeg_hw_id, (INT8U *) img_decode_struct->image_source, img_decode_struct->source_size);
    if (parse_status != JPEG_PARSE_OK) {
		DBG_PRINT("Parse header failed. Skip this file\r\n");
		img_decode_struct->decode_status = STATUS_FAIL;

		release_used_jpeg(jpeg_hw_id);
        return STATUS_FAIL;
	}

	img_decode_struct->input_image_width = jpeg_valid_width = jpeg_decode_image_width_get(jpeg_hw_id);
	img_decode_struct->input_image_height = jpeg_valid_height = jpeg_decode_image_height_get(jpeg_hw_id);
	jpeg_image_yuv_mode = jpeg_decode_image_yuv_mode_get(jpeg_hw_id);
	jpeg_extend_width = jpeg_decode_image_extended_width_get(jpeg_hw_id);
	jpeg_extend_height = jpeg_decode_image_extended_height_get(jpeg_hw_id);

	thumbnail_exist = jpeg_decode_thumbnail_exist_get(jpeg_hw_id);		// 0=No thumbnail image, 1=Thumbnail image exists
	thumbnail_width = jpeg_decode_thumbnail_width_get(jpeg_hw_id);
	thumbnail_height = jpeg_decode_thumbnail_height_get(jpeg_hw_id);
	thumbnail_yuv_mode = jpeg_decode_thumbnail_yuv_mode_get(jpeg_hw_id);

	#if C_BROWSE_FULL_SCREEN == CUSTOM_OFF
	DBG_PRINT("jpeg_valid_width = %d\r\n",jpeg_valid_width);
	DBG_PRINT("jpeg_valid_height = %d\r\n",jpeg_valid_height);
	DBG_PRINT("jpeg_output_image_width = %d\r\n",jpeg_output_image_width);
	DBG_PRINT("jpeg_output_image_height = %d\r\n",jpeg_output_image_height);
    if(jpeg_valid_width*jpeg_output_image_height != jpeg_valid_height*jpeg_output_image_width)
    {
        jpeg_output_image_width = (((jpeg_valid_width*jpeg_output_image_height)/jpeg_valid_height)/16)<<4;
        jpeg_output_image_x_offset = ((jpeg_output_buffer_width - jpeg_output_image_width)/4)<<1;
        DBG_PRINT("jpeg_output_image_width = %d\r\n",jpeg_output_image_width);
        DBG_PRINT("jpeg_output_image_x_offset = %d\r\n",jpeg_output_image_x_offset);
        if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
        {
            jpeg_output_buffer_pointer += jpeg_output_image_x_offset;
            DBG_PRINT("GP420\r\n");
        }
        else
        {
            jpeg_output_buffer_pointer += jpeg_output_image_x_offset*2;
            DBG_PRINT("GP565/YUYV\r\n");
        }
        ratio_flag = 1;
    }
	#endif

	// Check whether thumbnail exists and satisfies our requirement
	if (thumbnail_exist &&
		(jpeg_output_buffer_width <= thumbnail_width+(thumbnail_width>>2)) &&
		(jpeg_output_buffer_height <= thumbnail_height+(thumbnail_height>>2))) {

		jpeg_decode_init(jpeg_hw_id);
		jpeg_decode_thumbnail_image_enable(jpeg_hw_id);
	   	parse_status = jpeg_decode_parse_header(jpeg_hw_id, (INT8U *) img_decode_struct->image_source, img_decode_struct->source_size);
		if (parse_status != JPEG_PARSE_OK) {
			DBG_PRINT("Parse header failed. Skip this file\r\n");
			img_decode_struct->decode_status = STATUS_FAIL;

			release_used_jpeg(jpeg_hw_id);
		  	return STATUS_FAIL;
		}

	    jpeg_valid_width = thumbnail_width;
		jpeg_valid_height = thumbnail_height;
		jpeg_image_yuv_mode = thumbnail_yuv_mode;
		jpeg_extend_width = jpeg_decode_thumbnail_extended_width_get(jpeg_hw_id);
		jpeg_extend_height = jpeg_decode_thumbnail_extended_height_get(jpeg_hw_id);

	} else if (jpeg_decode_image_progressive_mode_get(jpeg_hw_id)) {
	  	jpeg_status = STATUS_FAIL;
		img_decode_struct->decode_status = jpeg_status;

		release_used_jpeg(jpeg_hw_id);
		return jpeg_status;

	} else if (clip_size) {
		INT16U clip_x, clip_y, clip_width, clip_height;
		INT16U align_x, align_y;

		// Initiate align_x and align_y according to YUV mode
		if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV422) {
			align_x = 0xF;
			align_y = 0x7;
		} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV420) {
			align_x = 0xF;
			align_y = 0xF;
		} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV422V) {
			align_x = 0x7;
			align_y = 0xF;
		} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV444) {
			align_x = 0x7;
			align_y = 0x7;
		} else if (jpeg_image_yuv_mode == C_JPG_CTRL_GRAYSCALE) {
			align_x = 0x7;
			align_y = 0x7;
		} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV411) {
			align_x = 0x1F;
			align_y = 0x7;
		} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV411V) {
			align_x = 0x7;
			align_y = 0x1F;
		} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV420H2) {
			align_x = 0xF;
			align_y = 0xF;
		} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV420V2) {
			align_x = 0xF;
			align_y = 0xF;
		} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV411H2) {
			align_x = 0x1F;
			align_y = 0x7;
		} else if (jpeg_image_yuv_mode == C_JPG_CTRL_YUV411V2) {
			align_x = 0x7;
			align_y = 0x1F;
		} else {
			align_x = 0x7;
			align_y = 0x7;
		}

		// Get input clipping start position and width/height
		clip_x = clip_xy >> 16;
		clip_y = clip_xy & 0xFFFF;
		clip_width = clip_size >> 16;
		clip_height = clip_size & 0xFFFF;
		// Make sure start position and width/height align to JPEG YUV mode
		if (clip_x >= jpeg_valid_width) {
			clip_x = 0;
		} else {
			clip_x = (clip_x + align_x) & (~align_x);
		}
		if (clip_y >= jpeg_valid_height) {
			clip_y = 0;
		} else {
			clip_y = (clip_y + align_y) & (~align_y);
		}
		if (!clip_width) {
			clip_width = align_x + 1;
		} else {
			clip_width = (clip_width + align_x) & (~align_x);
		}
		if (!clip_height) {
			clip_height = align_y + 1;
		} else {
			clip_height = (clip_height + align_y) & (~align_y);
		}

		// Check valid clipping range
		if (clip_x+clip_width > jpeg_extend_width) {
			clip_width = jpeg_extend_width - clip_x;
		}
		if (clip_y+clip_height > jpeg_extend_height) {
			clip_height = jpeg_extend_height - clip_y;
		}
		if (clip_x+clip_width > jpeg_valid_width) {
			jpeg_valid_width -= clip_x;
		} else {
			jpeg_valid_width = clip_width;
		}
		if (clip_y+clip_height > jpeg_valid_height) {
			jpeg_valid_height -= clip_y;
		} else {
			jpeg_valid_height = clip_height;
		}
		jpeg_extend_width = clip_width;
		jpeg_extend_height = clip_height;
		jpeg_decode_clipping_range_set(jpeg_hw_id, clip_x, clip_y, clip_width, clip_height);
		jpeg_decode_clipping_mode_enable(jpeg_hw_id);

	}

	#if 0 // for reducing memory(bandwidth) usage
	// Set Jpeg H/W pre scaledown.
	jpeg_pre_scaledown_set(jpeg_hw_id);
	#endif

	jpeg_memory_allocate(jpeg_fifo_line_num);
	if (!jpeg_out_y) {
		DBG_PRINT("Failed to allocate memory in jpeg_memory_allocate()\r\n");
		img_decode_struct->decode_status = -2;

		release_used_jpeg(jpeg_hw_id);
		return -2;
	}

	if (jpeg_decode_output_set(jpeg_hw_id, jpeg_out_y, jpeg_out_cb, jpeg_out_cr, jpeg_fifo_register)) {
		DBG_PRINT("Failed to call jpeg_decode_output_set()\r\n");
		img_decode_struct->decode_status = STATUS_FAIL;
	  	gp_free((void *) jpeg_out_y);
	  	if (jpeg_image_yuv_mode != C_JPG_CTRL_GRAYSCALE) {
	  		gp_free((void *) jpeg_out_cb);
	  		gp_free((void *) jpeg_out_cr);
	  	}

		release_used_jpeg(jpeg_hw_id);
		return STATUS_FAIL;
	}

	p_vlc = jpeg_decode_image_vlc_addr_get(jpeg_hw_id);

	if(img_decode_struct->source_type == TK_IMAGE_SOURCE_TYPE_BUFFER)
	{
	  	if (((INT32U) p_vlc) >= (img_decode_struct->image_source+img_decode_struct->source_size)) {
	  		DBG_PRINT("VLC address exceeds file range\r\n");
	  		img_decode_struct->decode_status = STATUS_FAIL;
		  	gp_free((void *) jpeg_out_y);
		  	if (jpeg_image_yuv_mode != C_JPG_CTRL_GRAYSCALE) {
		  		gp_free((void *) jpeg_out_cb);
		  		gp_free((void *) jpeg_out_cr);
		  	}

			release_used_jpeg(jpeg_hw_id);
	        return STATUS_FAIL;
	  	}

		// Now start JPEG decoding
		left_len = img_decode_struct->source_size - (((INT32U) p_vlc) - img_decode_struct->image_source);
		jpeg_decode_vlc_maximum_length_set(jpeg_hw_id, left_len);
		if (jpeg_decode_once_start(jpeg_hw_id, p_vlc, left_len)) {
			DBG_PRINT("Failed to call jpeg_decode_once_start()\r\n");
			img_decode_struct->decode_status = STATUS_FAIL;
		  	if (jpeg_image_yuv_mode != C_JPG_CTRL_GRAYSCALE) {
		  		gp_free((void *) jpeg_out_cb);
		  		gp_free((void *) jpeg_out_cr);
		  	}
		  	gp_free((void *) jpeg_out_y);

			release_used_jpeg(jpeg_hw_id);
	        return STATUS_FAIL;
	  	}
	}
	else if(img_decode_struct->source_type == TK_IMAGE_SOURCE_TYPE_FILE)
	{
		total_jpeg_dec = (((INT32U) p_vlc) - img_decode_struct->image_source);

		if(img_decode_struct->reload_size && (img_decode_struct->reload_size < img_decode_struct->source_size))
		{
			left_len = img_decode_struct->reload_size - (((INT32U) p_vlc) - img_decode_struct->image_source); // header length
		}
		else
		{
			left_len = img_decode_struct->source_size - (((INT32U) p_vlc) - img_decode_struct->image_source); // header length
		}

		total_jpeg_dec += left_len;

		// Now start JPEG decoding
		if (jpeg_decode_on_the_fly_start(jpeg_hw_id, p_vlc, left_len)) {
			DBG_PRINT("Failed to call jpeg_decode_once_start()\r\n");
			img_decode_struct->decode_status = STATUS_FAIL;
		  	if (jpeg_image_yuv_mode != C_JPG_CTRL_GRAYSCALE) {
		  		gp_free((void *) jpeg_out_cb);
		  		gp_free((void *) jpeg_out_cr);
		  	}
		  	gp_free((void *) jpeg_out_y);

			release_used_jpeg(jpeg_hw_id);
	        return STATUS_FAIL;
	  	}
	}


  	// Initiate Scaler
  	drv_l2_scaler_init();
	scaler_done = 0;
	// Setup Scaler
	jpeg_scaler_set_parameters(scaler_fifo_register);

  	if (!scaler_out_buffer) {
		jpeg_decode_stop(jpeg_hw_id);
		DBG_PRINT("Failed to allocate scaler_out_buffer\r\n");
		img_decode_struct->decode_status = STATUS_FAIL;
	  	gp_free((void *) jpeg_out_y);
	  	if (jpeg_image_yuv_mode != C_JPG_CTRL_GRAYSCALE) {
	  		gp_free((void *) jpeg_out_cb);
	  		gp_free((void *) jpeg_out_cr);
	  	}

		release_used_jpeg(jpeg_hw_id);
		return STATUS_FAIL;
  	}

	scaler_status = C_SCALER_STATUS_STOP;
  	while (1) {
  		jpeg_status = jpeg_decode_status_query(jpeg_hw_id, 1);

		if (jpeg_status & C_JPG_STATUS_DECODE_DONE) {
		  	// Wait until scaler finish its job
		  	while (!scaler_done) {
		  		if (scaler_status == C_SCALER_STATUS_STOP) {
					scaler_status = drv_l2_scaler_trigger(C_USE_SCALER_NUMBER, 1, &jpeg_src, &jpeg_para);
				} else if (scaler_status & C_SCALER_STATUS_DONE) {
					break;
				} else if (scaler_status & (C_SCALER_STATUS_TIMEOUT|C_SCALER_STATUS_INIT_ERR)) {
					DBG_PRINT("Scaler failed to finish its job\r\n");
					break;
				} else if (scaler_status & C_SCALER_STATUS_INPUT_EMPTY) {
		  			scaler_status = drv_l2_scaler_retrigger(C_USE_SCALER_NUMBER, &jpeg_src);
		  		} else {
			  		DBG_PRINT("Un-handled Scaler status!\r\n");
			  		break;
			  	}

			  	if(scaler_status == C_SCALER_STATUS_STOP)
			  	{
					scaler_status = C_SCALER_STATUS_DONE;
			  	}
		  	}
			break;
		}		// if (jpeg_status & C_JPG_STATUS_DECODE_DONE)

		if (jpeg_status & C_JPG_STATUS_OUTPUT_FULL) {
		  	// Start scaler to handle the full output FIFO now
		  	if (!scaler_done) {
			  	if (scaler_status == C_SCALER_STATUS_STOP) {
					scaler_status = drv_l2_scaler_trigger(C_USE_SCALER_NUMBER, 1, &jpeg_src, &jpeg_para);
			  	} else if (scaler_status & C_SCALER_STATUS_DONE) {
			  		// Scaler might finish its job before JPEG does when image is zoomed in.
			  		scaler_done = 1;
			  	} else if (scaler_status & (C_SCALER_STATUS_TIMEOUT|C_SCALER_STATUS_INIT_ERR)) {
					DBG_PRINT("Scaler failed to finish its job\r\n");
					break;
				} else if (scaler_status & C_SCALER_STATUS_INPUT_EMPTY) {
			  		scaler_status = drv_l2_scaler_retrigger(C_USE_SCALER_NUMBER, &jpeg_src);
			  	} else {
			  		DBG_PRINT("Un-handled Scaler status!\r\n");
			  		break;
			  	}

			  	if(scaler_status == C_SCALER_STATUS_STOP)
			  	{
					scaler_status = C_SCALER_STATUS_DONE;
			  	}
			}

	  		// Now restart JPEG to output to next FIFO
	  		if (jpeg_decode_output_restart(jpeg_hw_id)) {
	  			DBG_PRINT("Failed to call jpeg_decode_output_restart()\r\n");
	  			break;
	  		}
		}		// if (jpeg_status & C_JPG_STATUS_OUTPUT_FULL)

		if(img_decode_struct->source_type == TK_IMAGE_SOURCE_TYPE_FILE)
		{
			if (jpeg_status & C_JPG_STATUS_INPUT_EMPTY) {

				if(img_decode_struct->data_reload && img_decode_struct->reload_size)
				{
					if((img_decode_struct->source_size - total_jpeg_dec) > img_decode_struct->reload_size)
					{
						left_len = img_decode_struct->reload_size;
					}
					else
					{
						left_len = img_decode_struct->source_size - total_jpeg_dec;
					}

					if(left_len)
					{
						if(img_decode_struct->data_reload(file_handle,img_decode_struct->image_source, left_len) < 0)
						{
							DBG_PRINT("video_decode_jpeg_read_partial_process read jpeg file fail.\r\n");
							release_used_jpeg(jpeg_hw_id);
							return STATUS_FAIL;
						}
					}

					// Read the last size in a file
	                if(left_len != img_decode_struct->reload_size)
	                {
	                	// << 7 for make sure jpeg engine is done
						left_len = (left_len + 0x3F) >> 6 << 7; //64 byte alignment for hw constrain
	                }

				}
				else
				{
					if(img_decode_struct->source_size > total_jpeg_dec)
					{
						left_len = img_decode_struct->source_size - total_jpeg_dec;
					}
					else
					{
						left_len = 0;
					}
				}

		  		// Now restart JPEG to output to next FIFO
				if (jpeg_decode_on_the_fly_start(jpeg_hw_id, (INT8U *)(img_decode_struct->image_source), left_len)) {
		  			DBG_PRINT("Failed to call jpeg_decode_output_restart()\r\n");
		  			break;
		  		}

				total_jpeg_dec += left_len;

			}
		}

		if (jpeg_status & C_JPG_STATUS_STOP) {
			DBG_PRINT("JPEG is not started!\r\n");
			break;
		}
		if (jpeg_status & C_JPG_STATUS_TIMEOUT) {
			DBG_PRINT("JPEG execution timeout-3!\r\n");
			break;
		}
		if (jpeg_status & C_JPG_STATUS_INIT_ERR) {
			DBG_PRINT("JPEG init error!\r\n");
			break;
		}
		if (jpeg_status & C_JPG_STATUS_RST_VLC_DONE) {
			DBG_PRINT("JPEG Restart marker number is incorrect!\r\n");
			jpeg_status = C_JPG_STATUS_DECODE_DONE;
			scaler_status = C_SCALER_STATUS_DONE;
			break;
		}
		if (jpeg_status & C_JPG_STATUS_RST_MARKER_ERR) {
			DBG_PRINT("JPEG Restart marker sequence error!\r\n");
			jpeg_status = C_JPG_STATUS_DECODE_DONE;
			scaler_status = C_SCALER_STATUS_DONE;
			break;
		}
  	}
	jpeg_decode_stop(jpeg_hw_id);
	release_used_jpeg(jpeg_hw_id);

	if( (scaler_status >= 0) && (scaler_status != C_SCALER_STATUS_STOP) )
		drv_l2_scaler_stop(C_USE_SCALER_NUMBER);

	if(jpeg_out_y) gp_free((void *) jpeg_out_y);
	if (jpeg_image_yuv_mode != C_JPG_CTRL_GRAYSCALE) {
  		if(jpeg_out_cb) gp_free((void *) jpeg_out_cb);
  		if(jpeg_out_cr) gp_free((void *) jpeg_out_cr);
  	}

	if ((jpeg_status & C_JPG_STATUS_DECODE_DONE) && (scaler_status & C_SCALER_STATUS_DONE)) {
	    if (!jpeg_output_buffer_pointer) {
	        jpeg_output_buffer_pointer = scaler_out_buffer;
	    }
	    cache_invalid_range(scaler_out_buffer, (jpeg_output_buffer_width*jpeg_output_buffer_height)<<1);

	} else {
		img_decode_struct->decode_status = STATUS_FAIL;
    	if (!jpeg_output_buffer_pointer) {
    	    gp_free((void *) scaler_out_buffer);
    	}

        return STATUS_FAIL;
	}

    img_decode_struct->decode_status = STATUS_OK;
    if (!img_decode_struct->output_buffer_pointer && jpeg_output_buffer_pointer) {
        img_decode_struct->output_buffer_pointer = jpeg_output_buffer_pointer;
    }
    img_decode_struct->output_image_width = jpeg_output_image_width;
    img_decode_struct->output_image_height = jpeg_output_image_height;
    #if C_BROWSE_FULL_SCREEN == CUSTOM_OFF
    if(ratio_flag)
    {
        ratio_flag = 0;
        video_decode_display_do_padding(img_decode_struct->output_buffer_pointer,jpeg_output_buffer_width,jpeg_output_image_height,jpeg_output_image_x_offset);
    }
    #endif
	return STATUS_OK;
}

INT32S
h264_buffer_decode_and_scale(
	IMAGE_DECODE_STRUCT *img_decode_struct,
	INT32U clip_xy,
	INT32U clip_size
)
{
	INT32S nRet = STATUS_OK;
	#if C_BROWSE_FULL_SCREEN == CUSTOM_OFF
    static INT8U ratio_flag = 0;
	#endif
	p_vid_dec_para->file_handle = img_decode_struct->file_handle;
	p_vid_dec_para->FileSize = img_decode_struct->source_size;
	nRet = browse_parser_header_start();
	if(nRet == STATUS_FAIL)
	{
		RETURN(STATUS_FAIL);
	}

	/*
	 Reset variable value to 0 in browse_parser_header_start(),
	 so put vid_dec_set_scaler here
	*/
	#if C_BROWSE_FULL_SCREEN == CUSTOM_OFF
	INT32U video_output_width = img_decode_struct->output_image_width;
	DBG_PRINT("video_valid_width = %d\r\n",p_bitmap_info->biWidth);
	DBG_PRINT("video_valid_height = %d\r\n",p_bitmap_info->biHeight);
	DBG_PRINT("video_output_image_width = %d\r\n",img_decode_struct->output_image_width);
	DBG_PRINT("video_output_image_height = %d\r\n",img_decode_struct->output_image_height);
	jpeg_output_image_x_offset = 0;
    if(p_bitmap_info->biWidth*img_decode_struct->output_image_height != p_bitmap_info->biHeight*img_decode_struct->output_image_width)
    {
        img_decode_struct->output_image_width = (((p_bitmap_info->biWidth*img_decode_struct->output_image_height)/p_bitmap_info->biHeight)/16)<<4;
        jpeg_output_image_x_offset = ((video_output_width - img_decode_struct->output_image_width)/4)<<1;
        DBG_PRINT("video_output_image_width = %d\r\n",img_decode_struct->output_image_width);
        DBG_PRINT("video_output_image_x_offset = %d\r\n",jpeg_output_image_x_offset);
        if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
        {
            img_decode_struct->output_buffer_pointer += jpeg_output_image_x_offset;
            DBG_PRINT("GP420\r\n");
        }
        else
        {
            img_decode_struct->output_buffer_pointer += jpeg_output_image_x_offset*2;
            DBG_PRINT("GP565/YUYV\r\n");
        }
        ratio_flag = 1;
    }
	#endif


	vid_dec_set_scaler(SCALE_FIT_BUF,
		img_decode_struct->output_format,
		img_decode_struct->output_image_width,
		img_decode_struct->output_image_height,
		img_decode_struct->output_buffer_width,
		img_decode_struct->output_image_height
	);

	nRet = browse_vid_dec_start(1,img_decode_struct->output_buffer_pointer,&img_decode_struct->input_image_width,&img_decode_struct->input_image_height);
	if(nRet == STATUS_FAIL)
	{
		RETURN(STATUS_FAIL);
	}
    #if C_BROWSE_FULL_SCREEN == CUSTOM_OFF
    if(ratio_flag)
    {
        ratio_flag = 0;
        if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
        img_decode_struct->output_buffer_pointer -= jpeg_output_image_x_offset;
        else
        img_decode_struct->output_buffer_pointer -= jpeg_output_image_x_offset*2;
        video_decode_display_do_padding(img_decode_struct->output_buffer_pointer,img_decode_struct->output_buffer_width,img_decode_struct->output_image_height,jpeg_output_image_x_offset);
    }
    #endif
	RETURN(STATUS_OK);
Return:

	browse_vid_dec_stop();
	vid_dec_parser_end();
	return nRet;
}

/*
JPEG encode from DRAM buffer for APP
*/

INT32S Jpeg_Encode_For_Thumbnail(
JpegEnc_t *pJpegEnc
)
{
	INT32S ret = STATUS_FAIL;

	jpeg_picture_header[7] = (((pJpegEnc->height)& 0xFF00)>>8);
	jpeg_picture_header[8] = ((pJpegEnc->height) & 0x00FF);
	jpeg_picture_header[9] = (((pJpegEnc->width) & 0xFF00)>>8);
	jpeg_picture_header[10] = ((pJpegEnc->width) & 0x00FF);
    jpeg_picture_header[13] = JPEG_IMG_FORMAT_420;

#if C_APP_PLAYBACK_WITH_SOUND
    int m_time = vid_dec_get_total_time();
    //DBG_PRINT("thumbnail get time = %d\r\n",m_time);
    jpeg_picture_header[0x19] = 'T';
    jpeg_picture_header[0x1A] = 'I';
    jpeg_picture_header[0x1B] = 'M';
    jpeg_picture_header[0x1C] = 'E';
    jpeg_picture_header[0x1D] = (INT8U)((m_time >> 0x00)&0xFF);
    jpeg_picture_header[0x1E] = (INT8U)((m_time >> 0x08)&0xFF);
    jpeg_picture_header[0x1F] = (INT8U)((m_time >> 0x10)&0xFF);
    jpeg_picture_header[0x20] = (INT8U)((m_time >> 0x18)&0xFF);
#endif
	memcpy((void *)pJpegEnc->output_buffer, (INT8S*)jpeg_picture_header, 624);

    jpeg_header_quantization_table_calculate(pJpegEnc->hwidx,ENUM_JPEG_LUMINANCE_QTABLE, pJpegEnc->quality_value, jpeg_picture_header + 0x27);
	jpeg_header_quantization_table_calculate(pJpegEnc->hwidx,ENUM_JPEG_CHROMINANCE_QTABLE, pJpegEnc->quality_value, jpeg_picture_header + 0x6C);

	jpeg_encode_init(pJpegEnc->hwidx);
	gplib_jpeg_default_quantization_table_load(pJpegEnc->hwidx, pJpegEnc->quality_value);	// Load default qunatization table(quality)
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
	cache_invalid_range(pJpegEnc->output_buffer, 640);
	ret = jpeg_encode_output_addr_set(pJpegEnc->hwidx, (INT32U) pJpegEnc->output_buffer+624);
	if(ret < 0) {
		goto __exit;
	}

	ret = jpeg_encode_once_start(pJpegEnc->hwidx, pJpegEnc->input_buffer_y, pJpegEnc->input_buffer_u, pJpegEnc->input_buffer_v);
	if(ret < 0) {
		goto __exit;
	}

	while(1) {
		pJpegEnc->jpeg_status = jpeg_encode_status_query(pJpegEnc->hwidx, TRUE);
		if(pJpegEnc->jpeg_status & C_JPG_STATUS_ENCODE_DONE) {
			// Get encode length
			pJpegEnc->encode_size = jpeg_encode_vlc_cnt_get(pJpegEnc->hwidx);
			pJpegEnc->encode_size += 624;
			//DBG_PRINT("Encode Size = %d\r\n", pJpegEnc->encode_size);
			cache_invalid_range(pJpegEnc->output_buffer, pJpegEnc->encode_size);
			break;
		}
		else if(pJpegEnc->jpeg_status & C_JPG_STATUS_ENCODING) {
			continue;
		}
		else {
			ERR_PRINT("JPEG encode error!\r\n");
			ret = STATUS_FAIL;
			goto __exit;
		}
	}

	ret = STATUS_OK;
__exit:
	jpeg_encode_stop(pJpegEnc->hwidx);

	return ret;
}
/*
JPEG encode from DRAM buffer
*/
INT32S
jpeg_buffer_encode(
	JpegEnc_t *pJpegEnc
)
{
	INT32S ret;
	INT32U jpeg_header_size = jpeg_header_get_size();

	if ((pJpegEnc->width&0xF) || (pJpegEnc->height&0x7) || (pJpegEnc->output_buffer&0xF))
	{
		ERR_PRINT("input_width or input_height or output_addr alignment issue\r\n");
		return STATUS_FAIL;
	}

	pJpegEnc->header = jpeg_header_generate(pJpegEnc->yuv_sample_fmt, pJpegEnc->quality_value, pJpegEnc->width, pJpegEnc->height, 0);
	if(!pJpegEnc->header)
	{
		ERR_PRINT("pJpegEnc->header is null\r\n");
		return STATUS_FAIL;
	}

	memcpy((void *)pJpegEnc->output_buffer,pJpegEnc->header,jpeg_header_size);

	jpeg_encode_init(pJpegEnc->hwidx);
	gplib_jpeg_default_quantization_table_load(pJpegEnc->hwidx, pJpegEnc->quality_value);	// Load default qunatization table(quality)
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
	ret = jpeg_encode_output_addr_set(pJpegEnc->hwidx, (INT32U) pJpegEnc->output_buffer+jpeg_header_size);
	if(ret < 0) {
		goto __exit;
	}

	ret = jpeg_encode_once_start(pJpegEnc->hwidx, pJpegEnc->input_buffer_y, pJpegEnc->input_buffer_u, pJpegEnc->input_buffer_v);
	if(ret < 0) {
		goto __exit;
	}

	while(1) {
		pJpegEnc->jpeg_status = jpeg_encode_status_query(pJpegEnc->hwidx, TRUE);
		if(pJpegEnc->jpeg_status & C_JPG_STATUS_ENCODE_DONE) {
			// Get encode length
			pJpegEnc->encode_size = jpeg_encode_vlc_cnt_get(pJpegEnc->hwidx);
			pJpegEnc->encode_size += jpeg_header_size;
			//DBG_PRINT("Encode Size = %d\r\n", pJpegEnc->encode_size);
			cache_invalid_range(pJpegEnc->output_buffer, pJpegEnc->encode_size);
			break;
		}
		else if(pJpegEnc->jpeg_status & C_JPG_STATUS_ENCODING) {
			continue;
		}
		else {
			ERR_PRINT("JPEG encode error!\r\n");
			ret = STATUS_FAIL;
			goto __exit;
		}
	}

	ret = STATUS_OK;
__exit:
	jpeg_encode_stop(pJpegEnc->hwidx);
	if(pJpegEnc->header)
	{
		free(pJpegEnc->header);
	}
	return ret;
}




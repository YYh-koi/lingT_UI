/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Inc.                         *
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
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park,                      *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
#include "application.h"
#include <math.h>
#include "gp_paint.h"
#include "ap_blind_spot_detection.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define RED_Y       0x4C
#define RED_U       0x55
#define RED_V       0xFF

#define BLUE_Y      0x1B
#define BLUE_U      0xFF
#define BLUE_V      0x6C

#define BLUE1_Y     0xD5
#define BLUE1_U     0x92
#define BLUE1_V     0x48


//display_rec_time
#define REC_TIME_DRAW_MODE              1
#define LEFT_REC_TIME_DRAW_MODE         2
#define PLAY_TIME_DRAW_MODE             4

#define DIGIT_GAP                       (2)

//display date time
#define RTC_DATE_TIME_SHOW              1
#define FILE_DATE_TIME_SHOW             2

/**************************************************************************
 *               E X T E R N A L      R E F E R E N C E S                 *
 **************************************************************************/
extern INT32U fps_tft_vblank;
extern INT32U fps_tft_update;

/**************************************************************************
 *                      D E C L A R A T I O N S                           *
 **************************************************************************/
static void ap_display_vblank_isr(void);
void ap_display_timer(void);

static void ap_display_common_icon_draw(INT16U *buf);
static void ap_display_string_icon_draw(INT16U *buf);
#if C_WATER_MARK == CUSTOM_ON
static void ap_display_water_mark_icon_draw(INT16U *buf);
#endif

static void ap_display_rec_time_draw(INT16U *frame_buff);
static void ap_display_date_time_draw(INT16U *frame_buff);
static INT16S ap_display_digit_draw(INT16U *buf, INT8U digit, INT16S pos_x, INT16S pos_y, INT8U color_flag);

//static void ap_display_icon_draw_gp420(INT16U *buf, INT16U buf_w, INT16U buf_h, DISPLAY_ICONSHOW *icon_ptr, INT8U color_flag);
static void ap_display_icon_draw_rgb565(INT16U *buf, INT16U buf_w, INT16U buf_h, DISPLAY_ICONSHOW *icon_ptr, INT8U color_flag);
//static void ap_display_icon_draw_yuyv(INT16U *buf, INT16U buf_w, INT16U buf_h, DISPLAY_ICONSHOW *icon_ptr, INT8U color_flag);

static void ap_display_do_padding(DISP_P frame);

//external symbols
extern INT32S vid_dec_get_current_time(void);
extern void ap_browse_get_file_date_time(TIME_T *osd_time);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
INT16U		display_frame_pool_size;
DISP_T		display_frame_pool[DISPLAY_BUF_NUM];

DISP_QUEUE	display_ready_queue;
DISP_QUEUE	display_free_queue;
DISP_QUEUE  display_rotate_queue;

static DISP_P       display_frame_tft;
static DISP_P       display_frame_rot_in;
static DISP_P       display_frame_rot_out;
static DISP_P       display_frame_rot_keep;

static DISPLAY_REC_TIME	display_rec_time;

static INT8U        display_rec_time_enable = 0;        //REC_TIME_DRAW_MODE, LEFT_REC_TIME_DRAW_MODE or PLAY_TIME_DRAW_MODE
static INT32U       last_time_keep = 0;                 //keep last OSTimeTick or last play time

static INT8U        display_date_time_enable = 0;       //RTC_DATE_TIME_SHOW or FILE_DATE_TIME_SHOW

// rotate variables
static enum
{
	ROTATOR_IDLE,
	ROTATOR_BUSY
} rotator_st;

// effect variables
static INT32U display_icon_sts[(ICOM_MAX_NUM >> 5) + 1];
static DISPLAY_ICONSHOW     *string_icons[STR_ICONS_MAX];
#if C_WATER_MARK == CUSTOM_ON
static DISPLAY_ICONSHOW     *car_plate_icon = NULL;
#endif

#if  C_ASYN_DBLE_TFT
INT32U switch_tft_ap_mode;
#endif

#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
INT32U lvgl_display_buf = 0;
static INT32U display_path = DISPLAY_PATH_LVGL;
#endif
/***********************************************************************************************************/
/*                           E F F E C T      F U N C T I O N S                                            */
/***********************************************************************************************************/
INT32U ap_display_timer_rec_time_get(void)
{
	return display_rec_time.s + display_rec_time.m * 60 + display_rec_time.h * 3600;
}

void ap_display_effect_init(void)
{
	INT32U	i;

	for(i = 0; i < (ICOM_MAX_NUM >> 5) + 1; i++)
	{
		display_icon_sts[i] = 0;
	}

	for(i = 0; i < STR_ICONS_MAX; i++)
	{
        if(string_icons[i] && string_icons[i]->data)
        {
            gp_free(string_icons[i]->data);
        }

        if(string_icons[i])
        {
            gp_free(string_icons[i]);
            string_icons[i] = NULL;
        }
	}

    #if C_WATER_MARK == CUSTOM_ON
	ap_display_water_mark_icon_disable();
	#endif
}

void ap_display_icon_sts_set(INT32U msg)
{
	INT32U	i, icon_idx;

	icon_idx = 0;
	for(i = 0; i < 4; i++)
	{
		icon_idx = ((msg >> (i << 3)) & 0xFF);
		if(icon_idx)
		{
			display_icon_sts[icon_idx >> 5] |= 1 << (icon_idx & 0x1F);
		}
	}
}

void ap_display_icon_sts_clear(INT32U msg)
{
	INT32U	i, icon_idx;

	icon_idx = 0;
	for(i = 0; i < 4; i++)
	{
		icon_idx = ((msg >> (i << 3)) & 0xFF);
		if(icon_idx)
		{
			display_icon_sts[icon_idx >> 5] &= ~(1 << (icon_idx & 0x1F));
		}
	}
}

void ap_display_rec_time_enable(void)
{
    display_rec_time_enable = REC_TIME_DRAW_MODE;
    last_time_keep = xTaskGetTickCount();

    display_rec_time.h = 0;
    display_rec_time.m = 0;
    display_rec_time.s = 0;
}

void ap_display_rec_time_disable(void)
{
    display_rec_time_enable = 0;
}

void ap_display_left_rec_time_enable(DISPLAY_REC_TIME left_rec_time)
{
    display_rec_time_enable = LEFT_REC_TIME_DRAW_MODE;

    display_rec_time.h = left_rec_time.h;
    display_rec_time.m = left_rec_time.m;
    display_rec_time.s = left_rec_time.s;
}

void ap_display_play_time_enable(void)
{
    display_rec_time_enable = PLAY_TIME_DRAW_MODE;
    last_time_keep = 0;

    display_rec_time.h = 0;
    display_rec_time.m = 0;
    display_rec_time.s = 0;
}


void ap_display_rtc_date_time_enable(void)
{
    display_date_time_enable = RTC_DATE_TIME_SHOW;
}


void ap_display_file_date_time_enable(void)
{
    display_date_time_enable = FILE_DATE_TIME_SHOW;
}

void ap_display_rtc_date_time_disable(void)
{
    display_date_time_enable = 0;
}


void ap_display_string_icon_enable(INT16U idx, void *icon_ptr)
{
    DISPLAY_ICONSHOW    *ptr;

    ptr = icon_ptr;
    if(ptr && ptr->data && (idx < STR_ICONS_MAX))
    {
        ap_display_string_icon_disable(idx);
        string_icons[idx] = ptr;
    }
}


void ap_display_string_icon_disable(INT16U idx)
{
    if(idx < STR_ICONS_MAX)
    {
        if(string_icons[idx] && string_icons[idx]->data)
        {
            gp_free(string_icons[idx]->data);
        }

        if(string_icons[idx])
        {
            gp_free(string_icons[idx]);
            string_icons[idx] = NULL;
        }
    }
}


#if C_WATER_MARK == CUSTOM_ON
void ap_display_water_mark_icon_enable(void)
{
    car_plate_icon = &water_mark_icon;
}

void ap_display_water_mark_icon_disable(void)
{
    car_plate_icon = NULL;
}
#endif

/***********************************************************************************************************/
/*                           R O T A T E      F U N C T I O N S                                            */
/***********************************************************************************************************/

INT8U ap_display_rotate_busy(void)
{
	return (rotator_st != ROTATOR_IDLE);
}
void ap_display_rotate_buf(INT32U buf_in, INT32U buf_out, INT8U busy_wait, void (*isr_hook)(INT8U rot_num, INT32U flag))
{
	INT16U tft_w = drv_l2_display_tft_width_get();
	INT16U tft_h = drv_l2_display_tft_height_get();

	drv_l1_rotator_init(ROTATOR_2);
    drv_l1_rotator_burst16_set(ROTATOR_2, 0);
	drv_l1_rotator_tar_img_addr(ROTATOR_2, buf_out);

	switch(drv_l2_display_tft_fmt_get())
	{
    case DISP_FMT_RGB565:
        drv_l1_rotator_src_img_info(ROTATOR_2, IMAGE_RGB565, tft_w, tft_h, buf_in);
        break;
    case DISP_FMT_GP420:
        drv_l1_rotator_src_img_info(ROTATOR_2, IMAGE_GP420, tft_w, tft_h, buf_in);
        break;
    default:
        drv_l1_rotator_src_img_info(ROTATOR_2, IMAGE_YUYV, tft_w, tft_h, buf_in);
        break;
	}

	switch(drv_l2_display_tft_rot_get())
	{
    case TFT_ROT_RIGHT:
        drv_l1_rotator_mode_set(ROTATOR_2, ROTATOR_90);
        break;
    case TFT_ROT_LEFT:
        drv_l1_rotator_mode_set(ROTATOR_2, ROTATOR_270);
        break;
    case TFT_ROT_180:
        drv_l1_rotator_mode_set(ROTATOR_2, ROTATOR_180);
        break;
    default:
        drv_l1_rotator_mode_set(ROTATOR_2, ROTATOR_THETA);
        break;
	}

    drv_l1_rotator_dram2tft_set(ROTATOR_2, 0);
	drv_l1_rotator_isr_callback_set(ROTATOR_2, isr_hook);
	drv_l1_rotator_start(ROTATOR_2);

	if(busy_wait)
	{
		drv_l1_rotator_end_wait(ROTATOR_2, 1);
	}
}

void ap_display_rotate_isr(INT8U rot_num, INT32U flag)
{
    if((rot_num == ROTATOR_2) && (flag & C_ROTATOR_END_STATE))
    {
        display_task_msg_send(MSG_DISPLAY_TASK_ROTATE_DONE, NULL, 0, MSG_PRI_NORMAL);
    }
    else
    {
        DBG_PRINT("[ERR]: rot_num = 0x%x, flag = 0x%x\r\n", rot_num, flag);
    }
}

void ap_display_rotate_start(void)
{
	if(rotator_st != ROTATOR_IDLE)
	{
		return;
	}

	display_frame_rot_in = ap_display_queue_get(display_rotate_queue);
	if(!display_frame_rot_in)
	{
		return;
	}

	display_frame_rot_out = display_frame_rot_keep;
	if(!display_frame_rot_out)
	{
		display_frame_rot_out = ap_display_queue_get(display_free_queue);
	}

	if(!display_frame_rot_out)
	{
        ap_display_queue_put(display_free_queue, display_frame_rot_in);
        display_frame_rot_in = 0;
		return;
	}

	display_frame_rot_out->offset = display_frame_rot_in->offset;

	rotator_st = ROTATOR_BUSY;

	ap_display_rotate_buf(
		display_frame_rot_in->base
		, display_frame_rot_out->base
		, 0
		, ap_display_rotate_isr
	);
}

void ap_display_rotate_restart(void)
{
	/* show the frame that rotated done */
	ap_display_queue_put(display_ready_queue, display_frame_rot_out);

	if(drv_l2_display_lcd_ifce_get() == LCD_IFCE_I80)
	{
        drv_l2_tft_busy_wait();
        ap_display_vblank_isr();
        drv_l2_tft_en_set(TRUE);
	}

	display_frame_rot_out = NULL;

	/* keep last in-frame to rotate next frame */
	display_frame_rot_keep = display_frame_rot_in;
	display_frame_rot_in = NULL;

	/* release rotator*/
	rotator_st = ROTATOR_IDLE;

	/* starting a new job if there's queued frames */
	ap_display_rotate_start();
}


/***********************************************************************************************************/
/*                           D I S P L A Y      F U N C T I O N S                                          */
/***********************************************************************************************************/
INT32U getDispDevBufSize(void)
{
	INT32U size;
	#if C_ASYN_DBLE_TFT
	INT16U tft_w = drv_l2_asyn_multi_tft_maximum_width_get();
	INT16U tft_h = drv_l2_asyn_multi_tft_maximum_height_get();
	#else
	INT16U tft_w = drv_l2_display_tft_width_get();
	INT16U tft_h = drv_l2_display_tft_height_get();
	#endif

	if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
	{
		size = ((tft_w * (1 + tft_h)) * 3) >> 1;
	}
	else
	{
		size = (tft_w * (1 + tft_h)) << 1;
	}

	size += 0x003f;
	size &= ~0x003f;    //64Byte alignment

	return size;
}

void ap_display_init(void)
{
    INT32U i;
    INT32U buff_size;
    #if C_TFT_DISPLAY_FUNC
    DISP_P frm_startup;
    #endif

    INT32U uvc_buf_size =0;
    INT32U tft_buf_size =0;
    INT32U wifi_buf_size=0;

    #if C_WIFI_FUNC
    wifi_buf_size = wifi_display_buf_size_get();
    #endif

    #if C_USB_DEVICE
	uvc_buf_size = uvc_display_buf_size_get();
	#endif

	Displaydevice_assign();

    tft_buf_size = getDispDevBufSize();
    buff_size = max(max(uvc_buf_size, wifi_buf_size), tft_buf_size);

    #if C_TFT_DISPLAY_FUNC
    drv_l2_display_init();
    #endif

    if(!display_frame_pool[0].base || !display_frame_pool_size)
    {
        display_frame_pool_size = TFT_BUF_NUM;
        if((drv_l2_display_lcd_ifce_get() == LCD_IFCE_I80)
            && (drv_l2_display_tft_rot_get() != TFT_ROT_NONE)
        )
        {
            display_frame_pool_size += ROTATE_BUF_NUM;
        }

        #if C_WIFI_FUNC
        display_frame_pool_size += WIFI_SYMBOL_BUF_NUM;   //for static WIFI symbol display
        #endif

        if(display_frame_pool[0].base)
        {
            gp_free((void*)display_frame_pool[0].base);
        }

        display_frame_pool[0].base              = (INT32U) gp_malloc_align(buff_size * display_frame_pool_size, 64);
        display_frame_pool[0].offset            = 0;
        display_frame_pool[0].padding.top       = 0;
        display_frame_pool[0].padding.bottom    = 0;
        display_frame_pool[0].padding.left      = 0;
        display_frame_pool[0].padding.right     = 0;
		#if (defined C_HDMI_SCALE_UP_EN) && (C_HDMI_SCALE_UP_EN)
        display_frame_pool[0].scaleup.tgt_w     = drv_l2_display_tft_width_get();
        display_frame_pool[0].scaleup.tgt_h     = drv_l2_display_tft_height_get();
		#endif

        DBG_PRINT("display_frame(cnt=%d) addr = 0x%x\r\n"
            , display_frame_pool_size
            , display_frame_pool[0].base
        );

		if(display_frame_pool[0].base)
		{
			for(i = 1; i < display_frame_pool_size; ++i)
			{
                display_frame_pool[i].base              = display_frame_pool[i - 1].base + buff_size;
                display_frame_pool[i].offset            = 0;
                display_frame_pool[i].padding.top       = 0;
                display_frame_pool[i].padding.bottom    = 0;
                display_frame_pool[i].padding.left      = 0;
                display_frame_pool[i].padding.right     = 0;
				#if (defined C_HDMI_SCALE_UP_EN) && (C_HDMI_SCALE_UP_EN)
		        display_frame_pool[i].scaleup.tgt_w     = drv_l2_display_tft_width_get();
		        display_frame_pool[i].scaleup.tgt_h     = drv_l2_display_tft_height_get();
				#endif

                DBG_PRINT("display_frame_pool[%d] addr = 0x%x\r\n"
                    , i
                    , display_frame_pool[i].base
                );
			}
		}
        else
        {
            ERR_PRINT("display_frame_pool allocate fail\r\n");
        }

        for(i = 0; i < display_frame_pool_size; ++i)
        {
			ap_display_queue_put(display_free_queue, &display_frame_pool[i]);
        }
    }

    #if C_TFT_DISPLAY_FUNC
	frm_startup = ap_display_queue_get(display_free_queue);
	if(frm_startup)
	{
		frm_startup->offset = 0;
		ap_display_clear_screen(frm_startup->base);
		cache_invalid_range((INT32U)(frm_startup->base), buff_size);
	}

    display_frame_tft = frm_startup;
	drv_l2_tft_buffer_set(display_frame_tft->base);
	drv_l2_display_start(ap_display_vblank_isr);
	drv_l2_tft_backlight_en_set(TRUE);

	if(drv_l2_display_lcd_ifce_get() == LCD_IFCE_I80)
	{
        if(drv_l2_display_tft_rot_get() != TFT_ROT_NONE)
        {
            display_frame_rot_keep = ap_display_queue_get(display_free_queue);

            if(frm_startup && display_frame_rot_keep)
            {
                display_frame_rot_keep->offset = 0;
                ap_display_clear_screen(display_frame_rot_keep->base);
                cache_invalid_range((INT32U)(display_frame_rot_keep->base), buff_size);

                ap_display_rotate_buf(
                    display_frame_rot_keep->base
                    , frm_startup->base
                    , 1
                    , NULL
                );
            }
        }

        drv_l2_tft_busy_wait();
        drv_l2_tft_en_set(TRUE);
	}
	#endif
}

void ap_display_restart(TFT_ISR isr)
{
	DISP_P frame_new;
	INT32U buff_size;
	INT32U tft_buf_size =0;
	INT32U wifi_buf_size=0;

	/* reset the mipi tx restart,setting the dummy address to drv_l1_rotator_src_img_addr() to start hardware rotator isr */
	drv_l2_tft_buffer_set(DUMMY_BUFFER_ADDRS);

	drv_l2_display_start(isr);

	if(drv_l2_display_lcd_ifce_get() == LCD_IFCE_I80)
	{
		drv_l2_tft_busy_wait();
		drv_l2_tft_en_set(TRUE);
	}
}

void judge_and_release_display_buffer(DISP_P frame_show)
{
	INT32U i;

	if(frame_show == NULL)
	{
		return;
	}

	//check buff is got from display frame pool
	for(i = 0; i < display_frame_pool_size; ++i)
	{
		if(frame_show == &display_frame_pool[i])
		{
			ap_display_queue_put(display_free_queue, frame_show);
		}
	}
}

DISP_P displayaddr_to_displayframe(INT32U displayaddr)
{
       int i ;

	   for(i = 0; i < display_frame_pool_size; i++)
	   {
            if(display_frame_pool[i].base == displayaddr)
				break;
	   }

	#if C_ASYN_DBLE_TFT
	if(i==display_frame_pool_size)	/* double check if switch different TFT */
	{
		if(previous_tft_fmt() == DISP_FMT_RGB565) // *2
		{
			displayaddr-=(drv_l2_previous_display_width() * previous_h_off_lines_video() << 1);
		}
		else if(previous_tft_fmt()  == DISP_FMT_GP420) // *1.5
		{
			displayaddr-=(drv_l2_previous_display_width() * previous_h_off_lines_video() * 3 >> 1);
		}
		else	//GP422  *2
		{
			displayaddr-=(drv_l2_previous_display_width() * previous_h_off_lines_video() << 1);
		}

		for(i = 0; i < display_frame_pool_size; i++)
		{
			if(display_frame_pool[i].base == displayaddr)
			break;
		}

	}
	#endif


	   if(i == display_frame_pool_size)
	   {
	    	DBG_PRINT("Abnormal displayaddr\r\n");
			return NULL;
	   }
       return &display_frame_pool[i];
}

#define GET_START_POINT     0
#define GET_END_POINT       1

static INT16U   linepostion[3][4] = {
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
};

void ap_draw_line(INT8U get_start_end_flag, INT16U *buf, INT16U buf_w, INT16U buf_h, INT16S startx, INT16S starty, INT16S endx, INT16S endy, INT16U color)
{
    INT32S          xerr = 0, yerr = 0, delta_x, delta_y, incx, incy;
    INT32S          t, distance;
    INT16U			LINE_Y1_POSTION, LINE_Y2_POSTION, LINE_Y3_POSTION;
    INT16U			temp;

    if(startx < 0)  startx += buf_w;
    if(endx < 0)    endx += buf_w;
    if(starty < 0)  starty += buf_h;
    if(endy < 0)    endy += buf_h;

    temp = buf_h - starty;
    LINE_Y1_POSTION = starty + (temp * 1) / 9;
    LINE_Y2_POSTION = starty + (temp * 4) / 9;
    LINE_Y3_POSTION = starty + (temp * 7) / 9;

    /* compute the distances in both directions */
    delta_x = endx - startx;
    delta_y = endy - starty;

    /* Compute the direction of the increment,
    an increment of 0 means either a horizontal or vertical
    line.
    */
    if(delta_x > 0)
        incx = 1;
    else
    if(delta_x == 0)
        incx = 0;
    else
        incx = -1;

    if(delta_y > 0)
        incy = 1;
    else
    if(delta_y == 0)
        incy = 0;
    else
        incy = -1;

    /* determine which distance is greater */
    delta_x = ABS(delta_x);
    delta_y = ABS(delta_y);
    if(delta_x > delta_y)
        distance = delta_x;
    else
        distance = delta_y;

    /* draw the line */
    for(t = 0; t <= (distance + 1); t++)
    {
        if(startx > 0 && startx < buf_w && starty > 0 && starty < buf_h)
        {
            gp_paint_dot(buf, buf_w, buf_h, drv_l2_display_tft_fmt_get(), startx - 1, starty, color);
            gp_paint_dot(buf, buf_w, buf_h, drv_l2_display_tft_fmt_get(), startx, starty, color);
            gp_paint_dot(buf, buf_w, buf_h, drv_l2_display_tft_fmt_get(), startx + 1, starty, color);

            if(starty == LINE_Y1_POSTION)
            {
                if(get_start_end_flag == GET_START_POINT)
                {
                    linepostion[0][0] = startx;
                    linepostion[0][1] = starty;
                }
                else
                {
                    linepostion[0][2] = startx;
                    linepostion[0][3] = starty;
                }
            }
            else
            if(starty == LINE_Y2_POSTION)
            {
                if(get_start_end_flag == GET_START_POINT)
                {
                    linepostion[1][0] = startx;
                    linepostion[1][1] = starty;
                }
                else
                {
                    linepostion[1][2] = startx;
                    linepostion[1][3] = starty;
                }
            }
            else
            if(starty == LINE_Y3_POSTION)
            {
                if(get_start_end_flag == GET_START_POINT)
                {
                    linepostion[2][0] = startx;
                    linepostion[2][1] = starty;
                }
                else
                {
                    linepostion[2][2] = startx;
                    linepostion[2][3] = starty;
                }
            }
        }

        xerr += delta_x;
        yerr += delta_y;
        if(xerr > distance)
        {
            xerr -= distance;
            startx += incx;
        }

        if(yerr > distance)
        {
            yerr -= distance;
            starty += incy;
        }
    }
}

void ap_display_draw_line(DISP_P frame, INT16U buf_w, INT16U buf_h, INT8U mode)
{
    INT16U  start_x;
    INT16U  start_y;
    INT16U  end_x;
    //INT16U  end_y;
    INT16U *buf = (INT16U *)frame->base;
    INT16U  image_w = buf_w - (frame->padding.left + frame->padding.right);

    start_x = frame->padding.left + image_w / 3;//buf_w / 3;
    start_y = buf_h * 67 / 100 - mode * (buf_h * 4 / 100);
    end_x = buf_w - image_w / 3 - frame->padding.right;//(buf_w / 3);
    //end_y = start_y;
    ap_draw_line(GET_START_POINT, buf, buf_w, buf_h, start_x, start_y, frame->padding.left + (buf_h * 2) / 27, buf_h, YELLOW_COLOR);
    ap_draw_line(GET_END_POINT, buf, buf_w, buf_h, end_x, start_y, buf_w - (buf_h * 2) / 27 - frame->padding.right, buf_h, YELLOW_COLOR);

    for(; start_x < end_x; start_x++)
    {
        gp_paint_dot(buf, buf_w, buf_h, drv_l2_display_tft_fmt_get(), start_x, start_y, WHITE_COLOR);
        gp_paint_dot(buf, buf_w, buf_h, drv_l2_display_tft_fmt_get(), start_x, start_y + 1, WHITE_COLOR);
    }

    start_x = linepostion[0][0];
    start_y = linepostion[0][1];
    end_x = linepostion[0][2];
    //end_y = linepostion[0][3];
    for(; start_x < end_x; start_x++)
    {
        gp_paint_dot(buf, buf_w, buf_h, drv_l2_display_tft_fmt_get(), start_x, start_y, GREEN_COLOR);
        gp_paint_dot(buf, buf_w, buf_h, drv_l2_display_tft_fmt_get(), start_x, start_y + 1, GREEN_COLOR);
    }

    start_x = linepostion[1][0];
    start_y = linepostion[1][1];
    end_x = linepostion[1][2];
    //end_y = linepostion[1][3];
    for(; start_x < end_x; start_x++)
    {
        gp_paint_dot(buf, buf_w, buf_h, drv_l2_display_tft_fmt_get(), start_x, start_y, RED_COLOR);
        gp_paint_dot(buf, buf_w, buf_h, drv_l2_display_tft_fmt_get(), start_x, start_y + 1, RED_COLOR);
    }

    start_x = linepostion[2][0];
    start_y = linepostion[2][1];
    end_x = linepostion[2][2];
    //end_y = linepostion[2][3];
    for(; start_x < end_x; start_x++)
    {
        gp_paint_dot(buf, buf_w, buf_h, drv_l2_display_tft_fmt_get(), start_x, start_y, PURPLE_COLOR);
        gp_paint_dot(buf, buf_w, buf_h, drv_l2_display_tft_fmt_get(), start_x, start_y + 1, PURPLE_COLOR);
    }
}


void ap_display_mirror_pbp(INT32U buf)
{

	INT32S	i, j;
	INT32U	line_cnt = 0;
	INT32U	offset = 0;
    INT8U	temp8;
	INT16U	temp16;
	INT8U	*ptr8;
	INT16U	*ptr16;
	INT16U tft_width = drv_l2_display_tft_width_get();
	INT16U tft_height = drv_l2_display_tft_height_get();

    if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
    {
        line_cnt = ((tft_height * 3) >> 1);
        offset = 0;//tft_width / 2;

        for(i = 0 ; i < line_cnt; i++)
        {
            if((i % 3) == 1)
            {
                ptr16 = (INT16U *)(buf + offset + (tft_width * i));
                for(j = 0; j < (tft_width/4); j++)
                {
                    temp16 = *(ptr16 + j);
                    *(ptr16 + j) = *(ptr16 + (tft_width/2) - j - 1);
                    *(ptr16 + (tft_width/2) - j - 1) = temp16;
                }
            }
            else
            {
                ptr8 = (INT8U *)(buf + offset + (tft_width * i));
                for(j = 0; j < (tft_width/2); j++)
                {
                    temp8 = *(ptr8 + j);
                    *(ptr8 + j) = *(ptr8 + (tft_width/1) - j - 1);
                    *(ptr8 + (tft_width/1) - j - 1) = temp8;
                }
            }
        }
    }else if(drv_l2_display_tft_fmt_get() == DISP_FMT_RGB565){

         for(i = 0 ; i < tft_height; i++)
        {
            ptr16 = (INT16U *)(buf + offset + (tft_width*2 * i));
            for(j = 0; j < (tft_width/2); j++)
            {
                temp16 = *(ptr16 + j);
                *(ptr16 + j) = *(ptr16 + tft_width - j - 1);
                *(ptr16 + tft_width - j - 1) = temp16;
            }
        }
    }
    else
    {
        DBG_PRINT("mirror format not support\r\n");
    }
}

void ap_display_buff_copy_and_draw(DISP_P frame_show, INT32U src_type)
{
	INT16U tft_w = drv_l2_display_tft_width_get();
	INT16U tft_h = drv_l2_display_tft_height_get();
	DISP_P frame_get = NULL;
	INT32U buf_get;
	INT32U i;

	if(frame_show == NULL)
	{
		DBG_PRINT("=>frame_show null \r\n");
		return;
	}

	//check buff is got from display frame pool
	for(i = 0; i < display_frame_pool_size; ++i)
	{
		if(frame_show == &display_frame_pool[i])
		{
			frame_get = frame_show;
			break;
		}
	}

	//if not, get one and copy into
	if(frame_get == NULL)
	{
        ERR_PRINT("illegal frame = 0x%x, type = 0x%x\r\n", frame_show, src_type);

		frame_get = ap_display_queue_get(display_free_queue);
		if(frame_get != NULL)
		{
			INT32U	buf_size = getDispDevBufSize();
			INT32U	buf_show = frame_show->base;

			buf_get = frame_get->base;
			drv_l1_dma_buffer_copy(
				buf_show
				, buf_get
				, buf_size
				, tft_w << 1
				, tft_w << 1
			);

			frame_get->offset = frame_show->offset;
		}
	}

	//if got none from display frame pool, wait until getting successful
	if(!frame_get)
	{
		switch(src_type)
		{
		//cases must display
		case MSG_DISPLAY_TASK_JPEG_DRAW:
			DBG_PRINT("F1");

			osDelay(1);
			display_task_msg_send(MSG_DISPLAY_TASK_JPEG_DRAW, &frame_show, sizeof(void *), MSG_PRI_NORMAL);
            break;
		//otherwise, drop
		default:
			break;
		}

		return;
	}

	buf_get = frame_get->base;
    ap_display_do_padding(frame_get);

	//draw something before goes to show
	do
	{
		if(src_type == MSG_DISPLAY_TASK_JPEG_DRAW)
		{
            cache_invalid_range((INT32U)buf_get, getDispDevBufSize());
            break;
        }

		ap_display_timer();

		if(src_type == MSG_DISPLAY_TASK_BACK_A_CAR_DRAW)
		{
			ap_display_draw_line(frame_get, tft_w, tft_h, ap_state_config_draw_line_mode_get());
            cache_invalid_range((INT32U)buf_get, getDispDevBufSize());
			break;
		}

		#if (defined C_BLIND_SPOT_DETECTION) && (C_BLIND_SPOT_DETECTION)
		if(present_state == STATE_VIDEO_RECORD) {
			ap_display_bsd_draw((INT16U *)buf_get);
		}
		#endif

		ap_display_date_time_draw((INT16U *)buf_get);
		ap_display_rec_time_draw((INT16U *)buf_get);

        #if C_WATER_MARK == CUSTOM_ON
        ap_display_water_mark_icon_draw((INT16U *)buf_get);
        #endif

		ap_display_common_icon_draw((INT16U *)buf_get);
        ap_display_string_icon_draw((INT16U *)buf_get);

        cache_invalid_range((INT32U)buf_get, getDispDevBufSize());

	} while(0);

    frame_get->padding.top = 0;
    frame_get->padding.bottom = 0;
    frame_get->padding.left = 0;
    frame_get->padding.right = 0;

    if(drv_l2_display_lcd_ifce_get() == LCD_IFCE_I80)
    {
        if(drv_l2_display_tft_rot_get() == TFT_ROT_NONE)
        {
            ap_display_queue_put(display_ready_queue, frame_get);

            drv_l2_tft_busy_wait();
            ap_display_vblank_isr();
            drv_l2_tft_en_set(TRUE);
        }
        else
        {
            ap_display_queue_put(display_rotate_queue, frame_get);
            ap_display_rotate_start();
        }
    }
    else if(drv_l2_display_lcd_ifce_get() == LCD_IFCE_SPI)
    {
         if(drv_l2_display_tft_rot_get() != TFT_ROT_NONE)
        {
            ap_display_queue_put(display_rotate_queue, frame_get);
            ap_display_rotate_start();
        }
    }
    else
    {
		ap_display_queue_put(display_ready_queue, frame_get);
    }
}

#if (defined C_HDMI_SCALE_UP_EN) && (C_HDMI_SCALE_UP_EN)
void ap_display_scale_up(DISP_P frame_buf)
{
	if((frame_buf->scaleup.tgt_w > frame_buf->scaleup.src_w) ||
       (frame_buf->scaleup.tgt_h > frame_buf->scaleup.src_h)
	)
	{
		ppu_set_size(frame_buf->scaleup.src_w, frame_buf->scaleup.src_h);
		drv_l1_tft2_scaling_up_size_set(
			frame_buf->scaleup.src_w
			, frame_buf->scaleup.src_h
			, 0
			, 0
			, frame_buf->scaleup.tgt_w
			, frame_buf->scaleup.tgt_h
		);
	}
}
#endif

void ap_display_vblank_isr(void)
{
	DISP_P frame_new;

	++fps_tft_vblank;
	frame_new = ap_display_queue_get(display_ready_queue);

	if(frame_new)
	{
        #if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
        if(ap_state_handling_display_path_get() == DISPLAY_PATH_NORMAL)
        {
            //DBG_PRINT("frame buf 0x%x\r\n", frame_new->base);
            drv_l2_tft_buffer_set(frame_new->base);
        }
        #else
		drv_l2_tft_buffer_set(frame_new->base);
		#endif

		++fps_tft_update;
		if(display_frame_tft)
		{
			 ap_display_queue_put(display_free_queue, display_frame_tft);
		}
		display_frame_tft = frame_new;

		#if (defined C_HDMI_SCALE_UP_EN) && (C_HDMI_SCALE_UP_EN)
		ap_display_scale_up(frame_new);
		#endif
	}

	#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
    if(ap_state_handling_display_path_get() == DISPLAY_PATH_LVGL)
    {
        if(lvgl_display_buf != 0)
        {
            drv_l2_tft_buffer_set(lvgl_display_buf);
        }
    }
	#endif
}
#if C_ASYN_DBLE_TFT
/*
  this api will check if need to switch screen
  if yes , will resetting pscaler and inform display task to switch screen
  Note: due to resetting pscaler , the API should be called in  Pscaler interrupt
*/
INT32U ap_Handle_switch_screen_status(PS_ID PScalerNum,INT32U inW ,INT32U inH)
{
	INT16U outW,outH;
	INT32U factorW,factorH;
	INT32U cnt=0,loop_cnt=0;

	DISP_INFO*  pDisp_info = &pStreamArgs->disp_info;
	INT32U display_width = pDisp_info->display_width;
	INT32U display_height = pDisp_info->display_height;
	INT32U display_h_offset = pDisp_info->display_h_offset;
	INT32U display_format = pDisp_info->display_format;
	INT16U zoom_width;
	INT16U zoom_height;
	INT16U zoom_w_start;
	INT16U zoom_h_start;

	//DBG_PRINT("ap_Handle_switch_screen_status\r\n");

	if(Handle_switch_screen_status())
	{
		drv_l1_pscaler_stop1(PScalerNum); /* stop pscaler , but keep orginal interrupt setting */

		drv_l2_disable_te_pin();		/* disable i80 tft te pin *//* disable TE function */
		drv_l1_hdmi_exit();				/* disable HDMI */

		drv_l1_disp_dev_uninit(TFT_2);
		drv_msec_wait(1);
		drv_l1_mipi_tx_software_module_reset();	/* disable(reset) MIPI TX */
		drv_l2_display_uninit();		/* disable TFT2 */

		/* switch new TFT device */
		Switch_TFT_parameter();		/* renew TFT_Param to next TFT device */

        	outW = drv_l2_display_tft_width_get();		/* get new width of tft */
		outH = drv_l2_display_tft_height_get();	/* get new height of tft */

		if(outW >= (inW >> 1))
		{
			factorW = (inW << 16) / outW;
			if((inW << 16) % outW) ++factorW;
		}
		else
		{
			factorW = ((outW << 16) / (inW - 1)) + 1;
			if((outW << 16) % inW) ++factorW;
		}
		if(outH >= (inH >> 1))
		{
			factorH = (inH << 16) / outH;
			if((inH << 16) % outH) ++factorH;
		}
		else
		{
			factorH = ((outH << 16) / (inH - 1)) + 1;
			if((outH << 16) % inH) ++factorH;
		}

		//DBG_PRINT("setting pscaler pixel set inW %d, inH %d outW %d, outH %d\r\n",(INT16U)inW,(INT16U)inH,outW,outH);
		//DBG_PRINT("display_format %d\r\n");

		pDisp_info->display_format = drv_l2_display_tft_fmt_get();
		pDisp_info->pip_width = drv_l2_display_pip_width_get();
		pDisp_info->pip_height = drv_l2_display_pip_height_get();
		pDisp_info->pip_pos_x = drv_l2_display_pip_x_get();
		pDisp_info->pip_pos_y = drv_l2_display_pip_y_get();

		pDisp_info->display_width=outW;
	 	pDisp_info->display_height=outH;

		display_width = pDisp_info->display_width;
	 	display_height = pDisp_info->display_height;
		zoom_w_start = 0;
		zoom_h_start = 0;
		zoom_width=outW;
		zoom_height=outH;

		display_h_offset &= ~0x0001;
		display_height -= display_h_offset*2;

		/* set new parameters of the preview front */
		sensor_preview_zoom_pixel_set(PREVIEW_SRC_FRONT, zoom_width, zoom_height, zoom_w_start, zoom_h_start);

		drv_l1_pscaler_pixels_set(PScalerNum,(INT16U)inW,(INT16U)inH,outW,outH,factorW,factorH);

		drv_l1_pscaler_output_fifo_line_set(PScalerNum, outH, 0);

		sensor_preview_display_pixel_set(outW,outH);		/* renew preview width and hight of display */

		if(display_format == DISP_FMT_RGB565)
		{
			sensor_preview_display_format_set(PIPELINE_SCALER_OUTPUT_FORMAT_RGB565);
			sensor_preview_display_offset_set(outW * display_h_offset << 1);
		}
		else if(display_format == DISP_FMT_GP420)
		{
			sensor_preview_display_format_set(PIPELINE_SCALER_OUTPUT_FORMAT_GP420);
			sensor_preview_display_offset_set(outW * display_h_offset * 3 >> 1);
		}
		else
		{
			sensor_preview_display_format_set(PIPELINE_SCALER_OUTPUT_FORMAT_YUYV);
			sensor_preview_display_offset_set(outW * display_h_offset << 1);
		}

		//drv_l1_pscaler_start(PScalerNum);	/* start pscaler */

		Switch_screen_ACK();  /* send ack to inform display task to set another screen  */

		return 1;
  }
  else
  {
		return 0;
  }

}

void ap_Switch_Screen(void)
{
	tft_start_switch_screen_set(1);		/* tft switch start */

	if(ap_video_record_sts_get() & VIDEO_RECORD_BUSY)	/* stop recording in record mode */
	{
		tft_recording_tft_switch_set(1);
		app_msg_send(MSG_APQ_FUNCTION_KEY_ACTIVE, NULL, 0, MSG_PRI_NORMAL);	/* tft switch. if recording,need to stop recording */
	}
	tft_switch_wait_ACK();

	Switch_Screen(ap_display_vblank_isr);

	/* do preview on/off ; sensor on/off */
	video_encode_preview_off();
	video_encode_sensor_off();
	osDelay(100);//wait sensor frame to stop
	//drv_msec_wait(1);
	video_encode_sensor_on(ap_switch_tft_ap_mode_get());
	video_encode_preview_mode_set(PREVIEW_MODE_FRONT);		/* set front */


}

INT32U ap_switch_tft_ap_mode_get(void)		/*ap mode = record mode or photo mode */
{
	return switch_tft_ap_mode;
}

void ap_switch_tft_ap_mode_set(INT32U ap_mode)	/* set switch tft ap mode */
{
	switch_tft_ap_mode = ap_mode;

}

#endif		/*#if  C_ASYN_DBLE_TFT */

static void ap_display_do_padding(DISP_P frame)
{
	INT16U tft_w = drv_l2_display_tft_width_get();
	INT16U tft_h = drv_l2_display_tft_height_get();
	INT32U i, j, cnt, off;
	INT16U *ptr;

    if(frame->padding.top)
    {
        if(frame->padding.top & 0x0001)
        {
            frame->padding.top += 0x0001;
        }
    }

	switch(drv_l2_display_tft_fmt_get())
	{
    case DISP_FMT_GP420:
        frame->offset += (frame->padding.top * tft_w * 3) >> 1;
        if(frame->offset)
        {
            cnt = frame->offset / tft_w;
            for(i = 0; i < cnt; i++)
            {
                if((i % 3) == 1)
                {
                    gp_memset_32((INT32S*)(frame->base + i * tft_w), 0x80808080, tft_w);
                }
                else
                {
                    gp_memset_32((INT32S*)(frame->base + i * tft_w), 0, tft_w);
                }
            }
        }

        //padding
        if(frame->padding.left || frame->padding.right)
        {
            cnt = (tft_h * 3) >> 1;
            for(i = 0; i < cnt; i++)
            {
                if((i % 3) == 1)
                {
                    if(frame->padding.left)
                    {
                        gp_memset((INT8S *)(frame->base + i * tft_w), 0x80, frame->padding.left);
                    }

                    if(frame->padding.right)
                    {
                        gp_memset((INT8S *)(frame->base + (i + 1) * tft_w - frame->padding.right), 0x80, frame->padding.right);
                    }
                }
                else
                {
                    if(frame->padding.left)
                    {
                        gp_memset((INT8S *)(frame->base + i * tft_w), 0, frame->padding.left);
                    }

                    if(frame->padding.right)
                    {
                        gp_memset((INT8S *)(frame->base + (i + 1) * tft_w - frame->padding.right), 0, frame->padding.right);
                    }
                }
            }
        }

        if(frame->padding.bottom)
        {
            if(frame->padding.bottom & 0x0001)
            {
                frame->padding.bottom += 0x0001;
            }

            cnt = (frame->padding.bottom * 3) >> 1;
            off = (tft_h - frame->padding.bottom) * 3 >> 1;
            for(i = 0; i < cnt; i++)
            {
                if((i % 3) == 1)
                {
                    gp_memset((INT8S *)(frame->base + (i + off) * tft_w), 0x80, tft_w);
                }
                else
                {
                    gp_memset((INT8S *)(frame->base + (i + off) * tft_w), 0, tft_w);
                }
            }
        }
        break;

    case DISP_FMT_YUYV:
        frame->offset += frame->padding.top * tft_w * 2;
        if(frame->offset)
        {
            gp_memset_32((INT32S*)frame->base, 0x00800080UL, frame->offset);
        }

        if(frame->padding.left || frame->padding.right)
        {
            for(i = 0; i < tft_h; i++)
            {
                if(frame->padding.left)
                {
                    ptr = (INT16U *)(frame->base + i * tft_w * 2);
                    for(j = 0; j < frame->padding.left; j++)
                    {
                        *ptr++ = 0x0080;
                    }
                }

                if(frame->padding.right)
                {
                    ptr = (INT16U *)(frame->base + (i + 1) * tft_w * 2 - frame->padding.right * 2);
                    for(j = 0; j < frame->padding.right; j++)
                    {
                        *ptr++ = 0x0080;
                    }
                }
            }
        }

        if(frame->padding.bottom)
        {
            ptr = (INT16U *)(frame->base + (tft_h - frame->padding.bottom) * tft_w * 2);
            for(i = 0; i < frame->padding.bottom; i++)
            {
                for(j = 0; j < tft_w; j++)
                {
                    *ptr++ = 0x0080;
                }
            }
        }
        break;

    default: //RGB565
        frame->offset += frame->padding.top * tft_w * 2;
        if(frame->offset)
        {
            gp_memset_32((INT32S*)frame->base, 0x0, frame->offset);
        }

        if(frame->padding.left || frame->padding.right)
        {
            INT16U *ptr;

            for(i = 0; i < tft_h; i++)
            {
                if(frame->padding.left)
                {
                    ptr = (INT16U *)(frame->base + i * tft_w * 2);
                    for(j = 0; j < frame->padding.left; j++)
                    {
                        *ptr++ = 0;
                    }
                }

                if(frame->padding.right)
                {
                    ptr = (INT16U *)(frame->base + (i + 1) * tft_w * 2 - frame->padding.right * 2);
                    for(j = 0; j < frame->padding.right; j++)
                    {
                        *ptr++ = 0;
                    }
                }
            }
        }

        if(frame->padding.bottom)
        {
            ptr = (INT16U *)(frame->base + (tft_h - frame->padding.bottom) * tft_w * 2);
            for(i = 0; i < frame->padding.bottom; i++)
            {
                for(j = 0; j < tft_w; j++)
                {
                    *ptr++ = 0;
                }
            }
        }
        break;
	}
}

void ap_display_clear_screen(INT32U buf)
{
	INT16U tft_w = drv_l2_display_tft_width_get();
	INT16U tft_h = drv_l2_display_tft_height_get();
	INT32U i, cnt;

	switch(drv_l2_display_tft_fmt_get())
	{
    case DISP_FMT_GP420:
        cnt = (tft_h * 3) >> 1;
        for(i = 0; i < cnt; i++)
        {
            if((i % 3) == 1)
            {
                gp_memset_32((INT32S*)(buf + i * tft_w), 0x80808080, tft_w);
            }
            else
            {
                gp_memset_32((INT32S*)(buf + i * tft_w), 0, tft_w);
            }
        }
        break;

    case DISP_FMT_YUYV:
        gp_memset_32((INT32S*)buf, 0x00800080UL, (tft_w * tft_h) << 1);
        break;

    default:
        gp_memset_32((INT32S*)buf, 0x0, (tft_w * tft_h) << 1);
        break;
	}
}

DISP_P ap_display_showing_frame_get(void)
{
    if((drv_l2_display_lcd_ifce_get() == LCD_IFCE_I80)
    && (drv_l2_display_tft_rot_get() != TFT_ROT_NONE))
    {
        return display_frame_rot_keep;
    }
    else
    {
        return display_frame_tft;
    }
}

void ap_display_timer(void)
{
	if(display_rec_time_enable == REC_TIME_DRAW_MODE)
	{
        INT32U	current_os_time;
        INT32S	delta_time;
        INT8U	time_interval = ap_state_config_record_minute_get();
        #if C_TIME_LAPSE
        INT8U time_lapse_flag = ap_state_config_time_lapse_recording_get();
        INT8U time_lapse_temp = ap_state_config_time_lapse_recording_temp_get();
        INT8U time_lapse_fps = ap_state_config_time_lapse_fps_get();
        #endif
		current_os_time = xTaskGetTickCount();
		delta_time = (INT32S) current_os_time - (INT32S) last_time_keep;
		if(delta_time > 0)
		{
			delta_time /= configTICK_RATE_HZ;	//second
			if(delta_time)  //delta_time is greater than 1 second
			{
                #if C_TIME_LAPSE
				if (time_lapse_flag)
				{
                    if(delta_time >= time_lapse_fps*time_lapse_temp)
                    {
                        last_time_keep += delta_time * configTICK_RATE_HZ;
                        display_rec_time.s += 1;
                    }
				}
				else
				#endif
				{
                    last_time_keep += delta_time * configTICK_RATE_HZ;
                    display_rec_time.s += delta_time;
				}

				if(display_rec_time.s > 59)
				{
					display_rec_time.s = 0;
					display_rec_time.m++;
					if(display_rec_time.m > 59)
					{
						display_rec_time.m = 0;
						display_rec_time.h++;
						if(display_rec_time.h > 99)
						{
							display_rec_time.h = 99;
						}
					}
				}
                #if (defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
                if(present_state == STATE_AUDIO_RECORD)
                return;
                #endif
                //stop timer counting
                if((time_interval != 0) && (display_rec_time.m == time_interval))
                {
                    display_rec_time.s = 0;
                }
			}
		}
	}
	else if(display_rec_time_enable == LEFT_REC_TIME_DRAW_MODE)
	{
        //do nothing
	}
    else if(display_rec_time_enable == PLAY_TIME_DRAW_MODE)
	{
		INT32S sec = vid_dec_get_current_time();

        if((sec != last_time_keep) && (sec >= 0))
        {
			last_time_keep = sec;
			display_rec_time.s = sec % 60;
			sec /= 60;
			display_rec_time.m = sec % 60;
			sec /= 60;
			display_rec_time.h = sec % 100;
        }
	}
}

void ap_display_date_time_draw(INT16U *frame_buff)
{
	INT16U  tft_w = drv_l2_display_tft_width_get();
	INT16U  tft_h = drv_l2_display_tft_height_get();
	TIME_T	osd_time;
	INT32S	year, i;
	INT8U	color_flag, dt_mode;
	INT8U	num_tmp0, num_tmp1;
	INT8U   *ptr_y_hi, *ptr_y, *ptr_m, *ptr_d;
	INT16U  pos_x, pos_y;
	INT16U	pos_x_shift = 0;

    INT8U date_str[] = "0000/00/00";

	if(display_date_time_enable == 0)
	{
        return;
	}
    else if(display_date_time_enable == RTC_DATE_TIME_SHOW)
    {
        color_flag = 0;

        pos_x = POSITION_RTC_DATE_TIME_X;
        if(pos_x < 0)   pos_x += tft_w;

        pos_y = POSITION_RTC_DATE_TIME_Y;
        if(pos_y < 0)   pos_y += tft_h;

        ap_peripheral_osd_time_get(&osd_time);
    }
    else if(display_date_time_enable == FILE_DATE_TIME_SHOW)
    {
        color_flag = 0;

        pos_x = POSITION_FILE_DATE_TIME_X;
        if(pos_x < 0)   pos_x += tft_w;

        pos_y = POSITION_FILE_DATE_TIME_Y;
        if(pos_y < 0)   pos_y += tft_h;

        ap_browse_get_file_date_time(&osd_time);
    }

    // YEAR
    dt_mode = ap_state_config_date_time_mode_get();
    if(dt_mode == 0) // YY/MM/DD
    {
        ptr_y_hi = &date_str[0];
        ptr_y = &date_str[2];
        ptr_m = &date_str[5];
        ptr_d = &date_str[8];

        date_str[4] = NUM_TEXT_SLASH;
        date_str[7] = NUM_TEXT_SLASH;
    }
    else if(dt_mode == 1) // DD/MM/YY
    {
        ptr_d = &date_str[0];
        ptr_m = &date_str[3];
        ptr_y_hi = &date_str[6];
        ptr_y = &date_str[8];

        date_str[2] = NUM_TEXT_SLASH;
        date_str[5] = NUM_TEXT_SLASH;
    }
    else // MM/DD/YY
    {
        ptr_m = &date_str[0];
        ptr_d = &date_str[3];
        ptr_y_hi = &date_str[6];
        ptr_y = &date_str[8];

        date_str[2] = NUM_TEXT_SLASH;
        date_str[5] = NUM_TEXT_SLASH;
    }

    // YEAR
    ptr_y_hi[0] = 2;
    ptr_y_hi[1] = 0;

    year = (osd_time.tm_year) - 2000;
    if(year > 99)
    {
        while(year > 99)
        {
            year -= 100;
            ptr_y_hi[1] += 1;
        }
    }

    num_tmp0 = (INT8U)year;
    num_tmp1 = num_tmp0 / 10;
    num_tmp0 -= num_tmp1 * 10;
    ptr_y[0] = num_tmp1;
    ptr_y[1] = num_tmp0;

    // MONTH
    num_tmp0 = (INT8U)(osd_time.tm_mon);
    num_tmp1 = num_tmp0 / 10;
    num_tmp0 -= num_tmp1 * 10;
    ptr_m[0] = num_tmp1;
    ptr_m[1] = num_tmp0;

    // DAY
    num_tmp0 = (INT8U) (osd_time.tm_mday);
    num_tmp1 = num_tmp0 / 10;
    num_tmp0 -= num_tmp1 * 10;
    ptr_d[0] = num_tmp1;
    ptr_d[1] = num_tmp0;

    //draw year, month, day
    for(i = 0; i < 10; i++)
    {
        pos_x_shift = ap_display_digit_draw(frame_buff, date_str[i], (i == 0) ? pos_x : pos_x_shift, pos_y, color_flag);
    }
    pos_x_shift += DIGIT_GAP * 3;       //shift some more pixels

    // HOUR
    num_tmp0 = (INT8U) (osd_time.tm_hour);
    num_tmp1 = num_tmp0 / 10;
    num_tmp0 -= num_tmp1 * 10;
    pos_x_shift = ap_display_digit_draw(frame_buff, num_tmp1, pos_x_shift, pos_y, color_flag);
    pos_x_shift = ap_display_digit_draw(frame_buff, num_tmp0, pos_x_shift, pos_y, color_flag);
    pos_x_shift = ap_display_digit_draw(frame_buff, NUM_TEXT_COLON, pos_x_shift, pos_y, color_flag);

    // MINUTE
    num_tmp0 = (INT8U) (osd_time.tm_min);
    num_tmp1 = num_tmp0 / 10;
    num_tmp0 -= num_tmp1 * 10;
    pos_x_shift = ap_display_digit_draw(frame_buff, num_tmp1, pos_x_shift, pos_y, color_flag);
    pos_x_shift = ap_display_digit_draw(frame_buff, num_tmp0, pos_x_shift, pos_y, color_flag);
    pos_x_shift = ap_display_digit_draw(frame_buff, NUM_TEXT_COLON, pos_x_shift, pos_y, color_flag);

	// SECOND
	num_tmp0 = (INT8U)(osd_time.tm_sec);
	num_tmp1 = num_tmp0 / 10;
	num_tmp0 -= num_tmp1 * 10;
	pos_x_shift = ap_display_digit_draw(frame_buff, num_tmp1, pos_x_shift, pos_y, color_flag);
	pos_x_shift = ap_display_digit_draw(frame_buff, num_tmp0, pos_x_shift, pos_y, color_flag);
}


void ap_display_rec_time_draw(INT16U *frame_buff)
{
	INT16U  tft_w = drv_l2_display_tft_width_get();
	INT16U  tft_h = drv_l2_display_tft_height_get();
    INT8U   color_flag;
    INT16S  pos_x, pos_y;
	INT32U	i;
	INT8U	num_tmp0, num_tmp1;
	INT16U	pos_x_shift;

    if(display_rec_time_enable == 0)
    {
        return;
    }
	else if(display_rec_time_enable == REC_TIME_DRAW_MODE)
	{
        pos_x = POSITION_REC_TIME_X;
        if(pos_x < 0)   pos_x += tft_w;

        pos_y = POSITION_REC_TIME_Y;
        if(pos_y < 0)   pos_y += tft_h;

        color_flag = DISPLAY_ICON_RED_FLAG;
    }
    else if(display_rec_time_enable == LEFT_REC_TIME_DRAW_MODE)
	{
        pos_x = POSITION_LEFT_REC_TIME_X;
        if(pos_x < 0)   pos_x += tft_w;

        pos_y = POSITION_LEFT_REC_TIME_Y;
        if(pos_y < 0)   pos_y += tft_h;

        color_flag = 0;
    }
    else//if(display_rec_time_enable == PLAY_TIME_DRAW_MODE)
    {
        pos_x = POSITION_PLAY_TIME_X;
        if(pos_x < 0)   pos_x += tft_w;

        pos_y = POSITION_PLAY_TIME_Y;
        if(pos_y < 0)   pos_y += tft_h;

        color_flag = 0;
    }

    pos_x_shift = pos_x;
    for(i = 0; i < 3; i++)
    {
        if(i == 0)  num_tmp0 = display_rec_time.h;
        else if(i == 1)  num_tmp0 = display_rec_time.m;
        else if(i == 2)  num_tmp0 = display_rec_time.s;

        num_tmp1 = num_tmp0 / 10;
        num_tmp0 -= num_tmp1 * 10;

        pos_x_shift = ap_display_digit_draw(frame_buff, num_tmp1, pos_x_shift, pos_y, color_flag);
        pos_x_shift = ap_display_digit_draw(frame_buff, num_tmp0, pos_x_shift, pos_y, color_flag);
        if(i < 2)
        {
            pos_x_shift = ap_display_digit_draw(frame_buff, NUM_TEXT_COLON, pos_x_shift, pos_y, color_flag);
        }
	}
}


static INT16S ap_display_digit_draw(INT16U *buf, INT8U digit, INT16S pos_x, INT16S pos_y, INT8U color_flag)
{
    INT16U tft_w = drv_l2_display_tft_width_get();
    INT16U tft_h = drv_l2_display_tft_height_get();
    DISPLAY_ICONSHOW icon;

    if(digit < NUM_MAX)
    {
        icon = display_num_item[digit];

        icon.pos_x = pos_x;
        icon.pos_y = pos_y;
        ap_display_icon_draw(buf, tft_w, tft_h, &icon, color_flag);
        pos_x += icon.icon_w;
        pos_x += DIGIT_GAP;
    }
    return pos_x;
}


void ap_display_common_icon_draw(INT16U *buf)
{
    INT16U tft_w = drv_l2_display_tft_width_get();
    INT16U tft_h = drv_l2_display_tft_height_get();
    INT32S  i;

    for(i = 0; i < ICOM_MAX_NUM; i++)
    {
        if(display_icon_sts[i >> 5] & (1 << (i & 0x1F)))
        {
            if(i != ICON_RECORD_REC)
            {
                ap_display_icon_draw(buf, tft_w, tft_h, &display_icon_item[i], 0);
            }
            else if((i == ICON_RECORD_REC) && !(display_rec_time.s & 0x1))  //make the ICON_RECORD_REC Blink!
            {
                ap_display_icon_draw(buf, tft_w, tft_h, &display_icon_item[i], DISPLAY_ICON_RED_FLAG);  //red light
            }
        }
    }
}


void ap_display_string_icon_draw(INT16U *buf)
{
    INT32U i;
    INT16U tft_w = drv_l2_display_tft_width_get();
    INT16U tft_h = drv_l2_display_tft_height_get();

    for(i = 0; i < STR_ICONS_MAX; i++)
    {
        if(string_icons[i] && string_icons[i]->data)
        {
            ap_display_icon_draw(buf, tft_w, tft_h, string_icons[i], 0);
        }
    }
}

#if (defined C_BLIND_SPOT_DETECTION) && (C_BLIND_SPOT_DETECTION)
#include "ap_blind_spot_detection.h"
#include "LDW_api.h"
extern xQueueHandle disp_draw_state_queue;
#define ALARM_DURATION  24
#define ADAS_WIDTH	OBJ_PROCESS_SRC_WIDTH
#define ADAS_HEIGHT OBJ_PROCESS_SRC_HEIGHT
int g_draw_left = 0;
int g_draw_right = 0;

int ap_sound_bsd_play(IMAGE_FRAME_STRUCT	*disp_bsd)
{
	static int snd_state = 0;
	static int snd_state_bak = 0;

	if (snd_state==0)	// idle
	{
		if ( disp_bsd->LDW_process_follow_dist>0 ) {
			snd_state = snd_state_bak = 100;
		}
		else if ( (disp_bsd->LDW_process_lane_state==1)||(disp_bsd->LDW_process_lane_state==2) ) {
			snd_state = snd_state_bak = 1;
		}
		else if (disp_bsd->SnG_AlarmFlg==1) {
			snd_state = snd_state_bak = 200;
		}
		else if (disp_bsd->pedestrian_alarmFlg==1) {
			snd_state = snd_state_bak = 300;
		}
		else if (disp_bsd->virtual_bumperFlg==1) {
			snd_state = snd_state_bak = 400;
		}
	}
	else {
		// Allow interruption of sound
		int state = 0;
		if ( disp_bsd->LDW_process_follow_dist>0 ) {
			state = 100;
		}
		else if ( (disp_bsd->LDW_process_lane_state==1)||(disp_bsd->LDW_process_lane_state==2) ) {
			state = 1;
		}
		else if (disp_bsd->SnG_AlarmFlg==1) {
			state = 200;
		}
		else if (disp_bsd->pedestrian_alarmFlg==1) {
			state = 300;
		}
		else if (disp_bsd->virtual_bumperFlg==1) {
			state = 400;
		}
		if (state != snd_state_bak) {
			snd_state = state;
			DBG_PRINT("new ");
		}
	}

	if (snd_state!=0) DBG_PRINT("snd=%d\r\n",snd_state);
	switch(snd_state)
	{
		// IDLE
		case 0:
			break;
		// LDW
		case 1:
			if (disp_bsd->LDW_process_lane_state==1) {
				audio_effect_play(VOICE_ID_LDW_LEFT);
				g_draw_left = ALARM_DURATION;
				g_draw_right = 0;
			}
			else {
				audio_effect_play(VOICE_ID_LDW_RIGHT);
				g_draw_left = 0;
				g_draw_right = ALARM_DURATION;
			}
			snd_state = 2;
			break;
		case 2:		// Wait for the sound to finish
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			snd_state++;
			break;
		case 16:
			snd_state = 0;
			break;
		// FCW
		case 100:
			audio_effect_play(VOICE_ID_FCW_ALARM);
			g_draw_left = ALARM_DURATION;
			g_draw_right = ALARM_DURATION;
			snd_state = 101;
			break;
		case 101:	// Wait for the sound to finish
		case 102:
		case 103:
		case 104:
		case 105:
		case 106:
		case 107:
		case 108:
		case 109:
		case 110:
			snd_state++;
			break;
		case 111:
			snd_state = 0;
			break;
		// SnG
		case 200:
			audio_effect_play(VOICE_ID_SnG);
			snd_state = 201;
			break;
		case 201:	// Wait for the sound to finish
		case 202:
		case 203:
		case 204:
		case 205:
		case 206:
		case 207:
		case 208:
		case 209:
		case 210:
			snd_state++;
			break;
		case 211:
			snd_state = 0;
			break;
		// pedestrian
		case 300:
			audio_effect_play(VOICE_ID_PEDESTRIAN);
			snd_state = 301;
			break;
		case 301:	// Wait for the sound to finish
		case 302:
		case 303:
		case 304:
		case 305:
		case 306:
		case 307:
		case 308:
		case 309:
		case 310:
			snd_state++;
			break;
		case 311:
			snd_state = 0;
			break;
		// bumper
		case 400:
			audio_effect_play(VOICE_ID_VIRTUAL_BUMPER);
			snd_state = 401;
			break;
		case 401:	// Wait for the sound to finish
		case 402:
		case 403:
		case 404:
		case 405:
		case 406:
		case 407:
		case 408:
		case 409:
		case 410:
			snd_state++;
			break;
		case 411:
			snd_state = 0;
			break;
		default:
			snd_state++;
	}

	return 0;
}

void ap_display_bsd_draw(INT16U *buf)
{
    INT32U item;
	IMAGE_FRAME_STRUCT	*disp_bsd;
	INT32U vendor_width, vendor_height;
    INT16U tft_w = drv_l2_display_tft_width_get();
    INT16U tft_h = drv_l2_display_tft_height_get();
	INT16U line_width = 1;
	DISP_FMT type = DISP_FMT_RGB565; //DISP_FMT_GP420;

	vendor_width = BSD_IN_W;
	vendor_height = BSD_IN_H;

	item = nn_process_queue_buf_get(disp_draw_state_queue, 0);
	if (item != 0)
	{
		INT32U i;
		gpRect *obj_r;
		gpPoint *obj_p, *obj_p2;
		disp_bsd = (IMAGE_FRAME_STRUCT *)item;
		//DBG_PRINT("tft_w=%d, tft_h=%d\r\n",tft_w,tft_h);
		//DBG_PRINT("buf=0x%x, csi=0x%x\r\n",buf,disp_bsd->image_frame_ptr);
		for (i=0;i<OBJECT_MAX_RESULT;++i)
		{
			obj_r = &(disp_bsd->object_result_roi[i]);
			if (obj_r->width!=0)
			{
				int x = obj_r->x*tft_w/ADAS_WIDTH;
				int y = obj_r->y*tft_h/ADAS_HEIGHT;
				int width = obj_r->width*tft_w/ADAS_WIDTH;
				int height = obj_r->height*tft_h/ADAS_HEIGHT;				
				gp_paint_rect(buf, tft_w, tft_h, type, x, y, width, height, 1, 0, WHITE_COLOR);
			}
		}
		for (i=0;i<NUM_KEYPOINTS;i++)
		{
			obj_p = &(disp_bsd->object_result_dot[i]);
			if ((obj_p->x!=0)&&(obj_p->y!=0))
			{
				int x = obj_p->x*tft_w/ADAS_WIDTH;
				int y = obj_p->y*tft_h/ADAS_HEIGHT;
				gp_paint_circle(buf, tft_w, tft_h, type, x, y, 2, 1, RED_COLOR);
			}
		}

        obj_p = &(disp_bsd->lane_result_dot[0]);
        obj_p2 = &(disp_bsd->lane_result_dot[1]);
        if ((obj_p->x!=0)&&(obj_p->y!=0)&&(obj_p2->x!=0)&&(obj_p2->y!=0))
        {
        	int x1 = obj_p->x*tft_w/ADAS_WIDTH;
			int y1 = obj_p->y*tft_h/ADAS_HEIGHT;
			int x2 = obj_p2->x*tft_w/ADAS_WIDTH;
			int y2 = obj_p2->y*tft_h/ADAS_HEIGHT;
		    gp_paint_line(buf, tft_w, tft_h, type, x1, y1, x2, y2, GREEN_COLOR);
        }

        obj_p = &(disp_bsd->lane_result_dot[2]);
        obj_p2 = &(disp_bsd->lane_result_dot[3]);
        if ((obj_p->x!=0)&&(obj_p->y!=0)&&(obj_p2->x!=0)&&(obj_p2->y!=0))
        {
        	int x1 = obj_p->x*tft_w/ADAS_WIDTH;
			int y1 = obj_p->y*tft_h/ADAS_HEIGHT;
			int x2 = obj_p2->x*tft_w/ADAS_WIDTH;
			int y2 = obj_p2->y*tft_h/ADAS_HEIGHT;
		    gp_paint_line(buf, tft_w, tft_h, type, x1, y1, x2, y2, GREEN_COLOR);
        }

		obj_r = &(disp_bsd->object_follow_roi);
		if (obj_r->width!=0)
		{
			int x = obj_r->x*tft_w/ADAS_WIDTH;
			int y = obj_r->y*tft_h/ADAS_HEIGHT;
			int width = obj_r->width*tft_w/ADAS_WIDTH;
			int height = obj_r->height*tft_h/ADAS_HEIGHT;
		    gp_paint_rect(buf, tft_w, tft_h, type, x, y, width, height, 1, 0, RED_COLOR);
		}

		obj_r = &(disp_bsd->input_lock_roi);
		if (obj_r->width!=0)
		{
			int x = obj_r->x*tft_w/ADAS_WIDTH;
			int y = obj_r->y*tft_h/ADAS_HEIGHT;
			int width = obj_r->width*tft_w/ADAS_WIDTH;
			int height = obj_r->height*tft_h/ADAS_HEIGHT;
		    gp_paint_rect(buf, tft_w, tft_h, type, x, y, width, height, 2, 0, YELLOW_COLOR);
		}

		// 320x240 convert to display  W x H
		int x1,y1,x2,y2;
		x1 = 0;
		y1 = 150 * tft_h / ADAS_HEIGHT;
		x2 = 320 * tft_w / ADAS_WIDTH;
		y2 = 150 * tft_h / ADAS_HEIGHT;
		gp_paint_line(buf, tft_w, tft_h, type, x1, y1, x2, y2, BLUE_COLOR);
		x1 = 160 * tft_w / ADAS_WIDTH;
		y1 = 150 * tft_h / ADAS_HEIGHT;
		x2 = 160 * tft_w / ADAS_WIDTH;
		y2 = 240 * tft_h / ADAS_HEIGHT;
		gp_paint_line(buf, tft_w, tft_h, type, x1, y1, x2, y2, BLUE_COLOR);

		ap_sound_bsd_play(disp_bsd);
	}
	gp_paint_bsd(buf, tft_w, tft_h, &g_draw_left, &g_draw_right);
}
#endif

#if C_WATER_MARK == CUSTOM_ON
void ap_display_water_mark_icon_draw(INT16U *buf)
{
    INT16U tft_w = drv_l2_display_tft_width_get();
    INT16U tft_h = drv_l2_display_tft_height_get();

    if(car_plate_icon && car_plate_icon->data)
    {
        ap_display_icon_draw(buf, tft_w, tft_h, car_plate_icon, 0);
    }
}
#endif

void ap_display_icon_draw(INT16U *buf, INT16U buf_w, INT16U buf_h, DISPLAY_ICONSHOW *icon_ptr, INT8U color_flag)
{
    if(drv_l2_display_tft_fmt_get() == DISP_FMT_GP420)
    {
        ap_display_icon_draw_gp420(buf, buf_w, buf_h, icon_ptr, color_flag);
    }
    else if(drv_l2_display_tft_fmt_get() == DISP_FMT_RGB565)
    {
        ap_display_icon_draw_rgb565(buf, buf_w, buf_h, icon_ptr, color_flag);
    }
    else //YUYV
    {
        ap_display_icon_draw_yuyv(buf, buf_w, buf_h, icon_ptr, color_flag);
    }
}

void ap_display_icon_draw_yuyv(INT16U *buf, INT16U buf_w, INT16U buf_h, DISPLAY_ICONSHOW *icon_ptr, INT8U color_flag)
{
    INT32U  x, y, offset_pixel, offset_data;
    INT16S  pos_x, pos_y, icon_w, icon_h;
    INT16U  data;

    pos_x = icon_ptr->pos_x;
    if(pos_x < 0)   pos_x += buf_w;

    pos_y = icon_ptr->pos_y;
    if(pos_y < 0)   pos_y += buf_h;

    icon_w = icon_ptr->icon_w;
    icon_h = icon_ptr->icon_h;

    if((pos_x + icon_w) > buf_w)   return;
    if((pos_y + icon_h) > buf_h)   return;

    for(y = 0; y < icon_h; y++)
    {
        for(x = 0; x < icon_w; x++)
        {
            offset_data = y * icon_w + x;
            if(*(icon_ptr->data + offset_data) == icon_ptr->transparent)
            {
                continue;
            }

            offset_pixel = (pos_y + y) * buf_w + pos_x + x;
            if(color_flag == DISPLAY_ICON_RED_FLAG)
            {
                if((pos_x + x) & 0x0001)
                {
                    *(buf + offset_pixel) = (RED_Y << 8) | RED_U; //red
                }
                else
                {
                    *(buf + offset_pixel) =  (RED_Y << 8) | RED_V; //red
                }
            }
            else
            {
                data = *(icon_ptr->data + offset_data);

                if(color_flag && ((data & 0xff00) == 0xff00)) //replace white with blue color
                {
                    if(color_flag == DISPLAY_ICON_BLUE_FLAG)
                    {
                        if((pos_x + x) & 0x0001)
                        {
                            *(buf + offset_pixel) = (BLUE_Y << 8) | BLUE_U;
                        }
                        else
                        {
                            *(buf + offset_pixel) = (BLUE_Y << 8) | BLUE_V;
                        }
                    }
                    else //DISPLAY_ICON_BLUE1_FLAG
                    {
                        if((pos_x + x) & 0x0001)
                        {
                            *(buf + offset_pixel) = (BLUE1_Y << 8) | BLUE1_U;
                        }
                        else
                        {
                            *(buf + offset_pixel) = (BLUE1_Y << 8) | BLUE1_V;
                        }
                    }
                }
                else
                {
                    *(buf + offset_pixel) = data;
                }
            }
        }
    }
}

static void ap_display_icon_draw_rgb565(INT16U *buf, INT16U buf_w, INT16U buf_h, DISPLAY_ICONSHOW *icon_ptr, INT8U color_flag)
{
    INT32U  x, y, offset_pixel, offset_data;
    INT16S  pos_x, pos_y, icon_w, icon_h;
    INT16U  data;

    pos_x = icon_ptr->pos_x;
    if(pos_x < 0)   pos_x += buf_w;

    pos_y = icon_ptr->pos_y;
    if(pos_y < 0)   pos_y += buf_h;

    icon_w = icon_ptr->icon_w;
    icon_h = icon_ptr->icon_h;

    if((pos_x + icon_w) > buf_w)   return;
    if((pos_y + icon_h) > buf_h)   return;

    for(y = 0; y < icon_h; y++)
    {
        for(x = 0; x < icon_w; x++)
        {
            offset_data = y * icon_w + x;
            if(*(icon_ptr->data + offset_data) == icon_ptr->transparent)
            {
                continue;
            }

            offset_pixel = (pos_y + y) * buf_w + pos_x + x;
            if(color_flag == DISPLAY_ICON_RED_FLAG)
            {
                *(buf + offset_pixel) = 0xf800; //red
            }
            else
            {
                data = *(icon_ptr->data + offset_data);

                if(color_flag && (data == WHITE_COLOR))
                {
                    if(color_flag == DISPLAY_ICON_BLUE_FLAG)
                    {
                        *(buf + offset_pixel) = BLUE_COLOR;
                    }
                    else //DISPLAY_ICON_BLUE1_FLAG
                    {
                        *(buf + offset_pixel) = BLUE1_COLOR;
                    }
                }
                else
                {
                    *(buf + offset_pixel) = data;
                }
            }
        }
    }
}


void ap_display_icon_draw_gp420(INT16U *buf, INT16U buf_w, INT16U buf_h, DISPLAY_ICONSHOW *icon_ptr, INT8U color_flag)
{
    INT16S x, y, pos_x, pos_y, icon_w, icon_h;
    INT8U *icon_stream_data;
    INT8U *ptr8_buf = (INT8U *)buf;

    pos_x = icon_ptr->pos_x;
    pos_y = icon_ptr->pos_y;

    if(pos_x < 0)   pos_x += buf_w;
    if(pos_y < 0)   pos_y += buf_h;

    pos_x &= ~0x0001;
    pos_y &= ~0x0001;

    icon_w = icon_ptr->icon_w;
    icon_h = icon_ptr->icon_h;

    if((pos_x + icon_w) > buf_w)   return;
    if((pos_y + icon_h) > buf_h)   return;

    for(y = 0; y < icon_h; y++)
    {
        for(x = 0; x < icon_w; x++)
        {
            icon_stream_data = (INT8U *)icon_ptr->data;
            if((y & 0x0001) == 0)   //even
            {
                icon_stream_data += (y * 3 * icon_w >> 1) + x;
            }
            else //odd
            {
                icon_stream_data += ((y - 1) * 3 * icon_w >> 1) + x + 2 * icon_w;
            }

            if(*icon_stream_data == icon_ptr->transparent)
            {
                continue;
            }

            if((y & 0x0001) == 0)   //even
            {
                if(color_flag == DISPLAY_ICON_RED_FLAG)
                {
                    *(ptr8_buf + (((pos_y + y) * 3 * buf_w >> 1) + pos_x + x)) = RED_Y; //Y
                }
                else
                {
                    if(color_flag && (*icon_stream_data == 0xff))
                    {
                        if(color_flag == DISPLAY_ICON_BLUE_FLAG)
                        {
                            *(ptr8_buf + (((pos_y + y) * 3 * buf_w >> 1) + pos_x + x)) = BLUE_Y; //Y
                        }
                        else //DISPLAY_ICON_BLUE1_FLAG
                        {
                            *(ptr8_buf + (((pos_y + y) * 3 * buf_w >> 1) + pos_x + x)) = BLUE1_Y; //Y
                        }
                    }
                    else
                    {
                        *(ptr8_buf + (((pos_y + y) * 3 * buf_w >> 1) + pos_x + x)) = *icon_stream_data; //Y
                    }
                }

                if((x & 0x0001) == 0)
                {
                    if(color_flag == DISPLAY_ICON_RED_FLAG)
                    {
                        *(buf + ((((pos_y + y) * 3 * buf_w >> 1) + pos_x + x + buf_w) >> 1)) = RED_U + (RED_V << 8);   //UV
                    }
                    else
                    {
                        if(color_flag && (*icon_stream_data == 0xff))
                        {
                            if(color_flag == DISPLAY_ICON_BLUE_FLAG)
                            {
                                *(buf + ((((pos_y + y) * 3 * buf_w >> 1) + pos_x + x + buf_w) >> 1)) = BLUE_U | (BLUE_V << 8);   //UV
                            }
                            else //DISPLAY_ICON_BLUE1_FLAG
                            {
                                *(buf + ((((pos_y + y) * 3 * buf_w >> 1) + pos_x + x + buf_w) >> 1)) = BLUE1_U | (BLUE1_V << 8);   //UV
                            }
                        }
                        else
                        {
                            *(buf + ((((pos_y + y) * 3 * buf_w >> 1) + pos_x + x + buf_w) >> 1)) = *((INT16U *)icon_stream_data + (icon_w >> 1));   //UV
                        }
                    }
                }
            }
            else //odd
            {
                if(color_flag == DISPLAY_ICON_RED_FLAG)
                {
                    *(ptr8_buf + (((pos_y + y - 1) * 3 * buf_w >> 1) + pos_x + x + buf_w * 2)) = RED_Y; //Y
                }
                else
                {
                    if(color_flag && (*icon_stream_data == 0xff))
                    {
                        if(color_flag == DISPLAY_ICON_BLUE_FLAG)
                        {
                            *(ptr8_buf + (((pos_y + y - 1) * 3 * buf_w >> 1) + pos_x + x + buf_w * 2)) = BLUE_Y; //Y
                        }
                        else //DISPLAY_ICON_BLUE1_FLAG
                        {
                            *(ptr8_buf + (((pos_y + y - 1) * 3 * buf_w >> 1) + pos_x + x + buf_w * 2)) = BLUE1_Y; //Y
                        }
                    }
                    else
                    {
                        *(ptr8_buf + (((pos_y + y - 1) * 3 * buf_w >> 1) + pos_x + x + buf_w * 2)) = *icon_stream_data; //Y
                    }
                }
            }
        }
    }
}


/***********************************************************************************************************/
/*                           D I S P L A Y    Q U E U E    F U N C T I O N S                               */
/***********************************************************************************************************/

DISP_P ap_display_queue_get(DISP_QUEUE queue)
{
	DISP_P data = NULL;

	osENTER_CRITICAL();

	if(queue[0] != NULL)
	{
		INT32U i;

		data = queue[0];
		for(i = 0; i < (display_frame_pool_size - 1); ++i)
		{
			queue[i] = queue[i + 1];
		}
		queue[i] = NULL;
	}

	osEXIT_CRITICAL();
	return data;
}

INT32S ap_display_queue_put(DISP_QUEUE queue, DISP_P data)
{
	INT32U i;

	if(data == NULL)
	{
		ERR_PRINT("DisPFAIL-3");
		return STATUS_FAIL;
	}

	osENTER_CRITICAL();
	for(i = 0; i < (display_frame_pool_size - 1); ++i)
	{
		if(data == queue[i])
		{
			osEXIT_CRITICAL();
			ERR_PRINT("DisPFAIL-1");
			return STATUS_FAIL;
		}
	}

	for(i = 0; i < display_frame_pool_size; ++i)
	{
		if(!queue[i])
		{
			queue[i] = data;
			osEXIT_CRITICAL();
			return STATUS_OK;
		}
	}

	osEXIT_CRITICAL();
	ERR_PRINT("DisPFAIL-2");
	return STATUS_FAIL;
}

DISP_P ap_display_queue_query(DISP_QUEUE queue)
{
	DISP_P data = NULL;

	osENTER_CRITICAL();
	data = queue[0];
	osEXIT_CRITICAL();

	return data;
}

#if (defined C_LVGL_SUPPORT) && (C_LVGL_SUPPORT)
void ap_state_handling_display_path_set(INT32U path)
{
    if(path >= DISPLAY_PATH_MAX)
        display_path = DISPLAY_PATH_NORMAL;
    else
        display_path = path;
}

INT32U ap_state_handling_display_path_get(void)
{
    return display_path;
}

void ap_display_buff_lvgl_draw(INT32U* lvglbuf, INT32U msg_id)
{
    /* Update LVGL display buffer for display */
    lvgl_display_buf = *lvglbuf;
    //DBG_PRINT("Display LVGL draw buf 0x%x\r\n", lvgl_display_buf);
}
#endif

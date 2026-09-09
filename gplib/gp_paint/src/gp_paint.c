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
#include "gp_paint.h"
#include "application.h"
#include "math.h"

void gp_paint_dot(INT16U *buf, INT16U buf_w, INT16U buf_h, DISP_FMT fmt, INT16U pos_x, INT16U pos_y, INT16U color)
{
    INT8U   Y, U, V;
    INT8U   *ptr8 = (INT8U *)buf;
    if(pos_x >= buf_w || pos_y >= buf_h){
        return;
    }
    if(fmt != DISP_FMT_RGB565)
    {
        rgb_to_yuv(color, &Y, &U, &V);

        if(fmt == DISP_FMT_GP420)
        {
            if(pos_y & 0x01)  //Odd
            {
                *(ptr8 + ((pos_y - 1) * buf_w * 3 >> 1)  + (buf_w << 1) + pos_x) = Y;
            }
            else    //Even
            {
                *(ptr8 + (pos_y * buf_w * 3 >> 1) + pos_x) = Y;

                if(pos_x & 0x01) //Odd
                {
                    *(ptr8 + (pos_y * buf_w * 3 >> 1) + buf_w + pos_x - 1) = U;
                    *(ptr8 + (pos_y * buf_w * 3 >> 1) + buf_w + pos_x) = V;
                }
                else //Even
                {
                    *(ptr8 + (pos_y * buf_w * 3 >> 1) + buf_w + pos_x) = U;
                    *(ptr8 + (pos_y * buf_w * 3 >> 1) + buf_w + pos_x + 1) = V;
                }
            }
        }
        else // if(drv_l2_display_tft_fmt_get() == DISP_FMT_YUYV)
        {
            if(pos_x & 0x01)
            {
                buf[pos_y * buf_w + pos_x] = (Y << 8) | U;
            }
            else
            {
                buf[pos_y * buf_w + pos_x] = (Y << 8) | V;
            }
        }
    }
    else
    {
        buf[pos_y * buf_w + pos_x] = color;
    }
}

INT8U gp_paint_line(INT16U *buf, INT16U buf_w, INT16U buf_h, DISP_FMT fmt, INT32S start_x, INT32S start_y, INT32S end_x, INT32S end_y, INT16U color){
    INT8U count_x_flag = 0;
    INT32S a_numerator, a_denominator, b;

    if(end_x == start_x && end_y == start_y){
        gp_paint_dot(buf, buf_w, buf_h, fmt, start_x, start_y, color);
        return 1;
    }

    if(ABS(end_x - start_x) > ABS(end_y - start_y)){
        count_x_flag = 1;
        a_numerator = end_y - start_y;
        a_denominator = end_x - start_x;
        b = (start_y * end_x - end_y * start_x) / a_denominator;
        if(a_numerator){
            if((start_x < 0 || start_x >= buf_w) && (start_y < 0 || start_y >= buf_h)){
                start_x = (start_x < 0)?0:buf_w - 1;
                start_y = a_numerator * start_x / a_denominator + b;
                if(start_y < 0){
                    start_y = 0;
                    start_x = (start_y - b) * a_denominator / a_numerator;
                }
                else if(start_y >= buf_h){
                    start_y = buf_h - 1;
                    start_x = (start_y - b) * a_denominator / a_numerator;
                }
            }
            else if(start_x < 0 || start_x >= buf_w){
                start_x = (start_x < 0)?0:buf_w - 1;
                start_y = a_numerator * start_x / a_denominator + b;
            }
            else if(start_y < 0 || start_y >= buf_h){
                start_y = (start_y < 0)?0:buf_h - 1;
                start_x = (start_y - b) * a_denominator / a_numerator;
            }

            if(start_x < 0 || start_x >= buf_w || start_y < 0 || start_y >= buf_h){
                return 0;
            }

            if((end_x < 0 || end_x >= buf_w) && (end_y < 0 || end_y >= buf_h)){
                end_x = (end_x < 0)?0:buf_w - 1;
                end_y = a_numerator * end_x / a_denominator + b;
                if(end_y < 0){
                    end_y = 0;
                    end_x = (end_y - b) * a_denominator / a_numerator;
                }
                else if(end_y >= buf_h){
                    end_y = buf_h - 1;
                    end_x = (end_y - b) * a_denominator / a_numerator;
                }
            }
            else if(end_x < 0 || end_x >= buf_w){
                end_x = (end_x < 0)?0:buf_w - 1;
                end_y = a_numerator * end_x / a_denominator + b;
            }
            else if(end_y < 0 || end_y >= buf_h){
                end_y = (end_y < 0)?0:buf_h - 1;
                end_x = (end_y - b) * a_denominator / a_numerator;
            }

            if(end_x < 0 || end_x >= buf_w || end_y < 0 || end_y >= buf_h){
                return 0;
            }
        }
        else{
            if(start_y < 0 || start_y >= buf_h){
                return 0;
            }
            else{
                if(start_x < 0){
                    start_x = 0;
                }
                else if(start_x >= buf_w){
                    start_x = buf_w - 1;
                }
                if(end_x < 0){
                    end_x = 0;
                }
                else if(end_x >= buf_w){
                    end_x = buf_w - 1;
                }
            }
        }
    }
    else{
        count_x_flag = 0;
        a_numerator = end_x - start_x;
        a_denominator = end_y - start_y;
        b = (start_x * end_y - end_x * start_y) / a_denominator;
        if(a_numerator){
            if((start_x < 0 || start_x >= buf_w) && (start_y < 0 || start_y >= buf_h)){
                start_x = (start_x < 0)?0:buf_w - 1;
                start_y = (start_x - b) * a_denominator / a_numerator;
                if(start_y < 0){
                    start_y = 0;
                    start_x = a_numerator * start_y / a_denominator + b;
                }
                else if(start_y >= buf_h){
                    start_y = buf_h - 1;
                    start_x = a_numerator * start_y / a_denominator + b;
                }
            }
            else if(start_x < 0 || start_x >= buf_w){
                start_x = (start_x < 0)?0:buf_w - 1;
                start_y = (start_x - b) * a_denominator / a_numerator;
            }
            else if(start_y < 0 || start_y >= buf_h){
                start_y = (start_y < 0)?0:buf_h - 1;
                start_x = a_numerator * start_y / a_denominator + b;
            }

            if(start_x < 0 || start_x >= buf_w || start_y < 0 || start_y >= buf_h){
                return 0;
            }

            if((end_x < 0 || end_x >= buf_w) && (end_y < 0 || end_y >= buf_h)){
                end_x = (end_x < 0)?0:buf_w - 1;
                end_y = (end_x - b) * a_denominator / a_numerator;
                if(end_y < 0){
                    end_y = 0;
                    end_x = a_numerator * end_y / a_denominator + b;
                }
                else if(end_y >= buf_h){
                    end_y = buf_h - 1;
                    end_x = a_numerator * end_y / a_denominator + b;
                }
            }
            else if(end_x < 0 || end_x >= buf_w){
                end_x = (end_x < 0)?0:buf_w - 1;
                end_y = (end_x - b) * a_denominator / a_numerator;
            }
            else if(end_y < 0 || end_y >= buf_h){
                end_y = (end_y < 0)?0:buf_h - 1;
                end_x = a_numerator * end_y / a_denominator + b;
            }

            if(end_x < 0 || end_x >= buf_w || end_y < 0 || end_y >= buf_h){
                return 0;
            }
        }
        else{
            if(start_x < 0 || start_x >= buf_w){
                return 0;
            }
            else{
                if(start_y < 0){
                    start_y = 0;
                }
                else if(start_y >= buf_h){
                    start_y = buf_h - 1;
                }
                if(end_y < 0){
                    end_y = 0;
                }
                else if(end_y >= buf_h){
                    end_y = buf_h - 1;
                }
            }
        }
    }

    do{
        gp_paint_dot(buf, buf_w, buf_h, fmt, start_x, start_y, color);
        if(count_x_flag){
            INT32S y_buf = start_y;
            if(start_x < end_x){
                start_x++;
            }
            else if(start_x > end_x){
                start_x--;
            }
            else{
                return 1;
            }
            if(a_denominator){
                start_y = a_numerator * start_x / a_denominator + b;
                if(y_buf < end_y){
                    if(start_y < y_buf){
                        start_y = y_buf;
                    }
                }
                else if(y_buf > end_y){
                    if(start_y > y_buf){
                        start_y = y_buf;
                    }
                }
            }
        }
        else{
            INT32S x_buf = start_x;
            if(start_y < end_y){
                start_y++;
            }
            else if(start_y > end_y){
                start_y--;
            }
            else{
                return 1;
            }
            if(a_denominator){
                start_x = a_numerator * start_y / a_denominator + b;
                if(x_buf < end_x){
                    if(start_x < x_buf){
                        start_x = x_buf;
                    }
                }
                else if(x_buf > end_x){
                    if(start_x > x_buf){
                        start_x = x_buf;
                    }
                }
            }
        }
    }while(start_x != end_x || start_y != end_y);
    gp_paint_dot(buf, buf_w, buf_h, fmt, start_x, start_y, color);
    return 1;
}

void gp_paint_circle(INT16U *buf, INT16U buf_w, INT16U buf_h, DISP_FMT fmt, INT32S pos_x, INT32S pos_y, INT16U radius, INT8U solid, INT16U color){
    INT32S x = pos_x, y = pos_y + radius;
    INT32S change_x = 0;
    if(!radius){
        return;
    }
    if(radius == 1){
        gp_paint_dot(buf, buf_w, buf_h, fmt, pos_x, pos_y, color);
        return;
    }
    do{
        INT32S x_to_pos = ABS(x - pos_x);
        INT32S y_to_pos = ABS(y - pos_y);
        if(x_to_pos <= y_to_pos){
            y = sqrt(radius * radius - (x_to_pos) * (x_to_pos)) + pos_y;
            if(!solid){
                gp_paint_dot(buf, buf_w, buf_h, fmt, x, y, color);
                gp_paint_dot(buf, buf_w, buf_h, fmt, x, 2 * pos_y - y, color);
                if(x_to_pos){
                    gp_paint_dot(buf, buf_w, buf_h, fmt, 2 * pos_x - x, y, color);
                    gp_paint_dot(buf, buf_w, buf_h, fmt, 2 * pos_x - x, 2 * pos_y - y, color);
                }
            }
            else{
                gp_paint_line(buf, buf_w, buf_h, fmt, x, y, x, 2 * pos_y - y, color);
                if(x_to_pos){
                    gp_paint_line(buf, buf_w, buf_h, fmt, 2 * pos_x - x, y, 2 * pos_x - x, 2 * pos_y - y, color);
                }
            }
            change_x = x;
            x++;
        }
        else{
            x = sqrt(radius * radius - y_to_pos * y_to_pos) + pos_x;
            if(!solid){
                if(y_to_pos){
                    gp_paint_dot(buf, buf_w, buf_h, fmt, x, y, color);
                    gp_paint_dot(buf, buf_w, buf_h, fmt, x, 2 * pos_y - y, color);
                }
                gp_paint_dot(buf, buf_w, buf_h, fmt, 2 * pos_x - x, y, color);
                gp_paint_dot(buf, buf_w, buf_h, fmt, 2 * pos_x - x, 2 * pos_y - y, color);
            }
            else{
                gp_paint_line(buf, buf_w, buf_h, fmt, x, y, change_x, y, color);
                gp_paint_line(buf, buf_w, buf_h, fmt, 2 * pos_x - change_x, y, 2 * pos_x - x, y, color);
                if(y_to_pos){
                    gp_paint_line(buf, buf_w, buf_h, fmt, x, 2 * pos_y - y, change_x, 2 * pos_y - y, color);
                    gp_paint_line(buf, buf_w, buf_h, fmt, 2 * pos_x - change_x, 2 * pos_y - y, 2 * pos_x - x, 2 * pos_y - y, color);
                }
            }
        }
        if(y == pos_y){
            break;
        }
        y--;
    }while(1);
}

void gp_paint_rect(INT16U *buf, INT16U buf_w, INT16U buf_h, DISP_FMT fmt, INT32S start_x, INT32S start_y, INT32S width, INT32S height, INT16U thickness, INT8U solid, INT16U color){
    if(!width && !height){
        gp_paint_dot(buf, buf_w, buf_h, fmt, start_x, start_y, color);
    }
    else if(!width){
        INT16U axis;
        for(axis = 0; axis < thickness; axis++){
            INT16S bias = (thickness >> 1) - axis;
            gp_paint_line(buf, buf_w, buf_h, fmt, start_x - bias, start_y, start_x - bias, start_y + height, color);
        }
    }
    else if(!height){
        INT16U axis;
        for(axis = 0; axis < thickness; axis++){
            INT16S bias = (thickness >> 1) - axis;
            gp_paint_line(buf, buf_w, buf_h, fmt, start_x, start_y - bias, start_x + width, start_y - bias, color);
        }
    }
    else{
        INT16U axis;
        INT16S lu_x, ru_x, lb_x, rb_x, lu_y, ru_y, lb_y, rb_y;
        if(width > 0){
            lu_x = lb_x = start_x - (thickness >> 1);
            ru_x = rb_x = start_x + width + (thickness >> 1);
        }
        else{
            lu_x = lb_x = start_x + (thickness >> 1);
            ru_x = rb_x = start_x + width - (thickness >> 1);
        }

        if(height > 0){
            lu_y = ru_y = start_y + height + (thickness >> 1);
            lb_y = rb_y = start_y - (thickness >> 1);
        }
        else{
            lu_y = ru_y = start_y - (thickness >> 1);
            lb_y = rb_y = start_y + height + (thickness >> 1);
        }
        for(axis = 0; axis < thickness; axis++){
            gp_paint_line(buf, buf_w, buf_h, fmt, lu_x, lu_y, ru_x, ru_y, color);
            gp_paint_line(buf, buf_w, buf_h, fmt, lb_x, lb_y, rb_x, rb_y, color);
            gp_paint_line(buf, buf_w, buf_h, fmt, lb_x, lb_y, lu_x, lu_y, color);
            gp_paint_line(buf, buf_w, buf_h, fmt, rb_x, rb_y, ru_x, ru_y, color);
            if(width > 0){
                ++lu_x;
                --ru_x;
                ++lb_x;
                --rb_x;
            }
            else{
                --lu_x;
                ++ru_x;
                --lb_x;
                ++rb_x;
            }

            if(height > 0){
                --lu_y;
                --ru_y;
                ++lb_y;
                ++rb_y;
            }
            else{
                ++lu_y;
                ++ru_y;
                --lb_y;
                --rb_y;
            }
        }
        if(solid){
            INT16U line = ABS(lb_y - lu_y);
            do{
                gp_paint_line(buf, buf_w, buf_h, fmt, lu_x, lu_y, ru_x, ru_y, color);
                if(lu_y > lb_y){
                    lu_y--;
                    ru_y--;
                }
                else if(lu_y < lb_y){
                    lu_y++;
                    ru_y++;
                }
            }while(line--);
        }
    }
}

// only for GP420
gp_paint_bsd(unsigned short *buf, unsigned int tft_w, unsigned int tft_h, int *p_bsd_alert_left, int *p_bsd_alert_right)
{
	INT32U width, height;
	INT32U *gp420_ptr;
	INT32U *data;
	INT32U bsd_range;

	bsd_range = tft_w/320;
	if (bsd_range==0) bsd_range = 1;
	while((*p_bsd_alert_left != 0) || (*p_bsd_alert_right != 0))
	{
		data = (INT32U *)buf+tft_w;		// Skip Y-line and point to start address of UV line	
		for (height=0; height<tft_h; height+=2) 
		{
			// Draw left side bar
			if(*p_bsd_alert_left)
			{
				gp420_ptr = data;
				for (width=0; width<bsd_range; width++) 
				{		// 32-pixels per loop
					*gp420_ptr++ = (((*gp420_ptr>>1) & 0x7F007F00) + 0x7F007F00) | (((*gp420_ptr>>1) & 0x007F007F) + 0x002A002A);
					*gp420_ptr++ = (((*gp420_ptr>>1) & 0x7F007F00) + 0x7F007F00) | (((*gp420_ptr>>1) & 0x007F007F) + 0x002A002A);
					*gp420_ptr++ = (((*gp420_ptr>>1) & 0x7F007F00) + 0x7F007F00) | (((*gp420_ptr>>1) & 0x007F007F) + 0x002A002A);
					*gp420_ptr++ = (((*gp420_ptr>>1) & 0x7F007F00) + 0x7F007F00) | (((*gp420_ptr>>1) & 0x007F007F) + 0x002A002A);
					*gp420_ptr++ = (((*gp420_ptr>>1) & 0x7F007F00) + 0x7F007F00) | (((*gp420_ptr>>1) & 0x007F007F) + 0x002A002A);
					*gp420_ptr++ = (((*gp420_ptr>>1) & 0x7F007F00) + 0x7F007F00) | (((*gp420_ptr>>1) & 0x007F007F) + 0x002A002A);
					*gp420_ptr++ = (((*gp420_ptr>>1) & 0x7F007F00) + 0x7F007F00) | (((*gp420_ptr>>1) & 0x007F007F) + 0x002A002A);
					*gp420_ptr++ = (((*gp420_ptr>>1) & 0x7F007F00) + 0x7F007F00) | (((*gp420_ptr>>1) & 0x007F007F) + 0x002A002A);
				}
			}
	
			// Draw right side bar
			if(*p_bsd_alert_right)
			{
				gp420_ptr = data + (tft_w-(32/*pixel*/*bsd_range/4/*wrod*/));
				for (width=0; width<bsd_range; width++) 
				{		// 32-pixels per loop
					*gp420_ptr++ = (((*gp420_ptr>>1) & 0x7F007F00) + 0x7F007F00) | (((*gp420_ptr>>1) & 0x007F007F) + 0x002A002A);
					*gp420_ptr++ = (((*gp420_ptr>>1) & 0x7F007F00) + 0x7F007F00) | (((*gp420_ptr>>1) & 0x007F007F) + 0x002A002A);
					*gp420_ptr++ = (((*gp420_ptr>>1) & 0x7F007F00) + 0x7F007F00) | (((*gp420_ptr>>1) & 0x007F007F) + 0x002A002A);
					*gp420_ptr++ = (((*gp420_ptr>>1) & 0x7F007F00) + 0x7F007F00) | (((*gp420_ptr>>1) & 0x007F007F) + 0x002A002A);
					*gp420_ptr++ = (((*gp420_ptr>>1) & 0x7F007F00) + 0x7F007F00) | (((*gp420_ptr>>1) & 0x007F007F) + 0x002A002A);
					*gp420_ptr++ = (((*gp420_ptr>>1) & 0x7F007F00) + 0x7F007F00) | (((*gp420_ptr>>1) & 0x007F007F) + 0x002A002A);
					*gp420_ptr++ = (((*gp420_ptr>>1) & 0x7F007F00) + 0x7F007F00) | (((*gp420_ptr>>1) & 0x007F007F) + 0x002A002A);
					*gp420_ptr++ = (((*gp420_ptr>>1) & 0x7F007F00) + 0x7F007F00) | (((*gp420_ptr>>1) & 0x007F007F) + 0x002A002A);
				}
			}
	
			data += 3*(tft_w/4); 	// Jump to next UV line
		}
		if(*p_bsd_alert_left != 0)
			*p_bsd_alert_left -= 1;
		if(*p_bsd_alert_right != 0)
			*p_bsd_alert_right -= 1;
	}
}


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
#include "drv_l1.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define C_BOXFILTER_QUEUE_SIZE		    1
#define GPIO_DEBUG_EN                   0
#define C_BOXFILTER_END                 1

#define BOXFILTER_CLOCK_ENABLE            1
#define BOXFILTER_CLOCK_DISABLE           0

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

static INT32U box_int_flag = 0;
osMessageQId boxfilter_q = NULL;
//QueueHandle_t boxfilter_q =NULL;
osSemaphoreId boxfilter_hw_sem = NULL;

/**************************************************************************
 *                           F U N C T I O N S                            *
 **************************************************************************/


/*****************************************************
*	box filter lock:
*	@brief box filter lock function
*   @param  NONE
*   @return NONE
******************************************************/
void drv_l1_boxfilter_lock(void)
{

    if(boxfilter_hw_sem)
        osSemaphoreWait(boxfilter_hw_sem, osWaitForever);

}
/*****************************************************
*	box filter unlock:
*	@brief box filter unlock function
*   @param  NONE
*   @return NONE
*****************************************************/
void drv_l1_boxfilter_unlock(void)
{

	if(boxfilter_hw_sem)
        osSemaphoreRelease(boxfilter_hw_sem);

}


/*****************************************************
*	get_BOXFILTER_Reg_Base:
*	@brief  get box filter base address
*   @param  NONE
*   @return NONE
*****************************************************/
static __inline BOXFILTER_SFR get_BOXFILTER_Reg_Base(void)
{
    return (BOXFILTER_SFR)BOXFILTER_BASE;
}

/*****************************************************
*	devier layer 1 BOXFILTER ISR:
*	@brief box filter ISR service
*   @param  NONE
*   @return NONE
*****************************************************/
static void boxfilter_isr(void)
{
    INT32U ret;
    ret = R_BOXFILTER_IRQ_STATUS;

    if(boxfilter_q) {

//        INT32U event;
//        DBG_PRINT("t");
//        event = ret;
//        osMessagePut(boxfilter_q, (uint32_t)&event, osWaitForever);

        //xQueueSendFromISR(boxfilter_q, &ret, pdFAIL);

    }
    box_int_flag = ret;
    R_BOXFILTER_IRQ_STATUS = ret;

}

/*****************************************************
*	BOXFILTER_IRQHandler:
*	@brief box filter IRQ Handler
*   @param  NONE
*   @return NONE
*****************************************************/
void BOXFILTER_IRQHandler(void)
{
    boxfilter_isr();
    //DBG_PRINT("!");
}


/*****************************************************
*	BOXFILTER Clock setting:
*	@brief box filter set clock
*   @param  1: enable clock; 0: disable clock
*   @return NONE
*****************************************************/
static INT16S drv_l1_boxfilter_clock_set(INT16U enable)
{
    return drv_l1_clock_set_system_clk_en(CLK_EN6_BOX_FILTER, enable);
    //return 0;
}

/*****************************************************
*	Deiver Level 1 BOXFILTER INIT:
*	@brief box filter initial
*   @param  fiq_en = 0: IRQ; 1:FIQ
*   @return 0: success; -1: vic_num fail
*****************************************************/

INT32S drv_l1_boxfilter_init(INT32U fiq_en)
{

    box_int_flag = 0;
    drv_l1_boxfilter_clock_set(BOXFILTER_CLOCK_ENABLE);

    R_BOXFILTER_CTRL = 0;
    R_BOXFILTER_IRQ_STATUS = C_BOXFILTER_INT_END;	// Clear boxfilter pending interrupts
    R_BOXFILTER_IRQ_EN = C_BOXFILTER_INT_EN;


    if(boxfilter_q == NULL) {
        osMessageQDef_t boxf_q = { C_BOXFILTER_QUEUE_SIZE, sizeof(INT32U), 0 };


        boxfilter_q = osMessageCreate(&boxf_q, NULL);
        //boxfilter_q = xQueueCreate(1, sizeof(INT32U));
    }

    if(boxfilter_hw_sem == NULL) {
        osSemaphoreDef_t boxfilter_sem = { 0 };

        boxfilter_hw_sem = osSemaphoreCreate(&boxfilter_sem, 1);
    }

    vic_irq_register(VIC_BOX, BOXFILTER_IRQHandler);
    vic_irq_enable(VIC_BOX);
    if(fiq_en)
    {
        vic_fiq_enable(VIC_BOX);
    }
    else {
		vic_fiq_disable(VIC_BOX);
    }
    return 0;
}

/*****************************************************
*	Deiver Level 1 BOXFILTER UNINIT:
*	@brief box filter uninitial
*   @param  fiq_en = 0: IRQ; 1:FIQ
*   @return 0: success; -1: vic_num fail
*****************************************************/
INT32S drv_l1_boxfilter_uninit(INT32U fiq_en)
{

    R_BOXFILTER_CTRL = 0;
    R_BOXFILTER_IRQ_STATUS = C_BOXFILTER_INT_END;	// Clear Boxfilter pending interrupts
    R_BOXFILTER_IRQ_EN = 0;

    vic_irq_disable(VIC_BOX);
    vic_fiq_disable(VIC_BOX);

    drv_l1_boxfilter_clock_set(BOXFILTER_CLOCK_DISABLE);
    box_int_flag = 0;
    return 0;
}

/*****************************************************
*	Deiver Level 1 BOXFILTER Source Image Info:
*	@brief Set box filter input source information
*   @param  color: input color format;w: input x width;h: input y width;in_buf: input buffer address
*   @return 0:success; -1: fail
*****************************************************/
INT32S drv_l1_boxfilter_src_img_info(INT8S color, INT16S w, INT16S h, INT8U* in_buf, BOXFILTER_OUT_MODE out_mode)
{
    INT32U temp = R_BOXFILTER_CTRL;
    INT8S _color = color;

    if(!in_buf || !w || !h)
        return -1;
    if((w>640) && (!(out_mode == C_BOXFTR_BYPASS)))
        return -1;
//    drv_l1_boxfilter_lock();
    switch(color)
    {
        case IMG_FMT_GRAY:
            _color = 0x3;
        break;
        case IMG_FMT_YUYV:
            _color = 0x0;
        break;
        case IMG_FMT_UYVY:
            _color = 0x1;
        break;
        case IMG_FMT_GP420:
            _color = 0x2;
        break;
            _color = 0x4;
        break;
    }
    if(_color > C_MASK_COLOR_MODE)
    {
         return -1;
    }
    else
    {
         temp &= ~(C_MASK_COLOR_MODE<<2);
         temp |= (_color<<2);
         R_BOXFILTER_CTRL = temp;
    }

    R_BOXFILTER_IMG_WIDTH = ((w-1) & C_MASK_IMG_RANGE);
    R_BOXFILTER_IMG_HEIGHT = ((h-1) & C_MASK_IMG_RANGE);
    R_BOXFILTER_BUF_I_ADDR = (INT32U) in_buf;

//    drv_l1_boxfilter_unlock();

    return 0;
}
/*****************************************************
*	Deiver Level 1 BOXFILTER Output A buffer Image Info:
*	@brief Set output buffer A address
*   @param  out_buf: DRAM address
*   @return 0:success; -1: fail
*****************************************************/
INT32S drv_l1_boxfilter_outa_img_addr(INT8U* out_buf)
{
    if(!out_buf)
        return -1;
//    drv_l1_boxfilter_lock();
    R_BOXFILTER_BUFA_O_ADDR = (INT32U)out_buf;
//    drv_l1_boxfilter_unlock();

    return 0;
}

/*****************************************************
*	Deiver Level 1 BOXFILTER Output B buffer Image Info:
*   and Set continue mode
*	@brief Set continue mode and B buffer address
*   @param  outb_buf: DRAM address; enable = 1==>enable continue mode
*   @return 0:success; -1: fail
*****************************************************/
INT32S drv_l1_boxftr_continue_set(INT32U outb_buf, INT8U enable)
{
    INT32U temp = R_BOXFILTER_CTRL;
    if(!outb_buf)
        return -1;
//    drv_l1_boxfilter_lock();
    R_BOXFILTER_BUFB_O_ADDR = (INT32U)outb_buf;
    if (enable)
        temp |= (0x1<<5);
    else
        temp &=~(0x1<<5);

    R_BOXFILTER_CTRL = temp;

//    drv_l1_boxfilter_unlock();



    return 0;
}

/*****************************************************
*	Deiver Level 1 BOXFILTER Coef set:
*	@brief Set box filter coefficient
*   @param  coef: coefficient address point
*   @return 0:success; -1:fail
*****************************************************/
INT32S drv_l1_boxfilter_coef_set(INT8S* coef)
{
    INT32U temp,temp1,temp2, temp3;
    BOXFILTER_SFR box_sfr = get_BOXFILTER_Reg_Base();
    if(!coef)
        return -1;

//    drv_l1_boxfilter_lock();

//    box_sfr->boxfilter_coef1 = (((INT32U) *coef) &0xff) + ((((INT32U) *++coef) <<8)&0xff00) + ((((INT32U) *++coef) <<16)&0xff0000);
    //box_sfr->boxfilter_coef1 = (((INT32U) coef[0]) &0xff) + ((((INT32U) coef[1]) <<8)&0xff00) + ((((INT32U) coef[2]) <<16)&0xff0000);
    temp = (((INT32U) *coef) &0xff);
    temp1 = ((((INT32U) *++coef) <<8)&0xff00);
    temp2 = ((((INT32U) *++coef) <<16)&0xff0000);
    box_sfr->boxfilter_coef1 = temp + temp1 + temp2;

//    box_sfr->boxfilter_coef2 = ((INT32U)*++coef &0xff) + (((INT32U)*++coef <<8)&0xff00) +  (((INT32U)*++coef <<16)&0xff0000);
    temp = (((INT32U) *++coef) &0xff);
    temp1 = ((((INT32U) *++coef) <<8)&0xff00);
    temp2 = ((((INT32U) *++coef) <<16)&0xff0000);
    box_sfr->boxfilter_coef2 = temp + temp1 + temp2;

//    box_sfr->boxfilter_coef3 = ((INT32U)*++coef &0xff) + (((INT32U)*++coef <<8)&0xff00) +  (((INT32U)*++coef <<16)&0xff0000) + (((INT32U)*++coef <<24)&0xff000000);
    temp = (((INT32U) *++coef) &0xff);
    temp1 = ((((INT32U) *++coef) <<8)&0xff00);
    temp2 = ((((INT32U) *++coef) <<16)&0xff0000);
    temp3 = (((INT32U) *++coef <<24)&0xff000000);
    box_sfr->boxfilter_coef3 = temp + temp1 + temp2 + temp3;

//    box_sfr->boxfilter_coef1 = ((INT32U)coef->coef_a &0xff) + (((INT32U)coef->coef_b <<8)&0xff00) +  (((INT32U)coef->coef_c <<16)&0xff0000);
//    box_sfr->boxfilter_coef2 = ((INT32U)coef->coef_d &0xff) + (((INT32U)coef->coef_e <<8)&0xff00) +  (((INT32U)coef->coef_f <<16)&0xff0000);
//    box_sfr->boxfilter_coef3 = ((INT32U)coef->coef_g &0xff) + (((INT32U)coef->coef_h <<8)&0xff00) +  (((INT32U)coef->coef_i <<16)&0xff0000) + (((INT32U)coef->coef_j<<24)&0xff000000);
//    drv_l1_boxfilter_unlock();
    return 0;
}

/*****************************************************
*	Deiver Level 1 BOXFILTER Input CH:
*	@brief Set box filter input channel
*   @param  ch: reference BOXFTR_CH
*   @return NONE
*****************************************************/
void drv_l1_boxfilter_input_ch(BOXFTR_CH ch)
{
    INT32U temp = R_BOXFILTER_CTRL;

//    drv_l1_boxfilter_lock();
    temp &= ~C_BOX_CH_MASK;
    temp |= ch;
    R_BOXFILTER_CTRL = temp;
//    drv_l1_boxfilter_unlock();


}

/*****************************************************
*	Deiver Level 1 BOXFILTER OUTPUT FORMAT:
*	@brief Set box filter output format
*   @param  out_mode: reference BOXFILTER_OUT_MODE
*   @return NONE
*****************************************************/
void drv_l1_boxfilter_output_mode(BOXFILTER_OUT_MODE out_mode)
{
    INT32U temp = R_BOXFILTER_CTRL;

//    drv_l1_boxfilter_lock();
    temp &= ~C_FILTER_OUT_MASK;
    temp |= (out_mode<<6);
    R_BOXFILTER_CTRL = temp;
//    drv_l1_boxfilter_unlock();

}

/*****************************************************
*	Deiver Level 1 BOXFILTER SET:
*	@brief  box filter register initial
*   @param  img: input source information; coef: coefficient; out_mode: Output mode selsction
*   @return NONE
*****************************************************/
void  drv_l1_boxftr_reg_init(gpImage* img, INT8U* out_buf, INT8S* coef, BOXFILTER_OUT_MODE out_mode)
{

    drv_l1_boxfilter_src_img_info(img->format, img->widthStep, img->height, img->ptr, out_mode);
    drv_l1_boxfilter_outa_img_addr(out_buf);
    drv_l1_boxfilter_coef_set(coef);
    drv_l1_boxfilter_input_ch(img->ch);
    drv_l1_boxfilter_output_mode(out_mode);

}

/*****************************************************
*	Deiver Level 1 BOXFILTER Start:
*	@brief  box filter start work
*   @param  NONE
*   @return NONE
*****************************************************/
void drv_l1_boxfilter_start(void)
{

 //   drv_l1_boxfilter_lock();
    R_BOXFILTER_START = C_BOXFILTER_START;
//    drv_l1_boxfilter_unlock();

}

/*****************************************************
*	Deiver Level 1 BOXFILTER END wait:
*	@brief wait box filter done
*   @param 1: wait; 0: no wait
*   @return NONE
*****************************************************/
INT32S drv_l1_boxfilter_end_wait(INT32U wait)
{
    INT32S state = -1;
//    drv_l1_boxfilter_lock();
    if(wait)
    {
/*
        osEvent result;

        result = osMessageGet(boxfilter_q, osWaitForever);
        if (result.status != osEventMessage) {
            while(1);
        }
        state = result.value.v;
*/
///*
        while (1)
        {
            if ((box_int_flag & 1) == 1)
                break;

            osDelay(1);
        };
        state = box_int_flag;
        box_int_flag = 0;
//*/
//        DBG_PRINT("r");

//        if (xQueueReceive(boxfilter_q, &state, portMAX_DELAY) == pdTRUE)
//            return state;
    }
//    drv_l1_boxfilter_unlock();

    return state;
}

/**
 * @brief   gray format transfer to other color depend on sor_img->format
 * @param   sor_img: output image information
 * @param   g_img: gray image information
 * @return 	none
 */

void drv_l1_yuv_generator(gpImage* sor_img, gpImage* g_img)
{
    INT8U *sor, *gray;
    INT16S w, h;

    sor = sor_img->ptr;
    gray = g_img->ptr;
    switch(sor_img->format)
    {
        case IMG_FMT_GRAY:
            for (h=0; h<g_img->height;h++)
            {
                for (w=0; w<g_img->widthStep;w++)
                {
                    *sor++ = *gray++;//0xff; //*gray++;
                }
            }
        break;
        case IMG_FMT_UYVY:
        /* wiki define UYUV
            for (h=0; h<g_img->height;h++)
            {
                for (w=0; w<g_img->widthStep;w++)
                {
                    *sor++ = (INT8U)w;
                    *sor++ = *gray++;
                }
            }
        */
        ///* G+ define UYVY[31:0] = U0Y1V0Y0
            for (h=0; h<g_img->height;h++)
            {
                for (w=0; w<g_img->widthStep;w++)
                {
                    *sor++ = *gray++;//0xff; //*gray++;
                    *sor++ = w;
                }
            }
        //*/
        break;
        case IMG_FMT_YUYV:
        /* wiki define YUYV == Y0U0Y1V0
            for (h=0; h<g_img->height;h++)
            {
                for (w=0; w<g_img->widthStep;w++)
                {
                    *sor++ = *gray++;
                    *sor++ = w;
                }
            }
        */
        ///* G+ define YUYV[31:0] == Y1U0Y0V0
            for (h=0; h<g_img->height;h++)
            {
                for (w=0; w<g_img->widthStep;w++)
                {
                    *sor++ = (INT8U)w;
                    *sor++ = *gray++;//0xff; //*gray++;
                }
            }
        //*/
        break;
        case IMG_FMT_GP420:
            for (h=0; h<(g_img->height/2);h++)
            {
//                sor = h*g_img->height*2;
                for (w=0; w<g_img->widthStep;w++)
                    *sor++ = *gray++;//0xff; //*gray++;

                for (w=0; w<g_img->widthStep;w++)
                    *sor++ = (INT8U)w;

                for (w=0; w<g_img->widthStep;w++)
                    *sor++ = *gray++;//0xff; //*gray++;

            }
        break;

    }
}
#if 1
/*****************************************************
*	Deiver Level 1 SW BOX ONE Pixel OUTPUT:
*
*****************************************************/
static INT8U _sw_box_out(INT8U* matrix, BOXFILTER_COEF* coef, BOXFILTER_OUT_MODE mode)
{
    INT32S ret;
    INT8S* _coef;
    INT32U i;
    _coef = (INT8S*)coef;
    if (mode == C_BOXFTR_BYPASS)
    {
        ret = matrix[4];
    }else
    {
        ret = 0;
        for(i=0;i<9;i++)
        {
            ret += (INT32S)*matrix++*(INT32S)*_coef++;
        }
        if ((ret<0) && (mode == C_BOXFTR_OUT_ABS))
            ret = abs(ret);
        else if ((ret<0) && (mode == C_BOXFTR_OUT))
            ret = 0;
        ret/= (0x1<<*_coef);
        if (ret>0xff)
            ret = 0xff;
    }
    return (INT8U)ret;
}


/**
 * Deiver Level 1 BOXFILTER get image Y only:
 * @brief   tranfer image to gray image
 * @param   img: source image information
 * @param   tag_out: output buffer
 * @return 	SUCCESS
 */

INT32S drv_l1_get_y_only(gpImage* img, INT8U* tag_out)
{
    INT32U w, h, c;
    INT32U cnt,cnt1 = 0;
    switch(img->format)
    {
        case IMG_FMT_GRAY:
            for (c = 0; c<(img->widthStep*img->height);c++)
                tag_out[c] = img->ptr[c];
        break;
        case IMG_FMT_YUYV:
            for(c=0;c<(img->widthStep*img->height*2);c++)
                tag_out[c/2] = img->ptr[c];
        break;
        case IMG_FMT_UYVY:
            for(c=0;c<(img->widthStep*img->height*2-1);c++)
                tag_out[(c+1)/2] = img->ptr[c];
        break;
        case IMG_FMT_GP420:
            for(h=0;h<((img->height*3)>>1);h++)
            {
                cnt = h%3;
                if (cnt == 0)
                {
                    for(w=0;w<img->widthStep;w++)
                        tag_out[cnt1*img->widthStep+w]=img->ptr[(img->widthStep)*h+w];
                    cnt1++;
                }else if (cnt ==2)
                {
                    for(w=0;w<img->widthStep;w++)
                        tag_out[cnt1*img->widthStep+w]=img->ptr[(img->widthStep)*h+w];
                    cnt1++;
                }
            }
        break;
    }
    return SUCCESS;
}

/*****************************************************
*	Deiver Level 1 BOXFILTER SW C Model:
*
*****************************************************/

INT32S drv_l1_boxfilter_sw_start(gpImage* img, INT8U* sw_tag_out, BOXFILTER_COEF* coef,BOXFILTER_OUT_MODE mode)
{
    INT8U *gray_buf=0, gray_matrix[9];
    INT32U w, h, c, d;
    INT8U *_out;
    INT32U cnt,cnt1 = 0;

    _out = (INT8U*)sw_tag_out;
    switch(img->format)
    {
        case IMG_FMT_GRAY:
            gray_buf = img->ptr;
        break;
        case IMG_FMT_YUYV:
            gray_buf = (INT8U*)gp_malloc((INT32U)(img->widthStep*img->height));
            for(c=0;c<(img->widthStep*img->height*2);c++)
                gray_buf[c/2] = img->ptr[c];
        break;
        case IMG_FMT_UYVY:
            gray_buf = (INT8U*)gp_malloc(img->widthStep*img->height);
            for(c=0;c<(img->widthStep*img->height*2-1);c++)
                gray_buf[(c+1)/2] = img->ptr[c];
        break;
        case IMG_FMT_GP420:
            gray_buf = (INT8U*)gp_malloc(img->widthStep*img->height);
            for(h=0;h<((img->height*3)>>1);h++)
            {
                cnt = h%3;
                if (cnt == 0)
                {
                    for(w=0;w<img->widthStep;w++)
                        gray_buf[cnt1*img->widthStep+w]=img->ptr[(img->widthStep)*h+w];
                    cnt1++;
                }else if (cnt ==2)
                {
                    for(w=0;w<img->widthStep;w++)
                        gray_buf[cnt1*img->widthStep+w]=img->ptr[(img->widthStep)*h+w];
                    cnt1++;
                }else{
                    gray_buf[cnt1*img->widthStep+w] = gray_buf[cnt1*img->widthStep+w];
                }
            }
        break;
    }
    for(h=0;h<img->height;h++)
    {
        for(w=0;w<img->widthStep;w++)
        {
            c = (img->widthStep*h+w)/img->widthStep;
            d = (img->widthStep*h+w)%img->widthStep;
            if(h==0 && w==0) //[0]
                gray_matrix[0] =gray_buf[0];
            else if(w==0 && d==0)
                gray_matrix[0]=gray_buf[img->widthStep*(h-1)+w];
            else if (h==0 && c==0)
                gray_matrix[0]=gray_buf[w-1];
            else
                gray_matrix[0]=gray_buf[img->widthStep*(h-1)+w-1];

            if(h==0) //[1]
                gray_matrix[1]=gray_buf[w];
            else
                gray_matrix[1]=gray_buf[img->widthStep*(h-1)+w];
            //[2]
            if (w==(img->widthStep-1) && h==0)
                gray_matrix[2]= gray_buf[w];
            else if (h==0)
                gray_matrix[2]=gray_buf[w+1];
            else if(w==(img->widthStep-1))
                gray_matrix[2]=gray_buf[img->widthStep*(h-1)+w];
            else
                gray_matrix[2]=gray_buf[img->widthStep*(h-1)+w+1];
            //[3]
            if  (w==0)
                gray_matrix[3]=gray_buf[img->widthStep*h+w];
            else
                gray_matrix[3]=gray_buf[img->widthStep*h+w-1];
            //[4]
            gray_matrix[4]=gray_buf[img->widthStep*h+w];
            //[5]
            if (w==(img->widthStep-1))
                gray_matrix[5]=gray_buf[img->widthStep*h+w];
            else
                gray_matrix[5]=gray_buf[img->widthStep*h+w+1];
            //[6]
            if (w==0 && h==(img->height-1))
                gray_matrix[6]=gray_buf[img->widthStep*h+w];
            else if (w==0)
                gray_matrix[6]=gray_buf[img->widthStep*(h+1)];
            else if (h==(img->height-1))
                gray_matrix[6]=gray_buf[img->widthStep*h+w-1];
            else
                gray_matrix[6]=gray_buf[img->widthStep*(h+1)+w-1];
            //[7]
            if (h==(img->height-1))
                gray_matrix[7]=gray_buf[img->widthStep*h+w];
            else
                gray_matrix[7]=gray_buf[img->widthStep*(h+1)+w];
            //[8]
            if (w==(img->widthStep-1) && h==(img->height-1))
                gray_matrix[8]= gray_buf[img->widthStep*h+w];
            else if (h==(img->height-1))
                gray_matrix[8]=gray_buf[img->widthStep*h+w+1];
            else if (w==(img->widthStep-1))
                gray_matrix[8]=gray_buf[img->widthStep*(h+1)+w];
            else
                gray_matrix[8]=gray_buf[img->widthStep*(h+1)+w+1];

            *_out++ = _sw_box_out((INT8U*)&gray_matrix, coef, mode);
        }
//         DBG_PRINT("w = %d, h= %d \r\n",w,h);
    }
    if(!(img->format==IMG_FMT_GRAY))
        gp_free(gray_buf);
    return 0;
}
#endif







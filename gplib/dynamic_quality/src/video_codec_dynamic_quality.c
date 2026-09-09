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
#include "video_codec_dynamic_quality.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/


/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void
vid_dyna_tune_Q_init(
	JengDynaQ_t* pJpeg_dyna_q,
	INT32U encSizMax,
	INT32U encSizMid,
	INT32U encSizMin,
	INT8U  emerY,
	INT8U  maxY,
	INT8U  minY,
	INT8U  step
)
{
	pJpeg_dyna_q->enc_size_max = encSizMax;
	pJpeg_dyna_q->enc_size_mid = encSizMid;
	pJpeg_dyna_q->enc_size_min = encSizMin;
	pJpeg_dyna_q->emer_q.Y_q_table = emerY;
	pJpeg_dyna_q->max_q.Y_q_table = maxY;
	pJpeg_dyna_q->min_q.Y_q_table = minY;
	pJpeg_dyna_q->step_q = step;
}

void
vid_dyna_tune_Q_cal(
	JengDynaQ_t* pJpeg_dyna_q,
	INT32U encSize,
	INT32U* pRetYq
)
{
	INT32U curr_Y_q = *pRetYq;
	INT32U enc_size_max = pJpeg_dyna_q->enc_size_max;
	INT32U enc_size_mid = pJpeg_dyna_q->enc_size_mid;
	INT32U enc_size_min = pJpeg_dyna_q->enc_size_min;
	INT8U emer_Y_q = pJpeg_dyna_q->emer_q.Y_q_table;
	INT8U max_Y_q = pJpeg_dyna_q->max_q.Y_q_table;
	INT8U min_Y_q = pJpeg_dyna_q->min_q.Y_q_table;
	INT8U step_q = pJpeg_dyna_q->step_q;

	if(encSize <= 0) // Encode size overflow buffer size
	{
		if(curr_Y_q > emer_Y_q)
		{
			curr_Y_q = emer_Y_q;
		}
		else
		{
			curr_Y_q = min_Y_q;
		}
	}
	else
	{
		if(encSize > enc_size_max)
		{
			if(curr_Y_q > emer_Y_q)
			{
				curr_Y_q = emer_Y_q;
			}
			else
			{
				curr_Y_q = min_Y_q;
			}
		}
		else if(encSize > enc_size_mid)
		{
			if(curr_Y_q > min_Y_q)
			{
				curr_Y_q -= step_q;
			}
		}
		else if(encSize < enc_size_min)
		{
			if(curr_Y_q < max_Y_q)
			{
				curr_Y_q += step_q;
			}
		}
	}

	//DBG_PRINT("curr_Y_q:%d\r\n",curr_Y_q);

	if(*pRetYq != curr_Y_q)
	{
		*pRetYq = curr_Y_q;
	}
}


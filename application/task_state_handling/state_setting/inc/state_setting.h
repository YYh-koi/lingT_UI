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
#ifndef __STATE_SETTING_H__
#define __STATE_SETTING_H__

#include "typedef.h"

typedef struct
{
    void    (*USB_select_entry_ptr)(void *para);
    void    (*state_setting_entry_ptr)(void *para);
} Overlap_State_Setting_Function;

extern Overlap_State_Setting_Function State_Seting;
extern void     overlap_state_setting_function_initial(void);

extern void		state_setting_init(INT32U prev_state);
extern void		state_setting_exit(INT32U prev_state);

extern void     USB_select_entry(void *para);
extern void     state_setting_entry(void *para);
//extern void   USB_select_entry_TP(void *para);
//extern void   state_setting_entry_TP(void *para);


#endif //#ifndef __STATE_SETTING_H__

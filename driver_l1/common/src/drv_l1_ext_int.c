/*
* Purpose: External interrupt driver/interface
*
* Author: Allen Lin
*
* Date: 2008/05/28
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 1.00
*/
#include "drv_l1.h"

void EXTA_IRQHandler(void);
void EXTB_IRQHandler(void);
void EXTC_IRQHandler(void);

void (*exta_callback)(void);
void (*extb_callback)(void);
void (*extc_callback)(void);
void (*extd_callback)(void);
void (*exte_callback)(void);
void (*extf_callback)(void);
void (*extg_callback)(void);
void (*exth_callback)(void);
void (*exti_callback)(void);
void (*extj_callback)(void);
void (*extk_callback)(void);
void (*extl_callback)(void);

/**
 * @brief   External interrupt initialize.
 * @param   ext_src - EXTAB_ENUM
 * @param   vic_num - 0:IRQ, 1~4:FIQ number
 * @param   user_isr - callback function
 * @return 	none
 */
void drv_l1_ext_int_init(INT8U ext_src, INT8U vic_num, void (*user_isr)(void))
{
        drv_l1_ext_enable_set(ext_src, ENABLE);
		drv_l1_ext_user_isr_set(ext_src, user_isr);
        switch(ext_src){
            case EXTA:
                if (vic_num == 0)
                {
                    vic_irq_register(VIC_EXTA, EXTA_IRQHandler);
                    vic_irq_enable(VIC_EXTA);
                }else{
                    vic_fiq_enable(VIC_EXTA);
                }
            break;

            case EXTB:
                if (vic_num == 0)
                {
                    vic_irq_register(VIC_EXTB, EXTB_IRQHandler);
                    vic_irq_enable(VIC_EXTB);
                }else{
                    vic_fiq_enable(VIC_EXTB);
                }
            break;

            case EXTC:
                if (vic_num == 0)
                {
                    vic_irq_register(VIC_EXTC, EXTC_IRQHandler);
                    vic_irq_enable(VIC_EXTC);
                }else{
                    vic_fiq_enable(VIC_EXTC);
                }
            break;

            case EXTD:
                if (vic_num == 0)
                {
                    vic_irq_register(VIC_EXTD, EXTC_IRQHandler);
                    vic_irq_enable(VIC_EXTD);
                }else{
                    vic_fiq_enable(VIC_EXTD);
                }
            break;

            case EXTE:
                if (vic_num == 0)
                {
                    vic_irq_register(VIC_EXTE, EXTC_IRQHandler);
                    vic_irq_enable(VIC_EXTE);
                }else{
                    vic_fiq_enable(VIC_EXTE);
                }
            break;

            case EXTF:
                if (vic_num == 0)
                {
                    vic_irq_register(VIC_EXTF, EXTC_IRQHandler);
                    vic_irq_enable(VIC_EXTF);
                }else{
                    vic_fiq_enable(VIC_EXTF);
                }
            break;

            case EXTG:
                if (vic_num == 0)
                {
                    vic_irq_register(VIC_EXTG, EXTC_IRQHandler);
                    vic_irq_enable(VIC_EXTG);
                }else{
                    vic_fiq_enable(VIC_EXTG);
                }
            break;

            case EXTH:
                if (vic_num == 0)
                {
                    vic_irq_register(VIC_EXTH, EXTC_IRQHandler);
                    vic_irq_enable(VIC_EXTH);
                }else{
                    vic_fiq_enable(VIC_EXTH);
                }
            break;

            case EXTI:
                if (vic_num == 0)
                {
                    vic_irq_register(VIC_EXTI, EXTC_IRQHandler);
                    vic_irq_enable(VIC_EXTI);
                }else{
                    vic_fiq_enable(VIC_EXTI);
                }
            break;

            case EXTJ:
                if (vic_num == 0)
                {
                    vic_irq_register(VIC_EXTJ, EXTC_IRQHandler);
                    vic_irq_enable(VIC_EXTJ);
                }else{
                    vic_fiq_enable(VIC_EXTJ);
                }
            break;

            case EXTK:
                if (vic_num == 0)
                {
                    vic_irq_register(VIC_EXTK, EXTC_IRQHandler);
                    vic_irq_enable(VIC_EXTK);
                }else{
                    vic_fiq_enable(VIC_EXTK);
                }
            break;

            case EXTL:
                if (vic_num == 0)
                {
                    vic_irq_register(VIC_EXTL, EXTC_IRQHandler);
                    vic_irq_enable(VIC_EXTL);
                }else{
                    vic_fiq_enable(VIC_EXTL);
                }
            break;

            default:

            break;
        }
}
/**
 * @brief   External interrupt uninitialize.
 * @param   ext_src - EXTAB_ENUM
 * @param   vic_num - 0:IRQ, 1~4:FIQ number
 * @return 	none
 */
void drv_l1_ext_int_uninit(INT8U ext_src, INT8U vic_num)
{
    drv_l1_ext_enable_set(ext_src, DISABLE);
    drv_l1_ext_user_isr_clr(ext_src);

    switch(ext_src){
        case EXTA:
            if (vic_num == 0)
                vic_irq_disable(VIC_EXTA);
            else
                vic_fiq_disable(VIC_EXTA);
        break;

        case EXTB:
            if (vic_num == 0)
                vic_irq_disable(VIC_EXTB);
            else
                vic_fiq_disable(VIC_EXTB);
        break;

        case EXTC:
            if (vic_num == 0)
                vic_irq_disable(VIC_EXTC);
            else
                vic_fiq_disable(VIC_EXTC);
            break;

        case EXTD:
            if (vic_num == 0)
                vic_irq_disable(VIC_EXTD);
            else
                vic_fiq_disable(VIC_EXTD);
        break;

        case EXTE:
            if (vic_num == 0)
                vic_irq_disable(VIC_EXTE);
            else
                vic_fiq_disable(VIC_EXTE);
        break;

        case EXTF:
            if (vic_num == 0)
                vic_irq_disable(VIC_EXTF);
            else
                vic_fiq_disable(VIC_EXTF);
        break;

        case EXTG:
            if (vic_num == 0)
                vic_irq_disable(VIC_EXTG);
            else
                vic_fiq_disable(VIC_EXTG);
        break;

        case EXTH:
            if (vic_num == 0)
                vic_irq_disable(VIC_EXTH);
            else
                vic_fiq_disable(VIC_EXTH);
        break;

        case EXTI:
            if (vic_num == 0)
                vic_irq_disable(VIC_EXTI);
            else
                vic_fiq_disable(VIC_EXTI);
        break;

        case EXTJ:
            if (vic_num == 0)
                vic_irq_disable(VIC_EXTJ);
            else
                vic_fiq_disable(VIC_EXTJ);
        break;

        case EXTK:
            if (vic_num == 0)
                vic_irq_disable(VIC_EXTK);
            else
                vic_fiq_disable(VIC_EXTK);
        break;

        case EXTL:
            if (vic_num == 0)
                vic_irq_disable(VIC_EXTL);
            else
                vic_fiq_disable(VIC_EXTL);
        break;

        default:

        break;
    }
}
/**
 * @brief   External interrupt flag clear. (write 1 clear)
 * @param   ext_src - EXTAB_ENUM
 * @return 	none
 */
void drv_l1_ext_int_clr(INT8U ext_src)
{
	if (ext_src == EXTA) {
		R_INT_KECON |= EXTA_INT;
	}
	else if (ext_src == EXTB) {
		R_INT_KECON |= EXTB_INT;
	}
	else if (ext_src == EXTC) {
		R_INT_KECON |= EXTC_INT;
	}
	else if (ext_src == EXTD) {
		R_INT_KECON |= EXTD_INT;
	}
	else if (ext_src == EXTE) {
		R_INT_KECON |= EXTE_INT;
	}
	else if (ext_src == EXTF) {
		R_INT_KECON |= EXTF_INT;
	}
	else if (ext_src == EXTG) {
		R_INT_KECON2 |= EXTG_INT;
	}
	else if (ext_src == EXTH) {
		R_INT_KECON2 |= EXTH_INT;
	}
	else if (ext_src == EXTI) {
		R_INT_KECON2 |= EXTI_INT;
	}
	else if (ext_src == EXTJ) {
		R_INT_KECON2 |= EXTJ_INT;
	}
	else if (ext_src == EXTK) {
		R_INT_KECON2 |= EXTK_INT;
	}
	else if (ext_src == EXTL) {
		R_INT_KECON2 |= EXTL_INT;
	}
}
/**
 * @brief   External interrupt edge set.
 * @param   ext_src - EXTAB_ENUM
 * @param   edge_type - EXTAB_EDGE_ENUM
 * @return 	none
 */
void drv_l1_ext_edge_set(INT8U ext_src, INT8U edge_type)
{
	if (ext_src == EXTA) {
		R_INT_KECON &= ~EXTA_POL;
		R_INT_KECON |= (edge_type << 3);
	}
	else if (ext_src == EXTB){
		R_INT_KECON &= ~EXTB_POL;
		R_INT_KECON |= (edge_type << 4);
	}
	else if (ext_src == EXTC){
		R_INT_KECON &= ~EXTC_POL;
		R_INT_KECON |= (edge_type << 5);
	}
	else if (ext_src == EXTD){
		R_INT_KECON &= ~EXTD_POL;
		R_INT_KECON |= (edge_type << 19);
	}
	else if (ext_src == EXTE){
		R_INT_KECON &= ~EXTE_POL;
		R_INT_KECON |= (edge_type << 20);
	}
	else if (ext_src == EXTF){
		R_INT_KECON &= ~EXTF_POL;
		R_INT_KECON |= (edge_type << 21);
	}
	else if (ext_src == EXTG){
		R_INT_KECON2 &= ~EXTG_POL;
		R_INT_KECON2 |= (edge_type << 3);
	}
	else if (ext_src == EXTH){
		R_INT_KECON2 &= ~EXTH_POL;
		R_INT_KECON2 |= (edge_type << 4);
	}
	else if (ext_src == EXTI){
		R_INT_KECON2 &= ~EXTI_POL;
		R_INT_KECON2 |= (edge_type << 5);
	}
	else if (ext_src == EXTJ){
		R_INT_KECON2 &= ~EXTJ_POL;
		R_INT_KECON2 |= (edge_type << 19);
	}
	else if (ext_src == EXTK){
		R_INT_KECON2 &= ~EXTK_POL;
		R_INT_KECON2 |= (edge_type << 20);
	}
	else if (ext_src == EXTL){
		R_INT_KECON2 &= ~EXTL_POL;
		R_INT_KECON2 |= (edge_type << 21);
	}
}
/**
 * @brief   External interrupt enable set.
 * @param   ext_src - EXTAB_ENUM
 * @param   status - 1:enable, 0:disable
 * @return 	none
 */
void drv_l1_ext_enable_set(INT8U ext_src, BOOLEAN status)
{
	if (ext_src == EXTA) {
		if (status == TRUE) {
			R_INT_KECON |= EXTA_IEN;
		}
		else {
			R_INT_KECON &= ~EXTA_IEN;
		}
	}
	else if (ext_src == EXTB) {
		if (status == TRUE) {
			R_INT_KECON |= EXTB_IEN;
		}
		else {
			R_INT_KECON &= ~EXTB_IEN;
		}
	}
	else if (ext_src == EXTC) {
		if (status == TRUE) {
			R_INT_KECON |= EXTC_IEN;
		}
		else {
			R_INT_KECON &= ~EXTC_IEN;
		}
    }
    else if (ext_src == EXTD) {
		if (status == TRUE) {
			R_INT_KECON |= EXTD_IEN;
		}
		else {
			R_INT_KECON &= ~EXTD_IEN;
		}
    }
    else if (ext_src == EXTE) {
		if (status == TRUE) {
			R_INT_KECON |= EXTE_IEN;
		}
		else {
			R_INT_KECON &= ~EXTE_IEN;
		}
    }
    else if (ext_src == EXTF) {
		if (status == TRUE) {
			R_INT_KECON |= EXTF_IEN;
		}
		else {
			R_INT_KECON &= ~EXTF_IEN;
		}
    }
    else if (ext_src == EXTG) {
		if (status == TRUE) {
			R_INT_KECON2 |= EXTG_IEN;
		}
		else {
			R_INT_KECON2 &= ~EXTG_IEN;
		}
    }
    else if (ext_src == EXTH) {
		if (status == TRUE) {
			R_INT_KECON2 |= EXTH_IEN;
		}
		else {
			R_INT_KECON2 &= ~EXTH_IEN;
		}
    }
    else if (ext_src == EXTI) {
		if (status == TRUE) {
			R_INT_KECON2 |= EXTI_IEN;
		}
		else {
			R_INT_KECON2 &= ~EXTI_IEN;
		}
    }
    else if (ext_src == EXTJ) {
		if (status == TRUE) {
			R_INT_KECON2 |= EXTJ_IEN;
		}
		else {
			R_INT_KECON2 &= ~EXTJ_IEN;
		}
    }
    else if (ext_src == EXTK) {
		if (status == TRUE) {
			R_INT_KECON2 |= EXTK_IEN;
		}
		else {
			R_INT_KECON2 &= ~EXTK_IEN;
		}
    }
    else if (ext_src == EXTL) {
		if (status == TRUE) {
			R_INT_KECON2 |= EXTL_IEN;
		}
		else {
			R_INT_KECON2 &= ~EXTL_IEN;
		}
    }
}
/**
 * @brief   External interrupt callback set.
 * @param   ext_src - EXTAB_ENUM
 * @param   user_isr - callback function
 * @return 	none
 */
void drv_l1_ext_user_isr_set(INT8U ext_src, void (*user_isr)(void))
{
	if(user_isr == 0) {
		return;
	}
	if(ext_src == EXTA) {
		exta_callback = user_isr;
	}
	else if(ext_src == EXTB) {
		extb_callback = user_isr;
	}
	else if(ext_src == EXTC){
		extc_callback = user_isr;
	}
	else if(ext_src == EXTD){
        extd_callback = user_isr;
	}
	else if(ext_src == EXTE){
        exte_callback = user_isr;
	}
	else if(ext_src == EXTF){
        extf_callback = user_isr;
	}
	else if(ext_src == EXTG){
        extg_callback = user_isr;
	}
	else if(ext_src == EXTH){
        exth_callback = user_isr;
	}
	else if(ext_src == EXTI){
        exti_callback = user_isr;
	}
	else if(ext_src == EXTJ){
        extj_callback = user_isr;
	}
	else if(ext_src == EXTK){
        extk_callback = user_isr;
	}
	else if(ext_src == EXTL){
        extl_callback = user_isr;
	}
}
/**
 * @brief   External interrupt callback clear.
 * @param   ext_src - EXTAB_ENUM
 * @param   user_isr - callback function
 * @return 	none
 */
void drv_l1_ext_user_isr_clr(INT8U ext_src)
{
	if (ext_src == EXTA) {
		exta_callback = 0;
	}
	else if(ext_src == EXTB) {
		extb_callback = 0;
	}
	else if(ext_src == EXTC) {
		extc_callback = 0;
	}
	else if(ext_src == EXTD) {
		extd_callback = 0;
	}
	else if(ext_src == EXTE) {
		exte_callback = 0;
	}
	else if(ext_src == EXTF) {
		extf_callback = 0;
	}
	else if(ext_src == EXTG) {
		extg_callback = 0;
	}
	else if(ext_src == EXTH) {
		exth_callback = 0;
	}
	else if(ext_src == EXTI) {
		exti_callback = 0;
	}
	else if(ext_src == EXTJ) {
		extj_callback = 0;
	}
	else if(ext_src == EXTK) {
		extk_callback = 0;
	}
	else if(ext_src == EXTL) {
		extl_callback = 0;
	}
}
/**
 * @brief   EXTA IRQHandler
 * @param   none
 * @return 	none
 */
void EXTA_IRQHandler(void)
{
	INT32U status;
	status = R_INT_KECON;

	if (status & EXTA_INT) {
		R_INT_KECON |= EXTA_INT;
		if (exta_callback != 0) {
			(*exta_callback)();
		}
	}
}
/**
 * @brief   EXTB IRQHandler
 * @param   none
 * @return 	none
 */
void EXTB_IRQHandler(void)
{
	INT32U status;
	status = R_INT_KECON;

	if (status & EXTB_INT) {
		R_INT_KECON |= EXTB_INT;
		if (extb_callback != 0) {
			(*extb_callback)();
		}
	}
}
/**
 * @brief   EXTC_D_E_F IRQHandler
 * @param   none
 * @return 	none
 */
void EXTC_IRQHandler(void)
{
	INT32U status, status2;
	status = R_INT_KECON;
	status2 = R_INT_KECON2;

	if (status & EXTC_INT) {
		R_INT_KECON |= EXTC_INT;
		if (extc_callback != 0) {
			(*extc_callback)();
		}
	}
	else if (status & EXTD_INT)
	{
        R_INT_KECON |= EXTD_INT;
		if (extd_callback != 0) {
			(*extd_callback)();
		}
	}
	else if (status & EXTE_INT)
	{
        R_INT_KECON |= EXTE_INT;
		if (exte_callback != 0) {
			(*exte_callback)();
		}
	}
	else if (status & EXTF_INT)
	{
        R_INT_KECON |= EXTF_INT;
		if (extf_callback != 0) {
			(*extf_callback)();
		}
	}
	else if (status2 & EXTG_INT)
	{
        R_INT_KECON2 |= EXTG_INT;
		if (extg_callback != 0) {
			(*extg_callback)();
		}
	}
	else if (status2 & EXTH_INT)
	{
        R_INT_KECON2 |= EXTH_INT;
		if (exth_callback != 0) {
			(*exth_callback)();
		}
	}
	else if (status2 & EXTI_INT)
	{
        R_INT_KECON2 |= EXTI_INT;
		if (exti_callback != 0) {
			(*exti_callback)();
		}
	}
	else if (status2 & EXTJ_INT)
	{
        R_INT_KECON2 |= EXTJ_INT;
		if (extj_callback != 0) {
			(*extj_callback)();
		}
	}
	else if (status2 & EXTK_INT)
	{
        R_INT_KECON2 |= EXTK_INT;
		if (extk_callback != 0) {
			(*extk_callback)();
		}
	}
	else if (status2 & EXTL_INT)
	{
        R_INT_KECON2 |= EXTL_INT;
		if (extl_callback != 0) {
			(*extl_callback)();
		}
	}
}

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
#include "drv_l1_sfr.h"
#include "drv_l1_crypto.h"
#include "gp_stdlib.h"
#include "gplib_print_string.h"
#include "drv_l1_interrupt.h"
#include "drv_l1_clock.h"

#define C_RSA_RX_QUEUE_SZ 1

osMutexId crypto_sem = NULL;
osMessageQId RSA_rxQueue = NULL;
static kcrp_para        g_mod={0};
void RSA_IRQHandler(void);

/*
 * @brief: Allocate crypto lock semaphore.
 *         Must call first and using crypto/cipher L1, L2.
 * @param: None
 * @return: 0: SUCCESS, others: FAIL
*/
INT32S drv_l1_crypto_lock_alloc(void)
{
    crypto_sem = osMutexCreate(NULL);
    if(crypto_sem == NULL)
        return osErrorNoMemory;
    else
        return 0;
}

/*
 * @brief: Dispose crypto lock semaphore.
  *        Call this when no longer using crypto/cipher L1, L2.
 * @param: None
 * @return: None
*/
void drv_l1_crypto_lock_free(void)
{
    if(crypto_sem != NULL) {
        vQueueDelete(crypto_sem);
        crypto_sem = NULL;
    }
    return;
}

/*
 * @brief: Crypto lock semaphore.
 * @param: millisec: timeout in ms.
 * @return: 0: SUCCESS, others: refer to osStatus.
*/
INT32S drv_l1_crypto_lock(INT32U millisec)
{
    osStatus status = -1;

    if (crypto_sem != NULL)
    status = osMutexWait(crypto_sem, millisec);
    else
        status = drv_l1_crypto_lock_alloc();

    return status;
}

/*
 * @brief: Crypto unlock semaphore.
 * @param: None
 * @return: None
*/
void drv_l1_crypto_unlock(void)
{
    if (crypto_sem != NULL)
    osMutexRelease(crypto_sem);
}


/*
 * @brief: Initial the AES hardware
 * @param: None
 * @return: None
*/
void drv_l1_aes_init(void)
{
    drv_l1_clock_set_system_clk_en(CLK_EN7_SECURITY_IP, 1);
    R_SEC_CTRL = 0;
    R_AESDMA_RCSR &= ~BIT31; /*To disable auto dma*/
    R_SEC_CTRL1 |= 1;
	drv_l1_aes_dma_ie(ENABLE);

}

/*
 * @brief: Uninitial the AES hardware
 * @param: None
 * @return: None
*/
void drv_l1_aes_uninit(void)
{
    R_SEC_CTRL = 0;
	drv_l1_aes_dma_ie(DISABLE);
	drv_l1_clock_set_system_clk_en(CLK_EN7_SECURITY_IP, 0);
}
/*
 * @brief: AES set DMA transfer length
 * @param: len: DMA transfer length
 * @return: 0: Success; -1: Length is error; -2: AES DMA is not finished
*/
INT32S drv_l1_aes_set_dma_transfer_len(INT32U len)
{
	if (len<=0) {
		return DMA_LEN_ERR;
    }
	if (R_AESDMACS & BIT0) {
		DBG_PRINT("AES DMA is not finished\r\n");
		return DMA_NOT_FINISHED;
	}

	R_AESDMACS &= ~0xFFFF0000;
	R_AESDMACS |= (len<<16);

	return RET_SUCCESS;
}

/*
 * @brief: Enable AES uid key generator or not
 * @param: en: 0: disable; 1: enable
 * @return: None
*/
void drv_l1_aes_set_uid_key_gen_en(INT32U en)
{
    if (en) {
        R_AESDMACS |= BIT4;
    } else {
        R_AESDMACS &= (~BIT4);
    }
}

/*
 * @brief: AES DMA enable
 * @param: None
 * @return: 0: Success; -2: AES DMA is not finished.
*/
INT32S drv_l1_aes_dma_en(void)
{
	if (R_AESDMACS & BIT0) {
		DBG_PRINT("AES DMA is not finished\r\n");
		return DMA_NOT_FINISHED;
	}

	R_AESDMACS |= BIT0;
	return RET_SUCCESS;
}

/*
 * @brief: AES set DMA source address
 * @param: src: DMA source address
 * @return: 0: Success; -3: AES DMA source address is not 32B alignment.
*/
INT32S drv_l1_aes_set_dma_src(INT32U src)
{
	if (src & 0x1F ) {
		DBG_PRINT("aes dma src buffer is not 32B alignment!\r\n");
		return DMA_SRC_ADDR_ERR;
	}

	R_AESSPTR = src;
	return RET_SUCCESS;
}

/*
 * @brief: AES set DMA destination address
 * @param: dst: DMA destination address
 * @return: 0: Success; -4: Destination address is not 32Byte alignment.
*/
INT32S drv_l1_aes_set_dma_dst(INT32U dst)
{
	if (dst & 0x1F) {
		DBG_PRINT("aes dma destination buffer is not 32B alignment!\r\n");
		return DMA_DST_ADDR_ERR;
	}

	R_AESDPTR = dst;
	return RET_SUCCESS;
}

/*
 * @brief: AES set mode
 * @param: mode: AES mode.
 * @return: 0: Success; -2: AES DMA is not finished; -5: Mode is error;
*/
INT32S drv_l1_aes_set_mode(INT32U mode)
{
	if ( mode > AesMODE_MAX ) {
		DBG_PRINT("AES mode error!");
		return MODE_ERR;
	}

	if (R_AESDMACS & BIT0) {
		DBG_PRINT("AES DMA is not finished\r\n");
		return DMA_NOT_FINISHED;
	}

	R_AESPAR0 &= ~0x3F;
	R_AESPAR0 |= mode;

	return RET_SUCCESS;
}


/*
 * @brief: AES set direction
 * @param: dir: 0: Encrypt; 1: Decrypt
 * @return: 0: Success; -2: AES DMA is not finished; -6: Direction is error
*/
INT32S drv_l1_aes_set_dir(INT32U dir)
{
	if (dir > 1) {
		DBG_PRINT("AES direction error!");
		return DIR_ERR;
	}

	if (R_AESDMACS & BIT0) {
		DBG_PRINT("AES DMA is not finished\r\n");
		return DMA_NOT_FINISHED;
	}

	if (dir == AesENCRYPT) {	/* Encrypt */
		R_AESPAR0 &= ~BIT7;
	} else {					/* Decrypt */
		R_AESPAR0 |= BIT7;
	}

	return RET_SUCCESS;
}

/*
 * @brief: AES set key length
 * @param: key_len: 128, 192 or 256 bits
 * @return: 0: Success; -2: AES DMA is not finished; -7: AES key length is not 128, 192 or 256
*/
INT32S drv_l1_aes_set_key_len(INT32U key_len)
{
	if (key_len != 128 && key_len != 192 && key_len != 256) {
		DBG_PRINT("AES key length error!\r\n");
		return KEY_LEN_ERR;
	}

	if (R_AESDMACS & BIT0) {
		DBG_PRINT("AES DMA is not finished\r\n");
		return DMA_NOT_FINISHED;
	}

	R_AESPAR0 &= ~(0xFF<<16);
	R_AESPAR0 |= key_len<<11;

	return RET_SUCCESS;
}

/*
 * @brief: AES set IV
 * @param: iv: The initial vector adress
 * @return: 0: Success; -8: AES IV address is NULL.
*/
INT32S drv_l1_aes_set_iv(INT32U iv)
{
	if (iv == 0) {
		DBG_PRINT("IV ptr is NULL!\r\n");
		return IV_ERR;
	}

	R_AESPAR1 = (INT32U)iv;
	return RET_SUCCESS;
}

/*
 * @brief: AES set key
 * @param: key: The AES key address
 * @return: 0: Success; -9: AES key address is error.
*/
INT32S drv_l1_aes_set_key(INT32U key)
{
	if (key == 0) {
		DBG_PRINT("KEY ptr is NULL!\r\n");
		return KEY_ERR;
	}

	R_AESPAR2 = (INT32U)key;
	return RET_SUCCESS;
}

/*
 * @brief: AES set counter incensement part value
 * @param: cnt: Counter incensement part value
 * @return: 0: SUCCESS
*/
INT32S drv_l1_aes_set_CNT_M(INT32U cnt)
{
	R_AESPAR0 &= ~(0xFF<<24);
	R_AESPAR0 |= cnt<<24;

	return RET_SUCCESS;
}

/*
 * @brief: AES wait DMA done
 * @param: timeout_ticks: DMA timeout in ms. If this value is -1, it means no timeout.
 * @return: 0: Success; -10: DMA wait timeout.
*/
INT32S drv_l1_aes_wait_dma_done(INT32S timeout_ticks)
{
	INT32S ret = WAIT_TIMEOUT;

	int tstart = xTaskGetTickCount();
	do {
		if ( !(R_AESDMACS & BIT0) ) {
			ret = RET_SUCCESS;
			break;
		}
	} while ( (timeout_ticks<0)?1:(xTaskGetTickCount()-tstart<timeout_ticks));

	return ret;
}

/*
 * @brief: AES wait ring stop for AES auto DMA
 * @param: timeout_ticks: ring stop timeout in ms. If this value is -1, it means no timeout.
 * @return: 0: Success; -10: Wait ring stop timeout
*/
INT32S drv_l1_aes_wait_ring_stop(INT32S timeout_ticks)
{
	INT32S ret = WAIT_TIMEOUT;

	int tstart = xTaskGetTickCount();
	do {
		if ( !(R_AESDMA_CRCR & BIT3) ) {
			ret = RET_SUCCESS;
			break;
		}
	} while(( timeout_ticks<0)?1:(xTaskGetTickCount()-tstart<timeout_ticks));

    if (ret != RET_SUCCESS) {
        DBG_PRINT("aes ring not stop: %d\r\n", ret);
    }

	return ret;
}

/*
 * @brief: AES DMA interrupt enable
 * @param: enable: 0: disable AES interrupt; 1:enable AES interrupt
 * @return: 0: Success
*/
INT32S drv_l1_aes_dma_ie(INT32U enable)
{
	if (enable) {
		R_SECIE |= BIT2;
	} else {
		R_SECIE &= ~BIT2;
	}
	return RET_SUCCESS;
}

/*
 * @brief: Clear AES DMA interrupt flag
 * @param: None
 * @return: 0: Success
*/
INT32S drv_l1_aes_clr_dma_if(void)
{
	INT32U secif = R_SECIF;

	if ( R_SECIF & BIT6 ) {
		R_AESDMA_CRCR |= BIT1;	/* Command Ring STOP */
	}

	R_SECIF = secif; /*Clear flag*/

	return RET_SUCCESS;
}

/*
 * @brief: AES auto dma setting.
 * @param: ring: AES ring address.
 * @return: 0: Success; -11: AES ring is failed
*/
INT32S drv_l1_aes_auto_dma_setting(trb_ring_t *ring)
{
	INT32U phy_addr;
	INT32U event_ring_size;

	if (!ring) {
		DBG_PRINT("AES ring failed\r\n");
		return RING_FAIL;
	}
	phy_addr = ring->pa;
	if (!phy_addr) {
		DBG_PRINT("AES ring phy_addr failed\r\n");
		return RING_FAIL;
	}

	event_ring_size = ring->trb_num;
	if (event_ring_size <2) {
		DBG_PRINT("event ring size error:%d\r\n",event_ring_size);
		return RING_FAIL;
	}

	R_AESDMA_CRCR = (phy_addr|0x01);
	R_AESDMA_ERBAR = phy_addr;
	R_AESDMA_ERDPR = (phy_addr+event_ring_size*TRB_SIZE);
	R_AESDMA_RCSR = event_ring_size-1;
	R_AESDMA_RCSR |= (AUTODMA_RCSR_ERF);
	R_AESDMA_RCSR |= AUTODMA_RCSR_EN;
	R_SECIE = 0;
	R_SECIE |= BIT8;	/*bit8: AES_CRD_IE;*/
	R_SECIE |= BIT6;	/*bit6:AES_TRB_IE;*/

	return RET_SUCCESS;
}

/*
 * @brief: AES auto DMA trigger
 * @param: None
 * @return: 0: Success
*/
INT32S drv_l1_aes_auto_dma_trigger(void)
{
	R_AESDMA_RTR |= AUTODMA_TRIGGER;
	return RET_SUCCESS;
}



/************************** HASH driver **************************/
/*
 * @brief: Initial HASH hardware.
 * @param: None
 * @return: None
*/
void drv_l1_hash_init(void)
{
    drv_l1_clock_set_system_clk_en(CLK_EN7_SECURITY_IP, 1);
    //drv_l1_crypto_clear_register();
	R_SEC_CTRL = 1;
	R_HASHDMA_RCSR &= ~BIT31; /*To disable auto dma*/
    R_SEC_CTRL1 |= 1;
	drv_l1_hash_dma_ie(ENABLE);
}

/*
 * @brief: Unnitial HASH hardware.
 * @param: None
 * @return: None
*/
void drv_l1_hash_uninit(void)
{
    drv_l1_hash_dma_ie(DISABLE);
    R_SEC_CTRL1 = 0;
    R_SEC_CTRL = 0;
    drv_l1_clock_set_system_clk_en(CLK_EN7_SECURITY_IP, 0);
}
/*
 * @brief: HASH set DMA transfer length
 * @param: len: DMA transfer length
 * @return: 0: Success; -1: Length is error; -2: HASH DMA is not finished
*/
INT32S drv_l1_hash_set_dma_transfer_len(INT32U len)
{
	if (len<=0) {
		return DMA_LEN_ERR;
    }
	if (R_HASHDMACS & BIT0) {
		DBG_PRINT("hash DMA is not finished\r\n");
		return DMA_NOT_FINISHED;
	}

	R_HASHDMACS &= ~0xFFFF0000;
	R_HASHDMACS |= (len<<16);

	return RET_SUCCESS;
}

/*
 * @brief: HASH DMA enable
 * @param: None
 * @return: 0: Success; -2: HASH DMA is not finished.
*/
INT32S drv_l1_hash_dma_en(void)
{
	if (R_HASHDMACS & BIT0) {
		DBG_PRINT("hash DMA is not finished\r\n");
		return DMA_NOT_FINISHED;
	}

	R_HASHDMACS |= BIT0;
	return RET_SUCCESS;
}

/*
 * @brief: HASH set DMA source address
 * @param: src: DMA source address
 * @return: 0: Success; -3: HASH DMA source address is not 32B alignment.
*/
INT32S drv_l1_hash_set_dma_src(INT32U src)
{
	if (src&0x1F) {
		DBG_PRINT("hash dma src buffer is not 32B alignment!\r\n");
		return DMA_SRC_ADDR_ERR;
	}

	R_HASHSPTR = src;
	return RET_SUCCESS;
}

/*
 * @brief: HASH set DMA destination address
 * @param: dst: DMA destination address
 * @return: 0: Success; -4: Destination address is not 32Byte alignment.
*/
INT32S drv_l1_hash_set_dma_dst(INT32U dst)
{
	if (dst&0x1F) {
		DBG_PRINT("hash dma destination buffer is not 32B alignment!\r\n");
		return DMA_DST_ADDR_ERR;
	}

	R_HASHDPTR = dst;
	return RET_SUCCESS;
}

/*
 * @brief: HASH set IV
 * @param: iv: The initial vector adress
 * @return: 0: Success; -8: HASH IV address is NULL.
*/
INT32S drv_l1_hash_set_iv(INT32U iv)
{
	if (iv == 0) {
		DBG_PRINT("IV ptr is NULL!\r\n");
		return IV_ERR;
	}

	R_HASHPAR1 = iv;
	return RET_SUCCESS;
}

/*
 * @brief: HASH set mode
 * @param: mode: HASH mode.
 * @return: 0: Success; -5: Mode is error;
*/
INT32S drv_l1_hash_set_mode(INT32U mode)
{
	if (mode > HASH_MODE_MAX) {
		return MODE_ERR;
	}

	R_HASHPAR0 &= ~0x7F;
	R_HASHPAR0 |= mode;
	return RET_SUCCESS;
}

/*
 * @brief: HASH set output length
 * @param: hash_len: HASH out length,
 * @return: 0: Success; -2: HASH DMA is not finished
*/
INT32S drv_l1_hash_set_len(INT32U hash_len)
{
	if (R_HASHDMACS & BIT0) {
		DBG_PRINT("hash DMA is not finished\r\n");
		return DMA_NOT_FINISHED;
	}

	R_HASHPAR0 &= ~(0x3<<16);
	R_HASHPAR0 |= hash_len<<16;

	return RET_SUCCESS;
}

/*
 * @brief: Crypto get DMA interrupt flag
 * @param: None
 * @return: Interrupt flage register value.
*/
INT32U drv_l1_crypto_get_dma_if(void)
{
    return R_SECIF;
}

/*
 * @brief: HASH clear DMA interrupt flag
 * @param: None
 * @return: 0: Success
*/
INT32S drv_l1_hash_clr_dma_if(void)
{
	INT32U secif = R_SECIF;

	if ( R_SECIF & BIT5 ) {
		R_HASHDMA_CRCR |= BIT1;	/* Command Ring STOP */
	}

	R_SECIF = secif; /*Clear flag*/

	return RET_SUCCESS;
}

/*
 * @brief: HASH DMA interrupt enable
 * @param: enable: 0: Disable HASH interrupt; 1:Enable HASH interrupt
 * @return: 0: Success
*/
INT32S drv_l1_hash_dma_ie(INT32U enable)
{
	if (enable) {
		R_SECIE |= BIT1;
	} else {
		R_SECIE &= ~BIT1;
	}
	return RET_SUCCESS;
}

/*
 * @brief: HASH wait DMA done
 * @param: timeout_ticks: DMA timeout in ms. If this value is -1, it means no timeout.
 * @return: 0: Success; -10: DMA wait timeout.
*/
INT32S drv_l1_hash_wait_dma_done(INT32S timeout_ticks)
{
	INT32S ret = WAIT_TIMEOUT;

	int tstart = xTaskGetTickCount();
	do {
		if ( !(R_HASHDMACS & BIT0) ) {
			ret = RET_SUCCESS;
			break;
		}
	} while ( (timeout_ticks<0)?1:(xTaskGetTickCount()-tstart<timeout_ticks));

	return ret;
}

/*
 * @brief: HASH wait ring stop for HASH auto DMA
 * @param: timeout_ticks: ring stop timeout in ms. If this value is -1, it means no timeout.
 * @return: 0: Success; -10: Wait ring stop timeout
*/
INT32S drv_l1_hash_wait_ring_stop(INT32S timeout_ticks)
{
	INT32S ret = WAIT_TIMEOUT;

	int tstart = xTaskGetTickCount();
	do {
        if ( !(R_HASHDMA_CRCR & BIT3) ) {
			ret = RET_SUCCESS;
			break;
		}
	} while(( timeout_ticks<0)?1:(xTaskGetTickCount()-tstart<timeout_ticks));

    if (ret != RET_SUCCESS) {
        DBG_PRINT("hash ring not stop: %d\r\n", ret);
    }

	return ret;
}

/*
 * @brief: HASH auto dma setting.
 * @param: ring: HASH ring address.
 * @return: 0: Success; -11: HASH ring is failed
*/
INT32S drv_l1_hash_auto_dma_setting(trb_ring_t *ring)
{
	INT32U phy_addr;
	INT32U event_ring_size;

	if (!ring) {
		DBG_PRINT("HASH ring failed\r\n");
		return RING_FAIL;
	}
	phy_addr = ring->pa;
	if (!phy_addr) {
		DBG_PRINT("HASH ring phy_addr failed\r\n");
		return RING_FAIL;
	}

	event_ring_size = ring->trb_num;
	if (event_ring_size <2) {
		DBG_PRINT("HASH ring size error:%d\r\n",event_ring_size);
		return RING_FAIL;
	}

	R_HASHDMA_CRCR = (phy_addr|0x01);
	R_HASHDMA_ERBAR = phy_addr;
	R_HASHDMA_ERDPR = (phy_addr+event_ring_size*TRB_SIZE);
	R_HASHDMA_RCSR = event_ring_size-1;
	R_HASHDMA_RCSR |= (AUTODMA_RCSR_ERF);
	R_HASHDMA_RCSR |= AUTODMA_RCSR_EN;
	R_SECIE = 0;
	R_SECIE |= BIT5;	/*bit5:HASH_TRB_IE;*/

	return RET_SUCCESS;
}

/*
 * @brief: HASH auto DMA trigger
 * @param: None
 * @return: 0: Success
*/
INT32S drv_l1_hash_auto_dma_trigger(void)
{
	R_HASHDMA_RTR |= AUTODMA_TRIGGER;
	return RET_SUCCESS;
}

/*
 * @brief: Initial the RSA driver
 * @param vic_rsa: 0: use IRQ, 1: use FIQ
 * @return: 0: success, -1: fails to allocate RSA buffer
*/
INT32S drv_l1_rsa_init(INT32U vic_rsa)
{
    drv_l1_crypto_lock(osWaitForever);
    //drv_l1_crypto_clear_register();

    if (g_mod.crp_p == NULL) {
        g_mod.crp_p = (INT8U* ) gp_malloc(256); // maxmimum module length is 2048 bit
        if (g_mod.crp_p == NULL) {
            return  -1;
        }
        memset(g_mod.crp_p,0,256);
    }

    /* create a queue used for RSA completion */
    if (RSA_rxQueue == NULL) {
        osMessageQDef_t RSA_rxQueue_def = {C_RSA_RX_QUEUE_SZ, 0, 0};
        RSA_rxQueue = osMessageCreate(&RSA_rxQueue_def, NULL);
    }

    drv_l1_clock_set_system_clk_en(CLK_EN7_SECURITY_IP, 1);
    R_SEC_CTRL = 0x2; /* RSA master */
    R_SECIE |= 0x1; /* enable RSA enable interrupt */

    vic_irq_register(VIC_SEC, RSA_IRQHandler);
	vic_irq_enable(VIC_SEC);
    if (vic_rsa) {
        vic_fiq_enable(VIC_SEC);
    }
	return 0;
}

/*
 * @brief: Uninitial the RSA driver
 * @param: None
 * @return: None
*/
void drv_l1_rsa_uninit(void)
{
    if (g_mod.crp_p != NULL) {
        gp_free(g_mod.crp_p);
        g_mod.crp_p = NULL;
    }

    vic_irq_disable(VIC_SEC);
    vic_fiq_enable(VIC_SEC);

    if (RSA_rxQueue != NULL) {
        vQueueDelete(RSA_rxQueue);
        RSA_rxQueue = NULL;
    }

    R_SECIE &= ~0x1;
    R_SEC_CTRL = 0;
    drv_l1_clock_set_system_clk_en(CLK_EN7_SECURITY_IP, 0);

    drv_l1_crypto_unlock();
}

/*
 * @brief: Clear the interrupt flag
 * @param: None
 * @return: None
*/
static INT32S drv_l1_rsa_clr_if(void)
{
    INT32U secif = R_SECIF;

    R_SECIF = secif; /*Clear flag*/
    secif = R_SECIF;

	return 0;
}

/*
 * @brief: The RSA IRQ handler, when the RSA interrupt occure and the handler will send the done message
 * @param: None
 * @return: None
*/
void RSA_IRQHandler(void)
{
	drv_l1_rsa_clr_if();
	osMessagePut(RSA_rxQueue, 0, osWaitForever);
}

/*
 * @brief: Calculate the RSA Montgomery parameter for H/W
 * @param: mod: modulus
 * @return: Montgomery parameter
*/
static rsabase_t drv_l1_rsa_mont_w(kcrp_para *mod)
{
	rsabase_t t = 1;
	rsabase_t mode;
	int i;
	int lb = RSA_BASE;

	memcpy(&mode, mod->crp_p, sizeof(mode));

	for (i = 1 ; i < lb - 1; i++) {
		t = (t * t * mode);
	}
	t = -t;

	return t;
}

/*
 * @brief: Check if the modulus A is equal to modulus B
 * @param: a: modulus a
 * @param: b: modulus b
 * @return: 0:equal, -1 or 1:not equal
*/
static int drv_l1_rsa_cmp(kcrp_para *a, kcrp_para *b)
{
	if (a->crp_psize != b->crp_psize) {
		return 1;
    }
	return memcmp(a->crp_p, b->crp_p, a->crp_psize);
}

/*
 * @brief: The operation of exponent with modulus, i.e: y = x ^ e mod n
 * @param: res: The output of RSA operation
 * @param: base: The base of exponent, i.e: the x of y = x ^ e mod n
 * @param: exp: The exponent, i.e: the e of y = x ^ e mod n
 * @param: mod: The modulus, i.e: the n of y = x ^ e mod n
 * @param: p2: The internally used buffer, no need to apply data
 * @return: 0: success, -1: fail
*/
INT32S drv_l1_rsa_mod_exp(kcrp_para *res, kcrp_para *base, kcrp_para *exp, kcrp_para *mod, kcrp_para *p2)
{
    INT32U mod_bytes = 0;
    INT32U base_bytes = 0;
    osEvent result;
    INT32S ret = 0;

    mod_bytes = mod->crp_psize;
    base_bytes = base->crp_psize;

    cache_invalid_range((INT32U)res->crp_p,base_bytes);

    if (drv_l1_rsa_cmp(mod, &g_mod)) {
        /* The module value is different from previous value, so P2 must calculate again */
        rsabase_t w = drv_l1_rsa_mont_w(mod);
        R_RSAPAR3 = (INT32U) p2->crp_p;
        R_RSAPAR4 = (INT32U)w;
        R_RSAPAR5 = 0; // not use in H/W
        g_mod.crp_psize = mod->crp_psize;
        memcpy(g_mod.crp_p,mod->crp_p,mod->crp_psize);
        drv_l1_cp15_clean_dcache_range( (INT32U)g_mod.crp_p ,mod->crp_psize);
        R_RSAPAR0 = RSA_SET_PARA_D(mod_bytes * BITS_PER_BYTE) | RSA_PARA_PRECAL_P2;
    } else {
        R_RSAPAR0 = RSA_SET_PARA_D(mod_bytes * BITS_PER_BYTE) | RSA_PARA_FETCH_P2;
    }

    R_RSASPTR = (INT32U) base->crp_p;
    R_RSAYPTR = (INT32U) exp->crp_p;
    R_RSANPTR = (INT32U) mod->crp_p;
    R_RSADPTR = (INT32U) res->crp_p;

	R_RSADMACS = RSA_DMA_SIZE(mod_bytes) | RSA_DMA_ENABLE;
    result = osMessageGet(RSA_rxQueue, osWaitForever);

    if (result.status != osEventMessage) {
        drv_l1_cp15_invalidate_dcache_range((INT32U)res->crp_p,base_bytes);
        DBG_PRINT("drv_l1_rsa_mod_exp status error %d\r\n", result.status);
        return -1;
    }

    return ret;
}

void drv_l1_crypto_clear_register(void)
{
    INT32U reg_num = 0x8c>>2;
    INT32U k;

    for (k = 0; k < reg_num; k++)
        (*((volatile INT32U *)(R_CRYPTO_BASE + (k<<2)))) = 0;

    return;
}

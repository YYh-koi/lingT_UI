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
#include "drv_l1_arm_cp15.h"
#include "drv_l1_interrupt.h"
#include "drv_l2_crypto.h"
#include "gp_stdlib.h"
#include "gplib_mm_gplus.h"
#include "gplib_print_string.h"

extern osMutexId crypto_sem;
osMessageQId aes_done_q = NULL;
osMessageQId hash_done_q = NULL;

void AES_IRQHandler(void);
void HASH_IRQHandler(void);

AES_ISR_HOOK aes_isr_hook;
HASH_ISR_HOOK hash_isr_hook;

/*
 * @brief: Create the crypto trb ring.
 * @param: size: Crypto trb ring size
 * @return: Crypto trb ring address.
*/
trb_ring_t *drv_l2_crypto_trb_ring_new(INT32U size)
{
	trb_ring_t *ring;
	void *ptrb;
	trb_t *trb;

	ring = (trb_ring_t *)(gp_malloc(sizeof(trb_ring_t)) );
	ptrb = (void *)gp_malloc((((sizeof(trb_t)*size)+63)/64*64)+64);
	trb = (trb_t *)(((INT32U)ptrb+64) &(~(64-1)) );
	gp_memset((INT8S *)trb, 0, sizeof(trb_t)*size);

	ring->ptrb = ptrb;
	ring->head = ring->tail = ring->trb = trb;
	ring->pa = (INT32U)ring->head;
	ring->trb_num = size;

	size--;
	if (size) {
		/* the last trb "MUST BE" link trb */
		ring->trb[size].type = TRB_LINK;
		ring->trb[size].sptr = ring->pa;
		ring->trb[size].dptr = (INT32U)ring->trb;
		ring->trb[size].tc = 1;

		/* init normal trb */
		while (size--) {
			ring->trb[size].type = TRB_NORMAL;
			ring->trb[size].c = 0;
		}
	}

	return ring;
}

/*
 * @brief: Free the crypto trb ring.
 * @param: ring: Crypto trb ring address to be free
 * @return: None
*/
void drv_l2_crypto_trb_ring_free(trb_ring_t *ring)
{
	void *ptrb = ring->ptrb;
	gp_free(ptrb);
	gp_free(ring);
}

/*
 * @brief: Initial the AES driver
 * @param: vic_aes: 0: AES use IRQ; others: AES use FIQ
 * @return: None
*/
void drv_l2_aes_init(INT32U vic_aes)
{
    drv_l1_crypto_lock(osWaitForever);

    if (aes_done_q == NULL) {
        osMessageQDef_t aes_q = {1, sizeof(INT32U), 0};
        aes_done_q = osMessageCreate(&aes_q, NULL);
        if (aes_done_q == 0) {
            DBG_PRINT("aes_done_queue fail\r\n");
        }
    }

    drv_l1_aes_init();
    vic_irq_register(VIC_SEC, AES_IRQHandler);
    vic_irq_enable(VIC_SEC);
    if (vic_aes) {
        vic_fiq_enable(VIC_SEC);
    } else {
    	vic_fiq_disable(VIC_SEC);
    }
}

/*
 * @brief: Uninitial the AES driver
 * @param: None
 * @return: None
*/
void drv_l2_aes_uninit(void)
{
    vic_irq_disable(VIC_SEC);
    vic_fiq_disable(VIC_SEC);
    if (aes_done_q) {
        vQueueDelete(aes_done_q);
        aes_done_q = NULL;
    }
    drv_l1_aes_uninit();
    drv_l1_crypto_unlock();
}


static INT32S drv_l2_aes_trigger(AES_PARAM_t *param)
{
	INT32S ret = 0;

	/*set uid key gen enable*/
	drv_l1_aes_set_uid_key_gen_en(param->uid_key_gen_en);

	/* set dma src */
	if ((ret = drv_l1_aes_set_dma_src( (INT32U)param->src_addr))) {
		return ret;
    }

	/* set dma dst */
	if ((ret = drv_l1_aes_set_dma_dst( (INT32U)param->dst_addr))) {
		return ret;
    }

	/* set key & key length*/
	if ((ret = drv_l1_aes_set_key((INT32U)param->key_addr))) {
		return ret;
    }

	if ((ret = drv_l1_aes_set_key_len(param->key_len))) {	/* 4*32=192bit */
		return ret;
    }

	/* set mode*/
	if ((ret = drv_l1_aes_set_mode(param->AesMode))) {
		return ret;
    }

	if ((ret = param->AesMode) > AesECB) {
		if (!param->iv_addr) {
			return ret;
		} else {
			drv_l1_aes_set_iv((INT32U)param->iv_addr);
        }
	}
	/* set direct*/
	if ((ret = drv_l1_aes_set_dir(param->AesDir))) {
		return ret;
    }

	/* set dma length */
	if ((ret = drv_l1_aes_set_dma_transfer_len(param->AesDMALen))) {
		return ret;
    }

	/* set CNTR_M */
	drv_l1_aes_set_CNT_M(param->CNTR_M);

	/* enable dma */
	drv_l1_aes_dma_en();

	return RET_SUCCESS;
}

static INT32S drv_l2_aes_wait_dma_done(INT32S timeout_ticks)
{
    INT32S ret = 0;
    osEvent result;

    if (aes_done_q) {
        result = osMessageGet(aes_done_q, timeout_ticks);
        if ((result.status != osEventMessage) || (result.value.v != MSG_AES_DMA_DONE)) {
            ret = -1;
        }
    } else {
        ret = drv_l1_aes_wait_dma_done(timeout_ticks);
    }
    return ret;
}

static INT32S drv_l2_aes_wait_ring_stop(INT32S timeout_ticks)
{
    INT32S ret = 0;
    osEvent result;

    if (aes_done_q) {
        result = osMessageGet(aes_done_q, timeout_ticks);
        if ((result.status != osEventMessage) || (result.value.v != MSG_AES_AUTO_DMA_DONE)) {
            ret = -1;
        }
    } else {
        ret = drv_l1_aes_wait_ring_stop(timeout_ticks);
    }

    return ret;
}


/*
 * @brief: AES normal DMA
 * @param: param: Parameters for AES normal DMA
 * @return: 0: Success; others: Fail
*/
INT32S drv_l2_aes_dma(AES_PARAM_t *param)
{
    INT32U dma_size;
    INT32U total_len;
    INT32S ret = 0;
    INT32U dst_buf = (INT32U)param->dst_addr;
    INT8U *res_src_alloc = NULL, *res_src = NULL;

    total_len = param->data_len &(~(AES_BLOCK_SIZE - 1));

    if (param->data_len & (AES_BLOCK_SIZE-1)) {
        DBG_PRINT("WARNING: Data length %d is not multiple of aes block length(i.e. 16)!!!\r\n", param->data_len);

        if (param->AesDir == AesENCRYPT) {
            param->padding_len = AES_BLOCK_SIZE  - (param->data_len%AES_BLOCK_SIZE);
            DBG_PRINT("Padding %d bytes automatically.\r\n", param->padding_len);
        }
    }

    drv_l2_aes_init(0);

    if (param->padding_len) {
        INT32U i;

        res_src_alloc = (INT8U *)gp_malloc(AES_BLOCK_SIZE + 64);
        res_src = (INT8U *)(((INT32U)res_src_alloc+63)&(~63));

        INT32U idx0 = param->data_len&(~15);
        INT32U idx1 = param->data_len%16;

        for (i=0; i<idx1; i++) {
            res_src[i] = param->src_addr[idx0+i];
        }
        for (i=0; i<param->padding_len; i++) {
            res_src[idx1+i] = 0;
        }

        drv_l1_cp15_clean_dcache_range((INT32U)res_src, AES_BLOCK_SIZE);
    }

    param->AesDMALen = (total_len>AES_DMA_LEN_MAX)?AES_DMA_LEN_MAX:total_len;
    dma_size = param->AesDMALen;

    drv_l1_cp15_clean_dcache_range( (INT32U)param->src_addr , total_len);
    cache_invalid_range((INT32U)dst_buf, total_len);

    do {
        param->AesDMALen = dma_size;
        param->uid_key_gen_en = 0;

        if ((ret = drv_l2_aes_trigger(param))) {
            goto _aes_dma_exit;
        }
        if ((ret = drv_l2_aes_wait_dma_done(-1))) {	//-1 means no timeout;
            goto _aes_dma_exit;
        }
        param->src_addr += dma_size;
        param->dst_addr += dma_size;
        total_len -= dma_size;
        if (total_len < dma_size) {
            dma_size = total_len & (~0x0F);
        }
    } while (total_len >0);

    if (param->padding_len) {
        AES_PARAM_t res_param;

        gp_memcpy((INT8S *)(&res_param), (INT8S *)param, sizeof(AES_PARAM_t));
        res_param.AesDMALen = 16;
        res_param.src_addr = res_src;
        res_param.dst_addr = param->res;
        cache_invalid_range((INT32U)param->res, 16);

        if ((ret = drv_l2_aes_trigger(&res_param))) {
            goto _aes_dma_exit;
        }
        if ((ret = drv_l2_aes_wait_dma_done(-1))) {	//-1 means no timeout;
            goto _aes_dma_exit;
        }
        DBG_PRINT("res data aes done\r\n");
    }
_aes_dma_exit:
    if (res_src_alloc) {
        gp_free(res_src_alloc);
    }
    drv_l2_aes_uninit();
    return ret;
}


/*
 * @brief: AES auto DMA
 * @param: param: Parameters for AES auto DMA
 * @return: 0: Success; others: Fail
*/
INT32S drv_l2_aes_auto_dma(AES_PARAM_t *param)
{
    INT32S ret = 0;
    trb_ring_t *ring;
    trb_t *trb;
    INT32U total_len;
    INT32U aes_ring_size = 0;
    INT32U i;
    INT32U dst_buf = (INT32U)param->dst_addr;
    INT8U *res_src_alloc = NULL, *res_src = NULL;

    total_len = param->data_len&(~(AES_BLOCK_SIZE-1));

    if (param->key_len != 128 && param->key_len!= 192 && param->key_len != 256) {
        DBG_PRINT("ERROR: AES Key Length is not 128, 192 or 256!!\r\n");
        return -1;
    }

    if (param->data_len & (AES_BLOCK_SIZE-1)) {

        if (param->AesDir == AesENCRYPT) {
            DBG_PRINT("WARNING: Plaintext length %d is not multiple of aes block length(i.e. 16 byte)!!!\r\n", param->data_len);
            param->padding_len = AES_BLOCK_SIZE  - (param->data_len%AES_BLOCK_SIZE);
            DBG_PRINT("There is/are %d padding on the plaintext automatically.\r\n", param->padding_len);
        } else {
            DBG_PRINT("WARNING: Ciphertext length %d is not multiple of aes block length(i.e. 16 byte)!!!\r\n", param->data_len);
            DBG_PRINT("Please make sure your ciphertext or you will not get complete and correct plaintext.");
        }
    }

    drv_l2_aes_init(0);

    if (param->padding_len) {
        INT32U i;

        res_src_alloc = (INT8U *)gp_malloc(AES_BLOCK_SIZE + 64);
        res_src = (INT8U *)(((INT32U)res_src_alloc+63)&(~63));

        INT32U idx0 = param->data_len&(~15);
        INT32U idx1 = param->data_len%16;

        for (i=0; i<idx1; i++) {
            res_src[i] = param->src_addr[idx0+i];
        }
        for (i=0; i<param->padding_len; i++) {
            res_src[idx1+i] = 0;
        }

        drv_l1_cp15_clean_dcache_range((INT32U)res_src, AES_BLOCK_SIZE);
    }

    param->AesDMALen = (total_len>AES_DMA_LEN_MAX)?AES_DMA_LEN_MAX:total_len;

    drv_l1_cp15_clean_dcache_range( (INT32U)param->src_addr , total_len);
    cache_invalid_range((INT32U)dst_buf, total_len);

    if (param->padding_len) {
        cache_invalid_range((INT32U)param->res, 16);
    }

    aes_ring_size =  (total_len / param->AesDMALen) +((total_len%param->AesDMALen)?1:0)+((param->padding_len)?1:0)+ 1;

    ring = drv_l2_crypto_trb_ring_new(aes_ring_size);
    trb = ring->head;
    drv_l1_aes_auto_dma_setting(ring);

    /* Fill All TRBs */
    for (i=0; i < aes_ring_size -1; i++) {
        trb[i].ioc = (i== (aes_ring_size-2) )?1:0; /*interrupt only when the last Normal TRB(excluding Link TRB) is done*/

        if ((i == (aes_ring_size-3)) && (total_len%param->AesDMALen) && (param->padding_len) ) {
            trb[i].size = total_len % param->AesDMALen;
        } else if (i == (aes_ring_size-2) && (param->padding_len)) {
            trb[i].size = 16;
        } else if (i == (aes_ring_size-2) && !(param->padding_len) && (total_len%param->AesDMALen)) {
            trb[i].size = total_len % param->AesDMALen;
        } else {
            trb[i].size = param->AesDMALen;
        }

        if ((i == (aes_ring_size-2)) && (param->padding_len)) {
            trb[i].sptr = (INT32U)res_src;
            trb[i].dptr = (INT32U)param->res;
            cache_invalid_range((INT32U)param->res, 16);
        } else {
            trb[i].sptr = (INT32U)param->src_addr + i*param->AesDMALen;
            trb[i].dptr = (INT32U)param->dst_addr + i*param->AesDMALen;
        }

        trb[i].mode = param->AesMode|(param->AesDir<<7)|(param->key_len<<11)|(param->CNTR_M<<24);
        trb[i].iptr = (INT32U)param->iv_addr;
        trb[i].kptr = (INT32U)param->key_addr;
        trb[i].c    = ~0; /* this field must write at last */
    }

    drv_l1_cp15_clean_dcache_range( (INT32U)ring->head ,sizeof(trb_t)*aes_ring_size);
    param->uid_key_gen_en = 0;
    drv_l1_aes_set_uid_key_gen_en(param->uid_key_gen_en);
    drv_l1_aes_auto_dma_trigger();
    ret = drv_l2_aes_wait_ring_stop(-1);

    drv_l2_crypto_trb_ring_free(ring);

    if (res_src_alloc) {
        gp_free(res_src_alloc);
    }
    drv_l2_aes_uninit();
    return ret;
}


//========================================================================//
ALIGN64 const unsigned char sha256_iv[]=
{
/*	Big Endian of these values */
	BE32_TO_CPU(0x6a09e667),
	BE32_TO_CPU(0xbb67ae85),
	BE32_TO_CPU(0x3c6ef372),
	BE32_TO_CPU(0xa54ff53a),
	BE32_TO_CPU(0x510e527f),
	BE32_TO_CPU(0x9b05688c),
	BE32_TO_CPU(0x1f83d9ab),
	BE32_TO_CPU(0x5be0cd19)
};

ALIGN64 const unsigned char sha512_iv[]=
{
	BE64_TO_CPU(0x6A09E667F3BCC908),
	BE64_TO_CPU(0xBB67AE8584CAA73B),
	BE64_TO_CPU(0x3C6EF372FE94F82B),
	BE64_TO_CPU(0xA54FF53A5F1D36F1),
	BE64_TO_CPU(0x510E527FADE682D1),
	BE64_TO_CPU(0x9B05688C2B3E6C1F),
	BE64_TO_CPU(0x1F83D9ABFB41BD6B),
	BE64_TO_CPU(0x5BE0CD19137E2179)
};

/*
 * @brief: SHA256 preprocess
 * @param: buf: SHA256 source data buffer address.
 * @param: len: SHA256 source data length.
 * @return: Output length
*/
INT32U drv_l2_sha256_preprocess(INT8U *buf, INT32U len)
{
	INT64U len_64;
	INT8U *p;
	INT8U i;
	INT32U padding;
	INT32U t;
    INT8U *p_dirty_start;
    INT8U *p_dirty_end;
    INT8U *p_dirty_start_aligned;
    INT8U *p_dirty_end_aligned;
    INT32U out_len;

	p = buf + len;

	p_dirty_start = p;

	t = len%SHA256_BLOCK_SIZE;
	padding = (t<56)?(56-t):(120-t);	/*56 means 448/8; 120 means (448+512)/ 8*/

	p_dirty_end = p_dirty_start + padding + 8; // 8 is because the len_64
    out_len = len+padding+8;
	padding--;
	/*append the bit '1' to the message*/
	*p++ = 0x80;

	/*append k bits '0' ,where k is the minimum number >= 0 such that the resulting message
    length (in bits) is congruent to 448(mod 512)*/
	gp_memset((INT8S*)p, 0,padding);
	p+= padding;

	/*append length of message (before pre-processing), in bits, as 64-bit big-endian integer*/
	len_64 = (INT64U)len<<3;
	for (i=0; i<8; i++) {
		*p++ = ((INT8U *)(&len_64))[8-i-1];
	}

	p_dirty_start_aligned = (INT8U *)((INT32U)p_dirty_start & (~(64-1)));
	p_dirty_end_aligned = (INT8U *)((INT32U)p_dirty_end & (~(64-1)));
	drv_l1_cp15_clean_dcache_range( (INT32U)p_dirty_start_aligned, (p_dirty_end_aligned - p_dirty_start_aligned));

	return out_len;
}

/*
 * @brief: SHA256 calculate len with padding len added
 * @param: buf: SHA256 source data buffer address.
 * @param: len: SHA256 source data length.
 * @return: Output length
*/
INT32U drv_l2_sha256_cal_len_with_padding(INT32U len)
{
    INT32U t;
    INT32U padding;
    INT32U out_len;

	t = len%SHA256_BLOCK_SIZE;
	padding = (t<56)?(56-t):(120-t);	/*56 means 448/8; 120 means (448+512)/ 8*/
	out_len = len+padding+8;

    return out_len;
}

/*
 * @brief: SHA256 preprocess of last bulk
 * @param: buf: SHA256 source data buffer address.
 * @param: len: SHA256 source data length.
 * @return: Output length
*/
INT32U drv_l2_sha256_preprocess_last_bulk(INT8U *buf, INT32U len, INT32U len2)
{
	INT64U len_64;
	INT8U *p;
	INT8U i;
	INT32U padding;
	INT32U t;
    INT8U *p_dirty_start;
    INT8U *p_dirty_end;
    INT8U *p_dirty_start_aligned;
    INT8U *p_dirty_end_aligned;
    INT32U out_len;

	p = buf + len;

	p_dirty_start = p;

	t = len%SHA256_BLOCK_SIZE;
	padding = (t<56)?(56-t):(120-t);	/*56 means 448/8; 120 means (448+512)/ 8*/

	p_dirty_end = p_dirty_start + padding + 8; // 8 is because the len_64
    out_len = len+padding+8;
	padding--;
	/*append the bit '1' to the message*/
	*p++ = 0x80;

	/*append k bits '0' ,where k is the minimum number >= 0 such that the resulting message
    length (in bits) is congruent to 448(mod 512)*/
	gp_memset((INT8S*)p, 0,padding);
	p+= padding;

	/*append length of message (before pre-processing), in bits, as 64-bit big-endian integer*/
	len_64 = (INT64U)len2<<3;
	for(i=0; i<8; i++){
		*p++ = ((INT8U *)(&len_64))[8-i-1];
	}

	p_dirty_start_aligned = (INT8U *)((INT32U)p_dirty_start & (~(64-1)));
	p_dirty_end_aligned = (INT8U *)((INT32U)p_dirty_end & (~(64-1)));
	drv_l1_cp15_clean_dcache_range( (INT32U)p_dirty_start_aligned, (p_dirty_end_aligned - p_dirty_start_aligned));

	return out_len;
}

/*
 * @brief: SHA512 preprocess
 * @param: buf: SHA512 source data buffer address.
 * @param: len: SHA512 source data length.
 * @return: Output length
*/
INT32U drv_l2_sha512_preprocess(INT8U *buf, INT32U len)
{
	INT64U len_64;
	INT8U *p;
	INT8U i;
	INT32U padding;
	INT32U t;
    INT8U *p_dirty_start;
    INT8U *p_dirty_end;
    INT8U *p_dirty_start_aligned;
    INT8U *p_dirty_end_aligned;
    INT32U out_len;

	p = buf + len;

	p_dirty_start = p;

	t = len%SHA512_BLOCK_SIZE;
	padding = (t<112)?(112-t):(240-t);	/*112 means 896/8; 240 means (896+1024)/ 8*/

	p_dirty_end = p_dirty_start + padding + 16; // 16 is because 2 len_64
    out_len = len+padding+16;
	padding--;
	/*append the bit '1' to the message*/
	*p++ = 0x80;

	/*append k bits '0' ,where k is the minimum number >= 0 such that the resulting message
    length (in bits) is congruent to 896(mod 1024)*/
    gp_memset((INT8S*)p, 0, padding);
	p+= padding;

	/*append length of message (before pre-processing), in bits, as 64-bit big-endian integer*/
	len_64 = (INT64U)len<<3;

	for (i=0; i<8; i++) {
		*p++ = 0;
	}
	for (i=0; i<8; i++) {
		*p++ = ((INT8U *)(&len_64))[8-i-1];
	}

	p_dirty_start_aligned = (INT8U *)((INT32U)p_dirty_start & (~(64-1)));
	p_dirty_end_aligned = (INT8U *)((INT32U)p_dirty_end & (~(64-1)));
	drv_l1_cp15_clean_dcache_range( (INT32U)p_dirty_start_aligned, (p_dirty_end_aligned - p_dirty_start_aligned));

	return out_len;
}

/*
 * @brief: SHA512 calculate len with padding len added
 * @param: buf: SHA512 source data buffer address.
 * @param: len: SHA512 source data length.
 * @return: Output length
*/
INT32U drv_l2_sha512_cal_len_with_padding(INT32U len)
{
    INT32U t;
    INT32U padding;
    INT32U out_len;

	t = len%SHA512_BLOCK_SIZE;
	padding = (t<112)?(112-t):(240-t);	/*112 means 896/8; 240 means (896+1024)/ 8*/
	out_len = len+padding+16;

    return out_len;
}

/*
 * @brief: SHA512 preprocess of last bulk
 * @param: buf: SHA512 source data buffer address.
 * @param: len: SHA512 source data length.
 * @return: Output length
*/
INT32U drv_l2_sha512_preprocess_last_bulk(INT8U *buf, INT32U len, INT32U len2)
{
	INT64U len_64;
	INT8U *p;
	INT8U i;
	INT32U padding;
	INT32U t;
    INT8U *p_dirty_start;
    INT8U *p_dirty_end;
    INT8U *p_dirty_start_aligned;
    INT8U *p_dirty_end_aligned;
    INT32U out_len;

	p = buf + len;

	p_dirty_start = p;

	t = len%SHA512_BLOCK_SIZE;
	padding = (t<112)?(112-t):(240-t);	/*112 means 896/8; 240 means (896+1024)/ 8*/

	p_dirty_end = p_dirty_start + padding + 16; // 16 is because 2 len_64
	out_len = len+padding+16;
	padding--;
	/*append the bit '1' to the message*/
	*p++ = 0x80;

	/*append k bits '0' ,where k is the minimum number >= 0 such that the resulting message
    length (in bits) is congruent to 896(mod 1024)*/
    gp_memset((INT8S*)p, 0, padding);
	p+= padding;

	/*append length of message (before pre-processing), in bits, as 64-bit big-endian integer*/
	len_64 = (INT64U)len2<<3;

	for(i=0; i<8; i++){
		*p++ = 0;
	}
	for(i=0; i<8; i++){
		*p++ = ((INT8U *)(&len_64))[8-i-1];
	}

	p_dirty_start_aligned = (INT8U *)((INT32U)p_dirty_start & (~(64-1)));
	p_dirty_end_aligned = (INT8U *)((INT32U)p_dirty_end & (~(64-1)));
	drv_l1_cp15_clean_dcache_range( (INT32U)p_dirty_start_aligned, (p_dirty_end_aligned - p_dirty_start_aligned));

	return out_len;
}

/*
 * @brief: Initial the HASH driver
 * @param: vic_aes: 0: HASH use IRQ; others: HASH use FIQ
 * @return: None
*/
void drv_l2_hash_init(INT32U vic_hash)
{
    drv_l1_crypto_lock(osWaitForever);

    if (hash_done_q == NULL) {
        osMessageQDef_t hash_q = {1, sizeof(INT32U), 0};
        hash_done_q = osMessageCreate(&hash_q, NULL);
        if (hash_done_q == 0) {
            DBG_PRINT("hash_done_queue fail\r\n");
        }
    }

    drv_l1_hash_init();
	vic_irq_register(VIC_SEC, HASH_IRQHandler);
	vic_irq_enable(VIC_SEC);
    if (vic_hash) {
        vic_fiq_enable(VIC_SEC);
    } else {
    	vic_fiq_disable(VIC_SEC);
    }
}

/*
 * @brief: Uninitial the HASH driver
 * @param: None
 * @return: None
*/
void drv_l2_hash_uninit(void)
{
    vic_irq_disable(VIC_SEC);
    vic_fiq_disable(VIC_SEC);
    drv_l1_hash_uninit();
    if (hash_done_q) {
        vQueueDelete(hash_done_q);
        hash_done_q = NULL;
    }
    drv_l1_crypto_unlock();
}


static INT32S drv_l2_hash_trigger(HASH_PARAM_t *param)
{
	INT32S ret = 0;

	/* set dma src */
	if ((ret = drv_l1_hash_set_dma_src( (INT32U)param->src_addr))) {
		return ret;
    }

	/* set dma dst */
	if ((ret = drv_l1_hash_set_dma_dst( (INT32U)param->dst_addr))) {
		return ret;
    }

	/* set iv */
	if ((ret = drv_l1_hash_set_iv( (INT32U)param->iv_addr))) {
		return ret;
    }

	/* set mode */
	if ((ret = drv_l1_hash_set_mode(param->HashMode))) {
		return ret;
    }

	/* set hash out length */
	if ((ret = drv_l1_hash_set_len(param->HashOLen))) {
		return ret;
	}

	/* set dma length */
	if ((ret = drv_l1_hash_set_dma_transfer_len(param->HashDMALen))) {
		return ret;
    }

	/* enable dma */
	drv_l1_hash_dma_en();

	return RET_SUCCESS;
}

static INT32S drv_l2_hash_wait_dma_done(INT32S timeout_ticks)
{
    INT32S ret = 0;
    osEvent result;

    if (hash_done_q) {
        result = osMessageGet(hash_done_q, timeout_ticks);
        if ((result.status != osEventMessage) || (result.value.v != MSG_HASH_DMA_DONE)) {
            ret = -1;
        }
    } else {
        ret = drv_l1_hash_wait_dma_done(timeout_ticks);
    }
    return ret;
}

static INT32S drv_l2_hash_wait_ring_stop(INT32S timeout_ticks)
{
    INT32S ret = 0;
    osEvent result;

    if (hash_done_q) {
        result = osMessageGet(hash_done_q, timeout_ticks);
        if ((result.status != osEventMessage) || (result.value.v != MSG_HASH_AUTO_DMA_DONE)) {
            ret = -1;
        }
    } else {
        ret = drv_l1_hash_wait_ring_stop(timeout_ticks);
    }

    return ret;
}

/*
 * @brief: HASH normal DMA
 * @param: param: Parameters for HASH normal DMA
 * @return: 0: Success; others: Fail
*/
INT32S drv_l2_hash_dma(HASH_PARAM_t *param)
{
    INT32U ret = 0;
    INT32U total_len;
    INT32U dma_size;

	drv_l2_hash_init(0);

	/* Pre-processing: */
	if (param->HashOLen == SHA2_OUT256) {
        total_len = drv_l2_sha256_preprocess(param->src_addr, param->data_len);
        dma_size = (total_len>SHA256_DMA_LEN_MAX)?SHA256_DMA_LEN_MAX:total_len;
        drv_l1_cp15_clean_dcache_range((INT32U)param->iv_addr , SHA256_IV_SIZE);
	} else if (param->HashOLen == SHA2_OUT512) {
        total_len = drv_l2_sha512_preprocess(param->src_addr, param->data_len);
        dma_size = (total_len>SHA512_DMA_LEN_MAX)?SHA512_DMA_LEN_MAX:total_len;
        drv_l1_cp15_clean_dcache_range((INT32U)param->iv_addr , SHA512_IV_SIZE);
	} else {
        DBG_PRINT("hash type not support\r\n");
        ret = -1;
        return ret;
	}

    drv_l1_cp15_clean_dcache_range((INT32U)param->src_addr, total_len);
    cache_invalid_range((INT32U)param->dst_addr, (param->HashOLen == SHA2_OUT256)?SHA256_DIGEST_SIZE:SHA512_DIGEST_SIZE);

    param->HashDMALen = (total_len>SHA256_DMA_LEN_MAX)?SHA256_DMA_LEN_MAX:total_len;

    do {
        param->HashDMALen = dma_size;
        if ((ret = drv_l2_hash_trigger(param))) {
            break;
        }
        if ((ret = drv_l2_hash_wait_dma_done(-1))) {	//-1 means no timeout
            DBG_PRINT("hash wait dma fail\r\n");
            break;
        }
        param->src_addr += dma_size;
        total_len -= dma_size;
        if (total_len < dma_size) {
            dma_size = total_len;
        }
    } while (total_len>0);

    drv_l1_cp15_invalidate_dcache_range((INT32U)param->dst_addr, (param->HashOLen == SHA2_OUT256)?SHA256_DIGEST_SIZE:SHA512_DIGEST_SIZE);

    drv_l2_hash_uninit();
    return ret;
}

INT32S drv_l2_hash_begin_dma(INT8U *iv_buf, INT8U hashOLen)
{
    INT32U ret = 0;

	drv_l2_hash_init(0);

	if(hashOLen == SHA2_OUT256) {
        gp_memcpy((INT8S*)iv_buf, (INT8S *)sha256_iv, SHA256_IV_SIZE);
        drv_l1_cp15_clean_dcache_range((INT32U)iv_buf, SHA256_IV_SIZE);
    }
    else if(hashOLen == SHA2_OUT512) {
        gp_memcpy((INT8S*)iv_buf, (INT8S *)sha512_iv, SHA512_IV_SIZE);
        drv_l1_cp15_clean_dcache_range((INT32U)iv_buf, SHA512_IV_SIZE);
    }

    return ret;
}

/*
 * @brief: HASH continue dma
 * @param: param: Parameters for HASH normal DMA
 * @param: end: 1 when last bulk data
 * @return: 0: Success; others: Fail
*/
INT32S drv_l2_hash_continue_dma(HASH_PARAM_t *param)
{
    INT32U ret = 0;
    INT32U data_len;
    INT32U dma_size;
    INT32U sha_block_size;

	data_len = param->data_len;
	if(param->HashOLen == SHA2_OUT256) {
        dma_size = SHA256_DMA_LEN_MAX;
        sha_block_size = SHA256_BLOCK_SIZE;
    }
    else if(param->HashOLen == SHA2_OUT512) {
        dma_size = SHA512_DMA_LEN_MAX;
        sha_block_size = SHA512_BLOCK_SIZE;
    }
    else
        // illegal HashOLen
        return -1;

    // this api require data_len must multiple of dma_size
    if ((data_len % sha_block_size) != 0) {
        DBG_PRINT("data_len not multiple of sha_block_size, %u, %u\r\n", data_len, sha_block_size);
        return -2;
    }

    //dma_size = sha_block_size; // TEST
    if (dma_size > data_len)
        dma_size = data_len;

    drv_l1_cp15_clean_dcache_range((INT32U)param->src_addr, data_len);
    cache_invalid_range((INT32U)param->dst_addr, (param->HashOLen == SHA2_OUT256)?SHA256_DIGEST_SIZE:SHA512_DIGEST_SIZE);

    do{
        param->HashDMALen = dma_size;
        if((ret = drv_l2_hash_trigger(param))){
            break;
        }
        if((ret = drv_l2_hash_wait_dma_done(-1))){	//-1 means no timeout
            DBG_PRINT("hash wait dma fail\r\n");
            break;
        }
        param->src_addr += dma_size;
        data_len -= dma_size;
    }while(data_len>0);

    drv_l1_cp15_invalidate_dcache_range((INT32U)param->dst_addr, (param->HashOLen == SHA2_OUT256)?SHA256_DIGEST_SIZE:SHA512_DIGEST_SIZE);

    return ret;
}

INT32S drv_l2_hash_end_dma(HASH_PARAM_t *param, INT32U total_data_len)
{
    INT32U ret = 0;
    INT32U data_len;
    INT32U data_len_with_padding;
    INT32U total_data_len_with_padding;
    INT32U dma_size = SHA256_DMA_LEN_MAX;
    INT32U padding_len;
    INT32U sha_block_size;

    data_len = param->data_len;
	if(param->HashOLen == SHA2_OUT256) {
        dma_size = SHA256_DMA_LEN_MAX;
        sha_block_size = SHA256_BLOCK_SIZE;
        total_data_len_with_padding = drv_l2_sha256_cal_len_with_padding(total_data_len);
    }
    else if(param->HashOLen == SHA2_OUT512) {
        dma_size = SHA512_DMA_LEN_MAX;
        sha_block_size = SHA512_BLOCK_SIZE;
        total_data_len_with_padding = drv_l2_sha512_cal_len_with_padding(total_data_len);
    }
    else
        return -1;

    sha_block_size = (volatile INT32U)sha_block_size; // just to fix compile warning

    //dma_size = sha_block_size; // TEST
    padding_len = total_data_len_with_padding - total_data_len;
    data_len_with_padding = data_len + padding_len;

    if (dma_size > data_len_with_padding)
        dma_size = data_len_with_padding;

    drv_l1_cp15_clean_dcache_range((INT32U)param->src_addr, data_len_with_padding);
    cache_invalid_range((INT32U)param->dst_addr, (param->HashOLen == SHA2_OUT256)?SHA256_DIGEST_SIZE:SHA512_DIGEST_SIZE);

    do{
        if (dma_size == data_len_with_padding) {
            if(param->HashOLen == SHA2_OUT256)
                drv_l2_sha256_preprocess_last_bulk(param->src_addr, data_len_with_padding - padding_len, total_data_len);
            else if(param->HashOLen == SHA2_OUT512)
                drv_l2_sha512_preprocess_last_bulk(param->src_addr, data_len_with_padding - padding_len, total_data_len);
        }

        param->HashDMALen = dma_size;
        if((ret = drv_l2_hash_trigger(param))){
            break;
        }
        if((ret = drv_l2_hash_wait_dma_done(-1))){	//-1 means no timeout
            DBG_PRINT("hash wait dma fail\r\n");
            break;
        }

        param->src_addr += dma_size;
        data_len_with_padding -= dma_size;
        if(data_len_with_padding > 0 && data_len_with_padding < dma_size) {
            dma_size = data_len_with_padding;
        }
    }while(data_len_with_padding>0);

    drv_l1_cp15_invalidate_dcache_range((INT32U)param->dst_addr, (param->HashOLen == SHA2_OUT256)?SHA256_DIGEST_SIZE:SHA512_DIGEST_SIZE);

    drv_l2_hash_uninit();

    return ret;
}

/*
 * @brief: HASH auto DMA
 * @param: param: Parameters for HASH auto DMA
 * @return: 0: Success; others: Fail
*/
INT32S drv_l2_hash_auto_dma(HASH_PARAM_t *param)
{
    INT32U ret = 0;
    trb_ring_t *ring;
    trb_t *trb;
    INT32U hash_ring_size = 0;
    INT32U total_len;
    INT32U dma_size;
    INT32U i;

    drv_l2_hash_init(0);

	/* Pre-processing: */
	if (param->HashOLen == SHA2_OUT256) {
        total_len = drv_l2_sha256_preprocess(param->src_addr, param->data_len);
        dma_size = (total_len>SHA256_DMA_LEN_MAX)?SHA256_DMA_LEN_MAX:total_len;
        drv_l1_cp15_clean_dcache_range((INT32U)param->iv_addr , SHA256_IV_SIZE);
	} else if (param->HashOLen == SHA2_OUT512) {
        total_len = drv_l2_sha512_preprocess(param->src_addr, param->data_len);
        dma_size = (total_len>SHA512_DMA_LEN_MAX)?SHA512_DMA_LEN_MAX:total_len;
        drv_l1_cp15_clean_dcache_range((INT32U)param->iv_addr , SHA512_IV_SIZE);
	} else {
        DBG_PRINT("hash type not support\r\n");
        ret = -1;
        return ret;
	}
    drv_l1_cp15_clean_dcache_range((INT32U)param->src_addr, total_len);
    cache_invalid_range((INT32U)param->dst_addr, (param->HashOLen == SHA2_OUT256)?SHA256_DIGEST_SIZE:SHA512_DIGEST_SIZE);

    param->HashDMALen = dma_size;
    hash_ring_size = total_len/param->HashDMALen + ((total_len%param->HashDMALen)?1:0) + 1;

    /*create trb ring*/
    ring = drv_l2_crypto_trb_ring_new(hash_ring_size);
    trb = ring->head;
    drv_l1_hash_auto_dma_setting(ring);

	/* Fill All TRBs */
	for (i=0; i<hash_ring_size-1; i++) {
		trb[i].ioc = (i== (hash_ring_size-2) )?1:0;

        if ( i== (hash_ring_size-2) &&  (total_len % param->HashDMALen)) {
            trb[i].size = total_len % param->HashDMALen;
        } else {
            trb[i].size = param->HashDMALen;
        }

		trb[i].sptr = (INT32U)param->src_addr + i*param->HashDMALen;
		trb[i].dptr = (INT32U)param->dst_addr;
		trb[i].mode = param->HashMode|(param->HashOLen<<16);
		trb[i].iptr = (INT32U)param->iv_addr;

		trb[i].c    = ~0; /* this field must write at last */
	}
    drv_l1_cp15_clean_dcache_range( (INT32U)ring->head ,sizeof(trb_t)*hash_ring_size);

    drv_l1_hash_auto_dma_trigger();
    ret = drv_l2_hash_wait_ring_stop(-1);
    drv_l2_crypto_trb_ring_free(ring);

    drv_l1_cp15_invalidate_dcache_range((INT32U)param->dst_addr, (param->HashOLen == SHA2_OUT256)?SHA256_DIGEST_SIZE:SHA512_DIGEST_SIZE);

    drv_l2_hash_uninit();

    return ret;
}

/*
 * @brief: AES isr hook register
 * @param: cbk_func: AES isr hook callback function
 * @return: 0: Success; 0xFF: Fail
*/
INT32S drv_l2_aes_isr_hook_register(AES_ISR_HOOK cbk_func)
{
	if (cbk_func) {
		aes_isr_hook = cbk_func;
		return RET_SUCCESS;
	}

	return CRYPTO_FAIL;
}


void AES_IRQHandler(void)
{
    INT32U msg;
    INT32U secif = drv_l1_crypto_get_dma_if();
    INT32U flag;

	drv_l1_aes_clr_dma_if();

	if (aes_done_q) {
        if (secif & AES_DMA_IF) {
            msg = MSG_AES_DMA_DONE;
            osMessagePut(aes_done_q, (uint32_t)&msg, osWaitForever);
        } else if (secif & AES_TRB_IF) {
            msg = MSG_AES_AUTO_DMA_DONE;
            osMessagePut(aes_done_q, (uint32_t)&msg, osWaitForever);
        }
	}

	flag = secif &  (AES_TRB_IF | AES_ERF_IF | AES_DMA_IF | AES_CMD_RD_IF);
	if (flag && aes_isr_hook ) {
		aes_isr_hook(flag);
	}
}

/*
 * @brief: HASH isr hook register
 * @param: cbk_func: HASH isr hook callback function
 * @return: 0: Success; 0xFF: Fail
*/
INT32S drv_l2_hash_isr_hook_register(HASH_ISR_HOOK cbk_func)
{
	if (cbk_func) {
		hash_isr_hook = cbk_func;
		return RET_SUCCESS;
	}

	return CRYPTO_FAIL;
}

void HASH_IRQHandler(void)
{
    INT32U msg;
    INT32U secif = drv_l1_crypto_get_dma_if();
    INT32U flag;

	drv_l1_hash_clr_dma_if();
	if (hash_done_q) {
        if (secif & HASH_DMA_IF) {
            msg = MSG_HASH_DMA_DONE;
            osMessagePut(hash_done_q, (uint32_t)&msg, osWaitForever);
        } else if (secif & HASH_TRB_IF) {
            msg = MSG_HASH_AUTO_DMA_DONE;
            osMessagePut(hash_done_q, (uint32_t)&msg, osWaitForever);
        }
	}

	flag = secif &  (HASH_TRB_IF | HASH_ERF_IF | HASH_DMA_IF | HASH_CMD_RD_IF);
	if (flag && hash_isr_hook ) {
		hash_isr_hook(flag);
	}
}

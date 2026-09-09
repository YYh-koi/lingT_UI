#ifndef __DRV_L2_CRYPTO_H__
#define __DRV_L2_CRYPTO_H__

#include "drv_l1.h"

typedef struct AES_PARAM_s{
	INT8U  *src_addr;
	INT8U  *dst_addr;
	INT8U  *iv_addr;
	INT8U  *key_addr;
	INT32U  data_len;       /*Byte*/
	INT32U	AesDMALen;
	INT32U	key_len;
	INT8U	AesMode;
	INT8U	AesDir;
	INT8U	CNTR_M;
	INT8U   uid_key_gen_en;
	INT8U   padding_len;
	ALIGN64 INT8U   res[64];
}AES_PARAM_t;

typedef struct HASH_PARAM_s{
	INT8U *src_addr;
	INT8U *dst_addr;
	INT8U *iv_addr;
	INT8U *subkey_addr;
	INT32U  data_len;       /*Byte*/
	INT32U	HashDMALen;
	INT8U	HashOLen;
	INT8U	HashMode;
}HASH_PARAM_t;

typedef void (*AES_ISR_HOOK)(INT32U);
typedef void (*HASH_ISR_HOOK)(INT32U);

#define BE32_TO_CPU(x) (((x) >> 24) & 0xFF), (((x) >> 16) & 0xFF),(((x) >> 8) & 0xFF),((x) & 0xFF)
#define BE64_TO_CPU(x) (((x) >> 56) & 0xFF), (((x) >> 48) & 0xFF),(((x) >> 40) & 0xFF),(((x) >> 32) & 0xFF),(((x) >> 24) & 0xFF), (((x) >> 16) & 0xFF),(((x) >> 8) & 0xFF),((x) & 0xFF)

/*Constants*/
#define AES_BLOCK_SIZE			16  /*i.e. 128bit*/
#define AES_DMA_LEN_MAX         65504   /*65536-32*/

#define SHA256_BLOCK_SIZE		64
#define SHA256_DIGEST_SIZE		32
#define SHA256_IV_SIZE			32

#define SHA512_BLOCK_SIZE		128
#define SHA512_DIGEST_SIZE		64
#define SHA512_IV_SIZE			64

#define SHA256_DMA_LEN_MAX      (65536-SHA256_BLOCK_SIZE)
#define SHA512_DMA_LEN_MAX      (65536-SHA512_BLOCK_SIZE)

#define MSG_AES_DMA_DONE            0x5A
#define MSG_AES_AUTO_DMA_DONE       0x5B
#define MSG_HASH_DMA_DONE           0xA5
#define MSG_HASH_AUTO_DMA_DONE      0xA6


extern trb_ring_t *drv_l2_crypto_trb_ring_new(INT32U size);
extern void drv_l2_crypto_trb_ring_free(trb_ring_t *ring);

extern void drv_l2_aes_init(INT32U vic_aes);
extern INT32S drv_l2_aes_dma(AES_PARAM_t *param);
extern INT32S drv_l2_aes_auto_dma(AES_PARAM_t *param);
extern INT32S drv_l2_aes_isr_hook_register(AES_ISR_HOOK cbk_func);
extern void drv_l2_aes_uninit(void);

extern void drv_l2_hash_init(INT32U vic_has);
extern void drv_l2_hash_uninit(void);
extern INT32U drv_l2_sha256_preprocess(INT8U *buf, INT32U len);
extern INT32U drv_l2_sha256_cal_len_with_padding(INT32U len);
extern INT32U drv_l2_sha256_preprocess_last_bulk(INT8U *buf, INT32U len, INT32U len2);
extern INT32U drv_l2_sha512_preprocess(INT8U *buf, INT32U len);
extern INT32U drv_l2_sha512_cal_len_with_padding(INT32U len);
extern INT32U drv_l2_sha512_preprocess_last_bulk(INT8U *buf, INT32U len, INT32U len2);
extern INT32S drv_l2_hash_dma(HASH_PARAM_t *param);
extern INT32S drv_l2_hash_begin_dma(INT8U *iv_buf, INT8U hashOLen);
extern INT32S drv_l2_hash_continue_dma(HASH_PARAM_t *param);
extern INT32S drv_l2_hash_end_dma(HASH_PARAM_t *param, INT32U total_data_len);
extern INT32S drv_l2_hash_auto_dma(HASH_PARAM_t *param);
extern const unsigned char sha256_iv[];
extern const unsigned char sha512_iv[];
#endif

#ifndef __DRV_L1_CRYPTO_H__
#define __DRV_L1_CRYPTO_H__

#include "drv_l1.h"
#include "drv_l1_dma.h"
#include "drv_l1_sfr.h"
#include "cmsis_os.h"
#include "gplib_mm_gplus.h"

#define RET_SUCCESS			0
#define DMA_LEN_ERR			-1
#define DMA_NOT_FINISHED	-2
#define DMA_SRC_ADDR_ERR	-3
#define DMA_DST_ADDR_ERR	-4
#define MODE_ERR			-5
#define DIR_ERR				-6
#define KEY_LEN_ERR			-7
#define IV_ERR				-8
#define KEY_ERR				-9
#define WAIT_TIMEOUT		-10
#define RING_FAIL			-11
#define CRYPTO_FAIL			0xFF

#define DISABLE			0
#define ENABLE			1

#define AES_CMD_RD_IF           (1 << 8)
#define HASH_CMD_RD_IF          (1 << 7)
#define AES_TRB_IF              (1 << 6)
#define HASH_TRB_IF             (1 << 5)
#define AES_ERF_IF              (1 << 4)
#define HASH_ERF_IF             (1 << 3)
#define AES_DMA_IF              (1 << 2)
#define HASH_DMA_IF             (1 << 1)
#define RSA_DMA_IF              (1 << 0)

#define TRB_NORMAL              (1)
#define TRB_LINK                (2)

#define AUTODMA_RCSR_EN         (1 << 31)
#define AUTODMA_RCSR_ERF        (1 << 30)

#define AUTODMA_TRIGGER         (1)
#define TRB_SIZE                sizeof(trb_t)

#define M_ENC           (0 << 7)        /* 0: encrypt */
#define M_DEC           (1 << 7)        /* 1: decrypt */


#define RSA_BASE                32

#if RSA_BASE == 32
typedef INT32S   rsabase_t;
#elif RSA_BASE == 64
typedef INT64S   rsabase_t;
#else
#error "Unsupport RSA_BASE number!"
#endif

#define BITS_PER_REG            (32)
#define RSA_DMA_ENABLE          (1 << 0)
#define RSA_DMA_IF              (1 << 0)
#define RSA_DMA_SIZE(x)         (x << 16)
#define RSA_SET_PARA_D(x)       ((x) << 16)
#define RSA_PARA_PRECAL_P2      (0 << 7)
#define RSA_PARA_FETCH_P2       (1 << 7)
#define BITS_PER_BYTE	        (8)
#define MAX_RSA_BITS	        (2048)
#define MAX_RSA_BYTES	        (MAX_RSA_BITS / BITS_PER_BYTE)


#define RSA_BASE_MASK           ((1 << (RSA_BASE - 2)) - 1)

enum{
	AesECB = 0,
	AesCBC,
	AesCTR,
	AesMODE_MAX
};

typedef enum{
	KeyLen_128b = 4,
	KeyLen_192b = 6,
	KeyLen_256b = 8,
	KeyLenMax
}AESKeyLen;

enum{
	AesENCRYPT = 0,
	AesDECRYPT,
	AesDirMax
};

enum{
	SHA2_OUT256 = 0,
	SHA2_OUT512,
	SHA2_OUTMAX
};

enum{
	SHA3_OUT224=0,
	SHA3_OUT256,
	SHA3_OUT384,
	SHA3_OUT512,
	SHA3_OUTMAX
};

enum{
	HASH_MD5 = 0,
	HASH_SHA3,
	HASH_GHASH,
	HASH_SHA2,
	POLY1305,
	HASH_MODE_MAX
};

typedef struct trb_s {
	/* Cycle bits. indicates the current cycle of the ring */
	INT32U c;

	/* Completion Code. only use in event trb
	 *   0 Invalid Indicates this field has not been updated
	 *   1 Success indicates the transfer is successfully completed
	 */
	INT32U cc:1;

	/* Toggle Cycle bit. Used in link TRB only.
	 *   indicates the cycle bits will be toggle in trb_next segment.
	 */
	INT32U tc:1;

	/* Interrupt On Complete.
	 *   when this bit is set, controller will set an interrupt
	 *   after this TRB is transmitted.
	 */
	INT32U ioc:1;

	INT32U rsv1:1;

	/* TRB type:
	 *   0x1: Normal. Normal TRB used in trb_ring.
	 *   0x2: Link. Link TRB to link to trb_ring segments
	 */
	INT32U type:4;

	INT32U rsv2:8;

	/*  Plain text size in bytes.
	 * indicates the read/write data bytes of this TRB.
	 */
	INT32U size:16;

	/* For link TRB indicates the trb_next segment address.
	 * or  Source data pointer(depend on ENDC)
	 */
	INT32U sptr;

	/* Destination data pointer (depend on ENDC)
	 */
	INT32U dptr;

	/* Parameter
	 */
	INT32U mode;
	INT32U iptr; /* Initial Vector/Counter (IV/ICB) pointer */
	INT32U kptr; /* AES/GHASH: Key/Sub-Key pointer */
	void *priv;

}trb_t;

typedef struct trb_ring_s {
	void *ptrb;
	trb_t *trb;
	INT32U pa;
	INT32U trb_num;

	/* get trb at tail, put trb at head */
	trb_t *head;
	trb_t *tail;

} trb_ring_t;

typedef struct kcrp_para {
	INT8U   *crp_p;      /* buffer pointer */
	INT32U  crp_psize;   /* buffer size (unit:byte) */
	INT32U  crp_nbits;   /* data size (unit:bit) */
}kcrp_para;

/** AES Driver for New Crypto IP**/
extern INT32S drv_l1_crypto_lock_alloc(void);
extern void drv_l1_crypto_lock_free(void);
extern INT32S drv_l1_crypto_lock(INT32U millisec);
extern void drv_l1_crypto_unlock(void);
extern INT32U drv_l1_crypto_get_dma_if(void);

extern void drv_l1_aes_set_uid_key_gen_en(INT32U en);
extern void drv_l1_aes_init(void);
extern void drv_l1_aes_uninit(void);
extern INT32S drv_l1_aes_auto_dma_trigger(void);
extern INT32S drv_l1_aes_set_dma_transfer_len(INT32U len);
extern INT32S drv_l1_aes_dma_en(void);
extern INT32S drv_l1_aes_set_dma_src(INT32U src);
extern INT32S drv_l1_aes_set_dma_dst(INT32U dst);
extern INT32S drv_l1_aes_set_mode(INT32U mode);
extern INT32S drv_l1_aes_set_dir(INT32U dir);
extern INT32S drv_l1_aes_set_key_len(INT32U key_len);
extern INT32S drv_l1_aes_set_iv(INT32U iv);
extern INT32S drv_l1_aes_set_key(INT32U key);
extern INT32S drv_l1_aes_set_CNT_M(INT32U cnt);
extern INT32S drv_l1_aes_wait_dma_done(INT32S timeout_ticks);
extern INT32S drv_l1_aes_dma_ie(INT32U enable);
extern INT32S drv_l1_aes_clr_dma_if(void);
extern INT32S drv_l1_aes_auto_dma_setting(trb_ring_t *ring);
extern INT32S drv_l1_aes_wait_ring_stop(INT32S timeout_ticks);

/*** HASH driver ***/
extern void drv_l1_hash_init(void);
extern void drv_l1_hash_uninit(void);
extern INT32S drv_l1_hash_set_dma_transfer_len(INT32U len);
extern INT32S drv_l1_hash_dma_en(void);
extern INT32S drv_l1_hash_set_dma_src(INT32U src);
extern INT32S drv_l1_hash_set_dma_dst(INT32U dst);
extern INT32S drv_l1_hash_set_iv(INT32U iv);
extern INT32S drv_l1_hash_set_mode(INT32U mode);
extern INT32S drv_l1_hash_set_len(INT32U hash_len);
extern INT32S drv_l1_hash_clr_dma_if(void);
extern INT32S drv_l1_hash_dma_ie(INT32U enable);
extern INT32S drv_l1_hash_wait_dma_done(INT32S timeout_ticks);
extern INT32S drv_l1_hash_auto_dma_setting(trb_ring_t *ring);
extern INT32S drv_l1_hash_auto_dma_trigger(void);
extern INT32S drv_l1_hash_wait_ring_stop(INT32S timeout_ticks);

/*** RSA driver ***/
extern INT32S drv_l1_rsa_init(INT32U vic_rsa);
extern void drv_l1_rsa_uninit(void);
extern INT32S drv_l1_rsa_mod_exp(kcrp_para *res, kcrp_para *base, kcrp_para *exp, kcrp_para *mod, kcrp_para *p2);
extern void drv_l1_crypto_clear_register(void);

#endif

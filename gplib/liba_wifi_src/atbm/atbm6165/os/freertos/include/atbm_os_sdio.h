#ifndef ATBM_OS_SDIO_H
#define ATBM_OS_SDIO_H
#include "atbm_sysops.h"

//#include "atbm_hal.h"
#ifdef ATBM_GENERALPLUS_SDIO
#undef ATBM_GENERALPLUS_SDIO
#endif
#define ATBM_GENERALPLUS_SDIO 1
#if ATBM_GENERALPLUS_SDIO
#include "atbm_gp_sdio_card.h"

#ifdef time_after
#undef time_after
#endif
#ifdef time_before
#undef time_before
#endif
#ifdef time_after_eq
#undef time_after_eq
#endif
#ifdef time_before_eq
#undef time_before_eq
#endif

typedef void sdio_irq_handler_t(struct sdio_func *func);

#define atbm_sdio_func      sdio_func

struct os_sdio_device_id {
	unsigned char function;
	unsigned short manufacturer;
	unsigned short product;
};

struct os_sdio_driver {
	const char *name;
	struct os_sdio_device_id *id;
	int (*probe)(struct mmc_card *card);
	int (*remove)(struct mmc_card *card);
};
#else
#include "mmcsd_card.h"
#include "sdio.h"

#define atbm_sdio_func      sdio_func
#define os_sdio_driver      sdio_driver_t
#define os_sdio_device_id   sdio_device_id
#endif


struct atbm_sdio_device_id{
	char id[8];
};
struct atbm_sdio_driver {
	char name[16];
	struct atbm_sdio_device_id *match_id_table;
	int (*probe_func) (struct atbm_sdio_func *func,	const struct atbm_sdio_device_id *id);
	int (*discon_func) (struct atbm_sdio_func *func);
};

int atbm_sdio_register(struct os_sdio_driver *sdio_driver);
void atbm_sdio_deregister(struct os_sdio_driver *sdio_driver);
void atbm_sdio_claim_host(struct sdio_func *func);
void atbm_sdio_release_host(struct sdio_func *func);
atbm_int32 atbm_sdio_enable_func(struct sdio_func *func);
void atbm_sdio_disable_func(struct sdio_func *func);
void atbm_sdio_set_drvdata(struct sdio_func *func,void *priv);
void *atbm_sdio_get_drvdata(struct sdio_func *func);
int atbm_sdio_claim_irq(struct sdio_func *func,sdio_irq_handler_t *handle);
int atbm_sdio_release_irq(struct sdio_func *func);
int __atbm_sdio_memcpy_fromio(struct sdio_func *func,void *dst,unsigned int addr,int count);
int __atbm_sdio_memcpy_toio(struct sdio_func *func,unsigned int addr,void *dst,int count);
unsigned char atbm_sdio_f0_readb(struct sdio_func *func,unsigned int addr,int *retval);
void atbm_sdio_f0_writeb(struct sdio_func *func,unsigned char regdata,unsigned int addr,int *retval);
int atbm_sdio_set_blocksize(struct sdio_func *func,int blocksize);
atbm_uint32 atbm_sdio_alignsize(struct sdio_func *func,atbm_uint32 size);
struct sdio_func *atbm_sdio_check_init_func4(struct sdio_func *func);
int atbm_sdio_register_init();
int atbm_sdio_register_deinit();
void atbm_sdio_host_enable_irq(int enable);


#endif/* ATBM_OS_SDIO_H */

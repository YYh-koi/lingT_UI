#ifndef __ATBM_GP_SDIO_CARD_H__
#define __ATBM_GP_SDIO_CARD_H__

/**************************************************************************
 *                         H E A D E R   F I L E S                        *
**************************************************************************/
#include <stdbool.h>
#include "typedef.h"
#include "cmsis_os.h"

/*
 * The complete SDK may include the legacy RTK SDIO card header before this
 * private ATBM header. Keep the copied Linux-style tag names source-compatible
 * while giving ATBM distinct C types in that mixed-vendor translation unit.
 */
#define sdio_func_tuple atbm_gp_sdio_func_tuple
#define sdio_func atbm_gp_sdio_func
#define sdio_cccr atbm_gp_sdio_cccr
#define sdio_cis atbm_gp_sdio_cis
#define mmc_host atbm_gp_mmc_host
#define mmc_card atbm_gp_mmc_card

/**************************************************************************
*                           C O N S T A N T S                             *
**************************************************************************/
#define CONFIG_READ_CIS

/**************************************************************************
*                          D A T A    T Y P E S                           *
**************************************************************************/
#ifdef CONFIG_READ_CIS
/*
 * SDIO function CIS tuple (unknown to the core)
 */
struct sdio_func_tuple {
	struct sdio_func_tuple *next;
	unsigned char code;
	unsigned char size;
	unsigned char data[0];
};
#endif

/*
 * SDIO function devices
 */
struct sdio_func {
	struct mmc_card		*card;		/* the card this device belongs to */
	void	(*irq_handler)(struct sdio_func *); /* IRQ callback */

	unsigned	int	max_blksize;	/* maximum block size */ //add
	unsigned	int	cur_blksize;	/* current block size */	 //add
	unsigned	int	enable_timeout;	/* max enable timeout in msec */ //add
	unsigned int	num;		/* function number *///add
	unsigned short		vendor;		/* vendor id */ //add
	unsigned short		device;		/* device id */ //add
	unsigned		num_info;	/* number of info strings */ //add
	const char		**info;		/* info strings */ //add
	unsigned char		class;		/* standard interface class *///add

	unsigned int			tmpbuf_reserved; //for tmpbuf 4 byte alignment
	unsigned char			tmpbuf[4];	/* DMA:able scratch buffer */

#ifdef CONFIG_READ_CIS
	struct sdio_func_tuple *tuples;
#endif
	void *drv_priv;
};

struct sdio_cccr {
	unsigned int		sdio_vsn;
	unsigned int		sd_vsn;
	unsigned int		multi_block:1;
	unsigned int		low_speed:1;
	unsigned int		wide_bus:1;
	unsigned int		high_power:1;
	unsigned int		high_speed:1;
	unsigned int		disable_cd:1;
};

struct sdio_cis {
	unsigned short		vendor;
	unsigned short		device;
	unsigned short		blksize;
	unsigned int		max_dtr;
};

struct mmc_host {
    int			        device_id;      /*!< @brief Index of SD controller. */
    unsigned int		present;		/*!< @brief Card present (initial or not).*/
    osThreadId          sdio_irq_thread_id;
};

struct mmc_card {
	struct mmc_host		*host;		/* the host this device belongs to */
	struct sdio_cccr		cccr;		/* common card info */
	struct sdio_cis		cis;		/* common tuple info */ //add
	struct sdio_func	*sdio_func[7]; /* SDIO functions (devices) *///add
	unsigned int		sdio_funcs;	/* number of SDIO functions *///add

	unsigned int			rca;			/* relative card address of device */
	unsigned int			type;		/* card type */

	unsigned		num_info;	/* number of info strings *///add
	const char		**info;		/* info strings *///add
#ifdef CONFIG_READ_CIS
	struct sdio_func_tuple	*tuples;	/* unknown common tuples *///add
#endif
};

extern void atbm_gp_sdio0_irq_handler(void);
extern void atbm_gp_sdio1_irq_handler(void);

extern INT32S atbm_gp_sdio_host_mutex_init(void);
extern void atbm_gp_sdio_host_mutex_deinit(void);
extern void atbm_gp_sdio_claim_host(struct sdio_func *func);
extern int atbm_gp_sdio_memcpy_fromio(struct sdio_func *func, void *dst,unsigned int addr, int count);
extern void atbm_gp_sdio_release_host(struct sdio_func *func);
extern int atbm_gp_sdio_memcpy_toio(struct sdio_func *func, unsigned int addr,void *src, int count);
extern unsigned char atbm_gp_sdio_readb(struct sdio_func *func, unsigned int addr, int *err_ret);
extern unsigned short atbm_gp_sdio_readw(struct sdio_func *func, unsigned int addr, int *err_ret);
extern unsigned int atbm_gp_sdio_readl(struct sdio_func *func, unsigned int addr, int *err_ret);
extern void atbm_gp_sdio_writeb(struct sdio_func *func, unsigned char b,unsigned int addr, int *err_ret);
extern void atbm_gp_sdio_writew(struct sdio_func *func, unsigned short b,unsigned int addr, int *err_ret);
extern void atbm_gp_sdio_writel(struct sdio_func *func, unsigned int b,unsigned int addr, int *err_ret);
#endif

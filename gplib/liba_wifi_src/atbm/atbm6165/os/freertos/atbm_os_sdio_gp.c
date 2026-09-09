#include "atbm_hal.h"
#include "board_config.h"
#include "drv_l1_sdc.h"
#include "atbm_gp_sdio_l2.h"

#if ATBM_GENERALPLUS_SDIO

#define ATBM_SDIO_FUNCTION_NUM 1
#define ATBM_SDIO_CMD_FUNCTION_NUM 4
#define ATBM_SDIO_ENABLE_TIMEOUT_MS 100

extern int atbm_sdio_probe(struct atbm_sdio_func *func, const struct atbm_sdio_device_id *id);
extern int atbm_sdio_disconnect(struct atbm_sdio_func *func);
static struct sdio_func *g_atbm_sdio_func;

static const struct os_sdio_device_id atbm_sdio_ids[] = {
	{1, 0x007a, 0x6011},
	{1, 0x007a, 0x8893},
	{1, 0x007a, 0x8895},
};

static const struct os_sdio_device_id *atbm_gp_match_func(struct sdio_func *func)
{
	unsigned int i;

	if(func == ATBM_NULL)
	{
		return ATBM_NULL;
	}

	for(i = 0; i < ATBM_ARRAY_SIZE(atbm_sdio_ids); i++)
	{
		if((atbm_sdio_ids[i].function == func->num) &&
		   (atbm_sdio_ids[i].manufacturer == func->vendor) &&
		   (atbm_sdio_ids[i].product == func->device))
		{
			return &atbm_sdio_ids[i];
		}
	}

	return ATBM_NULL;
}

void atbm_sdio_host_enable_irq(int enable)
{
	if(g_atbm_sdio_func && g_atbm_sdio_func->card)
	{
		atbm_gp_sdio_irq_en(g_atbm_sdio_func->card, enable ? 1 : 0);
	}
}

void atbm_sdio_claim_host(struct sdio_func *func)
{
	atbm_gp_sdio_claim_host(func);
}

void atbm_sdio_release_host(struct sdio_func *func)
{
	atbm_gp_sdio_release_host(func);
}

int atbm_sdio_claim_irq(struct sdio_func *func, sdio_irq_handler_t *handle)
{
	int ret;

	if(func)
	{
		g_atbm_sdio_func = func;
	}

	ret = atbm_gp_sdio_claim_irq(func, handle);
	if(ret == 0)
	{
		atbm_sdio_host_enable_irq(1);
	}
	return ret;
}

int atbm_sdio_release_irq(struct sdio_func *func)
{
	return atbm_gp_sdio_release_irq(func);
}

int __atbm_sdio_memcpy_fromio(struct sdio_func *func, void *dst, unsigned int addr, int count)
{
	return atbm_gp_sdio_memcpy_fromio(func, dst, addr, count);
}

int __atbm_sdio_memcpy_toio(struct sdio_func *func, unsigned int addr, void *dst, int count)
{
	return atbm_gp_sdio_memcpy_toio(func, addr, dst, count);
}

unsigned char atbm_sdio_f0_readb(struct sdio_func *func, unsigned int addr, int *retval)
{
	INT8U out = 0xff;
	INT32S ret;

	ret = atbm_gp_sdio_rw_direct(func->card, 0, 0, addr, 0, &out);
	if(retval)
	{
		*retval = ret;
	}

	return out;
}

void atbm_sdio_f0_writeb(struct sdio_func *func, unsigned char regdata, unsigned int addr, int *retval)
{
	INT32S ret;

	ret = atbm_gp_sdio_rw_direct(func->card, 1, 0, addr, regdata, ATBM_NULL);
	if(retval)
	{
		*retval = ret;
	}
}

atbm_uint32 atbm_sdio_alignsize(struct sdio_func *func, atbm_uint32 size)
{
	atbm_uint32 block_size;

	if(func == ATBM_NULL)
	{
		return size;
	}

	block_size = func->cur_blksize;
	if((block_size == 0) || (size == 0))
	{
		return size;
	}

	return ((size + block_size - 1) / block_size) * block_size;
}

int atbm_sdio_register(struct os_sdio_driver *sdio_driver)
{
	(void)sdio_driver;
	return atbm_sdio_register_init();
}

void atbm_sdio_deregister(struct os_sdio_driver *sdio_driver)
{
	(void)sdio_driver;
}

void atbm_sdio_set_drvdata(struct sdio_func *func, void *priv)
{
	if(func)
	{
		func->drv_priv = priv;
	}
}

void *atbm_sdio_get_drvdata(struct sdio_func *func)
{
	return func ? func->drv_priv : ATBM_NULL;
}

atbm_int32 atbm_sdio_enable_func(struct sdio_func *func)
{
	INT8U reg;
	INT8U ready;
	INT32S ret;
	INT32U timeout;

	if(func == ATBM_NULL)
	{
		return -1;
	}

	ret = atbm_gp_sdio_rw_direct(func->card, 0, 0, SDIO_CCCR_IOEx, 0, &reg);
	if(ret < 0)
	{
		return ret;
	}

	reg |= (1 << func->num);
	ret = atbm_gp_sdio_rw_direct(func->card, 1, 0, SDIO_CCCR_IOEx, reg, ATBM_NULL);
	if(ret < 0)
	{
		return ret;
	}

	for(timeout = 0; timeout < ATBM_SDIO_ENABLE_TIMEOUT_MS; timeout++)
	{
		ret = atbm_gp_sdio_rw_direct(func->card, 0, 0, SDIO_CCCR_IORx, 0, &ready);
		if(ret < 0)
		{
			return ret;
		}

		if(ready & (1 << func->num))
		{
			return 0;
		}

		osDelay(1);
	}

	return -1;
}

void atbm_sdio_disable_func(struct sdio_func *func)
{
	INT8U reg;

	if(func == ATBM_NULL)
	{
		return;
	}

	if(atbm_gp_sdio_rw_direct(func->card, 0, 0, SDIO_CCCR_IOEx, 0, &reg) < 0)
	{
		return;
	}

	reg &= ~(1 << func->num);
	atbm_gp_sdio_rw_direct(func->card, 1, 0, SDIO_CCCR_IOEx, reg, ATBM_NULL);
}

int atbm_sdio_set_blocksize(struct sdio_func *func, int blocksize)
{
	INT32S ret;
	INT32U base;

	if(func == ATBM_NULL)
	{
		return -1;
	}

	base = SDIO_FBR_BASE(func->num) + SDIO_FBR_BLKSIZE;
	ret = atbm_gp_sdio_rw_direct(func->card, 1, 0, base, (INT8U)(blocksize & 0xff), ATBM_NULL);
	if(ret < 0)
	{
		return ret;
	}

	ret = atbm_gp_sdio_rw_direct(func->card, 1, 0, base + 1, (INT8U)((blocksize >> 8) & 0xff), ATBM_NULL);
	if(ret < 0)
	{
		return ret;
	}

	func->max_blksize = blocksize;
	func->cur_blksize = blocksize;
	return 0;
}

struct sdio_func *atbm_sdio_check_init_func4(struct sdio_func *func)
{
	struct sdio_func *cmd_func;
	int ret;

	if((func == ATBM_NULL) || (func->card == ATBM_NULL))
	{
		return ATBM_NULL;
	}

	cmd_func = func->card->sdio_func[ATBM_SDIO_CMD_FUNCTION_NUM - 1];
	if(cmd_func == ATBM_NULL)
	{
		cmd_func = atbm_gp_sdio_alloc_func(func->card);
		if(cmd_func == ATBM_NULL)
		{
			wifi_printk(WIFI_ALWAYS, "ATBM GP SDIO: alloc function %d failed\n",
				ATBM_SDIO_CMD_FUNCTION_NUM);
			return ATBM_NULL;
		}

		cmd_func->num = ATBM_SDIO_CMD_FUNCTION_NUM;
		ret = atbm_gp_sdio_read_func_cis(cmd_func);
		if(ret != 0)
		{
			wifi_printk(WIFI_ALWAYS,
				"ATBM GP SDIO: read function %d CIS failed ret=%d\n",
				ATBM_SDIO_CMD_FUNCTION_NUM, ret);
			atbm_gp_sdio_remove_func(cmd_func);
			return ATBM_NULL;
		}

		func->card->sdio_func[ATBM_SDIO_CMD_FUNCTION_NUM - 1] = cmd_func;
		if(func->card->sdio_funcs < ATBM_SDIO_CMD_FUNCTION_NUM)
		{
			func->card->sdio_funcs = ATBM_SDIO_CMD_FUNCTION_NUM;
		}
	}

	ret = atbm_sdio_enable_func(cmd_func);
	if(ret != 0)
	{
		wifi_printk(WIFI_ALWAYS,
			"ATBM GP SDIO: enable function %d failed ret=%d\n",
			ATBM_SDIO_CMD_FUNCTION_NUM, ret);
		return ATBM_NULL;
	}

	if(cmd_func->cur_blksize == 0)
	{
		ret = atbm_sdio_set_blocksize(cmd_func,
			cmd_func->max_blksize ? cmd_func->max_blksize : 512);
		if(ret != 0)
		{
			wifi_printk(WIFI_ALWAYS,
				"ATBM GP SDIO: set function %d blocksize failed ret=%d\n",
				ATBM_SDIO_CMD_FUNCTION_NUM, ret);
			return ATBM_NULL;
		}
	}

	return cmd_func;
}

int atbm_sdio_register_init(void)
{
	gpSDIOInfo_t *card;
	struct sdio_func *func;
	struct sdio_func *prev_func;
	const struct os_sdio_device_id *match;
	int ret;

	card = atbm_gp_sdio_get_info(SDIO_WIFI_DEVICE_ID);
	if((card == ATBM_NULL) || (card->host == ATBM_NULL) || (card->host->present == 0))
	{
		wifi_printk(WIFI_ALWAYS, "ATBM GP SDIO: card is not ready\n");
		return -1;
	}

	func = card->sdio_func[ATBM_SDIO_FUNCTION_NUM - 1];
	if(func == ATBM_NULL)
	{
		wifi_printk(WIFI_ALWAYS, "ATBM GP SDIO: function %d is missing\n", ATBM_SDIO_FUNCTION_NUM);
		return -1;
	}

	match = atbm_gp_match_func(func);
	if(match == ATBM_NULL)
	{
		wifi_printk(WIFI_ALWAYS,
			"ATBM GP SDIO: unsupported func=%d vendor=0x%04x device=0x%04x\n",
			func->num, func->vendor, func->device);
		return -1;
	}

	if(g_atbm_sdio_func == func)
	{
		wifi_printk(WIFI_ALWAYS, "ATBM GP SDIO: probe already registered\n");
		return 0;
	}

	prev_func = g_atbm_sdio_func;
	g_atbm_sdio_func = func;
	ret = atbm_sdio_probe(func, ATBM_NULL);
	if(ret != 0)
	{
		g_atbm_sdio_func = prev_func;
	}

	return ret;
}

int atbm_sdio_register_deinit(void)
{
	if(g_atbm_sdio_func)
	{
		atbm_sdio_disconnect(g_atbm_sdio_func);
		g_atbm_sdio_func = ATBM_NULL;
	}

	return 0;
}

#endif

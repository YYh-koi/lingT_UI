#include "wifi_io.h"
#include "osdep_service.h"
#include "gplib_print_string.h"
#if 1

/* test wifi driver */
#define ADDR_MASK 0x10000
#define LOCAL_ADDR_MASK 0x00000

int wifi_read(struct sdio_func *func, u32 addr, u32 cnt, void *pdata)
{
	int err;

	sdio_claim_host(func);

	err = sdio_memcpy_fromio(func, pdata, addr, cnt);
	if (err) {
		print_string("%s: FAIL(%d)! ADDR=%#x Size=%d\n", __func__, err, addr, cnt);
	}

	sdio_release_host(func);

	return err;
}

int wifi_write(struct sdio_func *func, u32 addr, u32 cnt, void *pdata)
{
	int err;
	u32 size;

	sdio_claim_host(func);

	size = cnt;
	err = sdio_memcpy_toio(func, addr, pdata, size);
	if (err) {
		print_string("%s: FAIL(%d)! ADDR=%#x Size=%d(%d)\n", __func__, err, addr, cnt, size);
	}

	sdio_release_host(func);

	return err;
}

u8 wifi_readb(struct sdio_func *func, u32 addr)
{
	int err;
	u8 ret = 0;

	sdio_claim_host(func);
	ret = sdio_readb(func, ADDR_MASK | addr, &err);
	sdio_release_host(func);

	if (err)
		print_string("%s: FAIL!(%d) addr=0x%05x\n", __func__, err, addr);

	return ret;
}

u16 wifi_readw(struct sdio_func *func, u32 addr)
{
	int err;
	u16 v;

	sdio_claim_host(func);
	v = sdio_readw(func, ADDR_MASK | addr, &err);
	sdio_release_host(func);
	if (err)
		print_string("%s: FAIL!(%d) addr=0x%05x\n", __func__, err, addr);

	return  v;
}

u32 wifi_readl(struct sdio_func *func, u32 addr)
{
	int err;
	u32 v;

	sdio_claim_host(func);
	v = sdio_readl(func, ADDR_MASK | addr, &err);
	sdio_release_host(func);

	return  v;
}

void wifi_writeb(struct sdio_func *func, u32 addr, u8 val)
{
	int err;

	sdio_claim_host(func);
	sdio_writeb(func, val, ADDR_MASK | addr, &err);
	sdio_release_host(func);
	if (err)
		print_string("%s: FAIL!(%d) addr=0x%05x val=0x%02x\n", __func__, err, addr, val);
}

void wifi_writew(struct sdio_func *func, u32 addr, u16 v)
{
	int err;

	sdio_claim_host(func);
	sdio_writew(func, v, ADDR_MASK | addr, &err);
	sdio_release_host(func);
	if (err)
		print_string("%s: FAIL!(%d) addr=0x%05x val=0x%04x\n", __func__, err, addr, v);
}

void wifi_writel(struct sdio_func *func, u32 addr, u32 v)
{
	int err;

	sdio_claim_host(func);
	sdio_writel(func, v, ADDR_MASK | addr, &err);
	sdio_release_host(func);
}

u8 wifi_readb_local(struct sdio_func *func, u32 addr)
{
	int err;
	u8 ret = 0;

	ret = sdio_readb(func, LOCAL_ADDR_MASK | addr, &err);

	return ret;
}

void wifi_writeb_local(struct sdio_func *func, u32 addr, u8 val)
{
	int err;

	sdio_writeb(func, val, LOCAL_ADDR_MASK | addr, &err);
}
extern int rtw_fake_driver_probe(struct sdio_func *func);
void wifi_fake_driver_probe_rtlwifi(struct sdio_func *func)
{
	rtw_fake_driver_probe(func);//todo1
}
#endif


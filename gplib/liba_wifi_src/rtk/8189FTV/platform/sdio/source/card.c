/*
* Purpose:
*
* Author:
*
* Date: 2016/04/12
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 1.0
*/

#include "wifi_io.h"
#include "include/card.h"
#include "core/sdio_irq.h"
#include "drv_l1_clock.h"
#include "drv_l1_sdc.h"
#include "drv_l2_sdc.h"
#include "drv_l1_gpio.h"
#include "gplib_print_string.h"

#if 1// (defined _RTK_WIFI_SDIO) && (_RTK_WIFI_SDIO == 1)

/**************************************************************************
 *                            Type Definitions                            *
**************************************************************************/
#define C_SDIO_THREAD_START          1
#define C_SDIO_THREAD_IRQ_HANDLER    2

int sdio_memcpy_fromio(struct sdio_func *func, void *dst,unsigned int addr, int count);
int sdio_memcpy_toio  (struct sdio_func *func, unsigned int addr,void *src, int count);
void sdio_claim_host  (struct sdio_func *func);
void sdio_release_host(struct sdio_func *func);

//---for debug
#define SDIO_DEBUG_EN                1
#if SDIO_DEBUG_EN == 1
#define	_SDIO_DEBUG_NORMAL_EN        1
#define	_SDIO_DEBUG_RW_EN            0
#define	_SDIO_DEBUG_FN_NAME          1
#endif

#define SDIO_DEBUG_NORMAL(fmt, ...)	 do { if (_SDIO_DEBUG_NORMAL_EN) print_string("[DEBUG] " fmt"\r\n", ##__VA_ARGS__); } while (0)
#define SDIO_DEBUG_RW(fmt, ...)		 do { if (_SDIO_DEBUG_RW_EN)     print_string("[DEBUG] " fmt"\r\n", ##__VA_ARGS__); } while (0)
#define SDIO_DEBUG_FN_NAME           do { if (_SDIO_DEBUG_FN_NAME)   print_string("[DEBUG] %s\r\n", __FUNCTION__); } while (0)

/**************************************************************************
 *                         G L O B A L    D A T A                         *
**************************************************************************/
osMutexId wifiMutex = NULL;
volatile osMessageQId sdio_irq_q = NULL;

extern int sdio_host_device_id;
extern struct sdio_func *wifi_sdio_func;

/**************************************************************************
 *                         G L O B A L    F U N C T I O N                 *
**************************************************************************/
extern bool sdio_host_enable_isr(bool enable);
//------------------------------------------------------
void sdio_irq_thread(void const* exinf)
{
    INT32U msg_id;
    osEvent result;
    INT32 ret;
    INT8U pending;

    while(1)
    {
        result = osMessageGet(sdio_irq_q, osWaitForever);
        msg_id = result.value.v;
        if((result.status != osEventMessage) || (msg_id == 0)) {
            continue;
        }

        switch(msg_id)
        {
            case C_SDIO_THREAD_IRQ_HANDLER:
            {
                sdio_claim_host(wifi_sdio_func);
                ret = drvl2_sdio_rw_direct(wifi_sdio_func->card, 0, 0, SDIO_CCCR_INTx, 0, &pending);
                if (ret){
                    print_string("[sdio thread]: error %d reading SDIO_CCCR_INTx\r\n",ret);
                    //sdio_host_enable_isr(true);
                   // break;
                }
                /* enable sdio control interupt */
                /*if(wifi_sdio_func)
                    sdio_host_enable_isr(true);*/

                if (wifi_sdio_func->irq_handler && (pending & 1<<wifi_sdio_func->num))
                {
                    wifi_sdio_func->irq_handler(wifi_sdio_func);
                }

                sdio_release_host(wifi_sdio_func);
                /* enable sdio control interupt */
                if(wifi_sdio_func)
                    sdio_host_enable_isr(true);
            break;
            }
            case C_SDIO_THREAD_START:
            {
                //print_string("[sdio thread] received start\r\n");
            break;
            }
            default:
                print_string("[sdio thread] no msg type %d\r\n",msg_id);
            break;
        }
  //      osDelay(1);
    }
    print_string("[sdio thread] exit!\r\n");
}

void sdio_host_isr(INT32U device_id)
{
    if(sdio_irq_q)
    {
        sdio_host_enable_isr(false);
        INT32U event;
        event = C_SDIO_THREAD_IRQ_HANDLER;
        osMessagePut(sdio_irq_q, (INT32U)&event, osWaitForever);
    }
}

void SD0_IRQHandler(void)
{
    sdio_host_isr(0);
}

void SD1_IRQHandler(void)
{
    sdio_host_isr(1);
}

bool sdio_host_enable_isr(bool enable)
{
    drvl2_sdio_irq_en(wifi_sdio_func->card,enable);

    //print_string("%x,%x,%x\r\n",*(volatile INT32U *)(0xC0250014),*(volatile INT32U *)(0xC0250018),*(volatile INT32U *)(0xC025001C));
    return true;
}

int sdio_bus_probe(void)
{
    SDIO_DEBUG_FN_NAME;
    INT32U device_id = sdio_host_device_id;
    INT8U retry = 10;

    while(drvl2_sdio_init(device_id) < 0 && retry)
    {
        print_string("sdio_bus_probe: retry(%d) sdio init\r\n",retry);
        retry--;
    }

    if (wifiMutex == NULL){
        wifiMutex = osMutexCreate(NULL);
        if(wifiMutex == 0)
            print_string("Failed to allocate wifiMutex memory\r\n");
    }

    if(retry)
        return 0;
    else
        return -1;
}

int sdio_bus_remove(void)
{
    INT32U device_id = sdio_host_device_id;

    if (drvl2_sdio_deinit(device_id) < 0)
        goto fail;

    if (wifiMutex){
        vSemaphoreDelete(wifiMutex);
        wifiMutex = 0;
    }

    return 0;
fail:
    return -1;
}

int sdio_claim_irq(struct sdio_func *func, void(*handler)(struct sdio_func *))
{
    SDIO_DEBUG_FN_NAME;
    INT32 ret;
    INT8U reg = 0;
    osThreadDef_t sdio_irq = { "sdio_irq_thread", sdio_irq_thread, osPriorityRealtime, 1, 4096 };

    if(sdio_irq_q == 0) {
        print_string("sdio_claim_irq create msg queue\r\n");
        osMessageQDef_t sdio_q = {1, sizeof(INT32U), 0};

        sdio_irq_q = osMessageCreate(&sdio_q, NULL);
        if(sdio_irq_q == 0)
        {
            print_string("sdio_irq_q is null\r\n");
        }
    }

    if(func->card->host->sdio_irq_thread_id == 0)
    {
        print_string("sdio_claim_irq create msg thread\r\n");
        func->card->host->sdio_irq_thread_id = osThreadCreate(&sdio_irq, (void *)NULL);
        if(func->card->host->sdio_irq_thread_id  == 0)
        {
            print_string("sdio_irq_thread_id is null\r\n");
        }
        INT32U event;
        event = C_SDIO_THREAD_START;
        osMessagePut(sdio_irq_q, (INT32U)&event, osWaitForever);
    }

    print_string("sdio_claim_irq irq_handler %x\r\n",handler);
    if (func->irq_handler) {
        print_string("sdio_claim_irq irq_handler(%x) already in use.\n",func->irq_handler);
        //return -1;
    }

    func->irq_handler = handler;

    ret = drvl2_sdio_rw_direct(func->card, 0, 0, SDIO_CCCR_IENx, 0, &reg);
    if (ret)
        return ret;

    reg |= 1 << func->num;

    reg |= 1; /* Master interrupt enable */

    ret = drvl2_sdio_rw_direct(func->card, 1, 0, SDIO_CCCR_IENx, reg, NULL);
    if (ret)
        return ret;

    ret = drvl2_sdio_rw_direct(func->card, 0, 0, SDIO_CCCR_IENx, 0, &reg);
    print_string("sdio_claim_irq reg 0x%x\r\n",reg);

    /* enable sdio control interupt */
    if(wifi_sdio_func)
        sdio_host_enable_isr(true);

    return 0;
}

int sdio_release_irq(struct sdio_func *func)
{
    SDIO_DEBUG_FN_NAME;
    INT32 ret;
    INT8U reg;

    if(func->irq_handler){
        func->irq_handler = NULL;
    }

    ret = drvl2_sdio_rw_direct(func->card, 0, 0, SDIO_CCCR_IENx, 0, &reg);
    if (ret)
        return ret;

    reg &= ~(1 << func->num);

    /* Disable master interrupt with the last function interrupt */
    if (!(reg & 0xFE))
        reg = 0;

    ret = drvl2_sdio_rw_direct(func->card, 1, 0, SDIO_CCCR_IENx, reg, NULL);
    if (ret)
        return ret;

    if (wifi_sdio_func) {
        sdio_host_enable_isr(0);
    }

    if (func->card->host->sdio_irq_thread_id) {
        osThreadTerminate(func->card->host->sdio_irq_thread_id);
        func->card->host->sdio_irq_thread_id = 0;
    }

    if (sdio_irq_q) {
        vQueueDelete(sdio_irq_q);
        sdio_irq_q = 0;
    }

    return 0;
}

void sdio_claim_host(struct sdio_func *func)
{
    if (!wifiMutex)
        return;

    osMutexWait(wifiMutex,osWaitForever);
}

void sdio_release_host(struct sdio_func *func)
{
    if (!wifiMutex)
        return;

    osMutexRelease(wifiMutex);
}

unsigned char sdio_readb(struct sdio_func *func, unsigned int addr, int *err_ret)
{
    INT8U in, out;
    INT32S ret;

    in  = 0;
    out = 0;

    if (err_ret)
        *err_ret = 0;

    ret = drvl2_sdio_rw_direct(func->card, 0, func->num, addr, in, &out);
    if (ret) {
        if (err_ret)
            *err_ret = ret;
        return 0xFF;
    }

    SDIO_DEBUG_RW("%-15s : 0x%08x, 0x%02x\r\n", "sdio_readb", addr, out);

    return out;
}

unsigned short sdio_readw(struct sdio_func *func, unsigned int addr, int *err_ret)
{
    INT32S ret = 0;
    INT32  size = 2 ;

    if ((INT32U)func->tmpbuf & 3)
    {
        print_string("data buf must be align 4 byte, data = 0x%08x \r\n", (INT32U)func->tmpbuf);
        return false;
    }

    ret = sdio_memcpy_fromio(func, &func->tmpbuf ,addr, size);

    if (0 != ret)
    {
        print_string("sdio_readw, ret = %d\r\n", ret);
        return false;
    }

    SDIO_DEBUG_RW("%-15s : 0x%x, tmpbuf 0x%08x, size 0x%02x\r\n", "sdio_readw", addr, *func->tmpbuf, size);

    return func->tmpbuf[3]<<8 | func->tmpbuf[2];
}

unsigned int sdio_readl(struct sdio_func *func, unsigned int addr, int *err_ret)
{
    INT32S ret = 0;
    INT32 size = 4 ;

    if ((INT32U)func->tmpbuf & 3)
    {
        print_string("data buf must be align 4 byte, data = 0x%08x\r\n", (INT32U)func->tmpbuf);
        return false;
    }

    ret = sdio_memcpy_fromio(func, &func->tmpbuf ,addr, size);

    if (0 != ret)
    {
        print_string("sdio_readl, ret = %d\r\n", ret);
        return false;
    }

    SDIO_DEBUG_RW("%-15s : 0x%x, 0x%08x, 0x%02x\r\n", "sdio_readl", addr, *func->tmpbuf, size);

    return *(INT32U *)func->tmpbuf;
}

void sdio_writeb(struct sdio_func *func, unsigned char b,unsigned int addr, int *err_ret)
{
    INT32S ret;

    ret = drvl2_sdio_rw_direct(func->card, 1, func->num, addr, b, NULL);
    if (err_ret)
        *err_ret = ret;

    SDIO_DEBUG_RW("%-15s : 0x%08x, 0x%02x err_ret %d\r\n", "sdio_writeb", addr, b,ret);
}

void sdio_writew(struct sdio_func *func, unsigned short b,unsigned int addr, int *err_ret)
{
    INT32S ret = 0;
    INT32  size = 2 ;
    //unsigned char in __attribute__ ((aligned (4)));

    if ((INT32U)func->tmpbuf & 3)
    {
        print_string("data buf must be align 4 byte, data = 0x%08x\r\n", (INT32U)func->tmpbuf);
        return;
    }

    *(INT32U *)func->tmpbuf = b;

    ret = sdio_memcpy_toio(func, addr, &func->tmpbuf, size);

    if (0 != ret)
    {
        print_string("sdio_writeb, ret = %d\r\n", ret);
        return ;
    }

    SDIO_DEBUG_RW("%-15s : 0x%x, 0x%08x, 0x%02x\r\n", "sdio_writew", addr, *func->tmpbuf, size);
}

void sdio_writel(struct sdio_func *func, unsigned int b,unsigned int addr, int *err_ret)
{
    INT32S ret = 0;
    INT32  size = 4 ;

    if ((INT32U)func->tmpbuf & 3)
    {
        print_string("data buf must be align 4 byte, data = 0x%08x\r\n", (INT32U)func->tmpbuf);
        return ;
    }

    *(INT32U *)func->tmpbuf = b;

    ret = sdio_memcpy_toio(func, addr, &func->tmpbuf, size);

    if (0 != ret)
    {
        print_string("sdio_writel, ret = %d\r\n", ret);
        return ;
    }

    SDIO_DEBUG_RW("%-15s : 0x%x, 0x%08x, 0x%02x\r\n", "sdio_writel", addr, b, size);
}

int sdio_memcpy_fromio(struct sdio_func *func, void *dst,unsigned int addr, int count)/*read a chunk of memory from a SDIO functio*/
{
    INT32U rx_blocks = 0, blksize = 0;
    INT32 remainder = count;
    INT32 incr_addr = 1;
    INT32S ret = 0;
    INT32 save_count = count;

    //if (remainder > func->cur_blksize))
    {
        while (remainder > func->cur_blksize) {
            rx_blocks = remainder / func->cur_blksize;

            count = rx_blocks * func->cur_blksize;

            ret = drvl2_sdio_rw_extended(func->card, 0, func->num, addr, incr_addr, dst, rx_blocks, func->cur_blksize);
            if (0 != ret)
            {
                print_string("drvl2_sdio_rw_extended(read)Block, blocks %d blksize %d ret=%d num=%d addr=0x%x\r\n", rx_blocks,func->cur_blksize,ret, func->num,addr);
                return ret;
            }

            remainder -= count;
            dst       += count;
            if (incr_addr)
                addr += count;
        }
    }

    if(remainder)
    {
        blksize = remainder;
        rx_blocks = 1;

        ret = drvl2_sdio_rw_extended(func->card, 0, func->num, addr, incr_addr, dst, rx_blocks, blksize);
        if (0 != ret)
        {
            print_string("drvl2_sdio_rw_extended(read)Byte, blocks %d blksize %d ret=%d save_count=%d num=%d addr=0x%x\r\n", rx_blocks,blksize, ret, save_count, func->num,addr);
            return ret;
        }
    }

    return 0;
}

int sdio_memcpy_toio(struct sdio_func *func, unsigned int addr,void *src, int count)  /*write a chunk of memory to a SDIO function*/
{
    INT32U rx_blocks = 0, blksize = 0;
    INT32 remainder = count;
    INT32 incr_addr = 1;
    INT32S ret = 0;

    //if(count > 1000) print_string("sdio_memcpy_toio addr %x src %x count %d\r\n",addr,src,count);
    //if (remainder > func->cur_blksize))
    {
        while (remainder > func->cur_blksize) {
            rx_blocks = remainder / func->cur_blksize;

            count = rx_blocks * func->cur_blksize;

            ret = drvl2_sdio_rw_extended(func->card, 1, func->num, addr, incr_addr, src, rx_blocks, func->cur_blksize);
            if (0 != ret)
            {
                print_string("drvl2_sdio_rw_extended(write)Block, blocks %d blksize %d ret=%d num=%d addr=0x%x\r\n", rx_blocks,func->cur_blksize,ret, func->num,addr);
                return ret;
            }

            remainder -= count;
            src       += count;
            if (incr_addr)
                addr += count;
        }
    }

    if(remainder)
    {
        blksize = remainder;
        rx_blocks = 1;

        ret = drvl2_sdio_rw_extended(func->card, 1, func->num, addr, incr_addr, src, rx_blocks, blksize);
        if (0 != ret)
        {
            print_string("drvl2_sdio_rw_extended(write)Byte, blocks %d blksize %d ret=%d num=%d addr=0x%x\r\n", rx_blocks,blksize, ret, func->num,addr);
            return ret;
        }
    }

    return 0;
}

const SDIO_BUS_OPS rtw_sdio_bus_ops =
{
	sdio_bus_probe,
	sdio_bus_remove,
	0,
	0,
	0,
	0,
	sdio_claim_irq,
	sdio_release_irq,
	sdio_claim_host,
	sdio_release_host,
	sdio_readb,
	sdio_readw,
	sdio_readl,
	sdio_writeb,
	sdio_writew,
	sdio_writel,
	sdio_memcpy_fromio,
	sdio_memcpy_toio
};

#endif



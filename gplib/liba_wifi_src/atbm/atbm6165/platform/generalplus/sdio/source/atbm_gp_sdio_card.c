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

#include "atbm_gp_sdio_card.h"
#include "atbm_gp_sdio_irq.h"
#include "atbm_gp_sdio_l2.h"
#include "drv_l1_clock.h"
#include "drv_l1_sdc.h"
#include "drv_l1_gpio.h"
#include "gplib_print_string.h"
#include "FreeRTOS.h"
#include "semphr.h"

#if 1// (defined _RTK_WIFI_SDIO) && (_RTK_WIFI_SDIO == 1)

/**************************************************************************
 *                            Type Definitions                            *
**************************************************************************/
#define ATBM_SDIO_IRQ_SEM_MAX_COUNT  32

int atbm_gp_sdio_memcpy_fromio(struct sdio_func *func, void *dst,unsigned int addr, int count);
int atbm_gp_sdio_memcpy_toio  (struct sdio_func *func, unsigned int addr,void *src, int count);
void atbm_gp_sdio_claim_host  (struct sdio_func *func);
void atbm_gp_sdio_release_host(struct sdio_func *func);

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
static osMutexId atbm_gp_wifi_mutex = NULL;
static SemaphoreHandle_t atbm_gp_sdio_irq_sem = NULL;

extern struct sdio_func *atbm_gp_wifi_sdio_func;

/**************************************************************************
 *                         G L O B A L    F U N C T I O N                 *
**************************************************************************/
extern bool atbm_gp_sdio_host_enable_isr(bool enable);
//------------------------------------------------------
void atbm_gp_sdio_irq_thread(void const* exinf)
{
    INT32 ret;
    INT8U pending;
    INT8U reenable_irq;

    (void)exinf;
    while(1)
    {
        if ((atbm_gp_sdio_irq_sem == NULL) ||
            (xSemaphoreTake(atbm_gp_sdio_irq_sem, portMAX_DELAY) != pdTRUE)) {
            continue;
        }

        reenable_irq = 1;
        ret = 0;

        if ((atbm_gp_wifi_sdio_func == NULL) ||
            (atbm_gp_wifi_sdio_func->card == NULL)) {
            continue;
        }

        atbm_gp_sdio_claim_host(atbm_gp_wifi_sdio_func);
        pending = 0;
        ret = atbm_gp_sdio_rw_direct(atbm_gp_wifi_sdio_func->card, 0, 0,
            SDIO_CCCR_INTx, 0, &pending);
        if (ret) {
            goto irq_done;
        }

        if (atbm_gp_wifi_sdio_func->irq_handler &&
            (pending & (1 << atbm_gp_wifi_sdio_func->num))) {
            /* RX owns IRQ re-enable after it has drained the card. */
            reenable_irq = 0;
            atbm_gp_wifi_sdio_func->irq_handler(atbm_gp_wifi_sdio_func);
        }

irq_done:
        atbm_gp_sdio_release_host(atbm_gp_wifi_sdio_func);
        if (reenable_irq) {
            atbm_gp_sdio_host_enable_isr(true);
        }
    }
}

void atbm_gp_sdio_host_isr(INT32U device_id)
{
    (void)device_id;

    if(atbm_gp_sdio_irq_sem)
    {
        BaseType_t task_woken = pdFALSE;

        atbm_gp_sdio_host_enable_isr(false);
        xSemaphoreGiveFromISR(atbm_gp_sdio_irq_sem, &task_woken);
        portYIELD_FROM_ISR(task_woken);
    }
}

void atbm_gp_sdio0_irq_handler(void)
{
    atbm_gp_sdio_host_isr(0);
}

void atbm_gp_sdio1_irq_handler(void)
{
    atbm_gp_sdio_host_isr(1);
}

INT32S atbm_gp_sdio_host_mutex_init(void)
{
    if (atbm_gp_wifi_mutex != NULL) {
        return 0;
    }

    atbm_gp_wifi_mutex = osMutexCreate(NULL);
    if (atbm_gp_wifi_mutex == NULL) {
        print_string("Failed to allocate atbm_gp_wifi_mutex\r\n");
        return -1;
    }

    return 0;
}

void atbm_gp_sdio_host_mutex_deinit(void)
{
    if (atbm_gp_wifi_mutex != NULL) {
        vSemaphoreDelete(atbm_gp_wifi_mutex);
        atbm_gp_wifi_mutex = NULL;
    }
}

bool atbm_gp_sdio_host_enable_isr(bool enable)
{
    if ((atbm_gp_wifi_sdio_func == NULL) ||
        (atbm_gp_wifi_sdio_func->card == NULL)) {
        return false;
    }
    atbm_gp_sdio_irq_en(atbm_gp_wifi_sdio_func->card,enable);

    //print_string("%x,%x,%x\r\n",*(volatile INT32U *)(0xC0250014),*(volatile INT32U *)(0xC0250018),*(volatile INT32U *)(0xC025001C));
    return true;
}

int atbm_gp_sdio_claim_irq(struct sdio_func *func, void(*handler)(struct sdio_func *))
{
    SDIO_DEBUG_FN_NAME;
    INT32 ret;
    INT8U reg = 0;
    osThreadDef_t sdio_irq = { "atbm_gp_sdio_irq_thread", atbm_gp_sdio_irq_thread, osPriorityRealtime, 1, 4096 };

    if(atbm_gp_sdio_irq_sem == NULL) {
        atbm_gp_sdio_irq_sem = xSemaphoreCreateCounting(
            ATBM_SDIO_IRQ_SEM_MAX_COUNT, 0);
        if(atbm_gp_sdio_irq_sem == NULL) {
            print_string("atbm_gp_sdio_irq_sem is null\r\n");
            return -1;
        }
    }

    if(func->card->host->sdio_irq_thread_id == 0)
    {
        func->card->host->sdio_irq_thread_id = osThreadCreate(&sdio_irq, (void *)NULL);
        if(func->card->host->sdio_irq_thread_id  == 0)
        {
            print_string("sdio_irq_thread_id is null\r\n");
            vSemaphoreDelete(atbm_gp_sdio_irq_sem);
            atbm_gp_sdio_irq_sem = NULL;
            return -1;
        }
    }

    print_string("atbm_gp_sdio_claim_irq irq_handler %x\r\n",handler);
    if (func->irq_handler) {
        print_string("atbm_gp_sdio_claim_irq irq_handler(%x) already in use.\n",func->irq_handler);
        //return -1;
    }

    func->irq_handler = handler;

    ret = atbm_gp_sdio_rw_direct(func->card, 0, 0, SDIO_CCCR_IENx, 0, &reg);
    if (ret)
        return ret;

    reg |= 1 << func->num;

    reg |= 1; /* Master interrupt enable */

    ret = atbm_gp_sdio_rw_direct(func->card, 1, 0, SDIO_CCCR_IENx, reg, NULL);
    if (ret)
        return ret;

    ret = atbm_gp_sdio_rw_direct(func->card, 0, 0, SDIO_CCCR_IENx, 0, &reg);
    print_string("atbm_gp_sdio_claim_irq reg 0x%x\r\n",reg);

    /* enable sdio control interupt */
    if(atbm_gp_wifi_sdio_func)
        atbm_gp_sdio_host_enable_isr(true);

    return 0;
}

int atbm_gp_sdio_release_irq(struct sdio_func *func)
{
    SDIO_DEBUG_FN_NAME;
    INT32 ret;
    INT8U reg;

    if(func->irq_handler){
        func->irq_handler = NULL;
    }

    ret = atbm_gp_sdio_rw_direct(func->card, 0, 0, SDIO_CCCR_IENx, 0, &reg);
    if (ret)
        return ret;

    reg &= ~(1 << func->num);

    /* Disable master interrupt with the last function interrupt */
    if (!(reg & 0xFE))
        reg = 0;

    ret = atbm_gp_sdio_rw_direct(func->card, 1, 0, SDIO_CCCR_IENx, reg, NULL);
    if (ret)
        return ret;

    if (atbm_gp_wifi_sdio_func) {
        atbm_gp_sdio_host_enable_isr(0);
    }

    if (func->card->host->sdio_irq_thread_id) {
        osThreadTerminate(func->card->host->sdio_irq_thread_id);
        func->card->host->sdio_irq_thread_id = 0;
    }

    if (atbm_gp_sdio_irq_sem) {
        vSemaphoreDelete(atbm_gp_sdio_irq_sem);
        atbm_gp_sdio_irq_sem = NULL;
    }

    return 0;
}

void atbm_gp_sdio_claim_host(struct sdio_func *func)
{
    if (atbm_gp_wifi_mutex == NULL)
        return;

    osMutexWait(atbm_gp_wifi_mutex,osWaitForever);
}

void atbm_gp_sdio_release_host(struct sdio_func *func)
{
    if (atbm_gp_wifi_mutex == NULL)
        return;

    osMutexRelease(atbm_gp_wifi_mutex);
}

unsigned char atbm_gp_sdio_readb(struct sdio_func *func, unsigned int addr, int *err_ret)
{
    INT8U in, out;
    INT32S ret;

    in  = 0;
    out = 0;

    if (err_ret)
        *err_ret = 0;

    ret = atbm_gp_sdio_rw_direct(func->card, 0, func->num, addr, in, &out);
    if (ret) {
        if (err_ret)
            *err_ret = ret;
        return 0xFF;
    }

    SDIO_DEBUG_RW("%-15s : 0x%08x, 0x%02x\r\n", "atbm_gp_sdio_readb", addr, out);

    return out;
}

unsigned short atbm_gp_sdio_readw(struct sdio_func *func, unsigned int addr, int *err_ret)
{
    INT32S ret = 0;
    INT32  size = 2 ;

    if ((INT32U)func->tmpbuf & 3)
    {
        print_string("data buf must be align 4 byte, data = 0x%08x \r\n", (INT32U)func->tmpbuf);
        return false;
    }

    ret = atbm_gp_sdio_memcpy_fromio(func, &func->tmpbuf ,addr, size);

    if (0 != ret)
    {
        print_string("atbm_gp_sdio_readw, ret = %d\r\n", ret);
        return false;
    }

    SDIO_DEBUG_RW("%-15s : 0x%x, tmpbuf 0x%08x, size 0x%02x\r\n", "atbm_gp_sdio_readw", addr, *func->tmpbuf, size);

    return func->tmpbuf[3]<<8 | func->tmpbuf[2];
}

unsigned int atbm_gp_sdio_readl(struct sdio_func *func, unsigned int addr, int *err_ret)
{
    INT32S ret = 0;
    INT32 size = 4 ;

    if ((INT32U)func->tmpbuf & 3)
    {
        print_string("data buf must be align 4 byte, data = 0x%08x\r\n", (INT32U)func->tmpbuf);
        return false;
    }

    ret = atbm_gp_sdio_memcpy_fromio(func, &func->tmpbuf ,addr, size);

    if (0 != ret)
    {
        print_string("atbm_gp_sdio_readl, ret = %d\r\n", ret);
        return false;
    }

    SDIO_DEBUG_RW("%-15s : 0x%x, 0x%08x, 0x%02x\r\n", "atbm_gp_sdio_readl", addr, *func->tmpbuf, size);

    return *(INT32U *)func->tmpbuf;
}

void atbm_gp_sdio_writeb(struct sdio_func *func, unsigned char b,unsigned int addr, int *err_ret)
{
    INT32S ret;

    ret = atbm_gp_sdio_rw_direct(func->card, 1, func->num, addr, b, NULL);
    if (err_ret)
        *err_ret = ret;

    SDIO_DEBUG_RW("%-15s : 0x%08x, 0x%02x err_ret %d\r\n", "atbm_gp_sdio_writeb", addr, b,ret);
}

void atbm_gp_sdio_writew(struct sdio_func *func, unsigned short b,unsigned int addr, int *err_ret)
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

    ret = atbm_gp_sdio_memcpy_toio(func, addr, &func->tmpbuf, size);

    if (0 != ret)
    {
        print_string("atbm_gp_sdio_writeb, ret = %d\r\n", ret);
        return ;
    }

    SDIO_DEBUG_RW("%-15s : 0x%x, 0x%08x, 0x%02x\r\n", "atbm_gp_sdio_writew", addr, *func->tmpbuf, size);
}

void atbm_gp_sdio_writel(struct sdio_func *func, unsigned int b,unsigned int addr, int *err_ret)
{
    INT32S ret = 0;
    INT32  size = 4 ;

    if ((INT32U)func->tmpbuf & 3)
    {
        print_string("data buf must be align 4 byte, data = 0x%08x\r\n", (INT32U)func->tmpbuf);
        return ;
    }

    *(INT32U *)func->tmpbuf = b;

    ret = atbm_gp_sdio_memcpy_toio(func, addr, &func->tmpbuf, size);

    if (0 != ret)
    {
        print_string("atbm_gp_sdio_writel, ret = %d\r\n", ret);
        return ;
    }

    SDIO_DEBUG_RW("%-15s : 0x%x, 0x%08x, 0x%02x\r\n", "atbm_gp_sdio_writel", addr, b, size);
}

int atbm_gp_sdio_memcpy_fromio(struct sdio_func *func, void *dst,unsigned int addr, int count)/*read a chunk of memory from a SDIO functio*/
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

            ret = atbm_gp_sdio_rw_extended(func->card, 0, func->num, addr, incr_addr, dst, rx_blocks, func->cur_blksize);
            if (0 != ret)
            {
                print_string("atbm_gp_sdio_rw_extended(read)Block, blocks %d blksize %d ret=%d num=%d addr=0x%x\r\n", rx_blocks,func->cur_blksize,ret, func->num,addr);
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

        ret = atbm_gp_sdio_rw_extended(func->card, 0, func->num, addr, incr_addr, dst, rx_blocks, blksize);
        if (0 != ret)
        {
            print_string("atbm_gp_sdio_rw_extended(read)Byte, blocks %d blksize %d ret=%d save_count=%d num=%d addr=0x%x\r\n", rx_blocks,blksize, ret, save_count, func->num,addr);
            return ret;
        }
    }

    return 0;
}

int atbm_gp_sdio_memcpy_toio(struct sdio_func *func, unsigned int addr,void *src, int count)  /*write a chunk of memory to a SDIO function*/
{
    INT32U rx_blocks = 0, blksize = 0;
    INT32 remainder = count;
    INT32 incr_addr = 1;
    INT32S ret = 0;

    //if(count > 1000) print_string("atbm_gp_sdio_memcpy_toio addr %x src %x count %d\r\n",addr,src,count);
    //if (remainder > func->cur_blksize))
    {
        while (remainder > func->cur_blksize) {
            rx_blocks = remainder / func->cur_blksize;

            count = rx_blocks * func->cur_blksize;

            ret = atbm_gp_sdio_rw_extended(func->card, 1, func->num, addr, incr_addr, src, rx_blocks, func->cur_blksize);
            if (0 != ret)
            {
                print_string("atbm_gp_sdio_rw_extended(write)Block, blocks %d blksize %d ret=%d num=%d addr=0x%x\r\n", rx_blocks,func->cur_blksize,ret, func->num,addr);
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

        ret = atbm_gp_sdio_rw_extended(func->card, 1, func->num, addr, incr_addr, src, rx_blocks, blksize);
        if (0 != ret)
        {
            print_string("atbm_gp_sdio_rw_extended(write)Byte, blocks %d blksize %d ret=%d num=%d addr=0x%x\r\n", rx_blocks,blksize, ret, func->num,addr);
            return ret;
        }
    }

    return 0;
}

#endif

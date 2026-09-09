/*
* Purpose: NV RAM access
*
* Author: wschung
*
* Date: 2008/06/03
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 2.01
* History :
*         1 2008/06/06 Created by wschung.  v1.00
*         2 2009/01/13 Using 19 bytes as resoure file name and 4 bytes as file sector count   V2.00
*         3 2009/01/21 Adding NVRAM_V3 for Nand flash APP area supporting
*/
#include "drv_l2.h"

#define RS_FILE_LEN     (19)
#define IsLower(c)      (((c)>='a') && ((c)<='z'))

INT32U combin_reg_data(INT8U *data, INT32S len);

//resource
//INT32U APP_area_position;
INT32U Resource_area_position = 0;
INT32U Resource_area_size;
INT32U User_area_position = 0;
//INT16U total_rs;
INT16U RS_TABLE_SIZE;

FLASHRSINDEX *RS_TABLE = NULL; //Index table of RES

//user setting
ALIGN64 INT32U tbuf[128];

//L3
//INT32U nvmemory_init(void);       //external
//void nvmemory_uninit(void);       //external

//INT16U  nv_open(INT8U *path);                                 //external
//INT32U  nv_read(INT16S fd, INT32U buf, INT32U size);          //external
INT32U  nv_mcu_read(INT16S fd, INT32U buf, INT32U size);        //reserved for future use
//INT32U  nv_lseek(INT16S fd, INT32S foffset, INT16S origin);   //external
//INT32U  nv_rs_size_get(INT16S fd);                            //external

//INT32U nvmemory_user_sector_load(INT16U itag, INT32U *pbuf , INT16U sector_count);    //external
//INT32U nvmemory_user_sector_store(INT16U itag, INT32U *pbuf , INT16U sector_count);   //external

//L2
INT32U  nvmemory_rs_byte_load(INT8U *name ,INT32U offset_byte, INT32U *pbuf , INT32U byte_count);           //called by nv_mcu_read()
INT32U  nvmemory_rs_sector_load(INT8U *name ,INT32U offset_secter, INT32U *pbuf , INT16U sector_count);     //called by nvmemory_rs_byte_load()


#if (C_NV_FLASH_TYPE == C_SPI_FLASH)

//--------------------------------------------------------------------------------------------------------------------------------
//NV SPI flash
#define IO_1BIT         0
#define IO_2BIT         1
#define IO_4BIT         2

// Note: when SPIFC_RES is 0, Micron SPINOR not yet ready adjust for IO_2BIT, IO_4BIT, 55500000
#define SPIFC_RES               1       //To use SPIFC, set this to 1
#define SPI_READ_BUS_WIDTH      IO_1BIT //IO_1BIT, IO_2BIT, IO_4BIT

//L1
INT32U  nvspiflash_retrieval(void);
INT32U  nvspiflash_rs_get(INT16U itag ,INT32U offset_secter, INT32U *pbuf , INT16U sector_count);   //called by nvmemory_rs_sector_load()
INT32U  nvspiflash_user_set (INT16U itag ,INT32U *pbuf , INT16U sector_count);                      //called by nvmemory_user_sector_store()
INT32U  nvspiflash_user_get (INT16U itag ,INT32U *pbuf , INT16U sector_count);                      //called by nvmemory_user_sector_load()

#if SPIFC_RES
static SemaphoreHandle_t sw_spifc_sem = NULL;
INT32S  SPIFC_Flash_erase_block(INT32U addr);
INT32S  SPIFC_Flash_read_page(INT32U addr, INT8U *buf, INT32U byte_len);
INT32S  SPIFC_Flash_write_page(INT32U addr, INT8U *buf);
#endif

#elif (C_NV_FLASH_TYPE == C_NAND_FLASH)

//--------------------------------------------------------------------------------------------------------------------------------
//NV NAND
//L1
INT32U nvnandflash_retrieval(void);
INT32U nvnandflash_rs_get(INT16U itag ,INT32U offset_secter, INT32U *pbuf , INT16U sector_count);

INT32U nvnandflash_user_get(INT16U itag ,INT32U *pbuf_align_16 , INT16U sector_count);
INT32U nvnandflash_user_set(INT16U itag ,INT32U *pbuf_align_16 , INT16U sector_count);

/*
#define BootToAPP_HEADER_OFFSET_SEC_CNT         136
#define APPToResource_HEADER_OFFSET_SEC_CNT     20
#define HEADER_BCH_BLK_CNT         16
#define HEADER_BOOT_BACKUP_CNT     19

#define BOOT_AREA_START			0
#define BOOT_AREA_SIZE			10
#define PHYSIC_BLOCK_START		(BOOT_AREA_START + BOOT_AREA_SIZE)
*/

extern INT32U NandBootWriteSector(INT32U wWriteLBA, INT16U wLen, INT32U DataBufAddr);
extern INT32U NandBootReadSector(INT32U wReadLBA, INT16U wLen, INT32U DataBufAddr);
extern INT32U NandBootFlush(void);
extern INT32S calc_spi_data_zone_byte_offset(INT8U* pAppHeader, INT32U appHeaderSize, INT32U* p_data_start_offset);

void    NVNAND_OS_LOCK(void);
void    NVNAND_OS_UNLOCK(void);

#if (defined GPLIB_FILE_SYSTEM_EN) && (GPLIB_FILE_SYSTEM_EN == 1)
    #if _OPERATING_SYSTEM == _OS_UCOS2
    extern OS_EVENT *gFS_sem;
    #elif _OPERATING_SYSTEM == _OS_FREERTOS
    extern xSemaphoreHandle gFS_sem;
    #endif

    #define _NVNAND_OS_     1
#elif _OPERATING_SYSTEM == _OS_UCOS2
    OS_EVENT *gFS_sem;
    #define _NVNAND_OS_     1
#else
    #define _NVNAND_OS_     0
#endif

void NVNAND_OS_LOCK(void)
{
    #if _NVNAND_OS_ == 1
    #if _OPERATING_SYSTEM == _OS_UCOS2
    INT8U err = NULL;
    OSSemPend(gFS_sem, 0, &err);
    #elif _OPERATING_SYSTEM == _OS_FREERTOS
    xSemaphoreTake(gFS_sem, portMAX_DELAY);
    #endif
    #endif
}

void NVNAND_OS_UNLOCK(void)
{
    #if _NVNAND_OS_ == 1
    #if _OPERATING_SYSTEM == _OS_UCOS2
    OSSemPost(gFS_sem);
    #elif _OPERATING_SYSTEM == _OS_FREERTOS
    xSemaphoreGive(gFS_sem);
    #endif
    #endif
}

#endif

//--------------------------------------------------------------------------------------------------------------------------------
INT32U combin_reg_data(INT8U *data, INT32S len)
{
	INT32S i;
	INT32U value = 0;

	for (i=0;i<len;i++) {
		value |= data[i] << i*8;
	}
	return value;
}

INT16U name2index(INT8U *name)
{
    INT16U i, src, dest;
    INT8U value = 0;

    //print_string("RS_TABLE_SIZE = %d\r\n", RS_TABLE_SIZE);
    for(i = 0 ; i < RS_TABLE_SIZE; ++i)
    {
        src  = 0;
        dest = 0;

        while(dest < RS_FILE_LEN)
        {
            //print_string("RS_TABLE[%d].name[%d] : %x\r\n",i,dest,RS_TABLE[i].name[dest]);
            if(RS_TABLE[i].name[dest] == 0x00) { return i; }

            value = name[src++];
            if(value == 0x2E) {
                //print_string("%c\r\n",value);
                continue;
            }

            if(IsLower(value)) { value -= 0x20; }

            if(value != RS_TABLE[i].name[dest]) {
                break;
            }
            else {
                //print_string("%c\r\n",value);
            }
            dest++;
        }

        if(dest == RS_FILE_LEN) {
            return i;
        }
    }
    return (INT16U)-1;
}

//////////////////////////////////////////////////////////
//NV FILE SYSTEM L3
/////////////////////////////////////////////////////////
INT16U nv_open(INT8U *path)
{
    INT16U index;

    index = name2index(path);
    if (index == 0xffff) {
        DBG_PRINT ("nv_open retry fail~~~~~~!\r\n");
        return 0xFFFF;
    }
    RS_TABLE[index].offset = 0;
    return index;
}

INT32U nv_mcu_read(INT16S fd, INT32U buf, INT32U byte_size)
{
    INT32U ret;

    if((RS_TABLE[fd].offset + byte_size) >  (RS_TABLE[fd].size * 512))
        return 1;
    ret = nvmemory_rs_byte_load(RS_TABLE[fd].name, RS_TABLE[fd].offset, (INT32U *)buf, byte_size);
    if(!ret)
    {
        RS_TABLE[fd].offset += byte_size;
    }
    return ret;
}


#define READ_RETRY_COUNTS        3
INT32U nv_read(INT16S fd, INT32U buf, INT32U byte_size)
{
	INT16U fd_u = (INT16U)fd;
	INT32S retval = STATUS_OK, i;
	INT32U remain_byte_size = 0;

	//Check Parameter
	if(fd_u >= RS_TABLE_SIZE) {
		return -1;
	}

    if((RS_TABLE[fd_u].offset) >= ((RS_TABLE[fd_u].size) << 9)) {
        return -1;
    }

	//Do the job
	remain_byte_size = byte_size;

	if( (RS_TABLE[fd_u].offset + remain_byte_size) > (RS_TABLE[fd_u].size) << 9) {
		remain_byte_size = ((RS_TABLE[fd_u].size) << 9) - RS_TABLE[fd_u].offset;
		//print_string("byte_size %x, remain_byte_size %x, size %x, offset %x\r\n",byte_size,remain_byte_size,RS_TABLE[fd_u].size,RS_TABLE[fd_u].offset);
	}

	//DBG_PRINT("nr:0x%x, %d\r\n",(Resource_area_position + RS_TABLE[fd_u].pos + RS_TABLE[fd_u].offset) >> 9, remain_byte_size);
    #if SPIFC_RES
    retval = SPIFC_Flash_read_page((Resource_area_position + RS_TABLE[fd_u].pos + RS_TABLE[fd_u].offset), (INT8U*)buf, remain_byte_size);
    #else
    retval = drv_l2_spiflash_read_nbyte(SPI_READ_BUS_WIDTH,(Resource_area_position + RS_TABLE[fd_u].pos + RS_TABLE[fd_u].offset), (INT8U*)buf, remain_byte_size);
    #endif

	if(retval != STATUS_OK) {
        for(i = 0; i < READ_RETRY_COUNTS; i++)
        {
            #if SPIFC_RES
            retval = SPIFC_Flash_read_page((Resource_area_position + RS_TABLE[fd_u].pos + RS_TABLE[fd_u].offset), (INT8U*)buf, remain_byte_size);
            #else
            retval = drv_l2_spiflash_read_nbyte(SPI_READ_BUS_WIDTH,(Resource_area_position + RS_TABLE[fd_u].pos + RS_TABLE[fd_u].offset), (INT8U*)buf, remain_byte_size);
            #endif

            if(retval == STATUS_OK) break;
        }
	}
	RS_TABLE[fd_u].offset += remain_byte_size;
//_NV_READ_END:
	return STATUS_OK;
}

INT32U nv_lseek(INT16S fd, INT32S foffset, INT16S origin)
{
	INT32U targetaddr = 0;
	INT16U fd_u = (INT16U)fd;

	if(fd_u >= RS_TABLE_SIZE) {
		targetaddr = (INT32U)-1;
		goto _NV_LEEK_END;
	}

	if(origin == SEEK_SET) {
		targetaddr = foffset;
	}
	else if (origin == SEEK_CUR) {
		targetaddr = RS_TABLE[fd_u].offset + foffset;
	}
	else {
        //print_string("size %x, foffset %d\r\n",RS_TABLE[fd_u].size,foffset);
		targetaddr = (RS_TABLE[fd_u].size << 9) - foffset;
	}

	RS_TABLE[fd_u].offset = targetaddr;
_NV_LEEK_END:
	return  targetaddr;
}

INT32U nv_rs_size_get(INT16S fd)
{
    INT32U ret;

    nv_lseek(fd,0,SEEK_SET);
	ret = nv_lseek(fd,0,SEEK_END)+1;
	nv_lseek(fd,0,SEEK_SET);

    return ret;
}


//////////////////////////////////////////////////////////
//
//  Flash access(SPI/Nand/Nor Flash)
//
//////////////////////////////////////////////////////////
#if SPIFC_RES

INT32U nvmemory_init(void)
{
	if(sw_spifc_sem == NULL)
	{
        #if _OPERATING_SYSTEM == _OS_UCOS2
        sw_spifc_sem = OSSemCreate(1);
        #elif _OPERATING_SYSTEM == _OS_FREERTOS
        sw_spifc_sem = xSemaphoreCreateMutex();
        #endif
	}

    drv_l1_spifc_init(18500000);
    //drv_l1_spifc_init(22200000);
    //drv_l1_spifc_init(27750000); // could work, but need to confirm timing by test with low high temperatue
    //drv_l1_spifc_init(37000000); // dont use, not yet adjust, it will FAIL
    //drv_l1_spifc_init(55500000); // dont use, not yet adjust, it will FAIL

    nvspiflash_retrieval();
    return 0;
}

#else

INT32U nvmemory_init(void)
{
    INT8U  data_buf[8];

    #if (C_NV_FLASH_TYPE == C_SPI_FLASH)
    {
        #if SPI_READ_BUS_WIDTH == IO_4BIT
        drv_l2_spiflash_init(13875000);
        //drv_l2_spiflash_init(27750000); // could work, but need to confirm timing by test with low high temperatue
        //drv_l2_spiflash_init(55500000); // dont use, not yet adjust, it will FAIL. SPI_RX_DMA_ONLY ok.
        #else
        drv_l2_spiflash_init(13875000);
        //drv_l2_spiflash_init(27750000); // could work, but need to confirm timing by test with low high temperatue
        //drv_l2_spiflash_init(55500000); // could work, but need to confirm timing by test with low high temperatue
        #endif

        drv_l2_spiflash_read_id(data_buf);//SPI_Flash_readID(data_buf);
        print_string("SPI ID %x %x %x\r\n",data_buf[0],data_buf[1],data_buf[2]);

        #if 1 && SPI_READ_BUS_WIDTH == IO_4BIT
        INT32S ret;
        ret = drv_l2_spi_set_quad_enable(1);//4-bit, some part number when 4 bit, no need to set this
        if(ret < 0) {
            DBG_PRINT("4 bit initial fail !\r\n");
        }
        #endif
        nvspiflash_retrieval();
    }
    #elif  (C_NV_FLASH_TYPE == C_NAND_FLASH)
    {
        if (Nand_Init())
            return 1;
        if (nvnandflash_retrieval())
            return 2;
    }
    #else
    { // C_NOR_FLASHY)
    }
    #endif

    //bypass boot area and calcular others areas's position
    return 0;
}

#endif

void nvmemory_uninit(void)
{
    #if SPIFC_RES == 1
    #else
    drv_l2_spiflash_uninit();
    #endif
}


INT32U nvmemory_rs_byte_load(INT8U *name ,INT32U offset_byte, INT32U *pbuf , INT32U byte_count)
{
    INT16U *tempbuffer;
    INT32U i, j;
    INT32U offset_secor, start_byte;
    INT32U ret,index;

    offset_secor = offset_byte / 512;
    //sector_count = (byte_count  >> 9) + 1;
    start_byte = offset_byte % 512;
    tempbuffer = (INT16U *)gp_malloc_align(2112 + 128, 64);
    if(!tempbuffer)
    {
        DBG_PRINT("nvmemory_rs_byte_load memloc fail\r\n");
        return 1;
    }

    //begin
    ret = nvmemory_rs_sector_load(name, offset_secor, (INT32U *)tempbuffer, 1);
    if(ret > 0)
    {
        gp_free((void *) tempbuffer);
        return 1;
    }

    index = 512 - start_byte;
    index = (byte_count > index) ? index : byte_count;
    for(i=0 ; i <index; i++)
        *((INT8U *)pbuf +i) = *((INT8U *)tempbuffer + start_byte + i);

    //mid
    byte_count -= index;
    while(byte_count > 0)
    {
        offset_secor++;
        ret = nvmemory_rs_sector_load(name, offset_secor, (INT32U *)tempbuffer, 1);
        if (ret >0)
        {
            gp_free((void *)tempbuffer);
            return 2;
        }
        if (byte_count >= 512) {
            for(j = 0; j < 512; j++)
                *((INT8U *)pbuf + j + index) = *((INT8U *)tempbuffer + j);
            index += 512;
            byte_count -= 512;
        } else{
            for(j = 0; j < byte_count; j++)
                *((INT8U *)pbuf + j + index) = *((INT8U *)tempbuffer + j);
            break;
        }
    }
    gp_free((void *)tempbuffer);
    return 0;
}

INT32U nvmemory_rs_sector_load(INT8U *name ,INT32U offset_secter, INT32U *pbuf , INT16U sector_count)
{
    INT16U itag ;

    itag = name2index(name);
    if (itag == 0xFFFF)     return 1;

    #if (C_NV_FLASH_TYPE == C_SPI_FLASH)
    return (nvspiflash_rs_get(itag, offset_secter, pbuf, sector_count));
    #else //(C_NV_FLASH_TYPE == C_NAND_FLASH){
    return (nvnandflash_rs_get(itag, offset_secter, pbuf, sector_count));
    #endif

    return 1;
}

INT32U nvmemory_user_sector_load(INT16U itag, INT32U *pbuf , INT16U sector_count)
{
    #if (C_NV_FLASH_TYPE == C_SPI_FLASH)
    return nvspiflash_user_get(itag, pbuf, sector_count);
    #else //if (C_NV_FLASH_TYPE == C_NAND_FLASH){
    return nvnandflash_user_get(itag, pbuf, sector_count);
    #endif

    return 1;
}

INT32U nvmemory_user_sector_store(INT16U itag, INT32U *pbuf , INT16U sector_count)
{
    #if (C_NV_FLASH_TYPE == C_SPI_FLASH)
    return nvspiflash_user_set(itag, pbuf, sector_count);
    #else //if (C_NV_FLASH_TYPE == C_NAND_FLASH){
    return nvnandflash_user_set(itag, pbuf, sector_count);
    #endif

    return 1;
}

///////////////////////////////////////////////////////
// SPI Flash
///////////////////////////////////////////////////////
#if C_NV_FLASH_TYPE == C_SPI_FLASH

#if SPIFC_RES
void sw_spifc_lock(void)
{
	#if _OPERATING_SYSTEM == _OS_UCOS2
		INT8U err;
		OSSemPend(sw_spifc_sem, 0, &err);
	#elif _OPERATING_SYSTEM == _OS_FREERTOS
		xSemaphoreTake(sw_spifc_sem, portMAX_DELAY);
	#endif
}

void sw_spifc_unlock(void)
{
	#if _OPERATING_SYSTEM == _OS_UCOS2
		OSSemPost(sw_spifc_sem);
	#elif _OPERATING_SYSTEM == _OS_FREERTOS
		xSemaphoreGive(sw_spifc_sem);
	#endif
}


INT32S SPIFC_Flash_erase_block(INT32U addr)
{
    INT32S ret = STATUS_OK;
	sw_spifc_lock();
	ret = drv_l1_spifcflash_block_erase(addr);
	sw_spifc_unlock();
    return ret;
}

#define TEMP_BUF_SIZE           (2048)

INT32S SPIFC_Flash_read_page(INT32U addr, INT8U *buf, INT32U byte_len)
{
    INT32S ret = STATUS_OK;
    INT32U align_byte;
    INT8U *ptr_byte;
    INT8U cmd_type = 0;
    INT8U *tmp_buf = NULL;

    sw_spifc_lock();

    #if SPI_READ_BUS_WIDTH == IO_1BIT
    cmd_type = SPIFC_READ_1_BIT_CMD_TYPE_FAST;
    #elif SPI_READ_BUS_WIDTH == IO_2BIT
    cmd_type = SPIFC_READ_2_BIT_CMD_TYPE_FAST;
    #elif SPI_READ_BUS_WIDTH == IO_4BIT
    cmd_type = SPIFC_READ_4_BIT_CMD_TYPE_FAST;
    #endif
    INT32U act_spi_clock_rate;

    drv_l1_spifc_clk_rate_get(&act_spi_clock_rate);
    drv_l1_spifc_set_timing_auto((((SPI_READ_BUS_WIDTH & 0x20) != 0) ? 1 : 0), (SPI_READ_BUS_WIDTH & ~0x20), act_spi_clock_rate);

    if((((INT32U)buf & 0x003f) == 0)
        && ((byte_len) % 64 == 0)
        && ((addr & 0x03) == 0)
    )
    {
        ret = drv_l1_spifcflash_read_nbyte(SPI_READ_BUS_WIDTH, addr, buf, byte_len, cmd_type);
        goto    Return;
    }

	tmp_buf = (INT8U*)gp_malloc_align(TEMP_BUF_SIZE, 64);
	if(tmp_buf == NULL)
	{
        ERR_PRINT("alloc tmp_buf fail\r\n");
        ret = STATUS_FAIL;
        goto    Return;
	}

    if(addr & 0x0003)
    {
        ret = drv_l1_spifcflash_read_nbyte(SPI_READ_BUS_WIDTH, addr & ~0x0003, tmp_buf, 4, cmd_type);
        align_byte = 4 - (addr & 0x0003);
        ptr_byte = tmp_buf + (addr & 0x0003);
        gp_memcpy((INT8S *)buf, (INT8S *)ptr_byte, (align_byte <= byte_len)? align_byte : byte_len);

        if(byte_len <= align_byte)
        {
            goto    Return;
        }

        byte_len -= align_byte;
        buf += align_byte;
    }
    addr += 0x0003;
    addr &= ~0x0003;

    while(byte_len > 0)
    {
        if(byte_len > TEMP_BUF_SIZE)
        {
            ret = drv_l1_spifcflash_read_nbyte(SPI_READ_BUS_WIDTH, addr, tmp_buf, TEMP_BUF_SIZE, cmd_type);
            gp_memcpy((INT8S *)buf, (INT8S *)tmp_buf, TEMP_BUF_SIZE);
            addr += TEMP_BUF_SIZE;
            buf += TEMP_BUF_SIZE;
            byte_len -= TEMP_BUF_SIZE;
        }
        else
        {
            ret = drv_l1_spifcflash_read_nbyte(SPI_READ_BUS_WIDTH, addr, tmp_buf, byte_len, cmd_type);
            gp_memcpy((INT8S *)buf, (INT8S *)tmp_buf, byte_len);
            byte_len = 0;
        }
    }

Return:
	if(tmp_buf)
	{
        gp_free((void *)tmp_buf);
        tmp_buf = NULL;
	}

    sw_spifc_unlock();
    return ret;
}

INT32S SPIFC_Flash_write_page(INT32U addr, INT8U *buf)
{
    INT32S ret = STATUS_OK;

    if(addr & 0x003f)
    {
        DBG_PRINT("[ERR]:write spifc address is not 64-alignment!!");
    }

	sw_spifc_lock();
	ret = drv_l1_spifcflash_write_page(IO_1BIT, addr, buf, 256);
	sw_spifc_unlock();
    return ret;
}
#endif

//retrieval from flash
INT32U nvspiflash_retrieval(void)
{
    INT32U j;
	INT8U *tmpHeaderBuffer = NULL;
	INT8U *pHeaderBuffer = NULL;
	INT16U rs_header_size;
	INT16U temp_header_size;
	INT16U i;
	INT8U* pFileName = NULL;
	INT8U* pFileOffset = NULL;
	INT32S retval = 0;
	//INT32U dataZoneStartByte;
	INT32U app_start_offset;

	rs_header_size = SPI_APP_HDR_SIZE;
	pHeaderBuffer = (INT8U*)gp_malloc_align(rs_header_size, 64);
	if(pHeaderBuffer == (INT8U*)NULL)
	{
		DBG_PRINT("spi malloc buffer failed\r\n");
		return 1;
	}

    #if SPIFC_RES

    INT32U spifc_tag;
    INT32U block_size = 64UL << 10; // 64K
    INT32U bootloader_size_in_byte;
    spifc_tag = SPIFC_READ32(C_SPIFC_HEADER_TAG_OFFSET >> 2);
    if (spifc_tag != C_SPIFC_HEADER_TAG_SPINOR) {
        DBG_PRINT("SPIFC Header Tag is wrong 0x%08x\r\n", spifc_tag);
		retval = 2;
		goto _NV_NAND_RETRIEVAL_END;
    }
    bootloader_size_in_byte = SPIFC_READ32(C_SPIFC_HEADER_CODE_SIZE_OFFSET >> 2);
    app_start_offset = (bootloader_size_in_byte + block_size - 1)/block_size * block_size;
    DBG_PRINT("SPIFC app_start_offset %x\r\n", app_start_offset);
   	Resource_area_position = app_start_offset;
    SPIFC_Flash_read_page(app_start_offset, pHeaderBuffer, rs_header_size);

    #else

	drv_l2_spiflash_read_nbyte(SPI_READ_BUS_WIDTH,0, pHeaderBuffer, 128);

	if(gp_strncmp((INT8S *)SPI_FLASH_TAG, (INT8S *)&pHeaderBuffer[SPI_HDR_TAG_ID], SPI_HDR_TAG_SIZE) != 0)
	{
		DBG_PRINT("Read SPI Header Failed. 0x%08x\r\n", *((INT32U *)&pHeaderBuffer[SPI_HDR_TAG_ID]));
		retval = 2;
		goto _NV_NAND_RETRIEVAL_END;
	}

    app_start_offset = combin_reg_data(&pHeaderBuffer[SPI_BTLDR_SIZE_ID], 2);
    app_start_offset = ((app_start_offset + 0x7F)&~0x7F) << 9;

   	print_string("SPI app_start_offset %x\r\n",app_start_offset);
   	Resource_area_position = app_start_offset;

    drv_l2_spiflash_read_nbyte(SPI_READ_BUS_WIDTH, app_start_offset, pHeaderBuffer, rs_header_size);

    #endif

    if(gp_strncmp((INT8S *)SPI_APP_HDR_TAG, (INT8S*)(&pHeaderBuffer[SPI_APP_HDR_TAG_ID]), SPI_APP_HDR_TAG_SIZE) != 0)
	{
        #if SPIFC_RES
		DBG_PRINT("Read SPIFC App Header Failed. 0x%08x\r\n", *((INT32U *)&pHeaderBuffer[SPI_HDR_TAG_ID]));
		#else
		DBG_PRINT("Read SPI App Header Failed. 0x%08x\r\n", *((INT32U *)&pHeaderBuffer[SPI_HDR_TAG_ID]));
		#endif

		retval = 3;
		goto _NV_NAND_RETRIEVAL_END;
	}

    //Parse App Header and fill the struct RS_TABLE[]
	RS_TABLE_SIZE = (INT16U)combin_reg_data(pHeaderBuffer + RS_FILE_NUM_OFFSET, RS_FILE_NUM_LEN);
	print_string("RS_TABLE_SIZE %d\r\n", RS_TABLE_SIZE);
	RS_TABLE_SIZE += 1; // so that to look at end FDB
	//total_rs = RS_TABLE_SIZE;
	if(RS_TABLE != NULL)
	{
		gp_free((void *) RS_TABLE);
		RS_TABLE = NULL;
	}

	RS_TABLE = (FLASHRSINDEX *)gp_malloc(RS_TABLE_SIZE * sizeof(FLASHRSINDEX));
	if(RS_TABLE == NULL)
	{
		retval = 4;
		goto _NV_NAND_RETRIEVAL_END;
	}

	gp_memset((INT8S *)RS_TABLE, 0x00, RS_TABLE_SIZE * sizeof(FLASHRSINDEX));
	temp_header_size = RS_FILE_NAME_OFFSET + (RS_TABLE_SIZE + 1) * SPI_FILE_INFO_LEN;
	if( temp_header_size > rs_header_size ){
		tmpHeaderBuffer = (INT8U*)gp_malloc_align((temp_header_size + 63)/64*64, 64);
		if(tmpHeaderBuffer == NULL){
			retval = 5;
			goto _NV_NAND_RETRIEVAL_END;
		}
        gp_free(pHeaderBuffer);
		rs_header_size = temp_header_size;
		pHeaderBuffer  = tmpHeaderBuffer;

        #if SPIFC_RES
        SPIFC_Flash_read_page(app_start_offset, pHeaderBuffer, rs_header_size);
        #else
        drv_l2_spiflash_read_nbyte(SPI_READ_BUS_WIDTH, app_start_offset, pHeaderBuffer, rs_header_size);
        #endif
	}

   	/*
   	retval = calc_spi_data_zone_byte_offset(pHeaderBuffer, rs_header_size, &dataZoneStartByte);
    if (retval != 0)
        goto _NV_NAND_RETRIEVAL_END;
    */

   	pFileName = pHeaderBuffer + RS_FILE_NAME_OFFSET;
   	pFileOffset = pHeaderBuffer + RS_FILE_START_OFFSET;

	for(i = 0; i < RS_TABLE_SIZE; i++)
	{
		for (j = 0; j < RS_FILE_LEN; j++) {
			RS_TABLE[i].name[j] = pFileName[j];
			if(pFileName[j] == 0x00){
				break;
			}
		}

		RS_TABLE[i].pos = (INT32U)combin_reg_data(pFileOffset, RS_FILE_START_LEN) << 9;
		RS_TABLE[i].offset = 0;

		if(i != 0) {
			RS_TABLE[i - 1].size = (RS_TABLE[i].pos - RS_TABLE[i - 1].pos) >> 9;
		}

		pFileName   += RS_FILE_INFO_LEN;
		pFileOffset += RS_FILE_INFO_LEN;
	}

    /*
	if ((RS_TABLE[RS_TABLE_SIZE - 1].name[0] == 0xFF) && (RS_TABLE[RS_TABLE_SIZE - 1].name[1] == 0x00)) {
        if(RS_TABLE[RS_TABLE_SIZE - 1].pos > dataZoneStartByte)
            // normaly when there is end FDB, it pos should less than dataZoneStartByte
            // if it greater than dataZoneStartByte, use dataZoneStartByte to calculate size
            RS_TABLE[RS_TABLE_SIZE - 1].size = (dataZoneStartByte - RS_TABLE[RS_TABLE_SIZE - 1].pos) >> 9;
        RS_TABLE_SIZE -= 1; // ignore end of FDB, it purpose is just use to calculate last FDB actual size
    }
	else
        RS_TABLE[RS_TABLE_SIZE - 1].size = (dataZoneStartByte - RS_TABLE[RS_TABLE_SIZE - 1].pos) >> 9;
    */

    User_area_position = RS_TABLE[RS_TABLE_SIZE - 1].pos;   //end_FDB start pos

    RS_TABLE_SIZE -= 1; //ignore end_FDB
    /*
    for(i = 0 ; i < RS_TABLE_SIZE; i++)
    {
    	DBG_PRINT("File %s\tpos 0x%08x\tsize 0x%08x\toffset 0x%08x\r\n", RS_TABLE[i].name, RS_TABLE[i].pos, RS_TABLE[i].size, RS_TABLE[i].offset);
    }
*/
    retval = 0;

_NV_NAND_RETRIEVAL_END:
	if(pHeaderBuffer != NULL) {
		gp_free(pHeaderBuffer);
		pHeaderBuffer = NULL;
	}

	if(retval != 0) {
		if(RS_TABLE != NULL) {
			gp_free(RS_TABLE);
			RS_TABLE = NULL;
		}

		RS_TABLE_SIZE = 0;
	}

	return retval;
}

INT32U nvspiflash_rs_get(INT16U itag, INT32U offset_secter, INT32U *pbuf, INT16U sector_count)
{
    INT16U i;
    INT16U max_sec_count, read_sec_count;
    INT32U start_addr_sec;

    //checking rs tag
    if(itag > RS_TABLE_SIZE)  return 1;

    max_sec_count = RS_TABLE[itag].size; //RS_TABLE[itag+1].pos - RS_TABLE[itag].pos;
    start_addr_sec = RS_TABLE[itag].pos + Resource_area_position;

    //Add offset
    start_addr_sec += offset_secter;
    read_sec_count = ((max_sec_count - offset_secter) < sector_count) ? (max_sec_count - offset_secter) : sector_count;
    for(i = 0; i < (read_sec_count << 1); i++)
    {
        #if SPIFC_RES
        SPIFC_Flash_read_page(start_addr_sec * 512 + 256 * i, (INT8U *)pbuf + 256 * i, 256);
        #else
        if (drv_l2_spiflash_read_page(SPI_READ_BUS_WIDTH,(start_addr_sec * 512) + (256 * i), (INT8U *)pbuf + 256 * i ) != STATUS_OK)
        return  2;
        #endif
    }
    return 0;
}

#if 1
//Each stored data is fixed as 512 bytes (1 sector) and is stored secter by secter on FLASH
INT32U nvspiflash_user_set(INT16U itag, INT32U *pbuf, INT16U sector_count)
{
    INT32U base_adr, i, j, found;

    if(sector_count > 0x80)    return 1;

    if((!User_area_position) || (!Resource_area_position))
    return 1;

    //Get Base address
    base_adr = (User_area_position + Resource_area_position); //64K per block

    if(base_adr & 0xffff)
    {
        base_adr += 0x10000;
    }
    base_adr &= 0xffff0000;
    base_adr >>= 9;        //Unit: sector(each is 512Byte)

    /*
    i = base_adr & (0x80 - 1);
    base_adr = base_adr & ~(0x80 -1);
    if(i) base_adr += 0x80; //bounary as 64K (ERASE UNIT)
    */

    base_adr += (itag * 0x80 * USER_SPI_BLK);

    //Get the least empty secter
    found = 0;
    for(j = 0; j < USER_SPI_BLK; j++)
    {
        #if (USER_SPI_AREA_SIZE == USER_SPI_512_B)
        for(i = 0; i < 0x80; i++)
        {
            #if SPIFC_RES
            SPIFC_Flash_read_page((base_adr + i) * 512, (INT8U *)tbuf, 256);
            #else
            if (drv_l2_spiflash_read_page(IO_1BIT, (base_adr + i) * 512, (INT8U *)tbuf ) != STATUS_OK)
            return 1;
            #endif

            if((tbuf[0] == 0xffffffff) && (tbuf[1] == 0xffffffff))
            {
                found = 1;
                break;
            }
        }
        #else
        for(i = 0; i < 0x100; i++)
        {
            #if SPIFC_RES
            SPIFC_Flash_read_page(base_adr * 512 + i * 256, (INT8U *)tbuf);
            #else
            if (drv_l2_spiflash_read_page((base_adr * 512) + (i * 256), (INT8U *)tbuf) != STATUS_OK)
                return 1;
            #endif

            if((tbuf[0] == 0xffffffff) && (tbuf[1] == 0xffffffff))
            {
                found = 1;
                break;
            }
        }
        #endif

        if(found)  break;
        base_adr += 0x80;
    }

    if((found) && ((128 - i) > sector_count))
    {
        if(itag == 0)
        {
            #if ( USER_SPI_AREA_SIZE == USER_SPI_512_B)
            for(j = 0; j < 2; j++)
            {
                #if SPIFC_RES
                if(SPIFC_Flash_write_page(((base_adr + i) * 512 + 256 * j) , (INT8U *)pbuf + 256 * j) != STATUS_OK)
                    return 1;
                #else
                if(drv_l2_spiflash_write_page(IO_1BIT, (((base_adr + i) * 512 ) + 256 * j), (INT8U *)pbuf + 256 * j) != STATUS_OK)
                    return 1;
                #endif
            }
            #else
                #if SPIFC_RES
                if(SPIFC_Flash_write_page(base_adr * 512 + i * 256, (INT8U *)pbuf) != STATUS_OK)
                    return 1;
                #else
                if(SPI_Flash_write_page((base_adr * 512) + (i * 256), (INT8U *)pbuf) != STATUS_OK)
                    return 1;
                #endif
            #endif
        }else
        {
            for(j = 0; j < (sector_count << 1); j++)
            {
                #if SPIFC_RES
                if(SPIFC_Flash_write_page((base_adr + i) * 512 + 256 * j , (INT8U *)pbuf + 256 * j ) != STATUS_OK)
                    return 1;
                #else
                if(drv_l2_spiflash_write_page(IO_1BIT, (((base_adr+i) * 512 ) + 256 * j) , (INT8U *)pbuf + 256 * j) != STATUS_OK)
                    return 1;
                #endif
            }
        }
    }
    else
    {
        if((found == 1) && ((128 - i) <= sector_count))
        {
            base_adr += 0x80;
        }

        //Erase All of blocks
        for(i = 0; i < USER_SPI_BLK; i++) {
            #if SPIFC_RES
            SPIFC_Flash_erase_block((base_adr - 0x80 - i * 0x80) * 512);
            #else
            drv_l2_spiflash_block_erase((base_adr - 0x80 - (i * 0x80)) * 512);//SPI_Flash_erase_block( (base_adr - 0x80 - (i* 0x80)) * 512 );
            #endif
        }

        //Write at the first page
        base_adr = base_adr - (0x80 * USER_SPI_BLK);
        if(itag == 0)
        {
            #if (USER_SPI_AREA_SIZE == USER_SPI_512_B)
            for (j = 0; j < 2; j++)
            {
                #if SPIFC_RES
                if(SPIFC_Flash_write_page(base_adr * 512 + 256 * j, (INT8U *)pbuf + 256 * j) != STATUS_OK)
                    return 1;
                #else
                if(drv_l2_spiflash_write_page(IO_1BIT, ((base_adr * 512 ) + (256 * j)) , (INT8U *)pbuf + 256 * j) != STATUS_OK)
                    return 1;
                #endif
            }
            #else
                #if SPIFC_RES
                if(SPIFC_Flash_write_page(base_adr * 512 + i * 256 + 256 * j, (INT8U *)pbuf + 256 * j) != STATUS_OK)
                    return 1;
                #else
                if(drv_l2_spiflash_write_page( (base_adr*512) + (i* 256)+ (256*j) , (INT8U *)pbuf +256*j  ) != STATUS_OK)
                r   eturn 1;
                #endif
            #endif
        }else
        {
            for(j = 0; j < (sector_count << 1); j++)
            {
                #if SPIFC_RES
                if(SPIFC_Flash_write_page((base_adr + i) * 512 + 256 * j, (INT8U *)pbuf + 256 * j) != STATUS_OK)
                    return 1;
                #else
                if(drv_l2_spiflash_write_page(IO_1BIT, (((base_adr + i) * 512) + 256 * j), (INT8U *)pbuf + 256 * j) != STATUS_OK)
                    return 1;
                #endif
            }
        }
    }
    return 0;
}

INT32U nvspiflash_user_get(INT16U itag, INT32U *pbuf, INT16U sector_count)
{
    INT32U  base_adr, i, j, found;

    if(sector_count > 0x80) return 1;

    base_adr = (User_area_position + Resource_area_position); //64K per block

    if(base_adr & 0xffff)
    {
        base_adr += 0x10000;
    }
    base_adr &= 0xffff0000;
    base_adr >>= 9;        //Unit: sector(each is 512Byte)

    /*
    i = base_adr & (0x80 - 1);
    base_adr = base_adr & ~(0x80 - 1);
    if(i) base_adr += 0x80; //bounary as 64K
    */

    base_adr += (itag * 0x80 * USER_SPI_BLK);
    found = 0;
    for(j = 0; j < USER_SPI_BLK; j++)
    {
        #if (USER_SPI_AREA_SIZE == USER_SPI_512_B)
        for (i = 0; i < 0x80; i++)
        {
            #if SPIFC_RES
            SPIFC_Flash_read_page((base_adr + i) * 512 , (INT8U *)tbuf, 256);
            #else
            if (drv_l2_spiflash_read_page(IO_1BIT, (base_adr + i) * 512, (INT8U *)tbuf) != STATUS_OK)
                return 1;
            #endif

            if((tbuf[0] == 0xffffffff) && (tbuf[1] == 0xffffffff))
            {
                found = 1;
                break;
            }
        }
        #else
        for(i = 0; i < 0x100; i++)
        {
            #if SPIFC_RES
            SPIFC_Flash_read_page(base_adr * 512 + i * 256, (INT8U *)tbuf);
            #else
            if (drv_l2_spiflash_read_page((base_adr * 512) + (i * 256), (INT8U *)tbuf) != STATUS_OK)
                return 1;
            #endif

            if((tbuf[0] == 0xffffffff) && (tbuf[1] == 0xffffffff))
            {
                found = 1;
                break;
            }
        }
        #endif

        if(found)   break;

        base_adr += 0x80;
    }

    if(found)
    {
        if(i == 0) //checking boundary
        {
            if(j == 0) return 0;
            else
            {
                base_adr = base_adr - 0x80;
                i = 0x80;
            }
        }

        if(itag == 0)
        {
            #if ( USER_SPI_AREA_SIZE == USER_SPI_512_B)
            for(j = 0; j < 2; j++)
            {
                #if SPIFC_RES
                SPIFC_Flash_read_page((base_adr + i - 1) * 512 + 256 * j, (INT8U *)pbuf + 256 * j, 256);
                #else
                if (drv_l2_spiflash_read_page(IO_1BIT, ((base_adr + i - 1) * 512) + 256 * j  , (INT8U *)pbuf + 256 * j) != STATUS_OK)
                    return 1;
                #endif
            }
            #else
                #if SPIFC_RES
                SPIFC_Flash_read_page(base_adr * 512 + (i - 1) * 256, (INT8U *)pbuf);
                #else
                if (SPI_Flash_read_page((base_adr * 512) + ((i - 1) * 256), (INT8U *)pbuf) != STATUS_OK)
                    return 1;
                #endif
            #endif
        }else
        {
            for(j = 0; j < (sector_count << 1); j++)
            {
                #if SPIFC_RES
                SPIFC_Flash_read_page((base_adr + i - 1) * 512 + 256 * j, (INT8U *)pbuf + 256 * j, 256);
                #else
                if (drv_l2_spiflash_read_page(IO_1BIT, ((base_adr + i - 1) * 512) + 256 * j, (INT8U *)pbuf + 256 * j) != STATUS_OK)
                    return 1;
                #endif
            }
        }
        return 0;
    }
    return 1;
}

#else

// Tag: 0~4  each slot is  64k (0x10000) ( 0x80  sectors )
INT32U nvspiflash_user_set (INT16U itag ,INT32U *pbuf , INT16U sector_count)
{
    INT32U base_adr,i;

    if(sector_count > 0x80) return 1;

    base_adr = (User_area_position + Resource_area_position); //64K per sector

    i = base_adr & (0x80 - 1);
    base_adr = base_adr & ~(0x80 - 1);
    if(i) base_adr += 0x80;

    base_adr += (itag * 0x80);
    SPI_Flash_erase_block(base_adr * 512 );

    for(i = 0; i < (sector_count << 1); i++)
    {
        if(drv_l2_spiflash_write_page((base_adr * 512 ) + (i << 8), (INT8U *)pbuf + 256 * i) != STATUS_OK)
            return 1;
    }
    return 0;
}
INT32U nvspiflash_user_get (INT16U itag ,INT32U *pbuf , INT16U sector_count)
{
    INT32U base_adr,i;

    base_adr = (User_area_position + Resource_area_position); //64K per sector

    i = base_adr & (0x80 - 1);
    base_adr = base_adr & ~(0x80 - 1);
    if(i) base_adr += 0x80;

    base_adr += (itag * 0x80);
    for(i = 0; i < (sector_count << 1); i++)
    {
        if(drv_l2_spiflash_read_page(SPI_READ_BUS_WIDTH, (base_adr * 512) + (i << 8), (INT8U *)pbuf + 256 * i) != STATUS_OK)
            return 1;
    }
    return 0;
}
#endif

#elif (C_NV_FLASH_TYPE == C_NAND_FLASH)

///////////////////////////////////////////////////////
//NAND Flash
///////////////////////////////////////////////////////

//retrieval from nand flash
// BOOT_AREA_START & BOOT_AREA_SIZE	are defined at L2 nand manager

//////////////////////////////////////////////////////////////////////////////////////////
INT32U nvnandflash_rs_get(INT16U itag ,INT32U offset_secter, INT32U *pbuf , INT16U sector_count)
{
   INT32U ret;
   INT16U max_sec_count,read_sec_count;
   INT32U start_addr_sec;
   //ALIGN16 INT16U    PageBuffer[1056];

   if ( itag > RS_TABLE_SIZE )  return 1;
   max_sec_count  =  RS_TABLE[itag].size;
   start_addr_sec =  RS_TABLE[itag].pos   + Resource_area_position;
      //Add offset
   start_addr_sec += offset_secter;
   read_sec_count = ( (max_sec_count-offset_secter) < sector_count) ?  (max_sec_count-offset_secter) : sector_count ;
   NVNAND_OS_LOCK();
   ret = NandBootReadSector(start_addr_sec,read_sec_count,(INT32U)pbuf);
   NVNAND_OS_UNLOCK();
   if (ret) {
	   return 1;
   } else {
	   return 0;
   }
}

//sector_count must be fixed
//pbuf_align_16 must been align 16 bytes
INT32U nvnandflash_user_get(INT16U itag ,INT32U *pbuf_align_16 , INT16U sector_count)
{
   INT32U ret;
   NVNAND_OS_LOCK();

   //ret = NandBootReadSector((User_area_position  +Resource_area_position),sector_count,(INT32U)pbuf_align_16);
   if (itag==0)
     ret= DrvNand_read_sector(0,1,(INT32U)pbuf_align_16);
   else
     ret= DrvNand_read_sector(itag,sector_count,(INT32U)pbuf_align_16);
   NVNAND_OS_UNLOCK();
   if (ret) {
	   return 1;
   } else {
	   return 0;
   }

}

//sector_count must be fixed
//pbuf must been align 16 bytes
INT32U nvnandflash_user_set(INT16U itag ,INT32U *pbuf_align_16 , INT16U sector_count)
{
   INT32U ret;

   NVNAND_OS_LOCK();
   NandBootEnableWrite();
  //ret = NandBootWriteSector((User_area_position  +Resource_area_position),sector_count,(INT32U)pbuf_align_16);
   if (itag==0)
	   ret= DrvNand_write_sector(0,1,(INT32U)pbuf_align_16);
   else
       ret= DrvNand_write_sector(itag,sector_count,(INT32U)pbuf_align_16);
   NandBootFlush();
   NandBootDisableWrite();
   NVNAND_OS_UNLOCK();
   if (ret){
	   return 1;
   } else {
       NVNAND_OS_LOCK();
       NandBootFlush();
       NVNAND_OS_UNLOCK();
	   return 0;
   }
}

#endif//#if C_NV_FLASH_TYPE == C_SPI_FLASH

#ifndef __DRV_l2_SPI_FLASH_H__
#define __DRV_l2_SPI_FLASH_H__

#define SPI_FLASH_BUSY		(-1)
#define SPI_FLASH_TIMEOUT	(-2)

#define MODE_1BIT           0
#define MODE_2BIT           1
#define MODE_4BIT           2
#define SPI_MODE	        MODE_1BIT

#if (SPI_MODE ==  MODE_1BIT)
#define MODE_1BIT_BY_4BIT_PINMUX   1
#endif

// spi flash status bit
#define SPI_FLASH_SR_WIP	(1 << 0)
#define SPI_FLASH_SR_WEL	(1 << 1)
#define SPI_FLASH_SR_QE 	(1 << 6)

//spi flash header
#define SPI_FLASH_TAG       	"PGpssiipps"
#define SPI_HDR_TAG_ID			(0)
#define SPI_HDR_TAG_SIZE		(10)  //Bytes
#define SPI_HDR_TAG_ID			(0)
#define SPI_BTLDR_SIZE_ID		(16)
#define SPI_APP_HDR_TAG			"GP"
#define SPI_APP_HDR_TAG_ID		(0)
#define SPI_APP_HDR_TAG_SIZE	(2) //Bytes
#define SPI_MAX_FILENAME_LEN	(19)
#define SPI_FILE_OFFSET_LEN		(4)
#define SPI_FILE_INFO_LEN		(SPI_MAX_FILENAME_LEN + SPI_FILE_OFFSET_LEN)
#define SPI_APP_HDR_SIZE		(512) //Bytes
#define SPI_RESERVE_ZONE_NAME	"RESERVEBLOCKBIN"
#define SPI_PAIRPAGE_TAG		"***** SPI BackUp PairPage *****"
#define SPI_PAGE_BIT_NUM		(12)//4KBytes
#define SPI_PAGE_MASK			(0xFFFUL)
#define SPI_SECTPR_BIT_NUM		(9)//512
#define SPI_BLOCK_BIT_NUM       (16)//64KBytes
#define SPI_PAGE_SIZE			(4096)

#define RESOURCE_TAG    		"GP"
#define RS_FILE_NUM_OFFSET		(2)
#define RS_FILE_NUM_LEN			(2)
#define RS_FILE_NAME_OFFSET		(4)
#define RS_FILE_START_OFFSET	(RS_FILE_NAME_OFFSET + SPI_MAX_FILENAME_LEN)
#define RS_FILE_START_LEN		(SPI_FILE_OFFSET_LEN)
#define RS_FILE_INFO_LEN		(SPI_FILE_INFO_LEN)

extern INT32S drv_l2_spiflash_init(INT32U speed);
extern INT32S drv_l2_spiflash_uninit(void);
extern INT32S drv_l2_spiflash_write_enable(void);
extern INT32S drv_l2_spiflash_write_disable(void);
extern INT32S drv_l2_spiflash_read_id(INT8U* ID);
extern INT32S drv_l2_spiflash_read_status(void);
extern INT32S drv_l2_spiflash_write_status(INT8U status);
extern INT32S drv_l2_spiflash_wait_status(void);
extern INT32S drv_l2_spiflash_sector_erase(INT32U addr);
extern INT32S drv_l2_spiflash_block_erase(INT32U addr);
extern INT32S drv_l2_spiflash_chip_erase(void);
extern INT32S drv_l2_spiflash_read_page(INT8U mio, INT32U addr, INT8U *buf);
extern INT32S drv_l2_spiflash_write_page(INT8U mio, INT32U addr, INT8U *buf);
extern INT32S drv_l2_spiflash_read_nbyte(INT8U mio, INT32U addr, INT8U *buf, INT32U nByte);
extern INT32S drv_l2_spiflash_write_nbyte(INT8U mio, INT32U addr, INT8U *buf, INT16U BufCnt);
extern INT32S drv_l2_spi_set_quad_enable(INT32U enable);
#endif	/*__drv_l1_SENSOR_H__*/

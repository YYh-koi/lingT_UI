#define CREAT_DRIVERLAYER_STRUCT

#include "fsystem.h"
#include "drv_l2.h"

#define SD_FS_ERR_PRINT(...)
//#define SD_FS_ERR_PRINT	FS_PRINT

#define SD_FS_PRINT(...)
//#define SD_FS_PRINT		FS_PRINT

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#if ((defined SD1_EN) && (SD1_EN == 1)) || ((defined SD_EN) && (SD_EN == 1))				//

//================================================================//
#define SD_RW_RETRY_COUNT	3
#define SDC_TAG0			0x73644750			// string: PGds
#define SDC_TAG1			0x64636373			// string: sccd
#define SDC_TAG2			0x7364				// string: ds
#define APP_TAG				0x5047				// string: GP
#define FS_CACHE_DBG_EN		0
#define FS_FLUSH_CAHCE_EN	1
#define FS_INFO_CACHE		1
#define DIR_INFO_CACHE		0
#define FAT_WRITE_TBL_ID	0					//0: FATTbl0->FATTbl1, 1: FATTbl1->FATTbl0
#if FAT_WRITE_TBL_ID == 0
#define FAT_MAIN_TBL_ID		0
#define FAT_SECOND_TBL_ID	1
#define FS_TBL_MIRROR_SYNC	0
#elif FAT_WRITE_TBL_ID == 1
#define FAT_MAIN_TBL_ID		1
#define FAT_SECOND_TBL_ID	0
#define FS_TBL_MIRROR_SYNC	1					// when FAT2->1 Mirror table must be enable
#endif

//--------------------------------------------------
#if DIR_INFO_CACHE
extern INT32S	start_sector_of_root;
extern INT32S	start_sector_of_dcim;
extern INT32S	start_sector_of_dcima;
extern INT32S	start_sector_of_dcimb;

INT8U			dir_loaded_flag = 0;
INT8U			dcima_dirty_flag = 0;
INT8U			dcimb_dirty_flag = 0;

#define ROOT_CACHE_BYTE_SIZE	512
#define DCIM_CACHE_BYTE_SIZE	512
#define DCIMA_CACHE_BYTE_SIZE	32768
#define DCIMB_CACHE_BYTE_SIZE	32768

ALIGN32 INT8U	root_cache_ram[ROOT_CACHE_BYTE_SIZE];
ALIGN32 INT8U	dcim_cache_ram[DCIM_CACHE_BYTE_SIZE];
ALIGN32 INT8U	dcima_cache_ram[DCIMA_CACHE_BYTE_SIZE];
ALIGN32 INT8U	dcimb_cache_ram[DCIMB_CACHE_BYTE_SIZE];

INT32S			root_cache_size = 0;
INT32S			dcim_cache_size = 0;
INT32S			dcima_cache_size = 0;
INT32S			dcimb_cache_size = 0;
#endif

//--------------------------------------------------
#if FS_FLUSH_CAHCE_EN
#define CACHE_FAT_TABLE_SYNC	0x80
#define CACHE_FAT_TABLE_HIT		0x40
#define CACHE_FAT_TABLE_INIT	0x20
#define CACHE_FAT_TABLE_MISS	0x10

#define CACHE_FAT_TABLE_CNT		10//4
//#define CACHE_FAT_TABLE_CNT		8

typedef struct
{
	INT32U	cache_sector_idx;
	INT32U	cache_addrs;
	INT32U	cache_time;
	INT8U	dirty_flag;
	INT8U	is_used_flag;
} cache_info;

cache_info	cache_fat_table[CACHE_FAT_TABLE_CNT] = { {0} };
#else
#define CACHE_FAT_TABLE_CNT 1
#endif

//--------------------------------------------------
//#define SDC_CACHE_SIZE			(32 * 1024)
#define SDC_CACHE_SECTOR_NUMS	(SDC_CACHE_SIZE / 512)
ALIGN64 INT8U	sd_cache_ram[SDC_CACHE_SIZE * CACHE_FAT_TABLE_CNT];

//--------------------------------------------------
#if (FS_INFO_CACHE == 1 || FAT_WRITE_TBL_ID == 1)
INT8U			fsinfo_loaded_flag = 0;
INT8U			fsinfo_dirty_flag = 0;
ALIGN64 INT8U	sd_fs_info_ram[512];
#endif

//--------------------------------------------------
INT32U			mirror_start_id;				// unit: sector
INT32U			mirror_end_id;					// unit: sector
INT8U			cache_init_flag = 0;			// cache has initialized, its value is set to 0xA8
INT8U			fat_cache_L1_en;				// if cache is intialized successfully, it will be set to 1
INT32U			fat_cluster_size;				// unit: sectors
INT32U			fat_tbl_size;					// unit: sectors
INT32U			fat_start_id[2];
static INT8U	fat_type;						//FAT16 if its value is 16, or FAT32 if its value is 32
static INT32U	fs_info_start;					//sector no of FSINFO
INT32U			BPB_Start_id;					//the first sector no of DOS partition

//--------------------------------------------------
#if FAT_WRITE_TBL_ID == 1
static INT32U	FSI_Free_Count = 0;
static INT32U	FSI_Next_Free = 0;
#endif

//--------------------------------------------------
static INT32S	fat_l1_cache_init(INT32U device_id);
void			fat_l1_cache_reinit(void);
void			fat_l1_cache_uninit(void);

static void		FsCpy4(void *_dst, const void *_src, int len);

#if DIR_INFO_CACHE
INT32S			dir_cache_init(INT32U device_id);
INT32S			dir_cache_flush(INT32U device_id);
#endif
#if FS_INFO_CACHE == 1
static INT32S	fs_info_flush(INT32U device_id);
#endif
#if FAT_WRITE_TBL_ID == 1						//FAT2->FAT1
static INT32S	fsinfo_sync(INT32U devide_id);	// dominant fat2->fat1
static INT32S	fat_tbl_mirror_init(INT32U device_id);
static INT32U	fat_tbl_tansfer(INT32U blkno);
#endif
#if FS_FLUSH_CAHCE_EN
INT8U find_cache_fat_table_for_read(INT32U sectorIdx)
{
	INT8U	i;
#if _OPERATING_SYSTEM != _OS_NONE
#if _OPERATING_SYSTEM == _OS_UCOS2
	INT32U	max_time_range = OSTimeGet();		// 10ms per tick
#elif _OPERATING_SYSTEM == _OS_FREERTOS
	INT32U	max_time_range = xTaskGetTickCount();
#endif
#endif
	INT8U	select_idx = 0;

	for(i = 0; i < CACHE_FAT_TABLE_CNT; i++)
	{
		// Return unused cache fat table
		if(cache_fat_table[i].is_used_flag == 0)
		{
			return(i | CACHE_FAT_TABLE_INIT);
		}

		// When all table is full, find the oldest cache time of cache fat table to will replace it later
#if _OPERATING_SYSTEM != _OS_NONE
		if(max_time_range >= cache_fat_table[i].cache_time)
		{
			max_time_range = cache_fat_table[i].cache_time;
			select_idx = i;
		}

#endif

		// BIGO! Return cache fat table	index
		if((sectorIdx >= cache_fat_table[i].cache_sector_idx) && (sectorIdx < (cache_fat_table[i].cache_sector_idx + SDC_CACHE_SECTOR_NUMS)))
		{
			return(i | CACHE_FAT_TABLE_HIT);
		}
	}

	return(select_idx | CACHE_FAT_TABLE_MISS);
}

INT8U find_cache_fat_table_for_write(INT32U sectorIdx)
{
	INT8U	i;
#if _OPERATING_SYSTEM != _OS_NONE
#if _OPERATING_SYSTEM == _OS_UCOS2
	INT32U	max_time_range = OSTimeGet();		// 10ms per tick
#elif _OPERATING_SYSTEM == _OS_FREERTOS
	INT32U	max_time_range = xTaskGetTickCount();
#endif
#endif
	INT8U	select_idx = 0;

	for(i = 0; i < CACHE_FAT_TABLE_CNT; i++)
	{
		if(cache_fat_table[i].is_used_flag == 0)
		{
			return(i | CACHE_FAT_TABLE_INIT);
		}

#if _OPERATING_SYSTEM != _OS_NONE
		if(max_time_range >= cache_fat_table[i].cache_time)
		{
			max_time_range = cache_fat_table[i].cache_time;
			select_idx = i;
		}

#endif
		if((sectorIdx >= cache_fat_table[i].cache_sector_idx) && (sectorIdx < (cache_fat_table[i].cache_sector_idx + SDC_CACHE_SECTOR_NUMS)))
		{
			return(i | CACHE_FAT_TABLE_HIT);
		}
	}

	return(select_idx | CACHE_FAT_TABLE_SYNC);
}

#endif

/****************************************************************************/
#if DIR_INFO_CACHE
INT32S dir_cache_init(INT32U device_id)
{
	INT32S	ret;

	if(dir_loaded_flag)
		return 0;

	if(fat_cache_L1_en && (start_sector_of_root != 0) && (start_sector_of_dcim != 0) && (start_sector_of_dcima != 0) && (start_sector_of_dcimb != 0))
	{
		root_cache_size = fat_cluster_size << 9;
		if(root_cache_size > ROOT_CACHE_BYTE_SIZE)
		{
			root_cache_size = ROOT_CACHE_BYTE_SIZE;
		}

		root_cache_size >>= 9;

		dcim_cache_size = fat_cluster_size << 9;
		if(dcim_cache_size > DCIM_CACHE_BYTE_SIZE)
		{
			dcim_cache_size = DCIM_CACHE_BYTE_SIZE;
		}

		dcim_cache_size >>= 9;

		dcima_cache_size = fat_cluster_size << 9;
		if(dcima_cache_size > DCIMA_CACHE_BYTE_SIZE)
		{
			dcima_cache_size = DCIMA_CACHE_BYTE_SIZE;
		}

		dcima_cache_size >>= 9;

		dcimb_cache_size = fat_cluster_size << 9;
		if(dcimb_cache_size > DCIMB_CACHE_BYTE_SIZE)
		{
			dcimb_cache_size = DCIMB_CACHE_BYTE_SIZE;
		}

		dcimb_cache_size >>= 9;

		ret = drvl2_sd_read(device_id, start_sector_of_root, (INT32U *) &root_cache_ram[0], root_cache_size);
		if(ret == 0)
		{
			ret = drvl2_sd_read(device_id, start_sector_of_dcim, (INT32U *) &dcim_cache_ram[0], dcim_cache_size);
			if(ret == 0)
			{
				ret = drvl2_sd_read(device_id, start_sector_of_dcima, (INT32U *) &dcima_cache_ram[0], dcima_cache_size);
				if(ret == 0)
				{
					ret = drvl2_sd_read(device_id, start_sector_of_dcimb, (INT32U *) &dcimb_cache_ram[0], dcimb_cache_size);
					if(ret == 0)
					{
						dir_loaded_flag = 1;
						dcima_dirty_flag = 0;
						dcimb_dirty_flag = 0;
						FS_PRINT("DIR cache init\r\n");
						return 0;
					}
				}
			}
		}
	}

	return -1;
}

INT32S dir_cache_flush(INT32U device_id)
{
	INT32S	ret = 0;

	if(dir_loaded_flag)
	{
		if(dcima_dirty_flag)
		{
			ret = drvl2_sd_write(device_id, start_sector_of_dcima, (INT32U *) &dcima_cache_ram[0], dcima_cache_size);
			dcima_dirty_flag = 0;
			FS_PRINT("DCIMA flush\r\n");
		}

		if(dcimb_dirty_flag)
		{
			ret = drvl2_sd_write(device_id, start_sector_of_dcimb, (INT32U *) &dcimb_cache_ram[0], dcimb_cache_size);
			dcimb_dirty_flag = 0;
			FS_PRINT("DCIMB flush\r\n");
		}
	}

	return ret;
}

#endif

/****************************************************************************/
void fat_l1_cache_uninit(void)
{
	fat_cache_L1_en = 0;

#if FS_INFO_CACHE == 1
	fsinfo_loaded_flag = 0;
#endif
#if DIR_INFO_CACHE
	dir_loaded_flag = 0;
	start_sector_of_root = 0;
	start_sector_of_dcim = 0;
	start_sector_of_dcima = 0;
	start_sector_of_dcimb = 0;
#endif
}

void fat_l1_cache_reinit(void)
{
	cache_init_flag = 0;

#if DIR_INFO_CACHE
	dir_loaded_flag = 0;
	start_sector_of_root = 0;
	start_sector_of_dcim = 0;
	start_sector_of_dcima = 0;
	start_sector_of_dcimb = 0;
#endif
}

INT32S fat_l1_cache_init(INT32U device_id)
{
	INT32S	ret;
	INT8U	*byte_buf;
	INT16U	*short_buf;
	INT32U	*word_buf;
#if FAT_WRITE_TBL_ID == 1
	INT32U	*fi_word_buf = (INT32U *) &sd_fs_info_ram[0];

	INT8U	mirror_en = 1;
	INT32U	Next_Free = 0xFFFFFFFF;
	INT32U	FS_info_tag = 0x00000000;
	INT32U	Free_Count = 0xFFFFFFFF;
#endif
	/*
#if FS_INFO_CACHE == 1
	INT32U	*fi_word_buf = (INT32U *) &sd_fs_info_ram[0];
#endif
#if FAT_WRITE_TBL_ID == 1
	INT8U	mirror_en = 1;
#endif*/
	if(cache_init_flag != 0xA8)
	{
#if FS_CACHE_DBG_EN == 1
		DBG_PRINT("\r\n SD FatL1 cache Init...");
#endif
		fs_info_start = 0xFFFFFFFF;

		BPB_Start_id = 0;						// NO MBR, BPB always is 0
		short_buf = (INT16U *) &sd_cache_ram[0];
		word_buf = (INT32U *) &sd_cache_ram[0];
		byte_buf = (INT8U *) &sd_cache_ram[0];

#if FS_FLUSH_CAHCE_EN
		for(ret = 0; ret < CACHE_FAT_TABLE_CNT; ret++)
		{
			cache_fat_table[ret].cache_sector_idx = 0;
			cache_fat_table[ret].dirty_flag = 0;
			cache_fat_table[ret].cache_time = 0;
			cache_fat_table[ret].is_used_flag = 0;
			cache_fat_table[ret].cache_addrs = (INT32U) (sd_cache_ram + (ret * SDC_CACHE_SIZE));
		}

#endif
		ret = drvl2_sd_read(device_id, BPB_Start_id, (INT32U *) &sd_cache_ram[0], 1);
		if(ret == 0 && short_buf[0x1fE / 2] == 0xAA55 && byte_buf[0] != 0xEB)
		{
			BPB_Start_id = ((short_buf[0x1C8 / 2] << 16) | short_buf[0x1C6 / 2]);
			FS_PRINT("MBR Find, first part (SectorId:%d,offset:%#8x)\r\n", BPB_Start_id , BPB_Start_id * 512);   // Sector=512, reference  "Api_fat_format.c"  bp[12] = 0x02;
			ret = drvl2_sd_read(device_id, BPB_Start_id, (INT32U *) &sd_cache_ram[0], 1);
		}

		if(ret != 0)
		{
			fat_cache_L1_en = 0;

			FS_PRINT("SD read fail 1\r\n");
			return -1;
		}
		else
		if(ret == 0)
		{
			fat_cache_L1_en = 1;

			fat_cluster_size = byte_buf[13];	// 8 sectors
			fat_start_id[0] = short_buf[7] + BPB_Start_id;			//0x24*0x200=0x4800
			fat_tbl_size = short_buf[22 / 2];						//this value must be set to 0 in FAT32(sector per fat)
			if(fat_tbl_size == 0)
			{
				//find FAT32
				fat_type = 32;
				FS_PRINT("FAT32\r\n");

				fat_tbl_size = word_buf[9];							//0x1d72*0x200=0x3AE400
				fs_info_start = short_buf[48 / 2] + BPB_Start_id;

#if FS_INFO_CACHE == 1
				if(drvl2_sd_read(device_id, fs_info_start, (INT32U *) &sd_fs_info_ram[0], 1) != 0)
				{
					fsinfo_loaded_flag = 0;
					FS_PRINT("SD read fail 2\r\n");
				}
				else
				{
					fsinfo_loaded_flag = 1;
				}

				fsinfo_dirty_flag = 0;

                #if FAT_WRITE_TBL_ID == 1	//FAT2->FAT1
				FS_info_tag = fi_word_buf[0];
				Free_Count = fi_word_buf[488 / 4] + BPB_Start_id;	//should not add BPB_Start_id, wwj comment@20140929
				Next_Free = fi_word_buf[492 / 4] + BPB_Start_id;
				#endif
#else
				if(drvl2_sd_read(device_id, fs_info_start, (INT32U *) &sd_cache_ram[0], 1) != 0)
				{
					FS_PRINT("SD read fail 2\r\n");
				}

                #if FAT_WRITE_TBL_ID == 1	//FAT2->FAT1
				FS_info_tag = word_buf[0];
				Free_Count = word_buf[488 / 4] + BPB_Start_id;		//should not add BPB_Start_id, wwj comment@20140929
				Next_Free = word_buf[492 / 4] + BPB_Start_id;
				#endif
#endif
				FS_PRINT("Fs info Tag: 0x%x (SectorId:%d,offset:%#8x)\r\n", FS_info_tag, fs_info_start,  fs_info_start * 512);// Sector=512, reference  "Api_fat_format.c"  bp[12] = 0x02;

#if FAT_WRITE_TBL_ID == 1	//FAT2->FAT1
				if(FS_info_tag == 0x41615252)
				{
					if((FSI_Free_Count == Free_Count) && (FSI_Next_Free == Next_Free))
					{
						mirror_en = 0;
					}
				}

#endif
			}
			else
			{
				fat_type = 16;
				FS_PRINT("FAT16\r\n");
			}

			fat_start_id[1] = fat_start_id[0] + fat_tbl_size;	// 0x3AE400+0x4800=0x3B2C00
		}

#if FAT_WRITE_TBL_ID == 1
		if(mirror_en)
		{
			fat_tbl_mirror_init(device_id);						// Mirror initial FAT1->FAT2
		}
		else
		{
			FS_PRINT("Tbl Mirror initial avoid\r\n");
		}

#endif
		mirror_start_id = 0x3FFFFFFF;
		mirror_end_id = 0;	// default mirror start
#if FS_CACHE_DBG_EN == 1
		FS_PRINT("\r\nfat_cluster_size:%d\r\n", fat_cluster_size);
		FS_PRINT("fat_start_id[0]:%d\r\n", fat_start_id[0]);
		FS_PRINT("fat_start_id[1]:%d\r\n", fat_start_id[1]);
		FS_PRINT("fat_tbl_size:%d\r\n", fat_tbl_size);
#endif
		cache_init_flag = 0xA8;
	}

	return 0;
}

#if FAT_WRITE_TBL_ID == 1
INT32S fsinfo_sync(INT32U device_id)	//  // dominant fat2->fat1
{
	INT32U	*word_buf = (INT32U *) &sd_fs_info_ram[0];
	INT32S	ret;

	if(fat_type == 32)
	{
#if FS_INFO_CACHE == 1
		FS_PRINT("FI_SYNC\r\n");
		ret = 0;
#else
		ret = drvl2_sd_read(device_id, fs_info_start, (INT32U *) &sd_fs_info_ram[0], 1);
#endif
		if(ret == 0)
		{
			FSI_Free_Count = word_buf[488 / 4] + BPB_Start_id;	//should not add BPB_Start_id, wwj comment@20140929
			FSI_Next_Free = word_buf[492 / 4] + BPB_Start_id;
		}
		else
		{
			FSI_Free_Count = 0xFFFFFFEE;
			FSI_Next_Free = 0;
			FS_PRINT("SD read fail 3\r\n");
		}

		return ret;
	}

	return -1;
}

#endif
#if FAT_WRITE_TBL_ID == 1
INT32S fat_tbl_mirror_init(INT32U device_id)	// dominant fat2->fat1
{
	INT32U	i;
	INT32U	sync_size;
	INT32U	move_cnt;
	INT32U	redundant_cnt;
	INT32U	mirror_start_offset;
	INT32U	fat0_sector_id;
	INT32U	fat1_sector_id;
	INT32U	fat_mirror_init_time = tiny_counter_get();

	sync_size = fat_tbl_size;
	fat0_sector_id = fat_start_id[0];
	fat1_sector_id = fat_start_id[1];

	move_cnt = sync_size / SDC_CACHE_SECTOR_NUMS;
	redundant_cnt = sync_size % SDC_CACHE_SECTOR_NUMS;
	mirror_start_offset = 0;
	for(i = 0; i < move_cnt; i++)
	{
		if(drvl2_sd_read(device_id, fat0_sector_id + SDC_CACHE_SECTOR_NUMS * i, (INT32U *) &sd_cache_ram[0], SDC_CACHE_SECTOR_NUMS) != 0)
			FS_PRINT("SD read fail 4\r\n");
		if(drvl2_sd_write(device_id, fat1_sector_id + SDC_CACHE_SECTOR_NUMS * i, (INT32U *) &sd_cache_ram[0], SDC_CACHE_SECTOR_NUMS) != 0)
			FS_PRINT("SD write fail 1\r\n");
	}

	if(redundant_cnt)
	{
		if(drvl2_sd_read(device_id, fat0_sector_id + SDC_CACHE_SECTOR_NUMS * i, (INT32U *) &sd_cache_ram[0], redundant_cnt) != 0)
			FS_PRINT("SD read fail 5\r\n");
		if(drvl2_sd_write(device_id, fat1_sector_id + SDC_CACHE_SECTOR_NUMS * i, (INT32U *) &sd_cache_ram[0], redundant_cnt) != 0)
			FS_PRINT("SD write fail 2\r\n");
	}

	mirror_start_id = 0x3FFFFFFF;
	mirror_end_id = 0;	// default mirror start
#if 1					//FS_CACHE_DBG_EN==1
	FS_PRINT("FAT1->FAT2 full size %d KB mirror time: %d\r\n", fat_tbl_size / 2, tiny_counter_get() - fat_mirror_init_time);
#endif
	return 0;
}

#endif
INT32S fat_cache_sync(INT32U device_id)
{
	INT8U	i;

#if FS_FLUSH_CAHCE_EN == 1
	if(fat_cache_L1_en)
	{
#if 0
		if(miss_thread > MISS_THREAD_MAX)
		{
			FS_PRINT("FAT L1 cache stop\r\n");
			fat_cache_L1_en = 0;
		}

		if(cache_ref_id != 0xFFFFFFFF)
		{
#if FS_CACHE_DBG_EN == 1
			FS_PRINT("Fat-L1 write back [%d:%d]\r\n", cache_ref_id, cache_ref_id + SDC_CACHE_SECTOR_NUMS - 1);
#endif
			if(drvl2_sd_write(device_id, cache_ref_id, (INT32U *) &sd_cache_ram[0], SDC_CACHE_SECTOR_NUMS) != 0)
			{
				FS_PRINT("SD write fail 6\r\n");
			}

			cache_ref_id = 0xFFFFFFFF;
			return 0;
		}

#else
		for(i = 0; i < CACHE_FAT_TABLE_CNT; i++)
		{
			if(cache_fat_table[i].dirty_flag)
			{
				cache_fat_table[i].dirty_flag = 0;
				if(drvl2_sd_write(device_id, cache_fat_table[i].cache_sector_idx, (INT32U *) cache_fat_table[i].cache_addrs, SDC_CACHE_SECTOR_NUMS) != 0)
				{
					DBG_PRINT("SD write fail 6\r\n");
				}
			}
		}

		return 0;
#endif
	}

	return -1;
#endif
}

#if FS_INFO_CACHE == 1
INT32S fs_info_flush(INT32U device_id)
{
	INT32S	ret = 0;

	if((fat_type == 32) && (fsinfo_loaded_flag == 1) && (fsinfo_dirty_flag == 1))
	{
		FS_PRINT("FI_FLUSH\r\n");
		ret = drvl2_sd_write(device_id, fs_info_start, (INT32U *) &sd_fs_info_ram[0], 1);
		if(ret != 0)
		{
			fsinfo_loaded_flag = 0;
			DBG_PRINT("SD write fail 7\r\n");
		}

		fsinfo_dirty_flag = 0;
	}

	return ret;
}

#endif

//read/write speed test function
#if (FAT_WRITE_TBL_ID == 1)
static INT32U fat_tbl_tansfer(INT32U blkno)
{
	if((blkno >= fat_start_id[0]) && (blkno < fat_start_id[0] + fat_tbl_size))
	{
		blkno += fat_tbl_size;
	}

	return blkno;
}

#endif

/****************************************************************************************************************************/
INT32S SDX_Initial(INT32U device_id)
{
	INT32S	ret;

	SD_FS_PRINT("[%s] id=%d\r\n", __func__, device_id);

	ret = drvl2_sd_init(device_id);

	if(ret == 0)
	{
		fs_sd_ms_plug_out_flag_reset();
		fat_l1_cache_init(device_id);
	}
	else
	{
		#if 0
		SD_FS_ERR_PRINT("[%s] drvl2_sd_init fail. id=%d ret=%d\r\n", __func__, device_id, ret);
		#endif
	}

	return ret;
}

INT32S SDX_Uninitial(INT32U device_id)
{
	SD_FS_PRINT("[%s] id=%d\r\n", __func__, device_id);

	drvl2_sd_card_remove(device_id);
	return 0;
}

void SDX_GetDrvInfo(INT32U device_id, struct DrvInfo *info)
{
	SD_FS_PRINT("[%s] id=%d\r\n", __func__, device_id);

	fat_l1_cache_init(device_id);
	info->nSectors = drvl2_sd_sector_number_get(device_id);
	info->nBytesPerSector = 512;
}

//read/write speed test function
INT32S SDX_ReadSector(INT32U device_id, INT32U blkno, INT32U blkcnt, INT32U buf)
{
	INT32S	ret;
	INT32S	i;
#if FS_FLUSH_CAHCE_EN == 1
	INT8U	retIdx;
	INT32U	cache_sector_idx;
#endif
	SD_FS_PRINT("[%s] id=%d blkno=%u blkcnt=%u buf=%u\r\n", __func__, device_id, blkno, blkcnt, buf);
//DBG_PRINT("[%s]\r\n", __func__);
	fat_l1_cache_init(device_id);

	if(fs_sd_ms_plug_out_flag_get() == 1)
	{
		return 0xFFFFFFFF;
	}

#if (FAT_WRITE_TBL_ID == 1)
	blkno = fat_tbl_tansfer(blkno);
#endif
#if FS_INFO_CACHE == 1
	if((fat_type == 32) && (blkcnt == 1) && (blkno == fs_info_start) && (fsinfo_loaded_flag))
	{
		//FS_PRINT ("FI_R\r\n");
		FsCpy4((void *) buf, (void *) &sd_fs_info_ram[0], 512);
		return 0;
	}

#endif
#if FS_FLUSH_CAHCE_EN == 1
	if(blkcnt == 1)
	{
		if(fat_cache_L1_en)
		{
			if((blkno >= fat_start_id[FAT_MAIN_TBL_ID]) && (blkno < fat_start_id[FAT_MAIN_TBL_ID] + fat_tbl_size))
			{
#if 0
				if(cache_ref_id == 0xFFFFFFFF)
				{
#if 0
					cache_ref_id = blkno;
					cache_read_from_nv();
#else
#if FS_CACHE_DBG_EN == 1
					FS_PRINT("Direct-R:%d\r\n", blkno);
#endif
					goto DIRECTLY_READ;
#endif
				}

				if((blkno >= cache_ref_id) && blkno < (cache_ref_id + SDC_CACHE_SECTOR_NUMS))
				{
#if FS_CACHE_DBG_EN == 1
					FS_PRINT("Hit-R:%d from [%d,%d]\r\n", blkno, cache_ref_id, (cache_ref_id + SDC_CACHE_SECTOR_NUMS - 1));
#endif
					FsCpy4((void *) buf, (void *) &sd_cache_ram[512 * (blkno - cache_ref_id)], 512);
					return 0;
				}

#else
				retIdx = find_cache_fat_table_for_read(blkno);
				if(retIdx & CACHE_FAT_TABLE_HIT)
				{
					retIdx &= ~(CACHE_FAT_TABLE_HIT);
					cache_sector_idx = ((blkno - cache_fat_table[retIdx].cache_sector_idx) << 9);
					FsCpy4((void *) buf, (void *) (cache_fat_table[retIdx].cache_addrs + cache_sector_idx), 512);
					return 0;
				}
				else
				if(retIdx & CACHE_FAT_TABLE_INIT)
				{
					retIdx &= ~(CACHE_FAT_TABLE_INIT);

					// Read data from SDC and store it into cache fat table
					cache_fat_table[retIdx].cache_sector_idx = ((blkno / SDC_CACHE_SECTOR_NUMS) * SDC_CACHE_SECTOR_NUMS);

					for(i = 0; i < SD_RW_RETRY_COUNT; i++)
					{
						ret = drvl2_sd_read(device_id, cache_fat_table[retIdx].cache_sector_idx, (INT32U *) cache_fat_table[retIdx].cache_addrs, SDC_CACHE_SECTOR_NUMS);
						if(ret == 0)
						{
							break;
						}
					}

					// Return data
					cache_sector_idx = ((blkno - cache_fat_table[retIdx].cache_sector_idx) << 9);
					FsCpy4((void *) buf, (void *) (cache_fat_table[retIdx].cache_addrs + cache_sector_idx), 512);
					cache_fat_table[retIdx].is_used_flag = 1;
#if _OPERATING_SYSTEM != _OS_NONE
#if _OPERATING_SYSTEM == _OS_UCOS2
					cache_fat_table[retIdx].cache_time = OSTimeGet();	// 10ms per tick
#elif _OPERATING_SYSTEM == _OS_FREERTOS
					cache_fat_table[retIdx].cache_time = xTaskGetTickCount();
#endif
#endif
					return 0;
				}
				else
				{
					retIdx &= ~(CACHE_FAT_TABLE_MISS);

					// Write the replaced data from the cache fat table to the SDC
					if(cache_fat_table[retIdx].dirty_flag)
					{
						for(i = 0; i < SD_RW_RETRY_COUNT; i++)
						{
							ret = drvl2_sd_write(device_id, cache_fat_table[retIdx].cache_sector_idx, (INT32U *) cache_fat_table[retIdx].cache_addrs, SDC_CACHE_SECTOR_NUMS);
							if(ret == 0)
							{
								break;
							}
						}
					}

					// Read data from SDC and store it into cache fat table
					cache_fat_table[retIdx].cache_sector_idx = ((blkno / SDC_CACHE_SECTOR_NUMS) * SDC_CACHE_SECTOR_NUMS);

					for(i = 0; i < SD_RW_RETRY_COUNT; i++)
					{
						ret = drvl2_sd_read(device_id, cache_fat_table[retIdx].cache_sector_idx, (INT32U *) cache_fat_table[retIdx].cache_addrs, SDC_CACHE_SECTOR_NUMS);
						if(ret == 0)
						{
							break;
						}
					}

					cache_sector_idx = ((blkno - cache_fat_table[retIdx].cache_sector_idx) << 9);
					FsCpy4((void *) buf, (void *) (cache_fat_table[retIdx].cache_addrs + cache_sector_idx), 512);
#if _OPERATING_SYSTEM != _OS_NONE
#if _OPERATING_SYSTEM == _OS_UCOS2
					cache_fat_table[retIdx].cache_time = OSTimeGet();	// 10ms per tick
#elif _OPERATING_SYSTEM == _OS_FREERTOS
					cache_fat_table[retIdx].cache_time = xTaskGetTickCount();
#endif
#endif
					cache_fat_table[retIdx].is_used_flag = 1;
					cache_fat_table[retIdx].dirty_flag = 0;
					return 0;
				}

#endif
			}
		}
	}

#endif
	for(i = 0; i < SD_RW_RETRY_COUNT; i++)
	{
		ret = drvl2_sd_read(device_id, blkno, (INT32U *) buf, blkcnt);
		if(ret == 0)
		{
			break;
		}
		else
		{
			SD_FS_ERR_PRINT("[%s] drvl2_sd_read fail. id=%d ret=%d try=%d\r\n", __func__, device_id, ret, i);
		}
	}

#if SUPPORT_STG_SUPER_PLUGOUT == 1
	if(ret != 0)
	{
		//if (drvl2_sd_read(device_id, 0, (INT32U *) buf, 1)!=0)
		{
			fs_sd_ms_plug_out_flag_en();
			SD_FS_ERR_PRINT("============>SUPER PLUG OUT DETECTED[R]<===========\r\n");
		}
	}

#endif
	return ret;
}

INT32S SDX_WriteSector(INT32U device_id, INT32U blkno, INT32U blkcnt, INT32U buf)
{
	INT32S	ret;
	INT32S	i;
#if FS_FLUSH_CAHCE_EN == 1
	INT8U	retIdx;
	INT32U	cache_sector_idx;
#endif
	//DBG_PRINT("[%s] id=%d blkno=%u blkcnt=%u buf=0x%x\r\n", __func__, device_id, blkno, blkcnt, buf);
	//DBG_PRINT("[%s]\r\n", __func__);
	fat_l1_cache_init(device_id);

	if(fs_sd_ms_plug_out_flag_get() == 1)
	{
		return 0xFFFFFFFF;
	}

#if (FAT_WRITE_TBL_ID == 1)
	blkno = fat_tbl_tansfer(blkno);
#endif
	if(blkcnt == 1)
	{
		if((blkno >= fat_start_id[FAT_MAIN_TBL_ID]) && (blkno < fat_start_id[FAT_MAIN_TBL_ID] + fat_tbl_size))
		{
			if(blkno < mirror_start_id)
			{
				mirror_start_id = blkno;
			}

			if(blkno > mirror_end_id)
			{
				mirror_end_id = blkno;
			}

#if FS_FLUSH_CAHCE_EN == 1
			if(fat_cache_L1_en)
			{
#if 0
				if(cache_ref_id == 0xFFFFFFFF)
				{
					cache_ref_id = blkno;
					cache_read_from_nv();
				}

				if((blkno >= cache_ref_id) && (blkno < (cache_ref_id + SDC_CACHE_SECTOR_NUMS)))
				{
#if FS_CACHE_DBG_EN == 1
					FS_PRINT("Hit-W:%d from [%d,%d]\r\n", blkno, cache_ref_id, (cache_ref_id + SDC_CACHE_SECTOR_NUMS - 1));
#endif
					cache_sync_flag = 0;
					FsCpy4((void *) &sd_cache_ram[512 * (blkno - cache_ref_id)], (const void *) buf, 512);
					hit_cnt++;
					return 0;
				}
				else
				{
#if FS_CACHE_DBG_EN == 1
					FS_PRINT("Miss-W:%d out-of [%d, %d]\r\n", blkno, cache_ref_id, (cache_ref_id + SDC_CACHE_SECTOR_NUMS - 1));
#endif
					miss_cnt++;
					total_cnt = hit_cnt + miss_cnt;
					if(total_cnt >= MISS_HIT_SAMPLE_CNTS)
					{
						if(cache_sync_flag == 1)
						{
							cache_switch_cnt++;
						}

						if(cache_switch_cnt > 2)
						{
							cache_switch_cnt = 0;
							miss_thread += 5;

							FS_PRINT("Miss thread +5\r\n");
						}

						miss_rate = (miss_cnt * 100 / total_cnt);
						if(miss_rate > miss_thread)
						{
							hit_cnt = 0;
							miss_cnt = 0;
							cache_sync(device_id);
							cache_sync_flag = 1;
						}
					}
				}

#else
				retIdx = find_cache_fat_table_for_write(blkno);
				if((retIdx & CACHE_FAT_TABLE_SYNC) || (retIdx & CACHE_FAT_TABLE_INIT))
				{
					// Write the replaced data from the cache fat table to the SDC
					if(retIdx & CACHE_FAT_TABLE_SYNC)
					{
						retIdx &= ~(CACHE_FAT_TABLE_SYNC);

						if(cache_fat_table[retIdx].dirty_flag)
						{
							cache_fat_table[retIdx].dirty_flag = 0;
							for(i = 0; i < SD_RW_RETRY_COUNT; i++)
							{
								ret = drvl2_sd_write(device_id, cache_fat_table[retIdx].cache_sector_idx, (INT32U *) cache_fat_table[retIdx].cache_addrs, SDC_CACHE_SECTOR_NUMS);
								if(ret == 0)
								{
									break;
								}
							}
						}
					}
					else
					{
						retIdx &= ~(CACHE_FAT_TABLE_INIT);
					}

					// Read data from SDC and store it into cache fat table
					cache_fat_table[retIdx].cache_sector_idx = ((blkno / SDC_CACHE_SECTOR_NUMS) * SDC_CACHE_SECTOR_NUMS);

					for(i = 0; i < SD_RW_RETRY_COUNT; i++)
					{
						ret = drvl2_sd_read(device_id, cache_fat_table[retIdx].cache_sector_idx, (INT32U *) cache_fat_table[retIdx].cache_addrs, SDC_CACHE_SECTOR_NUMS);
						if(ret == 0)
						{
							break;
						}
					}

					cache_sector_idx = ((blkno - cache_fat_table[retIdx].cache_sector_idx) << 9);
					FsCpy4((void *) (cache_fat_table[retIdx].cache_addrs + cache_sector_idx), (void *) buf, 512);

					//cache_fat_table[retIdx].cache_time = OSTimeGet();
#if _OPERATING_SYSTEM != _OS_NONE
#if _OPERATING_SYSTEM == _OS_UCOS2
					cache_fat_table[retIdx].cache_time = OSTimeGet();
#elif _OPERATING_SYSTEM == _OS_FREERTOS
					cache_fat_table[retIdx].cache_time = xTaskGetTickCount();
#endif
#endif
					//DBG_PRINT(" +CACHE_FAT_TABLE_SYNC\r\n");
					cache_fat_table[retIdx].is_used_flag = 1;
					cache_fat_table[retIdx].dirty_flag = 1;
					return 0;
				}
				else
				if(retIdx & CACHE_FAT_TABLE_HIT)
				{
					retIdx &= ~(CACHE_FAT_TABLE_HIT);
					//DBG_PRINT(" +CACHE_FAT_TABLE_HIT\r\n");
					cache_sector_idx = ((blkno - cache_fat_table[retIdx].cache_sector_idx) << 9);
					FsCpy4((void *) (cache_fat_table[retIdx].cache_addrs + cache_sector_idx), (void *) buf, 512);
					cache_fat_table[retIdx].dirty_flag = 1;
					return 0;
				}

#endif
			}

#endif
		}

#if FS_INFO_CACHE == 1
		else
		if((fat_type == 32) && (blkno == fs_info_start) && (fsinfo_loaded_flag))
		{
			//FS_PRINT ("FI_W\r\n");
			FsCpy4((void *) &sd_fs_info_ram[0], (const void *) buf, 512);
			fsinfo_dirty_flag = 1;
			return 0;
		}

#endif
	}

	for(i = 0; i < SD_RW_RETRY_COUNT; i++)
	{
		ret = drvl2_sd_write(device_id, blkno, (INT32U *) buf, blkcnt);
		if(ret == 0)
		{
			break;
		}
		else
		{
			SD_FS_ERR_PRINT("[%s] drvl2_sd_write fail. id=%d ret=%d try=%d\r\n", __func__, device_id, ret, i);
		}
	}

#if SUPPORT_STG_SUPER_PLUGOUT == 1
	if(ret != 0)
	{
		if(drvl2_sd_read(device_id, 0, (INT32U *) buf, 1) != 0)
		{
			fs_sd_ms_plug_out_flag_en();
			SD_FS_ERR_PRINT("============>SUPER PLUG OUT DETECTED[W]<===========\r\n");
		}
	}

#endif
	return ret;
}

INT32S SDX_Flush(INT32U device_id)
{
#if FS_INFO_CACHE == 1
	fs_info_flush(device_id);
#endif
	fat_cache_sync(device_id);
	drvl2_sd_flush(device_id);
	SD_FS_PRINT("[%s] id=%d\r\n", __func__, device_id);

	return 0;
}

INT32S SDX_tbl_mirror_sync(INT32U device_id)
{
#if FS_TBL_MIRROR_SYNC == 1
	INT32U	i;
	INT32U	sync_size;
	INT32U	move_cnt;
	INT32U	redundant_cnt;
	INT32U	mirror_start_offset;
	INT32S	ret = -1;
#else
	INT32S	ret = 0;
#endif

	//fat_cache_sync(device_id);
#if FS_INFO_CACHE == 1
	//fs_info_flush(device_id);
#endif
#if DIR_INFO_CACHE
	//dir_cache_flush(device_id);
#endif
#if FAT_WRITE_TBL_ID == 1
	fsinfo_sync(device_id);
#endif
#if FS_TBL_MIRROR_SYNC == 1
	sync_size = mirror_end_id - mirror_start_id + 1;
	if((mirror_start_id != 0x3FFFFFFF) && (sync_size <= fat_tbl_size))
	{
		move_cnt = sync_size / SDC_CACHE_SECTOR_NUMS;
		redundant_cnt = sync_size % SDC_CACHE_SECTOR_NUMS;
		mirror_start_offset = mirror_start_id - fat_start_id[FAT_MAIN_TBL_ID];
		for(i = 0; i < move_cnt; i++)
		{
			if(drvl2_sd_read(device_id, fat_start_id[FAT_MAIN_TBL_ID] + mirror_start_offset + SDC_CACHE_SECTOR_NUMS * i, (INT32U *) &sd_cache_ram[0], SDC_CACHE_SECTOR_NUMS))
				FS_PRINT("SD read fail 6\r\n");
			if(drvl2_sd_write(device_id, fat_start_id[FAT_SECOND_TBL_ID] + mirror_start_offset + SDC_CACHE_SECTOR_NUMS * i, (INT32U *) &sd_cache_ram[0], SDC_CACHE_SECTOR_NUMS))
				FS_PRINT("SD write fail 3\r\n");
		}

		if(redundant_cnt)
		{
			if(drvl2_sd_read(device_id, fat_start_id[FAT_MAIN_TBL_ID] + mirror_start_offset + SDC_CACHE_SECTOR_NUMS * i, (INT32U *) &sd_cache_ram[0], redundant_cnt))
				FS_PRINT("SD read fail 7\r\n");
			if(drvl2_sd_write(device_id, fat_start_id[FAT_SECOND_TBL_ID] + mirror_start_offset + SDC_CACHE_SECTOR_NUMS * i, (INT32U *) &sd_cache_ram[0], redundant_cnt))
				FS_PRINT("SD write fail 4\r\n");
		}

		mirror_start_id = 0x3FFFFFFF;
		mirror_end_id = 0;	// default mirror start
#if FS_CACHE_DBG_EN == 1
		FS_PRINT
		(
			"FS TBL SYNC %d Sectors [%d:%d]->[%d:%d]\r\n",
			sync_size,
			fat_start_id[FAT_MAIN_TBL_ID] + mirror_start_offset,
			fat_start_id[FAT_MAIN_TBL_ID] + mirror_start_offset + sync_size - 1,
			fat_start_id[FAT_SECOND_TBL_ID] + mirror_start_offset,
			fat_start_id[FAT_SECOND_TBL_ID] + mirror_start_offset + sync_size - 1
		);
#endif
		ret = 0;
	}

#if FS_CACHE_DBG_EN == 1
	if(ret == -1)
	{
		FS_PRINT("FS TBL SYNC NONE..\r\n");
	}

#endif
#endif
	return ret;
}

INT32S SDX_cache_sync_step(INT32U device_id)
{
	INT8U	i;
	INT32S	ret;

#if FS_FLUSH_CAHCE_EN == 1
	if(fat_cache_L1_en)
	{
		for(i = 0; i < CACHE_FAT_TABLE_CNT; i++)
		{
			if(cache_fat_table[i].dirty_flag)
			{
#if FS_CACHE_DBG_EN == 1
				FS_PRINT("s");
#endif
				cache_fat_table[i].dirty_flag = 0;
				if(drvl2_sd_write(device_id, cache_fat_table[i].cache_sector_idx, (INT32U *) cache_fat_table[i].cache_addrs, SDC_CACHE_SECTOR_NUMS) != 0)
				{
					FS_PRINT("SD write fail 6\r\n");
				}

				return 1;
			}
		}
	}

#endif
#if FS_INFO_CACHE == 1
	if((fat_type == 32) && (fsinfo_loaded_flag == 1) && (fsinfo_dirty_flag == 1))
	{
		//FS_PRINT ("FI_FLUSH\r\n");
		ret = drvl2_sd_write(device_id, fs_info_start, (INT32U *) &sd_fs_info_ram[0], 1);
		if(ret != 0)
		{
			fsinfo_loaded_flag = 0;
			FS_PRINT("SD write fail 7\r\n");
		}

		fsinfo_dirty_flag = 0;
		return 1;
	}

#endif
	return 0;
}

INT32S SDX_fill_fat_cache(INT32U device_id, INT32U start_clus, INT32U num_clus)
{
#if FS_FLUSH_CAHCE_EN == 1
	INT8U	retIdx, i;
	INT32U	blkno;
	INT32S	ret;
	INT32U	addr1, addr2;
	INT32U	*ptr, *ptr_end;

	if(fat_cache_L1_en && num_clus)
	{
		FS_PRINT(" SDX_fill_fat_cache\r\n");
		while(1)
		{
			blkno = fat_start_id[FAT_MAIN_TBL_ID] + (start_clus * 4) / 512;
			retIdx = find_cache_fat_table_for_write(blkno);
			if((retIdx & CACHE_FAT_TABLE_SYNC) || (retIdx & CACHE_FAT_TABLE_INIT))
			{
				// Write the replaced data from the cache fat table to the SDC
				if(retIdx & CACHE_FAT_TABLE_SYNC)
				{
					retIdx &= ~(CACHE_FAT_TABLE_SYNC);
					if(cache_fat_table[retIdx].dirty_flag)
					{
#if FS_CACHE_DBG_EN == 1
						DBG_PRINT("p");
#endif
						cache_fat_table[retIdx].dirty_flag = 0;
						for(i = 0; i < SD_RW_RETRY_COUNT; i++)
						{
							ret = drvl2_sd_write(device_id, cache_fat_table[retIdx].cache_sector_idx, (INT32U *) cache_fat_table[retIdx].cache_addrs, SDC_CACHE_SECTOR_NUMS);
							if(ret == 0)
							{
								break;
							}
						}
					}
				}
				else
				{
#if FS_CACHE_DBG_EN == 1
					DBG_PRINT("i");
#endif
					retIdx &= ~(CACHE_FAT_TABLE_INIT);
				}

				// Read data from SDC and store it into cache fat table
				cache_fat_table[retIdx].cache_sector_idx = ((blkno / SDC_CACHE_SECTOR_NUMS) * SDC_CACHE_SECTOR_NUMS);

				for(i = 0; i < SD_RW_RETRY_COUNT; i++)
				{
					ret = drvl2_sd_read(device_id, cache_fat_table[retIdx].cache_sector_idx, (INT32U *) cache_fat_table[retIdx].cache_addrs, SDC_CACHE_SECTOR_NUMS);
					if(ret == 0)
					{
						break;
					}
				}

#if _OPERATING_SYSTEM != _OS_NONE
#if _OPERATING_SYSTEM == _OS_UCOS2
				cache_fat_table[retIdx].cache_time = OSTimeGet();
#elif _OPERATING_SYSTEM == _OS_FREERTOS
				cache_fat_table[retIdx].cache_time = xTaskGetTickCount();
#endif
#else
				cache_fat_table[retIdx].cache_time = 0;
#endif
//DBG_PRINT(" [0.h]");
				cache_fat_table[retIdx].is_used_flag = 1;
				cache_fat_table[retIdx].dirty_flag = 1;
			}
			else
			if(retIdx & CACHE_FAT_TABLE_HIT)
			{
#if FS_CACHE_DBG_EN == 1
				//FS_PRINT ("h");
#endif
//DBG_PRINT(" [1.h]");
				retIdx &= ~(CACHE_FAT_TABLE_HIT);

				cache_fat_table[retIdx].dirty_flag = 1;
			}

			addr1 = fat_start_id[FAT_MAIN_TBL_ID] * 512 + (start_clus * 4) - cache_fat_table[retIdx].cache_sector_idx * 512 + cache_fat_table[retIdx].cache_addrs;
			ptr = (INT32U *) addr1;
			addr2 = cache_fat_table[retIdx].cache_addrs + SDC_CACHE_SIZE;
			ptr_end = (INT32U *) addr2;
			while(ptr < ptr_end)
			{
				*ptr++ = start_clus + 1;
				start_clus++;
				num_clus--;
				if(num_clus == 0)
				{
					ptr--;
					*ptr = LONG_LAST_CLUSTER;
					break;
				}
			}

			if(num_clus == 0)
				return STATUS_OK;
		}
	}

#endif
	return STATUS_FAIL;
}

//=== SD 0 setting ===//
INT32S SD0_Initial(void)
{
	return SDX_Initial(0);
}

INT32S SD0_Uninitial(void)
{
	return SDX_Uninitial(0);
}

void SD0_GetDrvInfo(struct DrvInfo *info)
{
	SDX_GetDrvInfo(0, info);
}

INT32S SD0_ReadSector(INT32U blkno, INT32U blkcnt, INT32U buf)
{
	return SDX_ReadSector(0, blkno, blkcnt, buf);
}

INT32S SD0_WriteSector(INT32U blkno, INT32U blkcnt, INT32U buf)
{
	return SDX_WriteSector(0, blkno, blkcnt, buf);
}

INT32S SD0_Flush(void)
{
	return SDX_Flush(0);
}

INT32S SD0_tbl_mirror_sync(void)
{
	return SDX_tbl_mirror_sync(0);
}

INT32S SD0_cache_sync_step(void)
{
	return SDX_cache_sync_step(0);
}

INT32S SD0_fill_fat_cache(INT32U start_clus, INT32U num_clus)
{
	return SDX_fill_fat_cache(0, start_clus, num_clus);
}

struct Drv_FileSystem	FS_SD0_driver =
{
	"SD0",
	DEVICE_READ_ALLOW | DEVICE_WRITE_ALLOW,
	SD0_Initial,
	SD0_Uninitial,
	SD0_GetDrvInfo,
	SD0_ReadSector,
	SD0_WriteSector,
	SD0_Flush,
	SD0_tbl_mirror_sync,
	SD0_cache_sync_step,
	SD0_fill_fat_cache,
};

//=== SD 1 setting ===//
INT32S SD1_Initial(void)
{
	return SDX_Initial(1);
}

INT32S SD1_Uninitial(void)
{
	return SDX_Uninitial(1);
}

void SD1_GetDrvInfo(struct DrvInfo *info)
{
	SDX_GetDrvInfo(1, info);
}

INT32S SD1_ReadSector(INT32U blkno, INT32U blkcnt, INT32U buf)
{
	return SDX_ReadSector(1, blkno, blkcnt, buf);
}

INT32S SD1_WriteSector(INT32U blkno, INT32U blkcnt, INT32U buf)
{
	return SDX_WriteSector(1, blkno, blkcnt, buf);
}

INT32S SD1_Flush(void)
{
	return SDX_Flush(1);
}

INT32S SD1_tbl_mirror_sync(void)
{
	return SDX_tbl_mirror_sync(1);
}

INT32S SD1_cache_sync_step(void)
{
	return SDX_cache_sync_step(1);
}

INT32S SD1_fill_fat_cache(INT32U start_clus, INT32U num_clus)
{
	return SDX_fill_fat_cache(1, start_clus, num_clus);
}

struct Drv_FileSystem	FS_SD1_driver =
{
	"SD1",
	DEVICE_READ_ALLOW | DEVICE_WRITE_ALLOW,
	SD1_Initial,
	SD1_Uninitial,
	SD1_GetDrvInfo,
	SD1_ReadSector,
	SD1_WriteSector,
	SD1_Flush,
	SD1_tbl_mirror_sync,
	SD1_cache_sync_step,
	SD1_fill_fat_cache,
};

static void FsCpy4(void *_dst, const void *_src, int len)
{
#if 1
   gp_memcpy((INT8S*)_dst, (INT8S*)_src, len);
#else
	register long *dst = (long*)_dst;
	const register long *src = (long*)_src;
	register long *end = (long*)((char*)_dst + len);
	len >>= 2;
	switch(len & 15)
	{
	case 15:
		__asm volatile
		(
			"LDMIA %1!,{r4-r10} \n"
			"STMIA %0!,{r4-r10} \n"
			"LDMIA %1!,{r4-r11} \n"
			"STMIA %0!,{r4-r11} \n"
			:"+r"(dst)
			:"r"(src)
		);
		break;
	case 14:
		__asm volatile
		(
			"LDMIA %1!,{r4-r9} \n"
			"STMIA %0!,{r4-r9} \n"
			"LDMIA %1!,{r4-r11} \n"
			"STMIA %0!,{r4-r11} \n"
			:"+r"(dst)
			:"r"(src)
		);
		break;
	case 13:
		__asm volatile
		(
			"LDMIA %1!,{r4-r8} \n"
			"STMIA %0!,{r4-r8} \n"
			"LDMIA %1!,{r4-r11} \n"
			"STMIA %0!,{r4-r11} \n"
			:"+r"(dst)
			:"r"(src)
		);
		break;
	case 12:
		__asm volatile
		(
			"LDMIA %1!,{r4-r7} \n"
			"STMIA %0!,{r4-r7} \n"
			"LDMIA %1!,{r4-r11} \n"
			"STMIA %0!,{r4-r11} \n"
			:"+r"(dst)
			:"r"(src)
		);
		break;
	case 11:
		__asm volatile
		(
			"LDMIA %1!,{r4-r6} \n"
			"STMIA %0!,{r4-r6} \n"
			"LDMIA %1!,{r4-r11} \n"
			"STMIA %0!,{r4-r11} \n"
			:"+r"(dst)
			:"r"(src)
		);
		break;
	case 10:
		__asm volatile
		(
			"LDMIA %1!,{r4-r5} \n"
			"STMIA %0!,{r4-r5} \n"
			"LDMIA %1!,{r4-r11} \n"
			"STMIA %0!,{r4-r11} \n"
			:"+r"(dst)
			:"r"(src)
		);
		break;
	case 9:
		__asm volatile
		(
			"LDMIA %1!,{r4-r12} \n"
			"STMIA %0!,{r4-r12} \n"
			:"+r"(dst)
			:"r"(src)
		);
		break;
	case 8:
		__asm volatile
		(
			"LDMIA %1!,{r4-r11} \n"
			"STMIA %0!,{r4-r11} \n"
			:"+r"(dst)
			:"r"(src)
		);
		break;
	case 7:
		__asm volatile
		(
			"LDMIA %1!,{r4-r10} \n"
			"STMIA %0!,{r4-r10} \n"
			:"+r"(dst)
			:"r"(src)
		);
		break;
	case 6:
		__asm volatile
		(
			"LDMIA %1!,{r4-r9} \n"
			"STMIA %0!,{r4-r9} \n"
			:"+r"(dst)
			:"r"(src)
		);
		break;
	case 5:
		__asm volatile
		(
			"LDMIA %1!,{r4-r8} \n"
			"STMIA %0!,{r4-r8} \n"
			:"+r"(dst)
			:"r"(src)
		);
		break;
	case 4:
		__asm volatile
		(
			"LDMIA %1!,{r4-r7} \n"
			"STMIA %0!,{r4-r7} \n"
			:"+r"(dst)
			:"r"(src)
		);
		break;
	case 3:
		__asm volatile
		(
			"LDMIA %1!,{r4-r6} \n"
			"STMIA %0!,{r4-r6} \n"
			:"+r"(dst)
			:"r"(src)
		);
		break;
	case 2:
		__asm volatile
		(
			"LDMIA %1!,{r4-r5} \n"
			"STMIA %0!,{r4-r5} \n"
			:"+r"(dst)
			:"r"(src)
		);
		break;
	case 1:
		*dst++ = *src++;
	}

	while(dst < end)
	{
		__asm volatile
		(
			"LDMIA %1!,{r4-r11} \n"
			"STMIA %0!,{r4-r11} \n"
			"LDMIA %1!,{r4-r11} \n"
			"STMIA %0!,{r4-r11} \n"
			:"+r"(dst)
			:"r"(src)
		);
	}
#endif
}

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#endif //((defined SD1_EN) && (SD1_EN == 1)) || ((defined SD0_EN) && (SD0_EN == 1))                          //

//================================================================//

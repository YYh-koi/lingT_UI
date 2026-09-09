/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  No.19, Industry E. Rd. IV, Hsinchu Science Park,                      *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
#include "application.h"

#define STORAGE_TIME_INTERVAL_MOUNT     128                     //128 = 1s
#define BKGROUND_DEL_INTERVAL           (1 * 128)               // 1 second
#define LOCK_FILE_PERCENT               70                       //1~100
#define MAX_THUMBNAIL_SIZE              (48*1024)

#define AP_STG_MAX_FILE_NUMS            625

#define FIXED_SIZE_FILE                 0
#define UNFIXED_SIZE_FILE               1
#define OPEN_FILE_MODE                  UNFIXED_SIZE_FILE

#if C_CREATE_FILE_DEBUG
extern void FatSpeedUpPrintf(INT16S dsk);	//for debug
#endif

static	INT8U	dual_mode = 0;
static	INT8U	cyclic_off = 0;
static	INT8U	rec_time_config, rec_resolution_config;

extern void FS_OS_LOCK(void);
extern void FS_OS_UNLOCK(void);
extern INT32S vid_dec_get_current_time(void);
/*=============================*/
INT64U  max_lock_file_size = 0;
INT64U  cur_lock_file_size = 0;
/*=============================*/

static	INT32S	g_jpeg_index;
static	INT32S	g_lock_index;
static	INT32S	g_h264_index;
static	INT32S	g_wav_index;
static	INT32S	g_file_index;
		INT32S	g_play_index;

static	INT16U	g_file_num;
static	INT16U	g_file_num_a;
static	INT16U	g_file_num_b;
		INT16U	g_trace_cnt;

static	INT16U	deleted_num_b_max;

static	INT16U	g_err_cnt;
static	INT32U	g_h264_file_time = 0;
static	INT32U	g_lock_file_time = 0;
static	INT32U	g_jpeg_file_time;
static	INT32U	g_wav_file_time;

static	INT8U	g_h264_index_9999_exist;
static	INT8U	g_lock_index_9999_exist;
static	INT8U	g_jpeg_index_9999_exist;
static	INT8U	g_wav_index_9999_exist;
static	INT8U	g_file_index_9999_exist;

static	INT16S	g_latest_h264_file_index;
static	INT16S	g_latest_lock_file_index;
static	INT16S	g_latest_jpeg_file_index;
static	INT16S	g_latest_wav_file_index;

static	INT32U	g_lock_file_oldest_time = 0;
static	INT16S	g_oldest_lock_file_index = 0;
static	INT32U	g_h264_file_oldest_time = 0;
static	INT16S	g_oldest_h264_file_index = 0;

static	INT16S	temp_oldest_lock_index = 10000;
static	INT16S	temp_oldest_h264_index = 10000;

		CHAR	g_file_path[256];
static	CHAR	g_file_path_b[256];
static	CHAR	g_next_file_path[256];
static	CHAR	g_next_file_path_b[256];

		CHAR	g_pic_path[256];
		CHAR	g_pic_path_b[256];

INT8U	curr_storage_id = 0;
static	INT8U	storage_mount_timerid;

#if C_AUTO_DEL_FILE == CUSTOM_ON
static	INT8U	storage_freesize_timerid;
#endif
		INT8U	device_plug_phase = 0;

INT16U	lock_file_table[625];
INT16U	h264_file_table[625];
static	INT16U	jpg_file_table[625];
INT16U	h264_file_table_b[625];
INT16U	lock_file_table_b[625];
static	INT16U	jpg_file_table_b[625];
static	INT16U	wav_file_table[625];

static	INT32U	h264_file_size;
static	INT32U	h264_file_size_back;

#if C_AUTO_DEL_FILE == CUSTOM_ON
static	INT8U	ap_step_work_start = 0;
static	INT8U	FAT_cache_fetched = 0;
static	INT32U	num_cache_fetch = 0;
static	INT32U	cache_fetch_cnt = 0;
static	INT32S	cluster_to_fetch = 0;
#endif

static	SemaphoreHandle_t	storage_fs_sem;
static	QueueHandle_t		storage_fs_ack;
static	INT16S	storage_fs_fd;
static	INT32U	storage_fs_buf;
static	INT32U	storage_fs_size;

//	prototypes
INT32S  get_file_final_h264_index(void);
INT32S	get_file_final_lock_index(void);
INT32S	get_file_final_jpeg_index(void);
INT32S	get_file_final_wav_index(void);
INT16U	get_deleted_file_number(INT8U flag);
INT16S  get_max_file_index(INT8U flag);

void    reinit_all_table(void);
void    get_file_index(void);

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/


/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
void ap_storage_service_init(void)
{
	if(storage_fs_sem == NULL)
	{
		storage_fs_sem = xSemaphoreCreateMutex();
	}

	if(storage_fs_ack == NULL)
	{
		storage_fs_ack = xQueueCreate(1, sizeof(INT32S));
	}

	#if C_AUTO_DEL_FILE == CUSTOM_ON
	storage_freesize_timerid = 0xFF;
	#endif

	// Speeding up first check
	storage_task_msg_send(MSG_STORAGE_SERVICE_STORAGE_CHECK, NULL, 0, MSG_PRI_NORMAL);

	storage_mount_timerid = STORAGE_SERVICE_MOUNT_TIMER_ID;
#if defined(C_STORAGE_DEVICE) && (C_STORAGE_DEVICE)
	drv_l2_sys_timer_set((void *) msgQSend, (void *) StorageServiceQ, MSG_STORAGE_SERVICE_STORAGE_CHECK, storage_mount_timerid, STORAGE_TIME_INTERVAL_MOUNT);
#endif
}

#if C_AUTO_DEL_FILE == CUSTOM_ON
void ap_storage_service_freesize_check_switch(INT8U type)
{
	const INT8U temp2[6] = { 5, 1, 2, 3, 5, 10 };			//unit: minute
	if(type == TRUE)
	{
		if(storage_freesize_timerid == 0xFF)
		{
			storage_freesize_timerid = STORAGE_SERVICE_FREESIZE_TIMER_ID;
			FAT_cache_fetched = 0;

			if(temp2[rec_time_config] == 1)
			{	//1min
				drv_l2_sys_timer_set((void *) msgQSend, (void *) StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, 10 * STORAGE_TIME_INTERVAL_MOUNT); //begin to delete old files after 10s
			}
			else
			{
				drv_l2_sys_timer_set
				(
					(void *) msgQSend,
					(void *) StorageServiceQ,
					MSG_STORAGE_SERVICE_FREESIZE_CHECK,
					storage_freesize_timerid,
					((temp2[rec_time_config] * STORAGE_TIME_INTERVAL_MOUNT * 60) - (90 * STORAGE_TIME_INTERVAL_MOUNT))
				);
			}
		}
	}
	else
	{
		if(storage_freesize_timerid != 0xFF)
		{
			drv_l2_sys_timer_kill(storage_freesize_timerid);
			storage_freesize_timerid = 0xFF;

			ap_step_work_start = 0;
			unlink_step_flush();
		}
	}
}


INT32S ap_storage_service_freesize_check_and_del(void)
{
	INT32U	file_size_to_create;
	INT32S	i, ret, del_index;
	INT32S	step_ret;
	INT16S	del_ret;
	CHAR	f_name[24], f_name_b[24];
	struct stat_t	buf_tmp, buf_tmp_b;

	if(storage_freesize_timerid == 0xff)
		return STATUS_OK;

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	#else
	chdir("C:\\DCIM");
	#endif

	if(ap_step_work_start == 0)
	{
		if(cyclic_off == 0)
		{
			if(dual_mode)
			{
				file_size_to_create = avi_file_size + avi_file_size_back;
			}
			else
			{
				file_size_to_create = avi_file_size;
			}

			file_size_to_create <<= 20;

			i = 0;
            #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
            if(vfsFreeSpace(STORAGE_DEVICE) < file_size_to_create)
            #else
            ret = UsrCheckFreeSize(STORAGE_DEVICE, file_size_to_create);
            if(ret < 2)
            #endif
			{	//delete old files for next recording
				if(cyclic_off)
					i = 10000;
				for(; i < 10000; i++)
				{
					del_index = g_oldest_avi_file_index;
					if((del_index & 0x000f) == 0)
					{
						if((avi_file_table[del_index >> 4] == 0) && (avi_file_table_b[del_index >> 4] == 0))
						{
							g_oldest_avi_file_index += 16;
							if(g_oldest_avi_file_index > 9999)
								g_oldest_avi_file_index = 0;
							i += 15;
							continue;
						}
					}

					if(avi_file_table[del_index >> 4] & (1 << (del_index & 0x000f)))
					{
						sprintf((char *) f_name, (const char *) "DCIMA\\MOVA%04d.mov", del_index);
						if((stat(f_name, &buf_tmp) == SUCCESS) && (buf_tmp.st_mode != D_RDONLY))
						{
							//DBG_PRINT("del = %s\r\n", f_name);
							unlink_step_start();
							del_ret = unlink((CHAR *) f_name);
							if(del_ret < 0)
							{
								del_ret = unlink((CHAR *) f_name);
							}

							avi_file_table[del_index >> 4] &= ~(1 << (del_index & 0xF));

							if(del_ret >= 0)
							{
								ap_step_work_start = 1;
								drv_l2_sys_timer_set((void *) msgQSend, (void *) StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, BKGROUND_DEL_INTERVAL);
								break;
							}
						}
					}

					if(avi_file_table_b[del_index >> 4] & (1 << (del_index & 0x000f)))
					{
						sprintf((char *) f_name_b, (const char *) "DCIMB\\MOVB%04d.mov", del_index);
						if((stat(f_name_b, &buf_tmp_b) == SUCCESS) && (buf_tmp_b.st_mode != D_RDONLY))
						{
							//DBG_PRINT("del = %s\r\n", f_name_b);
							unlink_step_start();
							del_ret = unlink((CHAR *) f_name_b);
							if(del_ret < 0)
							{
								del_ret = unlink((CHAR *) f_name_b);
							}

							avi_file_table_b[del_index >> 4] &= ~(1 << (del_index & 0xF));

							if(del_ret >= 0)
							{
								ap_step_work_start = 1;
								drv_l2_sys_timer_set((void *) msgQSend, (void *) StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, BKGROUND_DEL_INTERVAL);

								del_index += 1;
								if(del_index > 9999)
									del_index = 0;
								g_oldest_avi_file_index = del_index;
								break;
							}
						}
					}

					del_index += 1;
					if(del_index > 9999)
						del_index = 0;
					g_oldest_avi_file_index = del_index;
				}
			}
			else
			if(FAT_cache_fetched == 0)
			{	//prefetch FAT cache
				if(checkfattype(STORAGE_DEVICE) == FAT32_Type)
				{
					ap_step_work_start = 2;
					num_cache_fetch = ((file_size_to_create / GetBytesPerCluster(STORAGE_DEVICE)) * 4 + SDC_CACHE_SIZE - 1) / SDC_CACHE_SIZE;
					cache_fetch_cnt = 0;
					cluster_to_fetch = ret;
					drv_l2_sys_timer_set((void *) msgQSend, (void *) StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, BKGROUND_DEL_INTERVAL);
				}
				else
				{
					FAT_cache_fetched = 1;
				}
			}
			else
			{	//write back FAT cache
				ret = UsrCacheSyncStep(STORAGE_DEVICE);
				if(ret != 1)
				{
					//DBG_PRINT ("StepSync Done\r\n");
					drv_l2_sys_timer_kill(storage_freesize_timerid);
					storage_freesize_timerid = 0xFF;
				}
				else
				{
					//DBG_PRINT ("StepSync Continue\r\n");
					drv_l2_sys_timer_set((void *) msgQSend, (void *) StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, BKGROUND_DEL_INTERVAL);
				}
			}
		}
	}
	else
	if(ap_step_work_start == 1)
	{
		step_ret = unlink_step_work();
		if(step_ret != 0)
		{
			drv_l2_sys_timer_set((void *) msgQSend, (void *) StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, BKGROUND_DEL_INTERVAL);

			//DBG_PRINT ("StepDel Continue\r\n");
		}
		else
		{
			ap_step_work_start = 0;
			FAT_cache_fetched = 1;

			//DBG_PRINT ("StepDel Done\r\n");
		}
	}
	else
	{			//ap_step_work_start == 2
		UsrFatCacheFetchStep(STORAGE_DEVICE, cluster_to_fetch);
		cluster_to_fetch += SDC_CACHE_SIZE / 4;

		cache_fetch_cnt++;
		if(cache_fetch_cnt >= num_cache_fetch)
		{
			//DBG_PRINT ("StepPrefetch Done\r\n");
			ap_step_work_start = 0;
			FAT_cache_fetched = 1;
		}
		else
		{
			//DBG_PRINT ("StepPrefetch Continue\r\n");
			drv_l2_sys_timer_set((void *) msgQSend, (void *) StorageServiceQ, MSG_STORAGE_SERVICE_FREESIZE_CHECK, storage_freesize_timerid, BKGROUND_DEL_INTERVAL);
		}
	}

	return STATUS_OK;
}

#endif

void ap_storage_service_file_del(void)
{
	INT32S			ret;
	struct stat_t	buf_tmp;

	ap_storage_service_timer_stop();

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	#else
	chdir("C:\\DCIM");
	#endif

    if(g_play_index < 0)
	{
		ret = STATUS_FAIL;
        goto    Return;
	}

    stat(g_file_path, &buf_tmp);
	if(buf_tmp.st_mode & D_RDONLY)
	{
        ret = 0x55;
        goto    Return;
	}

	if(unlink(g_file_path) < 0)
	{
		DBG_PRINT("Delete file fail.\r\n");
		ret = STATUS_FAIL;
	}
	else
	{
		DBG_PRINT("Delete file OK.\r\n");

        if((g_file_path[4] == 'A') || (g_file_path[4] == 'R'))
        {
            h264_file_table[g_play_index >> 4] &= ~(1 << (g_play_index & 0xF));
            jpg_file_table[g_play_index >> 4] &= ~(1 << (g_play_index & 0xF));
            wav_file_table[g_play_index >> 4] &= ~(1 << (g_play_index & 0xF));

            if(g_file_num_a)    g_file_num_a--;
            if(g_file_num)      g_file_num--;
        }
        else
        {
            h264_file_table_b[g_play_index >> 4] &= ~(1 << (g_play_index & 0xF));
            jpg_file_table_b[g_play_index >> 4] &= ~(1 << (g_play_index & 0xF));

            if(g_file_num_b)    g_file_num_b--;
            if(g_file_num)      g_file_num--;
        }

		ret = STATUS_OK;
	}

	UsrFlushBuffers(STORAGE_DEVICE);
Return:
    ap_storage_service_timer_start();
    app_msg_send(MSG_APQ_SELECT_FILE_DEL_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);
    return;
}

void ap_storage_service_file_delete_all(void)
{
	struct f_info	file_info;
	INT32S			nRet;

	ap_storage_service_timer_stop();

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM\\DCIMA");
	#else
	chdir("C:\\DCIM\\DCIMA");
	#endif

	//MOV
	nRet = _findfirst("*.mov", &file_info, D_ALL);
	if(nRet >= 0)
	{
		while(1)
		{
			if(file_info.f_attrib & _A_RDONLY)
			{
				nRet = _findnext(&file_info);
				if(nRet < 0)
				{
					break;
				}

				continue;
			}

			unlink((CHAR *) file_info.f_name);

			nRet = _findnext(&file_info);
			if(nRet < 0)
			{
				break;
			}
		}
	}

	//JPEG
	nRet = _findfirst("*.jpg", &file_info, D_ALL);
	if(nRet >= 0)
	{
		while(1)
		{
			unlink((CHAR *) file_info.f_name);
			nRet = _findnext(&file_info);
			if(nRet < 0)
			{
				break;
			}
		}
	}

	//wav
	nRet = _findfirst("*.wav", &file_info, D_ALL);
	if(nRet >= 0)
	{
		while(1)
		{
			unlink((CHAR *) file_info.f_name);
			nRet = _findnext(&file_info);
			if(nRet < 0)
			{
				break;
			}
		}
	}

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM\\DCIMB");
	#else
	chdir("C:\\DCIM\\DCIMB");
	#endif

	//MOV
	nRet = _findfirst("*.mov", &file_info, D_ALL);
	if(nRet >= 0)
	{
		while(1)
		{
			if(file_info.f_attrib & _A_RDONLY)
			{
				nRet = _findnext(&file_info);
				if(nRet < 0)
				{
					break;
				}

				continue;
			}

			unlink((CHAR *) file_info.f_name);
			nRet = _findnext(&file_info);
			if(nRet < 0)
			{
				break;
			}
		}
	}

	//JPEG
	nRet = _findfirst("*.jpg", &file_info, D_ALL);
	if(nRet >= 0)
	{
		while(1)
		{
			unlink((CHAR *) file_info.f_name);
			nRet = _findnext(&file_info);
			if(nRet < 0)
			{
				break;
			}
		}
	}

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	#else
	chdir("C:\\DCIM");
	#endif

	//WAV
	nRet = _findfirst("*.wav", &file_info, D_ALL);
	if(nRet >= 0)
	{
		while(1)
		{
			unlink((CHAR *) file_info.f_name);
			nRet = _findnext(&file_info);
			if(nRet < 0)
			{
				break;
			}
		}
	}

	UsrFlushBuffers(STORAGE_DEVICE);

    reinit_all_table();

	ap_storage_service_timer_start();

	nRet = STATUS_OK;
	app_msg_send(MSG_APQ_FILE_DEL_ALL_REPLY, &nRet, sizeof(INT32S), MSG_PRI_NORMAL);
}

void ap_storage_service_file_lock_one(void)
{
	INT32S	ret;

	ap_storage_service_timer_stop();

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	#else
	chdir("C:\\DCIM");
	#endif

	//only lock video files
	if(	((h264_file_table[g_play_index >> 4] & (1 << (g_play_index & 0xF))) && (g_file_path[4] == 'A'))
	||	((h264_file_table_b[g_play_index >> 4] & (1 << (g_play_index & 0xF))) && (g_file_path[4] == 'B'))
	)
	{
#if C_RENAME_LOCK_FILE
		CHAR	temp_file_name[24];
#endif
		_setfattr(g_file_path, D_RDONLY);

#if C_RENAME_LOCK_FILE
		gp_memcpy((INT8S *) temp_file_name, (INT8S *) g_file_path, sizeof(temp_file_name));
		temp_file_name[6] = 'L';
		temp_file_name[7] = 'O';
		temp_file_name[8] = 'C';
		_rename((char *) g_file_path, temp_file_name);
		gp_memcpy((INT8S *) g_file_path, (INT8S *) temp_file_name, sizeof(temp_file_name));
#endif // C_RENAME_LOCK_FILE

        if(g_file_path[4] == 'A')
        {
            h264_file_table[g_play_index >> 4] &= ~(1 << (g_play_index & 0xF));
            lock_file_table[g_play_index >> 4] |= 1 << (g_play_index & 0xF);
        }
        else
        {
            h264_file_table_b[g_play_index >> 4] &= ~(1 << (g_play_index & 0xF));
            lock_file_table_b[g_play_index >> 4] |= 1 << (g_play_index & 0xF);
        }
	}

	UsrFlushBuffers(STORAGE_DEVICE);
	ap_storage_service_timer_start();

	ret = STATUS_OK;
	app_msg_send(MSG_APQ_FILE_LOCK_ONE_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);
}

void ap_storage_service_file_lock_all(void)
{
	INT32S      ret = STATUS_OK;
	INT32S      i;

	ap_storage_service_timer_stop();

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	#else
	chdir("C:\\DCIM");
	#endif

	for(i = 0; i < 10000; i++)
	{
        if((i & 0x000f) == 0)
        {
            if((h264_file_table[i >> 4] == 0) && (h264_file_table_b[i >> 4] == 0))
            {
                i += 15;
                continue;
            }
        }

        if(h264_file_table[i >> 4] & (1 << (i & 0xF)))
        {
            CHAR    f_name[24];
#if C_RENAME_LOCK_FILE
            CHAR	temp_file_name[24];
#endif

            sprintf((char *) f_name, (const char *) "DCIMA\\MOVA%04d.mov", i);
            _setfattr(f_name, D_RDONLY);

#if C_RENAME_LOCK_FILE
            gp_memcpy((INT8S *) temp_file_name, (INT8S *) f_name, sizeof(temp_file_name));
            temp_file_name[6] = 'L';
            temp_file_name[7] = 'O';
            temp_file_name[8] = 'C';
            _rename((char *) f_name, temp_file_name);
#endif // C_RENAME_LOCK_FILE

            h264_file_table[i >> 4] &= ~(1 << (i & 0xF));
            lock_file_table[i >> 4] |= 1 << (i & 0xF);
        }

        if(h264_file_table_b[i >> 4] & (1 << (i & 0xF)))
        {
            CHAR    f_name[24];
#if C_RENAME_LOCK_FILE
            CHAR	temp_file_name[24];
#endif

            sprintf((char *) f_name, (const char *) "DCIMB\\MOVB%04d.mov", i);
            _setfattr(f_name, D_RDONLY);

#if C_RENAME_LOCK_FILE
            gp_memcpy((INT8S *) temp_file_name, (INT8S *) f_name, sizeof(temp_file_name));
            temp_file_name[6] = 'L';
            temp_file_name[7] = 'O';
            temp_file_name[8] = 'C';
            _rename((char *) f_name, temp_file_name);
#endif // C_RENAME_LOCK_FILE

            h264_file_table_b[i >> 4] &= ~(1 << (i & 0xF));
            lock_file_table_b[i >> 4] |= 1 << (i & 0xF);
        }
    }

	UsrFlushBuffers(STORAGE_DEVICE);
	ap_storage_service_timer_start();
	app_msg_send(MSG_APQ_FILE_LOCK_ALL_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);
}

void ap_storage_service_file_unlock_one(void)
{
	INT32S	ret;

	ap_storage_service_timer_stop();

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	#else
	chdir("C:\\DCIM");
	#endif

    if(	((lock_file_table[g_play_index >> 4] & (1 << (g_play_index & 0xF))) && (g_file_path[4] == 'A'))
	||	((lock_file_table_b[g_play_index >> 4] & (1 << (g_play_index & 0xF))) && (g_file_path[4] == 'B'))
	)
	{
#if C_RENAME_LOCK_FILE
		CHAR	temp_file_name[24];
#endif
		_setfattr(g_file_path, D_NORMAL);

#if C_RENAME_LOCK_FILE
		gp_memcpy((INT8S *) temp_file_name, (INT8S *) g_file_path, sizeof(temp_file_name));
		temp_file_name[6] = 'M';
		temp_file_name[7] = 'O';
		temp_file_name[8] = 'V';
		_rename((char *) g_file_path, temp_file_name);
		gp_memcpy((INT8S *) g_file_path, (INT8S *) temp_file_name, sizeof(temp_file_name));
#endif // C_RENAME_LOCK_FILE

        if(g_file_path[4] == 'A')
        {
            lock_file_table[g_play_index >> 4] &= ~(1 << (g_play_index & 0xF));
            h264_file_table[g_play_index >> 4] |= 1 << (g_play_index & 0xF);
        }
        else
        {
            lock_file_table_b[g_play_index >> 4] &= ~(1 << (g_play_index & 0xF));
            h264_file_table_b[g_play_index >> 4] |= 1 << (g_play_index & 0xF);
        }
	}

	UsrFlushBuffers(STORAGE_DEVICE);
	ap_storage_service_timer_start();

	ret = STATUS_OK;
	app_msg_send(MSG_APQ_FILE_UNLOCK_ONE_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);
}

void ap_storage_service_file_unlock_all(void)
{
	INT32S      ret = STATUS_OK;
	INT32S      i;

	ap_storage_service_timer_stop();

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	#else
	chdir("C:\\DCIM");
	#endif

	for(i = 0; i < 10000; i++)
	{
        if((i & 0x000f) == 0)
        {
            if((lock_file_table[i >> 4] == 0) && (lock_file_table_b[i >> 4] == 0))
            {
                i += 15;
                continue;
            }
        }

        if(lock_file_table[i >> 4] & (1 << (i & 0xF)))
        {
            CHAR    f_name[24];
#if C_RENAME_LOCK_FILE
            CHAR	temp_file_name[24];
#endif

            sprintf((char *) f_name, (const char *) "DCIMA\\LOCA%04d.mov", i);
            _setfattr(f_name, D_NORMAL);

#if C_RENAME_LOCK_FILE
            gp_memcpy((INT8S *) temp_file_name, (INT8S *) f_name, sizeof(temp_file_name));
            temp_file_name[6] = 'M';
            temp_file_name[7] = 'O';
            temp_file_name[8] = 'V';
            _rename((char *) f_name, temp_file_name);
#endif // C_RENAME_LOCK_FILE

            lock_file_table[i >> 4] &= ~(1 << (i & 0xF));
            h264_file_table[i >> 4] |= 1 << (i & 0xF);
        }

        if(lock_file_table_b[i >> 4] & (1 << (i & 0xF)))
        {
            CHAR    f_name[24];
#if C_RENAME_LOCK_FILE
            CHAR	temp_file_name[24];
#endif

            sprintf((char *) f_name, (const char *) "DCIMB\\LOCB%04d.mov", i);
            _setfattr(f_name, D_NORMAL);

#if C_RENAME_LOCK_FILE
            gp_memcpy((INT8S *) temp_file_name, (INT8S *) f_name, sizeof(temp_file_name));
            temp_file_name[6] = 'M';
            temp_file_name[7] = 'O';
            temp_file_name[8] = 'V';
            _rename((char *) f_name, temp_file_name);
#endif // C_RENAME_LOCK_FILE

            lock_file_table_b[i >> 4] &= ~(1 << (i & 0xF));
            h264_file_table_b[i >> 4] |= 1 << (i & 0xF);
        }
    }

    UsrFlushBuffers(STORAGE_DEVICE);
	ap_storage_service_timer_start();

	app_msg_send(MSG_APQ_FILE_UNLOCK_ALL_REPLY, &ret, sizeof(INT32S), MSG_PRI_NORMAL);
}

void ap_storage_service_timer_start(void)
{
	if(storage_mount_timerid == 0xff)
	{
		storage_mount_timerid = STORAGE_SERVICE_MOUNT_TIMER_ID;
#if defined(C_STORAGE_DEVICE) && (C_STORAGE_DEVICE)
		drv_l2_sys_timer_set((void *) msgQSend, (void *) StorageServiceQ, MSG_STORAGE_SERVICE_STORAGE_CHECK, storage_mount_timerid, STORAGE_TIME_INTERVAL_MOUNT);
#endif
	}
}

void ap_storage_service_timer_stop(void)
{
	if(storage_mount_timerid != 0xff)
	{
		drv_l2_sys_timer_kill(storage_mount_timerid);
		storage_mount_timerid = 0xff;
	}
}

void ap_storage_service_usb_plug_in(void)
{
	device_plug_phase = 0;
}

INT16S SDC_ERR=0;
INT32S ap_storage_service_storage_mount(void)
{
	INT32S	nRet, i;
	INT32U	size;
    struct _diskfree_t st_free;

	if(storage_mount_timerid == 0xff)
		return STATUS_FAIL;

	if(device_plug_phase == 0)
	{
        //prohibit too many MSG_STORAGE_SERVICE_STORAGE_CHECK sent during sd mounting
        ap_storage_service_timer_stop();

		FS_OS_LOCK();
		nRet = drvl2_sd_init(STORAGE_SD_DEV);
		FS_OS_UNLOCK();

        if(!nRet){
			SDC_ERR = _devicemount(STORAGE_DEVICE);
		}
	}
	else
	{
		if(fs_sd_ms_plug_out_flag_get())
		{ //super plug out
			nRet = -1;
		}
		else
		{
			nRet = drvl2_sdc_live_response(STORAGE_SD_DEV);
			if(nRet != 0)
			{
				//prohibit too many MSG_STORAGE_SERVICE_STORAGE_CHECK sent during sd mounting
				ap_storage_service_timer_stop();
				//nRet = _devicemount(STORAGE_DEVICE);
				SDC_ERR = 0;
            }
        }
	}

	if(nRet < 0)
	{
        // plug out phase
        device_plug_phase = 0;

        FS_OS_LOCK();
        drvl2_sd_card_remove(STORAGE_SD_DEV);
        FS_OS_UNLOCK();

        if(curr_storage_id != NO_STORAGE)
        {
            #if C_SCREEN_SAVER
            // 若螢幕保護開時，要點亮背光
            if(screen_saver_enable)
            {
                screen_saver_enable = 0;
                ap_state_handling_lcd_backlight_switch(1);
            }
            ap_peripheral_clr_screen_saver_timer();  //自動關機要重數
            #endif

            g_file_index = 0;
            g_play_index = -1;
            g_file_num = g_file_num_a = g_file_num_b = 0;
            for(i = 0; i < 625; i++)
            {
                h264_file_table[i] = lock_file_table[i] = jpg_file_table[i] = wav_file_table[i] = 0;
                h264_file_table_b[i] = lock_file_table_b[i] = jpg_file_table_b[i] = 0;
            }

            curr_storage_id = NO_STORAGE;
            app_msg_send(MSG_STORAGE_SERVICE_NO_STORAGE, &curr_storage_id, sizeof(INT8U), MSG_PRI_NORMAL);
        }
	}
	else
	{
		// plug in phase
		device_plug_phase = 1;

		if(curr_storage_id != STORAGE_DEVICE)
		{
			#if C_SCREEN_SAVER
			// 若螢幕保護開時，要點亮背光
			if(screen_saver_enable)
			{
				screen_saver_enable = 0;
				ap_state_handling_lcd_backlight_switch(1);
			}

            ap_peripheral_clr_screen_saver_timer();    //自動關機要重數
			#endif // #if C_SCREEN_SAVER

			if(!SDC_ERR)
			{
                //for nonsupport NTFS SDC
                /*==============================================*/
                _getdiskfree(STORAGE_DEVICE, &st_free);
                DBG_PRINT("SDC info, st_free.total_clusters [%d]\r\n", st_free.total_clusters);
                DBG_PRINT("SDC info, st_free.avail_clusters [%d]\r\n", st_free.avail_clusters);
                DBG_PRINT("SDC info, st_free.sectors_per_cluster [%d]\r\n", st_free.sectors_per_cluster);
                DBG_PRINT("SDC info, st_free.bytes_per_sector [%d]\r\n", st_free.bytes_per_sector);
                max_lock_file_size = (INT64U)st_free.total_clusters*st_free.sectors_per_cluster*st_free.bytes_per_sector/(1024*1024);
				drvl2_sd_card_voloume_set(STORAGE_DEVICE, max_lock_file_size);
				DBG_PRINT("SDC total size [%lld]MB\r\n", max_lock_file_size);
                max_lock_file_size = max_lock_file_size*LOCK_FILE_PERCENT/100;
                DBG_PRINT("max_lock_file_size [%lld]MB\r\n", max_lock_file_size);
                /*==============================================*/

                size = vfsFreeSpace(STORAGE_DEVICE) >> 20;
                DBG_PRINT("Mount OK, free size [%d]\r\n", size);
                curr_storage_id = STORAGE_DEVICE;

                if(ap_state_firmware_upgrade_check())
                {
                    //ap_state_firmware_upgrade();
                    app_msg_send(MSG_STORAGE_FIRMWARE_UPGRADE, NULL, 0, MSG_PRI_NORMAL);
                }
                else
                {
                    //no need upgrade
                    #if STORAGE_DEVICE == FS_SD0
                    mkdir("G:\\DCIM");
                    mkdir("G:\\DCIM\\DCIMA");
                    mkdir("G:\\DCIM\\DCIMB");
                    chdir("G:\\DCIM");
                    #else
                    mkdir("C:\\DCIM");
                    mkdir("C:\\DCIM\\DCIMA");
                    mkdir("C:\\DCIM\\DCIMB");
                    chdir("C:\\DCIM");
                    #endif

                    UsrFlushBuffers(STORAGE_DEVICE);

                    reinit_all_table();
                }
            }
            else
            {
                curr_storage_id = STORAGE_DEVICE;
            }
			app_msg_send(MSG_STORAGE_SERVICE_MOUNT, &curr_storage_id, sizeof(INT8U), MSG_PRI_NORMAL);
		}
	}

	//enable again
	ap_storage_service_timer_start();
	return STATUS_OK;
}

void ap_storage_service_file_open_handle(INT32U req_type, INT16U usr_config)
{
	const INT32U MB_per_min_front[] = {	MB_PER_MIN_4K,
											MB_PER_MIN_2K,
                                            MB_PER_MIN_1080FHD,
											MB_PER_MIN_1080P,
                                            MB_PER_MIN_720P,
											MB_PER_MIN_WVGA,
											MB_PER_MIN_VGA,
											MB_PER_MIN_QVGA};  //4K, 2K, 1080FHD, 1080P, 720P, WVGA, VGA, QVGA

	const INT8U			temp2[6] = { 5, 1, 2, 3, 5, 10 };			//unit: minute
	INT32U				reply_type, MB_per_min, file_size_to_create;
	STOR_SERV_FILEINFO	file_info;
	CHAR				f_name[24], f_name_b[24];
	struct stat_t		buf_tmp, buf_tmp_b;
	INT32S				del_index, i, ret;
	INT8U				g_sensor_pwr_on_flag;
	INT8U				file_index_full_flag = 0;
	INT8U FlagToDel;
	INT8U FrontIsFound, RearIsFound;
	INT32S IndexForFront = 0, IndexForRear = 0;

	//DBG_PRINT("ap_storage_service_file_open_handle\r\n");
	ap_storage_service_timer_stop();

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	chkdirvol("C:\\DCIM\\DCIMA"); //exFAT extend DCIMA size
	#else
	chdir("C:\\DCIM");
	chkdirvol("C:\\DCIM\\DCIMA"); //exFAT extend DCIMA size
	#endif

	for(i=0; i<10000; i++)
	{
		if
		(
			(h264_file_table[g_file_index >> 4] & (1 << (g_file_index & 0xF)))
		||	(h264_file_table_b[g_file_index >> 4] & (1 << (g_file_index & 0xF)))
		||	(lock_file_table[g_file_index >> 4] & (1 << (g_file_index & 0xF)))
		||	(lock_file_table_b[g_file_index >> 4] & (1 << (g_file_index & 0xF)))
		||	(jpg_file_table[g_file_index >> 4] & (1 << (g_file_index & 0xF)))
		||	(jpg_file_table_b[g_file_index >> 4] & (1 << (g_file_index & 0xF)))
		||	(wav_file_table[g_file_index >> 4] & (1 << (g_file_index & 0xF)))
		)
		{
			g_file_index++;
			if(g_file_index > 9999)
			{
				g_file_index = 0;
			}
		}
		else
		{
			break;
		}
	}

	if(i==10000)
	{
		file_index_full_flag = 1;
	}

	switch(req_type)
	{
	case MSG_STORAGE_SERVICE_VID_H264_REQ:
		if(back_sensor_plug_status_get())
		{
			dual_mode = 1;
		}
		else
		{
			dual_mode = 0;
		}

		rec_time_config = usr_config >> 8;

		if(rec_time_config == 0xff)
		{
			g_sensor_pwr_on_flag = 1;
			rec_time_config = 0;
		}
		else
		{
			g_sensor_pwr_on_flag = 0;
		}

		if(rec_time_config > 5)
			rec_time_config = 5;

		if(rec_time_config == 0)
			cyclic_off = 1;
		else
			cyclic_off = 0;

		rec_resolution_config = usr_config & 0xff;
		if(rec_resolution_config > 7)
			rec_resolution_config = 7;

		if(g_sensor_pwr_on_flag)
		{
			// 20 seconds only
			h264_file_size = MB_per_min_front[rec_resolution_config] * 4 / 10;
			h264_file_size_back = MB_PER_MIN_BACK * 4 / 10;
		}
		else
		{
			MB_per_min = MB_per_min_front[rec_resolution_config];
			h264_file_size = temp2[rec_time_config] * MB_per_min;
			h264_file_size_back = temp2[rec_time_config] * MB_PER_MIN_BACK;
		}

		if(dual_mode)
		{
			file_size_to_create = h264_file_size + h264_file_size_back;
		}
		else
		{
			file_size_to_create = h264_file_size;
		}

		file_size_to_create <<= 20;

		FlagToDel = 0;
		FrontIsFound = 0;
		RearIsFound = 0;

		i = 0;
		if(file_index_full_flag)
		{
			i=10000;
		}

        /*===============================================*/
       DBG_PRINT("\r\ncur_lock_file_size= %lld\r\n", cur_lock_file_size);
       DBG_PRINT("max_lock_file_size= %lld\r\n", max_lock_file_size);
       if(cur_lock_file_size>=max_lock_file_size)
        {
            for(; i<10000; i++) {
                del_index = g_oldest_lock_file_index;
                if((del_index & 0x000f) == 0) {
                    if((lock_file_table[del_index >> 4] == 0)&&(lock_file_table_b[del_index >> 4] == 0))
                    {
                        g_oldest_lock_file_index += 16;
                        if(g_oldest_lock_file_index > 9999) g_oldest_lock_file_index = 0;
                            i += 15;
                        continue;
                    }
                }
                if (lock_file_table[del_index >> 4] & (1<<(del_index & 0x000f))) {
                        sprintf((char *) f_name, (const char *) "DCIMA\\LOCA%04d.mov", del_index);
                        stat((CHAR *) f_name, &buf_tmp);
                        _setfattr(f_name, D_NORMAL);
                        DBG_PRINT("del lock file = %s\r\n", f_name);
                        unlink((CHAR *)f_name);
                        lock_file_table[del_index >> 4] &= ~(1 << (del_index & 0xF));
                        cur_lock_file_size -= buf_tmp.st_size/(1024*1024);
                        DBG_PRINT("lock file size = %lld\r\n", cur_lock_file_size);
                        if(cur_lock_file_size<=max_lock_file_size)
                        {
                            break;
                        }
				}
                if (lock_file_table_b[del_index >> 4] & (1<<(del_index & 0x000f))) {
                        sprintf((char *) f_name, (const char *) "DCIMB\\LOCB%04d.mov", del_index);
                        stat((CHAR *) f_name, &buf_tmp);
                        _setfattr(f_name, D_NORMAL);
                        DBG_PRINT("del lock file = %s\r\n", f_name);
                        unlink((CHAR *)f_name);
                        lock_file_table_b[del_index >> 4] &= ~(1 << (del_index & 0xF));
                        cur_lock_file_size -= buf_tmp.st_size/(1024*1024);
                        DBG_PRINT("lock file size = %lld\r\n", cur_lock_file_size);
                        if(cur_lock_file_size<=max_lock_file_size)
                        {
                            break;
                        }
				}

                del_index += 1;
                if(del_index > 9999) del_index = 0;
                g_oldest_lock_file_index = del_index;
			}
        }
        /*===============================================*/
        #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
        if(vfsFreeSpace(STORAGE_DEVICE) < file_size_to_create)
        #else
        ret = UsrCheckFreeSize(STORAGE_DEVICE, file_size_to_create);
        if(ret < 2)
        #endif
		{
            i = 0;
            if(file_index_full_flag)
            {
                i=10000;
            }

			if(cyclic_off)
				i = 10000;
			for(; i < 10000; i++)
			{
				del_index = g_oldest_h264_file_index;
				if((del_index & 0x000f) == 0)
				{
					if((h264_file_table[del_index >> 4] == 0) && (h264_file_table_b[del_index >> 4] == 0))
					{
						g_oldest_h264_file_index += 16;
						if(g_oldest_h264_file_index > 9999)
							g_oldest_h264_file_index = 0;
						i += 15;
						continue;
					}
				}

				if(h264_file_table[del_index >> 4] & (1 << (del_index & 0x000f)))
				{
					sprintf((char *) f_name, (const char *) "DCIMA\\MOVA%04d.mov", del_index);
					if((stat(f_name, &buf_tmp) == SUCCESS) && (buf_tmp.st_mode != D_RDONLY))
					{
						if(FlagToDel == 0)
						{
							#if OPEN_FILE_MODE == FIXED_SIZE_FILE
							if(buf_tmp.st_size == (h264_file_size << 20))
							{
								if(FrontIsFound)
								{
									FlagToDel = 1;
									FrontIsFound = 0;

									sprintf((char *)f_name, (const char *)"DCIMA\\MOVA%04d.mov", IndexForFront);
									DBG_PRINT("del = %s\r\n", f_name);
									unlink((CHAR *)f_name);
									h264_file_table[IndexForFront >> 4] &= ~(1 << (IndexForFront & 0xF));

                                    #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
                                    if(vfsFreeSpace(STORAGE_DEVICE) > file_size_to_create)
                                    #else
                                    ret = UsrCheckFreeSize(STORAGE_DEVICE, file_size_to_create);
                                    if(ret > 2)
                                    #endif
									break;
								}
								else
								{
									FrontIsFound = 1;
									IndexForFront = del_index;

									if(dual_mode)
									{
										if(RearIsFound)
										{
											break;
										}
									}
									else
									{
										break;
									}
								}
							}
							else
							#endif
							{
								FlagToDel = 1;

								if(FrontIsFound)
								{
									FrontIsFound = 0;

									sprintf((char *)f_name, (const char *)"DCIMA\\MOVA%04d.mov", IndexForFront);
									DBG_PRINT("del = %s\r\n", f_name);
									unlink((CHAR *)f_name);
									h264_file_table[IndexForFront >> 4] &= ~(1 << (IndexForFront & 0xF));

                                    #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
                                    if(vfsFreeSpace(STORAGE_DEVICE) > file_size_to_create)
                                    #else
                                    ret = UsrCheckFreeSize(STORAGE_DEVICE, file_size_to_create);
                                    if(ret > 2)
                                    #endif
									break;
								}
								else if(RearIsFound)
								{
									RearIsFound = 0;

									sprintf((char *)f_name_b, (const char *)"DCIMB\\MOVB%04d.mov", IndexForRear);
									DBG_PRINT("del = %s\r\n", f_name_b);
									unlink((CHAR *)f_name_b);
									h264_file_table_b[IndexForRear >> 4] &= ~(1 << (IndexForRear & 0xF));

                                    #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
                                    if(vfsFreeSpace(STORAGE_DEVICE) > file_size_to_create)
                                    #else
                                    ret = UsrCheckFreeSize(STORAGE_DEVICE, file_size_to_create);
                                    if(ret > 2)
                                    #endif
									break;
								}
							}
						}

						if(FlagToDel)
						{
							sprintf((char *)f_name, (const char *)"DCIMA\\MOVA%04d.mov", del_index);
							DBG_PRINT("del = %s\r\n", f_name);
							unlink((CHAR *)f_name);
							h264_file_table[del_index >> 4] &= ~(1 << (del_index & 0xF));

                            #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
                            if(vfsFreeSpace(STORAGE_DEVICE) > file_size_to_create)
                            #else
                            ret = UsrCheckFreeSize(STORAGE_DEVICE, file_size_to_create);
                            if(ret > 2)
                            #endif
							break;
						}
					}
				}

				if(h264_file_table_b[del_index >> 4] & (1 << (del_index & 0x000f)))
				{
					sprintf((char *) f_name_b, (const char *) "DCIMB\\MOVB%04d.mov", del_index);
					if((stat(f_name_b, &buf_tmp_b) == SUCCESS) && (buf_tmp_b.st_mode != D_RDONLY))
					{
						if(FlagToDel == 0)
						{
							if(dual_mode)
							{
								#if OPEN_FILE_MODE == FIXED_SIZE_FILE
								if(buf_tmp_b.st_size == (h264_file_size_back << 20))
								{
									if(RearIsFound)
									{
										FlagToDel = 1;
										RearIsFound = 0;

										sprintf((char *)f_name_b, (const char *)"DCIMB\\MOVB%04d.mov", IndexForRear);
										DBG_PRINT("del = %s\r\n", f_name_b);
										unlink((CHAR *)f_name_b);
										h264_file_table_b[IndexForRear >> 4] &= ~(1 << (IndexForRear & 0xF));

                                        #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
                                        if(vfsFreeSpace(STORAGE_DEVICE) > file_size_to_create)
                                        #else
                                        ret = UsrCheckFreeSize(STORAGE_DEVICE, file_size_to_create);
                                        if(ret > 2)
                                        #endif
										break;
									}
									else
									{
										RearIsFound = 1;
										IndexForRear = del_index;

										if(FrontIsFound)
										{
											del_index += 1;
											if(del_index > 9999) del_index = 0;
											g_oldest_h264_file_index = del_index;
											break;
										}
									}
								}
								else
								#endif
								{
									FlagToDel = 1;

									if(FrontIsFound)
									{
										FrontIsFound = 0;

										sprintf((char *)f_name, (const char *)"DCIMA\\MOVA%04d.mov", IndexForFront);
										DBG_PRINT("del = %s\r\n", f_name);
										unlink((CHAR *)f_name);
										h264_file_table[IndexForFront >> 4] &= ~(1 << (IndexForFront & 0xF));

                                        #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
                                        if(vfsFreeSpace(STORAGE_DEVICE) > file_size_to_create)
                                        #else
                                        ret = UsrCheckFreeSize(STORAGE_DEVICE, file_size_to_create);
                                        if(ret > 2)
                                        #endif
										break;
									}
									else if(RearIsFound)
									{
										RearIsFound = 0;

										sprintf((char *)f_name_b, (const char *)"DCIMB\\MOVB%04d.mov", IndexForRear);
										DBG_PRINT("del = %s\r\n", f_name_b);
										unlink((CHAR *)f_name_b);
										h264_file_table_b[IndexForRear >> 4] &= ~(1 << (IndexForRear & 0xF));

                                        #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
                                        if(vfsFreeSpace(STORAGE_DEVICE) > file_size_to_create)
                                        #else
                                        ret = UsrCheckFreeSize(STORAGE_DEVICE, file_size_to_create);
                                        if(ret > 2)
                                        #endif
										break;
									}
								}
							}
							else
							{
								FlagToDel = 1;
							}
						}

						if(FlagToDel)
						{
							sprintf((char *)f_name_b, (const char *)"DCIMB\\MOVB%04d.mov", del_index);
							DBG_PRINT("del = %s\r\n", f_name_b);
							unlink((CHAR *)f_name_b);
							h264_file_table_b[del_index >> 4] &= ~(1 << (del_index & 0xF));

                            #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
                            if(vfsFreeSpace(STORAGE_DEVICE) > file_size_to_create)
                            #else
                            ret = UsrCheckFreeSize(STORAGE_DEVICE, file_size_to_create);
                            if(ret > 2)
                            #endif
                            {
								del_index += 1;
								if(del_index > 9999) del_index = 0;
								g_oldest_h264_file_index = del_index;
								break;
							}
						}
    				}
				}

				del_index += 1;
				if(del_index > 9999)
					del_index = 0;
				g_oldest_h264_file_index = del_index;
			}
		}

#if C_CREATE_FILE_DEBUG
		FatSpeedUpPrintf(STORAGE_DEVICE);
#endif
		reply_type = MSG_STORAGE_SERVICE_VID_REPLY;
		file_info.file_handle = -1;
		file_info.file_handle_b = -1;

#if 0	/* Check previous file size and FAT link size is matched or not, if not fix it */
		{
			INT16S check_file_handle = 0;
			INT32S check_file_index = 0;
			CHAR check_file_path[24] = {0};
			struct stat_t check_buf_tmp;

			if(g_file_index > 0)
			{
				check_file_index = g_file_index -1;
			}
			else
			{
				check_file_index = 9999;
			}

			sprintf((char *)check_file_path, (const char *)"DCIMA\\MOVA%04d.mov", check_file_index);
			DBG_PRINT("=>file index %d check file index %d file path %s\r\n", g_file_index, check_file_index, check_file_path);

			/* Delete file with file size = 0 */
			stat(check_file_path, &check_buf_tmp);
			if(check_buf_tmp.st_size == 0)
			{
				if(unlink(check_file_path) < 0)
				{
					DBG_PRINT("Delete file fail.\r\n");
				}
				else
				{
					DBG_PRINT("Delete check file OK deu to file size = 0.\r\n");
				}
			}
			else
			{
				check_file_handle = open(check_file_path, O_RDWR);
				if(check_file_handle >= 0)
				{
					//fs_Repairfileindir(check_file_handle);
					RepairFileInDir(check_file_handle);             //use locked version
					//fs_UpdateDir(check_file_handle);
					UpdateDir(check_file_handle);                   //use locked version

					//if(stat((char*)&check_file_path[0], &check_buf_tmp) != SUCCESS)
					if(fstat(check_file_handle, &check_buf_tmp) != SUCCESS)     //fstat() runs faster than stat()
					{
						DBG_PRINT("Stat check file %s failed \r\n", check_file_path);
					}
					else
					{
						DBG_PRINT("Check file size 0x%x\r\n", (INT32S)check_buf_tmp.st_size);
						DBG_PRINT("Check file mode 0x%x\r\n", check_buf_tmp.st_mode);
					}

					close(check_file_handle);
				}
				else
				{
					DBG_PRINT("open check file failed\r\n");
				}
				//while(1); //Stop here for test
			}
		}
#endif

		if(i < 10000)
		{
			sprintf((char *) g_file_path, (const char *) "DCIMA\\MOVA%04d.mov", g_file_index);
			sprintf((char *) g_file_path_b, (const char *) "DCIMB\\MOVB%04d.mov", g_file_index);
			file_info.file_path_addr = (INT32U)g_file_path;
			file_info.file_path_addr_b = (INT32U)g_file_path_b;
			#if OPEN_FILE_MODE == FIXED_SIZE_FILE
			if((FlagToDel == 0) && FrontIsFound)
			{
                _rename_fornewfile(f_name, g_file_path);
                DBG_PRINT("replace = %s\r\n", f_name);
				file_info.file_handle = open(g_file_path, O_RDWR);
				if(file_info.file_handle >= 0)
				{
					DBG_PRINT("\r\nFileName = %s\r\n", file_info.file_path_addr);
					h264_file_table[IndexForFront >> 4] &= ~(1 << (IndexForFront & 0xF));
					h264_file_table[g_file_index >> 4] |= 1 << (g_file_index & 0xF);
					if(RearIsFound)
					{
						_rename_fornewfile(f_name_b, g_file_path_b);
						DBG_PRINT("replace = %s\r\n", f_name_b);
						file_info.file_handle_b = open(g_file_path_b, O_RDWR);
						DBG_PRINT("BackFileName = %s\r\n", file_info.file_path_addr_b);
						h264_file_table_b[IndexForRear >> 4] &= ~(1 << (IndexForRear & 0xF));
						h264_file_table_b[g_file_index >> 4] |= 1 << (g_file_index & 0xF);
					}
				}
			}
			else
			#endif
			{
				#if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
				file_info.file_handle = open(g_file_path, O_WRONLY | O_CREAT | O_TRUNC);
				#else
				file_info.file_handle = CreatFileBySize(g_file_path, (h264_file_size << 20));
				#endif
				if(file_info.file_handle >= 0) {
					DBG_PRINT("\r\nFileName = %s\r\n", file_info.file_path_addr);
					h264_file_table[g_file_index >> 4] |= 1 << (g_file_index & 0xF);
					if(dual_mode) {
                        #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
                        file_info.file_handle_b = open(g_file_path_b, O_WRONLY | O_CREAT | O_TRUNC);
                        #else
						file_info.file_handle_b = CreatFileBySize(g_file_path_b, (h264_file_size_back << 20));
						#endif
						DBG_PRINT("BackFileName = %s\r\n", file_info.file_path_addr_b);
						h264_file_table_b[g_file_index >> 4] |= 1 << (g_file_index & 0xF);
					}
				}
			}

			g_file_index++;
			if(g_file_index == 10000){
				g_file_index = 0;
				g_file_index_9999_exist = 1;
			}

			#if C_CREATE_FILE_DEBUG
			FatSpeedUpPrintf(STORAGE_DEVICE);
			DBG_PRINT("\r\n");
			#endif
		}

        if(file_info.file_handle != -1)
		{
            UpdateDir(file_info.file_handle);
		}

		if(file_info.file_handle_b != -1)
		{
            UpdateDir(file_info.file_handle_b);
		}

		UsrFlushBuffers(STORAGE_DEVICE);
		break;

	case MSG_STORAGE_SERVICE_PIC_REQ:
		reply_type = MSG_STORAGE_SERVICE_PIC_REPLY;

		if((curr_storage_id == STORAGE_DEVICE) && !file_index_full_flag)
		{
#if C_SENSOR_SAVE_RAW_DATA
			sprintf((char *) g_file_path, (const char *) "PICT%04d.raw",  g_file_index);
#else
			//DBG_PRINT("\r\n==>g_file_index=%d\r\n",g_file_index);

			sprintf((char *) g_file_path, (const char *) "DCIMA\\PICA%04d.jpg", g_file_index);
			sprintf((char *) g_file_path_b, (const char *) "DCIMB\\PICB%04d.jpg", g_file_index);
#endif // C_SENSOR_SAVE_RAW_DATA
			file_info.file_path_addr = (INT32U) g_file_path;
			file_info.file_handle = open(g_file_path, O_WRONLY | O_CREAT | O_TRUNC);
			if(file_info.file_handle >= 0)
			{
				DBG_PRINT("\r\nFileName = %s\r\n", file_info.file_path_addr);
				jpg_file_table[g_file_index >> 4] |= 1 << (g_file_index & 0xF);

				#if !C_SENSOR_SAVE_RAW_DATA
				if(back_sensor_plug_status_get())
				{
					file_info.file_path_addr_b = (INT32U) g_file_path_b;
					file_info.file_handle_b = open(g_file_path_b, O_WRONLY | O_CREAT | O_TRUNC);
					DBG_PRINT("Back FileName = %s\r\n", file_info.file_path_addr_b);
					jpg_file_table_b[g_file_index >> 4] |= 1 << (g_file_index & 0xF);
				}
				else
				#endif // !C_SENSOR_SAVE_RAW_DATA
				{
					file_info.file_handle_b = -1;
				}

				g_file_index++;
				if(g_file_index == 10000)
				{
					g_file_index = 0;
					g_file_index_9999_exist = 1;
				}
			}
			else
			{
				file_info.file_handle_b = -1;
			}
		}
		else if(file_index_full_flag)
		{
			file_info.file_handle = -1;
			file_info.file_handle_b = -1;
		}
		else
		{
#if (defined RAMDISK_EN) && (RAMDISK_EN == 1)
			g_file_index = 0;

#if C_SENSOR_SAVE_RAW_DATA
			sprintf((char *) g_file_path, (const char *) "PICT%04d.raw", g_file_index);
#else
			sprintf((char *) g_file_path, (const char *) "PICT%04d.jpg", g_file_index);
#endif // C_SENSOR_SAVE_RAW_DATA
			unlink(g_file_path);	//because O_TRUNC is disabled
			file_info.file_path_addr = (INT32U) g_file_path;
			file_info.file_handle = open(g_file_path, O_WRONLY | O_CREAT | O_TRUNC);
			if(file_info.file_handle >= 0)
			{
				jpg_file_table[g_file_index >> 4] |= 1 << (g_file_index & 0xF);
			}

#else
			file_info.file_handle = -1;
			file_info.file_handle_b = -1;
#endif // (defined RAMDISK_EN) && (RAMDISK_EN == 1)
		}

		break;

	case MSG_STORAGE_SERVICE_PIC_REQ2:
		reply_type = MSG_STORAGE_SERVICE_PIC_REPLY;

		if((curr_storage_id == STORAGE_DEVICE) && !file_index_full_flag)
		{
			sprintf((char *) g_pic_path, (const char *) "DCIMA\\PICA%04d.jpg", g_file_index);
            sprintf((char *) g_pic_path_b, (const char *) "DCIMB\\PICB%04d.jpg", g_file_index);
			file_info.file_path_addr = (INT32U) g_pic_path;
			file_info.file_handle = open(g_pic_path, O_WRONLY | O_CREAT | O_TRUNC);
			if(file_info.file_handle >= 0)
			{
				DBG_PRINT("\r\nFileName = %s\r\n", file_info.file_path_addr);
				jpg_file_table[g_file_index >> 4] |= 1 << (g_file_index & 0xF);
				{
					file_info.file_handle_b = -1;
				}
				#if !C_SENSOR_SAVE_RAW_DATA
				if(back_sensor_plug_status_get())
				{
					file_info.file_path_addr_b = (INT32U) g_pic_path_b;
					file_info.file_handle_b = open(g_pic_path_b, O_WRONLY | O_CREAT | O_TRUNC);
					DBG_PRINT("Back FileName = %s\r\n", file_info.file_path_addr_b);
					jpg_file_table_b[g_file_index >> 4] |= 1 << (g_file_index & 0xF);
				}
				else
				#endif // !C_SENSOR_SAVE_RAW_DATA
				{
					file_info.file_handle_b = -1;
				}
				g_file_index++;
				if(g_file_index == 10000)
				{
					g_file_index = 0;
					g_file_index_9999_exist = 1;
				}
			}
			else
			{
				file_info.file_handle_b = -1;
			}
		}
		else if(file_index_full_flag)
		{
			file_info.file_handle = -1;
			file_info.file_handle_b = -1;
		}
		else
		{
#if (defined RAMDISK_EN) && (RAMDISK_EN == 1)
			g_file_index = 0;
			sprintf((char *) g_pic_path, (const char *) "PICT%04d.jpg", g_file_index);
			unlink(g_pic_path);	//because O_TRUNC is disabled
			file_info.file_path_addr = (INT32U) g_pic_path;
			file_info.file_handle = open(g_pic_path, O_WRONLY | O_CREAT | O_TRUNC);
			if(file_info.file_handle >= 0)
			{
				jpg_file_table[g_file_index >> 4] |= 1 << (g_file_index & 0xF);
			}

#else
			file_info.file_handle = -1;
			file_info.file_handle_b = -1;
#endif // (defined RAMDISK_EN) && (RAMDISK_EN == 1)
		}

		break;

	case MSG_STORAGE_SERVICE_AUD_REQ:
		reply_type = MSG_STORAGE_SERVICE_AUD_REPLY;

		if((curr_storage_id == STORAGE_DEVICE) && !file_index_full_flag)
		{
			sprintf((char *) g_file_path, (const char *) "DCIMA\\RECR%04d.wav", g_file_index); //20110927
            file_info.file_path_addr = (INT32U) g_file_path;
            DBG_PRINT("wav file path:%s\r\n",g_file_path);
			file_info.file_handle = open(g_file_path, O_WRONLY | O_CREAT | O_TRUNC);
			if(file_info.file_handle >= 0)
			{
                DBG_PRINT("\r\nFileName = %s\r\n", file_info.file_path_addr);
				wav_file_table[g_file_index >> 4] |= 1 << (g_file_index & 0xF);


				g_file_index++;
				if(g_file_index == 10000)
				{
					g_file_index = 0;
					g_file_index_9999_exist = 1;
				}
			}
		}
		else
		{
			file_info.file_handle = -1;
		}

		break;

	default:
		DBG_PRINT("UNKNOW STORAGE SERVICE\r\n");
		reply_type = MSG_STORAGE_SERVICE_VID_REPLY;
		break;
	}

	app_msg_send(reply_type, &file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
}

#if C_CYCLIC_VIDEO_RECORD == CUSTOM_ON
void ap_storage_service_cyclic_record_file_open_handle(INT32U req_type)
{
	INT32U				reply_type;
	STOR_SERV_FILEINFO	file_info;
	CHAR				f_name[24], f_name_b[24];
	struct stat_t		buf_tmp, buf_tmp_b;
	INT32S				del_index, i, ret;
	INT32U				file_size_to_create;
	INT8U				file_index_full_flag = 0;
	INT8U FlagToDel;
	INT8U FrontIsFound, RearIsFound;
	INT32S IndexForFront = 0, IndexForRear = 0;

	//DBG_PRINT("\r\n service_cyclic_record_file_open_handle\r\n");
	ap_storage_service_timer_stop();

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	chkdirvol("C:\\DCIM\\DCIMA"); //exFAT extend DCIMA size
	#else
	chdir("C:\\DCIM");
	chkdirvol("C:\\DCIM\\DCIMA"); //exFAT extend DCIMA size
	#endif

	for(i=0; i<10000; i++)
	{
		if
		(
			(h264_file_table[g_file_index >> 4] & (1 << (g_file_index & 0xF)))
		||	(h264_file_table_b[g_file_index >> 4] & (1 << (g_file_index & 0xF)))
		||	(lock_file_table[g_file_index >> 4] & (1 << (g_file_index & 0xF)))
		||	(lock_file_table_b[g_file_index >> 4] & (1 << (g_file_index & 0xF)))
		||	(jpg_file_table[g_file_index >> 4] & (1 << (g_file_index & 0xF)))
		||	(jpg_file_table_b[g_file_index >> 4] & (1 << (g_file_index & 0xF)))
		||	(wav_file_table[g_file_index >> 4] & (1 << (g_file_index & 0xF)))
		)
		{
			g_file_index++;
			if(g_file_index > 9999)
			{
				g_file_index = 0;
			}
		}
		else
		{
			break;
		}
	}

	if(i==10000)
	{
		file_index_full_flag = 1;
	}

	switch(req_type)
	{
    case MSG_STORAGE_SERVICE_VID_H264_CYCLIC_REQ:
        if(dual_mode)
        {
            file_size_to_create = h264_file_size + h264_file_size_back;
        }
        else
        {
            file_size_to_create = h264_file_size;
        }

        file_size_to_create <<= 20;

        FlagToDel = 0;
        FrontIsFound = 0;
        RearIsFound = 0;

        i = 0;
        if(file_index_full_flag)
        {
            i = 10000;
        }

        /*===============================================*/
        //DBG_PRINT("\r\ncur_lock_file_size= %lld\r\n", cur_lock_file_size);
        //DBG_PRINT("max_lock_file_size= %lld\r\n", max_lock_file_size);
        if(cur_lock_file_size >= max_lock_file_size)
        {
            for(; i<10000; i++) {
                del_index = g_oldest_lock_file_index;
                if((del_index & 0x000f) == 0) {
                    if((lock_file_table[del_index >> 4] == 0)&&(lock_file_table_b[del_index >> 4] == 0))
                    {
                        g_oldest_lock_file_index += 16;
                        if(g_oldest_lock_file_index > 9999) g_oldest_lock_file_index = 0;
                            i += 15;
                        continue;
                    }
                }
                /*DBG_PRINT("===============================\r\n");
                DBG_PRINT("g_oldest_lock_file_index= %d\r\n", g_oldest_lock_file_index);
                DBG_PRINT("del_index = %d\r\n", del_index);
                DBG_PRINT("===============================\r\n");*/
                if (lock_file_table[del_index >> 4] & (1<<(del_index & 0x000f))) {
                    sprintf((char *) f_name, (const char *) "DCIMA\\LOCA%04d.mov", del_index);
                    stat((CHAR *) f_name, &buf_tmp);
                    _setfattr(f_name, D_NORMAL);
                    DBG_PRINT("del lock file = %s\r\n", f_name);
                    unlink((CHAR *)f_name);
                    lock_file_table[del_index >> 4] &= ~(1 << (del_index & 0xF));
                    cur_lock_file_size -= buf_tmp.st_size/(1024*1024);
                    DBG_PRINT("lock file size = %lld\r\n", cur_lock_file_size);
                    if(cur_lock_file_size<=max_lock_file_size)
                    {
                        break;
                    }
				}
                if (lock_file_table_b[del_index >> 4] & (1<<(del_index & 0x000f))) {
                    sprintf((char *) f_name, (const char *) "DCIMB\\LOCB%04d.mov", del_index);
                    stat((CHAR *) f_name, &buf_tmp);
                    _setfattr(f_name, D_NORMAL);
                    DBG_PRINT("del lock file = %s\r\n", f_name);
                    unlink((CHAR *)f_name);
                    lock_file_table_b[del_index >> 4] &= ~(1 << (del_index & 0xF));
                    cur_lock_file_size -= buf_tmp.st_size/(1024*1024);
                    DBG_PRINT("lock file size = %lld\r\n", cur_lock_file_size);
                    if(cur_lock_file_size<=max_lock_file_size)
                    {
                        break;
                    }
				}

                del_index += 1;
                if(del_index > 9999) del_index = 0;
                g_oldest_lock_file_index = del_index;
			}
        }
        /*===============================================*/
        #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
        if(vfsFreeSpace(STORAGE_DEVICE) < file_size_to_create)
        #else
        ret = UsrCheckFreeSize(STORAGE_DEVICE, file_size_to_create);
        if(ret < 2)
        #endif
        {
            i = 0;
            if(file_index_full_flag)
            {
                i = 10000;
            }

            if(cyclic_off)
                i = 10000;

            for(; i < 10000; i++)
            {
                del_index = g_oldest_h264_file_index;
                if((del_index & 0x000f) == 0)
                {
                    if((h264_file_table[del_index >> 4] == 0) && (h264_file_table_b[del_index >> 4] == 0))
                    {
                        g_oldest_h264_file_index += 16;
                        if(g_oldest_h264_file_index > 9999)
                            g_oldest_h264_file_index = 0;
                        i += 15;
                        continue;
                    }
                }

                if(h264_file_table[del_index >> 4] & (1 << (del_index & 0x000f)))
                {
                    sprintf((char *) f_name, (const char *) "DCIMA\\MOVA%04d.mov", del_index);
                    if((stat(f_name, &buf_tmp) == SUCCESS) && (buf_tmp.st_mode != D_RDONLY))
                    {
                        if(FlagToDel == 0)
                        {
                            #if OPEN_FILE_MODE == FIXED_SIZE_FILE
                            if(buf_tmp.st_size == (h264_file_size << 20))
                            {
                                if(FrontIsFound)
                                {
                                    FlagToDel = 1;
                                    FrontIsFound = 0;

                                    sprintf((char *)f_name, (const char *)"DCIMA\\MOVA%04d.mov", IndexForFront);
                                    DBG_PRINT("del = %s\r\n", f_name);
                                    unlink((CHAR *)f_name);
                                    h264_file_table[IndexForFront >> 4] &= ~(1 << (IndexForFront & 0xF));

                                    #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
                                    if(vfsFreeSpace(STORAGE_DEVICE) > file_size_to_create)
                                    #else
                                    ret = UsrCheckFreeSize(STORAGE_DEVICE, file_size_to_create);
                                    if(ret > 2)
                                    #endif
                                    break;
                                }
                                else
                                {
                                    FrontIsFound = 1;
                                    IndexForFront = del_index;

                                    if(dual_mode)
                                    {
                                        if(RearIsFound)
                                        {
                                            break;
                                        }
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                            }
                            else
                            #endif
                            {
                                FlagToDel = 1;

                                if(FrontIsFound)
                                {
                                    FrontIsFound = 0;

                                    sprintf((char *)f_name, (const char *)"DCIMA\\MOVA%04d.mov", IndexForFront);
                                    DBG_PRINT("del = %s\r\n", f_name);
                                    unlink((CHAR *)f_name);
                                    h264_file_table[IndexForFront >> 4] &= ~(1 << (IndexForFront & 0xF));

                                    #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
                                    if(vfsFreeSpace(STORAGE_DEVICE) > file_size_to_create)
                                    #else
                                    ret = UsrCheckFreeSize(STORAGE_DEVICE, file_size_to_create);
                                    if(ret > 2)
                                    #endif
                                    break;
                                }
                                else if(RearIsFound)
                                {
                                    RearIsFound = 0;

                                    sprintf((char *)f_name_b, (const char *)"DCIMB\\MOVB%04d.mov", IndexForRear);
                                    DBG_PRINT("del = %s\r\n", f_name_b);
                                    unlink((CHAR *)f_name_b);
                                    h264_file_table_b[IndexForRear >> 4] &= ~(1 << (IndexForRear & 0xF));

                                    #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
                                    if(vfsFreeSpace(STORAGE_DEVICE) > file_size_to_create)
                                    #else
                                    ret = UsrCheckFreeSize(STORAGE_DEVICE, file_size_to_create);
                                    if(ret > 2)
                                    #endif
                                    break;
                                }
                            }
                        }

                        if(FlagToDel)
                        {
                            sprintf((char *)f_name, (const char *)"DCIMA\\MOVA%04d.mov", del_index);
                            DBG_PRINT("del = %s\r\n", f_name);
                            unlink((CHAR *)f_name);
                            h264_file_table[del_index >> 4] &= ~(1 << (del_index & 0xF));

                            #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
                            if(vfsFreeSpace(STORAGE_DEVICE) > file_size_to_create)
                            #else
                            ret = UsrCheckFreeSize(STORAGE_DEVICE, file_size_to_create);
                            if(ret > 2)
                            #endif
                            break;
                        }
                    }
                }

                if(h264_file_table_b[del_index >> 4] & (1 << (del_index & 0x000f)))
                {
                    sprintf((char *) f_name_b, (const char *) "DCIMB\\MOVB%04d.mov", del_index);
                    if((stat(f_name_b, &buf_tmp_b) == SUCCESS) && (buf_tmp_b.st_mode != D_RDONLY))
                    {
                        if(FlagToDel == 0)
                        {
                            if(dual_mode)
                            {
                                #if OPEN_FILE_MODE == FIXED_SIZE_FILE
                                if(buf_tmp_b.st_size == (h264_file_size_back << 20))
                                {
                                    if(RearIsFound)
                                    {
                                        FlagToDel = 1;
                                        RearIsFound = 0;

                                        sprintf((char *)f_name_b, (const char *)"DCIMB\\MOVB%04d.mov", IndexForRear);
                                        DBG_PRINT("del = %s\r\n", f_name_b);
                                        unlink((CHAR *)f_name_b);
                                        h264_file_table_b[IndexForRear >> 4] &= ~(1 << (IndexForRear & 0xF));

                                        #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
                                        if(vfsFreeSpace(STORAGE_DEVICE) > file_size_to_create)
                                        #else
                                        ret = UsrCheckFreeSize(STORAGE_DEVICE, file_size_to_create);
                                        if(ret > 2)
                                        #endif
                                        break;
                                    }
                                    else
                                    {
                                        RearIsFound = 1;
                                        IndexForRear = del_index;

                                        if(FrontIsFound)
                                        {
                                            del_index += 1;
                                            if(del_index > 9999) del_index = 0;
                                            g_oldest_h264_file_index = del_index;
                                            break;
                                        }
                                    }
                                }
                                else
                                #endif
                                {
                                    FlagToDel = 1;

                                    if(FrontIsFound)
                                    {
                                        FrontIsFound = 0;

                                        sprintf((char *)f_name, (const char *)"DCIMA\\MOVA%04d.mov", IndexForFront);
                                        DBG_PRINT("del = %s\r\n", f_name);
                                        unlink((CHAR *)f_name);
                                        h264_file_table[IndexForFront >> 4] &= ~(1 << (IndexForFront & 0xF));

                                        #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
                                        if(vfsFreeSpace(STORAGE_DEVICE) > file_size_to_create)
                                        #else
                                        ret = UsrCheckFreeSize(STORAGE_DEVICE, file_size_to_create);
                                        if(ret > 2)
                                        #endif
                                        break;
                                    }
                                    else if(RearIsFound)
                                    {
                                        RearIsFound = 0;

                                        sprintf((char *)f_name_b, (const char *)"DCIMB\\MOVB%04d.mov", IndexForRear);
                                        DBG_PRINT("del = %s\r\n", f_name_b);
                                        unlink((CHAR *)f_name_b);
                                        h264_file_table_b[IndexForRear >> 4] &= ~(1 << (IndexForRear & 0xF));

                                        #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
                                        if(vfsFreeSpace(STORAGE_DEVICE) > file_size_to_create)
                                        #else
                                        ret = UsrCheckFreeSize(STORAGE_DEVICE, file_size_to_create);
                                        if(ret > 2)
                                        #endif
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                FlagToDel = 1;
                            }
                        }

                        if(FlagToDel)
                        {
                            sprintf((char *)f_name_b, (const char *)"DCIMB\\MOVB%04d.mov", del_index);
                            DBG_PRINT("del = %s\r\n", f_name_b);
                            unlink((CHAR *)f_name_b);
                            h264_file_table_b[del_index >> 4] &= ~(1 << (del_index & 0xF));

                            #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
                            if(vfsFreeSpace(STORAGE_DEVICE) > file_size_to_create)
                            #else
                            ret = UsrCheckFreeSize(STORAGE_DEVICE, file_size_to_create);
                            if(ret > 2)
                            #endif
                            {
                                del_index += 1;
                                if(del_index > 9999) del_index = 0;
                                g_oldest_h264_file_index = del_index;
                                break;
                            }
                        }
                    }
                }

                del_index += 1;
                if(del_index > 9999)
                    del_index = 0;
                g_oldest_h264_file_index = del_index;
            }
        }

#if C_CREATE_FILE_DEBUG
        FatSpeedUpPrintf(STORAGE_DEVICE);
#endif
        reply_type = MSG_STORAGE_SERVICE_VID_CYCLIC_REPLY;

        file_info.file_handle = -1;
        file_info.file_handle_b = -1;
        if(i < 10000)
        {
            sprintf((char *) g_next_file_path, (const char *) "DCIMA\\MOVA%04d.mov", g_file_index);
            sprintf((char *) g_next_file_path_b, (const char *) "DCIMB\\MOVB%04d.mov", g_file_index);
            file_info.file_path_addr = (INT32U) g_next_file_path;
            file_info.file_path_addr_b = (INT32U) g_next_file_path_b;
            #if OPEN_FILE_MODE == FIXED_SIZE_FILE
            if((FlagToDel == 0) && FrontIsFound)
            {
                _rename_fornewfile(f_name,g_next_file_path);
                DBG_PRINT("replace = %s\r\n", f_name);
                file_info.file_handle = open(g_next_file_path, O_RDWR);
                if(file_info.file_handle >= 0)
                {
                    DBG_PRINT("\r\nFileName = %s\r\n", file_info.file_path_addr);
                    h264_file_table[IndexForFront >> 4] &= ~(1 << (IndexForFront & 0xF));
                    h264_file_table[g_file_index >> 4] |= 1 << (g_file_index & 0xF);
                    if(RearIsFound)
                    {
                        _rename_fornewfile(f_name_b, g_next_file_path_b);
                        DBG_PRINT("replace = %s\r\n", f_name_b);
                        file_info.file_handle_b = open(g_next_file_path_b, O_RDWR);
                        DBG_PRINT("BackFileName = %s\r\n", file_info.file_path_addr_b);
                        h264_file_table_b[IndexForRear >> 4] &= ~(1 << (IndexForRear & 0xF));
                        h264_file_table_b[g_file_index >> 4] |= 1 << (g_file_index & 0xF);
                    }
                }
            }
            else
            #endif
            {

				#if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
				file_info.file_handle = open(g_next_file_path, O_WRONLY | O_CREAT | O_TRUNC);
				#else
				file_info.file_handle = CreatFileBySize(g_next_file_path, (h264_file_size << 20));
                #endif
                if(file_info.file_handle >= 0) {
                    DBG_PRINT("\r\nFileName = %s\r\n", file_info.file_path_addr);
                    h264_file_table[g_file_index >> 4] |= 1 << (g_file_index & 0xF);
                    if(dual_mode) {

                        #if OPEN_FILE_MODE == UNFIXED_SIZE_FILE
                        file_info.file_handle_b = open(g_next_file_path_b, O_WRONLY | O_CREAT | O_TRUNC);
                        #else
                        file_info.file_handle_b = CreatFileBySize(g_next_file_path_b, (h264_file_size_back << 20));
                        #endif
                        DBG_PRINT("BackFileName = %s\r\n", file_info.file_path_addr_b);
                        h264_file_table_b[g_file_index >> 4] |= 1 << (g_file_index & 0xF);
                    }
                }
            }

            g_file_index++;
            if(g_file_index == 10000){
                g_file_index = 0;
                g_file_index_9999_exist = 1;
            }

            #if C_CREATE_FILE_DEBUG
            FatSpeedUpPrintf(STORAGE_DEVICE);
            #endif
        }

        if(file_info.file_handle != -1)
        {
            UpdateDir(file_info.file_handle);
        }

        if(file_info.file_handle_b != -1)
        {
            UpdateDir(file_info.file_handle_b);
        }

        UsrFlushBuffers(STORAGE_DEVICE);
        break;
	}

	app_msg_send(reply_type, &file_info, sizeof(STOR_SERV_FILEINFO), MSG_PRI_NORMAL);
}

#endif


void select_latest_file_for_browsing(STOR_SERV_PLAYINFO *info_ptr, INT8U select_a_b)
{
	INT16S		index_tmp, i = 0, k, l;
	INT8U       flag = 0;
	struct stat_t	buf_tmp;
	INT32U      max_temp;

	if(g_file_index_9999_exist)
	{
		max_temp = g_h264_file_time;
		index_tmp = g_h264_index;

		if(max_temp < g_lock_file_time)
		{
			max_temp = g_lock_file_time;
			index_tmp = g_lock_index;
		}

		if(max_temp < g_jpeg_file_time)
		{
			max_temp = g_jpeg_file_time;
			index_tmp = g_jpeg_index;
		}

        if(max_temp < g_wav_file_time)
		{
			max_temp = g_wav_file_time;
			index_tmp = g_wav_index;
		}
    }
    else
    {
        index_tmp = max(max(max(g_h264_index, g_lock_index), g_jpeg_index),g_wav_index);
    }
    index_tmp -= 1;
    if(index_tmp < 0)
    {
        index_tmp = 9999;
    }

    k = index_tmp >> 4;
    l = index_tmp & 0xF;
    while(i <= 626)
    {
        i++;
        if(lock_file_table[k] || jpg_file_table[k] || lock_file_table_b[k] || jpg_file_table_b[k] ||h264_file_table_b[k] ||h264_file_table[k]||wav_file_table[k])
        {
            for(; l >= 0; l--)
            {
                if(select_a_b == 0)
                {
                    if(lock_file_table[k] & (1 << l))
                    {
                        g_play_index = (k << 4) + l;
                        info_ptr->file_type = TK_IMAGE_TYPE_H264;
#if C_RENAME_LOCK_FILE
                        sprintf((char *) g_file_path, (const char *) "DCIMA\\LOCA%04d.mov", g_play_index);
                        if(stat(g_file_path, &buf_tmp) != SUCCESS)
                        {
#else
                        sprintf((char *) g_file_path, (const char *) "DCIMA\\MOVA%04d.mov", g_play_index);
                        if(stat(g_file_path, &buf_tmp) != SUCCESS)
                        {
#endif
                            continue;
                        }

                        flag = 1;
                        break;
                    }
                    else
                    if(jpg_file_table[k] & (1 << l))
                    {
                        g_play_index = (k << 4) + l;
                        info_ptr->file_type = TK_IMAGE_TYPE_JPEG;
                        sprintf((char *) g_file_path, (const char *) "DCIMA\\PICA%04d.jpg", g_play_index);
                        flag = 1;
                        break;
                    }
                    else
                    if(wav_file_table[k] & (1 << l))
                    {
                        g_play_index = (k << 4) + l;
                        info_ptr->file_type = TK_IMAGE_TYPE_WAV;
                        sprintf((char *) g_file_path, (const char *) "DCIMA\\RECR%04d.wav", g_play_index);
                        flag = 1;
                        break;
                    }
                    else if(h264_file_table[k] & (1 << l))
                    {
                        g_play_index = (k << 4) + l;
                        info_ptr->file_type = TK_IMAGE_TYPE_H264;
                        sprintf((char *) g_file_path, (const char *) "DCIMA\\MOVA%04d.mov", g_play_index);
                        if(stat(g_file_path, &buf_tmp) != SUCCESS)
                        {
                            continue;
                        }
                        flag = 1;
                        break;
                    }
                }
                else
                {
                    if(lock_file_table_b[k] & (1 << l))
                    {
                        g_play_index = (k << 4) + l;
                        info_ptr->file_type = TK_IMAGE_TYPE_H264;
#if C_RENAME_LOCK_FILE
                        sprintf((char *) g_file_path, (const char *) "DCIMB\\LOCB%04d.mov", g_play_index);
                        if(stat(g_file_path, &buf_tmp) != SUCCESS)
                        {
#else
                        sprintf((char *) g_file_path, (const char *) "DCIMB\\MOVB%04d.mov", g_play_index);
                        if(stat(g_file_path, &buf_tmp) != SUCCESS)
                        {
#endif
                                continue;
                        }

                        flag = 1;
                        break;
                    }
                    else
                    if(jpg_file_table_b[k] & (1 << l))
                    {
                        g_play_index = (k << 4) + l;
                        info_ptr->file_type = TK_IMAGE_TYPE_JPEG;
                        sprintf((char *) g_file_path, (const char *) "DCIMB\\PICB%04d.jpg", g_play_index);
                        flag = 1;
                        break;
                    }
                    else if(h264_file_table_b[k] & (1 << l))
                    {
                        g_play_index = (k << 4) + l;
                        info_ptr->file_type = TK_IMAGE_TYPE_H264;
                        sprintf((char *) g_file_path, (const char *) "DCIMB\\MOVB%04d.mov", g_play_index);
                        if(stat(g_file_path, &buf_tmp) != SUCCESS)
                        {
                            continue;
                        }
                        flag = 1;
                        break;
                    }
                }
            }

            if(flag)
            {
                break;
            }
        }

        l = 0xF;
        k--;
        if(k < 0)
        {
            k = 624;
        }
    }

    if(i > 626)
    {
        ERR_PRINT("find file err\r\n");
    }
}


void select_current_file_type(STOR_SERV_PLAYINFO *info_ptr, INT8U select_a_b, INT16S file_index)
{
	INT16S		k, l;

    g_play_index = file_index;
    k = g_play_index >> 4;
    l = g_play_index & 0xF;

    if(select_a_b == 0)
    {
        if(lock_file_table[k] & (1 << l))
        {
            info_ptr->file_type = TK_IMAGE_TYPE_H264;
#if C_RENAME_LOCK_FILE
            sprintf((char *) g_file_path, (const char *) "DCIMA\\LOCA%04d.mov", g_play_index);
#else
            sprintf((char *) g_file_path, (const char *) "DCIMA\\MOVA%04d.mov", g_play_index);
#endif
        }
        else
        if(jpg_file_table[k] & (1 << l))
        {
            info_ptr->file_type = TK_IMAGE_TYPE_JPEG;
            sprintf((char *) g_file_path, (const char *) "DCIMA\\PICA%04d.jpg", g_play_index);
        }
        else
        if(wav_file_table[k] & (1 << l))
        {
            info_ptr->file_type = TK_IMAGE_TYPE_WAV;
            sprintf((char *) g_file_path, (const char *) "DCIMA\\RECR%04d.wav", g_play_index);
        }
        else
        if(h264_file_table[k] & (1 << l))
        {
            info_ptr->file_type = TK_IMAGE_TYPE_H264;
            sprintf((char *) g_file_path, (const char *) "DCIMA\\MOVA%04d.mov", g_play_index);
        }
    }
    else
    {
        if(lock_file_table_b[k] & (1 << l))
        {
            info_ptr->file_type = TK_IMAGE_TYPE_H264;
#if C_RENAME_LOCK_FILE
            sprintf((char *) g_file_path, (const char *) "DCIMB\\LOCB%04d.mov", g_play_index);
#else
            sprintf((char *) g_file_path, (const char *) "DCIMB\\MOVB%04d.mov", g_play_index);
#endif
        }
        else
        if(jpg_file_table_b[k] & (1 << l))
        {
            info_ptr->file_type = TK_IMAGE_TYPE_JPEG;
            sprintf((char *) g_file_path, (const char *) "DCIMB\\PICB%04d.jpg", g_play_index);
        }
        else
        if(h264_file_table_b[k] & (1 << l))
        {
            info_ptr->file_type = TK_IMAGE_TYPE_H264;
            sprintf((char *) g_file_path, (const char *) "DCIMB\\MOVB%04d.mov", g_play_index);
        }
    }
}

#if (defined C_WIFI_FUNC) && (C_WIFI_FUNC)

void rtsp_method_type_set(int val);
int Playback_Type_Get(INT32U req_msg)
{
	INT16S	k, l;
	INT32U play_index = req_msg;

	k = play_index >> 4;
	l = play_index & 0xF;

	if(jpg_file_table[k] & (1 << l))
	{
		rtsp_method_type_set(1);		// jpeg
		///DBG_PRINT("JPEG flow = %d\r\n",play_index);
	}
	else
	{
		rtsp_method_type_set(1);		// jpeg
		//rtsp_method_type_set(0);		// h264
		///DBG_PRINT("H264 flow\r\n",play_index);
	}

	return 0;
}

#endif


void ap_storage_service_play_req(INT32U req_msg)
{
	STOR_SERV_PLAYINFO	play_info;
	STOR_SERV_PLAYINFO	*info_ptr = &play_info;

	INT16S				index_tmp, i = 0, k, l;
	INT8U				type = (req_msg & 0xFF), err_flag = ((req_msg & 0xFF00) >> 8);
	INT16U				given_play_index = ((req_msg >> 16) & 0xFFFF);
	struct stat_t		buf_tmp;

	ap_storage_service_timer_stop();

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	#else
	chdir("C:\\DCIM");
	#endif

	#if defined(C_WIFI_FUNC) && C_WIFI_FUNC
	if((state_wifi_status_get() & FLAG_CONNECT)&&(type == STOR_SERV_SEARCH_GIVEN))
	{
		g_play_index = given_play_index;
	}
    #endif

	info_ptr->search_type = type;
	if(type == STOR_SERV_SEARCH_INIT)
	{
		g_play_index = -1;
	}

	if(g_play_index < 0)
	{
		INT8U	select_a_b = 0;

        reinit_all_table();

		if(g_file_num_a)
		{
			select_a_b = 0;
			g_trace_cnt = g_file_num_a - 1;
		}
		else
		if(g_file_num_b)
		{
			select_a_b = 1;
			g_trace_cnt = g_file_num_b - 1;
		}
		/*else
		if(g_wav_index)
		{
			info_ptr->file_type = TK_IMAGE_TYPE_WAV;
			g_play_index = g_wav_index - 1;
			if(g_play_index < 0)
			{
				g_play_index = 9999;
			}

			sprintf((char *) g_file_path, (const char *) "RECR%04d.wav", g_play_index);
		}*/
		else
		{
			info_ptr->err_flag = STOR_SERV_NO_MEDIA;
			app_msg_send(MSG_STORAGE_SERVICE_BROWSE_REPLY, info_ptr, sizeof(STOR_SERV_PLAYINFO), MSG_PRI_NORMAL);
			return;
		}

		if(g_file_num_a || g_file_num_b)
		{
            select_latest_file_for_browsing(info_ptr, select_a_b);
		}
	}
	else
	{
		if(type == STOR_SERV_SEARCH_PREV)
		{
			INT8U	flag = 0;
			INT8U	select_a_b = 0, switch_flag = 0;

			if(g_file_num_a || g_file_num_b)
			{
				g_trace_cnt--;
				if(g_trace_cnt > 9999)
				{
					switch_flag = 1;

					if(((g_file_path[4] == 'A')||(g_file_path[4] == 'R')) && g_file_num_b)
					{
						g_trace_cnt = g_file_num_b - 1;
						select_a_b = 1;
					}
					else
					if((g_file_path[4] == 'B') && g_file_num_a)
					{
						g_trace_cnt = g_file_num_a - 1;
						select_a_b = 0;
					}
					else
					if((g_file_path[4] == 'A')||(g_file_path[4] == 'R'))
					{
						g_trace_cnt = g_file_num_a - 1;
						select_a_b = 0;
					}
					else
					{
						g_trace_cnt = g_file_num_b - 1;
						select_a_b = 1;
					}
				}
				else
				{
					if((g_file_path[4] == 'A')||(g_file_path[4] == 'R'))
						select_a_b = 0;
					else
						select_a_b = 1;
				}
			}

			if(switch_flag)
			{
                select_latest_file_for_browsing(info_ptr, select_a_b);
			}
			else
			{
				index_tmp = g_play_index - 1;
				if(index_tmp < 0)
				{
					index_tmp = 9999;
				}

				k = index_tmp >> 4;
				l = index_tmp & 0xF;
				while(i <= 626)
				{
					i++;
					if(lock_file_table[k] || jpg_file_table[k] || lock_file_table_b[k] || jpg_file_table_b[k] ||h264_file_table_b[k] ||h264_file_table[k]||wav_file_table[k])
					{
						for(; l >= 0; l--)
						{
							if(select_a_b == 0)
							{
								if(lock_file_table[k] & (1 << l))
								{
									g_play_index = (k << 4) + l;
									info_ptr->file_type = TK_IMAGE_TYPE_H264;
#if C_RENAME_LOCK_FILE
									sprintf((char *) g_file_path, (const char *) "DCIMA\\LOCA%04d.mov", g_play_index);
									if(stat(g_file_path, &buf_tmp) != SUCCESS)
									{
#else
									sprintf((char *) g_file_path, (const char *) "DCIMA\\MOVA%04d.mov", g_play_index);
									if(stat(g_file_path, &buf_tmp) != SUCCESS)
									{
#endif
                                        continue;
                                    }

									flag = 1;
									break;
								}
								else
								if(jpg_file_table[k] & (1 << l))
								{
									g_play_index = (k << 4) + l;
									info_ptr->file_type = TK_IMAGE_TYPE_JPEG;
									sprintf((char *) g_file_path, (const char *) "DCIMA\\PICA%04d.jpg", g_play_index);
									flag = 1;
									break;
								}
								else
								if(wav_file_table[k] & (1 << l))
								{
									g_play_index = (k << 4) + l;
									info_ptr->file_type = TK_IMAGE_TYPE_WAV;
									sprintf((char *) g_file_path, (const char *) "DCIMA\\RECR%04d.wav", g_play_index);
									flag = 1;
									break;
								}
								else if(h264_file_table[k] & (1 << l))
								{
                                    g_play_index = (k << 4) + l;
                                    info_ptr->file_type = TK_IMAGE_TYPE_H264;
                                    sprintf((char *) g_file_path, (const char *) "DCIMA\\MOVA%04d.mov", g_play_index);
                                    if(stat(g_file_path, &buf_tmp) != SUCCESS)
                                    {
                                        continue;
                                    }
                                    flag = 1;
                                    break;
								}
							}
							else
							{
								if(lock_file_table_b[k] & (1 << l))
								{
									g_play_index = (k << 4) + l;
									info_ptr->file_type = TK_IMAGE_TYPE_H264;
#if C_RENAME_LOCK_FILE
									sprintf((char *) g_file_path, (const char *) "DCIMB\\LOCB%04d.mov", g_play_index);
									if(stat(g_file_path, &buf_tmp) != SUCCESS)
									{
#else
									sprintf((char *) g_file_path, (const char *) "DCIMB\\MOVB%04d.mov", g_play_index);
									if(stat(g_file_path, &buf_tmp) != SUCCESS)
									{
#endif
											continue;
									}

									flag = 1;
									break;
								}
								else
								if(jpg_file_table_b[k] & (1 << l))
								{
									g_play_index = (k << 4) + l;
									info_ptr->file_type = TK_IMAGE_TYPE_JPEG;
									sprintf((char *) g_file_path, (const char *) "DCIMB\\PICB%04d.jpg", g_play_index);
									flag = 1;
									break;
								}
								else if(h264_file_table_b[k] & (1 << l))
								{
									g_play_index = (k << 4) + l;
									info_ptr->file_type = TK_IMAGE_TYPE_H264;
									sprintf((char *) g_file_path, (const char *) "DCIMB\\MOVB%04d.mov", g_play_index);
									if(stat(g_file_path, &buf_tmp) != SUCCESS)
									{
										continue;
									}
									flag = 1;
									break;
								}
							}
						}

						if(flag)
						{
							break;
						}
					}

					l = 0xF;
					k--;
					if(k < 0)
					{
						k = 624;
					}
				}

				if(i > 626)
				{
					info_ptr->err_flag = STOR_SERV_NO_MEDIA;
					g_err_cnt = 0;
					app_msg_send(MSG_STORAGE_SERVICE_BROWSE_REPLY, info_ptr, sizeof(STOR_SERV_PLAYINFO), MSG_PRI_NORMAL);
					return;
				}
			}
		}
		else
		if(type == STOR_SERV_SEARCH_NEXT)
		{
			INT8U	flag = 0;
			INT8U	select_a_b = 0, switch_flag = 0;

			if(g_file_num_a || g_file_num_b)
			{
				g_trace_cnt++;
				if((g_file_path[4] == 'A')||(g_file_path[4] == 'R'))
				{
					if(g_trace_cnt >= g_file_num_a)
					{
						g_trace_cnt = 0;
						switch_flag = 1;

						if(g_file_num_b)
						{
							select_a_b = 1;
						}
						else
						{
							select_a_b = 0;
						}
					}
					else
					{
						if((g_file_path[4] == 'A')||(g_file_path[4] == 'R'))
							select_a_b = 0;
						else
							select_a_b = 1;
					}
				}
				else
				{
					if(g_trace_cnt >= g_file_num_b)
					{
						g_trace_cnt = 0;
						switch_flag = 1;

						if(g_file_num_a)
						{
							select_a_b = 0;
						}
						else
						{
							select_a_b = 1;
						}
					}
					else
					{
						if((g_file_path[4] == 'A')||(g_file_path[4] == 'R'))
							select_a_b = 0;
						else
							select_a_b = 1;
					}
				}
			}

			if(switch_flag)
			{
                select_latest_file_for_browsing(info_ptr, select_a_b);
			}

			index_tmp = g_play_index + 1;
			if(index_tmp > 9999)
			{
				index_tmp = 0;
			}

			k = index_tmp >> 4;
			l = index_tmp & 0xF;
			while(i <= 626)
			{
				i++;
				if(lock_file_table[k] || jpg_file_table[k] || lock_file_table_b[k] || jpg_file_table_b[k] ||h264_file_table_b[k]|| h264_file_table[k] || wav_file_table[k])
				{
					for(; l < 0x10; l++)
					{
						if(select_a_b == 0)
						{
							if(lock_file_table[k] & (1 << l))
							{
								g_play_index = (k << 4) + l;
								info_ptr->file_type = TK_IMAGE_TYPE_H264;

#if C_RENAME_LOCK_FILE
								sprintf((char *) g_file_path, (const char *) "DCIMA\\LOCA%04d.mov", g_play_index);
								if(stat(g_file_path, &buf_tmp) != SUCCESS)
								{
#else
								sprintf((char *) g_file_path, (const char *) "DCIMA\\MOVA%04d.mov", g_play_index);
								if(stat(g_file_path, &buf_tmp) != SUCCESS)
								{
#endif
										continue;
								}

								flag = 1;
								break;
							}
							else
							if(jpg_file_table[k] & (1 << l))
							{
								g_play_index = (k << 4) + l;
								info_ptr->file_type = TK_IMAGE_TYPE_JPEG;
								sprintf((char *) g_file_path, (const char *) "DCIMA\\PICA%04d.jpg", g_play_index);
								flag = 1;
								break;
							}
							else
							if(wav_file_table[k] & (1 << l))
							{
								g_play_index = (k << 4) + l;
								info_ptr->file_type = TK_IMAGE_TYPE_WAV;
								sprintf((char *) g_file_path, (const char *) "DCIMA\\RECR%04d.wav", g_play_index);
								flag = 1;
								break;
							}
							else if(h264_file_table[k] & (1 << l))
							{
                                g_play_index = (k << 4) + l;
								info_ptr->file_type = TK_IMAGE_TYPE_H264;
								sprintf((char *) g_file_path, (const char *) "DCIMA\\MOVA%04d.mov", g_play_index);
							    if(stat(g_file_path, &buf_tmp) != SUCCESS)
                                {
                                    continue;
                                }
								flag = 1;
								break;
							}
						}
						else
						{
							if(lock_file_table_b[k] & (1 << l))
							{
								g_play_index = (k << 4) + l;
								info_ptr->file_type = TK_IMAGE_TYPE_H264;
#if C_RENAME_LOCK_FILE
                                sprintf((char *) g_file_path, (const char *) "DCIMB\\LOCB%04d.mov", g_play_index);
                                if(stat(g_file_path, &buf_tmp) != SUCCESS)
                                {
#else
								sprintf((char *) g_file_path, (const char *) "DCIMB\\MOVB%04d.mov", g_play_index);
								if(stat(g_file_path, &buf_tmp) != SUCCESS)
								{
#endif
										continue;
								}

								flag = 1;
								break;
							}
							else
							if(jpg_file_table_b[k] & (1 << l))
							{
								g_play_index = (k << 4) + l;
								info_ptr->file_type = TK_IMAGE_TYPE_JPEG;
								sprintf((char *) g_file_path, (const char *) "DCIMB\\PICB%04d.jpg", g_play_index);
								flag = 1;
								break;
							}
							else if(h264_file_table_b[k] & (1 << l))
							{
                                g_play_index = (k << 4) + l;
								info_ptr->file_type = TK_IMAGE_TYPE_H264;
								sprintf((char *) g_file_path, (const char *) "DCIMB\\MOVB%04d.mov", g_play_index);
							    if(stat(g_file_path, &buf_tmp) != SUCCESS)
                                {
                                    continue;
                                }
								flag = 1;
								break;

							}
						}
					}

					if(flag)
					{
						break;
					}
				}

				l = 0x0;
				k++;
				if(k > 624)
				{
					k = 0;
				}
			}

			if(i > 626)
			{
				info_ptr->err_flag = STOR_SERV_NO_MEDIA;
				g_err_cnt = 0;
				app_msg_send(MSG_STORAGE_SERVICE_BROWSE_REPLY, info_ptr, sizeof(STOR_SERV_PLAYINFO), MSG_PRI_NORMAL);
				return;
			}
		}
		else
		{
			INT8U	select_a_b;

			if(type == STOR_SERV_SEARCH_GIVEN)
			{
                #if defined(C_WIFI_FUNC) && C_WIFI_FUNC
                if(state_wifi_status_get() & FLAG_CONNECT)
                {
                    g_play_index = given_play_index;
                    if(g_play_index < 10000)
                    {
                        select_a_b = 0;		// DCIMA
                        g_trace_cnt = g_file_num_a - get_deleted_file_number(0);
                    }
                    else
                    {
                        select_a_b = 1;		// DCIMB
                        g_play_index -= 10000;
                        g_trace_cnt = g_file_num_b - get_deleted_file_number(1);
                    }
                    DBG_PRINT("select_a_b = %d, g_play_index = %d\r\n", select_a_b, g_play_index);
                }
                else
                #endif
                {
                    g_play_index = get_max_file_index(1);
                    deleted_num_b_max = get_deleted_file_number(1);

                    g_play_index = given_play_index;
                    if(g_file_num_a && (g_play_index >= (g_file_num_b + deleted_num_b_max)))
                    {
                        select_a_b = 0;
                        g_play_index -= (g_file_num_b + deleted_num_b_max);
                        g_trace_cnt = g_play_index - get_deleted_file_number(0);
                    }
                    else
                    {
                        select_a_b = 1;
                        g_trace_cnt = g_play_index - get_deleted_file_number(1);
                    }
                }
			}
			else
			{
				if((g_file_path[4] == 'A')||(g_file_path[4] == 'R'))
					select_a_b = 0;
				else
					select_a_b = 1;
			}

            select_current_file_type(info_ptr, select_a_b, g_play_index);
		}
	}

	DBG_PRINT("File Path=%s\r\n", g_file_path);

	info_ptr->file_path_addr = (INT32U) g_file_path;
	info_ptr->file_handle = open(g_file_path, O_RDONLY);
	if(info_ptr->file_handle >= 0)
	{
		stat(g_file_path, &buf_tmp);
		info_ptr->file_size = buf_tmp.st_size;
		if(!err_flag)
		{
			info_ptr->err_flag = STOR_SERV_OPEN_OK;
			g_err_cnt = 0;
		}
		else
		{
			g_err_cnt++;
			if(g_err_cnt <= g_file_num)
			{
				info_ptr->err_flag = STOR_SERV_OPEN_OK;
			}
			else
			{
				info_ptr->err_flag = STOR_SERV_DECODE_ALL_FAIL;
			}
		}
	}
	else
	{
		info_ptr->err_flag = STOR_SERV_OPEN_FAIL;
		g_err_cnt = 0;
	}

	if(type != STOR_SERV_SEARCH_GIVEN)          //re-compute deleted_num_b_max
	{
        index_tmp = g_play_index;               //backup g_play_index
        g_play_index = get_max_file_index(1);
        deleted_num_b_max = get_deleted_file_number(1);
        g_play_index = index_tmp;               //restore g_play_index
    }

	if((g_file_path[4] == 'A')||(g_file_path[4] == 'R'))
	{
		info_ptr->deleted_file_number = get_deleted_file_number(0) + deleted_num_b_max;
	}
	else
	{
		info_ptr->deleted_file_number = get_deleted_file_number(1);
	}

	info_ptr->play_index = g_play_index;
	if((g_file_path[4] == 'A')||(g_file_path[4] == 'R'))
	{
		info_ptr->play_index += (g_file_num_b + deleted_num_b_max);
	}

	info_ptr->total_file_number = g_file_num;
	//DBG_PRINT("g_play_index=%d \r\n",g_play_index);
	app_msg_send(MSG_STORAGE_SERVICE_BROWSE_REPLY, info_ptr, sizeof(STOR_SERV_PLAYINFO), MSG_PRI_NORMAL);
}

INT32S get_file_final_h264_index(void)
{
	CHAR			*pdata;
	INT32S			nRet, temp;
	INT32U			temp_time;
	struct f_info	file_info;

	g_h264_index = -1;
	g_h264_index_9999_exist = g_h264_file_time = g_h264_file_oldest_time = 0;
	temp_oldest_h264_index = 10000;

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM\\DCIMA");
	#else
	chdir("C:\\DCIM\\DCIMA");
	#endif

	nRet = _findfirst("*.mov", &file_info, D_ALL);
	if(nRet >= 0)
	{
		while(1)
		{
			pdata = (CHAR *) file_info.f_name;

#if !C_RENAME_LOCK_FILE
            if((gp_strncmp((INT8S *) pdata, (INT8S *) "MOV", 3) == 0) && ((file_info.f_attrib & _A_RDONLY) == 0))
#else
            if(gp_strncmp((INT8S *) pdata, (INT8S *) "MOV", 3) == 0)
#endif
            {
                temp = (*(pdata + 4) - 0x30) * 1000;
                temp += (*(pdata + 5) - 0x30) * 100;
                temp += (*(pdata + 6) - 0x30) * 10;
                temp += (*(pdata + 7) - 0x30);
                if(temp < 10000)
                {
                    h264_file_table[temp >> 4] |= 1 << (temp & 0xF);

                    g_file_num++;
                    g_file_num_a++;

                    if(temp > g_h264_index)
                    {
                        g_h264_index = temp;
                    }

                    temp_time = (file_info.f_date << 16) | file_info.f_time;
                    if((!g_h264_file_time) || (temp_time > g_h264_file_time))
                    {
                        g_h264_file_time = temp_time;
                        g_latest_h264_file_index = temp;
                    }

                    if(((!g_h264_file_oldest_time) || (temp_time < g_h264_file_oldest_time)) && ((file_info.f_attrib & _A_RDONLY) == 0))
                    {
                        g_h264_file_oldest_time = temp_time;
                        g_oldest_h264_file_index = temp;
                    }

                    if(temp < temp_oldest_h264_index)
                    {
                        temp_oldest_h264_index = temp;
                    }
                }
            }

			nRet = _findnext(&file_info);
			if(nRet < 0)
			{
				break;
			}
		}
	}

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM\\DCIMB");
	#else
	chdir("C:\\DCIM\\DCIMB");
	#endif

	nRet = _findfirst("*.mov", &file_info, D_ALL);
	if(nRet >= 0)
	{
		while(1)
		{
			pdata = (CHAR *) file_info.f_name;
#if !C_RENAME_LOCK_FILE
            if((gp_strncmp((INT8S *) pdata, (INT8S *) "MOV", 3) == 0) && ((file_info.f_attrib & _A_RDONLY) == 0))
#else
            if(gp_strncmp((INT8S *) pdata, (INT8S *) "MOV", 3) == 0)
#endif
            {
                temp = (*(pdata + 4) - 0x30) * 1000;
                temp += (*(pdata + 5) - 0x30) * 100;
                temp += (*(pdata + 6) - 0x30) * 10;
                temp += (*(pdata + 7) - 0x30);
                if(temp < 10000)
                {
                    h264_file_table_b[temp >> 4] |= 1 << (temp & 0xF);

                    g_file_num++;
                    g_file_num_b++;

                    if(temp > g_h264_index)
                    {
                        g_h264_index = temp;
                    }

                    temp_time = (file_info.f_date << 16) | file_info.f_time;
                    if((!g_h264_file_time) || (temp_time > g_h264_file_time))
                    {
                        g_h264_file_time = temp_time;
                        g_latest_h264_file_index = temp;
                    }

                    if(((!g_h264_file_oldest_time) || (temp_time < g_h264_file_oldest_time)) && ((file_info.f_attrib & _A_RDONLY) == 0))
                    {
                        g_h264_file_oldest_time = temp_time;
                        g_oldest_h264_file_index = temp;
                    }

                    if(temp < temp_oldest_h264_index)
                    {
                        temp_oldest_h264_index = temp;
                    }
                }
            }

			nRet = _findnext(&file_info);
			if(nRet < 0)
			{
				break;
			}
		}
	}

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	#else
	chdir("C:\\DCIM");
	#endif

	g_h264_index++;
	if(g_h264_index > 9999)
	{
		g_h264_index_9999_exist = 1;
	}

	return g_h264_index;
}


INT32S get_file_final_lock_index(void)
{
	CHAR			*pdata;
	INT32S			nRet, temp;
	INT32U			temp_time;
	struct f_info	file_info;
    INT64U temp_lock_file_size = 0;

	g_lock_index = -1;
	g_lock_index_9999_exist = g_lock_file_time = g_lock_file_oldest_time = 0;
	temp_oldest_lock_index = 10000;

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM\\DCIMA");
	#else
	chdir("C:\\DCIM\\DCIMA");
	#endif

	nRet = _findfirst("*.mov", &file_info, D_ALL);
	if(nRet >= 0)
	{
		while(1)
		{
			pdata = (CHAR *) file_info.f_name;

#if !C_RENAME_LOCK_FILE
			if((gp_strncmp((INT8S *) pdata, (INT8S *) "MOV", 3) == 0) && (file_info.f_attrib & _A_RDONLY))
#else
            if(gp_strncmp((INT8S *) pdata, (INT8S *) "LOC", 3) == 0)
#endif
            {
                temp = (*(pdata + 4) - 0x30) * 1000;
                temp += (*(pdata + 5) - 0x30) * 100;
                temp += (*(pdata + 6) - 0x30) * 10;
                temp += (*(pdata + 7) - 0x30);
                if(temp < 10000)
                {
                    lock_file_table[temp >> 4] |= 1 << (temp & 0xF);

                    g_file_num++;
                    g_file_num_a++;

                    temp_lock_file_size += file_info.f_size; //lock file size
                    DBG_PRINT("lock file index [%d]\r\n",temp);

                    if(temp > g_lock_index)
                    {
                        g_lock_index = temp;
                    }

                    temp_time = (file_info.f_date << 16) | file_info.f_time;
                    if((!g_lock_file_time) || (temp_time > g_lock_file_time))
                    {
                        g_lock_file_time = temp_time;
                        g_latest_lock_file_index = temp;
                    }

                    if(((!g_lock_file_oldest_time) || (temp_time < g_lock_file_oldest_time)))
                    {
                        g_lock_file_oldest_time = temp_time;
                        g_oldest_lock_file_index = temp;
                        DBG_PRINT("oldest lock file index [%d]\r\n",g_oldest_lock_file_index);
                    }

                    if(temp < temp_oldest_lock_index)
                    {
                        temp_oldest_lock_index = temp;
                    }
                }
            }

			nRet = _findnext(&file_info);
			if(nRet < 0)
			{
				break;
			}
		}
	}

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM\\DCIMB");
	#else
	chdir("C:\\DCIM\\DCIMB");
	#endif

	nRet = _findfirst("*.mov", &file_info, D_ALL);
	if(nRet >= 0)
	{
		while(1)
		{
			pdata = (CHAR *) file_info.f_name;

#if !C_RENAME_LOCK_FILE
			if((gp_strncmp((INT8S *) pdata, (INT8S *) "MOV", 3) == 0) && (file_info.f_attrib & _A_RDONLY))
#else
            if(gp_strncmp((INT8S *) pdata, (INT8S *) "LOC", 3) == 0)
#endif
            {
                temp = (*(pdata + 4) - 0x30) * 1000;
                temp += (*(pdata + 5) - 0x30) * 100;
                temp += (*(pdata + 6) - 0x30) * 10;
                temp += (*(pdata + 7) - 0x30);
                if(temp < 10000)
                {
                    lock_file_table_b[temp >> 4] |= 1 << (temp & 0xF);

                    g_file_num++;
                    g_file_num_b++;

                    temp_lock_file_size += file_info.f_size; //lock file size
                    DBG_PRINT("lock file index [%d]\r\n",temp);

                    if(temp > g_lock_index)
                    {
                        g_lock_index = temp;
                    }

                    temp_time = (file_info.f_date << 16) | file_info.f_time;
                    if((!g_lock_file_time) || (temp_time > g_lock_file_time))
                    {
                        g_lock_file_time = temp_time;
                        g_latest_lock_file_index = temp;
                    }

                    if(((!g_lock_file_oldest_time) || (temp_time < g_lock_file_oldest_time)))
                    {
                        g_lock_file_oldest_time = temp_time;
                        g_oldest_lock_file_index = temp;
                    }

                    if(temp < temp_oldest_lock_index)
                    {
                        temp_oldest_lock_index = temp;
                        DBG_PRINT("oldest lock file index [%d]\r\n",g_oldest_lock_file_index);
                    }
                }
            }

			nRet = _findnext(&file_info);
			if(nRet < 0)
			{
				break;
			}
		}
	}

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	#else
	chdir("C:\\DCIM");
	#endif

	g_lock_index++;
	if(g_lock_index > 9999)
	{
		g_lock_index_9999_exist = 1;
	}
	cur_lock_file_size = temp_lock_file_size/(1024*1024);

	/*
    int i;
    for(i = 0; i < 100; i++)
    {
        if(lock_file_table[i] != 0)
        {
            DBG_PRINT("lock_file_table[%d]=%d\r\n",i,lock_file_table[i]);
        }
    }

    for(i = 0; i < 100; i++)
    {
        if(lock_file_table_b[i] != 0)
        {
            DBG_PRINT("lock_file_table_b[%d]=%d\r\n",i,lock_file_table_b[i]);
        }
    }
    */

	DBG_PRINT("cur_lock_file_size [%lld] \r\n", cur_lock_file_size);
	DBG_PRINT("max_lock_file_size [%lld] \r\n", max_lock_file_size);
	DBG_PRINT("oldest lock file index [%d]\r\n",g_oldest_lock_file_index);

	return g_lock_index;
}

INT32S get_file_final_jpeg_index(void)
{
	CHAR			*pdata;
	struct f_info	file_info;
	INT32S			nRet, temp;
	INT32U			temp_time;

	g_jpeg_index = -1;
	g_jpeg_index_9999_exist = g_jpeg_file_time = 0;

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM\\DCIMA");
	#else
	chdir("C:\\DCIM\\DCIMA");
	#endif

#if C_SENSOR_SAVE_RAW_DATA
	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	#else
	chdir("C:\\DCIM");
	#endif
	nRet = _findfirst("*.raw", &file_info, D_ALL);
#else
	nRet = _findfirst("*.jpg", &file_info, D_ALL);
#endif // C_SENSOR_SAVE_RAW_DATA
	if(nRet >= 0)
	{
		while(1)
		{
			pdata = (CHAR *) file_info.f_name;
			if(gp_strncmp((INT8S *) pdata, (INT8S *) "PIC", 3) == 0)
			{
				temp = (*(pdata + 4) - 0x30) * 1000;
				temp += (*(pdata + 5) - 0x30) * 100;
				temp += (*(pdata + 6) - 0x30) * 10;
				temp += (*(pdata + 7) - 0x30);
				if(temp < 10000)
				{
					jpg_file_table[temp >> 4] |= 1 << (temp & 0xF);

					g_file_num++;
					g_file_num_a++;

					if(temp > g_jpeg_index)
					{
						g_jpeg_index = temp;
					}

					temp_time = (file_info.f_date << 16) | file_info.f_time;
					if((!g_jpeg_file_time) || (temp_time > g_jpeg_file_time))
					{
						g_jpeg_file_time = temp_time;
						g_latest_jpeg_file_index = temp;
					}
				}
			}

			nRet = _findnext(&file_info);
			if(nRet < 0)
			{
				break;
			}
		}
	}

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM\\DCIMB");
	#else
	chdir("C:\\DCIM\\DCIMB");
	#endif

#if C_SENSOR_SAVE_RAW_DATA
	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	#else
	chdir("C:\\DCIM");
	#endif
	nRet = _findfirst("*.raw", &file_info, D_ALL);
#else
	nRet = _findfirst("*.jpg", &file_info, D_ALL);
#endif
	if(nRet >= 0)
	{
		while(1)
		{
			pdata = (CHAR *) file_info.f_name;
			if(gp_strncmp((INT8S *) pdata, (INT8S *) "PIC", 3) == 0)
			{
				temp = (*(pdata + 4) - 0x30) * 1000;
				temp += (*(pdata + 5) - 0x30) * 100;
				temp += (*(pdata + 6) - 0x30) * 10;
				temp += (*(pdata + 7) - 0x30);
				if(temp < 10000)
				{
					jpg_file_table_b[temp >> 4] |= 1 << (temp & 0xF);

					g_file_num++;
					g_file_num_b++;

					if(temp > g_jpeg_index)
					{
						g_jpeg_index = temp;
					}

					temp_time = (file_info.f_date << 16) | file_info.f_time;
					if((!g_jpeg_file_time) || (temp_time > g_jpeg_file_time))
					{
						g_jpeg_file_time = temp_time;
						g_latest_jpeg_file_index = temp;
					}
				}
			}

			nRet = _findnext(&file_info);
			if(nRet < 0)
			{
				break;
			}
		}
	}

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	#else
	chdir("C:\\DCIM");
	#endif

	g_jpeg_index++;
	if(g_jpeg_index > 9999)
	{
		g_jpeg_index_9999_exist = 1;
	}

	return g_jpeg_index;
}


INT32S get_file_final_wav_index(void)
{
	CHAR			*pdata;
	struct f_info	file_info;
	INT32S			nRet, temp;
	INT32U			temp_time;

	g_wav_index = -1;
	g_wav_index_9999_exist = g_wav_file_time = 0;

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM\\DCIMA");
	#else
	chdir("C:\\DCIM\\DCIMA");
	#endif

	nRet = _findfirst("*.wav", &file_info, D_ALL);
	if(nRet >= 0)
	{
		while(1)
		{
			pdata = (CHAR *) file_info.f_name;
			if(gp_strncmp((INT8S *) pdata, (INT8S *) "REC", 3) == 0)
			{
				temp = (*(pdata + 4) - 0x30) * 1000;
				temp += (*(pdata + 5) - 0x30) * 100;
				temp += (*(pdata + 6) - 0x30) * 10;
				temp += (*(pdata + 7) - 0x30);
				if(temp < 10000)
				{
					wav_file_table[temp >> 4] |= 1 << (temp & 0xF);

					g_file_num++;
					g_file_num_a++;

					if(temp > g_wav_index)
					{
						g_wav_index = temp;
					}

					temp_time = (file_info.f_date << 16) | file_info.f_time;
					if((!g_wav_file_time) || (temp_time > g_wav_file_time))
					{
						g_wav_file_time = temp_time;
						g_latest_wav_file_index = temp;
					}
				}
			}

			nRet = _findnext(&file_info);
			if(nRet < 0)
			{
				break;
			}
		}
	}

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	#else
	chdir("C:\\DCIM");
	#endif

	g_wav_index++;
	if(g_wav_index > 9999)
	{
		g_wav_index_9999_exist = 1;
	}

	return g_wav_index;
}

void get_file_index(void)
{
	INT32U	max_temp;

	if(device_plug_phase==0)
	{
		//DBG_PRINT("No SDC\r\n");
		return;
	}

	get_file_final_h264_index();
	get_file_final_lock_index();
	get_file_final_jpeg_index();
	#if 1//(defined C_AUDIO_RECORD_EN) && (C_AUDIO_RECORD_EN)
	get_file_final_wav_index();
	#endif

	g_file_index_9999_exist = (g_lock_index_9999_exist | g_h264_index_9999_exist | g_jpeg_index_9999_exist | g_wav_index_9999_exist);
	if(g_file_index_9999_exist)
	{
		g_h264_index = g_latest_h264_file_index + 1;
		if(g_h264_index > 9999)
		{
			g_h264_index = 0;
		}

		g_lock_index = g_latest_lock_file_index + 1;
		if(g_lock_index > 9999)
		{
			g_lock_index = 0;
		}

		g_jpeg_index = g_latest_jpeg_file_index + 1;
		if(g_jpeg_index > 9999)
		{
			g_jpeg_index = 0;
		}

		g_wav_index = g_latest_wav_file_index + 1;
		if(g_wav_index > 9999)
		{
			g_wav_index = 0;
		}

		max_temp = g_h264_file_time;
		g_file_index = g_h264_index;

		if(max_temp < g_lock_file_time)
		{
			max_temp = g_lock_file_time;
			g_file_index = g_lock_index;
		}

		if(max_temp < g_jpeg_file_time)
		{
			max_temp = g_jpeg_file_time;
			g_file_index = g_jpeg_index;
		}
	}
	else
	{
        g_file_index = max(max(max(g_h264_index, g_lock_index), g_jpeg_index), g_wav_index);
	}
}

void reinit_all_table(void)
{
	int i;

	g_play_index = -1;
	g_file_num = g_file_num_a = g_file_num_b = 0;
    for(i = 0; i < AP_STG_MAX_FILE_NUMS; i++)
    {
        lock_file_table[i] = jpg_file_table[i] = wav_file_table[i] = 0;
        lock_file_table_b[i] = jpg_file_table_b[i] = 0;
        h264_file_table[i] = h264_file_table_b[i] = 0;
    }

	//Scan again
	get_file_index();

    if(!g_file_index_9999_exist )
    {
        g_oldest_lock_file_index = temp_oldest_lock_index;
        if(g_oldest_lock_file_index > 9999)
            g_oldest_lock_file_index = 0;

        g_oldest_h264_file_index = temp_oldest_h264_index;
        if(g_oldest_h264_file_index > 9999)
            g_oldest_h264_file_index = 0;
    }
}

INT16S get_max_file_index(INT8U flag)
{
	INT16S	i;
	INT16S  file_index = -1;

    for(i = 0; i < 10000; i++)
    {
        if(flag == 0)
        {
            if((lock_file_table[i >> 4] & (1 << (i & 0xF))) || (jpg_file_table[i >> 4] & (1 << (i & 0xF))) || (h264_file_table[i >> 4] & (1 << (i & 0xF))) || (wav_file_table[i >> 4] & (1 << (i & 0xF))))
            {
                file_index = i;
            }
        }
        else
        {
            if((lock_file_table_b[i >> 4] & (1 << (i & 0xF))) || (jpg_file_table_b[i >> 4] & (1 << (i & 0xF))) || (h264_file_table_b[i >> 4] & (1 << (i & 0xF))))
            {
                file_index = i;
            }
        }
    }

	return file_index;
}

INT16U get_deleted_file_number(INT8U flag)
{
	INT16U	i;
	INT16U	deleted_file_number = 0;

	if(flag == 0)
	{		//DCIMA
		for(i = 0; i < g_play_index + 1; i++)
		{
			if(((lock_file_table[i >> 4] & (1 << (i & 0xF))) == 0) && ((jpg_file_table[i >> 4] & (1 << (i & 0xF))) == 0) &&((h264_file_table[i >> 4] & (1 << (i & 0xF)))==0) && ((wav_file_table[i >> 4] & (1 << (i & 0xF))) == 0))
			{
				deleted_file_number++;
			}
		}
	}
	else
	{		//DCIMB
		for(i = 0; i < g_play_index + 1; i++)
		{
			if(((lock_file_table_b[i >> 4] & (1 << (i & 0xF))) == 0) && ((jpg_file_table_b[i >> 4] & (1 << (i & 0xF)))==0)&&((h264_file_table_b[i >> 4] & (1 << (i & 0xF))) == 0))
			{
				deleted_file_number++;
			}
		}
	}

	return deleted_file_number;
}

void ap_storage_service_format_req(void)
{
    INT64U size;
	INT16S	ret;
    struct _diskfree_t st_free;

    size = drvl2_sd_card_voloume_get(STORAGE_DEVICE);
    if(size <= (32<<10)){
    	DBG_PRINT("SDC FAT32 formating.... \r\n");
        ret = _format(STORAGE_DEVICE, FAT32_Type);
    } else {
    	DBG_PRINT("SDC exFAT formating.... \r\n");
        ret = _format(STORAGE_DEVICE, EXFAT_Type);
    }

	if(!ret)
	{
		DBG_PRINT("SDC format OK !\r\n");
		//_deviceunmount(STORAGE_DEVICE);
		//SDC_ERR = _devicemount(STORAGE_DEVICE);
		SDC_ERR = 0;
		if(!SDC_ERR)
		{
            _getdiskfree(STORAGE_DEVICE, &st_free);
            DBG_PRINT("SDC info, st_free.total_clusters [%d]\r\n", st_free.total_clusters);
            DBG_PRINT("SDC info, st_free.avail_clusters [%d]\r\n", st_free.avail_clusters);
            DBG_PRINT("SDC info, st_free.sectors_per_cluster [%d]\r\n", st_free.sectors_per_cluster);
            DBG_PRINT("SDC info, st_free.bytes_per_sector [%d]\r\n", st_free.bytes_per_sector);
            max_lock_file_size = (INT64U)st_free.total_clusters*st_free.sectors_per_cluster*st_free.bytes_per_sector/(1024*1024);
			drvl2_sd_card_voloume_set(STORAGE_DEVICE, max_lock_file_size);
			DBG_PRINT("SDC total size [%lld]MB\r\n", max_lock_file_size);
            max_lock_file_size = max_lock_file_size*LOCK_FILE_PERCENT/100;
            DBG_PRINT("max_lock_file_size [%lld]MB\r\n", max_lock_file_size);
            /*==============================================*/
			size = vfsFreeSpace(STORAGE_DEVICE) >> 20;
			DBG_PRINT("Mount OK, free size [%lld]\r\n", size);
        }
	}
	else
	{
		DBG_PRINT("SDC format fail !\r\n");
		ap_state_handling_show_sd_err();
	}

	#if STORAGE_DEVICE == FS_SD0
	mkdir("G:\\DCIM");
	mkdir("G:\\DCIM\\DCIMA");
	mkdir("G:\\DCIM\\DCIMB");
	chdir("G:\\DCIM");
	#else
	mkdir("C:\\DCIM");
	mkdir("C:\\DCIM\\DCIMA");
	mkdir("C:\\DCIM\\DCIMB");
	chdir("C:\\DCIM");
	#endif

	UsrFlushBuffers(STORAGE_DEVICE);

    reinit_all_table();
	app_msg_send(MSG_STORAGE_SERVICE_FORMAT_REPLY, NULL, 0, MSG_PRI_NORMAL);
}

void ap_storage_service_write_file(INT16S fd, INT32U buf, INT32U size)
{
	INT32U msg;

	xSemaphoreTake(storage_fs_sem, portMAX_DELAY);

	storage_fs_fd	= fd;
	storage_fs_buf	= buf;
	storage_fs_size	= size;

    xQueueReset(storage_fs_ack);
#if defined(C_STORAGE_DEVICE) && (C_STORAGE_DEVICE)
	msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_WRITE_FILE, NULL, 0, MSG_PRI_NORMAL);
#endif
	xQueueReceive(storage_fs_ack, (void*)&msg, portMAX_DELAY);

	xSemaphoreGive(storage_fs_sem);
}

void ap_storage_service_write_file_process(void)
{
	INT32S msg;
	INT32S ret;

	ret = write(storage_fs_fd, storage_fs_buf, storage_fs_size);
	if(ret < 0)
	{
		DBG_PRINT("[%s] Fail to write\r\n",__FUNCTION__);
	}

	msg = C_ACK_SUCCESS;
	xQueueSend(storage_fs_ack, (void*)&msg, 0);
}

void ap_storage_service_close_file(INT16S fd)
{
	INT32U msg;

	xSemaphoreTake(storage_fs_sem, portMAX_DELAY);

    xQueueReset(storage_fs_ack);
	storage_fs_fd = fd;
#if defined(C_STORAGE_DEVICE) && (C_STORAGE_DEVICE)
	msgQSend(StorageServiceQ, MSG_STORAGE_SERVICE_CLOSE_FILE, NULL, 0, MSG_PRI_NORMAL);
#endif
	xQueueReceive(storage_fs_ack, (void*)&msg, portMAX_DELAY);

	xSemaphoreGive(storage_fs_sem);
}

void ap_storage_service_close_file_process(void)
{
	INT32S msg;

	close(storage_fs_fd);

	msg = C_ACK_SUCCESS;
	xQueueSend(storage_fs_ack, (void*)&msg, 0);
}

INT16U ap_storage_service_playfile_totalcount(INT16U *playfile_index_table)
{
    INT16U playfile_total_count,play_file_index;
    INT16S index_tmp, i,k, l;
    INT8U flag;
    INT16U* ph264_file_table;
    INT16U* pjpeg_file_table;
    INT16U* plock_file_table;
    playfile_total_count = 0;
    reinit_all_table();


	if(device_plug_phase==0)
	{
		return playfile_total_count;
	}


    for(i=0; i<2; i++) // Find folder A + B
    {
        if(i==0)
        {
            ph264_file_table = h264_file_table;
            pjpeg_file_table = jpg_file_table;
            plock_file_table = lock_file_table;
        }
        else
        {
            ph264_file_table = h264_file_table_b;
            pjpeg_file_table = jpg_file_table_b;
            plock_file_table = lock_file_table_b;
        }
        play_file_index = 0xffff;
        while(1)
        {
            flag = 0;

            index_tmp = play_file_index+1;
            if (index_tmp > 9999) {
                break;
            }

            k = index_tmp >> 4;
            l = index_tmp & 0xF;
            while (1) {
                if (ph264_file_table[k] || pjpeg_file_table[k] || plock_file_table[k]) {
                    for ( ; l<0x10 ; l++) {
                        if (ph264_file_table[k] & (1<<l)) {
                            play_file_index = (k << 4) + l;
                            playfile_total_count++;
                            flag=1;
                            break;
                        }
                        if (pjpeg_file_table[k] & (1<<l)) {
                            play_file_index = (k << 4) + l;
                            playfile_total_count++;
                            flag=1;
                            break;
                        }
                        if (plock_file_table[k] & (1<<l)) {
                            play_file_index = (k << 4) + l;
                            playfile_total_count++;
                            flag=1;
                            break;
                        }
                    }

                    if ( (playfile_total_count>0)&&(playfile_total_count&0x1FF)==0x0)	// Each 512 files creates a record.
                    {
                        playfile_index_table[playfile_total_count>>9] = play_file_index;        // Each 512 files creates a record.
                    }

                    if(flag) {
                        break;
                    }
                }

                l = 0x0;
                k++;
                if (k > 624) {
                    break;
                }
            }

            if (k > 624) {
                break;
            }
        }
    }
    return playfile_total_count;
}

INT16S ap_storage_service_playfilelist_req(INT8U *buff_addr, INT16U play_file_index, INT32U dcim_a_b)
{
	INT16S index_tmp, i = 0, k, l;
	struct stat_t buf_tmp;
	INT8U *p_buf_addr;
	INT8U *p_file_info;
	INT8U  play_file_index_cnt, play_file_err_cnt,flag;
	STOR_SERV_PLAYINFO info_ptr;
	TIME_T time_info;
	INT16U palyload_size;
	INT16U* ph264_file_table;
	INT16U* pjpeg_file_table;
	INT16U* plock_file_table;


	p_buf_addr = buff_addr;
	p_file_info = buff_addr+3;
	play_file_index_cnt = 0;
	play_file_err_cnt = 0;

	ap_storage_service_timer_stop();

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	#else
	chdir("C:\\DCIM");
	#endif

	if (dcim_a_b == 0)
	{
		ph264_file_table = h264_file_table;
		pjpeg_file_table = jpg_file_table;
		plock_file_table = lock_file_table;
	}
	else
	{
		ph264_file_table = h264_file_table_b;
		pjpeg_file_table = jpg_file_table_b;
		plock_file_table = lock_file_table_b;
	}

    for(play_file_index_cnt = 0;play_file_index_cnt<16;play_file_index_cnt++)
	{
		flag = 0;
		index_tmp = play_file_index+1;
		if (index_tmp > 9999) {
				index_tmp = 0;
			}
		k = index_tmp >> 4;
		l=index_tmp & 0xF;
		while (i <= 626) {
			i++;
			if (ph264_file_table[k] || plock_file_table[k] || pjpeg_file_table[k]) {
				for ( ; l<0x10 ; l++) {
					if (ph264_file_table[k] & (1<<l)) {
						play_file_index = (k << 4) + l;
						info_ptr.file_type = TK_IMAGE_TYPE_MOTION_JPEG;
						if (dcim_a_b == 0)
						{
							sprintf((char *)g_file_path, (const char *)"DCIMA\\MOVA%04d.mov", play_file_index);
						}
						else
						{
							sprintf((char *)g_file_path, (const char *)"DCIMB\\MOVB%04d.mov", play_file_index);
						}
						flag = 1;
						break;
					}
					if (plock_file_table[k] & (1<<l)) {
						play_file_index = (k << 4) + l;
						info_ptr.file_type = TK_IMAGE_TYPE_MOTION_JPEG;
						if (dcim_a_b == 0)
						{
							sprintf((char *)g_file_path, (const char *)"DCIMA\\LOCA%04d.mov", play_file_index);
						}
						else
						{
							sprintf((char *)g_file_path, (const char *)"DCIMB\\LOCB%04d.mov", play_file_index);
						}
						flag = 2;
						break;
					}
					if (pjpeg_file_table[k] & (1<<l)) {
						play_file_index = (k << 4) + l;
						info_ptr.file_type = TK_IMAGE_TYPE_JPEG;
						if (dcim_a_b == 0)
						{
							sprintf((char *)g_file_path, (const char *)"DCIMA\\PICA%04d.jpg", play_file_index);
						}
						else
						{
							sprintf((char *)g_file_path, (const char *)"DCIMB\\PICB%04d.jpg", play_file_index);
						}
						flag = 1;
						break;
					}
				}
				if(flag) {
					break;
				}
			}
			l = 0x0;
			k++;
			if (k > 624) {
				break;
			}
		}
		if(flag)
		{
			info_ptr.file_path_addr = (INT32U) g_file_path;
			info_ptr.file_handle = open(g_file_path, O_RDONLY);
			if (info_ptr.file_handle >= 0) {
				fstat(info_ptr.file_handle, &buf_tmp);
				close(info_ptr.file_handle);
				info_ptr.file_size = buf_tmp.st_size/1024;
				ap_browse_decode_file_date_time_1(buf_tmp.st_mtime,buf_tmp.st_mstime, &time_info);
				if(info_ptr.file_type == TK_IMAGE_TYPE_MOTION_JPEG)
				{
					if(flag == 1)
					*(p_file_info++) = 'A';
					else
					*(p_file_info++) = 'L';
				}
				else if(info_ptr.file_type == TK_IMAGE_TYPE_JPEG)
				{
					*(p_file_info++) = 'J';
				}

				if (dcim_a_b == 0)
				{
					*(p_file_info++) = play_file_index &0xFF;
					*(p_file_info++) = (play_file_index >>8)&0xFF;
				}
				else
				{
					*(p_file_info++) = ((play_file_index+10000) &0xFF);
					*(p_file_info++) = ((play_file_index+10000) >>8)&0xFF;
				}
				*(p_file_info++) = time_info.tm_year - 2000;
				*(p_file_info++) = time_info.tm_mon;
				*(p_file_info++) = time_info.tm_mday;
				*(p_file_info++) = time_info.tm_hour;
				*(p_file_info++) = time_info.tm_min;
				*(p_file_info++) = time_info.tm_sec;
				*(p_file_info++) = info_ptr.file_size & 0xFF;
				*(p_file_info++) = (info_ptr.file_size>>8) & 0xFF;
				*(p_file_info++) = (info_ptr.file_size>>16) & 0xFF;
				*(p_file_info++) = (info_ptr.file_size>>24) & 0xFF;
			}
			else {
				play_file_err_cnt++;
			}
		}
		else
		{
			break;
		}
	}
	if (play_file_err_cnt == 0)	// NO error
	{
		palyload_size = 1+13*play_file_index_cnt;
		*(p_buf_addr++) = palyload_size &0xFF;
		*(p_buf_addr++) = (palyload_size>>8) &0xFF;
		*p_buf_addr     = play_file_index_cnt;
	}
	else
	{	// maybe remove SD card
		palyload_size = 0xFFFF;
	}

	ap_storage_service_timer_start();
	return palyload_size;
}

void * ap_storage_service_thumbnail_req(INT16U file_index, INT32U *buf_size, INT16U out_w, INT16U out_h)
{
	INT16U t_playfile_index;
	INT16S k, l;
	struct stat_t buf_tmp;
	STOR_SERV_PLAYINFO info_ptr;
	INT32S ret;
	INT32U display_addr = 0,jpg_addr;
	INT16U* ph264_file_table;
	INT16U* pjpeg_file_table;
	INT16U* plock_file_table;
	INT32U dcim_a_b = 0;

	ap_storage_service_timer_stop();

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	#else
	chdir("C:\\DCIM");
	#endif

	if (file_index <= 9999)
	{
		ph264_file_table = h264_file_table;
		pjpeg_file_table = jpg_file_table;
		plock_file_table = lock_file_table;
		dcim_a_b = 0;
	}
	else
	{
		ph264_file_table = h264_file_table_b;
		pjpeg_file_table = jpg_file_table_b;
		plock_file_table = lock_file_table_b;
		dcim_a_b = 1;
		file_index -= (9999+1);
	}
	t_playfile_index = file_index;
	k = t_playfile_index >> 4;
	l = t_playfile_index & 0xF;
	if (ph264_file_table[k] & (1<<l)) {
        info_ptr.file_type = TK_IMAGE_TYPE_H264;
		if(dcim_a_b == 0)
		{
			sprintf((char *)g_file_path, (const char *)"DCIMA\\MOVA%04d.mov", t_playfile_index);
		}
		else
		{
			sprintf((char *)g_file_path, (const char *)"DCIMB\\MOVB%04d.mov", t_playfile_index);
		}
	}
    else if (plock_file_table[k] & (1<<l)) {
        info_ptr.file_type = TK_IMAGE_TYPE_H264;
		if(dcim_a_b == 0)
		{
			sprintf((char *)g_file_path, (const char *)"DCIMA\\LOCA%04d.mov", t_playfile_index);
		}
		else
		{
			sprintf((char *)g_file_path, (const char *)"DCIMB\\LOCB%04d.mov", t_playfile_index);
		}
	}
	else if (pjpeg_file_table[k] & (1<<l)) {
		info_ptr.file_type = TK_IMAGE_TYPE_JPEG;
		if(dcim_a_b == 0)
		{
			sprintf((char *)g_file_path, (const char *)"DCIMA\\PICA%04d.jpg", t_playfile_index);
		}
		else
		{
			sprintf((char *)g_file_path, (const char *)"DCIMB\\PICB%04d.jpg", t_playfile_index);
		}
	}
	info_ptr.file_path_addr = (INT32U) g_file_path;
	info_ptr.file_handle = open(g_file_path, O_RDONLY);

	#if 0
	DBG_PRINT("thumbnail open(%s)=%d\r\n", g_file_path, info_ptr.file_handle);
	#endif

	if(info_ptr.file_handle > -1)
	{
		fstat(info_ptr.file_handle, &buf_tmp);
		info_ptr.file_size = buf_tmp.st_size;
		jpg_addr = (INT32U) gp_malloc_align(MAX_THUMBNAIL_SIZE, 64);
		display_addr = (INT32U) gp_malloc_align(((out_w*out_h*3)>>1), 64); //scaler output buffer (GP420)

		*buf_size = 0;

		ret = ap_browse_reply_get_first_frame(&info_ptr,1,display_addr,out_w,out_h);
		if(ret == STATUS_OK)
		{
			JpegEnc_t JpegEnc = {0};

			JpegEnc.hwidx = JPEGA;
			JpegEnc.quality_value = 50;
			JpegEnc.yuv_fmt = VID_DATA_420;
			JpegEnc.yuv_sample_fmt = JPEG_YUV420;
			JpegEnc.width = out_w;
			JpegEnc.height = out_h;
			JpegEnc.input_buffer_y = display_addr;
			JpegEnc.output_buffer = jpg_addr+64; // 64: reserve for rtsp packet header
			JpegEnc.output_buffer_len = MAX_THUMBNAIL_SIZE-64;

			cache_invalid_range(jpg_addr, MAX_THUMBNAIL_SIZE);
            cache_invalid_range(display_addr, (out_w*out_h*3)>>1);
			ret = Jpeg_Encode_For_Thumbnail(&JpegEnc);
			if(ret == STATUS_OK)
			{
				*buf_size = JpegEnc.encode_size;
			}
		}

		close(info_ptr.file_handle);

		ap_storage_service_timer_start();
		if(ret == STATUS_OK)
		{
			if (display_addr)
				gp_free((void *) display_addr);
			if(ret == 0)
			{
				return ((void *) jpg_addr);
			}
			else
			{
				if(jpg_addr)
					gp_free((void *) jpg_addr);
				return (NULL);
			}
		}
		else
		{
			if (display_addr)
				gp_free((void *) display_addr);
			if(jpg_addr)
				gp_free((void *) jpg_addr);
			return (NULL);
		}
	}
	else
	{
		//fail
		ap_storage_service_timer_start();
		return (NULL);
	}
}

static INT32U g_sd_read_size = (10*1024);
INT32S ap_storage_service_download_file_block_size(INT32U size)
{
	g_sd_read_size = size;
	return 0;
}

INT32S ap_storage_service_download_file_req(INT16U file_index,STOR_SERV_DOWNLOAD_FILEINFO *dw_file_info,INT8U *buf_addr,INT32U *buf_size)
{
	INT16U t_playfile_index;
	INT16S k, l;
	struct stat_t buf_tmp;
	STOR_SERV_PLAYINFO info_ptr;
	INT16U* ph264_file_table;
	INT16U* pjpeg_file_table;
	INT16U* plock_file_table;
	INT32U dcim_a_b = 0;


	ap_storage_service_timer_stop();

	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	#else
	chdir("C:\\DCIM");
	#endif

	t_playfile_index = file_index;
	if(((t_playfile_index==dw_file_info->play_index)||(t_playfile_index == (dw_file_info->play_index+10000)))&&(dw_file_info->file_size != 0))
	{

		//dw_file_info->file_handle = open((CHAR *)dw_file_info->file_path_addr, O_RDONLY);
		if(dw_file_info->file_handle >= 0)
		{
			//lseek(dw_file_info->file_handle, dw_file_info->file_offset, SEEK_SET);
			*buf_size = read(dw_file_info->file_handle, (INT32U) buf_addr, g_sd_read_size);
			dw_file_info->file_offset += *buf_size;
			//close(dw_file_info->file_handle);
			ap_storage_service_timer_start();
			return 0;
		}
		else
		{
			ap_storage_service_timer_start();
			DBG_PRINT("file handle error\r\n");
			return -1;
		}
	}
	else
	{
        if (file_index <= 9999)
        {
            ph264_file_table = h264_file_table;
            pjpeg_file_table = jpg_file_table;
            plock_file_table = lock_file_table;
            dcim_a_b = 0;
        }
        else
        {
            ph264_file_table = h264_file_table_b;
            pjpeg_file_table = jpg_file_table_b;
            plock_file_table = lock_file_table_b;
            dcim_a_b = 1;
            file_index -= (9999+1);
        }

        t_playfile_index = file_index;
		k = t_playfile_index >> 4;
		l = t_playfile_index & 0xF;
		if (ph264_file_table[k] & (1<<l)) {
			info_ptr.file_type = TK_IMAGE_TYPE_H264;
			if(dcim_a_b == 0)
			sprintf((char *)g_file_path, (const char *)"DCIMA\\MOVA%04d.mov", t_playfile_index);
			else
			sprintf((char *)g_file_path, (const char *)"DCIMB\\MOVB%04d.mov", t_playfile_index);
		}else if (plock_file_table[k] & (1<<l)) {
			info_ptr.file_type = TK_IMAGE_TYPE_H264;
			if(dcim_a_b == 0)
			sprintf((char *)g_file_path, (const char *)"DCIMA\\LOCA%04d.mov", t_playfile_index);
			else
			sprintf((char *)g_file_path, (const char *)"DCIMB\\LOCB%04d.mov", t_playfile_index);
		}
		 else if (pjpeg_file_table[k] & (1<<l)) {
			info_ptr.file_type = TK_IMAGE_TYPE_JPEG;
			if(dcim_a_b == 0)
			sprintf((char *)g_file_path, (const char *)"DCIMA\\PICA%04d.jpg", t_playfile_index);
			else
			sprintf((char *)g_file_path, (const char *)"DCIMB\\PICB%04d.jpg", t_playfile_index);
		}

		info_ptr.file_path_addr = (INT32U) g_file_path;
		DBG_PRINT("g_file_path %s...\r\n",g_file_path);
		info_ptr.file_handle = open(g_file_path, O_RDONLY);
		dw_file_info->play_index = t_playfile_index;
		dw_file_info->file_path_addr = (INT32U) g_file_path;
		dw_file_info->file_handle = info_ptr.file_handle;
		dw_file_info->file_offset = 0;
		if(info_ptr.file_handle >= 0)
		{
			fstat(info_ptr.file_handle, &buf_tmp);
			info_ptr.file_size = buf_tmp.st_size;
			dw_file_info->file_size = buf_tmp.st_size;
			*buf_size = read(info_ptr.file_handle, (INT32U) buf_addr, g_sd_read_size);
			dw_file_info->file_offset += *buf_size;
			//close(info_ptr.file_handle);
			ap_storage_service_timer_start();
			return 0;
		}
		else
		{
			ap_storage_service_timer_start();
			return -1;
		}
	}
}

INT16S ap_storage_service_indexfile_req(INT16U playfile_order, INT16U *playfile_index_table)
{
    //INT32S i;
    INT32S order_level;
    INT16S k, l;
    INT16U play_file_index;
    INT16U playfile_count = 0;

	if (playfile_order==0)
	{
		play_file_index = 0xFFFF;	// error
		goto AP_STORAGE_SERVICE_INDEXFILE_REQ_END;
	}

	if (playfile_order>9999)
	{
		playfile_order = 0;
	}
	order_level = (playfile_order>>9);   // Each 512 files creates a record.
	playfile_order = playfile_order - (order_level<<9);		// Each 512 files creates a record.
	play_file_index = playfile_index_table[order_level];
	if ( (playfile_order==0)&&(order_level!=0) )
	{
		goto AP_STORAGE_SERVICE_INDEXFILE_REQ_END;
	}
	play_file_index++;
	if (play_file_index>9999)
	{
		play_file_index = 0;
	}

    k = play_file_index >> 4;
    l = play_file_index & 0xF;
    while (1) {
        if (h264_file_table[k] || jpg_file_table[k]) {
            for ( ; l<0x10 ; l++) {
                if (h264_file_table[k] & (1<<l)) {
                    playfile_count++;
                    if (playfile_count >= playfile_order)
                    {
                        play_file_index = (k << 4) + l;
                        goto AP_STORAGE_SERVICE_INDEXFILE_REQ_END;
                    }
                }
                if (jpg_file_table[k] & (1<<l)) {
                    playfile_count++;
                    if (playfile_count >= playfile_order)
                    {
                        play_file_index = (k << 4) + l;
                        goto AP_STORAGE_SERVICE_INDEXFILE_REQ_END;
                    }
                }
            }
        }

        l = 0x0;
        k++;
        if (k > 624) {
        play_file_index = 0xFFFF;	// error
        break;
        }
    }

AP_STORAGE_SERVICE_INDEXFILE_REQ_END:
    return (INT16S)play_file_index;
}
INT16S ap_storage_service_index_delfile(INT16U file_index)
{
	INT16S ret = -1;
	INT16S k, l;
	INT16U t_playfile_index;
	STOR_SERV_PLAYINFO info_ptr;
	INT16U* ph264_file_table;
	INT16U* pjpeg_file_table;
	INT16U* plock_file_table;
	INT32U dcim_a_b = 0;

	struct stat_t		buf_tmp;

	ap_storage_service_timer_stop();
	#if STORAGE_DEVICE == FS_SD0
	chdir("G:\\DCIM");
	#else
	chdir("C:\\DCIM");
	#endif

	if (file_index <= 9999)
	{
		ph264_file_table = h264_file_table;
		pjpeg_file_table = jpg_file_table;
		plock_file_table = lock_file_table;
		dcim_a_b = 0;
	}
	else
	{
		ph264_file_table = h264_file_table_b;
		pjpeg_file_table = jpg_file_table_b;
		plock_file_table = lock_file_table_b;
		dcim_a_b = 1;
		file_index -= (9999+1);
	}
	t_playfile_index = file_index;
	k = t_playfile_index >> 4;
	l = t_playfile_index & 0xF;
	if (ph264_file_table[k] & (1<<l)) {
        info_ptr.file_type = TK_IMAGE_TYPE_H264;
		if(dcim_a_b == 0)
		{
			sprintf((char *)g_file_path, (const char *)"DCIMA\\MOVA%04d.mov", t_playfile_index);
		}
		else
		{
			sprintf((char *)g_file_path, (const char *)"DCIMB\\MOVB%04d.mov", t_playfile_index);
		}
	}
    else if (plock_file_table[k] & (1<<l)) {
        info_ptr.file_type = TK_IMAGE_TYPE_H264;
		if(dcim_a_b == 0)
		{
			sprintf((char *)g_file_path, (const char *)"DCIMA\\LOCA%04d.mov", t_playfile_index);
		}
		else
		{
			sprintf((char *)g_file_path, (const char *)"DCIMB\\LOCB%04d.mov", t_playfile_index);
		}
		info_ptr.file_path_addr = (INT32U) g_file_path;
		info_ptr.file_handle = open(g_file_path, O_RDONLY);
		_setfattr(g_file_path, D_NORMAL);
		close(info_ptr.file_handle);
	}
	else if (pjpeg_file_table[k] & (1<<l)) {
		info_ptr.file_type = TK_IMAGE_TYPE_JPEG;
		if(dcim_a_b == 0)
		{
			sprintf((char *)g_file_path, (const char *)"DCIMA\\PICA%04d.jpg", t_playfile_index);
		}
		else
		{
			sprintf((char *)g_file_path, (const char *)"DCIMB\\PICB%04d.jpg", t_playfile_index);
		}
	}

	if(unlink((CHAR *) g_file_path) < 0){
		ret = -1;
		DBG_PRINT("unlink fail\r\n");
	}else{
		ret =  STATUS_OK;
	}
	UsrFlushBuffers(STORAGE_DEVICE);

//	ret = ap_storage_service_file_del();

AP_STORAGE_SERVICE_INDEX_DEL_END:
//	osDelay(10);
	ap_storage_service_timer_start();

	DBG_PRINT("delete %s... ",g_file_path);
	return ret;
}



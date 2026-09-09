#ifndef __AP_STORAGE_SERVICE_H__
#define __AP_STORAGE_SERVICE_H__

extern INT8U curr_storage_id;

extern void		ap_storage_service_init(void);
extern INT32S	ap_storage_service_storage_mount(void);
extern void		ap_storage_service_file_open_handle(INT32U type, INT16U usr_config);
extern void		ap_storage_service_timer_start(void);
extern void		ap_storage_service_timer_stop(void);
extern void		ap_storage_service_play_req(INT32U req_msg);
extern void		ap_storage_service_file_del(void);
extern void		ap_storage_service_file_delete_all(void);
extern void		ap_storage_service_file_lock_one(void);
extern void		ap_storage_service_file_lock_all(void);
extern void		ap_storage_service_file_unlock_one(void);
extern void		ap_storage_service_file_unlock_all(void);
extern void		ap_storage_service_format_req(void);

#if C_CYCLIC_VIDEO_RECORD
extern void		ap_storage_service_cyclic_record_file_open_handle(INT32U req_type);
#endif

#if C_AUTO_DEL_FILE
extern void		ap_storage_service_freesize_check_switch(INT8U type);
extern INT32S	ap_storage_service_freesize_check_and_del(void);
#endif

extern void		ap_storage_service_write_file(INT16S fd, INT32U buf, INT32U size);
extern void		ap_storage_service_write_file_process(void);
extern void		ap_storage_service_close_file(INT16S fd);
extern void		ap_storage_service_close_file_process(void);

extern INT16U	ap_storage_service_playfile_totalcount(INT16U *playfile_index_table);
extern INT16S	ap_storage_service_playfilelist_req(INT8U *buff_addr, INT16U play_file_index, INT32U dcim_a_b);
extern void*	ap_storage_service_thumbnail_req(INT16U file_index,INT32U *buf_size, INT16U out_w, INT16U out_h);

extern INT32S	ap_storage_service_download_file_block_size(INT32U size);
extern INT32S	ap_storage_service_download_file_req(INT16U file_index,STOR_SERV_DOWNLOAD_FILEINFO *dw_file_info,INT8U *buf_addr,INT32U *buf_size);
extern INT16S	ap_storage_service_indexfile_req(INT16U playfile_order, INT16U *playfile_index_table);

#endif // __AP_STORAGE_SERVICE_H__

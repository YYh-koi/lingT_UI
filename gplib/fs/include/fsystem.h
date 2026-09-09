/*
 * file system all head File fsystem.h
 * Copyright (C) 2007 GeneralPlus
 *
 */
#ifndef __FSYSTEM_H__
#define __FSYSTEM_H__

//#pragma arm section code = "lib_fs", rodata = "lib_fs"

#define FS_PRINT_EN		0
#if FS_PRINT_EN
#define FS_PRINT(...) do{DBG_PRINT(__VA_ARGS__);}while(0)
#else
#define FS_PRINT(...) do{}while(0)
#endif

#define NOFATCHAIN		0	//0:Exfat can support fragment cluster, 1:continuous cluster

#include "gplib.h"
#include "fs.h"
#include "portab.h"

#if MALLOC_USE == 1
#include "ucBS.h"
#endif
#include <string.h>

//#include	"vfs.h"
#include "error.h"
#include "buffer.h"
#include "fs_driver.h"
#include "device.h"
#include "date.h"
#include "cds.h"
#include "dcb.h"
#include "fat.h"
#include "file.h"
#include "fat32.h"
#include "fnode.h"
#include "dirmatch.h"
#include "speedup.h"
#include "format.h"

#include "errors32.h"
#include "filesys32.h"
#include "unicode32.h"

#include "proto.h"
#endif /* __FSYSTEM_H__ */
extern INT32U	fs_sd_ms_plug_out_flag_get(void);
extern void		fs_sd_ms_plug_out_flag_reset(void);
extern INT8U	fs_usbh_plug_out_flag_get(void);
extern void		fs_usbh_plug_out_flag_reset(void);

extern void		fs_sd_ms_plug_out_flag_en(void);
extern INT32U	fs_usbh_plug_out_flag_en(void);

extern void		fs_lfndel_step_start(void);
extern void		fs_lfndel_step_end(void);
extern INT32S	fs_lfndel_step(void);

extern INT32S	unlink_step_run(void);
extern void		unlink_step_reg(void);
extern void		unlink_step_end(void);
extern INT8S 	DirCountGet(void);
extern INT32U 	GetSignName(void);

extern INT32U fs_huntercam_checkname_for_speedup(char *name);
extern INT32S fs_huntercam_get_file_index(void);

/* end of file */

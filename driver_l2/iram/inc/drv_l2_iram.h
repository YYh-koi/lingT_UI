#ifndef __drv_l2_IRAM_H_
#define __drv_l2_IRAM_H_

typedef struct{
	INT8U	is_attached;
	INT32U	*saddr;
	INT32U	size;
	INT32U	size_kB;
	//void (*config)(void);
}IRAM_INFO_t;

typedef struct{
	INT32U 		offset;
	IRAM_INFO_t info[12];
}IRAM_MANAGE_t;

extern void drv_l2_iram_init(void);
extern INT32S drv_l2_iram_attach_one(I_RAM_SHARED_MODULES module, INT32U size_kB);

#endif

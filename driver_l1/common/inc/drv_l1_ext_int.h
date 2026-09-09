#ifndef __drv_l1_EXT_INT_H__
#define __drv_l1_EXT_INT_H__

typedef enum
{
	EXTA,
	EXTB,
	EXTC,
	EXTD,
	EXTE,
	EXTF,
	EXTG,
	EXTH,
	EXTI,
	EXTJ,
	EXTK,
	EXTL
}EXTAB_ENUM;

typedef enum
{
	FALLING,
	RISING
}EXTAB_EDGE_ENUM;

#define EXTC_INT 0x00000100
#define EXTB_INT 0x00000080
#define EXTA_INT 0x00000040
#define EXTC_POL 0x00000020
#define EXTB_POL 0x00000010
#define EXTA_POL 0x00000008
#define EXTC_IEN 0x00000004
#define EXTB_IEN 0x00000002
#define EXTA_IEN 0x00000001

#define EXTF_INT 0x01000000
#define EXTE_INT 0x00800000
#define EXTD_INT 0x00400000
#define EXTF_POL 0x00200000
#define EXTE_POL 0x00100000
#define EXTD_POL 0x00080000
#define EXTF_IEN 0x00040000
#define EXTE_IEN 0x00020000
#define EXTD_IEN 0x00010000

#define EXTI_INT 0x00000100
#define EXTH_INT 0x00000080
#define EXTG_INT 0x00000040
#define EXTI_POL 0x00000020
#define EXTH_POL 0x00000010
#define EXTG_POL 0x00000008
#define EXTI_IEN 0x00000004
#define EXTH_IEN 0x00000002
#define EXTG_IEN 0x00000001

#define EXTL_INT 0x01000000
#define EXTK_INT 0x00800000
#define EXTJ_INT 0x00400000
#define EXTL_POL 0x00200000
#define EXTK_POL 0x00100000
#define EXTJ_POL 0x00080000
#define EXTL_IEN 0x00040000
#define EXTK_IEN 0x00020000
#define EXTJ_IEN 0x00010000

extern void drv_l1_ext_int_init(INT8U ext_src, INT8U vic_num, void (*user_isr)(void));
extern void drv_l1_ext_int_uninit(INT8U ext_src, INT8U vic_num);
extern void drv_l1_ext_int_clr(INT8U ext_src);
extern void drv_l1_ext_edge_set(INT8U ext_src, INT8U edge_type);
extern void drv_l1_ext_enable_set(INT8U ext_src, BOOLEAN status);
extern void drv_l1_ext_user_isr_set(INT8U ext_src,void (*user_isr)(void));
extern void drv_l1_ext_user_isr_clr(INT8U ext_src);

#endif		// __drv_l1_EXT_INT_H__

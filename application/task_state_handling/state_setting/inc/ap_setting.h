#ifndef __AP_SETTING_H__
#define __AP_SETTING_H__

#define SUB_MENU_DRAW					0x1
#define SUB_MENU_MOVE_UP				0x2
#define SUB_MENU_MOVE_DOWN				0x3
#define FUNCTION_ACTIVE					0x4

typedef struct
{
	INT16U	index;
	INT16U  str_idx_start;
	INT16U	item_max;
} SETUP_ITEM_INFO;

typedef struct
{
	void    (*ap_setting_init_ptr)(INT32U prev_state);
	void    (*ap_setting_exit_ptr)(INT32U prev_state);
	EXIT_FLAG_ENUM  (*ap_setting_mode_key_active_ptr)(INT32U prev_state);
	EXIT_FLAG_ENUM  (*ap_setting_menu_key_active_ptr)(INT32U prev_state);
	void    (*ap_setting_func_key_active_ptr)(void);
	void    (*ap_setting_direction_key_active_ptr)(INT32U msg_id);
	void    (*ap_setting_format_reply_ptr)(void);
} Overlap_Setting_Function;

extern Overlap_Setting_Function Seting;
extern void overlap_setting_function_initial(void);

extern INT8U g_setting_reply_action_flag;

extern void ap_setting_init(INT32U prev_state);
extern void ap_setting_exit(INT32U prev_state);

extern EXIT_FLAG_ENUM ap_setting_mode_key_active(INT32U prev_state);
extern EXIT_FLAG_ENUM ap_setting_menu_key_active(INT32U prev_state);
extern void ap_setting_func_key_active(void);
extern void ap_setting_direction_key_active(INT32U msg_id);

extern void ap_setting_format_reply(void);
extern void ap_setting_other_reply(INT32U msg_id);

extern void ap_USB_setting_direction_key_active(INT32U msg_id);
extern INT32S ap_USB_setting_page_draw(void);
extern void ap_USB_setting_display_ok(void);


#endif // #define __AP_SETTING_H__
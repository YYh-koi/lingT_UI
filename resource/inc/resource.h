#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include "resource_general.h"

/*== Icon Info ==*/
typedef struct
{
    INT16U *data;
	INT16U	icon_w;
	INT16U	icon_h;
	INT32U  transparent;
	INT16S	pos_x;
	INT16S	pos_y;
} DISPLAY_ICONSHOW;

typedef struct
{
    INT16U *icon_data;
    INT16U str_idx;
    INT16U sub_str_start;
    INT16U sub_item_num;
    INT8U (*config_get)(void);
    void (*config_set)(INT8U val);
} SETTING_ITEM;

/*--------- draw string type -----------*/
#define REC_FONT_TYPE	            2           //1(big) or 2(small)

#define YUYV_DRAW		            0x01
#define YUV420_DRAW		            0x02
#define RGB565_DRAW                 0x03

#define DRAW_BLACK_EDGE_OFF         0
#define DRAW_BLACK_EDGE_ON          1

#define DRAW_UV_OFF                 0
#define DRAW_UV_ON                  1

/*--------- color define -----------*/
#define RED_COLOR					0xF800
#define GREEN_COLOR                 0x07E0
#define BLUE_COLOR					0x001F
#define YELLOW_COLOR                0xFFE0
#define WHITE_COLOR					0xFFFF
#define BLACK_COLOR                 0x0000
#define BLUE1_COLOR					0x87BE
#define PURPLE_COLOR                0x8010

/*  the following definitions for video record and photo capture */
#define POSITION_RTC_DATE_TIME_X        (40)
#define POSITION_RTC_DATE_TIME_Y        (-30)

/*  the following definitions for video record  */
#define POSITION_REC_TIME_X             (-130)
#define POSITION_REC_TIME_Y             (8)

#define POSITION_LEFT_REC_TIME_X        (-130)
#define POSITION_LEFT_REC_TIME_Y        (8)

#define RECORD_RESV_POS_X               (-100)
#define RECORD_RESV_POS_Y               36

#define RECORD_PARK_POS_X				(-160)
#define RECORD_PARK_POS_Y				6

/*  the following definitions for photo capture  */
#define CAPTURE_RESV_POS_X				(-100)
#define CAPTURE_RESV_POS_Y				36
#define CAPTURE_LEFT_NUM_POS_X			(-100)
#define CAPTURE_LEFT_NUM_POS_Y			6

/*  the following definitions for browse  */
#define POSITION_PLAY_TIME_X            (-130)
#define POSITION_PLAY_TIME_Y            (8)

#define POSITION_FILE_DATE_TIME_X       (40)
#define POSITION_FILE_DATE_TIME_Y       (-30)

#define BROWSE_FILE_NAME_POS_X			120
#define BROWSE_FILE_NAME_POS_Y			6
#define BROWSE_RESV_POS_X				(-100)
#define BROWSE_RESV_POS_Y				36

/*  the following definitions for setting    */
#define SETTING_BACKGROUND_WIDTH		320 //menu box size
#define SETTING_BACKGROUND_HEIGHT		240

#define SETTING_TOP_BAR_WIDTH			48  //top bar icon
#define SETTING_TOP_BAR_HEIGHT			32
#define SETTING_TOP_BAR_POS_X			12
#define SETTING_TOP_BAR_POS_Y			10

#define SETTING_BACKGROUND_MODE_WIDTH	64  //mode icon
#define SETTING_BACKGROUND_MODE_HEIGHT	32
#define SETTING_BACKGROUND_MODE_POS_X	12
#define SETTING_BACKGROUND_MODE_POS_Y	(SETTING_BACKGROUND_HEIGHT - SETTING_BACKGROUND_MODE_HEIGHT - 2)

#define SETTING_BACKGROUND_UP_WIDTH     32  // up icon
#define SETTING_BACKGROUND_UP_HEIGHT    32
#define SETTING_BACKGROUND_UP_POS_X     (SETTING_BACKGROUND_MODE_POS_X + 110)
#define SETTING_BACKGROUND_UP_POS_Y     (SETTING_BACKGROUND_MODE_POS_Y)

#define SETTING_HEAD_WIDTH				32  //head icon of setting item
#define SETTING_HEAD_HEIGHT				32
#define SETTING_HEAD_POS_X				18
#define SETTING_HEAD_POS_Y				52

#define SETTING_SELECT_BAR_WIDTH		296 //select bar for main menu
#define SETTING_SELECT_BAR_HEIGHT		40
#define SETTING_SELECT_BAR_POS_X		14
#define SETTING_SELECT_BAR_POS_Y		48

#define SETTING_SELECTBOX_SUB_WIDTH		256 //select box(sub menu)
#define SETTING_SELECTBOX_SUB_HEIGHT	160
#define SETTING_SELECTBOX_SUB_POS_X		54
#define SETTING_SELECTBOX_SUB_POS_Y		46

#define SETTING_SELECTBAR_MIDD_WIDTH	216 //select bar for select box(sub menu)
#define SETTING_SELECTBAR_MIDD_HEIGHT	32
#define SETTING_SELECTBAR_MIDD_POS_X	(SETTING_SELECTBOX_SUB_POS_X + ((SETTING_SELECTBOX_SUB_WIDTH - SETTING_SELECTBAR_MIDD_WIDTH) >> 1))
#define SETTING_SELECTBAR_MIDD_POS_Y	(SETTING_SELECTBOX_SUB_POS_Y + 32)

#define SETTING_YEAR_SELECTBAR_WIDTH	64  //select bar for year
#define SETTING_YEAR_SELECTBAR_HEIGHT	32

#define SETTING_DAY_SELECTBAR_WIDTH		32  //select bar for month, day, hour, minute, second
#define SETTING_DAY_SELECTBAR_HEIGHT	32

#define WARN_STR_COLOR                  WHITE_COLOR
#define WARN_BACK_COLOR                 BLUE_COLOR
#define BROWSE_FILE_NAME_COLOR			YELLOW_COLOR


extern DISPLAY_ICONSHOW display_icon_item[];
extern DISPLAY_ICONSHOW display_num_item[];

extern INT16U *setting_background[];
extern SETTING_ITEM setting_record_table[];
extern SETTING_ITEM setting_capture_table[];
extern SETTING_ITEM setting_play_table[];
extern SETTING_ITEM setting_base_table[];

#endif // __RESOURCE_H__

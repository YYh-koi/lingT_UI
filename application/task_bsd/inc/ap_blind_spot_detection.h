#ifndef __AP_BLIND_SPOT_DETECTION_H__
#define __AP_BLIND_SPOT_DETECTION_H__

#include "typedef.h"
#include "define.h"
#include "BSD_nn_kernel.h"

#define BSD_SCALE_TAR_WIDTH     640
#define BSD_SCALE_TAR_HEIGHT    360
#define BSD_BUF_SIZE			(BSD_SCALE_TAR_WIDTH * BSD_SCALE_TAR_HEIGHT) // bsd process w*h
#define BSD_Q_CNT			    3

#ifndef Max
#define Max(a,b)       (((a) > (b)) ? (a) : (b))
#endif
#ifndef Min
#define Min(a,b)       (((a) < (b)) ? (a) : (b))
#endif

#define OBJ_PROCESS_SRC_WIDTH                       320
#define OBJ_PROCESS_SRC_HEIGHT                      240
#define OBJECT_MAX_RESULT 20

// Generalplus
// Level 1
#define DET_THRES 0.5
#define NUM_CLASSES 4
#define CLASS_NAMES {"Car", "Bus", "Truck", "Person"}  //"Motor_Bike"
#define BSD_IN_W 448
#define BSD_IN_H 256
#define BSD_H_PADDING 2
#define BSD_OUTPUT_CH 64
#define LDW_THRE1 200   //ms
#define LDW_THRE2 2000  //ms
#define LDW_THRE3 2000  //ms
#define LDW_THRE4 4000  //ms
#define MISS_THRE 4000  //ms

//FCW & VB & S&G  & perdestrian
#define VIRTUAL_BUMPERTHRE  4
#define PEDESTRIAN_THRE  40
#define TTC_TIMETHRE  1500


#define DET_LAYERS 3 //2
#define NUM_ANCHORS 7
// NOTE: All the shapes here should be (width, height).
//#define ANCHORS {{{33,65}, {63,40}, {85,60}, {42,123}, {64,131}, {122,87}, {145,145}}, {{7,6}, {11,9}, {8,19}, {18,13}, {25,20}, {16,42}, {39,28}}}
#define ANCHORS {{{73,40}, {42,73}, {80,59}, {69,97}, {105,68}, {146,75}, {169,209}}, {{26,20}, {40,20}, {22,44}, {36,30}, {54,30}, {29,58}, {57,45}}, {{8,6}, {11,9}, {8,16}, {16,12}, {12,22}, {22,15}, {16,31}}}
//#define BSD_OUTPUT_SHAPES {{14,8},{28,16}}
#define BSD_OUTPUT_SHAPES {{14,8},{28,16},{56, 32}}
//#define BSD_OUTPUT_STRIDES {32,16}
#define BSD_OUTPUT_STRIDES {32,16,8}

// Level 2
#define BSD_IMG_IN_W BSD_IN_W
#define BSD_IMG_IN_H (BSD_IN_H-2*BSD_H_PADDING)

typedef struct
{
    nnResult BBoxResult[OBJECT_MAX_RESULT];
    gpRect object_result_roi[OBJECT_MAX_RESULT];
    gpRect object_follow_roi;
    gpRect input_lock_roi;
    INT32U type_index[OBJECT_MAX_RESULT];
    INT32U Score[OBJECT_MAX_RESULT];
	gpPoint object_result_dot[36];
	gpPoint lane_result_dot[4];
    INT32U image_frame_ptr;
    INT32U image_y_frame_ptr;
    INT32U times;
    INT32U num_dets;
	INT32U LDW_process_lane_state;
	INT32U LDW_process_follow_dist;
	INT32U SnG_AlarmFlg;
	INT32U pedestrian_alarmFlg;
	INT32U virtual_bumperFlg;
} IMAGE_FRAME_STRUCT;

typedef struct{
  int id;
  int x;
	int y;
	int width;
	int height;
	int distance;
} Final_TrackingBox;

extern void bsd_q_reset(void);
extern INT32S bsd_q_get_idle(INT32U *buf);
extern void PScaler_BSD_Gray_Callback(PS_ID scaler, PS_ST PScaler_Event);

#endif // __AP_BLIND_SPOT_DETECTION_H__

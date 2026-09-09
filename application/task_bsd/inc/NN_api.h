#ifndef _NN_API_H_
#define _NN_API_H_
#include "typedef.h"
#include "define.h"
#include "face_nn_kernel.h"

typedef struct
{
	int x;
	int y;
	int width;
	int height;
	int confidence;
	int label;
}BoundingBox;

typedef struct
{
	BoundingBox box;
	int kept;
}DetectionResult;

typedef struct
{
	float x;
	float y;
	float confidence;
}Keypoint;

typedef struct
{
	// ROI in the image
	int x1;
	int x2;
	int y1;
	int y2;
	// Paddings out of image
	int pleft;
	int ptop;
	int pright;
	int pdown;
}PoseInputInfo;

typedef struct {
    GP_NN_Model_NEW *nn_model;
    float *feature_out;
    INT32U IN_MODEL_image;
    INT32U GPDLA_BUF;
    INT32U Model_buffer;
    gpPoint *fl_list;
    int pass_flag;
    int point_count;
    gpRect clip_mp;
    gpRect clip_pre;
    int cut_angle;
    int *recognition_ID;
}FL_construction;

typedef struct {
	float score;
	int type;
	BoundingBox boxse;
	int keep;
}NMS;

typedef struct {
    GP_NN_Model_NEW *face_model;
    float *feature_face;
    char *update_index;
    BoundingBox *update_face;
    BoundingBox *best_face;
    INT32U IN_image;
    INT32U Crop_imge;
    INT32U IN_MODEL_image;
    INT32U GPDLA_BUF_face;
    int face_handel;
    int count;
    float update_factor;
    int max_find_face;
    void (*FD_crop_scale_to_224) (INT32U fd_y_buf,INT32U fd_y1,INT32U fd_224_y);
    float *last_IOU;
    INT32U Model_buffer;
    float conf_thre;
}FD_ContexOPT;

typedef struct {
    GP_NN_Model_NEW *nn_model;
    float *feature_out;
    BoundingBox *best_object_result;
    INT32U IN_image;
    INT32U Crop_imge;
    INT32U IN_MODEL_image;
    INT32U GPDLA_BUF;
    void (*FD_crop_scale_to_256) (INT32U fd_y_buf,INT32U fd_y1,INT32U fd_256_y);
    INT32U Model_buffer;
    int max_find_obj;
    float conf_thre;
    int count;
    int recognitionID;
}NN_construction;

void run_nn(NN_construction *NN_contex);
void nn_model_init(NN_construction *NN_contex, const GP_NN_Model_NEW* nn_model);
void softmax(float *input,float *output, int input_len);
void NN_Alphabet_Flow(NN_construction *FD_contex);
void NN_Animal_Flow(NN_construction *animal_contex);
void NN_Smile_Flow(NN_construction *animal_contex);
void NN_CartoonFace_Flow(NN_construction *CF_contex);
void NN_FL_TRAIN_Flow(FL_construction *FL_contex);
void NN_BodySeg_Half_Flow(NN_construction *SG_contex);
void NN_BodySeg_Full_Flow(NN_construction *SG_contex);
void MV2_FD_smooth_boxes(NN_construction *obj_contex, DetectionResult *NMS_list, DetectionResult *NMS_list_prev, int current_count, float alpha, float thr, int prev_count, int* tracking_index);
int MV2_FD_flow(NN_construction *NN_contex, DetectionResult *NMS_list);
int yolo_person_flow(NN_construction *NN_contex, DetectionResult *NMS_list);
void yolo_person_smooth_boxes(DetectionResult *NMS_list, DetectionResult *NMS_list_prev, int current_count, float alpha, float thr, int prev_count, int* tracking_index);
void pose_smooth_boxes(DetectionResult *NMS_list, DetectionResult *NMS_list_prev, int current_count, float alpha, float thr, int prev_count, int* tracking_index, float* ious);
void pose_smooth_bounding_boxes(DetectionResult *NMS_list, DetectionResult *NMS_list_prev, int current_count, float alpha, float thr, int prev_count, int* tracking_index, float* ious);
void body_pose_flow(NN_construction *NN_contex, Keypoint *kp_list, PoseInputInfo *info);
void smooth_keypoints(Keypoint *kp_list_ptr, Keypoint *kp_list_prev_ptr, float* kp_thres_ptr, float iou);  //for body keypoint
void pose_smooth_keypoints(Keypoint *kp_list_ptr, Keypoint *kp_list_prev_ptr, float* kp_thres_ptr, float iou);  //for body keypoint
int MV2_HAND_flow(NN_construction *NN_contex, DetectionResult *NMS_list);
void MV2_HAND_smooth_boxes(NN_construction *obj_contex, DetectionResult *NMS_list, DetectionResult *NMS_list_prev, int current_count, float alpha, float thr, int prev_count, int* tracking_index);
#endif

#include "typedef.h"
#include "define.h"
#include "LDW_nn_kernel.h"

#define LDW_IN_W                        320
#define LDW_IN_H                        192
#define LDW_IN_C                          1

#define NUM_KEYPOINTS                    36

typedef struct {
    float x;
    float y;
} line_keypoint;

typedef struct {
    line_keypoint keypoints[NUM_KEYPOINTS];
} line_result;

typedef struct {
    GP_NN_Model_NEW *nn_model;
    float *feature_out;
    INT32U IN_MODEL_image;
    INT32U GPDLA_BUF;
    INT32U Model_buffer;
    line_keypoint *LDW_result;
}LDW_construction;

void NN_LDW_Flow(LDW_construction *LDW_contex, unsigned char *s_d);
//void NN_LDW_Flow(LDW_construction *LDW_contex);

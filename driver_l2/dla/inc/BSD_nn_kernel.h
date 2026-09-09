//Timestamp: 2023-09-08 15:00:30
#ifndef _BSD_GP_NN_NEW_KERNEL_H_
#define _BSD_GP_NN_NEW_KERNEL_H_

#include "typedef.h"
#include "define.h"
#ifndef _GP_NN_NEW_STRUCT_
#define _GP_NN_NEW_STRUCT_
typedef struct {
	INT32U out_w;
	INT32U out_h;
	INT32U out_c;
	INT32  zero_point;
	float  scale;
	INT32U offset;
}Output_Info;

typedef struct {
	char *name;
	char *version;
	INT32U (*dla_run_start) (INT32U input_data, INT32U output_buf, INT32U in_buf1);
	INT32U input_size;
	INT32U output_layers;
	INT32U buffer_size;
	INT32U buffer_blocks;;
	INT32U layer_num;
	Output_Info output_info[];;
} GP_NN_Model_NEW;

#define GP_NN_Model_NEW(NAME)      (&NAME##_gp_nn_model)
#endif

extern const GP_NN_Model_NEW BSD_gp_nn_model;

INT32U BSD_dla_run_start(INT32U input_data, INT32U output_buf, INT32U in_buf1);

#endif

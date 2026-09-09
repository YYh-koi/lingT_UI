#include "gplib_print_string.h"
#include "project.h"
#include "gplib_mm_gplus.h"
#include "NN_api.h"
#include <math.h>
#if 0
void softmax(float *input,float *output, int input_len)
{
	//assert(input != NULL);
	//assert(input_len != 0);
	int i;
	float m;
	/* Find maximum value from input array */
	m = -INFINITY;
	for (i = 1; i < input_len; i++) {
		if (input[i] > m) {
			m = input[i];
		}
	}

	float sum = 0;
	for (i = 0; i < input_len; i++) {
		sum += expf(input[i] - m);
	}

	for (i = 0; i < input_len; i++) {
		output[i] = expf(input[i] - m - log(sum));

	}
}
#endif
static int gp_arm_q7_to_float_s8(
	q7_t * pSrc,
	float32_t * pDst,
	float scale,
	int   offset,
	uint32_t blockSize)
{
	q7_t *pIn = pSrc;                              /* Src pointer */
	uint32_t blkCnt;                               /* loop counter */
	//float sca_factor = (float)128;
	//float sca_factor = (float) 16;
	/* Run the below code for Cortex-M4 and Cortex-M3 */

	/*loop Unrolling */
	blkCnt = blockSize >> 2U;


	/* First part of the processing with loop unrolling.  Compute 4 outputs at a time.
	 ** a second loop below computes the remaining 1 to 3 samples. */
	while (blkCnt > 0U)
	{
		/* C = (float32_t) A / 128 */
		/* convert from q7 to float and then store the results in the destination buffer */
		*pDst++ = ((float32_t)* pIn++ - (float)offset)*scale;
		*pDst++ = ((float32_t)* pIn++ - (float)offset) * scale;
		*pDst++ = ((float32_t)* pIn++ - (float)offset) *scale;
		*pDst++ = ((float32_t)* pIn++ - (float)offset) * scale;

		/* Decrement the loop counter */
		blkCnt--;
	}

	/* If the blockSize is not a multiple of 4, compute any remaining output samples here.
	 ** No loop unrolling is used. */
	blkCnt = blockSize % 0x4U;

	while (blkCnt > 0U)
	{
		/* C = (float32_t) A / 128 */
		/* convert from q7 to float and then store the results in the destination buffer */
		*pDst++ = ((float32_t)* pIn++ - (float)offset) * scale;

		/* Decrement the loop counter */
		blkCnt--;
	}
	return 0;
}

void run_nn(NN_construction *NN_contex){
    portTASK_USES_FLOATING_POINT();
    float* feature_addr[8] = { };
    INT8S *OPUTDATA_REODER;
    INT32U BUFFER_START_ADD,buf_size,i;
    GP_NN_Model_NEW *Model=NN_contex->nn_model;
    feature_addr[0] = NN_contex->feature_out;
    OPUTDATA_REODER=(INT8S *)Model->dla_run_start((INT32U)NN_contex->IN_MODEL_image, (INT32U)NN_contex->GPDLA_BUF, (INT32U)NN_contex->Model_buffer);
    BUFFER_START_ADD=(INT32U)OPUTDATA_REODER;
    for(i=0;i<Model->output_layers;i++)
    {
        buf_size=Model->output_info[i].out_c*Model->output_info[i].out_h*Model->output_info[i].out_w;
		// DBG_PRINT("%s buf_size=%d\r\n",__func__,buf_size);
        gp_arm_q7_to_float_s8((signed char*)BUFFER_START_ADD,(float32_t*)feature_addr[i],Model->output_info[i].scale,Model->output_info[i].zero_point,buf_size);
        BUFFER_START_ADD=BUFFER_START_ADD+buf_size;
        feature_addr[i+1] = (float*)(feature_addr[i]+buf_size);
    }

}


static float *object_feature=0;
static INT32U process_buf_size=0;
static INT32U object_process_buf=0,object_out_buf=0;
void nn_model_init(NN_construction *NN_contex, const GP_NN_Model_NEW *nn_model)
{

	if(nn_model == 0)
	{
		DBG_PRINT("nn_model error\r\n");
		while (1);
	}

   if(object_process_buf==0)
    {

        process_buf_size = drv_l2_dla_module_max_input_size_get();
        if(process_buf_size)
        {
            object_process_buf = (INT32U)gp_malloc_align(process_buf_size, 64);
            if(object_process_buf == 0)
            {
                DBG_PRINT("object_process_buf malloc error\r\n");
                while(1);
            }
            DBG_PRINT("object_process_buf = 0x%x,(%d)\r\n", object_process_buf,process_buf_size);
        }
        else
        {
            DBG_PRINT("drv_l2_dla_module_max_input_size_get error\r\n");
            while (1);
        }
    }
    if(object_out_buf==0)
    {
        process_buf_size = drv_l2_dla_module_max_output_size_get();
        if(process_buf_size)
        {
            object_out_buf = (INT32U)gp_malloc_align(process_buf_size, 64);
            if(object_out_buf == 0)
            {
                DBG_PRINT("object_out_buf malloc error\r\n");
                while(1);
            }
            DBG_PRINT("object_out_buf = 0x%x(%d)\r\n", object_out_buf,process_buf_size);

            process_buf_size = process_buf_size*sizeof(float);
            object_feature = (float *)gp_malloc_align(process_buf_size, 64);
            if(object_feature == 0)
            {
                DBG_PRINT("object_feature malloc error\r\n");
                while(1);
            }
            DBG_PRINT("object_feature = 0x%x(%d)\r\n", object_feature,process_buf_size);
        }
        else
        {
            DBG_PRINT("drv_l2_dla_module_max_output_size_get error\r\n");
            while (1);
        }
    }


    NN_contex->nn_model = (GP_NN_Model_NEW *)nn_model;
    NN_contex->conf_thre = 0;
    NN_contex->feature_out = (float *)object_feature;
    NN_contex->GPDLA_BUF = (INT32U)object_out_buf;
    NN_contex->Model_buffer = (INT32U)object_process_buf;
    NN_contex->count = 0;
    NN_contex->recognitionID = 999;

}

#include "LDW_api.h"
void LDW_model_init(LDW_construction *NN_contex, const GP_NN_Model_NEW *nn_model)
{
    NN_contex->nn_model = (GP_NN_Model_NEW *)nn_model;
	NN_contex->feature_out = (float *)object_feature;
    NN_contex->GPDLA_BUF = (INT32U)object_out_buf;
    NN_contex->Model_buffer = (INT32U)object_process_buf;
	DBG_PRINT("LDW_model_init:0x%x, 0x%x, 0x%x\r\n",(INT32U)object_feature,(INT32U)object_out_buf,(INT32U)object_process_buf);
}

void nn_model_uninit(NN_construction *NN_contex)
{
	process_buf_size = 0;
	// free NN memroy
    gp_free((void*)(NN_contex->feature_out));
	object_feature = 0;
    gp_free((void*)(NN_contex->GPDLA_BUF));
	object_out_buf = 0;	
    gp_free((void*)(NN_contex->Model_buffer));
	object_process_buf = 0;
}

void nn_model_cache_invalid(NN_construction *NN_contex)
{
    INT32U process_buf_size;

 	{
        process_buf_size = drv_l2_dla_module_max_input_size_get();
		cache_invalid_range(NN_contex->Model_buffer, process_buf_size);
	}
	
    {
        process_buf_size = drv_l2_dla_module_max_output_size_get();
		cache_invalid_range(NN_contex->GPDLA_BUF, process_buf_size);

		process_buf_size = process_buf_size*sizeof(float);
		cache_invalid_range(NN_contex->feature_out, process_buf_size);
    }
}



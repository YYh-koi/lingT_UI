#ifndef __DRV_L1_FFT_H_
#define __DRV_L1_FFT_H_

#define C_FFT_ISR_QUEUE_SIZE	1
#define C_FFT_ACK				1

#define SET_MODE_FFT			0x00000000
#define SET_MODE_IFFT			0x00000008

#define SET_FFT_32P			    0x00000000
#define SET_FFT_64P			    0x00000001
#define SET_FFT_128P			0x00000002
#define SET_FFT_256P			0x00000003
#define SET_FFT_512P			0x00000004
#define SET_FFT_1024P			0x00000005
#define SET_FFT_2048P			0x00000006

#define SET_SCL_STG_0_12		0x00000001
#define SET_SCL_STG_0_14		0x00000002
#define SET_SCL_STG_1_12		0x00000004
#define SET_SCL_STG_1_14		0x00000008
#define SET_SCL_STG_2_12		0x00000010
#define SET_SCL_STG_2_14		0x00000020
#define SET_SCL_STG_3_12		0x00000040
#define SET_SCL_STG_3_14		0x00000080
#define SET_SCL_STG_4_12		0x00000100
#define SET_SCL_STG_4_14		0x00000200
#define SET_SCL_STG_5_12		0x00000400
#define SET_SCL_STG_5_14		0x00000800

#define SET_FFT_INT_ENA			0x00008000
#define SET_FFT_HW_ENA			0x00000100	//enable FFT module
#define SET_FFT_TRIG			0x00000001	//FFT start
#define SET_FFT_RST			    0x00000002	//FFT reset

#define SET_16b_FFTIN_R_AND_I	0x00000000
#define SET_16b_FFTIN_R			0x00000010
#define SET_24b_FFTIN_R_AND_I	0x00000020
#define SET_24b_FFTIN_R			0x00000030


#define SET_16b_FFTOUT_R_AND_I	0x00000000
#define SET_16b_FFTOUT_R		0x00000040
#define SET_24b_FFTOUT_R_AND_I	0x00000080
#define SET_24b_FFTOUT_R		0x000000C0

#define SET_DIR_FFT			    0x00000000
#define SET_DIR_IFFT			0x00000008

#define C_DIR_FFT               0
#define C_DIR_IFFT              1


typedef enum{
    C_IN_R_16b=1,		//input 16-bit real only
    C_IN_R_I_16b,		//input 16-bit real and image
    C_IN_R_24b,			//input 24-bit real only
    C_IN_R_I_24b		//input 24-bit real and image
}FFT_IN_FMT_ENUM;

typedef enum{
    C_OUT_R_16b=1,		//output 16-bit real only
    C_OUT_R_I_16b,		//output 16-bit real and image
    C_OUT_R_24b,		//output 24-bit real only
    C_OUT_R_I_24b		//output 24-bit real and image
}FFT_OUT_FMT_ENUM;

/**
 * @brief   FFT init
 * @param   vic_fft: 0: use IRQ; other value: use FIQ
 * @return  none
 */
extern void drv_l1_fft_init(INT32U vic_fft);

/**
 * @brief   FFT hardware enable
 * @param   none
 * @return  none
 */
extern void drv_l1_fft_hw_en(void);

/**
 * @brief   FFT hardware disable
 * @param   none
 * @return  none
 */
extern void drv_l1_fft_hw_dis(void);

/**
 * @brief   FFT interrupt enable
 * @param   none
 * @return  none
 */
extern void drv_l1_fft_ena_intr(void);

/**
 * @brief   FFT interrupt disable
 * @param   none
 * @return  none
 */
extern void drv_l1_fft_dis_intr(void);

/**
 * @brief   FFT isr hook setting
 * @param   fft_cbk: FFT isr call back function
 * @return  none
 */
extern void drv_l1_fft_isr_hook_set( void(*fft_cbk)(void));

/**
 * @brief   FFT isr hook clear
 * @param   none
 * @return  none
 */
extern void drv_l1_fft_isr_hook_clear(void);

/**
 * @brief   FFT set points
 * @param   FFT_N: FFT points: 32/64/128/256/512/1024/2048
 * @return  0 is success, -1 is fail.
 */
extern INT32S drv_l1_fft_set_points(INT32U FFT_N);

/**
 * @brief   FFT set direction
 * @param   inv: 0: FFT mode; 1: IFFT mode
 * @return  0 is success, -1 is fail
 */
extern INT32S drv_l1_set_fft_dir(INT32U inv);

/**
 * @brief   FFT set scale
 * @param   stage: 0~5 to select which stage to scale
 * @param   div: 0: no scale; 1: scale to 1/2; 2: scale to 1/4
 * @return  0 is success, -1 is fail
 */
extern INT32S drv_l1_fft_set_scl(INT32U stage, INT32U div);

/**
 * @brief   FFT input buffer address
 * @param   addr: input buffer address
 * @return  none
 */
extern void drv_l1_fft_in_addr(INT32U *addr);

/**
 * @brief   FFT output buffer address
 * @param   addr: output buffer address
 * @return  none
 */
extern void drv_l1_fft_out_addr(INT32U *addr);

/**
 * @brief   FFT set input format
 * @param   in_fmt: 0: 16b real + imagine input; 1: 16b real only input; 2: 24b real + imagine input; 3: 24b real only input
 * @return  0 is success, -1 is fail
 */
extern INT32S drv_l1_fft_set_in_fmt(FFT_IN_FMT_ENUM in_fmt);

/**
 * @brief   FFT set output format
 * @param   out_fmt: 0: 16b real + imagine input; 1: 16b real only input; 2: 24b real + imagine input; 3: 24b real only input
 * @return  0 is success, -1 is fail
 */
extern INT32S drv_l1_fft_set_out_fmt(FFT_OUT_FMT_ENUM out_fmt);

/**
 * @brief   FFT start
 * @param   none
 * @return  none
 */
extern void drv_l1_fft_start(void);

/**
 * @brief   Lock FFT semaphore
 * @param   none
 * @return  none
 */
extern void drv_l1_fft_obj_lock(void);

/**
 * @brief   Unlock FFT semaphore
 * @param   none
 * @return  none
 */
extern void drv_l1_fft_obj_unlock(void);

/**
 * @brief   FFT uninit
 * @param   none
 * @return  none
 */
extern void drv_l1_fft_uninit(void);

/**
 * @brief   FFT wait done
 * @param   timeout: 0: osWaitForever; others: timeout in ms
 * @return  none
 */
extern INT32S drv_l1_fft_wait_done(INT32U timeout);

/**
 * @brief   FFT get overflow status
 * @param   none
 * @return  overflow status: bit5~0 corresponding to stage5~0, bit6 is output stage. 1 means overflow is occured.
 */
extern INT32U drv_l1_get_fft_ovf_sts(void);

#endif // __DRV_L1_FFT_H_

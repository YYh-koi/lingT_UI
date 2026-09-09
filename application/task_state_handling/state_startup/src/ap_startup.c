#include "application.h"

extern int	ap_peripheral_power_key_read(int pin);
extern void video_encode_entrance(void);
INT8U		avi_enc_start_flag = 0;

void
ap_startup_init(
	void
)
{
	/* 1.when power on , only adapter in , video auto record
	2. avi_enc_start_flag status judge by power on
 */

	#if !defined ADP_OUT_PIN
		avi_enc_start_flag = 1;
	#else
		#if (ADP_PIN_TYPE == READ_FROM_GPIO)
			if(gpio_read_io(ADP_OUT_PIN))
		#else
			if(drv_l1_sys_pwr_key_read(ADP_OUT_PIN))
		#endif
			{
				avi_enc_start_flag = 1;
			}
	#endif


	video_encode_entrance();
}

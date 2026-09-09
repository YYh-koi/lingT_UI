
#include "task_state_handling.h"


/**************************************************************************
 *                      C O N S T A N T S                            *
 **************************************************************************/
typedef enum
{
	FLAG_DISCONNECT	 = 0,		// Disable WIFI module
	FLAG_CONNECT     = (1 << 0),// Enable WIFI module
	FLAG_OFF_DISPLAY = (1 << 1) // Turn off the TFT display
} WIFI_STATE_FLAG;


/**************************************************************************
 *                      F U N C T I O N S                                 *
 **************************************************************************/
extern void sw_wifi_ack_init(void);
extern void sw_wifi_ack_send(INT8U retResult);
extern INT16U wifi_display_height_get(void);
extern INT16U wifi_display_width_get(void);
extern INT16U wifi_display_pip_height_get(void);
extern INT16U wifi_display_pip_width_get(void);
extern DISP_FMT wifi_display_format_get(void);
extern INT32U wifi_encode_type_get(void);

extern void state_wifi_status_set(INT8S state_flag);
extern INT8U state_wifi_status_get(void);
extern INT32S state_wifi_connect(void);
extern INT32S state_wifi_disconnect(void);
extern INT32S state_wifi_symbol_show(INT8U EnDisable);

extern INT32U wifi_display_buf_size_get(void);


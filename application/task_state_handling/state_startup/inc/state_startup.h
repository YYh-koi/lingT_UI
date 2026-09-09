#ifndef __STATE_STARTUP_H__
#define __STATE_STARTUP_H__

#if C_GSENSOR_FUNCTION
extern INT8U gsensor_power_on;
#endif

extern void state_startup_entry(void *para);
#endif // __STATE_STARTUP_H__

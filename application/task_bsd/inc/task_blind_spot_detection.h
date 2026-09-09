#ifndef __TASK_BLIND_SPOT_DETECTION_H__
#define __TASK_BLIND_SPOT_DETECTION_H__

extern void bsd_task_init(void);
extern void bsd_task_entry(void const *para);
extern INT32S bsd_task_msg_send(INT32U msg, void *para, INT32U bytes, INT32U prio);

#endif // __TASK_BLIND_SPOT_DETECTION_H__

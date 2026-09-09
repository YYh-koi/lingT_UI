#ifndef __TASK_STORAGE_SERVICE_H__
#define __TASK_STORAGE_SERVICE_H__

extern void     storage_service_init(void);

extern void		task_storage_service_entry(void const *turnkey_args);
extern INT32S   storage_task_msg_send(INT32U msg, void *para, INT32U bytes, INT32U prio);
extern INT32S   storage_task_msg_send_with_ack(INT32U msg, INT32U msc_timeout);

#endif // __TASK_STORAGE_SERVICE_H__

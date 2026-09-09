#ifndef LOG_SERVICE_H
#define LOG_SERVICE_H

#include "dlist.h"

#ifdef __ICCARM__
#define STRINGIFY(s) #s
#define SECTION(_name) _Pragma( STRINGIFY(location=##_name##))
#define log_module_init(fn) \
	SECTION(".data.log_init") __root static void* log_##fn = (void*)fn
#elif defined(__CC_ARM)
#define log_module_init(fn) \
static void* log_##fn __attribute__((section(".data.log_init")))  = (void*)fn;
#define DiagPrintf printf
#else
//#error "not implement"
#define log_module_init(fn) \
static void* log_##fn = (void*)fn;
#endif

#define ATC_INDEX_NUM 32

#define STACKSIZE               1280

//#define CONFIG_LOG_HISTORY
#ifdef CONFIG_LOG_HISTORY
#define LOG_HISTORY_LEN    5
#endif

typedef void (*log_init_t)(void);
typedef void (*log_act_t)(void*);
typedef struct _at_command_item_{
	char *log_cmd;
	log_act_t at_act;
	struct list_head node;
}log_item_t;

void log_service_add_table(log_item_t *tbl, int len);

#endif

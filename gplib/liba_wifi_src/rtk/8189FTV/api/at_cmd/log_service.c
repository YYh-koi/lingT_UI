#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "log_service.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define AT_LOG_INIT 0
//======================================================
struct list_head log_hash[ATC_INDEX_NUM];

extern void at_wifi_init(void);
char log_buf[128];
#ifdef CONFIG_LOG_HISTORY
char log_history[LOG_HISTORY_LEN][64];
static unsigned int log_history_count = 0;
#endif
extern char uart_buf[128];
xSemaphoreHandle log_rx_interrupt_sema = NULL;
//extern xSemaphoreHandle	uart_rx_interrupt_sema;
xSemaphoreHandle uart_rx_interrupt_sema = NULL;

#if defined (__ICCARM__)
#pragma section=".data.log_init"

unsigned int __log_init_begin__;
unsigned int __log_init_end__;
#elif defined ( __CC_ARM   )
//#pragma section=".data.log_init"
log_init_t* __log_init_begin__;
log_init_t* __log_init_end__;
#else
//#error "not implement, add to linker script"
/*extern*/ unsigned int __log_init_begin__;
/*extern*/ unsigned int __log_init_end__;
#endif

log_init_t log_init_table[] = {
	at_wifi_init
};

//======================================================
int hash_index(char *str)
{
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;

    while (*str)
    {
        hash = hash * seed + (*str++);
    }

    return (hash & 0x7FFFFFFF);
}

void log_add_new_command(log_item_t *new)
{
	int index = hash_index(new->log_cmd)%ATC_INDEX_NUM;

	list_add(&new->node, &log_hash[index]);
}
void start_log_service(void);
void log_service_init(void)
{
	int i;

#if defined (__ICCARM__)
	log_init_t *log_init_table;
	__log_init_begin__ = (unsigned int)__section_begin(".data.log_init");
	__log_init_end__ = (unsigned int)__section_end(".data.log_init");
	log_init_table = (log_init_t *)__log_init_begin__;
#elif defined(__CC_ARM)
	__log_init_begin__ = log_init_table;
	__log_init_end__ = log_init_table + sizeof(log_init_table);
#else
	//#error "not implement"
	__log_init_begin__ = (unsigned int)log_init_table;
	__log_init_end__ = (unsigned int)log_init_table + sizeof(log_init_table);
#endif


	for(i=0;i<ATC_INDEX_NUM;i++)
		INIT_LIST_HEAD(&log_hash[i]);

	for(i=0;i<(__log_init_end__-__log_init_begin__)/sizeof(log_init_t); i++)
		log_init_table[i]();

  	/* Initial uart rx swmaphore*/
//	vSemaphoreCreateBinary(log_rx_interrupt_sema);
//	xSemaphoreTake(log_rx_interrupt_sema, 1/portTICK_RATE_MS);

    /* Initial uart rx swmaphore*/
    vSemaphoreCreateBinary(uart_rx_interrupt_sema);
    xSemaphoreTake(uart_rx_interrupt_sema, 1/portTICK_RATE_MS);
    /* Initial uart rx swmaphore*/
    vSemaphoreCreateBinary(log_rx_interrupt_sema);
    xSemaphoreTake(log_rx_interrupt_sema, 1/portTICK_RATE_MS);

    //start_log_service();
}

//sizeof(log_items)/sizeof(log_items[0])
void log_service_add_table(log_item_t *tbl, int len)
{
	int i;
	for(i=0;i<len;i++)
		log_add_new_command(&tbl[i]);
}

void* log_action(char *cmd)
{
	int search_cnt=0;
	int index = hash_index(cmd)%ATC_INDEX_NUM;
	struct list_head *head = &log_hash[index];
	struct list_head *iterator;
	log_item_t *item;
	void *act = NULL;

	list_for_each(iterator, head) {
		item = list_entry(iterator, log_item_t, node);
		search_cnt++;
		if( strcmp(item->log_cmd, cmd) == 0){
			//printf("%s match %s, search cnt %d\n\r", cmd, item->log_cmd, search_cnt);
			act = (void*)item->at_act;
			break;
		}
	}

	return act;
}

void* log_handler(char *cmd)
{
	log_act_t action=NULL;
	char buf[100] = {0};
	char *copy=buf;
	char *token = NULL;
	char *param = NULL;
	char tok[5] = {0};//'\0'
#ifdef CONFIG_LOG_HISTORY
	strcpy(log_history[((log_history_count++)%LOG_HISTORY_LEN)], log_buf);
#endif
	strcpy(copy, cmd);
        //token = _strsep(&copy, "=");
	token = strtok(copy, "=");
	if(token && (strlen(token) <= 4))
		strcpy(tok, token);
	else{
		//printf("\n\rAT Cmd format error!\n");
		return NULL;
	};
	//printf(" Command %s \n\r ", tok);
	#if 1
	{
		int t_len = strlen(tok);
		// printf("t_len=%d\r\n",t_len);
		param = (char *)(copy+(t_len+1));		
	}
	#else
	param = strtok(NULL, " ");
	#endif
	//printf(" Param %s \n\r", param);
	action = (log_act_t)log_action(tok);

	if(action){
		action(param);
		printf("\n\r[MEM] After do cmd, available heap %d\n\r", xPortGetFreeHeapSize());
	}
	return (void*)action;

}

void legency_interactive_handler(unsigned char argc, unsigned char **argv)
{
#if 0 //defined(CONFIG_PLATFORM_8195A)
   if(argc<1)
	{
		DiagPrintf("Wrong argument number!\r\n");
		return;
	}


	DiagPrintf("Wlan Normal Mode\n");

	WlanNormal( argc, argv);
#else
   strcpy(uart_buf, log_buf);
   xSemaphoreGive(uart_rx_interrupt_sema);
#endif
}

void log_service(void *param)
{
	printf("\n\rStart LOG SERVICE MODE\n\r");
	printf("\n\r# ");
	while(1){
		while(xSemaphoreTake(log_rx_interrupt_sema, portMAX_DELAY) != pdTRUE);
		if(log_handler((char *)log_buf) == NULL){
			legency_interactive_handler(0, 0);
		}
		log_buf[0] = '\0';
		printf("\r\n\n# ");
	}
}

void start_log_service(void)
{
	if(xTaskCreate(log_service, (const char *)"log_service", STACKSIZE, NULL, tskIDLE_PRIORITY + 5, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate failed", __FUNCTION__);
}
void fAT_exit(void *arg){
	printf("\n\rLeave LOG SERVICE");
	vTaskDelete(NULL);
}
#ifdef CONFIG_LOG_HISTORY
void fAT_log(void *arg){
        int i = 0;
        printf("[AT]log history:\n\n\r");
        if(log_history_count > LOG_HISTORY_LEN){
          for(i=0; i<4; i++)
            printf("  %s\n\r", log_history[((log_history_count+i)%LOG_HISTORY_LEN)]);
        }
        else{
          for(i=0; i<(log_history_count-1); i++)
            printf("  %s\n\r", log_history[i]);
        }
}
#endif
log_item_t at_log_items[ ] = {
      {"AT--", fAT_exit,},
#ifdef CONFIG_LOG_HISTORY
      {"AT??", fAT_log,},
#endif
      {"ATxx", fAT_exit,}
};
void at_log_init(void)
{
	log_service_add_table(at_log_items, sizeof(at_log_items)/sizeof(at_log_items[0]));
}

#if AT_LOG_INIT
log_module_init(at_log_init);
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "profiles/btstack_pbap_client.h"
#include "btstack_defines.h"
#include "btstack_run_loop.h"
#include "bluetooth.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

static int init_calls, connect_calls, flow_calls, pull_calls, next_calls;
static int abort_calls, disconnect_calls, auth_calls, goep_disconnect_calls, timer_add_calls;
static int native_call_sequence, last_abort_sequence, last_disconnect_sequence;
static int schedule_count, native_singleton_busy, goep_client_exists;
static uint8_t connect_result, flow_result, pbap_disconnect_status, goep_disconnect_status;
static int pbap_disconnect_sync_close, goep_disconnect_sync_close;
static uint16_t native_cid, next_native_cid = 0x1234u;
static btstack_packet_handler_t native_handler;
static char paths[64][32];
static const char *native_password;
static btstack_context_callback_registration_t *scheduled;
static btstack_timer_source_t *active_timer;
static btstack_pbap_client_event_t events[256];
static btstack_pbap_client_state_t xstack_cached_pbap_state;
static int event_count;
static const char *reject_name;
static int reject_count;
static btstack_pbap_client_event_type_t reentry_event;
static int reentry_action, reentry_result;
static const char *reentry_name;
static btstack_bd_addr_t reentry_address = {61,62,63,64,65,66};
static btstack_pbap_client_state_t reentry_observed_state;
static btstack_pbap_client_state_t state_reentry_state;
static btstack_pbap_client_state_t last_state_callback_state;
static int state_reentry_action, state_reentry_result;
static int callback_sequence, idle_state_callback_sequence;
static int completed_event_callback_sequence;

static void emit_simple(uint8_t subevent, uint8_t status);
static void on_state(btstack_pbap_client_state_t state);

static int event_state(btstack_pbap_client_event_type_t type,
                       btstack_pbap_client_state_t *state){
    switch(type){
    case BTSTACK_PBAP_CLIENT_EVENT_DISCONNECTED:*state=BTSTACK_PBAP_CLIENT_STATE_IDLE;return 1;
    case BTSTACK_PBAP_CLIENT_EVENT_CONNECTING:*state=BTSTACK_PBAP_CLIENT_STATE_CONNECTING;return 1;
    case BTSTACK_PBAP_CLIENT_EVENT_AUTHENTICATING:*state=BTSTACK_PBAP_CLIENT_STATE_AUTHENTICATING;return 1;
    case BTSTACK_PBAP_CLIENT_EVENT_PULLING:*state=BTSTACK_PBAP_CLIENT_STATE_PULLING;return 1;
    case BTSTACK_PBAP_CLIENT_EVENT_PAUSED:*state=BTSTACK_PBAP_CLIENT_STATE_PAUSED;return 1;
    case BTSTACK_PBAP_CLIENT_EVENT_DISCONNECTING:*state=BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING;return 1;
    default:return 0;
    }
}

void pbap_client_init(void){ init_calls++; }
uint8_t pbap_connect(btstack_packet_handler_t h, bd_addr_t a, uint16_t *cid){ (void)a; connect_calls++; if(native_singleton_busy)return BTSTACK_BUSY; if(connect_result)return connect_result; native_singleton_busy=1; goep_client_exists=1; native_cid=next_native_cid++; native_handler=h; *cid=native_cid; return 0; }
uint8_t pbap_set_flow_control_mode(uint16_t cid, int enable){ CHECK(cid==native_cid && enable==1); flow_calls++; return flow_result; }
uint8_t pbap_pull_phonebook(uint16_t cid, const char *path){ CHECK(cid==native_cid); CHECK(pull_calls < 64); strncpy(paths[pull_calls++], path, 31); return 0; }
uint8_t pbap_next_packet(uint16_t cid){ CHECK(cid==native_cid); next_calls++; return 0; }
uint8_t pbap_abort(uint16_t cid){ CHECK(cid==native_cid); abort_calls++; last_abort_sequence=++native_call_sequence; return 0; }
uint8_t pbap_disconnect(uint16_t cid){ uint8_t status; CHECK(cid==native_cid); disconnect_calls++; last_disconnect_sequence=++native_call_sequence; status=pbap_disconnect_status; if(status==ERROR_CODE_SUCCESS && pbap_disconnect_sync_close) emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0); return status; }
uint8_t pbap_authentication_password(uint16_t cid, const char *p){ CHECK(cid==native_cid); auth_calls++; native_password=p; return 0; }
uint8_t goep_client_disconnect(uint16_t cid){ uint8_t status; CHECK(cid==native_cid); if(!goep_client_exists)return 2u; goep_disconnect_calls++; status=goep_disconnect_status; if(status==ERROR_CODE_SUCCESS && goep_disconnect_sync_close) emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0); return status; }
void btstack_run_loop_set_timer_handler(btstack_timer_source_t *t, void (*p)(btstack_timer_source_t *)){ t->process=p; }
void btstack_run_loop_set_timer(btstack_timer_source_t *t, uint32_t ms){ t->timeout=ms; }
void btstack_run_loop_add_timer(btstack_timer_source_t *t){ active_timer=t; timer_add_calls++; }
int btstack_run_loop_remove_timer(btstack_timer_source_t *t){ if(active_timer==t) active_timer=NULL; return 1; }
void btstack_run_loop_execute_on_main_thread(btstack_context_callback_registration_t *r){ schedule_count++; scheduled=r; }

static uint8_t on_event(const btstack_pbap_client_event_t *event){
    btstack_pbap_client_state_t state;
    if(event_state(event->type,&state)){
        on_state(state);
        return 1;
    }
    int sequence=++callback_sequence;
    xstack_cached_pbap_state = btstack_pbap_client_get_state();
    if(event->type==BTSTACK_PBAP_CLIENT_EVENT_COMPLETED) completed_event_callback_sequence=sequence;
    CHECK(event_count < 256);
    events[event_count++] = *event;
    if (reentry_action != 0 && event->type == reentry_event &&
        (reentry_name == NULL || strcmp(event->record.name,reentry_name)==0)) {
        int action = reentry_action;
        reentry_action = 0;
        reentry_name = NULL;
        if (action == 1 || action == 2) {
            reentry_result = btstack_pbap_client_cancel();
            if (action == 2) return 0;
        } else if (action == 3) {
            reentry_result = btstack_pbap_client_authenticate("inline");
        } else if (action == 4) {
            reentry_result = btstack_pbap_client_sync(
                reentry_address, BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL);
        } else if (action == 5) {
            reentry_observed_state = btstack_pbap_client_get_state();
            reentry_result = btstack_pbap_client_sync(
                reentry_address, BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS);
        }
    }
    if ((event->type == BTSTACK_PBAP_CLIENT_EVENT_CONTACT || event->type == BTSTACK_PBAP_CLIENT_EVENT_CALL) &&
        reject_name && strcmp(event->record.name, reject_name)==0 && reject_count>0){ reject_count--; return 0; }
    return 1;
}
static void on_state(btstack_pbap_client_state_t state){
    int sequence=++callback_sequence;
    xstack_cached_pbap_state=state;
    last_state_callback_state=state;
    if(state==BTSTACK_PBAP_CLIENT_STATE_IDLE) idle_state_callback_sequence=sequence;
    if(state_reentry_action!=0 && state==state_reentry_state){
        state_reentry_action=0;
        state_reentry_result=btstack_pbap_client_cancel();
    }
}

static void emit_open_cid(uint8_t status, const btstack_bd_addr_t addr, uint16_t cid, uint16_t channel){
    uint8_t e[15]={HCI_EVENT_PBAP_META,13,PBAP_SUBEVENT_CONNECTION_OPENED,0,0,status}; unsigned i;
    e[3]=(uint8_t)cid; e[4]=(uint8_t)(cid>>8);
    for(i=0;i<6;i++) {
        e[6+i]=addr[5-i];
    }
    if(status!=ERROR_CODE_SUCCESS && status!=OBEX_CONNECT_FAILED && cid==native_cid && channel==native_cid){ native_singleton_busy=0; goep_client_exists=0; }
    native_handler(HCI_EVENT_PACKET,channel,e,sizeof(e));
}
static void emit_open(uint8_t status, const btstack_bd_addr_t addr){ emit_open_cid(status,addr,native_cid,native_cid); }
static void emit_simple_cid(uint8_t subevent, uint8_t status, uint16_t cid, uint16_t channel, uint16_t size){
    uint8_t e[15]={HCI_EVENT_PBAP_META,4,subevent,0,0,status,0};
    e[3]=(uint8_t)cid; e[4]=(uint8_t)(cid>>8);
    if(subevent==PBAP_SUBEVENT_CONNECTION_CLOSED && cid==native_cid){ native_singleton_busy=0; goep_client_exists=0; }
    native_handler(HCI_EVENT_PACKET,channel,e,size);
}
static void emit_simple(uint8_t subevent, uint8_t status){ emit_simple_cid(subevent,status,native_cid,native_cid,subevent==PBAP_SUBEVENT_AUTHENTICATION_REQUEST ? 7u : (subevent==PBAP_SUBEVENT_OPERATION_COMPLETED ? 6u : 5u)); }
static void emit_truncated(uint8_t subevent, uint16_t size){ emit_simple_cid(subevent,0,native_cid,native_cid,size); }
static void emit_data_cid(const char *text, uint16_t cid){ native_handler(PBAP_DATA_PACKET,cid,(uint8_t *)(uintptr_t)text,(uint16_t)strlen(text)); }
static void emit_data(const char *text){ emit_data_cid(text,native_cid); }
static int count_record_name(const char *name){ int i,n=0; for(i=0;i<event_count;i++) if((events[i].type==BTSTACK_PBAP_CLIENT_EVENT_CONTACT || events[i].type==BTSTACK_PBAP_CLIENT_EVENT_CALL) && strcmp(events[i].record.name,name)==0)n++; return n; }
static int count_event_type(btstack_pbap_client_event_type_t type){ int i,n=0; for(i=0;i<event_count;i++) if(events[i].type==type)n++; return n; }
static int count_category_completion(btstack_pbap_client_category_t category){ int i,n=0; for(i=0;i<event_count;i++) if(events[i].type==BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED && events[i].category==category)n++; return n; }
static int count_category_status(btstack_pbap_client_category_t category, btstack_pbap_client_status_t status){ int i,n=0; for(i=0;i<event_count;i++) if(events[i].type==BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED && events[i].category==category && events[i].status==status)n++; return n; }
static int count_final_status(btstack_pbap_client_status_t status){ int i,n=0; for(i=0;i<event_count;i++) if((events[i].type==BTSTACK_PBAP_CLIENT_EVENT_COMPLETED || events[i].type==BTSTACK_PBAP_CLIENT_EVENT_ERROR) && events[i].status==status)n++; return n; }
static const btstack_pbap_client_event_t *last_event(void){ CHECK(event_count>0); return &events[event_count-1]; }
static void begin_sync_mask(const btstack_bd_addr_t addr, btstack_pbap_client_category_mask_t mask){ CHECK(btstack_pbap_client_sync(addr,mask)==0); emit_open(0,addr); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_PULLING); }
static void begin_sync(const btstack_bd_addr_t addr){ begin_sync_mask(addr,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL); }
static void complete_remaining(void){ while(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_PULLING) emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,0); if(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING) emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0); }
static void append_text(char *buffer, size_t capacity, size_t *length, const char *text){ size_t n=strlen(text); CHECK(*length+n<capacity); memcpy(buffer+*length,text,n+1u); *length+=n; }
static void release_failed_native_cleanup(void){ native_singleton_busy=0; goep_client_exists=0; pbap_disconnect_status=ERROR_CODE_SUCCESS; goep_disconnect_status=ERROR_CODE_SUCCESS; }

static void test_setup_sync_and_cancel(void){
    btstack_bd_addr_t a={1,2,3,4,5,6};
    CHECK(btstack_pbap_client_setup()==0); CHECK(btstack_pbap_client_setup()==0); CHECK(init_calls==1);
    CHECK(btstack_pbap_client_set_event_callback(on_event)==0);
    CHECK(btstack_pbap_client_sync(NULL,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==BTSTACK_STATUS_INVALID_ARGUMENT);
    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==0); CHECK(connect_calls==1); CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==BTSTACK_STATUS_BUSY);
    emit_open(0,a); CHECK(flow_calls==1); CHECK(pull_calls==1); CHECK(strcmp(paths[0],"telecom/pb.vcf")==0);
    CHECK(btstack_pbap_client_cancel()==0); CHECK(abort_calls==1 && disconnect_calls==1);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0); CHECK(last_event()->status==BTSTACK_PBAP_CLIENT_STATUS_CANCELED);
}

static void test_invalid_category_masks_precede_busy(void){
    btstack_bd_addr_t a={2,3,4,5,6,7};
    int before_connect=connect_calls, before_events=event_count;
    btstack_pbap_client_category_mask_t unknown=(btstack_pbap_client_category_mask_t)0x80u;

    CHECK(btstack_pbap_client_sync(a,0u)==BTSTACK_STATUS_INVALID_ARGUMENT);
    CHECK(btstack_pbap_client_sync(a,unknown)==BTSTACK_STATUS_INVALID_ARGUMENT);
    CHECK(btstack_pbap_client_sync(a,(btstack_pbap_client_category_mask_t)(BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS|unknown))==BTSTACK_STATUS_INVALID_ARGUMENT);
    CHECK(connect_calls==before_connect && event_count==before_events);

    begin_sync(a); before_connect=connect_calls; before_events=event_count;
    CHECK(btstack_pbap_client_sync(a,0u)==BTSTACK_STATUS_INVALID_ARGUMENT);
    CHECK(btstack_pbap_client_sync(a,unknown)==BTSTACK_STATUS_INVALID_ARGUMENT);
    CHECK(connect_calls==before_connect && event_count==before_events);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_PULLING);
    CHECK(btstack_pbap_client_cancel()==BTSTACK_STATUS_OK);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
}

static void test_subset_contacts_and_missed_only(void){
    btstack_bd_addr_t a={3,4,5,6,7,8};
    int base=pull_calls, before_connect=connect_calls;
    int before_contacts=count_category_completion(BTSTACK_PBAP_CLIENT_CATEGORY_CONTACTS);
    int before_incoming=count_category_completion(BTSTACK_PBAP_CLIENT_CATEGORY_INCOMING);
    int before_outgoing=count_category_completion(BTSTACK_PBAP_CLIENT_CATEGORY_OUTGOING);
    int before_missed=count_category_completion(BTSTACK_PBAP_CLIENT_CATEGORY_MISSED);
    btstack_pbap_client_category_mask_t mask=(btstack_pbap_client_category_mask_t)(
        BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS|
        BTSTACK_PBAP_CLIENT_CATEGORY_MASK_MISSED);

    begin_sync_mask(a,mask);
    CHECK(connect_calls==before_connect+1);
    CHECK(pull_calls==base+1 && strcmp(paths[base],"telecom/pb.vcf")==0);
    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,OBEX_NOT_FOUND);
    CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED);
    CHECK(last_event()->category==BTSTACK_PBAP_CLIENT_CATEGORY_CONTACTS);
    CHECK(last_event()->status==BTSTACK_PBAP_CLIENT_STATUS_UNSUPPORTED);
    CHECK(pull_calls==base+2 && strcmp(paths[base+1],"telecom/mch.vcf")==0);
    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,0);
    CHECK(pull_calls==base+2);
    CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED);
    CHECK(last_event()->category==BTSTACK_PBAP_CLIENT_CATEGORY_MISSED);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING);
    CHECK(count_category_completion(BTSTACK_PBAP_CLIENT_CATEGORY_CONTACTS)==before_contacts+1);
    CHECK(count_category_completion(BTSTACK_PBAP_CLIENT_CATEGORY_INCOMING)==before_incoming);
    CHECK(count_category_completion(BTSTACK_PBAP_CLIENT_CATEGORY_OUTGOING)==before_outgoing);
    CHECK(count_category_completion(BTSTACK_PBAP_CLIENT_CATEGORY_MISSED)==before_missed+1);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
    CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_COMPLETED);
    CHECK(last_event()->status==BTSTACK_PBAP_CLIENT_STATUS_PARTIAL);
}

static void test_single_category_only(void){
    btstack_bd_addr_t a={4,5,6,7,8,9};
    int base=pull_calls, before_connect=connect_calls;
    int before_completed=count_event_type(BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED);

    begin_sync_mask(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_OUTGOING);
    CHECK(connect_calls==before_connect+1);
    CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_STARTED);
    CHECK(last_event()->category==BTSTACK_PBAP_CLIENT_CATEGORY_OUTGOING);
    CHECK(pull_calls==base+1 && strcmp(paths[base],"telecom/och.vcf")==0);
    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,0);
    CHECK(pull_calls==base+1);
    CHECK(count_event_type(BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED)==before_completed+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
    CHECK(events[event_count-2].type==BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED);
    CHECK(events[event_count-2].category==BTSTACK_PBAP_CLIENT_CATEGORY_OUTGOING);
    CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_COMPLETED);
    CHECK(last_event()->status==BTSTACK_PBAP_CLIENT_STATUS_SUCCESS);
}

static void test_fatal_operation_statuses_stop_categories_and_cleanup(void){
    btstack_bd_addr_t a={4,6,8,10,12,14};
    btstack_pbap_client_category_mask_t mask=(btstack_pbap_client_category_mask_t)(
        BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS|
        BTSTACK_PBAP_CLIENT_CATEGORY_MASK_MISSED);
    const uint8_t native_statuses[]={
        ERROR_CODE_AUTHENTICATION_FAILURE,
        ERROR_CODE_CONNECTION_REJECTED_DUE_TO_SECURITY_REASONS,
        OBEX_UNKNOWN_ERROR
    };
    const btstack_pbap_client_status_t expected_statuses[]={
        BTSTACK_PBAP_CLIENT_STATUS_DENIED,
        BTSTACK_PBAP_CLIENT_STATUS_DENIED,
        BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR
    };
    unsigned i;

    for(i=0u;i<sizeof(native_statuses)/sizeof(native_statuses[0]);++i){
        int base_pull=pull_calls;
        int before_category=count_category_status(
            BTSTACK_PBAP_CLIENT_CATEGORY_CONTACTS,expected_statuses[i]);
        int before_final=count_final_status(expected_statuses[i]);
        int before_disconnect=disconnect_calls;

        begin_sync_mask(a,mask);
        CHECK(pull_calls==base_pull+1);
        emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,native_statuses[i]);
        CHECK(count_category_status(BTSTACK_PBAP_CLIENT_CATEGORY_CONTACTS,
                                    expected_statuses[i])==before_category+1);
        CHECK(pull_calls==base_pull+1);
        CHECK(disconnect_calls==before_disconnect+1);
        CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING);
        CHECK(count_final_status(expected_statuses[i])==before_final);

        emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
        CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_ERROR);
        CHECK(last_event()->status==expected_statuses[i]);
        CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
        CHECK(count_final_status(expected_statuses[i])==before_final+1);
        emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
        CHECK(count_final_status(expected_statuses[i])==before_final+1);
    }
}

static void test_fatal_category_callback_cannot_cancel_or_replace_error(void){
    btstack_bd_addr_t a={4,7,10,13,16,19};
    const uint8_t native_statuses[]={
        ERROR_CODE_AUTHENTICATION_FAILURE,
        OBEX_UNKNOWN_ERROR
    };
    const btstack_pbap_client_status_t expected_statuses[]={
        BTSTACK_PBAP_CLIENT_STATUS_DENIED,
        BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR
    };
    unsigned i;

    for(i=0u;i<sizeof(native_statuses)/sizeof(native_statuses[0]);++i){
        int before_events;
        int before_final=count_final_status(expected_statuses[i]);
        int before_canceled=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED);

        begin_sync_mask(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS);
        before_events=event_count;
        reentry_event=BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED;
        reentry_action=1;
        reentry_result=BTSTACK_STATUS_OK;
        emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,native_statuses[i]);
        CHECK(reentry_result==BTSTACK_STATUS_NOT_READY);
        CHECK(event_count==before_events+1);
        CHECK(events[before_events].type==BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED);
        CHECK(events[before_events].status==expected_statuses[i]);
        CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING);
        CHECK(count_final_status(expected_statuses[i])==before_final);

        emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
        CHECK(event_count==before_events+2);
        CHECK(events[before_events+1].type==BTSTACK_PBAP_CLIENT_EVENT_ERROR);
        CHECK(events[before_events+1].status==expected_statuses[i]);
        CHECK(count_final_status(expected_statuses[i])==before_final+1);
        CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED)==before_canceled);
        CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    }
}

static void test_all_categories_order_and_one_connection(void){
    btstack_bd_addr_t a={5,6,7,8,9,10};
    int base=pull_calls, before_connect=connect_calls;

    begin_sync_mask(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL);
    CHECK(connect_calls==before_connect+1);
    CHECK(pull_calls==base+1 && strcmp(paths[base],"telecom/pb.vcf")==0);
    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,0);
    CHECK(strcmp(paths[base+1],"telecom/ich.vcf")==0);
    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,0);
    CHECK(strcmp(paths[base+2],"telecom/och.vcf")==0);
    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,0);
    CHECK(strcmp(paths[base+3],"telecom/mch.vcf")==0);
    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,0);
    CHECK(pull_calls==base+4 && connect_calls==before_connect+1);
    CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
    CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_COMPLETED);
    CHECK(last_event()->status==BTSTACK_PBAP_CLIENT_STATUS_SUCCESS);
}

static void test_categories_parser_and_partial(void){
    btstack_bd_addr_t a={7,8,9,10,11,12}; int base=pull_calls;
    begin_sync(a);
    emit_data("BEGIN:VCARD\r\nFN:Alice\r\nTEL:111\r\nEND:VCARD\r\n"); CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_CONTACT); CHECK(next_calls==1);
    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,0); CHECK(strcmp(paths[base+1],"telecom/ich.vcf")==0);
    emit_data("BEGIN:VCARD\r\nFN:In\r\nTEL:222\r\nEND:VCARD\r\n"); CHECK(last_event()->record.direction==BTSTACK_VCARD_DIRECTION_INCOMING);
    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,OBEX_NOT_FOUND); CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED); CHECK(last_event()->status==BTSTACK_PBAP_CLIENT_STATUS_UNSUPPORTED); CHECK(strcmp(paths[base+2],"telecom/och.vcf")==0);
    emit_data("BEGIN:VCARD\r\nFN:Out\r\nTEL:333\r\nEND:VCARD\r\n"); CHECK(last_event()->record.direction==BTSTACK_VCARD_DIRECTION_OUTGOING);
    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,0); CHECK(strcmp(paths[base+3],"telecom/mch.vcf")==0);
    emit_data("BEGIN:VCARD\r\nFN:Miss\r\nTEL:444\r\nEND:VCARD\r\n"); CHECK(last_event()->record.direction==BTSTACK_VCARD_DIRECTION_MISSED);
    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,0); CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0); CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_COMPLETED); CHECK(last_event()->status==BTSTACK_PBAP_CLIENT_STATUS_PARTIAL); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE); CHECK(xstack_cached_pbap_state==BTSTACK_PBAP_CLIENT_STATE_IDLE);
}

static void test_busy_sync_reports_requested_peer_and_preserves_active_sync(void){
    btstack_bd_addr_t active={7,8,9,10,11,12};
    btstack_bd_addr_t rejected={61,62,63,64,65,66};
    int before_events;

    begin_sync(active);
    before_events=event_count;
    CHECK(btstack_pbap_client_sync(rejected,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==BTSTACK_STATUS_BUSY);
    CHECK(event_count==before_events+1);
    CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_ERROR);
    CHECK(last_event()->status==BTSTACK_PBAP_CLIENT_STATUS_BUSY);
    CHECK(memcmp(last_event()->peer,rejected,sizeof(rejected))==0);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_PULLING);

    emit_data("BEGIN:VCARD\r\nFN:Active Continues\r\nTEL:123\r\nEND:VCARD\r\n");
    CHECK(strcmp(last_event()->record.name,"Active Continues")==0);
    CHECK(memcmp(last_event()->peer,active,sizeof(active))==0);
    complete_remaining();
}

static void test_pause_replay_and_deferred_completion(void){
    btstack_bd_addr_t a={13,14,15,16,17,18}; int before_next=next_calls, before_pull, schedules=schedule_count;
    char boundary[1022]={0}; size_t boundary_length=0u;
    begin_sync(a); reject_name="One"; reject_count=2;
    emit_data("BEGIN:VCARD\r\nFN:One\r\nEND:VCARD\r\nBEGIN:VCARD\r\nFN:Two\r\nEND:VCARD\r\n");
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_PAUSED); CHECK(next_calls==before_next);
    btstack_pbap_client_event_space_available(); btstack_pbap_client_event_space_available(); CHECK(scheduled!=NULL); CHECK(schedule_count==schedules+1);
    { btstack_context_callback_registration_t *r=scheduled; scheduled=NULL; r->callback(r->context); }
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_PAUSED); CHECK(count_record_name("One")==2);
    btstack_pbap_client_event_space_available(); btstack_pbap_client_event_space_available(); CHECK(schedule_count==schedules+2);
    { btstack_context_callback_registration_t *r=scheduled; scheduled=NULL; r->callback(r->context); }
    CHECK(count_record_name("One")==3 && count_record_name("Two")==1); CHECK(next_calls==before_next+1);

    reject_name="Solo"; reject_count=1;
    emit_data("BEGIN:VCARD\r\nFN:Solo\r\nEND:VCARD\r\n");
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_PAUSED);
    btstack_pbap_client_event_space_available();
    { btstack_context_callback_registration_t *r=scheduled; scheduled=NULL; r->callback(r->context); }
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_PULLING);
    CHECK(xstack_cached_pbap_state==BTSTACK_PBAP_CLIENT_STATE_PULLING);

    append_text(boundary,sizeof(boundary),&boundary_length,"BEGIN:VCARD\r\nFN:Early\r\nEND:VCARD\r\nBEGIN:VCARD\r\nFN:Boundary\r\n");
    while(boundary_length+strlen("PHOTO:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\nEND:VCARD\r\n")<=1021u){
        append_text(boundary,sizeof(boundary),&boundary_length,"PHOTO:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\n");
    }
    append_text(boundary,sizeof(boundary),&boundary_length,"END:VCARD\r\n");
    CHECK(boundary_length>900u && boundary_length<=1021u); reject_name="Early"; reject_count=1;
    emit_data(boundary); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_PAUSED);
    btstack_pbap_client_event_space_available(); { btstack_context_callback_registration_t *r=scheduled; scheduled=NULL; r->callback(r->context); }
    CHECK(count_record_name("Early")==2 && count_record_name("Boundary")==1);

    reject_name="Hold"; reject_count=1; before_pull=pull_calls;
    emit_data("BEGIN:VCARD\r\nFN:Hold\r\nEND:VCARD\r\n"); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_PAUSED);
    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,0); CHECK(pull_calls==before_pull);
    btstack_pbap_client_event_space_available(); { btstack_context_callback_registration_t *r=scheduled; scheduled=NULL; r->callback(r->context); }
    CHECK(pull_calls==before_pull+1); reject_name=NULL; complete_remaining();
}

static void test_parser_fatal_and_replay_overflow_cleanup(void){
    btstack_bd_addr_t a={15,16,17,18,19,20};
    char overflow_packet[1200];
    const char *overflow_prefix="BEGIN:VCARD\r\nFN:Overflow\r\nEND:VCARD\r\n";
    size_t prefix_length=strlen(overflow_prefix);
    int before_disconnect, before_final, before_category;

    begin_sync_mask(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS);
    before_disconnect=disconnect_calls;
    before_final=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR);
    before_category=count_category_status(BTSTACK_PBAP_CLIENT_CATEGORY_CONTACTS,
                                          BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR);
    emit_data("BEGIN:VCARD\r\nFN:Unfinished");
    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,ERROR_CODE_SUCCESS);
    CHECK(count_category_status(BTSTACK_PBAP_CLIENT_CATEGORY_CONTACTS,
                                BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR)==before_category+1);
    CHECK(disconnect_calls==before_disconnect+1);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR)==before_final);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
    CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_ERROR);
    CHECK(last_event()->status==BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);

    begin_sync_mask(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS);
    before_disconnect=disconnect_calls;
    before_final=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR);
    memcpy(overflow_packet,overflow_prefix,prefix_length);
    memset(overflow_packet+prefix_length,'X',sizeof(overflow_packet)-prefix_length-1u);
    overflow_packet[sizeof(overflow_packet)-1u]='\0';
    reject_name="Overflow";
    reject_count=1;
    emit_data(overflow_packet);
    reject_name=NULL;
    CHECK(disconnect_calls==before_disconnect+1);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR)==before_final);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
    CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_ERROR);
    CHECK(last_event()->status==BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);

    begin_sync_mask(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS);
    CHECK(btstack_pbap_client_cancel()==BTSTACK_STATUS_OK);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
}

static void test_terminal_cleanup_failure_paths_do_not_stick_busy(void){
    btstack_bd_addr_t a={19,20,21,22,23,24};
    int before_abort, before_disconnect, before_goep, before_final;

    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS)==BTSTACK_STATUS_OK);
    emit_simple(PBAP_SUBEVENT_AUTHENTICATION_REQUEST,0);
    before_goep=goep_disconnect_calls;
    before_final=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_AUTH_TIMEOUT);
    goep_disconnect_status=BTSTACK_BUSY;
    { btstack_timer_source_t *timer=active_timer; timer->process(timer); }
    CHECK(goep_disconnect_calls==before_goep+1);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_AUTH_TIMEOUT)==before_final+1);
    CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_COMPLETED);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    release_failed_native_cleanup();

    begin_sync_mask(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS);
    before_abort=abort_calls; before_disconnect=disconnect_calls;
    before_goep=goep_disconnect_calls;
    before_final=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED);
    pbap_disconnect_status=BTSTACK_BUSY; goep_disconnect_status=BTSTACK_BUSY;
    CHECK(btstack_pbap_client_cancel()==BTSTACK_STATUS_OK);
    CHECK(abort_calls==before_abort+1 && disconnect_calls==before_disconnect+1);
    CHECK(last_abort_sequence<last_disconnect_sequence);
    CHECK(goep_disconnect_calls==before_goep+1);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED)==before_final+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    release_failed_native_cleanup();

    begin_sync_mask(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS);
    reject_name="Paused Cleanup"; reject_count=1;
    emit_data("BEGIN:VCARD\r\nFN:Paused Cleanup\r\nEND:VCARD\r\n");
    reject_name=NULL;
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_PAUSED);
    before_abort=abort_calls; before_disconnect=disconnect_calls;
    before_goep=goep_disconnect_calls;
    before_final=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED);
    pbap_disconnect_status=BTSTACK_BUSY; goep_disconnect_status=BTSTACK_BUSY;
    CHECK(btstack_pbap_client_cancel()==BTSTACK_STATUS_OK);
    CHECK(abort_calls==before_abort+1 && disconnect_calls==before_disconnect+1);
    CHECK(last_abort_sequence<last_disconnect_sequence);
    CHECK(goep_disconnect_calls==before_goep+1);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED)==before_final+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    release_failed_native_cleanup();

    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS)==BTSTACK_STATUS_OK);
    CHECK(btstack_pbap_client_cancel()==BTSTACK_STATUS_OK);
    before_disconnect=disconnect_calls; before_goep=goep_disconnect_calls;
    before_final=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED);
    pbap_disconnect_status=BTSTACK_BUSY; goep_disconnect_status=BTSTACK_BUSY;
    emit_open(ERROR_CODE_SUCCESS,a);
    CHECK(disconnect_calls==before_disconnect+1);
    CHECK(goep_disconnect_calls==before_goep+1);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED)==before_final+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    release_failed_native_cleanup();

    begin_sync_mask(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS);
    before_disconnect=disconnect_calls; before_goep=goep_disconnect_calls;
    before_final=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST);
    pbap_disconnect_status=BTSTACK_BUSY; goep_disconnect_status=BTSTACK_BUSY;
    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,OBEX_DISCONNECTED);
    CHECK(disconnect_calls==before_disconnect+1);
    CHECK(goep_disconnect_calls==before_goep+1);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST)==before_final+1);
    CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_COMPLETED);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    release_failed_native_cleanup();

    begin_sync_mask(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS);
    before_abort=abort_calls; before_disconnect=disconnect_calls;
    before_goep=goep_disconnect_calls;
    before_final=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST);
    pbap_disconnect_status=BTSTACK_BUSY; goep_disconnect_status=BTSTACK_BUSY;
    btstack_pbap_client_peer_disconnected(a);
    CHECK(abort_calls==before_abort+1 && disconnect_calls==before_disconnect+1);
    CHECK(last_abort_sequence<last_disconnect_sequence);
    CHECK(goep_disconnect_calls==before_goep+1);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST)==before_final+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    release_failed_native_cleanup();

    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS)==BTSTACK_STATUS_OK);
    before_goep=goep_disconnect_calls;
    before_final=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST);
    goep_disconnect_status=BTSTACK_BUSY;
    btstack_pbap_client_peer_disconnected(a);
    CHECK(goep_disconnect_calls==before_goep+1);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST)==before_final+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    release_failed_native_cleanup();
}

static void test_late_open_success_rebinds_connected_cleanup(void){
    btstack_bd_addr_t a={20,22,24,26,28,30};
    int before_disconnect, before_goep, before_final;

    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS)==BTSTACK_STATUS_OK);
    before_goep=goep_disconnect_calls;
    before_final=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST);
    btstack_pbap_client_peer_disconnected(a);
    CHECK(goep_disconnect_calls==before_goep+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST)==before_final);

    before_disconnect=disconnect_calls;
    before_goep=goep_disconnect_calls;
    pbap_disconnect_status=BTSTACK_BUSY;
    emit_open(ERROR_CODE_SUCCESS,a);
    CHECK(disconnect_calls==before_disconnect+1);
    CHECK(goep_disconnect_calls==before_goep+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST)==before_final);

    pbap_disconnect_status=ERROR_CODE_SUCCESS;
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
    CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_COMPLETED);
    CHECK(last_event()->status==BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST)==before_final+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST)==before_final+1);

    begin_sync_mask(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS);
    CHECK(btstack_pbap_client_cancel()==BTSTACK_STATUS_OK);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
}

static void test_late_open_success_double_cleanup_failure_finishes(void){
    btstack_bd_addr_t a={21,23,25,27,29,31};
    int before_disconnect, before_goep, before_final;

    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS)==BTSTACK_STATUS_OK);
    emit_simple(PBAP_SUBEVENT_AUTHENTICATION_REQUEST,0);
    before_goep=goep_disconnect_calls;
    before_final=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_AUTH_TIMEOUT);
    { btstack_timer_source_t *timer=active_timer; timer->process(timer); }
    CHECK(goep_disconnect_calls==before_goep+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_AUTH_TIMEOUT)==before_final);

    before_disconnect=disconnect_calls;
    before_goep=goep_disconnect_calls;
    pbap_disconnect_status=BTSTACK_BUSY;
    goep_disconnect_status=BTSTACK_BUSY;
    emit_open(ERROR_CODE_SUCCESS,a);
    CHECK(disconnect_calls==before_disconnect+1);
    CHECK(goep_disconnect_calls==before_goep+1);
    CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_COMPLETED);
    CHECK(last_event()->status==BTSTACK_PBAP_CLIENT_STATUS_AUTH_TIMEOUT);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_AUTH_TIMEOUT)==before_final+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    release_failed_native_cleanup();
}

static void test_late_open_failure_finishes_saved_terminal_status(void){
    btstack_bd_addr_t a={22,24,26,28,30,32};
    int before_final;

    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS)==BTSTACK_STATUS_OK);
    emit_simple(PBAP_SUBEVENT_AUTHENTICATION_REQUEST,0);
    before_final=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_AUTH_TIMEOUT);
    { btstack_timer_source_t *timer=active_timer; timer->process(timer); }
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_AUTH_TIMEOUT)==before_final);

    emit_open(ERROR_CODE_COMMAND_DISALLOWED,a);
    CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_COMPLETED);
    CHECK(last_event()->status==BTSTACK_PBAP_CLIENT_STATUS_AUTH_TIMEOUT);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_AUTH_TIMEOUT)==before_final+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_AUTH_TIMEOUT)==before_final+1);

    begin_sync_mask(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS);
    CHECK(btstack_pbap_client_cancel()==BTSTACK_STATUS_OK);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
}

static void test_auth_timeout_password_and_loss(void){
    btstack_bd_addr_t a={21,22,23,24,25,26}; char password[16]="secret"; int finals, before_auth, before_goep, auth_events;
    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==0); emit_simple(PBAP_SUBEVENT_AUTHENTICATION_REQUEST,0);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_AUTHENTICATING); CHECK(active_timer && active_timer->timeout==30000u);
    CHECK(btstack_pbap_client_authenticate(password)==0); password[0]='X'; CHECK(strcmp(native_password,"secret")==0 && auth_calls==1);
    emit_open(0,a); CHECK(btstack_pbap_client_cancel()==0); emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==0); emit_simple(PBAP_SUBEVENT_AUTHENTICATION_REQUEST,0); finals=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_AUTH_TIMEOUT); before_auth=auth_calls; before_goep=goep_disconnect_calls; auth_events=count_event_type(BTSTACK_PBAP_CLIENT_EVENT_AUTHENTICATION_REQUIRED);
    { btstack_timer_source_t *timer=active_timer; timer->process(timer); }
    CHECK(auth_calls==before_auth); CHECK(goep_disconnect_calls==before_goep+1); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING); CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==BTSTACK_STATUS_BUSY); CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_AUTH_TIMEOUT)==finals);
    emit_simple(PBAP_SUBEVENT_AUTHENTICATION_REQUEST,0); emit_simple(PBAP_SUBEVENT_AUTHENTICATION_REQUEST,0); CHECK(goep_disconnect_calls==before_goep+3); CHECK(count_event_type(BTSTACK_PBAP_CLIENT_EVENT_AUTHENTICATION_REQUIRED)==auth_events); CHECK(auth_calls==before_auth);
    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,OBEX_DISCONNECTED); CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_AUTH_TIMEOUT)==finals); emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0); CHECK(last_event()->status==BTSTACK_PBAP_CLIENT_STATUS_AUTH_TIMEOUT); CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_AUTH_TIMEOUT)==finals+1); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);

    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==0); emit_simple(PBAP_SUBEVENT_AUTHENTICATION_REQUEST,0); finals=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED); before_auth=auth_calls; before_goep=goep_disconnect_calls; auth_events=count_event_type(BTSTACK_PBAP_CLIENT_EVENT_AUTHENTICATION_REQUIRED);
    CHECK(btstack_pbap_client_cancel()==0); CHECK(auth_calls==before_auth); CHECK(goep_disconnect_calls==before_goep+1); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING); CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==BTSTACK_STATUS_BUSY); CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED)==finals);
    emit_simple(PBAP_SUBEVENT_AUTHENTICATION_REQUEST,0); CHECK(goep_disconnect_calls==before_goep+2); CHECK(count_event_type(BTSTACK_PBAP_CLIENT_EVENT_AUTHENTICATION_REQUIRED)==auth_events); emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,OBEX_DISCONNECTED); emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0); CHECK(last_event()->status==BTSTACK_PBAP_CLIENT_STATUS_CANCELED); CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED)==finals+1); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
}

static void test_disconnected_operation_and_open_failures(void){
    btstack_bd_addr_t a={31,32,33,34,35,36}; int before_pull, finals, before_goep, before_disconnect;
    begin_sync(a); before_pull=pull_calls; finals=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST);
    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,OBEX_DISCONNECTED); CHECK(pull_calls==before_pull); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING); CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST)==finals);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE); CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST)==finals+1); emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0); CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST)==finals+1);

    before_goep=goep_disconnect_calls; finals=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_FAILED);
    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==0); emit_open(1,a); CHECK(goep_disconnect_calls==before_goep); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE); CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_FAILED)==finals+1); CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_ERROR); CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==0); emit_open(0,a); CHECK(btstack_pbap_client_cancel()==0); emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
    before_goep=goep_disconnect_calls; finals=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_DENIED);
    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==0); emit_open(ERROR_CODE_CONNECTION_REJECTED_DUE_TO_SECURITY_REASONS,a); CHECK(goep_disconnect_calls==before_goep); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE); CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_DENIED)==finals+1);

    before_goep=goep_disconnect_calls; finals=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_FAILED);
    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==0); emit_open(OBEX_CONNECT_FAILED,a); CHECK(goep_disconnect_calls==before_goep+1); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING); CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==BTSTACK_STATUS_BUSY); CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_FAILED)==finals); emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0); CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_ERROR); CHECK(last_event()->status==BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_FAILED); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);

    flow_result=1u; before_disconnect=disconnect_calls; finals=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR);
    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==0); emit_open(0,a); CHECK(disconnect_calls==before_disconnect+1); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING); CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR)==finals); flow_result=0u; emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0); CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_ERROR); CHECK(last_event()->status==BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR);
}

static void test_peer_loss(void){
    btstack_bd_addr_t a={41,42,43,44,45,46}; btstack_bd_addr_t other={46,45,44,43,42,41}; int finals=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST); int before_abort=abort_calls, before_disconnect=disconnect_calls;
    btstack_pbap_client_peer_disconnected(a); CHECK(abort_calls==before_abort && disconnect_calls==before_disconnect); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    begin_sync(a); btstack_pbap_client_peer_disconnected(other); CHECK(abort_calls==before_abort && disconnect_calls==before_disconnect); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_PULLING);
    btstack_pbap_client_peer_disconnected(a); CHECK(abort_calls==before_abort+1 && disconnect_calls==before_disconnect+1); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING); CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST)==finals);
    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,OBEX_DISCONNECTED); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING); CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST)==finals);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE); CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST)==finals+1);
    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,OBEX_DISCONNECTED); emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,0); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE); CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST)==finals+1);
    begin_sync(a); CHECK(btstack_pbap_client_cancel()==0); emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
}

static void test_callback_reentry(void){
    btstack_bd_addr_t a={51,52,53,54,55,56}; int before_pull, before_next, before_timer, before_auth, before_disconnect;

    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==0); before_pull=pull_calls; reentry_event=BTSTACK_PBAP_CLIENT_EVENT_STARTED; reentry_action=1; emit_open(0,a);
    CHECK(reentry_result==BTSTACK_STATUS_OK); CHECK(pull_calls==before_pull); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING); emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);

    begin_sync(a); before_next=next_calls; reentry_event=BTSTACK_PBAP_CLIENT_EVENT_CONTACT; reentry_action=2; emit_data("BEGIN:VCARD\r\nFN:Cancel Contact\r\nEND:VCARD\r\n");
    CHECK(reentry_result==BTSTACK_STATUS_OK); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING); CHECK(next_calls==before_next); CHECK(scheduled==NULL); emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);

    begin_sync(a); emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,0); before_next=next_calls; reentry_event=BTSTACK_PBAP_CLIENT_EVENT_CALL; reentry_action=2; emit_data("BEGIN:VCARD\r\nFN:Cancel Call\r\nEND:VCARD\r\n");
    CHECK(reentry_result==BTSTACK_STATUS_OK); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING); CHECK(next_calls==before_next); emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);

    begin_sync(a); before_pull=pull_calls; reentry_event=BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED; reentry_action=1; emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,0);
    CHECK(reentry_result==BTSTACK_STATUS_OK); CHECK(pull_calls==before_pull); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING); emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);

    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==0); before_timer=timer_add_calls; before_auth=auth_calls; reentry_event=BTSTACK_PBAP_CLIENT_EVENT_AUTHENTICATION_REQUIRED; reentry_action=3; emit_simple(PBAP_SUBEVENT_AUTHENTICATION_REQUEST,0);
    CHECK(reentry_result==BTSTACK_STATUS_OK); CHECK(auth_calls==before_auth+1); CHECK(timer_add_calls==before_timer+1); CHECK(active_timer==NULL); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_CONNECTING); emit_open(0,a); CHECK(btstack_pbap_client_cancel()==0); emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);

    before_disconnect=goep_disconnect_calls; CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==0); emit_open(OBEX_CONNECT_FAILED,a); CHECK(goep_disconnect_calls==before_disconnect+1); reentry_event=BTSTACK_PBAP_CLIENT_EVENT_ERROR; reentry_action=4; emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
    CHECK(reentry_result==BTSTACK_STATUS_OK); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_CONNECTING); emit_open(0,reentry_address); CHECK(btstack_pbap_client_cancel()==0); emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
}

static void test_cid_and_event_length_isolation(void){
    btstack_bd_addr_t a={71,72,73,74,75,76}; uint16_t old_cid, current_cid; int before_flow, before_pull, before_next, before_events;

    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==0); old_cid=native_cid; emit_open(1,a); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==0); current_cid=native_cid; CHECK(current_cid!=old_cid); before_flow=flow_calls; before_events=event_count;
    emit_simple_cid(PBAP_SUBEVENT_CONNECTION_CLOSED,0,old_cid,old_cid,5u); emit_open_cid(0,a,old_cid,old_cid); emit_open_cid(0,a,current_cid,old_cid);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_CONNECTING); CHECK(flow_calls==before_flow); CHECK(event_count==before_events);
    emit_truncated(PBAP_SUBEVENT_CONNECTION_OPENED,14u); emit_truncated(PBAP_SUBEVENT_CONNECTION_CLOSED,4u); emit_truncated(PBAP_SUBEVENT_OPERATION_COMPLETED,5u); emit_truncated(PBAP_SUBEVENT_AUTHENTICATION_REQUEST,6u);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_CONNECTING); CHECK(event_count==before_events);
    emit_open(0,a); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_PULLING); before_pull=pull_calls; before_next=next_calls;
    emit_simple_cid(PBAP_SUBEVENT_OPERATION_COMPLETED,0,old_cid,old_cid,6u); emit_data_cid("BEGIN:VCARD\r\nFN:Old\r\nEND:VCARD\r\n",old_cid);
    CHECK(pull_calls==before_pull); CHECK(next_calls==before_next); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_PULLING);
    CHECK(btstack_pbap_client_cancel()==0); emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
}

static void test_resume_replay_callback_cancel(void){
    btstack_bd_addr_t a={81,82,83,84,85,86}; int before_next;
    begin_sync(a); reject_name="Replay First"; reject_count=1; before_next=next_calls;
    emit_data("BEGIN:VCARD\r\nFN:Replay First\r\nEND:VCARD\r\nBEGIN:VCARD\r\nFN:Replay Cancel\r\nEND:VCARD\r\n");
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_PAUSED); reentry_event=BTSTACK_PBAP_CLIENT_EVENT_CONTACT; reentry_name="Replay Cancel"; reentry_action=1;
    btstack_pbap_client_event_space_available(); { btstack_context_callback_registration_t *r=scheduled; scheduled=NULL; r->callback(r->context); }
    CHECK(reentry_result==BTSTACK_STATUS_OK); CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING); CHECK(next_calls==before_next); emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0); reject_name=NULL;

    begin_sync(a); reject_name="Cancel Before Space"; reject_count=1;
    emit_data("BEGIN:VCARD\r\nFN:Cancel Before Space\r\nEND:VCARD\r\n");
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_PAUSED);
    CHECK(btstack_pbap_client_cancel()==BTSTACK_STATUS_OK);
    btstack_pbap_client_event_space_available(); CHECK(scheduled==NULL);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);

    begin_sync(a); reject_name="Disconnect Before Space"; reject_count=1;
    emit_data("BEGIN:VCARD\r\nFN:Disconnect Before Space\r\nEND:VCARD\r\n");
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_PAUSED);
    btstack_pbap_client_peer_disconnected(a);
    btstack_pbap_client_event_space_available(); CHECK(scheduled==NULL);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0); reject_name=NULL;
}

static void test_data_callback_cancel_stops_same_packet(void){
    btstack_bd_addr_t a={82,83,84,85,86,87}; int before_next;
    begin_sync(a); before_next=next_calls;
    reentry_event=BTSTACK_PBAP_CLIENT_EVENT_CONTACT;
    reentry_name="Packet Cancel A"; reentry_action=1;
    emit_data("BEGIN:VCARD\r\nFN:Packet Cancel A\r\nEND:VCARD\r\nBEGIN:VCARD\r\nFN:Packet Cancel B\r\nEND:VCARD\r\n");
    CHECK(reentry_result==BTSTACK_STATUS_OK);
    CHECK(count_record_name("Packet Cancel A")==1);
    CHECK(count_record_name("Packet Cancel B")==0);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING);
    CHECK(next_calls==before_next && scheduled==NULL);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
}

static void test_event_space_callback_cancel_discards_replay(void){
    btstack_bd_addr_t a={83,84,85,86,87,88};
    begin_sync(a); reject_name="Space Cancel A"; reject_count=1;
    emit_data("BEGIN:VCARD\r\nFN:Space Cancel A\r\nEND:VCARD\r\nBEGIN:VCARD\r\nFN:Space Cancel B\r\nEND:VCARD\r\n");
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_PAUSED);
    reentry_event=BTSTACK_PBAP_CLIENT_EVENT_CONTACT;
    reentry_name="Space Cancel A"; reentry_action=1;
    btstack_pbap_client_event_space_available();
    { btstack_context_callback_registration_t *r=scheduled; scheduled=NULL; r->callback(r->context); }
    CHECK(reentry_result==BTSTACK_STATUS_OK);
    CHECK(count_record_name("Space Cancel A")==2);
    CHECK(count_record_name("Space Cancel B")==0);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0); reject_name=NULL;
}

static void test_state_callback_cancel_discards_replay(void){
    btstack_bd_addr_t a={84,85,86,87,88,89};
    begin_sync(a); reject_name="State Cancel A"; reject_count=1;
    emit_data("BEGIN:VCARD\r\nFN:State Cancel A\r\nEND:VCARD\r\nBEGIN:VCARD\r\nFN:State Cancel B\r\nEND:VCARD\r\n");
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_PAUSED);
    reject_name=NULL;
    state_reentry_state=BTSTACK_PBAP_CLIENT_STATE_PULLING;
    state_reentry_action=1;
    btstack_pbap_client_event_space_available();
    { btstack_context_callback_registration_t *r=scheduled; scheduled=NULL; r->callback(r->context); }
    CHECK(state_reentry_result==BTSTACK_STATUS_OK);
    CHECK(count_record_name("State Cancel A")==2);
    CHECK(count_record_name("State Cancel B")==0);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
}

static void test_connecting_cancel_starts_cleanup_without_native_event(void){
    btstack_bd_addr_t a={85,86,87,88,89,90};
    int before_goep=goep_disconnect_calls;
    int before_finals=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED);

    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==BTSTACK_STATUS_OK);
    CHECK(btstack_pbap_client_cancel()==BTSTACK_STATUS_OK);
    CHECK(goep_disconnect_calls==before_goep+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED)==before_finals);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED)==before_finals+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
}

static void test_connecting_cancel_goep_failure_finishes_immediately(void){
    btstack_bd_addr_t a={86,87,88,89,90,91};
    int before_goep=goep_disconnect_calls;
    int before_finals=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED);

    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==BTSTACK_STATUS_OK);
    goep_disconnect_status=BTSTACK_BUSY;
    CHECK(btstack_pbap_client_cancel()==BTSTACK_STATUS_OK);
    CHECK(goep_disconnect_calls==before_goep+1);
    CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_COMPLETED);
    CHECK(last_event()->status==BTSTACK_PBAP_CLIENT_STATUS_CANCELED);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED)==before_finals+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    release_failed_native_cleanup();
}

static void test_connecting_cancel_synchronous_close_finishes_once(void){
    btstack_bd_addr_t a={87,88,89,90,91,92};
    int before_goep=goep_disconnect_calls;
    int before_finals=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED);

    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==BTSTACK_STATUS_OK);
    goep_disconnect_sync_close=1;
    CHECK(btstack_pbap_client_cancel()==BTSTACK_STATUS_OK);
    CHECK(goep_disconnect_calls==before_goep+1);
    CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_COMPLETED);
    CHECK(last_event()->status==BTSTACK_PBAP_CLIENT_STATUS_CANCELED);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED)==before_finals+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED)==before_finals+1);
    goep_disconnect_sync_close=0;
}

static void test_connecting_cancel_late_auth_restarts_cleanup(void){
    btstack_bd_addr_t a={88,89,90,91,92,93};
    int before_goep=goep_disconnect_calls;
    int before_finals=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED);
    int before_auth_events=count_event_type(
        BTSTACK_PBAP_CLIENT_EVENT_AUTHENTICATION_REQUIRED);
    int before_timers=timer_add_calls;

    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==BTSTACK_STATUS_OK);
    CHECK(btstack_pbap_client_cancel()==BTSTACK_STATUS_OK);
    CHECK(goep_disconnect_calls==before_goep+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING);

    emit_simple(PBAP_SUBEVENT_AUTHENTICATION_REQUEST,0);
    CHECK(goep_disconnect_calls==before_goep+2);
    CHECK(count_event_type(BTSTACK_PBAP_CLIENT_EVENT_AUTHENTICATION_REQUIRED)==before_auth_events);
    CHECK(timer_add_calls==before_timers);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED)==before_finals);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING);

    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED)==before_finals+1);
    CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_COMPLETED);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED)==before_finals+1);
}

static void test_connecting_cancel_late_auth_cleanup_failure_finishes(void){
    btstack_bd_addr_t a={89,90,91,92,93,94};
    int before_goep=goep_disconnect_calls;
    int before_finals=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED);

    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==BTSTACK_STATUS_OK);
    CHECK(btstack_pbap_client_cancel()==BTSTACK_STATUS_OK);
    CHECK(goep_disconnect_calls==before_goep+1);

    goep_disconnect_status=BTSTACK_BUSY;
    emit_simple(PBAP_SUBEVENT_AUTHENTICATION_REQUEST,0);
    CHECK(goep_disconnect_calls==before_goep+2);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED)==before_finals+1);
    CHECK(last_event()->type==BTSTACK_PBAP_CLIENT_EVENT_COMPLETED);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    release_failed_native_cleanup();
}

static void test_connecting_cancel_late_auth_synchronous_close_allows_resync(void){
    btstack_bd_addr_t a={90,91,92,93,94,95};
    int before_goep=goep_disconnect_calls;
    int before_finals=count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED);

    CHECK(btstack_pbap_client_sync(a,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL)==BTSTACK_STATUS_OK);
    CHECK(btstack_pbap_client_cancel()==BTSTACK_STATUS_OK);
    CHECK(goep_disconnect_calls==before_goep+1);

    reentry_event=BTSTACK_PBAP_CLIENT_EVENT_COMPLETED;
    reentry_action=5;
    reentry_result=BTSTACK_STATUS_ERROR;
    reentry_observed_state=BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING;
    goep_disconnect_sync_close=1;
    emit_simple(PBAP_SUBEVENT_AUTHENTICATION_REQUEST,0);
    CHECK(goep_disconnect_calls==before_goep+2);
    CHECK(count_final_status(BTSTACK_PBAP_CLIENT_STATUS_CANCELED)==before_finals+1);
    CHECK(reentry_observed_state==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    CHECK(reentry_result==BTSTACK_STATUS_OK);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_CONNECTING);

    goep_disconnect_sync_close=0;
    CHECK(btstack_pbap_client_cancel()==BTSTACK_STATUS_OK);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
}

static void test_normal_completion_waits_for_native_close_and_allows_inline_resync(void){
    btstack_bd_addr_t first_peer={91,92,93,94,95,96};
    uint16_t first_cid;
    int before_categories=count_event_type(BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED);
    int before_completed=count_event_type(BTSTACK_PBAP_CLIENT_EVENT_COMPLETED);
    int before_disconnect=disconnect_calls, before_connect=connect_calls;

    begin_sync_mask(first_peer,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS);
    first_cid=native_cid;
    reentry_event=BTSTACK_PBAP_CLIENT_EVENT_COMPLETED;
    reentry_action=5;
    reentry_result=BTSTACK_STATUS_ERROR;
    reentry_observed_state=BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING;
    last_state_callback_state=BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING;

    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,0);
    CHECK(count_event_type(BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED)==before_categories+1);
    CHECK(count_event_type(BTSTACK_PBAP_CLIENT_EVENT_COMPLETED)==before_completed);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING);
    CHECK(disconnect_calls==before_disconnect+1);

    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,0);
    CHECK(count_event_type(BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED)==before_categories+1);
    CHECK(count_event_type(BTSTACK_PBAP_CLIENT_EVENT_COMPLETED)==before_completed);
    CHECK(disconnect_calls==before_disconnect+1);

    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
    CHECK(count_event_type(BTSTACK_PBAP_CLIENT_EVENT_COMPLETED)==before_completed+1);
    CHECK(last_state_callback_state==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    CHECK(reentry_observed_state==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    CHECK(reentry_result==BTSTACK_STATUS_OK);
    CHECK(connect_calls==before_connect+2);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_CONNECTING);

    emit_simple_cid(PBAP_SUBEVENT_CONNECTION_CLOSED,0,first_cid,first_cid,5u);
    emit_simple_cid(PBAP_SUBEVENT_OPERATION_COMPLETED,0,first_cid,first_cid,6u);
    CHECK(count_event_type(BTSTACK_PBAP_CLIENT_EVENT_COMPLETED)==before_completed+1);
    CHECK(count_event_type(BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED)==before_categories+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_CONNECTING);

    emit_open(0,reentry_address);
    CHECK(btstack_pbap_client_cancel()==BTSTACK_STATUS_OK);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
}

static void test_normal_finish_falls_back_to_goep_with_synchronous_close(void){
    btstack_bd_addr_t peer={92,93,94,95,96,97};
    int before_disconnect=disconnect_calls, before_goep=goep_disconnect_calls;
    int before_completed=count_event_type(BTSTACK_PBAP_CLIENT_EVENT_COMPLETED);

    begin_sync_mask(peer,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS);
    pbap_disconnect_status=BTSTACK_BUSY;
    goep_disconnect_status=ERROR_CODE_SUCCESS;
    goep_disconnect_sync_close=1;
    idle_state_callback_sequence=0;
    completed_event_callback_sequence=0;

    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,0);
    CHECK(disconnect_calls==before_disconnect+1);
    CHECK(goep_disconnect_calls==before_goep+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    CHECK(count_event_type(BTSTACK_PBAP_CLIENT_EVENT_COMPLETED)==before_completed+1);
    CHECK(idle_state_callback_sequence>0);
    CHECK(completed_event_callback_sequence>idle_state_callback_sequence);

    pbap_disconnect_status=ERROR_CODE_SUCCESS;
    goep_disconnect_status=ERROR_CODE_SUCCESS;
    goep_disconnect_sync_close=0;
}

static void test_normal_finish_disconnect_failures_complete_without_close(void){
    btstack_bd_addr_t first_peer={93,94,95,96,97,98};
    btstack_bd_addr_t second_peer={94,95,96,97,98,99};
    uint16_t first_cid;
    int before_disconnect=disconnect_calls, before_goep=goep_disconnect_calls;
    int before_completed=count_event_type(BTSTACK_PBAP_CLIENT_EVENT_COMPLETED);
    int before_connect=connect_calls;

    begin_sync_mask(first_peer,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS);
    first_cid=native_cid;
    pbap_disconnect_status=BTSTACK_BUSY;
    goep_disconnect_status=BTSTACK_BUSY;
    idle_state_callback_sequence=0;
    completed_event_callback_sequence=0;

    emit_simple(PBAP_SUBEVENT_OPERATION_COMPLETED,0);
    CHECK(disconnect_calls==before_disconnect+1);
    CHECK(goep_disconnect_calls==before_goep+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_IDLE);
    CHECK(count_event_type(BTSTACK_PBAP_CLIENT_EVENT_COMPLETED)==before_completed+1);
    CHECK(idle_state_callback_sequence>0);
    CHECK(completed_event_callback_sequence>idle_state_callback_sequence);

    pbap_disconnect_status=ERROR_CODE_SUCCESS;
    goep_disconnect_status=ERROR_CODE_SUCCESS;
    native_singleton_busy=0;
    goep_client_exists=0;
    CHECK(btstack_pbap_client_sync(second_peer,BTSTACK_PBAP_CLIENT_CATEGORY_MASK_CONTACTS)==BTSTACK_STATUS_OK);
    CHECK(connect_calls==before_connect+2);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_CONNECTING);

    emit_simple_cid(PBAP_SUBEVENT_CONNECTION_CLOSED,0,first_cid,first_cid,5u);
    emit_simple_cid(PBAP_SUBEVENT_OPERATION_COMPLETED,0,first_cid,first_cid,6u);
    CHECK(count_event_type(BTSTACK_PBAP_CLIENT_EVENT_COMPLETED)==before_completed+1);
    CHECK(btstack_pbap_client_get_state()==BTSTACK_PBAP_CLIENT_STATE_CONNECTING);

    emit_open(0,second_peer);
    CHECK(btstack_pbap_client_cancel()==BTSTACK_STATUS_OK);
    emit_simple(PBAP_SUBEVENT_CONNECTION_CLOSED,0);
}

int main(void){
    test_setup_sync_and_cancel(); test_invalid_category_masks_precede_busy(); test_subset_contacts_and_missed_only(); test_single_category_only(); test_fatal_operation_statuses_stop_categories_and_cleanup(); test_fatal_category_callback_cannot_cancel_or_replace_error(); test_all_categories_order_and_one_connection(); test_categories_parser_and_partial(); test_busy_sync_reports_requested_peer_and_preserves_active_sync(); test_pause_replay_and_deferred_completion(); test_parser_fatal_and_replay_overflow_cleanup(); test_terminal_cleanup_failure_paths_do_not_stick_busy(); test_late_open_success_rebinds_connected_cleanup(); test_late_open_success_double_cleanup_failure_finishes(); test_late_open_failure_finishes_saved_terminal_status(); test_resume_replay_callback_cancel(); test_data_callback_cancel_stops_same_packet(); test_event_space_callback_cancel_discards_replay(); test_state_callback_cancel_discards_replay(); test_connecting_cancel_starts_cleanup_without_native_event(); test_connecting_cancel_goep_failure_finishes_immediately(); test_connecting_cancel_synchronous_close_finishes_once(); test_connecting_cancel_late_auth_restarts_cleanup(); test_connecting_cancel_late_auth_cleanup_failure_finishes(); test_connecting_cancel_late_auth_synchronous_close_allows_resync(); test_disconnected_operation_and_open_failures(); test_peer_loss(); test_auth_timeout_password_and_loss(); test_callback_reentry(); test_cid_and_event_length_isolation(); test_normal_completion_waits_for_native_close_and_allows_inline_resync(); test_normal_finish_falls_back_to_goep_with_synchronous_close(); test_normal_finish_disconnect_failures_complete_without_close();
    printf("PASS: 33 PBAP production-wrapper state-machine groups\n"); return 0;
}

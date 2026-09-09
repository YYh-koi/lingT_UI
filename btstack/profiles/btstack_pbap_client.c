#include "btstack_config.h"
#include "profiles/btstack_pbap_client.h"

#if BTSTACK_CLASSIC && BTSTACK_PBAP_CLIENT

#include <stddef.h>
#include <string.h>

#include "bluetooth.h"
#include "btstack_defines.h"
#include "btstack_event.h"
#include "btstack_run_loop.h"
#include "classic/goep_client.h"
#include "classic/obex.h"
#include "classic/pbap_client.h"

#define PBAP_AUTH_TIMEOUT_MS 30000u
#define PBAP_CATEGORY_COUNT 4u
#ifdef HCI_ACL_PAYLOAD_SIZE
#define PBAP_REPLAY_SIZE HCI_ACL_PAYLOAD_SIZE
#else
#define PBAP_REPLAY_SIZE 1021u
#endif

typedef struct {
    btstack_pbap_client_state_t state;
    btstack_pbap_client_event_callback_t callback;
    btstack_bd_addr_t peer;
    uint16_t cid;
    uint8_t initialized;
    btstack_pbap_client_category_mask_t category_mask;
    uint8_t category_index;
    uint8_t any_category_failed;
    uint8_t cancel_pending;
    uint8_t final_emitted;
    uint8_t pending_valid;
    uint8_t resume_scheduled;
    uint8_t completion_deferred;
    uint8_t deferred_status;
    uint8_t terminal_pending;
    uint8_t terminal_cleanup_started;
    btstack_pbap_client_status_t terminal_status;
    btstack_pbap_client_event_type_t terminal_event_type;
    btstack_pbap_client_event_t pending_event;
    uint8_t replay[PBAP_REPLAY_SIZE];
    size_t replay_length;
    char password[BTSTACK_PBAP_CLIENT_PASSWORD_MAX + 1u];
    btstack_vcard_parser_t parser;
    btstack_timer_source_t auth_timer;
    btstack_context_callback_registration_t resume_registration;
} btstack_pbap_client_context_t;

static btstack_pbap_client_context_t pbap;

static const char * const category_paths[PBAP_CATEGORY_COUNT] = {
    "telecom/pb.vcf", "telecom/ich.vcf", "telecom/och.vcf", "telecom/mch.vcf"
};

static void native_packet_handler(uint8_t packet_type, uint16_t channel,
                                  uint8_t *packet, uint16_t size);
static void resume_on_main_thread(void *context);
static void handle_operation_completed(uint8_t native_status);

static btstack_pbap_client_category_t current_category(void)
{
    return (btstack_pbap_client_category_t) pbap.category_index;
}

static uint8_t select_category_at_or_after(uint8_t category_index)
{
    while (category_index < PBAP_CATEGORY_COUNT) {
        if ((pbap.category_mask & (1u << category_index)) != 0u) {
            pbap.category_index = category_index;
            return 1u;
        }
        ++category_index;
    }
    return 0u;
}

static void initialize_event(btstack_pbap_client_event_t *event,
                             btstack_pbap_client_event_type_t type,
                             btstack_pbap_client_status_t status)
{
    memset(event, 0, sizeof(*event));
    event->type = type;
    event->category = current_category();
    event->status = status;
    memcpy(event->peer, pbap.peer, sizeof(event->peer));
    event->parser_error_count = btstack_vcard_parser_error_count(&pbap.parser);
}

static uint8_t deliver_event(const btstack_pbap_client_event_t *event)
{
    if (pbap.callback == NULL) {
        return 1u;
    }
    return pbap.callback(event);
}

static void deliver_state(void)
{
    static const btstack_pbap_client_event_type_t event_types[] = {
        BTSTACK_PBAP_CLIENT_EVENT_DISCONNECTED,
        BTSTACK_PBAP_CLIENT_EVENT_CONNECTING,
        BTSTACK_PBAP_CLIENT_EVENT_AUTHENTICATING,
        BTSTACK_PBAP_CLIENT_EVENT_PULLING,
        BTSTACK_PBAP_CLIENT_EVENT_PAUSED,
        BTSTACK_PBAP_CLIENT_EVENT_DISCONNECTING
    };
    btstack_pbap_client_event_t event;
    initialize_event(&event, event_types[(uint8_t)pbap.state],
        BTSTACK_PBAP_CLIENT_STATUS_SUCCESS);
    (void)deliver_event(&event);
}

static void clear_transfer_buffers(void)
{
    pbap.pending_valid = 0u;
    pbap.replay_length = 0u;
    pbap.completion_deferred = 0u;
    pbap.resume_scheduled = 0u;
}

static void reset_to_idle(void)
{
    (void) btstack_run_loop_remove_timer(&pbap.auth_timer);
    clear_transfer_buffers();
    pbap.cancel_pending = 0u;
    pbap.terminal_pending = 0u;
    pbap.terminal_cleanup_started = 0u;
    pbap.cid = 0u;
    pbap.state = BTSTACK_PBAP_CLIENT_STATE_IDLE;
    deliver_state();
}

static void emit_final(btstack_pbap_client_status_t status,
                       btstack_pbap_client_event_type_t type,
                       uint8_t become_idle)
{
    btstack_pbap_client_event_t event;

    if (pbap.final_emitted) {
        return;
    }
    pbap.final_emitted = 1u;
    initialize_event(&event, type, status);
    if (become_idle) {
        reset_to_idle();
    }
    (void) deliver_event(&event);
}

static void prepare_terminal_cleanup(btstack_pbap_client_status_t status,
                                     btstack_pbap_client_event_type_t type)
{
    pbap.terminal_pending = 1u;
    pbap.terminal_status = status;
    pbap.terminal_event_type = type;
    clear_transfer_buffers();
    pbap.state = BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING;
}

static void start_terminal_cleanup(btstack_pbap_client_status_t status,
                                   btstack_pbap_client_event_type_t type,
                                   uint8_t pbap_connected)
{
    uint8_t cleanup_status;

    prepare_terminal_cleanup(status, type);
    if (!pbap.terminal_cleanup_started) {
        pbap.terminal_cleanup_started = 1u;
        if (pbap_connected) {
            cleanup_status = pbap_disconnect(pbap.cid);
            if (cleanup_status != ERROR_CODE_SUCCESS &&
                pbap.terminal_pending) {
                cleanup_status = goep_client_disconnect(pbap.cid);
            }
        } else {
            cleanup_status = goep_client_disconnect(pbap.cid);
        }
        if (cleanup_status != ERROR_CODE_SUCCESS &&
            pbap.terminal_pending) {
            emit_final(status, type, 1u);
        }
    }
}

static btstack_pbap_client_status_t connection_status(uint8_t native_status)
{
    if (native_status == ERROR_CODE_CONNECTION_REJECTED_DUE_TO_SECURITY_REASONS) {
        return BTSTACK_PBAP_CLIENT_STATUS_DENIED;
    }
    return BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_FAILED;
}

static btstack_pbap_client_status_t operation_status(uint8_t native_status)
{
    if (native_status == OBEX_NOT_FOUND) {
        return BTSTACK_PBAP_CLIENT_STATUS_UNSUPPORTED;
    }
    if (native_status == ERROR_CODE_AUTHENTICATION_FAILURE ||
        native_status == ERROR_CODE_CONNECTION_REJECTED_DUE_TO_SECURITY_REASONS) {
        return BTSTACK_PBAP_CLIENT_STATUS_DENIED;
    }
    return native_status == ERROR_CODE_SUCCESS ?
        BTSTACK_PBAP_CLIENT_STATUS_SUCCESS :
        BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR;
}

static uint8_t parser_record(void *context, const btstack_vcard_record_t *record)
{
    btstack_pbap_client_event_t event;
    uint8_t accepted;

    (void) context;
    initialize_event(&event,
        pbap.category_index == BTSTACK_PBAP_CLIENT_CATEGORY_CONTACTS ?
            BTSTACK_PBAP_CLIENT_EVENT_CONTACT : BTSTACK_PBAP_CLIENT_EVENT_CALL,
        BTSTACK_PBAP_CLIENT_STATUS_SUCCESS);
    event.record = *record;
    if (event.record.direction == BTSTACK_VCARD_DIRECTION_NONE) {
        switch (current_category()) {
            case BTSTACK_PBAP_CLIENT_CATEGORY_INCOMING:
                event.record.direction = BTSTACK_VCARD_DIRECTION_INCOMING;
                break;
            case BTSTACK_PBAP_CLIENT_CATEGORY_OUTGOING:
                event.record.direction = BTSTACK_VCARD_DIRECTION_OUTGOING;
                break;
            case BTSTACK_PBAP_CLIENT_CATEGORY_MISSED:
                event.record.direction = BTSTACK_VCARD_DIRECTION_MISSED;
                break;
            default:
                break;
        }
    }
    accepted = deliver_event(&event);
    if (pbap.state != BTSTACK_PBAP_CLIENT_STATE_PULLING ||
        pbap.cancel_pending || pbap.terminal_pending) {
        return 0u;
    }
    if (!accepted) {
        pbap.pending_event = event;
        pbap.pending_valid = 1u;
        pbap.state = BTSTACK_PBAP_CLIENT_STATE_PAUSED;
    }
    return accepted;
}

static uint8_t start_category(void)
{
    uint8_t status;

    btstack_vcard_parser_init(&pbap.parser, parser_record, NULL);
    pbap.state = BTSTACK_PBAP_CLIENT_STATE_PULLING;
    status = pbap_pull_phonebook(pbap.cid, category_paths[pbap.category_index]);
    if (status != ERROR_CODE_SUCCESS) {
        pbap.any_category_failed = 1u;
        handle_operation_completed(status);
        return 0u;
    }
    return 1u;
}

static void finish_all_categories(void)
{
    btstack_pbap_client_status_t status = pbap.any_category_failed ?
        BTSTACK_PBAP_CLIENT_STATUS_PARTIAL : BTSTACK_PBAP_CLIENT_STATUS_SUCCESS;

    start_terminal_cleanup(status, BTSTACK_PBAP_CLIENT_EVENT_COMPLETED, 1u);
}

static void handle_operation_completed(uint8_t native_status)
{
    btstack_pbap_client_event_t event;
    btstack_pbap_client_status_t status = operation_status(native_status);
    btstack_vcard_parser_result_t parser_result;

    if (pbap.state == BTSTACK_PBAP_CLIENT_STATE_IDLE ||
        pbap.final_emitted) {
        return;
    }
    if (native_status == OBEX_DISCONNECTED) {
        if (pbap.terminal_pending) {
            return;
        }
        start_terminal_cleanup(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST,
                               BTSTACK_PBAP_CLIENT_EVENT_COMPLETED, 1u);
        return;
    }
    if (pbap.state == BTSTACK_PBAP_CLIENT_STATE_PAUSED) {
        pbap.completion_deferred = 1u;
        pbap.deferred_status = native_status;
        return;
    }
    if (pbap.state != BTSTACK_PBAP_CLIENT_STATE_PULLING) {
        return;
    }
    parser_result = btstack_vcard_parser_finish(&pbap.parser);
    if (parser_result == BTSTACK_VCARD_PARSER_PAUSED) {
        pbap.completion_deferred = 1u;
        pbap.deferred_status = native_status;
        return;
    }
    if (parser_result == BTSTACK_VCARD_PARSER_ERROR) {
        status = BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR;
        pbap.any_category_failed = 1u;
    } else if (status != BTSTACK_PBAP_CLIENT_STATUS_SUCCESS) {
        pbap.any_category_failed = 1u;
    }
    if (status == BTSTACK_PBAP_CLIENT_STATUS_DENIED ||
        status == BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR) {
        prepare_terminal_cleanup(status, BTSTACK_PBAP_CLIENT_EVENT_ERROR);
    }
    initialize_event(&event, BTSTACK_PBAP_CLIENT_EVENT_CATEGORY_COMPLETED,
                     status);
    (void) deliver_event(&event);
    if (status == BTSTACK_PBAP_CLIENT_STATUS_DENIED ||
        status == BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR) {
        if (!pbap.final_emitted && pbap.terminal_pending &&
            pbap.state == BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING &&
            pbap.terminal_status == status &&
            pbap.terminal_event_type == BTSTACK_PBAP_CLIENT_EVENT_ERROR) {
            start_terminal_cleanup(status, BTSTACK_PBAP_CLIENT_EVENT_ERROR,
                                   1u);
        }
        return;
    }
    if (pbap.state != BTSTACK_PBAP_CLIENT_STATE_PULLING ||
        pbap.cancel_pending || pbap.terminal_pending) {
        return;
    }
    if (select_category_at_or_after((uint8_t) (pbap.category_index + 1u))) {
        (void) start_category();
    } else {
        finish_all_categories();
    }
}

static btstack_vcard_parser_result_t parse_bytes(const uint8_t *bytes,
                                                  size_t length,
                                                  uint8_t replaying)
{
    btstack_vcard_parser_result_t result;
    size_t consumed = 0u;
    size_t remainder;

    result = btstack_vcard_parser_process(&pbap.parser, bytes, length,
                                          &consumed);
    if (result == BTSTACK_VCARD_PARSER_PAUSED) {
        if (pbap.state != BTSTACK_PBAP_CLIENT_STATE_PULLING &&
            pbap.state != BTSTACK_PBAP_CLIENT_STATE_PAUSED) {
            pbap.replay_length = 0u;
            return result;
        }
        remainder = length - consumed;
        if (remainder > sizeof(pbap.replay)) {
            start_terminal_cleanup(BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR,
                                   BTSTACK_PBAP_CLIENT_EVENT_ERROR, 1u);
            return BTSTACK_VCARD_PARSER_ERROR;
        }
        if (remainder != 0u) {
            if (replaying) {
                memmove(pbap.replay, pbap.replay + consumed, remainder);
            } else {
                memcpy(pbap.replay, bytes + consumed, remainder);
            }
        }
        pbap.replay_length = remainder;
    } else if (result == BTSTACK_VCARD_PARSER_ERROR) {
        start_terminal_cleanup(BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR,
                               BTSTACK_PBAP_CLIENT_EVENT_ERROR, 1u);
    } else if (replaying) {
        pbap.replay_length = 0u;
    }
    return result;
}

static void handle_data_packet(const uint8_t *packet, uint16_t size)
{
    if (pbap.state != BTSTACK_PBAP_CLIENT_STATE_PULLING) {
        return;
    }
    if (parse_bytes(packet, size, 0u) == BTSTACK_VCARD_PARSER_OK) {
        if (pbap.state == BTSTACK_PBAP_CLIENT_STATE_PULLING &&
            !pbap.cancel_pending && !pbap.terminal_pending) {
            (void) pbap_next_packet(pbap.cid);
        }
    }
}

static void auth_timeout(btstack_timer_source_t *timer)
{
    (void) timer;
    if (pbap.state != BTSTACK_PBAP_CLIENT_STATE_AUTHENTICATING) {
        return;
    }
    (void) btstack_run_loop_remove_timer(&pbap.auth_timer);
    start_terminal_cleanup(pbap.cancel_pending ?
                               BTSTACK_PBAP_CLIENT_STATUS_CANCELED :
                               BTSTACK_PBAP_CLIENT_STATUS_AUTH_TIMEOUT,
                           BTSTACK_PBAP_CLIENT_EVENT_COMPLETED, 0u);
}

static void handle_connection_opened(const uint8_t *event)
{
    uint8_t status = pbap_subevent_connection_opened_get_status(event);
    btstack_pbap_client_event_t generic_event;

    if (pbap.state != BTSTACK_PBAP_CLIENT_STATE_CONNECTING &&
        pbap.state != BTSTACK_PBAP_CLIENT_STATE_AUTHENTICATING &&
        (pbap.state != BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING ||
         !pbap.terminal_pending)) {
        return;
    }
    (void) btstack_run_loop_remove_timer(&pbap.auth_timer);
    pbap.cid = pbap_subevent_connection_opened_get_pbap_cid(event);
    if (pbap.terminal_pending) {
        btstack_pbap_client_status_t final_status = pbap.terminal_status;
        btstack_pbap_client_event_type_t final_type =
            pbap.terminal_event_type;
        if (status == ERROR_CODE_SUCCESS) {
            pbap.terminal_cleanup_started = 0u;
            start_terminal_cleanup(final_status, final_type, 1u);
        } else {
            emit_final(final_status, final_type, 1u);
        }
        return;
    }
    if (status != ERROR_CODE_SUCCESS) {
        btstack_pbap_client_status_t final_status = pbap.cancel_pending ?
            BTSTACK_PBAP_CLIENT_STATUS_CANCELED : connection_status(status);
        btstack_pbap_client_event_type_t final_type = pbap.cancel_pending ?
            BTSTACK_PBAP_CLIENT_EVENT_COMPLETED :
            BTSTACK_PBAP_CLIENT_EVENT_ERROR;
        start_terminal_cleanup(final_status, final_type, 0u);
        return;
    }
    if (pbap.cancel_pending) {
        start_terminal_cleanup(BTSTACK_PBAP_CLIENT_STATUS_CANCELED,
                               BTSTACK_PBAP_CLIENT_EVENT_COMPLETED, 1u);
        return;
    }
    if (pbap_set_flow_control_mode(pbap.cid, 1) != ERROR_CODE_SUCCESS) {
        start_terminal_cleanup(BTSTACK_PBAP_CLIENT_STATUS_PROTOCOL_ERROR,
                               BTSTACK_PBAP_CLIENT_EVENT_ERROR, 1u);
        return;
    }
    initialize_event(&generic_event, BTSTACK_PBAP_CLIENT_EVENT_STARTED,
                     BTSTACK_PBAP_CLIENT_STATUS_SUCCESS);
    pbap.state = BTSTACK_PBAP_CLIENT_STATE_PULLING;
    (void) deliver_event(&generic_event);
    if (pbap.state == BTSTACK_PBAP_CLIENT_STATE_PULLING &&
        !pbap.cancel_pending && !pbap.terminal_pending) {
        (void) start_category();
    }
}

static void handle_connection_closed(void)
{
    btstack_pbap_client_status_t terminal_status;

    if (pbap.state == BTSTACK_PBAP_CLIENT_STATE_IDLE) {
        return;
    }
    if (pbap.terminal_pending) {
        terminal_status = pbap.terminal_status;
        if (!pbap.final_emitted) {
            emit_final(terminal_status, pbap.terminal_event_type, 1u);
        } else {
            reset_to_idle();
        }
        return;
    }
    if (pbap.state == BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING) {
        if (!pbap.final_emitted) {
            emit_final(BTSTACK_PBAP_CLIENT_STATUS_CANCELED,
                       BTSTACK_PBAP_CLIENT_EVENT_COMPLETED, 1u);
        } else {
            reset_to_idle();
        }
        return;
    }
    emit_final(pbap.cancel_pending ? BTSTACK_PBAP_CLIENT_STATUS_CANCELED :
                                     BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST,
               BTSTACK_PBAP_CLIENT_EVENT_COMPLETED, 1u);
}

static void handle_authentication_request(const uint8_t *event)
{
    btstack_pbap_client_event_t generic_event;

    if (pbap.terminal_pending) {
        btstack_pbap_client_status_t final_status = pbap.terminal_status;
        btstack_pbap_client_event_type_t final_type =
            pbap.terminal_event_type;
        pbap.cid = pbap_subevent_authentication_request_get_goep_cid(event);
        pbap.terminal_cleanup_started = 0u;
        start_terminal_cleanup(final_status, final_type, 0u);
        return;
    }
    if (pbap.state != BTSTACK_PBAP_CLIENT_STATE_CONNECTING) {
        return;
    }
    pbap.cid = pbap_subevent_authentication_request_get_goep_cid(event);
    if (pbap.cancel_pending) {
        start_terminal_cleanup(BTSTACK_PBAP_CLIENT_STATUS_CANCELED,
                               BTSTACK_PBAP_CLIENT_EVENT_COMPLETED, 0u);
        return;
    }
    pbap.state = BTSTACK_PBAP_CLIENT_STATE_AUTHENTICATING;
    btstack_run_loop_set_timer_handler(&pbap.auth_timer, auth_timeout);
    btstack_run_loop_set_timer(&pbap.auth_timer, PBAP_AUTH_TIMEOUT_MS);
    btstack_run_loop_add_timer(&pbap.auth_timer);
    initialize_event(&generic_event,
                     BTSTACK_PBAP_CLIENT_EVENT_AUTHENTICATION_REQUIRED,
                     BTSTACK_PBAP_CLIENT_STATUS_SUCCESS);
    (void) deliver_event(&generic_event);
}

static void native_packet_handler(uint8_t packet_type, uint16_t channel,
                                  uint8_t *packet, uint16_t size)
{
    uint8_t subevent;

    if (pbap.state == BTSTACK_PBAP_CLIENT_STATE_IDLE ||
        channel != pbap.cid) {
        return;
    }
    if (packet_type == PBAP_DATA_PACKET) {
        handle_data_packet(packet, size);
        return;
    }
    if (packet_type != HCI_EVENT_PACKET || size < 3u ||
        hci_event_packet_get_type(packet) != HCI_EVENT_PBAP_META) {
        return;
    }
    subevent = hci_event_pbap_meta_get_subevent_code(packet);
    switch (subevent) {
        case PBAP_SUBEVENT_CONNECTION_OPENED:
            if (size < 15u ||
                pbap_subevent_connection_opened_get_pbap_cid(packet) !=
                    pbap.cid) {
                return;
            }
            handle_connection_opened(packet);
            break;
        case PBAP_SUBEVENT_CONNECTION_CLOSED:
            if (size < 5u ||
                pbap_subevent_connection_closed_get_goep_cid(packet) !=
                    pbap.cid) {
                return;
            }
            handle_connection_closed();
            break;
        case PBAP_SUBEVENT_OPERATION_COMPLETED:
            if (size < 6u ||
                pbap_subevent_operation_completed_get_goep_cid(packet) !=
                    pbap.cid) {
                return;
            }
            handle_operation_completed(
                pbap_subevent_operation_completed_get_status(packet));
            break;
        case PBAP_SUBEVENT_AUTHENTICATION_REQUEST:
            if (size < 7u ||
                pbap_subevent_authentication_request_get_goep_cid(packet) !=
                    pbap.cid) {
                return;
            }
            handle_authentication_request(packet);
            break;
        default:
            break;
    }
}

static void resume_on_main_thread(void *context)
{
    btstack_vcard_parser_result_t result;
    uint8_t deferred_status;

    (void) context;
    pbap.resume_scheduled = 0u;
    if (pbap.state != BTSTACK_PBAP_CLIENT_STATE_PAUSED ||
        !pbap.pending_valid) {
        return;
    }
    {
        uint8_t accepted = deliver_event(&pbap.pending_event);
        if (pbap.state != BTSTACK_PBAP_CLIENT_STATE_PAUSED ||
            pbap.cancel_pending || pbap.terminal_pending) {
            pbap.pending_valid = 0u;
            pbap.replay_length = 0u;
            return;
        }
        if (!accepted) {
            return;
        }
    }
    pbap.pending_valid = 0u;
    pbap.state = BTSTACK_PBAP_CLIENT_STATE_PULLING;
    deliver_state();
    if (pbap.state != BTSTACK_PBAP_CLIENT_STATE_PULLING ||
        pbap.cancel_pending || pbap.terminal_pending) {
        pbap.replay_length = 0u;
        return;
    }
    if (pbap.replay_length != 0u) {
        result = parse_bytes(pbap.replay, pbap.replay_length, 1u);
        if (result != BTSTACK_VCARD_PARSER_OK) {
            return;
        }
    }
    if (pbap.state != BTSTACK_PBAP_CLIENT_STATE_PULLING ||
        pbap.cancel_pending || pbap.terminal_pending) {
        return;
    }
    if (pbap.completion_deferred) {
        deferred_status = pbap.deferred_status;
        pbap.completion_deferred = 0u;
        handle_operation_completed(deferred_status);
    } else {
        (void) pbap_next_packet(pbap.cid);
    }
}

int btstack_pbap_client_setup(void)
{
    if (!pbap.initialized) {
        memset(&pbap, 0, sizeof(pbap));
        pbap.state = BTSTACK_PBAP_CLIENT_STATE_IDLE;
        pbap.resume_registration.callback = resume_on_main_thread;
        pbap_client_init();
        pbap.initialized = 1u;
    }
    return BTSTACK_STATUS_OK;
}

int btstack_pbap_client_set_event_callback(
    btstack_pbap_client_event_callback_t callback)
{
    pbap.callback = callback;
    return BTSTACK_STATUS_OK;
}

int btstack_pbap_client_sync(
    const btstack_bd_addr_t address,
    btstack_pbap_client_category_mask_t category_mask)
{
    btstack_pbap_client_event_t event;
    uint8_t status;

    if (address == NULL || category_mask == 0u ||
        (category_mask & (btstack_pbap_client_category_mask_t)
            ~BTSTACK_PBAP_CLIENT_CATEGORY_MASK_ALL) != 0u) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (pbap.state != BTSTACK_PBAP_CLIENT_STATE_IDLE) {
        memset(&event, 0, sizeof(event));
        event.type = BTSTACK_PBAP_CLIENT_EVENT_ERROR;
        event.status = BTSTACK_PBAP_CLIENT_STATUS_BUSY;
        memcpy(event.peer, address, sizeof(event.peer));
        (void) deliver_event(&event);
        return BTSTACK_STATUS_BUSY;
    }
    memcpy(pbap.peer, address, sizeof(pbap.peer));
    pbap.category_mask = category_mask;
    (void) select_category_at_or_after(0u);
    pbap.any_category_failed = 0u;
    pbap.cancel_pending = 0u;
    pbap.terminal_pending = 0u;
    pbap.terminal_cleanup_started = 0u;
    pbap.final_emitted = 0u;
    clear_transfer_buffers();
    pbap.state = BTSTACK_PBAP_CLIENT_STATE_CONNECTING;
    status = pbap_connect(native_packet_handler, pbap.peer, &pbap.cid);
    if (status != ERROR_CODE_SUCCESS) {
        emit_final(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_FAILED,
                   BTSTACK_PBAP_CLIENT_EVENT_ERROR, 1u);
        return BTSTACK_STATUS_ERROR;
    }
    return BTSTACK_STATUS_OK;
}

int btstack_pbap_client_cancel(void)
{
    if (pbap.state == BTSTACK_PBAP_CLIENT_STATE_IDLE ||
        pbap.state == BTSTACK_PBAP_CLIENT_STATE_DISCONNECTING) {
        return BTSTACK_STATUS_NOT_READY;
    }
    pbap.cancel_pending = 1u;
    if (pbap.state == BTSTACK_PBAP_CLIENT_STATE_CONNECTING) {
        start_terminal_cleanup(BTSTACK_PBAP_CLIENT_STATUS_CANCELED,
                               BTSTACK_PBAP_CLIENT_EVENT_COMPLETED, 0u);
        return BTSTACK_STATUS_OK;
    }
    if (pbap.state == BTSTACK_PBAP_CLIENT_STATE_AUTHENTICATING) {
        auth_timeout(&pbap.auth_timer);
        return BTSTACK_STATUS_OK;
    }
    if (pbap.state == BTSTACK_PBAP_CLIENT_STATE_PULLING ||
        pbap.state == BTSTACK_PBAP_CLIENT_STATE_PAUSED) {
        (void) pbap_abort(pbap.cid);
        start_terminal_cleanup(BTSTACK_PBAP_CLIENT_STATUS_CANCELED,
                               BTSTACK_PBAP_CLIENT_EVENT_COMPLETED, 1u);
    }
    return BTSTACK_STATUS_OK;
}

int btstack_pbap_client_authenticate(const char *password)
{
    size_t length;
    uint8_t status;

    if (password == NULL) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    if (pbap.state != BTSTACK_PBAP_CLIENT_STATE_AUTHENTICATING ||
        pbap.cancel_pending) {
        return BTSTACK_STATUS_NOT_READY;
    }
    length = strlen(password);
    if (length > BTSTACK_PBAP_CLIENT_PASSWORD_MAX) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }
    memcpy(pbap.password, password, length + 1u);
    status = pbap_authentication_password(pbap.cid, pbap.password);
    if (status != ERROR_CODE_SUCCESS) {
        return BTSTACK_STATUS_ERROR;
    }
    (void) btstack_run_loop_remove_timer(&pbap.auth_timer);
    pbap.state = BTSTACK_PBAP_CLIENT_STATE_CONNECTING;
    return BTSTACK_STATUS_OK;
}

btstack_pbap_client_state_t btstack_pbap_client_get_state(void)
{
    return pbap.state;
}

void btstack_pbap_client_event_space_available(void)
{
    if (pbap.state != BTSTACK_PBAP_CLIENT_STATE_PAUSED ||
        !pbap.pending_valid || pbap.resume_scheduled) {
        return;
    }
    pbap.resume_scheduled = 1u;
    btstack_run_loop_execute_on_main_thread(&pbap.resume_registration);
}

void btstack_pbap_client_peer_disconnected(const btstack_bd_addr_t address)
{
    uint8_t pbap_connected;

    if (address == NULL || pbap.state == BTSTACK_PBAP_CLIENT_STATE_IDLE ||
        memcmp(address, pbap.peer, sizeof(pbap.peer)) != 0) {
        return;
    }
    if (pbap.terminal_pending) {
        return;
    }
    pbap_connected = pbap.state == BTSTACK_PBAP_CLIENT_STATE_PULLING ||
                     pbap.state == BTSTACK_PBAP_CLIENT_STATE_PAUSED;
    clear_transfer_buffers();
    if (pbap_connected) {
        (void) pbap_abort(pbap.cid);
    }
    start_terminal_cleanup(BTSTACK_PBAP_CLIENT_STATUS_CONNECTION_LOST,
                           BTSTACK_PBAP_CLIENT_EVENT_COMPLETED,
                           pbap_connected);
}
#else
int btstack_pbap_client_setup(void)
{
    return BTSTACK_STATUS_NOT_SUPPORTED;
}

int btstack_pbap_client_set_event_callback(
    btstack_pbap_client_event_callback_t callback)
{
    (void)callback;
    return BTSTACK_STATUS_NOT_SUPPORTED;
}

int btstack_pbap_client_sync(
    const btstack_bd_addr_t address,
    btstack_pbap_client_category_mask_t category_mask)
{
    (void)address;
    (void)category_mask;
    return BTSTACK_STATUS_NOT_SUPPORTED;
}

int btstack_pbap_client_cancel(void)
{
    return BTSTACK_STATUS_NOT_SUPPORTED;
}

int btstack_pbap_client_authenticate(const char *password)
{
    (void)password;
    return BTSTACK_STATUS_NOT_SUPPORTED;
}

btstack_pbap_client_state_t btstack_pbap_client_get_state(void)
{
    return BTSTACK_PBAP_CLIENT_STATE_IDLE;
}

void btstack_pbap_client_event_space_available(void)
{
}

void btstack_pbap_client_peer_disconnected(
    const btstack_bd_addr_t address)
{
    (void)address;
}
#endif

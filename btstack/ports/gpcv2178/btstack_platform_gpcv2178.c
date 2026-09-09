#include "port/btstack_platform.h"

#include "adapter/btstack_types.h"
#include "btstack_config.h"
#include "btstack_event.h"
#include "btstack_gpcv2178_platform.h"
#include "btstack_hci_dump_gpcv2178.h"
#include "btstack_rtos_gpcv2178.h"
#include "btstack_run_loop_gpcv2178.h"
#include "btstack_uart_gpcv2178.h"
#include "btstack_uart_slip_wrapper.h"
#include "gplib_print_string.h"
#include "hci.h"
#include "hci_cmd.h"
#include "hci_transport_h4.h"
#include "hci_transport_h5.h"
#include "project.h"

extern const btstack_control_t btstack_control_gpcv2178;
extern int btstack_storage_gpcv2178_read(void *context,
    btstack_port_ops_storage_key_t key, uint8_t tag, uint8_t *data,
    uint16_t *data_len);
extern int btstack_storage_gpcv2178_write(void *context,
    btstack_port_ops_storage_key_t key, uint8_t tag, const uint8_t *data,
    uint16_t data_len);
extern int btstack_storage_gpcv2178_delete(void *context,
    btstack_port_ops_storage_key_t key, uint8_t tag);

static btstack_packet_callback_registration_t btstack_hci_event_callback;
static bool btstack_hci_event_registered;

static const hci_transport_config_uart_t hci_transport_config = {
    HCI_TRANSPORT_CONFIG_UART,
    BTSTACK_GPCV2178_HCI_UART_BAUD_INIT,
    BTSTACK_GPCV2178_HCI_UART_BAUD_MAIN,
    BTSTACK_GPCV2178_HCI_UART_FLOWCONTROL,
    "uart0",
    BTSTACK_GPCV2178_HCI_UART_PARITY
};

static const btstack_profile_manager_config_t profile_manager_config = {
    .struct_size = sizeof(btstack_profile_manager_config_t),
    .device_name = "GPCV2178 Audio",
    .manufacturer = "Generalplus",
    .classic_profile_mask = BTSTACK_PROFILE_MANAGER_CLASSIC_ALL,
    .le_profile_mask = BTSTACK_PROFILE_MANAGER_LE_GATT_CLIENT |
        BTSTACK_PROFILE_MANAGER_LE_GATT_SERVER,
    .classic = {
        .class_of_device = 0x240404u,
        .a2dp_sdp_handle = 0x10001u,
        .hfp_sdp_handle = 0x10002u,
        .avrcp_controller_sdp_handle = 0x10004u,
        .avrcp_target_sdp_handle = 0x10003u,
        .hfp_rfcomm_channel = 1u,
        .hfp_microphone_gain = 8u,
        .hfp_speaker_gain = 14u
    }
};

static void btstack_gpcv2178_hci_event_packet_handler(
    uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    (void)channel;
    if ((packet_type != HCI_EVENT_PACKET) || (packet == NULL)) return;
    if ((size >= 6u) &&
        (hci_event_packet_get_type(packet) == HCI_EVENT_COMMAND_COMPLETE) &&
        (hci_event_command_complete_get_command_opcode(packet) ==
            HCI_OPCODE_HCI_WRITE_AFH_CHANNEL_ASSESSMENT_MODE)) {
        DBG_PRINT("btstack: AFH channel assessment status=0x%02X\r\n",
            (unsigned int)
                hci_event_command_complete_get_return_parameters(packet)[0]);
        return;
    }
    if ((size < 6u) ||
        (hci_event_packet_get_type(packet) !=
            HCI_EVENT_DISCONNECTION_COMPLETE)) return;
    DBG_PRINT("btstack disconnect handle=0x%04X reason=0x%02X\r\n",
        (unsigned int)
            hci_event_disconnection_complete_get_connection_handle(packet),
        (unsigned int)hci_event_disconnection_complete_get_reason(packet));
}

static void enter_critical(void *context)
{ (void)context; taskENTER_CRITICAL(); }
static void exit_critical(void *context)
{ (void)context; taskEXIT_CRITICAL(); }
static uint32_t time_ms(void *context)
{ (void)context; return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS); }
static void delay_ms(void *context, uint32_t milliseconds)
{ (void)context; vTaskDelay(pdMS_TO_TICKS(milliseconds)); }

static void platform_log(void *context, btstack_port_ops_log_level_t level,
    const char *tag, const char *message)
{
    (void)context;
    DBG_PRINT("[btstack][%u][%s] %s\r\n", (unsigned int)level,
        tag != NULL ? tag : "-", message != NULL ? message : "");
}

static int worker_validate(void *context, btstack_worker_id_t id,
    const btstack_worker_config_t *config)
{
    const btstack_worker_ops_t *ops = btstack_rtos_gpcv2178_worker_ops();
    (void)context;
    return ops->validate_config(ops->context, id, config);
}

static int worker_start(void *context, btstack_worker_id_t id,
    btstack_worker_entry_t entry, void *argument,
    const btstack_worker_config_t *config)
{
    const btstack_worker_ops_t *ops = btstack_rtos_gpcv2178_worker_ops();
    (void)context;
    return ops->start(ops->context, id, entry, argument, config);
}

static int worker_notify(void *context, btstack_worker_id_t id,
    uint32_t signals)
{
    const btstack_worker_ops_t *ops = btstack_rtos_gpcv2178_worker_ops();
    (void)context;
    return ops->notify(ops->context, id, signals);
}

static uint32_t worker_wait(void *context, btstack_worker_id_t id,
    uint32_t mask, uint32_t timeout_ms)
{
    const btstack_worker_ops_t *ops = btstack_rtos_gpcv2178_worker_ops();
    (void)context;
    return ops->wait(ops->context, id, mask, timeout_ms);
}

static int worker_stop(void *context, btstack_worker_id_t id)
{
    const btstack_worker_ops_t *ops = btstack_rtos_gpcv2178_worker_ops();
    (void)context;
    return ops->stop(ops->context, id);
}

static int early_init(void *context)
{
    (void)context;
    DBG_PRINT("btstack: platform early init\r\n");
    return BTSTACK_STATUS_OK;
}

static void late_deinit(void *context)
{
    (void)context;
    DBG_PRINT("btstack: platform late deinit\r\n");
}

static const btstack_run_loop_t *get_run_loop(void *context)
{
    (void)context;
    return btstack_run_loop_gpcv2178_get_instance();
}

static const hci_transport_t *get_hci_transport(void *context)
{
    (void)context;
#if BTSTACK_GPCV2178_USE_H4_TRANSPORT
    return hci_transport_h4_instance_for_uart(
        btstack_uart_gpcv2178_instance());
#else
#if BTSTACK_GPCV2178_UART_RX_USE_DMA
    return hci_transport_h5_instance(btstack_uart_gpcv2178_instance());
#else
    return hci_transport_h5_instance(btstack_uart_slip_wrapper_instance(
        btstack_uart_gpcv2178_instance()));
#endif
#endif
}

static const void *get_hci_transport_config(void *context)
{ (void)context; return &hci_transport_config; }

static const btstack_profile_manager_config_t *get_profile_manager_config(
    void *context)
{ (void)context; return &profile_manager_config; }

static int after_hci_init(void *context)
{
    (void)context;
#if BTSTACK_GPCV2178_HCI_DUMP_ENABLE
    hci_dump_init(btstack_hci_dump_gpcv2178_instance());
    hci_dump_enable_packet_log(true);
#endif
    if (!btstack_hci_event_registered) {
        btstack_hci_event_callback.callback =
            btstack_gpcv2178_hci_event_packet_handler;
        hci_add_event_handler(&btstack_hci_event_callback);
        btstack_hci_event_registered = true;
    }
    return BTSTACK_STATUS_OK;
}

static bool post_callback(void *context,
    btstack_context_callback_registration_t *callback)
{
    (void)context;
    return btstack_run_loop_gpcv2178_enqueue_callback(callback);
}

static void get_diagnostics(void *context,
    btstack_stack_queue_diagnostics_t *diagnostics)
{
    (void)context;
    btstack_run_loop_gpcv2178_get_diagnostics(diagnostics);
}

static const btstack_platform_t btstack_platform_gpcv2178 = {
    .abi_version = BTSTACK_PLATFORM_ABI_VERSION,
    .struct_size = sizeof(btstack_platform_t),
    .name = "gpcv2178-freertos",
    .system = {
        .enter_critical = enter_critical,
        .exit_critical = exit_critical,
        .time_ms = time_ms,
        .delay_ms = delay_ms,
        .log = platform_log
    },
    .workers = {
        .validate_config = worker_validate,
        .start = worker_start,
        .notify = worker_notify,
        .wait = worker_wait,
        .stop = worker_stop
    },
    .storage = {
        .read = btstack_storage_gpcv2178_read,
        .write = btstack_storage_gpcv2178_write,
        .delete_value = btstack_storage_gpcv2178_delete
    },
    .controller = { .control = &btstack_control_gpcv2178 },
    .stack = {
        .early_init = early_init,
        .late_deinit = late_deinit,
        .get_run_loop = get_run_loop,
        .get_hci_transport = get_hci_transport,
        .get_hci_transport_config = get_hci_transport_config,
        .get_profile_manager_config = get_profile_manager_config,
        .after_hci_init = after_hci_init,
        .post_callback = post_callback,
        .get_diagnostics = get_diagnostics
    }
};

const btstack_platform_t *btstack_platform_default(void)
{
    return &btstack_platform_gpcv2178;
}

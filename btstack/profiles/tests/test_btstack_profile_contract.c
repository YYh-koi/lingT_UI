#include "profiles/btstack_a2dp_sink.h"
#include "profiles/btstack_avrcp.h"
#include "core/btstack_profile_manager.h"
#include "profiles/btstack_hfp_hf.h"

int btstack_profile_contract_test(void)
{
    btstack_profile_manager_config_t manager_config = {0};
    btstack_hfp_hf_event_t hfp_event = {0};
    btstack_a2dp_sink_event_t a2dp_event = {0};
    btstack_avrcp_event_t avrcp_event = {0};
    btstack_bd_addr_t address = {0};
    int status = 0;

    manager_config.struct_size = sizeof(manager_config);
    manager_config.device_name = "BTstack";
    manager_config.manufacturer = "BTstack";
    status += btstack_profile_manager_init(&manager_config);
    status += btstack_hfp_hf_connect(address);
    status += btstack_hfp_hf_disconnect();
    status += btstack_a2dp_sink_connect(address);
    status += btstack_a2dp_sink_disconnect();
    status += btstack_avrcp_control(BTSTACK_AVRCP_CONTROL_PLAY);
    status += (int)hfp_event.type;
    status += (int)a2dp_event.type;
    status += (int)avrcp_event.type;
    return status;
}

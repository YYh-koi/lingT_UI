#include "profiles/btstack_a2dp_sink.h"
#include "profiles/btstack_avrcp.h"
#include "profiles/btstack_gatt_client.h"
#include "profiles/btstack_gatt_server.h"
#include "profiles/btstack_hfp_hf.h"
#include "profiles/btstack_hid_device.h"
#include "profiles/btstack_hogp_device.h"
#include "profiles/btstack_pbap_client.h"

static int expect_not_supported(int status)
{
    return status == BTSTACK_STATUS_NOT_SUPPORTED ? 0 : 1;
}

int main(void)
{
    int failures = 0;

    failures += expect_not_supported(btstack_a2dp_sink_setup(0));
    failures += expect_not_supported(btstack_avrcp_setup(0));
    failures += expect_not_supported(btstack_hfp_hf_setup(0));
    failures += expect_not_supported(btstack_hid_device_setup(0));
    failures += expect_not_supported(btstack_hogp_device_setup(0));
    failures += expect_not_supported(btstack_gatt_client_init());
    failures += expect_not_supported(btstack_gatt_server_init());
    failures += expect_not_supported(btstack_pbap_client_setup());
    return failures;
}

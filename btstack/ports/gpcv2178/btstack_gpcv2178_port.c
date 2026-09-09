#include "btstack_gpcv2178_port.h"

#include "adapter/btstack_types.h"
#include "btstack_runtime.h"
#include "port/btstack_platform.h"

int btstack_gpcv2178_init(void)
{
    if ((btstack_platform_get() == NULL) &&
        (btstack_platform_activate(btstack_platform_default()) !=
            BTSTACK_PLATFORM_OK)) {
        return BTSTACK_STATUS_ERROR;
    }
    return btstack_runtime_init();
}

int btstack_gpcv2178_start_run_loop(void)
{
    return btstack_runtime_start();
}

int btstack_gpcv2178_run_loop_started(void)
{
    return btstack_runtime_started();
}

#include "btstack_control.h"
#include "btstack_gpcv2178_platform.h"
#include "project.h"

static void (*btstack_control_power_callback)(POWER_NOTIFICATION_t event);

static void btstack_control_gpcv2178_init(const void *config)
{
    (void)config;
}

static int btstack_control_gpcv2178_on(void)
{
    if (btstack_control_power_callback != NULL) {
        btstack_control_power_callback(POWER_WILL_WAKE_UP);
    }
    return 0;
}

static int btstack_control_gpcv2178_off(void)
{
    return 0;
}

static int btstack_control_gpcv2178_sleep(void)
{
    if (btstack_control_power_callback != NULL) {
        btstack_control_power_callback(POWER_WILL_SLEEP);
    }
    return 0;
}

static int btstack_control_gpcv2178_wake(void)
{
    if (btstack_control_power_callback != NULL) {
        btstack_control_power_callback(POWER_WILL_WAKE_UP);
    }
    return 0;
}

static void btstack_control_gpcv2178_register_for_power_notifications(void (*cb)(POWER_NOTIFICATION_t event))
{
    btstack_control_power_callback = cb;
}

const btstack_control_t btstack_control_gpcv2178 = {
    &btstack_control_gpcv2178_init,
    &btstack_control_gpcv2178_on,
    &btstack_control_gpcv2178_off,
    &btstack_control_gpcv2178_sleep,
    &btstack_control_gpcv2178_wake,
    &btstack_control_gpcv2178_register_for_power_notifications
};

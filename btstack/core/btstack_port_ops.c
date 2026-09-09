#include "adapter/btstack_port_ops.h"

#include <string.h>

static btstack_port_ops_t btstack_port_ops_table;

int btstack_port_ops_set_ops(const btstack_port_ops_t *ops)
{
    if (ops == NULL) {
        memset(&btstack_port_ops_table, 0, sizeof(btstack_port_ops_table));
        return BTSTACK_STATUS_OK;
    }

    if ((ops->system.enter_critical == NULL) !=
        (ops->system.exit_critical == NULL)) {
        return BTSTACK_STATUS_INVALID_ARGUMENT;
    }

    btstack_port_ops_table = *ops;
    return BTSTACK_STATUS_OK;
}

const btstack_port_ops_t *btstack_port_ops_get_ops(void)
{
    return &btstack_port_ops_table;
}

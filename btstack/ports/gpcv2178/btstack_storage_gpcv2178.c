#include "adapter/btstack_port_ops.h"
#include "project.h"

int btstack_storage_gpcv2178_read(void *context, btstack_port_ops_storage_key_t key, uint8_t tag, uint8_t *data, uint16_t *data_len)
{
    (void)context;
    (void)key;
    (void)tag;
    (void)data;
    if (data_len != NULL) {
        *data_len = 0;
    }
    return -1;
}

int btstack_storage_gpcv2178_write(void *context, btstack_port_ops_storage_key_t key, uint8_t tag, const uint8_t *data, uint16_t data_len)
{
    (void)context;
    (void)key;
    (void)tag;
    (void)data;
    (void)data_len;
    return 0;
}

int btstack_storage_gpcv2178_delete(void *context, btstack_port_ops_storage_key_t key, uint8_t tag)
{
    (void)context;
    (void)key;
    (void)tag;
    return 0;
}

#include "atbm_wifi.h"
static atbm_wifi_connection_info_t g_connection_info;
static atbm_wifi_ip_info_t g_ip_info;
static atbm_wifi_client_info_t g_client_info;
int atbm_public_header_probe(void)
{
    return (int)sizeof(g_connection_info) +
        (int)sizeof(g_ip_info) +
        (int)sizeof(g_client_info);
}

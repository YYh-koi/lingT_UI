#include <stddef.h>

#include "lwip/tcpip.h"

#include "atbm_lwip_port.h"

void atbm_lwip_system_init(void)
{
	tcpip_init(NULL, NULL);
}

void atbm_lwip_system_deinit(void)
{
	tcpip_deinit();
}

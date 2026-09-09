#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__

#include "lwip/err.h"
#include "lwip/netif.h"

struct link_struct {
    struct netif *netif;
    void (*link_change)(struct netif *netif);
};

extern netif_init_fn eth_ethernetif_init();
extern netif_init_fn eth_ethernetif_uninit();
extern void ethernetif_update_config(struct netif *netif);
extern void ethernetif_set_link(void const *argument);

#endif

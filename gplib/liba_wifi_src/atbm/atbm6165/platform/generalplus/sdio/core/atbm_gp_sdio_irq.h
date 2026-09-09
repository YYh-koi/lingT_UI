#ifndef __ATBM_GP_SDIO_IRQ_H__
#define __ATBM_GP_SDIO_IRQ_H__

#include "atbm_gp_sdio_card.h"

int atbm_gp_sdio_claim_irq(struct sdio_func *func, void(*handler)(struct sdio_func *));
int atbm_gp_sdio_release_irq(struct sdio_func *func);
void atbm_gp_sdio_irq_thread(void const* exinf);
#endif

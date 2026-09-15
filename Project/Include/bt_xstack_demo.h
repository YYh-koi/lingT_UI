#ifndef BT_XSTACK_DEMO_H
#define BT_XSTACK_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

int bt_xstack_demo_start(void);
/* Re-pull PBAP data for the currently connected HFP phone, if any. */
int bt_xstack_demo_phonebook_sync(void);

#ifdef __cplusplus
}
#endif

#endif

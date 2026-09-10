#ifndef UI_DASHBOARD_H
#define UI_DASHBOARD_H

#include <stdbool.h>

void ui_dashboard_create(void);
void ui_dashboard_set_track(const char *title, const char *artist, bool playing);
void ui_dashboard_set_call(const char *number, bool ringing, bool active);

#endif

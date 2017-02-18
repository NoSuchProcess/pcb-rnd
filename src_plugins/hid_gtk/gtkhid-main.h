#ifndef PCB_GTKHID_MAIN_H
#define PCB_GTKHID_MAIN_H

#include "conf_hid.h"
extern conf_hid_id_t ghid_conf_id;

void ghid_notify_gui_is_up(void);
extern int gtkhid_active;

void gtkhid_begin(void);
void gtkhid_end(void);

int ghid_control_is_pressed(void);
int ghid_mod1_is_pressed(void);
int ghid_shift_is_pressed(void);

#endif

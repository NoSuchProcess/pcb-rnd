#ifndef PCB_GTKHID_MAIN_H
#define PCB_GTKHID_MAIN_H

#include "conf_hid.h"

extern int gtkhid_active;

int ghid_control_is_pressed(void);
int ghid_mod1_is_pressed(void);
int ghid_shift_is_pressed(void);

#endif

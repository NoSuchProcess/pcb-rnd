#ifndef PCB_HID_GTK_GTKHID_H
#define PCB_HID_GTK_GTKHID_H

#include "conf_hid.h"

void ghid_notify_gui_is_up(void);

extern int gtkhid_active;
void gtkhid_begin(void);
void gtkhid_end(void);

extern conf_hid_id_t ghid_conf_id;

#endif /* PCB_HID_GTK_GTKHID_H */

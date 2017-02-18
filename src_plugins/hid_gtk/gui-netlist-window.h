#include "pcb_bool.h"
#include "unit.h"

void ghid_netlist_window_create(void);
void ghid_netlist_window_show(pcb_bool raise);
void ghid_netlist_window_update(pcb_bool init_nodes);

/* Temporary back references to hid_gtk */
void ghid_invalidate_all();
void ghid_cancel_lead_user(void);
void ghid_lead_user_to_location(pcb_coord_t x, pcb_coord_t y);
extern const char *ghid_cookie;


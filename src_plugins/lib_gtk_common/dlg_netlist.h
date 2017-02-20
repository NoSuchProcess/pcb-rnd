#ifndef PCB_GTK_DLG_NETLIST_H
#define PCB_GTK_DLG_NETLIST_H

#include "pcb_bool.h"
#include "unit.h"

/** Creates or raises the _Netlist_ window dialog.
    \param raise    if `TRUE`, presents the window.
 */
void pcb_gtk_dlg_netlist_show(pcb_bool raise);

/** If code in PCB should change the netlist, call this to update
    what's in the netlist window.
    \param init_nodes    ?
*/
void pcb_gtk_dlg_netlist_update(pcb_bool init_nodes);

/* Temporary back references to hid_gtk */
void ghid_invalidate_all();
void ghid_cancel_lead_user(void);
void ghid_lead_user_to_location(pcb_coord_t x, pcb_coord_t y);
extern const char *ghid_cookie;

#endif /* PCB_GTK_DLG_NETLIST_H */

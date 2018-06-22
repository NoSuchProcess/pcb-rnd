#ifndef PCB_GTK_DLG_NETLIST_H
#define PCB_GTK_DLG_NETLIST_H

#include <libfungw/fungw.h>
#include "pcb_bool.h"
#include "unit.h"
#include "event.h"
#include "glue.h"

/* Creates or raises the _Netlist_ window dialog. */
void pcb_gtk_dlg_netlist_show(pcb_gtk_common_t *com, pcb_bool raise);

/* Called on netlist changes to keep the gui in sync */
void pcb_gtk_dlg_netlist_update(pcb_gtk_common_t *com, pcb_bool init_nodes);

void pcb_gtk_netlist_changed(pcb_gtk_common_t *com, void *user_data, int argc, pcb_event_arg_t argv[]);


/* Actions */
extern const char pcb_gtk_acts_netlistshow[];
extern const char pcb_gtk_acth_netlistshow[];
fgw_error_t pcb_gtk_act_netlistshow(pcb_gtk_common_t *com, int oargc, const char **oargv);

extern const char pcb_gtk_acts_netlistpresent[];
extern const char pcb_gtk_acth_netlistpresent[];
fgw_error_t pcb_gtk_act_netlistpresent(pcb_gtk_common_t *com, int oargc, const char **oargv);

#endif /* PCB_GTK_DLG_NETLIST_H */

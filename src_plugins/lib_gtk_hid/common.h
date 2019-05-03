#include "event.h"

#include "../src_plugins/lib_gtk_common/dlg_topwin.h"

/* code used by multiple different glue layers */
void pcb_gtk_tw_ranges_scale(pcb_gtk_topwin_t *tw);
void ghid_note_event_location(GdkEventButton *ev);

void ghid_interface_input_signals_connect(void);
void ghid_interface_input_signals_disconnect(void);

int ghid_shift_is_pressed();
int ghid_control_is_pressed();
int ghid_mod1_is_pressed();

void ghid_LayersChanged(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[]);
void ghid_LayervisChanged(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[]);


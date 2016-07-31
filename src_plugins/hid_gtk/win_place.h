#ifndef GHID_WIN_PLACE
#define GHID_WIN_PLACE
#include "gui.h"

typedef enum {
	WPLC_TOP,
	WPLC_LOG,
	WPLC_DRC,
	WPLC_LIBRARY,
	WPLC_NETLIST,
	WPLC_KEYREF,
	WPLC_PINOUT,
	WPLC_max
} wplc_win_t;

/* Place the window if it's enabled and there are coords in the config. */
void wplc_place(wplc_win_t id, GtkWidget *win);

/* query window current window sizes and update wgeo cache */
void wplc_config_event(GtkWidget *win, long *cx, long *cy, long *cw, long *ch);
#endif

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include "win_place.h"
#include "conf_core.h"

#define CONF_PREFIX "plugins/hid_gtk/window_geometry/"
static const char *conf_prefix[WPLC_max] = { /* order DOES matter */
	"top_",
	"log_",
	"drc_",
	"library_",
	"netlist_",
	"keyref_",
	"pinout_",
	"search_"
};

static GtkWidget *wplc_windows[WPLC_max];

/* true if the given configuration item has exactly one integer value */
#define HAVE(native) ((native != NULL) && ((native)->used == 1) && ((native)->type == CFN_INTEGER))
#define HAVE_NZ(native) ((HAVE(native)) && ((native)->val.integer[0] != 0))

void wplc_place(wplc_win_t id, GtkWidget *new_win)
{
	char path[128], *pe;
	conf_native_t *nx, *ny, *nw, *nh;
	GtkWidget *win;

	if ((id < 0) || (id >= WPLC_max))
		return; /* invalid window */

	if (!conf_core.editor.auto_place) {
		if (new_win != NULL) /* remember window widgets in case editor.auto_place gets enabled later */
			wplc_windows[id] = new_win;
		return; /* feature disabled */
	}

	/* build base path for the specific window */
	pe = path;
	strcpy(pe, CONF_PREFIX);      pe += strlen(CONF_PREFIX);
	strcpy(pe, conf_prefix[id]);  pe += strlen(conf_prefix[id]);

	/* query each parameter */
	strcpy(pe, "height"); nh = conf_get_field(path);
	strcpy(pe, "width");  nw = conf_get_field(path);
	strcpy(pe, "x");      nx = conf_get_field(path);
	strcpy(pe, "y");      ny = conf_get_field(path);

	if (new_win != NULL) {
		wplc_windows[id] = new_win;
		win = new_win;
		/* for new windows set hint */
		if (HAVE(nw) && HAVE(nh))
			gtk_window_set_default_size(GTK_WINDOW(win), nw->val.integer[0], nh->val.integer[0]);
		if (HAVE(nx) && HAVE(ny))
			gtk_window_move(GTK_WINDOW(win), nx->val.integer[0], ny->val.integer[0]);
		else
			gtk_window_move(GTK_WINDOW(win), 10, 10); /* original behaviour */
	}
	else {
		win = wplc_windows[id];
		if (win != NULL) {
			if (HAVE_NZ(nw) && HAVE_NZ(nh))
				gtk_window_resize(GTK_WINDOW(win), nw->val.integer[0], nh->val.integer[0]);
			if (HAVE(nx) && HAVE(ny))
				gtk_window_move(GTK_WINDOW(win), nx->val.integer[0], ny->val.integer[0]);
		}
	}
}
#undef HAVE

void wplc_config_event(GtkWidget *win, GdkEventConfigure *ev, long *cx, long *cy, long *cw, long *ch)
{
	gboolean new_x, new_y, new_w, new_h;

	new_x = (*cx != ev->x);
	new_y = (*cy != ev->y);
	new_w = (*cw != ev->width);
	new_h = (*ch != ev->height);

	*cx = ev->x;
	*cy = ev->y;
	*cw = ev->width;
	*ch = ev->height;

	if (new_x || new_y || new_w || new_h)
		hid_gtk_wgeo_update();
}

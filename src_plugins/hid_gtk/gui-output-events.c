/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* This file written by Bill Wilson for the PCB Gtk port */

#include "config.h"
#include "conf_core.h"

#include "hid_cfg.h"
#include "gui-top-window.h"

#include <gdk/gdkkeysyms.h>

#include "action_helper.h"
#include "crosshair.h"
#include "draw.h"
#include "error.h"
#include "layer.h"
#include "find.h"
#include "search.h"
#include "rats.h"
#include "gtkhid-main.h"
#include "gui-top-window.h"

#include "../src_plugins/lib_gtk_common/bu_status_line.h"
#include "../src_plugins/lib_gtk_common/in_mouse.h"
#include "../src_plugins/lib_gtk_common/in_keyboard.h"
#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"

#warning TODO: remove
#include "gui.h"


void ghid_port_ranges_changed(pcb_gtk_topwin_t *tw)
{
	GtkAdjustment *h_adj, *v_adj;

	h_adj = gtk_range_get_adjustment(GTK_RANGE(tw->h_range));
	v_adj = gtk_range_get_adjustment(GTK_RANGE(tw->v_range));
	gport->view.x0 = gtk_adjustment_get_value(h_adj);
	gport->view.y0 = gtk_adjustment_get_value(v_adj);

	ghid_invalidate_all();
}

/* Do scrollbar scaling based on current port drawing area size and
   |  overall PCB board size.
 */
void pcb_gtk_tw_ranges_scale(pcb_gtk_topwin_t *tw)
{
	/* Update the scrollbars with PCB units.  So Scale the current
	   |  drawing area size in pixels to PCB units and that will be
	   |  the page size for the Gtk adjustment.
	 */
	pcb_gtk_zoom_post(&gport->view);

	pcb_gtk_zoom_adjustment(gtk_range_get_adjustment(GTK_RANGE(tw->h_range)), gport->view.width, PCB->MaxWidth);
	pcb_gtk_zoom_adjustment(gtk_range_get_adjustment(GTK_RANGE(tw->v_range)), gport->view.height, PCB->MaxHeight);
}

gboolean ghid_idle_cb(void *topwin)
{
	pcb_gtk_topwin_t *tw = topwin;
	if (conf_core.editor.mode == PCB_MODE_NO)
		pcb_crosshair_set_mode(PCB_MODE_ARROW);
	tw->com->mode_cursor_main(conf_core.editor.mode);
	if (tw->mode_btn.settings_mode != conf_core.editor.mode) {
		ghid_mode_buttons_update();
	}
	tw->mode_btn.settings_mode = conf_core.editor.mode;
	return FALSE;
}

gboolean ghid_port_key_release_cb(GtkWidget * drawing_area, GdkEventKey * kev, pcb_gtk_topwin_t *tw)
{
	gint ksym = kev->keyval;

	if (ghid_is_modifier_key_sym(ksym))
		ghid_note_event_location(NULL);

	pcb_adjust_attached_objects();
	ghid_invalidate_all();
	g_idle_add(ghid_idle_cb, tw);
	return FALSE;
}

void ghid_confchg_line_refraction(conf_native_t *cfg)
{
	/* test if PCB struct doesn't exist at startup */
	if (!PCB)
		return;
	ghid_set_status_line_label();
}

void ghid_confchg_all_direction_lines(conf_native_t *cfg)
{
	/* test if PCB struct doesn't exist at startup */
	if (!PCB)
		return;
	ghid_set_status_line_label();
}

void ghid_confchg_flip(conf_native_t *cfg)
{
	/* test if PCB struct doesn't exist at startup */
	if (!PCB)
		return;
	ghid_set_status_line_label();
}

void ghid_confchg_fullscreen(conf_native_t *cfg)
{
	if (gtkhid_active)
		ghid_fullscreen_apply(&ghidgui->topwin);
}


void ghid_confchg_checkbox(conf_native_t *cfg)
{
	if (gtkhid_active)
		ghid_update_toggle_flags(&ghidgui->topwin);
}

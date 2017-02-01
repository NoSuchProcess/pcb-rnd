/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 */

/* This file was originally written by Bill Wilson for the PCB Gtk port */

#include "config.h"
#include "conf_core.h"

#include "math_helper.h"
#include "crosshair.h"
#include "pcb-printf.h"
#include "compat_nls.h"

/*FIXME: needed for ghidgui , but need to be removed */
#include "gui.h"

const char *ghid_cookie = "gtk hid";
const char *ghid_menu_cookie = "gtk hid menu";


static void ghid_label_set_markup(GtkWidget * label, const gchar * text)
{
	if (label)
		gtk_label_set_markup(GTK_LABEL(label), text ? text : "");
}

void ghid_status_line_set_text(const gchar * text)
{
	if (ghidgui->command_entry_status_line_active)
		return;

	ghid_label_set_markup(ghidgui->status_line_label, text);
}

void ghid_cursor_position_label_set_text(gchar * text)
{
	ghid_label_set_markup(ghidgui->cursor_position_absolute_label, text);
}

void ghid_cursor_position_relative_label_set_text(gchar * text)
{
	ghid_label_set_markup(ghidgui->cursor_position_relative_label, text);
}

/* ---------------------------------------------------------------------------
 * output of status line
 */
void ghid_set_status_line_label(void)
{
	const gchar *flag = conf_core.editor.all_direction_lines
		? "*" : (conf_core.editor.line_refraction == 0 ? "X" : (conf_core.editor.line_refraction == 1 ? "_/" : "\\_"));
	char *text = pcb_strdup_printf(_("%m+<b>view</b>=%s  "
																	 "<b>grid</b>=%$mS  "
																	 "%s%s  "
																	 "<b>line</b>=%mS  "
																	 "<b>via</b>=%mS (%mS)  %s"
																	 "<b>clearance</b>=%mS  " "<b>text</b>=%i%%  " "<b>buffer</b>=#%i"),
																 conf_core.editor.grid_unit->allow,
																 conf_core.editor.show_solder_side ? _("solder") : _("component"),
																 PCB->Grid,
																 flag, conf_core.editor.rubber_band_mode ? ",R  " : "  ",
																 conf_core.design.line_thickness,
																 conf_core.design.via_thickness,
																 conf_core.design.via_drilling_hole,
																 conf_hid_gtk.plugins.hid_gtk.compact_horizontal ? "\n" : "",
																 conf_core.design.clearance,
																 conf_core.design.text_scale, conf_core.editor.buffer_number + 1);

	ghid_status_line_set_text(text);
	free(text);
}

/* ---------------------------------------------------------------------------
 * output of cursor position
 */
void ghid_set_cursor_position_labels(void)
{
	char *text, sep = ' ';
	if (conf_hid_gtk.plugins.hid_gtk.compact_vertical)
		sep = '\n';

	if (pcb_marked.status) {
		pcb_coord_t dx = pcb_crosshair.X - pcb_marked.X;
		pcb_coord_t dy = pcb_crosshair.Y - pcb_marked.Y;
		pcb_coord_t r = pcb_distance(pcb_crosshair.X, pcb_crosshair.Y, pcb_marked.X, pcb_marked.Y);
		double a = atan2(dy, dx) * PCB_RAD_TO_DEG;


		text = pcb_strdup_printf(_("%m+r %-mS;%cphi %-.1f;%c%-mS %-mS"), conf_core.editor.grid_unit->allow, r, sep, a, sep, dx, dy);
		ghid_cursor_position_relative_label_set_text(text);
		free(text);
	}
	else {
		char text[64];
		sprintf(text, _("r __.__;%cphi __._;%c__.__ __.__"), sep, sep);
		ghid_cursor_position_relative_label_set_text(text);
	}


	text = pcb_strdup_printf("%m+%-mS%c%-mS", conf_core.editor.grid_unit->allow, pcb_crosshair.X, sep, pcb_crosshair.Y);
	ghid_cursor_position_label_set_text(text);
	free(text);
}

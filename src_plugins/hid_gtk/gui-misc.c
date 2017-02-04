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

void ghid_status_line_set_text(const gchar * text)
{
	if (ghidgui->command_entry_status_line_active)
		return;

	if (ghidgui->status_line_label != NULL)
		gtk_label_set_markup(GTK_LABEL(ghidgui->status_line_label), text ? text : "");
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


/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include "config.h"
#include "bu_status_line.h"
#include "conf_core.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "board.h"

GtkWidget *pcb_gtk_status_line_label_new(void)
{
	GtkWidget *label = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);

	return label;
}

void pcb_gtk_status_line_set_text(GtkWidget *status_line_label, const gchar * text)
{
	if (status_line_label != NULL)
		gtk_label_set_markup(GTK_LABEL(status_line_label), text ? text : "");
}

void pcb_gtk_status_line_update(GtkWidget *status_line_label, int compat_horiz)
{
	const gchar *flag = conf_core.editor.all_direction_lines
		? "*" : (conf_core.editor.line_refraction == 0 ? "X" : (conf_core.editor.line_refraction == 1 ? "_/" : "\\_"));
	char *text;
	text = pcb_strdup_printf(_(
		"%m+<b>view</b>=%s  "
		"<b>grid</b>=%$mS  "
		"<b>line</b>=%mS (%s%s) "
		"<b>via</b>=%mS (%mS)  %s"
		"<b>clearance</b>=%mS  "
		"<b>text</b>=%i%%  "
		"<b>buffer</b>=#%i"),
		conf_core.editor.grid_unit->allow, conf_core.editor.show_solder_side ? _("solder") : _("component"),
		PCB->Grid,
		conf_core.design.line_thickness, flag, conf_core.editor.rubber_band_mode ? ",R" : "",
		conf_core.design.via_thickness, conf_core.design.via_drilling_hole, compat_horiz ? "\n" : "",
		conf_core.design.clearance,
		conf_core.design.text_scale,
		conf_core.editor.buffer_number + 1);

	pcb_gtk_status_line_set_text(status_line_label, text);
	free(text);
}


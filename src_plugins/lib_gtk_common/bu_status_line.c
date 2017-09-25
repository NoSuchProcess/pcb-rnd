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
	GtkWidget *label = gtk_label_new("\n");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);

	return label;
}

void pcb_gtk_status_line_set_text(GtkWidget *status_line_label, const gchar * text)
{
	if (status_line_label != NULL)
		gtk_label_set_markup(GTK_LABEL(status_line_label), text ? text : "");
}

static inline void gen_status_long(char *text, size_t text_size, int compat_horiz, const pcb_unit_t *unit)
{
	const gchar *flag = conf_core.editor.all_direction_lines
		? "*" : (conf_core.editor.line_refraction == 0 ? "X" : (conf_core.editor.line_refraction == 1 ? "_/" : "\\_"));

	pcb_snprintf(text, text_size, _(
		"%m+<b>view</b>=%s  "
		"<b>grid</b>=%$mS  "
		"<b>line</b>=%mS (%s%s)%s"
		"<b>via</b>=%mS (%mS)  "
		"<b>clearance</b>=%mS  "
		"<b>text</b>=%i%%  "
		"<b>buffer</b>=#%i"),
		unit->allow, conf_core.editor.show_solder_side ? _("solder") : _("component"),
		PCB->Grid,
		conf_core.design.line_thickness, flag, conf_core.editor.rubber_band_mode ? ",R" : "", compat_horiz ? "\n" : "",
		conf_core.design.via_thickness, conf_core.design.via_drilling_hole, 
		conf_core.design.clearance,
		conf_core.design.text_scale,
		conf_core.editor.buffer_number + 1);
}

static inline void gen_status_short(char *text, size_t text_size, int compat_horiz, const pcb_unit_t *unit)
{
	pcb_snprintf(text, text_size, _(
		"%m+"
		"grid=%$mS  "
		"line=%mS "
		"via=%mS (%mS) "
		"clearance=%mS"),
		unit->allow,
		PCB->Grid,
		conf_core.design.line_thickness,
		conf_core.design.via_thickness, conf_core.design.via_drilling_hole, 
		conf_core.design.clearance);
}

void pcb_gtk_status_line_update(GtkWidget *status_line_label, int compat_horiz)
{
	char text[1024];
	static const pcb_unit_t *unit_mm = NULL, *unit_mil;
	const pcb_unit_t *unit_inv;

	if (status_line_label == NULL)
		return;

	if (unit_mm == NULL) { /* cache mm and mil units to save on the lookups */
		unit_mm  = get_unit_struct("mm");
		unit_mil = get_unit_struct("mil");
	}

	gen_status_long(text, sizeof(text), compat_horiz, conf_core.editor.grid_unit);
	pcb_gtk_status_line_set_text(status_line_label, text);

	if (conf_core.editor.grid_unit == unit_mm)
		unit_inv = unit_mil;
	else
		unit_inv = unit_mm;

	gen_status_short(text, sizeof(text), compat_horiz, unit_inv);
	gtk_widget_set_tooltip_text(GTK_WIDGET(status_line_label), text);
}


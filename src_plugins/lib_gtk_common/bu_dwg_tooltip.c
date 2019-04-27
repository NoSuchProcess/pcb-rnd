/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
 *  pcb-rnd Copyright (C) 2017,2019 Tibor 'Igor2' Palinkas
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* This file was originally written by Bill Wilson for the PCB Gtk port
   and refactored by Tibor 'Igor2' Palinkas for pcb-rnd */

/* Drawing area tooltips */

#include "config.h"
#include "actions.h"
#include "bu_dwg_tooltip.h"

#define TOOLTIP_UPDATE_DELAY 200

static int tooltip_update_timeout_id = 0;
gboolean pcb_gtk_dwg_tooltip_check_object(GtkWidget *drawing_area, pcb_coord_t crosshairx, pcb_coord_t crosshairy)
{
	const char *description;
	fgw_arg_t res, argv[3];

	/* Make sure the timer is not removed - we are called by the timer and it is
	   automatically removed because we are returning false */
	tooltip_update_timeout_id = 0;

	/* check if there are any pins or pads at that position */
	argv[1].type = FGW_COORD_;
	fgw_coord(&argv[1]) = crosshairx;
	argv[2].type = FGW_COORD_;
	fgw_coord(&argv[2]) = crosshairy;

	if (pcb_actionv_bin("DescribeLocation", &res, 3, argv) != 0)
		return FALSE;

	description = res.val.cstr;
	if (description == NULL)
		return FALSE;

	gtk_widget_set_tooltip_text(drawing_area, description);

	return FALSE;
}

void pcb_gtk_dwg_tooltip_cancel_update(void)
{
	if (tooltip_update_timeout_id)
		g_source_remove(tooltip_update_timeout_id);
	tooltip_update_timeout_id = 0;
}

/* FIXME: If the GHidPort is ever destroyed, we must call
 * pcb_gtk_dwg_tooltip_cancel_update()
 * fire after the data it utilises has been free'd.
 */
void pcb_gtk_dwg_tooltip_queue(GtkWidget *drawing_area, GSourceFunc cb, void *ctx)
{
	/* Zap the old tool-tip text and force it to be removed from the screen */
	gtk_widget_set_tooltip_text(drawing_area, NULL);
	gtk_widget_trigger_tooltip_query(drawing_area);

	pcb_gtk_dwg_tooltip_cancel_update();

	tooltip_update_timeout_id = g_timeout_add(TOOLTIP_UPDATE_DELAY, cb, ctx);
}

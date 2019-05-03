/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2019 Tibor 'Igor2' Palinkas
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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include "win_place.h"
#include "hidlib_conf.h"
#include "event.h"

TODO("DAD: the only legitimate user is top window, move this code there once all other gtk-only dialogs are gone");
void pcb_gtk_winplace(pcb_hidlib_t *hidlib, GtkWidget *dialog, const char *id)
{
	int plc[4] = {-1, -1, -1, -1};

	pcb_event(hidlib, PCB_EVENT_DAD_NEW_DIALOG, "psp", NULL, id, plc);

	if (pcbhl_conf.editor.auto_place) {
		if ((plc[2] > 0) && (plc[3] > 0))
			gtk_window_resize(GTK_WINDOW(dialog), plc[2], plc[3]);
		if ((plc[0] >= 0) && (plc[1] >= 0))
			gtk_window_move(GTK_WINDOW(dialog), plc[0], plc[1]);
	}
}

gint pcb_gtk_winplace_cfg(pcb_hidlib_t *hidlib, GtkWidget *widget, void *ctx, const char *id)
{
	GtkAllocation allocation;

	gtk_widget_get_allocation(widget, &allocation);

	/* For whatever reason, get_allocation doesn't set these. Gtk. */
	gtk_window_get_position(GTK_WINDOW(widget), &allocation.x, &allocation.y);

	pcb_event(hidlib, PCB_EVENT_DAD_NEW_GEO, "psiiii", ctx, id,
		(int)allocation.x, (int)allocation.y, (int)allocation.width, (int)allocation.height);

	return 0;
}

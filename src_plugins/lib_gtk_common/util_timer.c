/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include <glib.h>
#include "util_timer.h"
#include "glue_common.h"

typedef struct {
	void (*func) (pcb_hidval_t);
	guint id;
	pcb_hidval_t user_data;
	pcb_gtk_common_t *com;
} GuiTimer;

	/* We need a wrapper around the hid timer because a gtk timer needs
	   |  to return FALSE else the timer will be restarted.
	 */
static gboolean ghid_timer(GuiTimer * timer)
{
	(*timer->func) (timer->user_data);
	pcb_gtk_mode_cursor_main();
	return FALSE;									/* Turns timer off */
}

pcb_hidval_t pcb_gtk_add_timer(pcb_gtk_common_t *com, void (*func) (pcb_hidval_t user_data), unsigned long milliseconds, pcb_hidval_t user_data)
{
	GuiTimer *timer = g_new0(GuiTimer, 1);
	pcb_hidval_t ret;

	timer->func = func;
	timer->user_data = user_data;
	timer->com = com;
	timer->id = g_timeout_add(milliseconds, (GSourceFunc) ghid_timer, timer);
	ret.ptr = (void *) timer;
	return ret;
}

void ghid_stop_timer(pcb_hidval_t timer)
{
	void *ptr = timer.ptr;

	g_source_remove(((GuiTimer *) ptr)->id);
	g_free(ptr);
}

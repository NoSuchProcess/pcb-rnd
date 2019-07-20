/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include "util_watch.h"
#include "in_mouse.h"
#include "glue_common.h"

typedef struct pcb_gtk_watch_s {
	pcb_bool (*func)(pcb_hidval_t, int, unsigned int, pcb_hidval_t);
	pcb_hidval_t user_data;
	int fd;
	GIOChannel *channel;
	gint id;
	pcb_gtk_t *gctx;
} pcb_gtk_watch_t;

/* We need a wrapper around the hid file watch to pass the correct flags */
static gboolean ghid_watch(GIOChannel *source, GIOCondition condition, gpointer data)
{
	unsigned int pcb_condition = 0;
	pcb_hidval_t x;
	pcb_gtk_watch_t *watch = (pcb_gtk_watch_t *)data;
	pcb_bool res;

	if (condition & G_IO_IN)
		pcb_condition |= PCB_WATCH_READABLE;
	if (condition & G_IO_OUT)
		pcb_condition |= PCB_WATCH_WRITABLE;
	if (condition & G_IO_ERR)
		pcb_condition |= PCB_WATCH_ERROR;
	if (condition & G_IO_HUP)
		pcb_condition |= PCB_WATCH_HANGUP;

	x.ptr = (void *)watch;
	res = watch->func(x, watch->fd, pcb_condition, watch->user_data);

	pcb_gtk_mode_cursor_main();

	return res;
}

pcb_hidval_t pcb_gtk_watch_file(pcb_gtk_t *gctx, int fd, unsigned int condition,
	pcb_bool (*func)(pcb_hidval_t watch, int fd, unsigned int condition, pcb_hidval_t user_data),
	pcb_hidval_t user_data)
{
	pcb_gtk_watch_t *watch = g_new0(pcb_gtk_watch_t, 1);
	pcb_hidval_t ret;
	unsigned int glib_condition = 0;

	if (condition & PCB_WATCH_READABLE)
		glib_condition |= G_IO_IN;
	if (condition & PCB_WATCH_WRITABLE)
		glib_condition |= G_IO_OUT;
	if (condition & PCB_WATCH_ERROR)
		glib_condition |= G_IO_ERR;
	if (condition & PCB_WATCH_HANGUP)
		glib_condition |= G_IO_HUP;

	watch->func = func;
	watch->user_data = user_data;
	watch->fd = fd;
	watch->channel = g_io_channel_unix_new(fd);
	watch->id = g_io_add_watch(watch->channel, (GIOCondition) glib_condition, ghid_watch, watch);
	watch->gctx = gctx;

	ret.ptr = (void *) watch;
	return ret;
}

void pcb_gtk_unwatch_file(pcb_hid_t *hid, pcb_hidval_t data)
{
	pcb_gtk_watch_t *watch = (pcb_gtk_watch_t *)data.ptr;

	g_io_channel_shutdown(watch->channel, TRUE, NULL);
	g_io_channel_unref(watch->channel);
	g_free(watch);
}

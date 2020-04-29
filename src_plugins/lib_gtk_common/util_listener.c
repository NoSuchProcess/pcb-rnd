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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#define _POSIX_SOURCE
#include "config.h"

#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>

#include <librnd/core/hid.h>
#include <librnd/core/actions.h>
#include <librnd/core/compat_misc.h>

#include "util_listener.h"

static gboolean ghid_listener_cb(GIOChannel * source, GIOCondition condition, gpointer data)
{
	pcb_gtk_t *gctx = data;
	GIOStatus status;
	gchar *str;
	gsize len;
	gsize term;
	GError *err = NULL;


	if (condition & G_IO_HUP) {
		rnd_message(RND_MSG_ERROR, "gtk listener: Read end of pipe died!\n");
		return FALSE;
	}

	if (condition == G_IO_IN) {
		status = g_io_channel_read_line(source, &str, &len, &term, &err);
		switch (status) {
		case G_IO_STATUS_NORMAL:
			rnd_parse_actions(gctx->hidlib, str);
			g_free(str);
			break;

		case G_IO_STATUS_ERROR:
			rnd_message(RND_MSG_ERROR, "gtk listener: status from g_io_channel_read_line\n");
			return FALSE;
			break;

		case G_IO_STATUS_EOF:
			rnd_message(RND_MSG_ERROR, "gtk listener: Input pipe returned EOF.  The --listen option is \n" "probably not running anymore in this session.\n");
			return FALSE;
			break;

		case G_IO_STATUS_AGAIN:
			rnd_message(RND_MSG_ERROR, "gtk listener: AGAIN status from g_io_channel_read_line\n");
			return FALSE;
			break;

		default:
			rnd_message(RND_MSG_ERROR, "gtk listener: unhandled error case\n");
			return FALSE;
			break;
		}

	}
	else
		fprintf(stderr, "Unknown condition in ghid_listener_cb\n");

	return TRUE;
}

void pcb_gtk_create_listener(pcb_gtk_t *gctx)
{
	GIOChannel *channel;
	int fd = rnd_fileno(stdin);

	channel = g_io_channel_unix_new(fd);
	g_io_add_watch(channel, G_IO_IN, ghid_listener_cb, gctx);
}


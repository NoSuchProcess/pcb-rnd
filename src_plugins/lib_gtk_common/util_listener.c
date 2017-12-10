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

#define _POSIX_SOURCE
#include "config.h"

#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>

#include "hid.h"
#include "hid_actions.h"
#include "compat_misc.h"

static gboolean ghid_listener_cb(GIOChannel * source, GIOCondition condition, gpointer data)
{
	GIOStatus status;
	gchar *str;
	gsize len;
	gsize term;
	GError *err = NULL;


	if (condition & G_IO_HUP) {
		pcb_gui->log("Read end of pipe died!\n");
		return FALSE;
	}

	if (condition == G_IO_IN) {
		status = g_io_channel_read_line(source, &str, &len, &term, &err);
		switch (status) {
		case G_IO_STATUS_NORMAL:
			pcb_hid_parse_actions(str);
			g_free(str);
			break;

		case G_IO_STATUS_ERROR:
			pcb_gui->log("ERROR status from g_io_channel_read_line\n");
			return FALSE;
			break;

		case G_IO_STATUS_EOF:
			pcb_gui->log("Input pipe returned EOF.  The --listen option is \n" "probably not running anymore in this session.\n");
			return FALSE;
			break;

		case G_IO_STATUS_AGAIN:
			pcb_gui->log("AGAIN status from g_io_channel_read_line\n");
			return FALSE;
			break;

		default:
			fprintf(stderr, "ERROR:  unhandled case in ghid_listener_cb\n");
			return FALSE;
			break;
		}

	}
	else
		fprintf(stderr, "Unknown condition in ghid_listener_cb\n");

	return TRUE;
}

void pcb_gtk_create_listener(void)
{
	GIOChannel *channel;
	int fd = pcb_fileno(stdin);

	channel = g_io_channel_unix_new(fd);
	g_io_add_watch(channel, G_IO_IN, ghid_listener_cb, NULL);
}


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

#include "config.h"

#include <glib.h>
#include <gio/gio.h>

#include "util_ext_chg.h"

#include "board.h"
#include "pcb_bool.h"

pcb_bool check_externally_modified(pcb_gtk_ext_chg_t *ec)
{
	GFile *file;
	GFileInfo *info;
	GTimeVal timeval;

	/* Treat zero time as a flag to indicate we've not got an mtime yet */
	if (PCB->hidlib.filename == NULL || (ec->our_mtime.tv_sec == 0 && ec->our_mtime.tv_usec == 0))
		return pcb_false;

	file = g_file_new_for_path(PCB->hidlib.filename);
	info = g_file_query_info(file, G_FILE_ATTRIBUTE_TIME_MODIFIED, G_FILE_QUERY_INFO_NONE, NULL, NULL);
	g_object_unref(file);

	if (info == NULL || !g_file_info_has_attribute(info, G_FILE_ATTRIBUTE_TIME_MODIFIED))
		return pcb_false;

	g_file_info_get_modification_time(info, &timeval);	/*&ec->last_seen_mtime); */
	g_object_unref(info);

	/* Ignore when the file on disk is the same age as when we last looked */
	if (timeval.tv_sec == ec->last_seen_mtime.tv_sec && timeval.tv_usec == ec->last_seen_mtime.tv_usec)
		return pcb_false;

	ec->last_seen_mtime = timeval;

	return (ec->last_seen_mtime.tv_sec > ec->our_mtime.tv_sec) ||
		(ec->last_seen_mtime.tv_sec == ec->our_mtime.tv_sec &&
		 ec->last_seen_mtime.tv_usec > ec->our_mtime.tv_usec);
}

void update_board_mtime_from_disk(pcb_gtk_ext_chg_t *ec)
{
	GFile *file;
	GFileInfo *info;

	ec->our_mtime.tv_sec = 0;
	ec->our_mtime.tv_usec = 0;
	ec->last_seen_mtime = ec->our_mtime;

	if (PCB->hidlib.filename == NULL)
		return;

	file = g_file_new_for_path(PCB->hidlib.filename);
	info = g_file_query_info(file, G_FILE_ATTRIBUTE_TIME_MODIFIED, G_FILE_QUERY_INFO_NONE, NULL, NULL);
	g_object_unref(file);

	if (info == NULL || !g_file_info_has_attribute(info, G_FILE_ATTRIBUTE_TIME_MODIFIED))
		return;

	g_file_info_get_modification_time(info, &ec->our_mtime);
	g_object_unref(info);

	ec->last_seen_mtime = ec->our_mtime;
}

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include <genvector/gds_char.h>

#include "board.h"
#include "conf_core.h"

static gds_t title_buf;
static int gui_inited = 0, brd_changed;

static void update_title(void)
{
	const char *filename, *name;

	if ((pcb_gui == NULL) || (pcb_gui->set_top_title == NULL) || (!gui_inited))
		return;
pcb_trace("TITLE!!\n");
	if ((PCB->hidlib.name == NULL) || (*PCB->hidlib.name == '\0'))
		name = "Unnamed";
	else
		name = PCB->hidlib.name;

	if ((PCB->hidlib.filename == NULL) || (*PCB->hidlib.filename == '\0'))
		filename = "<board with no file name or format>";
	else
		filename = PCB->hidlib.filename;

	title_buf.used = 0;
	pcb_append_printf(&title_buf, "%s%s (%s) - %s - pcb-rnd", PCB->Changed ? "*" : "", name, filename, PCB->is_footprint ? "footprint" : "board");
	pcb_gui->set_top_title(&PCB->hidlib, title_buf.array);
}

static void pcb_title_board_changed_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	brd_changed = 0;
	update_title();
}

static void pcb_title_meta_changed_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (brd_changed == PCB->Changed)
		return;
	brd_changed = PCB->Changed;
	update_title();
}

static void pcb_title_gui_init_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	gui_inited = 1;
	update_title();
}


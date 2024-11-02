/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019,2022 Tibor 'Igor2' Palinkas
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

#define PCB do not use

static gds_t title_buf;
static int gui_inited = 0, brd_changed;

static void update_title(rnd_design_t *hl, int changed, int is_footprint)
{
	const char *filename, *name;

	if ((rnd_gui == NULL) || (rnd_gui->set_top_title == NULL) || (!gui_inited))
		return;

	if ((hl->name == NULL) || (*hl->name == '\0'))
		name = "Unnamed";
	else
		name = hl->name;

	if ((hl->loadname == NULL) || (*hl->loadname == '\0'))
		filename = "<board with no file name or format>";
	else
		filename = hl->loadname;

	title_buf.used = 0;
	rnd_append_printf(&title_buf, "%s%s (%s) - %s - pcb-rnd", changed ? "*" : "", name, filename, is_footprint ? "footprint" : "board");
	rnd_gui->set_top_title(rnd_gui, title_buf.array);
}

static void pcb_title_board_changed_ev(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pcb_board_t *pcb = (pcb_board_t *)hidlib;
	brd_changed = 0;
	update_title(hidlib, pcb->Changed, pcb->is_footprint);
}

static char last_hl_name;

static void pcb_title_meta_changed_ev(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pcb_board_t *pcb = (pcb_board_t *)hidlib;
	if ((brd_changed == pcb->Changed) && (last_hl_name == hidlib->name))
		return;
	brd_changed = pcb->Changed;
	last_hl_name = hidlib->name;
	update_title(hidlib, pcb->Changed, pcb->is_footprint);
}

static void pcb_title_gui_init_ev(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pcb_board_t *pcb = (pcb_board_t *)hidlib;
	gui_inited = 1;
	update_title(hidlib, pcb->Changed, pcb->is_footprint);
}


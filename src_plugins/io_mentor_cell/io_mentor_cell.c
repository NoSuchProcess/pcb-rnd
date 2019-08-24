/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This module, io_mentor_cell, was written and is Copyright (C) 2016 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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
#include "plugins.h"
#include "plug_io.h"
#include "read.h"
#include "actions.h"

static pcb_plug_io_t io_mentor_cell;
static const char *mentor_cell_cookie = "mentor_cell plugin";

int io_mentor_cell_fmt(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, int wr, const char *fmt)
{
	if (strcmp(ctx->description, fmt) == 0)
		return 200;

	if ((strcmp(fmt, "mentor_cell") != 0) ||
		((typ & (~(PCB_IOT_FOOTPRINT | PCB_IOT_BUFFER | PCB_IOT_PCB))) != 0))
		return 0;

	return 100;
}

int pplg_check_ver_io_mentor_cell(int ver_needed) { return 0; }

void pplg_uninit_io_mentor_cell(void)
{
	pcb_remove_actions_by_cookie(mentor_cell_cookie);
	PCB_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_mentor_cell);
}

#include "dolists.h"

int pplg_init_io_mentor_cell(void)
{
	PCB_API_CHK_VER;

	/* register the IO hook */
	io_mentor_cell.plugin_data = NULL;
	io_mentor_cell.fmt_support_prio = io_mentor_cell_fmt;
	io_mentor_cell.test_parse = io_mentor_cell_test_parse;
	io_mentor_cell.parse_pcb = io_mentor_cell_read_pcb;
	io_mentor_cell.parse_footprint = NULL;
	io_mentor_cell.parse_font = NULL;
	io_mentor_cell.write_buffer = NULL;
	io_mentor_cell.write_footprint = NULL;
	io_mentor_cell.write_pcb = NULL;
	io_mentor_cell.default_fmt = "mentor_cell";
	io_mentor_cell.description = "Mentor graphics cell footprint lib";
	io_mentor_cell.save_preference_prio = 60;
	io_mentor_cell.default_extension = ".hkp";
	io_mentor_cell.fp_extension = ".hkp";
	io_mentor_cell.mime_type = "application/x-mentor_cell";


	PCB_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_mentor_cell);

	return 0;
}


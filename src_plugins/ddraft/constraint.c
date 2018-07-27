/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  2d drafting plugin: trim objects
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "crosshair.h"

typedef struct {
	double line_angle[32];
	
	int line_angle_len;
} ddraft_cnst_t;

static ddraft_cnst_t cons;

static void cnst_line_angle(ddraft_cnst_t *cn)
{
	if (cn->line_angle_len == 0)
		return;
	pcb_trace("line angle enforce: %d [%f]\n", cn->line_angle_len, cn->line_angle[0]);
}


static void cnst_line2(ddraft_cnst_t *cn)
{
	cnst_line_angle(cn);
}

static void cnst_enforce(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (pcb_crosshair.AttachedLine.State == PCB_CH_STATE_SECOND)
		cnst_line2(&cons);
}

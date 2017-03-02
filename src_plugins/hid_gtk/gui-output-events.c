/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
 *  pcb-rnd Copyright (C) 2016, 2017 Tibor 'Igor2' Palinkas
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#include "config.h"
#include "conf_core.h"
#include "gui.h"
#include "gtkhid-main.h"

void ghid_confchg_line_refraction(conf_native_t *cfg)
{
	/* test if PCB struct doesn't exist at startup */
	if (!PCB)
		return;
	ghid_set_status_line_label();
}

void ghid_confchg_all_direction_lines(conf_native_t *cfg)
{
	/* test if PCB struct doesn't exist at startup */
	if (!PCB)
		return;
	ghid_set_status_line_label();
}

void ghid_confchg_flip(conf_native_t *cfg)
{
	/* test if PCB struct doesn't exist at startup */
	if (!PCB)
		return;
	ghid_set_status_line_label();
}

void ghid_confchg_fullscreen(conf_native_t *cfg)
{
	if (gtkhid_active)
		ghid_fullscreen_apply(&ghidgui->topwin);
}


void ghid_confchg_checkbox(conf_native_t *cfg)
{
	if (gtkhid_active)
		ghid_update_toggle_flags(&ghidgui->topwin);
}

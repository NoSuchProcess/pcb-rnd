/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

/* Build an in-memory lihata document that represents the board then save it.
   A document is built for the merge-save. */

#include <stdio.h>
#include "config.h"
#include "data.h"
#include "macro.h"
#include "common.h"
#include "thermal.h"

static const char *thermal_style[] = {
	NULL,
	"diagonal-sharp",
	"horver-sharp",
	"solid",
	"diagonal-round",
	"horver-round"
	"noshape"
};

static const int thermal_style_bits[] = {
	0,
	PCB_THERMAL_ON | PCB_THERMAL_DIAGONAL | PCB_THERMAL_SHARP,
	PCB_THERMAL_ON | PCB_THERMAL_SHARP,
	PCB_THERMAL_ON | PCB_THERMAL_SOLID,
	PCB_THERMAL_ON | PCB_THERMAL_DIAGONAL | PCB_THERMAL_ROUND,
	PCB_THERMAL_ON | PCB_THERMAL_ROUND,
	PCB_THERMAL_ON
};

int io_lihata_resolve_thermal_style_old(const char *name, int ver)
{
	int n, omit = 0;
	char *end;

	if (name == NULL)
		return 0;

	if (ver < 6) /* old file versions did not have explicit no-shape */
		omit = 1;

	for(n = 1; n < PCB_ENTRIES(thermal_style) - omit; n++)
		if (strcmp(name, thermal_style[n]) == 0)
			return thermal_style_bits[n];

	n = strtol(name, &end, 10);
	if (*end == '\0') {
		if ((n >= 0) && (n < sizeof(thermal_style_bits) / sizeof(thermal_style_bits[0])))
			return thermal_style_bits[n];
	}

	return 0;
}

const char *io_lihata_thermal_style_old(int idx)
{
	if ((idx > 0) && (idx < PCB_ENTRIES(thermal_style)))
		return thermal_style[idx];
	return NULL;
}

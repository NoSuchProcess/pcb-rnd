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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* Build an in-memory lihata document that represents the board then save it.
   A document is built for the merge-save. */

#include <stdio.h>
#include "config.h"
#include "data.h"
#include "macro.h"
#include "common.h"

static const char *thermal_style[] = {
	NULL,
	"diagonal-sharp",
	"horver-sharp",
	"solid",
	"diagonal-round",
	"horver-round"
};

int io_lihata_resolve_thermal_style(const char *name)
{
	int n;
	char *end;

	if (name == NULL)
		return 0;

	for(n = 1; n < PCB_ENTRIES(thermal_style); n++)
		if (strcmp(name, thermal_style[n]) == 0)
			return n;

	n = strtol(name, &end, 10);
	if (*end == '\0')
		return n;

	return 0;
}

const char *io_lihata_thermal_style(int idx)
{
	if ((idx > 0) && (idx < PCB_ENTRIES(thermal_style)))
		return thermal_style[idx];
	return NULL;
}

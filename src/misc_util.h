/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,2006 Thomas Nau
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* misc - independent of PCB data types */

#ifndef	PCB_MISC_UTIL_H
#define	PCB_MISC_UTIL_H

#include "unit.h"
#include "pcb_bool.h"

double pcb_distance(double x1, double y1, double x2, double y2);
double pcb_distance2(double x1, double y1, double x2, double y2);	/* distance square */

enum pcb_unit_flags_e { PCB_UNIT_PERCENT = 1 };

typedef struct {
	const char *suffix;
	double scale;
	enum pcb_unit_flags_e flags;
} pcb_unit_list_t[];

/* Convert string to coords; if units is not NULL, it's the caller supplied unit
   string; absolute is set to false if non-NULL and val starts with + or -.
   success indicates whether the conversion was successful. */
double pcb_get_value(const char *val, const char *units, pcb_bool *absolute, pcb_bool *success);
double pcb_get_value_ex(const char *val, const char *units, pcb_bool * absolute, pcb_unit_list_t extra_units, const char *default_unit, pcb_bool *success);

char *pcb_concat(const char *first, ...); /* end with NULL */
int pcb_mem_any_set(unsigned char *ptr, int bytes);

char *pcb_strdup_strip_wspace(const char *S);

#endif

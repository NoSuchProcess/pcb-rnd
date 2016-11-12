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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* prototypes for misc routines - independent of PCB data types */

#ifndef	PCB_MISC_UTIL_H
#define	PCB_MISC_UTIL_H

#include "pcb_bool.h"

double Distance(double x1, double y1, double x2, double y2);
double Distance2(double x1, double y1, double x2, double y2);	/* distance square */

enum pcb_unit_flags_e { UNIT_PERCENT = 1 };

typedef struct {
	const char *suffix;
	double scale;
	enum pcb_unit_flags_e flags;
} pcb_unit_list_t[];

double GetValue(const char *, const char *, pcb_bool *, pcb_bool *success);
double GetValueEx(const char *, const char *, pcb_bool *, pcb_unit_list_t, const char *, pcb_bool *success);
Coord GetNum(char **s, const char *default_unit);


char *Concat(const char *, ...);	/* end with NULL */
int mem_any_set(unsigned char *ptr, int bytes);

char *StripWhiteSpaceAndDup(const char *S);

#endif

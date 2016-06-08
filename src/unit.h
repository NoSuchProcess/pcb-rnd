/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2011 Andrew Poelstra
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
 *  Contact addresses for paper mail and Email:
 *  Andrew Poelstra, 16966 60A Ave, V3S 8X5 Surrey, BC, Canada
 *  asp11@sfu.ca
 *
 */

#ifndef	PCB_UNIT_H
#define	PCB_UNIT_H

#include "config.h"

typedef COORD_TYPE Coord;				/* pcb base unit */
typedef double Angle;						/* degrees */

enum e_allow {
	NO_PRINT = 0,									/* suffixes we can read but not print (i.e., "inch") */
	ALLOW_NM = 1,
	ALLOW_UM = 2,
	ALLOW_MM = 4,
	ALLOW_CM = 8,
	ALLOW_M = 16,
	ALLOW_KM = 32,

	ALLOW_CMIL = 1024,
	ALLOW_MIL = 2048,
	ALLOW_IN = 4096,

	ALLOW_METRIC = ALLOW_NM | ALLOW_UM | ALLOW_MM | ALLOW_CM | ALLOW_M | ALLOW_KM,
	ALLOW_IMPERIAL = ALLOW_CMIL | ALLOW_MIL | ALLOW_IN,
	/* This is all units allowed in parse_l.l */
#if 0
	ALLOW_READABLE = ALLOW_NM | ALLOW_UM | ALLOW_MM | ALLOW_M | ALLOW_KM | ALLOW_CMIL | ALLOW_MIL | ALLOW_IN,
#else
	ALLOW_READABLE = ALLOW_CMIL,
#endif

	ALLOW_ALL = ~0
};

enum e_family { METRIC, IMPERIAL };
enum e_suffix { NO_SUFFIX, SUFFIX, FILE_MODE };

struct unit {
	int index;										/* Index into Unit[] list */
	const char *suffix;
	const char *in_suffix;				/* internationalized suffix */
	char printf_code;
	double scale_factor;
	enum e_family family;
	enum e_allow allow;
	int default_prec;
	/* used for gui spinboxes */
	double step_tiny;
	double step_small;
	double step_medium;
	double step_large;
	double step_huge;
	/* aliases -- right now we only need 1 ("inch"->"in"), add as needed */
	const char *alias[1];
};

struct increments {
	const char *suffix;
	/* key g and <shift>g value  */
	Coord grid;
	Coord grid_min;
	Coord grid_max;
	/* key s and <shift>s value  */
	Coord size;
	Coord size_min;
	Coord size_max;
	/* key l and <shift>l value  */
	Coord line;
	Coord line_min;
	Coord line_max;
	/* key k and <shift>k value  */
	Coord clear;
	Coord clear_min;
	Coord clear_max;
};

typedef struct unit Unit;
typedef struct increments Increments;
extern Unit Units[];
extern Increments increments[];

const Unit *get_unit_struct(const char *suffix);
const Unit *get_unit_list(void);
const Unit *get_unit_by_idx(int idx);
int get_n_units(void);
double coord_to_unit(const Unit *, Coord);
double unit_to_factor(const Unit * unit);
Coord unit_to_coord(const Unit *, double);
Increments *get_increments_struct(const char *suffix);


#endif

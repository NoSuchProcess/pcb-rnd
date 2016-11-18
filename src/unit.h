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

/* typedef ... pcb_coord_t;				pcb base unit, typedef'd in config.h */
typedef double pcb_angle_t;						/* degrees */

enum pcb_allow_e {
	PCB_UNIT_NO_PRINT = 0,									/* suffixes we can read but not print (i.e., "inch") */
	PCB_UNIT_ALLOW_NM = 1,
	PCB_UNIT_ALLOW_UM = 2,
	PCB_UNIT_ALLOW_MM = 4,
	PCB_UNIT_ALLOW_CM = 8,
	PCB_UNIT_ALLOW_M = 16,
	PCB_UNIT_ALLOW_KM = 32,

	PCB_UNIT_ALLOW_CMIL = 1024,
	PCB_UNIT_ALLOW_MIL = 2048,
	PCB_UNIT_ALLOW_IN = 4096,

	PCB_UNIT_ALLOW_DMIL = 8192, /* for kicad legacy decimil units */

	PCB_UNIT_ALLOW_METRIC = PCB_UNIT_ALLOW_NM | PCB_UNIT_ALLOW_UM | PCB_UNIT_ALLOW_MM | PCB_UNIT_ALLOW_CM | PCB_UNIT_ALLOW_M | PCB_UNIT_ALLOW_KM,
	PCB_UNIT_ALLOW_IMPERIAL = PCB_UNIT_ALLOW_DMIL | PCB_UNIT_ALLOW_CMIL | PCB_UNIT_ALLOW_MIL | PCB_UNIT_ALLOW_IN,
	/* This is all units allowed in parse_l.l */
#if 0
	PCB_UNIT_ALLOW_READABLE = PCB_UNIT_ALLOW_NM | PCB_UNIT_ALLOW_UM | PCB_UNIT_ALLOW_MM | PCB_UNIT_ALLOW_M | PCB_UNIT_ALLOW_KM | PCB_UNIT_ALLOW_CMIL | PCB_UNIT_ALLOW_MIL | PCB_UNIT_ALLOW_IN,
#else
	PCB_UNIT_ALLOW_READABLE = PCB_UNIT_ALLOW_CMIL,
#endif

	/* Used for pcb-printf %mS - should not include unusual units like km, cmil and dmil */
	PCB_UNIT_ALLOW_NATURAL = PCB_UNIT_ALLOW_NM | PCB_UNIT_ALLOW_UM | PCB_UNIT_ALLOW_MM | PCB_UNIT_ALLOW_M | PCB_UNIT_ALLOW_MIL | PCB_UNIT_ALLOW_IN,

	PCB_UNIT_ALLOW_ALL = ~0
};

enum pcb_family_e { PCB_UNIT_METRIC, PCB_UNIT_IMPERIAL };
enum pcb_suffix_e { PCB_UNIT_NO_SUFFIX, PCB_UNIT_SUFFIX, PCB_UNIT_FILE_MODE };

struct pcb_unit_s {
	int index;										/* Index into Unit[] list */
	const char *suffix;
	const char *in_suffix;				/* internationalized suffix */
	char printf_code;
	double scale_factor;
	enum pcb_family_e family;
	enum pcb_allow_e allow;
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

struct pcb_increments_s {
	const char *suffix;
	/* key g and <shift>g value  */
	pcb_coord_t grid;
	pcb_coord_t grid_min;
	pcb_coord_t grid_max;
	/* key s and <shift>s value  */
	pcb_coord_t size;
	pcb_coord_t size_min;
	pcb_coord_t size_max;
	/* key l and <shift>l value  */
	pcb_coord_t line;
	pcb_coord_t line_min;
	pcb_coord_t line_max;
	/* key k and <shift>k value  */
	pcb_coord_t clear;
	pcb_coord_t clear_min;
	pcb_coord_t clear_max;
};

typedef struct pcb_unit_s pcb_unit_t;
typedef struct pcb_increments_s pcb_increments_t;
extern pcb_unit_t Units[];
extern pcb_increments_t increments[];

const pcb_unit_t *get_unit_struct(const char *suffix);
const pcb_unit_t *get_unit_struct_by_allow(enum pcb_allow_e allow);
const pcb_unit_t *get_unit_list(void);
const pcb_unit_t *get_unit_by_idx(int idx);
int pcb_get_n_units(void);
double pcb_coord_to_unit(const pcb_unit_t *, pcb_coord_t);
double pcb_unit_to_factor(const pcb_unit_t * unit);
pcb_coord_t pcb_unit_to_coord(const pcb_unit_t *, double);
pcb_increments_t *pcb_get_increments_struct(const char *suffix);
pcb_angle_t pcb_normalize_angle(pcb_angle_t a);

void pcb_units_init(void);

/* PCB/physical unit conversions */
#define PCB_COORD_TO_MIL(n)	((n) / 25400.0)
#define PCB_MIL_TO_COORD(n)	((n) * 25400.0)
#define PCB_COORD_TO_MM(n)	((n) / 1000000.0)
#define PCB_MM_TO_COORD(n)	((n) * 1000000.0)
#define PCB_COORD_TO_INCH(n)	(PCB_COORD_TO_MIL(n) / 1000.0)
#define PCB_INCH_TO_COORD(n)	(PCB_MIL_TO_COORD(n) * 1000.0)
#define PCB_COORD_TO_DECIMIL(n)    (PCB_COORD_TO_MIL(n) * 10.0)
#define PCB_DECIMIL_TO_COORD(n)    (PCB_MIL_TO_COORD(n) / 10.0)

#define	PCB_SWAP_ANGLE(a)		(-(a))
#define	PCB_SWAP_DELTA(d)		(-(d))

#endif

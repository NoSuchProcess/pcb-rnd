/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2011 Andrew Poelstra
 *  Copyright (C) 2016,2017 Tibor 'Igor2' Palinkas
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
 *
 *
 *  Old contact info:
 *  Andrew Poelstra, 16966 60A Ave, V3S 8X5 Surrey, BC, Canada
 *  asp11@sfu.ca
 *
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "compat_misc.h"
#include "unit.h"

/* Helper macros for tables */
#define PCB_MM_TO_COORD3(a,b,c)		PCB_MM_TO_COORD (a), PCB_MM_TO_COORD (b), PCB_MM_TO_COORD (c)
#define PCB_MIL_TO_COORD3(a,b,c)		PCB_MIL_TO_COORD (a), PCB_MIL_TO_COORD (b), PCB_MIL_TO_COORD (c)
#define PCB_MM_TO_COORD5(a,b,c,d,e)		PCB_MM_TO_COORD (a), PCB_MM_TO_COORD (b), PCB_MM_TO_COORD (c),	\
                              		PCB_MM_TO_COORD (d), PCB_MM_TO_COORD (e)
#define PCB_MIL_TO_COORD5(a,b,c,d,e)	PCB_MIL_TO_COORD (a), PCB_MIL_TO_COORD (b), PCB_MIL_TO_COORD (c),	\
                               		PCB_MIL_TO_COORD (d), PCB_MIL_TO_COORD (e)

/* These should be kept in order of smallest scale_factor
 * to largest -- the code uses this ordering when finding
 * the best scale to use for a group of measures */
pcb_unit_t pcb_units[] = {
	{0, "km", NULL, 'k', 0.000001, PCB_UNIT_METRIC, PCB_UNIT_ALLOW_KM, 5,
	 0.00005, 0.0005, 0.0025, 0.05, 0.25,
	 {""}},
	{0, "m", NULL, 'f', 0.001, PCB_UNIT_METRIC, PCB_UNIT_ALLOW_M, 5,
	 0.0005, 0.005, 0.025, 0.5, 2.5,
	 {""}},
	{0, "cm", NULL, 'e', 0.1, PCB_UNIT_METRIC, PCB_UNIT_ALLOW_CM, 5,
	 0.005, 0.05, 0.25, 5, 25,
	 {""}},
	{0, "mm", NULL, 'm', 1, PCB_UNIT_METRIC, PCB_UNIT_ALLOW_MM, 4,
	 0.005, 0.05, 0.25, 5, 25,
	 {""}},
	{0, "um", NULL, 'u', 1000, PCB_UNIT_METRIC, PCB_UNIT_ALLOW_UM, 2,
	 0.005, 0.05, 0.25, 5, 25,
	 {""}},
	{0, "du", NULL, 'd', 10000, PCB_UNIT_METRIC, PCB_UNIT_ALLOW_DU, 2, /* eagle bin decimicron */
	 0.005, 0.05, 0.25, 5, 25,
	 {""}},
	{0, "nm", NULL, 'n', 1000000, PCB_UNIT_METRIC, PCB_UNIT_ALLOW_NM, 0,
	 5, 50, 2500, 5000, 25000,
	 {""}},

	{0, "in", NULL, 'i', 0.001, PCB_UNIT_IMPERIAL, PCB_UNIT_ALLOW_IN, 5,
	 0.1, 1.0, 5.0, 25, 100,
	 {"inch"}},
	{0, "mil", NULL, 'l', 1, PCB_UNIT_IMPERIAL, PCB_UNIT_ALLOW_MIL, 2,
	 0.1, 1.0, 10, 100, 1000,
	 {""}},
	{0, "dmil", NULL, 'k', 10, PCB_UNIT_IMPERIAL, PCB_UNIT_ALLOW_DMIL, 1,	/* kicad legacy decimil unit */
	 0.1, 1.0, 10, 100, 1000,				/* wild guess at factors */
	 {""}},
	{0, "cmil", NULL, 'c', 100, PCB_UNIT_IMPERIAL, PCB_UNIT_ALLOW_CMIL, 0,
	 1, 10, 100, 1000, 10000,
	 {"pcb"}}
};

#define N_UNITS ((int) (sizeof pcb_units / sizeof pcb_units[0]))

void pcb_units_init(void)
{
	int i;
	for (i = 0; i < N_UNITS; ++i) {
		pcb_units[i].index = i;
		pcb_units[i].in_suffix = pcb_units[i].suffix;
	}
}

const pcb_unit_t *get_unit_struct_(const char *suffix, int strict)
{
	int i;
	int s_len = 0;

	/* Determine bounds */
	while (isspace(*suffix))
		suffix++;
	while (isalnum(suffix[s_len]))
		s_len++;

	/* Also understand plural suffixes: "inches", "mils" */
	if (s_len > 2) {
		if (suffix[s_len - 2] == 'e' && suffix[s_len - 1] == 's')
			s_len -= 2;
		else if (suffix[s_len - 1] == 's')
			s_len -= 1;
	}

	/* Do lookup */
	if (s_len <= 0)
		return NULL;

	if (strict) {
		for (i = 0; i < N_UNITS; ++i)
			if (strcmp(suffix, pcb_units[i].suffix) == 0)
				return &pcb_units[i];
	}
	else {
		for (i = 0; i < N_UNITS; ++i)
			if (strncmp(suffix, pcb_units[i].suffix, s_len) == 0 || strncmp(suffix, pcb_units[i].alias[0], s_len) == 0)
				return &pcb_units[i];
	}

	return NULL;
}

const pcb_unit_t *get_unit_struct(const char *suffix)
{
	return get_unit_struct_(suffix, 0);
}


const pcb_unit_t *get_unit_struct_by_allow(enum pcb_allow_e allow)
{
	int i;
	for (i = 0; i < N_UNITS; ++i)
		if (pcb_units[i].allow == allow)
			return &pcb_units[i];

	return NULL;
}

const pcb_unit_t *get_unit_by_idx(int idx)
{
	if ((idx < 0) || (idx >= N_UNITS))
		return NULL;
	return pcb_units + idx;
}

int pcb_get_n_units(void)
{
	return N_UNITS;
}

double pcb_coord_to_unit(const pcb_unit_t * unit, pcb_coord_t x)
{
	double base;
	if (unit == NULL)
		return -1;
	base = unit->family == PCB_UNIT_METRIC ? PCB_COORD_TO_MM(1)
		: PCB_COORD_TO_MIL(1);
	return x * unit->scale_factor * base;
}

pcb_coord_t pcb_unit_to_coord(const pcb_unit_t * unit, double x)
{
	double base, res;

	if (unit == NULL)
		return -1;

	base = unit->family == PCB_UNIT_METRIC ? PCB_MM_TO_COORD(x) : PCB_MIL_TO_COORD(x);
	res = pcb_round(base/unit->scale_factor);

	/* clamp */
	if (res >= (double)COORD_MAX)
		return COORD_MAX;
	if (res <= -1.0 * (double)COORD_MAX)
		return -COORD_MAX;

	return res;
}

double pcb_unit_to_factor(const pcb_unit_t * unit)
{
	return 1.0 / pcb_coord_to_unit(unit, 1);
}

pcb_angle_t pcb_normalize_angle(pcb_angle_t a)
{
	while (a < 0)
		a += 360.0;
	while (a >= 360.0)
		a -= 360.0;
	return a;
}

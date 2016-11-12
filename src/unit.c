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
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "compat_misc.h"
#include "compat_nls.h"
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
pcb_unit_t Units[] = {
	{0, "km", NULL, 'k', 0.000001, METRIC, ALLOW_KM, 5,
	 0.00005, 0.0005, 0.0025, 0.05, 0.25,
	 {""}},
	{0, "m", NULL, 'f', 0.001, METRIC, ALLOW_M, 5,
	 0.0005, 0.005, 0.025, 0.5, 2.5,
	 {""}},
	{0, "cm", NULL, 'e', 0.1, METRIC, ALLOW_CM, 5,
	 0.005, 0.05, 0.25, 5, 25,
	 {""}},
	{0, "mm", NULL, 'm', 1, METRIC, ALLOW_MM, 4,
	 0.005, 0.05, 0.25, 5, 25,
	 {""}},
	{0, "um", NULL, 'u', 1000, METRIC, ALLOW_UM, 2,
	 0.005, 0.05, 0.25, 5, 25,
	 {""}},
	{0, "nm", NULL, 'n', 1000000, METRIC, ALLOW_NM, 0,
	 5, 50, 2500, 5000, 25000,
	 {""}},

	{0, "in", NULL, 'i', 0.001, IMPERIAL, ALLOW_IN, 5,
	 0.1, 1.0, 5.0, 25, 100,
	 {"inch"}},
	{0, "mil", NULL, 'l', 1, IMPERIAL, ALLOW_MIL, 2,
	 0.1, 1.0, 10, 100, 1000,
	 {""}},
	{0, "dmil", NULL, 'k', 10, IMPERIAL, ALLOW_DMIL, 1,	/* kicad legacy decimil unit */
	 0.1, 1.0, 10, 100, 1000,				/* wild guess at factors */
	 {""}},
	{0, "cmil", NULL, 'c', 100, IMPERIAL, ALLOW_CMIL, 0,
	 1, 10, 100, 1000, 10000,
	 {"pcb"}}
};

#define N_UNITS ((int) (sizeof Units / sizeof Units[0]))
/* \brief Initialize non-static data for pcb-printf
 * \par Function Description
 * Assigns each unit its index for quick access through the
 * main units array, and internationalize the units for GUI
 * display.
 */
void initialize_units()
{
	int i;
	for (i = 0; i < N_UNITS; ++i) {
		Units[i].index = i;
		Units[i].in_suffix = _(Units[i].suffix);
	}
}

/* This list -must- contain all printable units from the above list */
/* For now I have just copy/pasted the same values for all metric
 * units and the same values for all imperial ones */
pcb_increments_t increments[] = {
	/* TABLE FORMAT   |  default  |  min  |  max
	 *          grid  |           |       |
	 *          size  |           |       |
	 *          line  |           |       |
	 *         clear  |           |       |
	 */
	{"km", PCB_MM_TO_COORD3(0.1, 0.01, 1.0),
	 PCB_MM_TO_COORD3(0.2, 0.01, 0.5),
	 PCB_MM_TO_COORD3(0.1, 0.005, 0.5),
	 PCB_MM_TO_COORD3(0.05, 0.005, 0.5)},
	{"m", PCB_MM_TO_COORD3(0.1, 0.01, 1.0),
	 PCB_MM_TO_COORD3(0.2, 0.01, 0.5),
	 PCB_MM_TO_COORD3(0.1, 0.005, 0.5),
	 PCB_MM_TO_COORD3(0.05, 0.005, 0.5)},
	{"cm", PCB_MM_TO_COORD3(0.1, 0.01, 1.0),
	 PCB_MM_TO_COORD3(0.2, 0.01, 0.5),
	 PCB_MM_TO_COORD3(0.1, 0.005, 0.5),
	 PCB_MM_TO_COORD3(0.05, 0.005, 0.5)},
	{"mm", PCB_MM_TO_COORD3(0.1, 0.01, 1.0),
	 PCB_MM_TO_COORD3(0.2, 0.01, 0.5),
	 PCB_MM_TO_COORD3(0.1, 0.005, 0.5),
	 PCB_MM_TO_COORD3(0.05, 0.005, 0.5)},
	{"um", PCB_MM_TO_COORD3(0.1, 0.01, 1.0),
	 PCB_MM_TO_COORD3(0.2, 0.01, 0.5),
	 PCB_MM_TO_COORD3(0.1, 0.005, 0.5),
	 PCB_MM_TO_COORD3(0.05, 0.005, 0.5)},
	{"nm", PCB_MM_TO_COORD3(0.1, 0.01, 1.0),
	 PCB_MM_TO_COORD3(0.2, 0.01, 0.5),
	 PCB_MM_TO_COORD3(0.1, 0.005, 0.5),
	 PCB_MM_TO_COORD3(0.05, 0.005, 0.5)},

	{"cmil", PCB_MIL_TO_COORD3(5, 1, 25),
	 PCB_MIL_TO_COORD3(10, 1, 10),
	 PCB_MIL_TO_COORD3(5, 0.5, 10),
	 PCB_MIL_TO_COORD3(2, 0.5, 10)},
	{"dmil", PCB_MIL_TO_COORD3(5, 1, 25), /* kicad legacy decimil unit */
	 PCB_MIL_TO_COORD3(10, 1, 10),
	 PCB_MIL_TO_COORD3(5, 0.5, 10),
	 PCB_MIL_TO_COORD3(2, 0.5, 10)},
	{"mil", PCB_MIL_TO_COORD3(5, 1, 25),
	 PCB_MIL_TO_COORD3(10, 1, 10),
	 PCB_MIL_TO_COORD3(5, 0.5, 10),
	 PCB_MIL_TO_COORD3(2, 0.5, 10)},
	{"in", PCB_MIL_TO_COORD3(5, 1, 25),
	 PCB_MIL_TO_COORD3(10, 1, 10),
	 PCB_MIL_TO_COORD3(5, 0.5, 10),
	 PCB_MIL_TO_COORD3(2, 0.5, 10)},
};

#define N_INCREMENTS (sizeof increments / sizeof increments[0])

/* \brief Obtain a unit object from its suffix
 * \par Function Description
 * Looks up a given suffix in the main units array. Internationalized
 * unit suffixes are not supported, though pluralized units are, for
 * backward-compatibility.
 *
 * \param [in] const_suffix   The suffix to look up
 *
 * \return A const pointer to the pcb_unit_t struct, or NULL if none was found
 */
const pcb_unit_t *get_unit_struct(const char *suffix)
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
	if (s_len > 0)
		for (i = 0; i < N_UNITS; ++i)
			if (strncmp(suffix, Units[i].suffix, s_len) == 0 || strncmp(suffix, Units[i].alias[0], s_len) == 0)
				return &Units[i];

	return NULL;
}

const pcb_unit_t *get_unit_struct_by_allow(enum pcb_allow_e allow)
{
	int i;
	for (i = 0; i < N_UNITS; ++i)
		if (Units[i].allow == allow)
			return &Units[i];

	return NULL;
}

/* ACCESSORS */
/* \brief Returns the master unit list. This may not be modified. */
const pcb_unit_t *get_unit_list(void)
{
	return Units;
}

/* \brief Returns the unit by its index */
const pcb_unit_t *get_unit_by_idx(int idx)
{
	if ((idx < 0) || (idx >= N_UNITS))
		return NULL;
	return Units + idx;
}

/* \brief Returns the length of the master unit list. */
int get_n_units(void)
{
	return N_UNITS;
}

/* \brief Convert a pcb coord to the given unit
 *
 * \param [in] unit  The unit to convert to
 * \param [in] x     The quantity to convert
 *
 * \return The converted measure
 */
double coord_to_unit(const pcb_unit_t * unit, Coord x)
{
	double base;
	if (unit == NULL)
		return -1;
	base = unit->family == METRIC ? PCB_COORD_TO_MM(1)
		: PCB_COORD_TO_MIL(1);
	return x * unit->scale_factor * base;
}

/* \brief Convert a given unit to pcb coords
 *
 * \param [in] unit  The unit to convert from
 * \param [in] x     The quantity to convert
 *
 * \return The converted measure
 */
Coord unit_to_coord(const pcb_unit_t * unit, double x)
{
	double base;
	if (unit == NULL)
		return -1;
	base = unit->family == METRIC ? PCB_MM_TO_COORD(x)
		: PCB_MIL_TO_COORD(x);
	return pcb_round(base/unit->scale_factor);
}

/* \brief Return how many PCB-internal-Coord-unit a unit translates to
 *
 * \param [in] unit  The unit to convert
 *
 * \return The converted measure
 */
double unit_to_factor(const pcb_unit_t * unit)
{
	return 1.0 / coord_to_unit(unit, 1);
}

/* \brief Obtain an increment object from its suffix
 * \par Function Description
 * Looks up a given suffix in the main increments array. Internationalized
 * unit suffixes are not supported, nor are pluralized units.
 *
 * \param [in] suffix   The suffix to look up
 *
 * \return A const pointer to the pcb_increments_t struct, or NULL if none was found
 */
pcb_increments_t *get_increments_struct(const char *suffix)
{
	int i;
	/* Do lookup */
	for (i = 0; i < N_INCREMENTS; ++i)
		if (strcmp(suffix, increments[i].suffix) == 0)
			return &increments[i];
	return NULL;
}

/* Bring an angle into [0, 360) range */
Angle NormalizeAngle(Angle a)
{
	while (a < 0)
		a += 360.0;
	while (a >= 360.0)
		a -= 360.0;
	return a;
}

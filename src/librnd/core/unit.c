/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2011 Andrew Poelstra
 *  Copyright (C) 2016,2017,2019 Tibor 'Igor2' Palinkas
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
#include <librnd/config.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/unit.h>

/* Should be kept in order of smallest scale_factor to largest -- the code
   uses this ordering for finding the best scale to use for a group of units */
rnd_unit_t pcb_units[] = {
	{"km",   'k', 0.000001, PCB_UNIT_METRIC,   PCB_UNIT_ALLOW_KM, 5},
	{"m",    'f', 0.001,    PCB_UNIT_METRIC,   PCB_UNIT_ALLOW_M, 5},
	{"cm",   'e', 0.1,      PCB_UNIT_METRIC,   PCB_UNIT_ALLOW_CM, 5},
	{"mm",   'm', 1,        PCB_UNIT_METRIC,   PCB_UNIT_ALLOW_MM, 4},
	{"um",   'u', 1000,     PCB_UNIT_METRIC,   PCB_UNIT_ALLOW_UM, 2},
	{"du",   'd', 10000,    PCB_UNIT_METRIC,   PCB_UNIT_ALLOW_DU, 2},  /* eagle bin decimicron */
	{"nm",   'n', 1000000,  PCB_UNIT_METRIC,   PCB_UNIT_ALLOW_NM, 0},

	{"in",   'i', 0.001,    PCB_UNIT_IMPERIAL, PCB_UNIT_ALLOW_IN, 5},
	{"mil",  'l', 1,        PCB_UNIT_IMPERIAL, PCB_UNIT_ALLOW_MIL, 2},
	{"dmil", 'k', 10,       PCB_UNIT_IMPERIAL, PCB_UNIT_ALLOW_DMIL, 1},/* kicad legacy decimil unit */
	{"cmil", 'c', 100,      PCB_UNIT_IMPERIAL, PCB_UNIT_ALLOW_CMIL, 0},

	{"Hz",   'z', 1,           PCB_UNIT_FREQ,  PCB_UNIT_ALLOW_HZ, 3},
	{"kHz",  'Z', 0.001,       PCB_UNIT_FREQ,  PCB_UNIT_ALLOW_KHZ, 6},
	{"MHz",  'M', 0.000001,    PCB_UNIT_FREQ,  PCB_UNIT_ALLOW_MHZ, 6},
	{"GHz",  'G', 0.000000001, PCB_UNIT_FREQ,  PCB_UNIT_ALLOW_GHZ, 6},

	/* aliases - must be a block at the end */
	{"inch",  0,  0.001,    PCB_UNIT_IMPERIAL, PCB_UNIT_ALLOW_IN, 5},
	{"pcb",   0,  100,      PCB_UNIT_IMPERIAL, PCB_UNIT_ALLOW_CMIL, 0} /* old io_pcb unit */
};

#define N_UNITS ((int) (sizeof pcb_units / sizeof pcb_units[0]))

void pcb_units_init(void)
{
	int i;
	for (i = 0; i < N_UNITS; ++i)
		pcb_units[i].index = i;
}

const rnd_unit_t *get_unit_struct_(const char *suffix, int strict)
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
			if (strncmp(suffix, pcb_units[i].suffix, s_len) == 0)
				return &pcb_units[i];
	}

	return NULL;
}

const rnd_unit_t *get_unit_struct(const char *suffix)
{
	return get_unit_struct_(suffix, 0);
}


const rnd_unit_t *get_unit_struct_by_allow(enum pcb_allow_e allow)
{
	int i;
	for (i = 0; i < N_UNITS; ++i)
		if (pcb_units[i].allow == allow)
			return &pcb_units[i];

	return NULL;
}

const rnd_unit_t *get_unit_by_idx(int idx)
{
	if ((idx < 0) || (idx >= N_UNITS))
		return NULL;
	return pcb_units + idx;
}

const rnd_unit_t *get_unit_by_suffix(const char *suffix)
{
	int i;
	for (i = 0; i < N_UNITS; ++i)
		if (strcmp(pcb_units[i].suffix, suffix) == 0)
			return &pcb_units[i];
	return NULL;
}

int pcb_get_n_units(int aliases_too)
{
	static int num_base = -1;
	if (aliases_too)
		return N_UNITS;
	if (num_base < 0)
		for(num_base = 0; pcb_units[num_base].printf_code != 0; num_base++) ;
	return num_base;
}

double pcb_coord_to_unit(const rnd_unit_t *unit, rnd_coord_t x)
{
	double base;
	if (unit == NULL)
		return -1;
	switch(unit->family) {
		case PCB_UNIT_METRIC: base = PCB_COORD_TO_MM(1); break;
		case PCB_UNIT_IMPERIAL: base = PCB_COORD_TO_MIL(1); break;
		case PCB_UNIT_FREQ: base = 1.0; break;
	}
	return x * unit->scale_factor * base;
}

rnd_coord_t pcb_unit_to_coord(const rnd_unit_t *unit, double x)
{
	double base, res;

	if (unit == NULL)
		return -1;

	switch(unit->family) {
		case PCB_UNIT_METRIC:   base = PCB_MM_TO_COORD(x); break;
		case PCB_UNIT_IMPERIAL: base = PCB_MIL_TO_COORD(x); break;
		case PCB_UNIT_FREQ:     base = x; break;
	}
	res = rnd_round(base/unit->scale_factor);

	/* clamp */
	switch(unit->family) {
		case PCB_UNIT_METRIC:
		case PCB_UNIT_IMPERIAL:
			if (res >= (double)RND_COORD_MAX)
				return RND_COORD_MAX;
			if (res <= -1.0 * (double)RND_COORD_MAX)
				return -RND_COORD_MAX;
			break;
		case PCB_UNIT_FREQ:
			break;
	}
	return res;
}

double pcb_unit_to_factor(const rnd_unit_t *unit)
{
	return 1.0 / pcb_coord_to_unit(unit, 1);
}

rnd_angle_t pcb_normalize_angle(rnd_angle_t a)
{
	while (a < 0)
		a += 360.0;
	while (a >= 360.0)
		a -= 360.0;
	return a;
}

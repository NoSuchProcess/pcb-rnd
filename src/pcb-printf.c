/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2011 Andrew Poelstra
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

/*! \file <pcb-printf.c>
 *  \brief Implementation of printf wrapper to output pcb coords and angles
 *  \par Description
 *  For details of all supported specifiers, see the comment at the
 *  top of pcb-printf.h
 */

#include "config.h"
#include "global.h"

#include <genvector/gds_char.h>

#include "pcb-printf.h"

/* Helper macros for tables */
#define MM_TO_COORD3(a,b,c)		MM_TO_COORD (a), MM_TO_COORD (b), MM_TO_COORD (c)
#define MIL_TO_COORD3(a,b,c)		MIL_TO_COORD (a), MIL_TO_COORD (b), MIL_TO_COORD (c)
#define MM_TO_COORD5(a,b,c,d,e)		MM_TO_COORD (a), MM_TO_COORD (b), MM_TO_COORD (c),	\
                              		MM_TO_COORD (d), MM_TO_COORD (e)
#define MIL_TO_COORD5(a,b,c,d,e)	MIL_TO_COORD (a), MIL_TO_COORD (b), MIL_TO_COORD (c),	\
                               		MIL_TO_COORD (d), MIL_TO_COORD (e)

/* These should be kept in order of smallest scale_factor
 * to largest -- the code uses this ordering when finding
 * the best scale to use for a group of measures */
static Unit Units[] = {
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
static Increments increments[] = {
	/* TABLE FORMAT   |  default  |  min  |  max
	 *          grid  |           |       |
	 *          size  |           |       |
	 *          line  |           |       |
	 *         clear  |           |       |
	 */
	{"km", MM_TO_COORD3(0.1, 0.01, 1.0),
	 MM_TO_COORD3(0.2, 0.01, 0.5),
	 MM_TO_COORD3(0.1, 0.005, 0.5),
	 MM_TO_COORD3(0.05, 0.005, 0.5)},
	{"m", MM_TO_COORD3(0.1, 0.01, 1.0),
	 MM_TO_COORD3(0.2, 0.01, 0.5),
	 MM_TO_COORD3(0.1, 0.005, 0.5),
	 MM_TO_COORD3(0.05, 0.005, 0.5)},
	{"cm", MM_TO_COORD3(0.1, 0.01, 1.0),
	 MM_TO_COORD3(0.2, 0.01, 0.5),
	 MM_TO_COORD3(0.1, 0.005, 0.5),
	 MM_TO_COORD3(0.05, 0.005, 0.5)},
	{"mm", MM_TO_COORD3(0.1, 0.01, 1.0),
	 MM_TO_COORD3(0.2, 0.01, 0.5),
	 MM_TO_COORD3(0.1, 0.005, 0.5),
	 MM_TO_COORD3(0.05, 0.005, 0.5)},
	{"um", MM_TO_COORD3(0.1, 0.01, 1.0),
	 MM_TO_COORD3(0.2, 0.01, 0.5),
	 MM_TO_COORD3(0.1, 0.005, 0.5),
	 MM_TO_COORD3(0.05, 0.005, 0.5)},
	{"nm", MM_TO_COORD3(0.1, 0.01, 1.0),
	 MM_TO_COORD3(0.2, 0.01, 0.5),
	 MM_TO_COORD3(0.1, 0.005, 0.5),
	 MM_TO_COORD3(0.05, 0.005, 0.5)},

	{"cmil", MIL_TO_COORD3(5, 1, 25),
	 MIL_TO_COORD3(10, 1, 10),
	 MIL_TO_COORD3(5, 0.5, 10),
	 MIL_TO_COORD3(2, 0.5, 10)},
	{"mil", MIL_TO_COORD3(5, 1, 25),
	 MIL_TO_COORD3(10, 1, 10),
	 MIL_TO_COORD3(5, 0.5, 10),
	 MIL_TO_COORD3(2, 0.5, 10)},
	{"in", MIL_TO_COORD3(5, 1, 25),
	 MIL_TO_COORD3(10, 1, 10),
	 MIL_TO_COORD3(5, 0.5, 10),
	 MIL_TO_COORD3(2, 0.5, 10)},
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
 * \return A const pointer to the Unit struct, or NULL if none was found
 */
const Unit *get_unit_struct(const char *suffix)
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

/* ACCESSORS */
/* \brief Returns the master unit list. This may not be modified. */
const Unit *get_unit_list(void)
{
	return Units;
}

/* \brief Returns the unit by its index */
const Unit *get_unit_by_idx(int idx)
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

/* \brief Obtain an increment object from its suffix
 * \par Function Description
 * Looks up a given suffix in the main increments array. Internationalized
 * unit suffixes are not supported, nor are pluralized units.
 *
 * \param [in] suffix   The suffix to look up
 *
 * \return A const pointer to the Increments struct, or NULL if none was found
 */
Increments *get_increments_struct(const char *suffix)
{
	int i;
	/* Do lookup */
	for (i = 0; i < N_INCREMENTS; ++i)
		if (strcmp(suffix, increments[i].suffix) == 0)
			return &increments[i];
	return NULL;
}

/* \brief Convert a pcb coord to the given unit
 *
 * \param [in] unit  The unit to convert to
 * \param [in] x     The quantity to convert
 *
 * \return The converted measure
 */
double coord_to_unit(const Unit * unit, Coord x)
{
	double base;
	if (unit == NULL)
		return -1;
	base = unit->family == METRIC ? COORD_TO_MM(1)
		: COORD_TO_MIL(1);
	return x * unit->scale_factor * base;
}

/* \brief Convert a given unit to pcb coords
 *
 * \param [in] unit  The unit to convert from
 * \param [in] x     The quantity to convert
 *
 * \return The converted measure
 */
Coord unit_to_coord(const Unit * unit, double x)
{
	return x / coord_to_unit(unit, 1);
}

/* \brief Return how many PCB-internal-Coord-unit a unit translates to
 *
 * \param [in] unit  The unit to convert
 *
 * \return The converted measure
 */
double unit_to_factor(const Unit * unit)
{
	return 1.0 / coord_to_unit(unit, 1);
}

static int min_sig_figs(double d)
{
	char buf[50];
	int rv;

	if (d == 0)
		return 0;

	/* Normalize to x.xxxx... form */
	if (d < 0)
		d *= -1;
	while (d >= 10)
		d /= 10;
	while (d < 1)
		d *= 10;

	rv = sprintf(buf, "%g", d);
	return rv;
}

/* \brief Internal coord-to-string converter for pcb-printf
 * \par Function Description
 * Converts a (group of) measurement(s) to a comma-deliminated
 * string, with appropriate units. If more than one coord is
 * given, the list is enclosed in parens to make the scope of
 * the unit suffix clear.
 *
 * \param [in] dest         Append the output to this dynamic string
 * \param [in] coord        Array of coords to convert
 * \param [in] n_coords     Number of coords in array
 * \param [in] printf_spec  printf sub-specifier to use with %f
 * \param [in] e_allow      Bitmap of units the function may use
 * \param [in] suffix_type  Whether to add a suffix
 *
 * \return 0 on success, -1 on error
 */
static int CoordsToString(gds_t *dest, Coord coord[], int n_coords, const gds_t *printf_spec_, enum e_allow allow,
														 enum e_suffix suffix_type)
{
	char *printf_buff;
	char filemode_buff[G_ASCII_DTOSTR_BUF_SIZE];
	enum e_family family;
	double *value, value_local[32];
	const char *suffix;
	int i, n;
	const char *printf_spec = printf_spec_->array;

	if (n_coords > (sizeof(value_local) / sizeof(value_local[0]))) {
		value = malloc(n_coords * sizeof *value);
		if (value == NULL)
			return -1;
	}
	else
		value = value_local;

	if (allow == 0)
		allow = ALLOW_ALL;
	if (printf_spec == NULL)
		printf_spec = "";

	/* Check our freedom in choosing units */
	if ((allow & ALLOW_IMPERIAL) == 0)
		family = METRIC;
	else if ((allow & ALLOW_METRIC) == 0)
		family = IMPERIAL;
	else {
		int met_votes = 0, imp_votes = 0;

		for (i = 0; i < n_coords; ++i)
			if (min_sig_figs(COORD_TO_MIL(coord[i])) < min_sig_figs(COORD_TO_MM(coord[i])))
				++imp_votes;
			else
				++met_votes;

		if (imp_votes > met_votes)
			family = IMPERIAL;
		else
			family = METRIC;
	}

	/* Set base unit */
	for (i = 0; i < n_coords; ++i) {
		switch (family) {
		case METRIC:
			value[i] = COORD_TO_MM(coord[i]);
			break;
		case IMPERIAL:
			value[i] = COORD_TO_MIL(coord[i]);
			break;
		}
	}

	/* Determine scale factor -- find smallest unit that brings
	 * the whole group above unity */
	for (n = 0; n < N_UNITS; ++n) {
		if ((Units[n].allow & allow) != 0 && (Units[n].family == family)) {
			int n_above_one = 0;

			for (i = 0; i < n_coords; ++i)
				if (fabs(value[i] * Units[n].scale_factor) > 1)
					++n_above_one;
			if (n_above_one == n_coords)
				break;
		}
	}
	/* If nothing worked, wind back to the smallest allowable unit */
	if (n == N_UNITS) {
		do {
			--n;
		} while ((Units[n].allow & allow) == 0 || Units[n].family != family);
	}

	/* Apply scale factor */
	suffix = Units[n].suffix;
	for (i = 0; i < n_coords; ++i)
		value[i] = value[i] * Units[n].scale_factor;

	/* Create sprintf specifier, using default_prec no preciscion is given */
	i = 0;
	while (printf_spec[i] == '%' || isdigit(printf_spec[i]) ||
				 printf_spec[i] == '-' || printf_spec[i] == '+' || printf_spec[i] == '#' || printf_spec[i] == '0')
		++i;
	if (printf_spec[i] == '.')
		printf_buff = g_strdup_printf(", %sf", printf_spec);
	else
		printf_buff = g_strdup_printf(", %s.%df", printf_spec, Units[n].default_prec);

	/* Actually sprintf the values in place
	 *  (+ 2 skips the ", " for first value) */
	if (n_coords > 1)
		gds_append(dest,  '(');
	if (suffix_type == FILE_MODE)
		g_ascii_formatd(filemode_buff, sizeof filemode_buff, printf_buff + 2, value[0]);
	else
		sprintf(filemode_buff, printf_buff + 2, value[0]);
	gds_append_str(dest, filemode_buff);
	for (i = 1; i < n_coords; ++i) {
		if (suffix_type == FILE_MODE)
			g_ascii_formatd(filemode_buff, sizeof filemode_buff, printf_buff, value[i]);
		else
			sprintf(filemode_buff, printf_buff, value[i]);
		gds_append_str(dest, filemode_buff);
	}
	if (n_coords > 1)
		gds_append(dest, ')');
	/* Append suffix */
	if (value[0] != 0 || n_coords > 1) {
		switch (suffix_type) {
		case NO_SUFFIX:
			break;
		case SUFFIX:
			gds_append(dest, ' ');
			gds_append_str(dest, suffix);
			break;
		case FILE_MODE:
			gds_append_str(dest, suffix);
			break;
		}
	}

	g_free(printf_buff);

	if (value != value_local)
		free(value);
	return 0;
}

/* \brief Main pcb-printf function
 * \par Function Description
 * This is a printf wrapper that accepts new format specifiers to
 * output pcb coords as various units. See the comment at the top
 * of pcb-printf.h for full details.
 *
 * \param [in] fmt    Format specifier
 * \param [in] args   Arguments to specifier
 *
 * \return A formatted string. Must be freed with g_free.
 */
char *pcb_vprintf(const char *fmt, va_list args)
{
	gds_t string, spec;
	char tmp[128]; /* large enough for rendering a long long int */
	int tmplen;

	enum e_allow mask = ALLOW_ALL;

	gds_init(&string);
	gds_init(&spec);

	while (*fmt) {
		enum e_suffix suffix = NO_SUFFIX;

		if (*fmt == '%') {
			const char *ext_unit = "";
			Coord value[10];
			int count, i;

			gds_truncate(&spec, 0);

			/* Get printf sub-specifiers */
			gds_append(&spec, *fmt++);
			while (isdigit(*fmt) || *fmt == '.' || *fmt == ' ' || *fmt == '*'
						 || *fmt == '#' || *fmt == 'l' || *fmt == 'L' || *fmt == 'h' || *fmt == '+' || *fmt == '-') {
				if (*fmt == '*') {
					char itmp[32];
					int ilen;
					ilen = sprintf(itmp, "%d", va_arg(args, int));
					gds_append_len(&spec, itmp, ilen);
					fmt++;
				}
				else
					gds_append(&spec, *fmt++);
			}
			/* Get our sub-specifiers */
			if (*fmt == '#') {
				mask = ALLOW_CMIL;			/* This must be pcb's base unit */
				fmt++;
			}
			if (*fmt == '$') {
				suffix = SUFFIX;
				fmt++;
			}
			/* Tack full specifier onto specifier */
			if (*fmt != 'm')
				gds_append(&spec, *fmt);
			switch (*fmt) {
				/* Printf specs */
			case 'o':
			case 'i':
			case 'd':
			case 'u':
			case 'x':
			case 'X':
				if (spec.array[1] == 'l') {
					if (spec.array[2] == 'l')
						tmplen = sprintf(tmp, spec.array, va_arg(args, long long));
					else
						tmplen = sprintf(tmp, spec.array, va_arg(args, long));
				}
				else {
					tmplen = sprintf(tmp, spec.array, va_arg(args, int));
				}
				gds_append_len(&string, tmp, tmplen);
				break;
			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G':
				if (strchr(spec.array, '*')) {
					int prec = va_arg(args, int);
					tmplen = sprintf(tmp, spec.array, va_arg(args, double), prec);
				}
				else
					tmplen = sprintf(tmp, spec.array, va_arg(args, double));
				gds_append_len(&string, tmp, tmplen);
				break;
			case 'c':
				if (spec.array[1] == 'l' && sizeof(int) <= sizeof(wchar_t))
					tmplen = sprintf(tmp, spec.array, va_arg(args, wchar_t));
				else
					tmplen = sprintf(tmp, spec.array, va_arg(args, int));
				gds_append_len(&string, tmp, tmplen);
				break;
			case 's':
				if (spec.array[0] == 'l')
					tmplen = sprintf(tmp, spec.array, va_arg(args, wchar_t *));
				else
					tmplen = sprintf(tmp, spec.array, va_arg(args, char *));
				gds_append_len(&string, tmp, tmplen);
				break;
			case 'n':
				/* Depending on gcc settings, this will probably break with
				 *  some silly "can't put %n in writeable data space" message */
				tmplen = sprintf(tmp, spec.array, va_arg(args, int *));
				gds_append_len(&string, tmp, tmplen);
				break;
			case 'p':
				tmplen = sprintf(tmp, spec.array, va_arg(args, void *));
				gds_append_len(&string, tmp, tmplen);
				break;
			case '%':
				gds_append(&string, '%');
				break;
				/* Our specs */
			case 'm':
				++fmt;
				if (*fmt == '*')
					ext_unit = va_arg(args, const char *);
				if (*fmt != '+' && *fmt != 'a')
					value[0] = va_arg(args, Coord);
				count = 1;
				switch (*fmt) {
				case 'I':
					CoordsToString(&string, value, 1, &spec, ALLOW_NM, NO_SUFFIX);
					break;
				case 's':
					CoordsToString(&string, value, 1, &spec, ALLOW_MM | ALLOW_MIL, suffix);
					break;
				case 'S':
					CoordsToString(&string, value, 1, &spec, mask & ALLOW_ALL, suffix);
					break;
				case 'M':
					CoordsToString(&string, value, 1, &spec, mask & ALLOW_METRIC, suffix);
					break;
				case 'L':
					CoordsToString(&string, value, 1, &spec, mask & ALLOW_IMPERIAL, suffix);
					break;
#if 0
				case 'r':
					CoordsToString(&string, value, 1, &spec, ALLOW_READABLE, FILE_MODE);
					break;
#else
				case 'r':
					CoordsToString(&string, value, 1, &spec, ALLOW_READABLE, NO_SUFFIX);
					break;
#endif
					/* All these fallthroughs are deliberate */
				case '9':
					value[count++] = va_arg(args, Coord);
				case '8':
					value[count++] = va_arg(args, Coord);
				case '7':
					value[count++] = va_arg(args, Coord);
				case '6':
					value[count++] = va_arg(args, Coord);
				case '5':
					value[count++] = va_arg(args, Coord);
				case '4':
					value[count++] = va_arg(args, Coord);
				case '3':
					value[count++] = va_arg(args, Coord);
				case '2':
				case 'D':
					value[count++] = va_arg(args, Coord);
					CoordsToString(&string, value, count, &spec, mask & ALLOW_ALL, suffix);
					break;
				case 'd':
					value[1] = va_arg(args, Coord);
					CoordsToString(&string, value, 2, &spec, ALLOW_MM | ALLOW_MIL, suffix);
					break;
				case '*':
					{
						int found = 0;
						for (i = 0; i < N_UNITS; ++i) {
							if (strcmp(ext_unit, Units[i].suffix) == 0) {
								CoordsToString(&string, value, 1, &spec, Units[i].allow, suffix);
								found = 1;
								break;
							}
						}
						if (!found)
							CoordsToString(&string, value, 1, &spec, mask & ALLOW_ALL, suffix);
					}
					break;
				case 'a':
					gds_append_len(&spec, ".0f", 3);
					if (suffix == SUFFIX)
						gds_append_len(&spec, " deg", 4);
					tmplen = sprintf(tmp, spec.array, (double) va_arg(args, Angle));
					gds_append_len(&string, tmp, tmplen);
					break;
				case '+':
					mask = va_arg(args, enum e_allow);
					break;
				default:
					{
						int found = 0;
						for (i = 0; i < N_UNITS; ++i) {
							if (*fmt == Units[i].printf_code) {
								CoordsToString(&string, value, 1, &spec, Units[i].allow, suffix);
								found = 1;
								break;
							}
						}
						if (!found)
							CoordsToString(&string, value, 1, &spec, ALLOW_ALL, suffix);
					}
					break;
				}
				break;
			}
		}
		else
			gds_append(&string, *fmt);
		++fmt;
	}
	gds_uninit(&spec);

	/* Return just the char* part of our string */
	return string.array;
}


/* \brief Wrapper for pcb_vprintf that outputs to a string
 *
 * \param [in] string  Pointer to string to output into
 * \param [in] fmt     Format specifier
 *
 * \return The length of the written string
 */
int pcb_sprintf(char *string, const char *fmt, ...)
{
	char *tmp;

	va_list args;
	va_start(args, fmt);

	tmp = pcb_vprintf(fmt, args);
	strcpy(string, tmp);
	free(tmp);

	va_end(args);
	return strlen(string);
}

/* \brief Wrapper for pcb_vprintf that outputs to a file
 *
 * \param [in] fh   File to output to
 * \param [in] fmt  Format specifier
 *
 * \return The length of the written string
 */
int pcb_fprintf(FILE * fh, const char *fmt, ...)
{
	int rv;
	char *tmp;

	va_list args;
	va_start(args, fmt);

	if (fh == NULL)
		rv = -1;
	else {
		tmp = pcb_vprintf(fmt, args);
		rv = fprintf(fh, "%s", tmp);
		free(tmp);
	}

	va_end(args);
	return rv;
}

/* \brief Wrapper for pcb_vprintf that outputs to stdout
 *
 * \param [in] fmt  Format specifier
 *
 * \return The length of the written string
 */
int pcb_printf(const char *fmt, ...)
{
	int rv;
	char *tmp;

	va_list args;
	va_start(args, fmt);

	tmp = pcb_vprintf(fmt, args);
	rv = printf("%s", tmp);
	free(tmp);

	va_end(args);
	return rv;
}

/* \brief Wrapper for pcb_vprintf that outputs to a newly allocated string
 *
 * \param [in] fmt  Format specifier
 *
 * \return The newly allocated string. Must be freed with g_free.
 */
char *pcb_g_strdup_printf(const char *fmt, ...)
{
	char *tmp;

	va_list args;
	va_start(args, fmt);
	tmp = pcb_vprintf(fmt, args);
	va_end(args);
	return tmp;
}

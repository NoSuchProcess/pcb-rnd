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

/*! \file <pcb-printf.c>
 *  \brief Implementation of printf wrapper to output pcb coords and angles
 *  \par Description
 *  For details of all supported specifiers, see the comment at the
 *  top of pcb-printf.h
 */

#include <locale.h>
#include <ctype.h>
#include <math.h>
#include "config.h"
#include "pcb-printf.h"

const char *pcb_printf_slot[PCB_PRINTF_SLOT_max] =
{
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 8 user formats */
	"%mr",      /* original unitless cmil file format coord */
	"%.07$$mS"  /* safe file format coord */
};

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

/* Truncate trailing 0's from str */
static void do_trunc0(char *str)
{
	char *end = str + strlen(str) - 1;
	while((end > str) && (*end == '0') && (end[-1] != '.')) {
		*end = '\0';
		end--;
	}
}


/* Create sprintf specifier, using default_prec no precision is given */
static inline void make_printf_spec(char *printf_spec_new, const char *printf_spec, int precision, int *trunc0)
{
	int i = 0;

	while (printf_spec[i] == '%' || isdigit(printf_spec[i]) ||
				 printf_spec[i] == '-' || printf_spec[i] == '+' || printf_spec[i] == '#' || printf_spec[i] == '0')
		++i;

	if (printf_spec[i] == '.') {
		if ((printf_spec[i+1] == '0') && (isdigit(printf_spec[i+2])))
			*trunc0 = 1;
		sprintf(printf_spec_new, ", %sf", printf_spec);
	}
	else
		sprintf(printf_spec_new, ", %s.%df", printf_spec, precision);
}


/* sprintf a value */
#define sprintf_lc_safe(is_file_mode, out, fmt, val) \
do { \
	if (is_file_mode) { \
		setlocale(LC_ALL, "C"); \
		sprintf(out, fmt, val); \
		setlocale(LC_ALL, ""); \
	} \
	else \
		sprintf(out, fmt, val); \
	if (trunc0) \
		do_trunc0(filemode_buff); \
} while(0)

/* append a suffix, with or without space */
static int inline append_suffix(gds_t *dest, enum e_suffix suffix_type, const char *suffix)
{
	switch (suffix_type) {
	case NO_SUFFIX:
		break;
	case SUFFIX:
		if (gds_append(dest, ' ') != 0) return -1;
		/* deliberate fall-thru */
	case FILE_MODE:
		if (gds_append_str(dest, suffix) != 0) return -1;
		break;
	}
	return 0;
}


/* \brief Internal coord-to-string converter for pcb-printf
 * \par Function Description
 * Converts a (group of) measurement(s) to a comma-delimited
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
static int CoordsToString(gds_t *dest, pcb_coord_t coord[], int n_coords, const gds_t *printf_spec_, enum pcb_allow_e allow,
														 enum e_suffix suffix_type)
{
	char filemode_buff[128]; /* G_ASCII_DTOSTR_BUF_SIZE */
	char printf_spec_new_local[256];
	double *value, value_local[32];
	enum e_family family;
	const char *suffix;
	int i, n, retval = -1, trunc0 = 0;
	const char *printf_spec = printf_spec_->array;
	char *printf_spec_new;

	if (n_coords > (sizeof(value_local) / sizeof(value_local[0]))) {
		value = malloc(n_coords * sizeof *value);
		if (value == NULL)
			return -1;
	}
	else
		value = value_local;

	if (allow == 0)
		allow = ALLOW_ALL;

	i = printf_spec_->used + 64;
	if (i > sizeof(printf_spec_new_local))
		printf_spec_new = malloc(i);
	else
		printf_spec_new = printf_spec_new_local;

	/* Check our freedom in choosing units */
	if ((allow & ALLOW_IMPERIAL) == 0)
		family = METRIC;
	else if ((allow & ALLOW_METRIC) == 0)
		family = IMPERIAL;
	else {
		int met_votes = 0, imp_votes = 0;

		for (i = 0; i < n_coords; ++i)
			if (min_sig_figs(PCB_COORD_TO_MIL(coord[i])) < min_sig_figs(PCB_COORD_TO_MM(coord[i])))
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
			value[i] = PCB_COORD_TO_MM(coord[i]);
			break;
		case IMPERIAL:
			value[i] = PCB_COORD_TO_MIL(coord[i]);
			break;
		}
	}

	/* Determine scale factor -- find smallest unit that brings
	 * the whole group above unity */
	for (n = 0; n < get_n_units(); ++n) {
		if ((Units[n].allow & allow) != 0 && (Units[n].family == family)) {
			int n_above_one = 0;

			for (i = 0; i < n_coords; ++i)
				if (abs(value[i] * Units[n].scale_factor) > 1)
					++n_above_one;
			if (n_above_one == n_coords)
				break;
		}
	}
	/* If nothing worked, wind back to the smallest allowable unit */
	if (n == get_n_units()) {
		do {
			--n;
		} while ((Units[n].allow & allow) == 0 || Units[n].family != family);
	}

	/* Apply scale factor */
	suffix = Units[n].suffix;
	for (i = 0; i < n_coords; ++i)
		value[i] = value[i] * Units[n].scale_factor;

	make_printf_spec(printf_spec_new, printf_spec, Units[n].default_prec, &trunc0);

	/* Actually sprintf the values in place
	 *  (+ 2 skips the ", " for first value) */
	if (n_coords > 1)
		if (gds_append(dest,  '(') != 0)
			goto err;

	sprintf_lc_safe((suffix_type == FILE_MODE), filemode_buff, printf_spec_new + 2, value[0]);
	if (gds_append_str(dest, filemode_buff) != 0)
		goto err;

	for (i = 1; i < n_coords; ++i) {
		sprintf_lc_safe((suffix_type == FILE_MODE), filemode_buff, printf_spec_new, value[i]);
		if (gds_append_str(dest, filemode_buff) != 0)
			goto err;
	}
	if (n_coords > 1)
		if (gds_append(dest, ')') != 0) goto err;


	/* Append suffix */
	if (value[0] != 0 || n_coords > 1)
		if (append_suffix(dest, suffix_type, suffix) != 0)
			goto err;

	retval = 0;
err:;
	if (printf_spec_new != printf_spec_new_local)
		free(printf_spec_new);

	if (value != value_local)
		free(value);
	return retval;
}

typedef struct {
	int           score_factor;

	enum pcb_allow_e  base;
	double        down_limit;
	enum pcb_allow_e  down;
	double        up_limit;
	enum pcb_allow_e  up;

	/* persistent, calculated once */
	const pcb_unit_t    *base_unit, *down_unit, *up_unit;
} human_coord_t;

/* Conversion preference table */
static human_coord_t human_coord[] = {
	{2, ALLOW_MM,  0.001, ALLOW_UM, 1000.0, ALLOW_M     ,NULL,NULL,NULL},
	{1, ALLOW_MIL, 0,     0,        1000.0, ALLOW_IN    ,NULL,NULL,NULL}
};
#define NUM_HUMAN_COORD (sizeof(human_coord) / sizeof(human_coord[0]))

static inline int try_human_coord(pcb_coord_t coord, const pcb_unit_t *unit, double down_limit, double up_limit, int score_factor, double *value, unsigned int *best, const char **suffix)
{
	double v, frac;
	long int digs, zeros;
	unsigned int score;

	/* convert the value to the proposed unit */
	if (unit->family == METRIC)
		v = PCB_COORD_TO_MM(coord);
	else
		v = PCB_COORD_TO_MIL(coord);
	v = v * unit->scale_factor;

	/* Check if neighbour units are better */
	if ((down_limit > 0) && (v < down_limit))
		return -1;
	if ((up_limit > 0) && (v > up_limit))
		return +1;

	/* count trailing zeros after the decimal point up to 8 digits */
	frac = v - floor(v);
	digs = frac * 100000000.0;
	if (digs != 0)
		for(zeros = 0; (digs % 10) == 0; zeros++, digs /= 10);
	else
		zeros = 8;

	/* score is higher for more zeroes */
	score = score_factor + 8 * zeros;

/*	printf("try: %s '%.8f' -> %.8f %d score=%d\n", unit->suffix, v, frac, zeros, score);*/

	/* update the best score */
	if (score > *best) {
		*value = v;
		*best = score;
		*suffix = unit->suffix;
	}

	return 0;
}

/* Same as CoordsToString but take only one coord and print it in human readable format */
static int CoordsToHumanString(gds_t *dest, pcb_coord_t coord, const gds_t *printf_spec_, enum e_suffix suffix_type)
{
	char filemode_buff[128]; /* G_ASCII_DTOSTR_BUF_SIZE */
	char printf_spec_new_local[256];
	char *printf_spec_new;
	int i, retval = -1, trunc0;
	const char *printf_spec = printf_spec_->array;
	const char *suffix;
	double value;
	unsigned int best_score = 0;

	i = printf_spec_->used + 64;
	if (i > sizeof(printf_spec_new_local))
		printf_spec_new = malloc(i);
	else
		printf_spec_new = printf_spec_new_local;

	/* cache unit lookup */
	if (human_coord[0].base_unit == NULL) {
		for(i = 0; i < NUM_HUMAN_COORD; i++) {
			human_coord[i].base_unit = get_unit_struct_by_allow(human_coord[i].base);
			human_coord[i].down_unit = get_unit_struct_by_allow(human_coord[i].down);
			human_coord[i].up_unit = get_unit_struct_by_allow(human_coord[i].up);
		}
	}

	for(i = 0; i < NUM_HUMAN_COORD; i++) {
		int res;
		/* try the base units first */
		res = try_human_coord(coord, human_coord[i].base_unit, human_coord[i].down_limit, human_coord[i].up_limit, human_coord[i].score_factor,&value, &best_score, &suffix);
		if (res < 0)
			try_human_coord(coord, human_coord[i].down_unit, 0, 0, human_coord[i].score_factor, &value, &best_score, &suffix);
		else if (res > 0)
			try_human_coord(coord, human_coord[i].up_unit, 0, 0, human_coord[i].score_factor,&value, &best_score, &suffix);
	}

	make_printf_spec(printf_spec_new, printf_spec, 8, &trunc0);
	sprintf_lc_safe((suffix_type == FILE_MODE), filemode_buff, printf_spec_new + 2, value);
	if (gds_append_str(dest, filemode_buff) != 0)
		goto err;

	if (value != 0) {
		if (suffix_type == NO_SUFFIX)
			suffix_type = SUFFIX;
		if (append_suffix(dest, suffix_type, suffix) != 0)
			goto err;
	}

	err:;
	if (printf_spec_new != printf_spec_new_local)
		free(printf_spec_new);
	return retval;
}

/* \brief Main low level pcb-printf function
 * \par Function Description
 * This is a printf wrapper that accepts new format specifiers to
 * output pcb coords as various units. See the comment at the top
 * of pcb-printf.h for full details.
 *
 * \param [in] string Append anything new at the end of this dynamic string (must be initialized before the call)
 * \param [in] fmt    Format specifier
 * \param [in] args   Arguments to specifier
 *
 * \return 0 on success
 */
int pcb_append_vprintf(gds_t *string, const char *fmt, va_list args)
{
	gds_t spec;
	char tmp[128]; /* large enough for rendering a long long int */
	int tmplen, retval = -1, slot_recursion = 0;
	char *free_fmt = NULL;
	enum pcb_allow_e mask = ALLOW_ALL;

	gds_init(&spec);

	new_fmt:;
	while (*fmt) {
		enum e_suffix suffix = NO_SUFFIX;

		if (*fmt == '%') {
			const char *ext_unit = "";
			pcb_coord_t value[10];
			int count, i;

			gds_truncate(&spec, 0);

			/* Get printf sub-specifiers */
			if (gds_append(&spec, *fmt++) != 0) goto err;
			while (isdigit(*fmt) || *fmt == '.' || *fmt == ' ' || *fmt == '*'
						 || *fmt == '#' || *fmt == 'l' || *fmt == 'L' || *fmt == 'h'
						 || *fmt == '+' || *fmt == '-' || *fmt == '[') {
				if (*fmt == '*') {
					char itmp[32];
					int ilen;
					ilen = sprintf(itmp, "%d", va_arg(args, int));
					if (gds_append_len(&spec, itmp, ilen) != 0) goto err;
					fmt++;
				}
				else
					if (gds_append(&spec, *fmt++) != 0) goto err;
			}

			/* check if specifier is %[] */
			if ((gds_len(&spec) > 2) && (spec.array[1] == '[')) {
				int slot = atoi(spec.array+2);
				gds_t new_spec;
				if ((slot < 0) || (slot > PCB_PRINTF_SLOT_max)) {
					fprintf(stderr, "Internal error: invalid printf slot addressed: '%s'\n", spec.array);
					abort();
				}
				slot_recursion++;
				if (slot_recursion > 16) {
					fprintf(stderr, "Internal error: printf slot recursion too deep on addressed: '%s'\n", spec.array);
					abort();
				}
				if (pcb_printf_slot[slot] == NULL) {
					fprintf(stderr, "Internal error: printf empty slot reference: '%s'\n", spec.array);
					abort();
				}
				gds_init(&new_spec);
				gds_append_str(&new_spec, pcb_printf_slot[slot]);
				if (*fmt == ']')
					fmt++;
				gds_append_str(&new_spec, fmt);
				if (free_fmt != NULL)
					free(free_fmt);
				fmt = free_fmt = new_spec.array;
				memset(&new_spec, 0, sizeof(new_spec));
				gds_truncate(&spec, 0);
				goto new_fmt;
			}
			else
				slot_recursion = 0;

			/* Get our sub-specifiers */
			if (*fmt == '#') {
				mask = ALLOW_CMIL;			/* This must be pcb's base unit */
				fmt++;
			}
			if (*fmt == '$') {
				suffix = SUFFIX;
				fmt++;
				if (*fmt == '$') {
					fmt++;
					suffix = FILE_MODE;
				}
			}
			/* Tack full specifier onto specifier */
			if (*fmt != 'm')
				if (gds_append(&spec, *fmt) != 0) goto err;
			switch (*fmt) {
				/* Printf specs */
			case 'o':
			case 'i':
			case 'd':
			case 'u':
			case 'x':
			case 'X':
				if (spec.array[1] == 'l') {
#if 0
/* C89 does not have long long int - best to avoid %lld */
					if (spec.array[2] == 'l')
						tmplen = sprintf(tmp, spec.array, va_arg(args, long long));
					else
#endif
						tmplen = sprintf(tmp, spec.array, va_arg(args, long));
				}
				else {
					tmplen = sprintf(tmp, spec.array, va_arg(args, int));
				}
				if (gds_append_len(string, tmp, tmplen) != 0) goto err;
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
				if (gds_append_len(string, tmp, tmplen) != 0) goto err;
				break;
			case 'c':
				if (spec.array[1] == 'l' && sizeof(int) <= sizeof(wchar_t))
					tmplen = sprintf(tmp, spec.array, va_arg(args, wchar_t));
				else
					tmplen = sprintf(tmp, spec.array, va_arg(args, int));
				if (gds_append_len(string, tmp, tmplen) != 0) goto err;
				break;
			case 's':
				if (spec.array[0] == 'l') {
					/* TODO: convert wchar to char and append it */
					fprintf(stderr, "Internal error: appending %%ls is not supported\n");
					abort();
				}
				else {
					const char *s = va_arg(args, const char *);
					if (s == NULL) s = "(null)";
					if (gds_append_str(string, s) != 0) goto err;
				}
				break;
			case 'n':
				/* Depending on gcc settings, this will probably break with
				 *  some silly "can't put %n in writable data space" message */
				tmplen = sprintf(tmp, spec.array, va_arg(args, int *));
				if (gds_append_len(string, tmp, tmplen) != 0) goto err;
				break;
			case 'p':
				tmplen = sprintf(tmp, spec.array, va_arg(args, void *));
				if (gds_append_len(string, tmp, tmplen) != 0) goto err;
				break;
			case '%':
				if (gds_append(string, '%') != 0) goto err;
				break;
				/* Our specs */
			case 'm':
				++fmt;
				if (*fmt == '*')
					ext_unit = va_arg(args, const char *);
				if (*fmt != '+' && *fmt != 'a' && *fmt != 'A')
					value[0] = va_arg(args, pcb_coord_t);
				count = 1;
				switch (*fmt) {
				case 'I':
					if (CoordsToString(string, value, 1, &spec, ALLOW_NM, NO_SUFFIX) != 0) goto err;
					break;
				case 's':
					if (CoordsToString(string, value, 1, &spec, ALLOW_MM | ALLOW_MIL, suffix) != 0) goto err;
					break;
				case 'S':
					if (CoordsToString(string, value, 1, &spec, mask & ALLOW_NATURAL, suffix) != 0) goto err;
					break;
				case 'H':
					if (CoordsToHumanString(string, value[0], &spec, suffix) != 0) goto err;
					break;
				case 'M':
					if (CoordsToString(string, value, 1, &spec, mask & ALLOW_METRIC, suffix) != 0) goto err;
					break;
				case 'L':
					if (CoordsToString(string, value, 1, &spec, mask & ALLOW_IMPERIAL, suffix) != 0) goto err;
					break;
				case 'k':
					if (CoordsToString(string, value, 1, &spec, mask & ALLOW_DMIL, suffix) != 0) goto err;
					break;
				case 'r':
					if (CoordsToString(string, value, 1, &spec, ALLOW_READABLE, NO_SUFFIX) != 0) goto err;
					break;
					/* All these fallthroughs are deliberate */
				case '9':
					value[count++] = va_arg(args, pcb_coord_t);
				case '8':
					value[count++] = va_arg(args, pcb_coord_t);
				case '7':
					value[count++] = va_arg(args, pcb_coord_t);
				case '6':
					value[count++] = va_arg(args, pcb_coord_t);
				case '5':
					value[count++] = va_arg(args, pcb_coord_t);
				case '4':
					value[count++] = va_arg(args, pcb_coord_t);
				case '3':
					value[count++] = va_arg(args, pcb_coord_t);
				case '2':
				case 'D':
					value[count++] = va_arg(args, pcb_coord_t);
					if (CoordsToString(string, value, count, &spec, mask & ALLOW_ALL, suffix) != 0) goto err;
					break;
				case 'd':
					value[1] = va_arg(args, pcb_coord_t);
					if (CoordsToString(string, value, 2, &spec, ALLOW_MM | ALLOW_MIL, suffix) != 0) goto err;
					break;
				case '*':
					{
						int found = 0;
						for (i = 0; i < get_n_units(); ++i) {
							if (strcmp(ext_unit, Units[i].suffix) == 0) {
								if (CoordsToString(string, value, 1, &spec, Units[i].allow, suffix) != 0) goto err;
								found = 1;
								break;
							}
						}
						if (!found)
							if (CoordsToString(string, value, 1, &spec, mask & ALLOW_ALL, suffix) != 0) goto err;
					}
					break;
				case 'a':
					if (gds_append_len(&spec, ".0f", 3) != 0) goto err;
					if (suffix == SUFFIX)
						if (gds_append_len(&spec, " deg", 4) != 0) goto err;
					tmplen = sprintf(tmp, spec.array, (double) va_arg(args, pcb_angle_t));
					if (gds_append_len(string, tmp, tmplen) != 0) goto err;
					break;
				case 'A':
					if (gds_append_len(&spec, ".0f", 3) != 0) goto err;
					/* if (suffix == SUFFIX)
						if (gds_append_len(&spec, " deg", 4) != 0) goto err;*/
					tmplen = sprintf(tmp, spec.array, 10*((double) va_arg(args, pcb_angle_t))); /* kicad legacy needs decidegrees */
					if (gds_append_len(string, tmp, tmplen) != 0) goto err;
					break;
				case '+':
					mask = va_arg(args, enum pcb_allow_e);
					break;
				default:
					{
						int found = 0;
						for (i = 0; i < get_n_units(); ++i) {
							if (*fmt == Units[i].printf_code) {
								if (CoordsToString(string, value, 1, &spec, Units[i].allow, suffix) != 0) goto err;
								found = 1;
								break;
							}
						}
						if (!found)
							if (CoordsToString(string, value, 1, &spec, ALLOW_ALL, suffix) != 0) goto err;
					}
					break;
				}
				break;
			}
		}
		else
			if (gds_append(string, *fmt) != 0) goto err;
		++fmt;
	}

	retval = 0;
err:;
	gds_uninit(&spec);
	if (free_fmt != NULL)
		free(free_fmt);

	return retval;
}

/* \brief Wrapper for pcb_append_vprintf that outputs to a string
 *
 * \param [in] string  Pointer to string to output into
 * \param [in] fmt     Format specifier
 *
 * \return The length of the written string
 */
int pcb_sprintf(char *string, const char *fmt, ...)
{
	gds_t str;
	va_list args;

	gds_init(&str);
	va_start(args, fmt);

	/* pretend the string is already allocated to something huge; this doesn't
	   make the code less safe but saves a copy */
	str.array = string;
	str.alloced = 1<<31;
	str.no_realloc = 1;

	pcb_append_vprintf(&str, fmt, args);

	va_end(args);
	return str.used;
}

/* \brief Wrapper for pcb_append_vprintf that outputs to a string with a size limit
 *
 * \param [in] string  Pointer to string to output into
 * \param [in] len     Maximum length
 * \param [in] fmt     Format specifier
 *
 * \return The length of the written string
 */
int pcb_snprintf(char *string, size_t len, const char *fmt, ...)
{
	gds_t str;
	va_list args;

	gds_init(&str);
	va_start(args, fmt);

	str.array = string;
	str.alloced = len;
	str.no_realloc = 1;

	pcb_append_vprintf(&str, fmt, args);

	va_end(args);
	return str.used;
}

int pcb_vsnprintf(char *string, size_t len, const char *fmt, va_list args)
{
	gds_t str;

	gds_init(&str);

	str.array = string;
	str.alloced = len;
	str.no_realloc = 1;

	pcb_append_vprintf(&str, fmt, args);

	return str.used;
}

/* \brief Wrapper for pcb_append_vprintf that outputs to a file
 *
 * \param [in] fh   File to output to
 * \param [in] fmt  Format specifier
 *
 * \return The length of the written string
 */
int pcb_fprintf(FILE * fh, const char *fmt, ...)
{
	int rv;
	va_list args;

	va_start(args, fmt);
	rv = pcb_vfprintf(fh, fmt, args);
	va_end(args);

	return rv;
}

/* \brief Wrapper for pcb_append_vprintf that outputs to a file
 *
 * \param [in] fh   File to output to
 * \param [in] fmt  Format specifier
 * \param [in] args   Arguments to specifier
 *
 * \return The length of the written string
 */
int pcb_vfprintf(FILE * fh, const char *fmt, va_list args)
{
	int rv;
	gds_t str;
	gds_init(&str);

	if (fh == NULL)
		rv = -1;
	else {
		pcb_append_vprintf(&str, fmt, args);
		rv = fprintf(fh, "%s", str.array);
	}

	gds_uninit(&str);
	return rv;
}

/* \brief Wrapper for pcb_append_vprintf that outputs to stdout
 *
 * \param [in] fmt  Format specifier
 *
 * \return The length of the written string
 */
int pcb_printf(const char *fmt, ...)
{
	int rv;
	gds_t str;
	va_list args;

	gds_init(&str);
	va_start(args, fmt);

	pcb_append_vprintf(&str, fmt, args);
	rv = printf("%s", str.array);

	va_end(args);
	gds_uninit(&str);
	return rv;
}

/* \brief Wrapper for pcb_append_vprintf that outputs to a newly allocated string
 *
 * \param [in] fmt  Format specifier
 *
 * \return The newly allocated string. Must be freed with free.
 */
char *pcb_strdup_printf(const char *fmt, ...)
{
	gds_t str;
	va_list args;

	gds_init(&str);

	va_start(args, fmt);
	pcb_append_vprintf(&str, fmt, args);
	va_end(args);

	return str.array; /* no other allocation has been made */
}

/* \brief Wrapper for pcb_append_vprintf that outputs to a string
 *
 * \param [in] string  Pointer to string to output into
 * \param [in] fmt     Format specifier
 *
 * \return return the new string; must be free()'d later
 */
char *pcb_strdup_vprintf(const char *fmt, va_list args)
{
	gds_t str;
	gds_init(&str);

	pcb_append_vprintf(&str, fmt, args);

	return str.array; /* no other allocation has been made */
}


/* \brief Wrapper for pcb_append_vprintf that appends to a string using vararg API
 *
 * \param [in] str  Existing dynamic string
 * \param [in] fmt  Format specifier
 *
 * \return 0 on success
 */
int pcb_append_printf(gds_t *str, const char *fmt, ...)
{
	int retval;

	va_list args;
	va_start(args, fmt);
	retval = pcb_append_vprintf(str, fmt, args);
	va_end(args);

	return retval;
}


char *pcb_strdup_subst(const char *template, int (*cb)(void *ctx, gds_t *s, const char **input), void *ctx)
{
	gds_t s;
	const char *curr, *next;

	gds_init(&s);
	for(curr = template;;) {
		next = strchr(curr, '%');
		if (next == NULL) {
			gds_append_str(&s, curr);
			return s.array;
		}
		if (next > curr)
			gds_append_len(&s, curr, next-curr);
		next++;
		switch(*next) {
			case '%':
				gds_append(&s, '%');
				curr = next+1;
				break;
			default:
				if (cb(ctx, &s, &next) != 0) {
					/* keep the directive intact */
					gds_append(&s, '%');
				}
				curr = next;
		}
	}
	abort(); /* can't get here */
}

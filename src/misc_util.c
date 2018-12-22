/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,2004,2006 Thomas Nau
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
 */

/* misc functions used by several modules
 */

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <genvector/gds_char.h>
#include <math.h>
#include <ctype.h>
#include "misc_util.h"
#include "unit.h"

/* pcb_distance() should be used so that there is only one
 *  place to deal with overflow/precision errors
 */
double pcb_distance(double x1, double y1, double x2, double y2)
{
	double delta_x = (x2 - x1);
	double delta_y = (y2 - y1);
	return sqrt(delta_x * delta_x + delta_y * delta_y);
}

/* pcb_distance2() should be used so that there is only one
 *  place to deal with overflow/precision errors
 */
double pcb_distance2(double x1, double y1, double x2, double y2)
{
	double delta_x = (x2 - x1);
	double delta_y = (y2 - y1);
	return delta_x * delta_x + delta_y * delta_y;
}

double pcb_get_value(const char *val, const char *units, pcb_bool * absolute, pcb_bool *success)
{
	return pcb_get_value_ex(val, units, absolute, NULL, "cmil", success);
}

pcb_bool pcb_get_value_unit(const char *val, pcb_bool *absolute, int unit_strict, double *val_out, const pcb_unit_t **unit_out)
{
	int ul, ulo = 0;
	const char *start = val;

	if ((*start == '-') || (*start == '+')) {
		start++;
		ulo = 1;
		if (absolute != NULL)
			*absolute = pcb_false;
	}
	else if (absolute != NULL)
		*absolute = pcb_true;

	ul = strspn(start, "0123456789.");
	if (ul > 0)
		ul += ulo;
	if ((ul > 0) && (val[ul] != '\0')) {
		const char *unit = val+ul;
		const pcb_unit_t *u;

		while(isspace(*unit)) unit++;
		if (*unit == '\0')
			goto err;
		else if ((unit[0] == 'm') && (unit[1] == '\0')) {
			/* corner case: mil and mm starts with m, and we rarely want to specify anything in meter, so ignore it */
			goto err;
		}

		u = get_unit_struct_(unit, unit_strict);
		if (u != NULL) {
			pcb_bool succ;
			double crd;

			crd = pcb_get_value(val, unit, NULL, &succ);
			if (succ) {
				*val_out = crd;
				*unit_out = u;
				return pcb_true;
			}
		}
	}

	err:;
		*val_out = 0;
		*unit_out = NULL;
		return pcb_false;
}


double pcb_get_value_ex(const char *val, const char *units, pcb_bool * absolute, pcb_unit_list_t extra_units, const char *default_unit, pcb_bool *success)
{
	double value;
	int n = -1;
	pcb_bool scaled = 0;
	pcb_bool dummy;

	/* Allow NULL to be passed for absolute */
	if (absolute == NULL)
		absolute = &dummy;

	/* if the first character is a sign we have to add the
	 * value to the current one
	 */
	if (*val == '=') {
		*absolute = pcb_true;
		sscanf(val + 1, "%lf%n", &value, &n);
		n++;
	}
	else {
		if (isdigit((int) *val))
			*absolute = pcb_true;
		else
			*absolute = pcb_false;
		sscanf(val, "%lf%n", &value, &n);
	}
	if (n <= 0)
		goto fail;

	if (!units && n > 0)
		units = val + n;

	while (units && isspace(*units))
		units++;

	if (units && *units) {
		int i, unit_ok = 0;
		const pcb_unit_t *unit = get_unit_struct(units);
		if (unit != NULL) {
			value = pcb_unit_to_coord(unit, value);
			scaled = 1;
			unit_ok = 1;
		}
		if (extra_units) {
			for (i = 0; *extra_units[i].suffix; ++i) {
				if (strncmp(units, extra_units[i].suffix, strlen(extra_units[i].suffix)) == 0) {
					value *= extra_units[i].scale;
					if (extra_units[i].flags & PCB_UNIT_PERCENT)
						value /= 100.0;
					scaled = 1;
					unit_ok = 1;
				}
			}
		}
		if ((!unit_ok) && (success != NULL)) /* there was something after the number but it doesn't look like a valid unit */
			goto fail;
	}

	/* Apply default unit */
	if (!scaled && default_unit && *default_unit) {
		int i;
		const pcb_unit_t *unit = get_unit_struct(default_unit);
		if (extra_units)
			for (i = 0; *extra_units[i].suffix; ++i)
				if (strcmp(extra_units[i].suffix, default_unit) == 0) {
					value *= extra_units[i].scale;
					if (extra_units[i].flags & PCB_UNIT_PERCENT)
						value /= 100.0;
					scaled = 1;
				}
		if (!scaled && unit != NULL)
			value = pcb_unit_to_coord(unit, value);
	}

	if (success != NULL)
		*success = 1;
	return value;

	fail:;
	if (success != NULL)
		*success = 0;
	return 0;
}

char *pcb_concat(const char *first, ...)
{
	gds_t buf;
	va_list a;
	const char *s;

	gds_init(&buf);
	va_start(a, first);
	for(s = first; s != NULL; s = va_arg(a, const char *))
		gds_append_str(&buf, s);
	va_end(a);
	return buf.array;
}

/* ---------------------------------------------------------------------------
 * strips leading and trailing blanks from the passed string and
 * returns a pointer to the new 'duped' one or NULL if the old one
 * holds only white space characters
 */
char *pcb_strdup_strip_wspace(const char *S)
{
	const char *p1, *p2;
	char *copy;
	size_t length;

	if (!S || !*S)
		return NULL;

	/* strip leading blanks */
	for (p1 = S; *p1 && isspace((int) *p1); p1++);

	/* strip trailing blanks and get string length */
	length = strlen(p1);
	for (p2 = p1 + length - 1; length && isspace((int) *p2); p2--, length--);

	/* string is not empty -> allocate memory */
	if (length) {
		copy = (char *) realloc(NULL, length + 1);
		strncpy(copy, p1, length);
		copy[length] = '\0';
		return copy;
	}
	else
		return NULL;
}

char *pcb_text_wrap(char *inp, int len, int sep, int nonsep)
{
	int cnt;
	char *s, *lastspc = NULL;
	for(s = inp, cnt = 0; *s != '\0'; s++,cnt++) {
		if ((*s == sep) && (nonsep >= 0))
			*s = nonsep;
		if ((cnt >= len) && (lastspc != NULL)) {
			cnt = 0;
			*lastspc = sep;
			lastspc = NULL;
		}
		else if (isspace(*s))
			lastspc = s;
	}
	return inp;
}

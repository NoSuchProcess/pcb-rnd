/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* misc functions used by several modules
 */

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "misc_util.h"
#include "unit.h"

/* Distance() should be used so that there is only one
 *  place to deal with overflow/precision errors
 */
double Distance(double x1, double y1, double x2, double y2)
{
	double delta_x = (x2 - x1);
	double delta_y = (y2 - y1);
	return sqrt(delta_x * delta_x + delta_y * delta_y);
}

/* Distance2() should be used so that there is only one
 *  place to deal with overflow/precision errors
 */
double Distance2(double x1, double y1, double x2, double y2)
{
	double delta_x = (x2 - x1);
	double delta_y = (y2 - y1);
	return delta_x * delta_x + delta_y * delta_y;
}

/* Get Value returns a numeric value passed from the string and sets the
 * pcb_bool variable absolute to false if it leads with a +/- character
 */
double GetValue(const char *val, const char *units, pcb_bool * absolute, pcb_bool *success)
{
	return GetValueEx(val, units, absolute, NULL, "cmil", success);
}

double GetValueEx(const char *val, const char *units, pcb_bool * absolute, pcb_unit_list_t extra_units, const char *default_unit, pcb_bool *success)
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
			value = unit_to_coord(unit, value);
			scaled = 1;
			unit_ok = 1;
		}
		if (extra_units) {
			for (i = 0; *extra_units[i].suffix; ++i) {
				if (strncmp(units, extra_units[i].suffix, strlen(extra_units[i].suffix)) == 0) {
					value *= extra_units[i].scale;
					if (extra_units[i].flags & UNIT_PERCENT)
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
					if (extra_units[i].flags & UNIT_PERCENT)
						value /= 100.0;
					scaled = 1;
				}
		if (!scaled && unit != NULL)
			value = unit_to_coord(unit, value);
	}

	if (success != NULL)
		*success = 1;
	return value;

	fail:;
	if (success != NULL)
		*success = 0;
	return 0;
}

char *Concat(const char *first, ...)
{
	char *rv;
	int len;
	va_list a;

	len = strlen(first);
	rv = (char *) malloc(len + 1);
	strcpy(rv, first);

	va_start(a, first);
	while (1) {
		const char *s = va_arg(a, const char *);
		if (!s)
			break;
		len += strlen(s);
		rv = (char *) realloc(rv, len + 1);
		strcat(rv, s);
	}
	va_end(a);
	return rv;
}

pcb_coord_t GetNum(char **s, const char *default_unit)
{
	/* Read value */
	pcb_coord_t ret_val = GetValueEx(*s, NULL, NULL, NULL, default_unit, NULL);
	/* Advance pointer */
	while (isalnum(**s) || **s == '.')
		(*s)++;
	return ret_val;
}

/* ---------------------------------------------------------------------------
 * strips leading and trailing blanks from the passed string and
 * returns a pointer to the new 'duped' one or NULL if the old one
 * holds only white space characters
 */
char *StripWhiteSpaceAndDup(const char *S)
{
	const char *p1, *p2;
	char *copy;
	size_t length;

	if (!S || !*S)
		return (NULL);

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
		return (copy);
	}
	else
		return (NULL);
}

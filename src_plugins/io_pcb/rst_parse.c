/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,2004,2006 Thomas Nau
 *  Copyright (C) 2016,2021 Tibor 'Igor2' Palinkas
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

#include "../lib_compat_help/route_style.h"

static const char rst_cookie[] = "core route style";

static rnd_coord_t pcb_get_num(char **s, const char *default_unit)
{
	/* Read value */
	rnd_coord_t ret_val = rnd_get_value_ex(*s, NULL, NULL, NULL, default_unit, NULL);
	/* Advance pointer */
	while (isalnum(**s) || **s == '.')
		(*s)++;
	return ret_val;
}

/* parses the routes definition string which is a colon separated list of
   comma separated Name, Dimension, Dimension, Dimension, Dimension
   e.g. Signal,20,40,20,10:Power,40,60,28,10:...
   Args: name,thickness,pad_dia,hole,[clearance],[mask]
   A more recent file format versio (2017? 2018?) has an 5th Dimension for
   mask. */
int pcb_route_string_parse1(pcb_data_t *data, char **str, pcb_route_style_t *routeStyle, const char *default_unit)
{
	rnd_coord_t hole_dia = 0, pad_dia = 0, mask = 0;
	char *s = *str;
	char Name[256];
	int i, len;

	while (*s && isspace((int) *s))
		s++;
	for (i = 0; *s && *s != ','; i++)
		Name[i] = *s++;
	Name[i] = '\0';
	len = strlen(Name);
	if (len > sizeof(routeStyle->name)-1) {
		memcpy(routeStyle->name, Name, sizeof(routeStyle->name)-1);
		routeStyle->name[sizeof(routeStyle->name)-1] = '\0';
		rnd_message(RND_MSG_WARNING, "Route style name '%s' too long, truncated to '%s'\n", Name, routeStyle->name);
	}
	else
		strcpy(routeStyle->name, Name);
	if (!isdigit((int) *++s))
		goto error;
	routeStyle->fid = -1;
	routeStyle->Thick = pcb_get_num(&s, default_unit);
	while (*s && isspace((int) *s))
		s++;
	if (*s++ != ',')
		goto error;
	while (*s && isspace((int) *s))
		s++;
	if (!isdigit((int) *s))
		goto error;
	pad_dia = pcb_get_num(&s, default_unit);
	while (*s && isspace((int) *s))
		s++;
	if (*s++ != ',')
		goto error;
	while (*s && isspace((int) *s))
		s++;
	if (!isdigit((int) *s))
		goto error;
	hole_dia = pcb_get_num(&s, default_unit);

	/* for backwards-compatibility, we use a 10-mil default
	   for styles which omit the clearance specification. */
	if (*s != ',')
		routeStyle->Clearance = RND_MIL_TO_COORD(10);
	else {
		s++;
		while (*s && isspace((int) *s))
			s++;
		if (!isdigit((int) *s))
			goto error;
		routeStyle->Clearance = pcb_get_num(&s, default_unit);
		while (*s && isspace((int) *s))
			s++;
	}

	/* Optional fields, extended file format */

	/* for backwards-compatibility, use mask=0 (tented via)
	   for styles which omit the mask specification. */
	if (*s != ',')
		mask = 0;
	else {
		s++;
		while (*s && isspace((int) *s))
			s++;
		if (!isdigit((int) *s))
			goto error;
		mask = pcb_get_num(&s, default_unit);
		while (*s && isspace((int) *s))
			s++;
	}

	if (pcb_compat_route_style_via_load(data, routeStyle, hole_dia, pad_dia, mask) != 0)
		rnd_message(RND_MSG_WARNING, "Route style '%s': falied to create via padstack prototype\n", routeStyle->name);

	*str = s;
	return 0;
	error:;
		*str = s;
		return -1;
}

int pcb_route_string_parse(pcb_data_t *data, char *s, vtroutestyle_t *styles, const char *default_unit)
{
	int n;

	vtroutestyle_truncate(styles, 0);
	for(n = 0;;n++) {
		vtroutestyle_enlarge(styles, n+1);
		if (pcb_route_string_parse1(data, &s, &styles->array[n], default_unit) != 0) {
			n--;
			break;
		}
		while (*s && isspace((int) *s))
			s++;
		if (*s == '\0')
			break;
		if (*s++ != ':') {
			vtroutestyle_truncate(styles, 0);
			return -1;
		}
	}
	vtroutestyle_truncate(styles, n+1);
	return 0;
}


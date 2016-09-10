/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,2004,2006 Thomas Nau
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
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */
#include "config.h"
#include "global.h"
#include "pcb-printf.h"
#include "genvector/gds_char.h"
#include "route_style.h"
#include "misc.h"
#include "error.h"
#include "conf.h"

RouteStyleType pcb_custom_route_style;

/*! \brief Serializes the route style list 
 *  \par Function Description
 *  Right now n_styles should always be set to NUM_STYLES,
 *  since that is the number of route styles ParseRouteString()
 *  expects to parse.
 */
char *make_route_string(vtroutestyle_t *styles)
{
	gds_t str;
	int i;

	gds_init(&str);
	for (i = 0; i < vtroutestyle_len(styles); ++i) {
		pcb_append_printf(&str, "%s,%mc,%mc,%mc,%mc", styles->array[i].name,
																				 styles->array[i].Thick, styles->array[i].Diameter,
																				 styles->array[i].Hole, styles->array[i].Clearance);
		if (i > 0)
			gds_append(&str, ':');
	}
	return str.array; /* this is the only allocation made, return this and don't uninit */
}

/* ----------------------------------------------------------------------
 * parses the routes definition string which is a colon separated list of
 * comma separated Name, Dimension, Dimension, Dimension, Dimension
 * e.g. Signal,20,40,20,10:Power,40,60,28,10:...
 */
int ParseRoutingString1(char **str, RouteStyleTypePtr routeStyle, const char *default_unit)
{
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
		Message(PCB_MSG_DEFAULT, "Route style name '%s' too long, truncated to '%s'\n", Name, routeStyle->name);
	}
	else
		strcpy(routeStyle->name, Name);
	if (!isdigit((int) *++s))
		goto error;
	routeStyle->Thick = GetNum(&s, default_unit);
	while (*s && isspace((int) *s))
		s++;
	if (*s++ != ',')
		goto error;
	while (*s && isspace((int) *s))
		s++;
	if (!isdigit((int) *s))
		goto error;
	routeStyle->Diameter = GetNum(&s, default_unit);
	while (*s && isspace((int) *s))
		s++;
	if (*s++ != ',')
		goto error;
	while (*s && isspace((int) *s))
		s++;
	if (!isdigit((int) *s))
		goto error;
	routeStyle->Hole = GetNum(&s, default_unit);
	/* for backwards-compatibility, we use a 10-mil default
	 * for styles which omit the clearance specification. */
	if (*s != ',')
		routeStyle->Clearance = PCB_MIL_TO_COORD(10);
	else {
		s++;
		while (*s && isspace((int) *s))
			s++;
		if (!isdigit((int) *s))
			goto error;
		routeStyle->Clearance = GetNum(&s, default_unit);
		while (*s && isspace((int) *s))
			s++;
	}

	*str = s;
	return 0;
	error:;
		*str = s;
		return -1;
}

int ParseRouteString(char *s, vtroutestyle_t *styles, const char *default_unit)
{
	int n;

	vtroutestyle_truncate(styles, 0);
	for(n = 0;;n++) {
		vtroutestyle_enlarge(styles, n+1);
		if (ParseRoutingString1(&s, &styles->array[n], default_unit) != 0) {
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

void pcb_use_route_style(RouteStyleType * rst)
{
	conf_set_design("design/line_thickness", "%$mS", rst->Thick);
	conf_set_design("design/via_thickness", "%$mS", rst->Diameter);
	conf_set_design("design/via_drilling_hole", "%$mS", rst->Hole);
	conf_set_design("design/clearance", "%$mS", rst->Clearance);
}

int pcb_use_route_style_idx(vtroutestyle_t *styles, int idx)
{
	if ((idx < 0) || (idx >= vtroutestyle_len(styles)))
		return -1;
	pcb_use_route_style(styles->array+idx);
	return 0;
}

#define cmp(a,b) (((a) != 0) && (coord_abs((a)-(b)) > 32))
#define cmps(a,b) (((a) != NULL) && (strcmp((a), (b)) != 0))
int pcb_route_style_lookup(vtroutestyle_t *styles, Coord Thick, Coord Diameter, Coord Hole, Coord Clearance, char *Name)
{
	int n;
	for (n = 0; n < vtroutestyle_len(styles); n++) {
		if (cmp(Thick, styles->array[n].Thick)) continue;
		if (cmp(Diameter, styles->array[n].Diameter)) continue;
		if (cmp(Hole, styles->array[n].Hole)) continue;
		if (cmp(Clearance, styles->array[n].Clearance)) continue;
		if (cmps(Name, styles->array[n].name)) continue;
		return n;
	}
	return -1;
}
#undef cmp


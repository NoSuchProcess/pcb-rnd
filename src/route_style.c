/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */
#include <ctype.h>
#include "config.h"
#include "pcb-printf.h"
#include "genvector/gds_char.h"
#include "route_style.h"
#include "error.h"
#include "conf.h"
#include "misc_util.h"
#include "board.h"
#include "funchash_core.h"
#include "conf_core.h"
#include "compat_nls.h"

pcb_route_style_t pcb_custom_route_style;


static pcb_coord_t pcb_get_num(char **s, const char *default_unit)
{
	/* Read value */
	pcb_coord_t ret_val = pcb_get_value_ex(*s, NULL, NULL, NULL, default_unit, NULL);
	/* Advance pointer */
	while (isalnum(**s) || **s == '.')
		(*s)++;
	return ret_val;
}


/* Serializes the route style list */
char *pcb_route_string_make(vtroutestyle_t *styles)
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
int pcb_route_string_parse1(char **str, pcb_route_style_t *routeStyle, const char *default_unit)
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
		pcb_message(PCB_MSG_WARNING, "Route style name '%s' too long, truncated to '%s'\n", Name, routeStyle->name);
	}
	else
		strcpy(routeStyle->name, Name);
	if (!isdigit((int) *++s))
		goto error;
	routeStyle->Thick = pcb_get_num(&s, default_unit);
	while (*s && isspace((int) *s))
		s++;
	if (*s++ != ',')
		goto error;
	while (*s && isspace((int) *s))
		s++;
	if (!isdigit((int) *s))
		goto error;
	routeStyle->Diameter = pcb_get_num(&s, default_unit);
	while (*s && isspace((int) *s))
		s++;
	if (*s++ != ',')
		goto error;
	while (*s && isspace((int) *s))
		s++;
	if (!isdigit((int) *s))
		goto error;
	routeStyle->Hole = pcb_get_num(&s, default_unit);
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
		routeStyle->Clearance = pcb_get_num(&s, default_unit);
		while (*s && isspace((int) *s))
			s++;
	}

	*str = s;
	return 0;
	error:;
		*str = s;
		return -1;
}

int pcb_route_string_parse(char *s, vtroutestyle_t *styles, const char *default_unit)
{
	int n;

	vtroutestyle_truncate(styles, 0);
	for(n = 0;;n++) {
		vtroutestyle_enlarge(styles, n+1);
		if (pcb_route_string_parse1(&s, &styles->array[n], default_unit) != 0) {
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

void pcb_use_route_style(pcb_route_style_t * rst)
{
	conf_set_design("design/line_thickness", "%$mS", rst->Thick);
	conf_set_design("design/text_scale", "%d", rst->texts <= 0 ? 100 : rst->texts);
	conf_set_design("design/text_thickness", "%$mS", rst->textt);
	conf_set_design("design/via_thickness", "%$mS", rst->Diameter);
	conf_set_design("design/via_drilling_hole", "%$mS", rst->Hole);
	conf_set_design("design/clearance", "%$mS", rst->Clearance);
	PCB->pen_attr = &rst->attr;
}

int pcb_use_route_style_idx(vtroutestyle_t *styles, int idx)
{
	if ((idx < 0) || (idx >= vtroutestyle_len(styles)))
		return -1;
	pcb_use_route_style(styles->array+idx);
	return 0;
}

#define cmp(a,b) (((a) != -1) && (coord_abs((a)-(b)) > 32))
#define cmps(a,b) (((a) != NULL) && (strcmp((a), (b)) != 0))
int pcb_route_style_match(pcb_route_style_t *rst, pcb_coord_t Thick, pcb_coord_t Diameter, pcb_coord_t Hole, pcb_coord_t Clearance, char *Name)
{
	if (cmp(Thick, rst->Thick)) return 0;
	if (cmp(Diameter, rst->Diameter)) return 0;
	if (cmp(Hole, rst->Hole)) return 0;
	if (cmp(Clearance, rst->Clearance)) return 0;
	if (cmps(Name, rst->name)) return 0;
	return 1;
}
#undef cmp
#undef cmps

int pcb_route_style_lookup(vtroutestyle_t *styles, pcb_coord_t Thick, pcb_coord_t Diameter, pcb_coord_t Hole, pcb_coord_t Clearance, char *Name)
{
	int n;
	for (n = 0; n < vtroutestyle_len(styles); n++)
		if (pcb_route_style_match(&styles->array[n], Thick, Diameter, Hole, Clearance, Name))
			return n;
	return -1;
}


int pcb_get_style_size(int funcid, pcb_coord_t * out, int type, int size_id)
{
	switch (funcid) {
	case F_Object:
		switch (type) {
		case PCB_OBJ_LINE:
			return pcb_get_style_size(F_SelectedLines, out, 0, size_id);
		case PCB_OBJ_ARC:
			return pcb_get_style_size(F_SelectedArcs, out, 0, size_id);
		}
		pcb_message(PCB_MSG_ERROR, "Sorry, can't fetch the style of that object type (%x)\n", type);
		return -1;
	case F_SelectedPads:
		if (size_id != 2)						/* don't mess with pad size */
			return -1;
	case F_SelectedVias:
	case F_SelectedPins:
	case F_SelectedObjects:
	case F_Selected:
	case F_SelectedElements:
		if (size_id == 0)
			*out = conf_core.design.via_thickness;
		else if (size_id == 1)
			*out = conf_core.design.via_drilling_hole;
		else
			*out = conf_core.design.clearance;
		break;
	case F_SelectedArcs:
	case F_SelectedLines:
		if (size_id == 2)
			*out = conf_core.design.clearance;
		else
			*out = conf_core.design.line_thickness;
		return 0;
	case F_SelectedTexts:
	case F_SelectedNames:
		pcb_message(PCB_MSG_ERROR, "Sorry, can't change style of every selected object\n");
		return -1;
	}
	return 0;
}

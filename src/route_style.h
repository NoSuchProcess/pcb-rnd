/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2004 Thomas Nau
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "vtroutestyle.h"

/* Set design configuration (the pen we draw with) to a given route style */
void pcb_use_route_style(pcb_route_style_t *rst);

/* Same as pcb_use_route_style() but uses one of the styles in a vector;
   returns -1 if idx is out of bounds, 0 on success. */
int pcb_use_route_style_idx(vtroutestyle_t *styles, int idx);

/* Compare supplied parameters to each style in the vector and return the index
   of the first matching style. All non-(-1) parameters need to match to accept
   a style. Return -1 on no match. The strict version returns match only if
   the route style did set all values explicitly and not matching "wildcard" from
   the style's side */
int pcb_route_style_lookup(vtroutestyle_t *styles, rnd_coord_t Thick, rnd_coord_t textt, int texts, pcb_font_id_t fid, rnd_coord_t Diameter, rnd_coord_t Hole, rnd_coord_t Clearance, rnd_cardinal_t via_proto, char *Name);
int pcb_route_style_lookup_strict(vtroutestyle_t *styles, rnd_coord_t Thick, rnd_coord_t textt, int texts, pcb_font_id_t fid, rnd_coord_t Diameter, rnd_coord_t Hole, rnd_coord_t Clearance, rnd_cardinal_t via_proto, char *Name);

/* Return 1 if rst matches the style in supplied args. Same matching rules as
   in pcb_route_style_lookup(). */
int pcb_route_style_match(pcb_route_style_t *rst, rnd_coord_t Thick, rnd_coord_t textt, int texts, pcb_font_id_t fid, rnd_coord_t Diameter, rnd_coord_t Hole, rnd_coord_t Clearance, rnd_cardinal_t via_proto, char *Name);

/* helper: get route style size for a function and selected object type.
   size_id: 0=main size; 1=2nd size (drill); 2=clearance */
int pcb_get_style_size(int funcid, rnd_coord_t * out, int type, int size_id);

#define PCB_LOOKUP_ROUTE_STYLE_PEN_(pcb, how) \
	pcb_route_style_ ## how(&pcb->RouteStyle,\
		conf_core.design.line_thickness, \
		conf_core.design.text_thickness, \
		conf_core.design.text_scale, \
		conf_core.design.text_font_id, \
		conf_core.design.via_thickness, \
		conf_core.design.via_drilling_hole, \
		conf_core.design.clearance, \
		conf_core.design.via_proto, \
		NULL)

#define PCB_LOOKUP_ROUTE_STYLE_PEN(pcb) \
	PCB_LOOKUP_ROUTE_STYLE_PEN_(pcb, lookup)

#define PCB_LOOKUP_ROUTE_STYLE_PEN_STRICT(pcb) \
	PCB_LOOKUP_ROUTE_STYLE_PEN_(pcb, lookup_strict)

/* Tries strict first, if that fails, returns normal lookup */
int pcb_lookup_route_style_pen_bestfit(pcb_board_t *pcb);


#define PCB_MATCH_ROUTE_STYLE_PEN(rst) \
	pcb_route_style_match(rst,\
		conf_core.design.line_thickness, \
		conf_core.design.text_thickness, \
		conf_core.design.text_scale, \
		conf_core.design.text_font_id, \
		conf_core.design.via_thickness, \
		conf_core.design.via_drilling_hole, \
		conf_core.design.clearance, \
		conf_core.design.via_proto, \
		NULL)


/*** Undoable changes to route styles ***/

/* Change a field. Returns 0 on success. */
int pcb_route_style_change(pcb_board_t *pcb, int rstidx, rnd_coord_t *thick, rnd_coord_t *textt, int *texts, pcb_font_id_t *fid, rnd_coord_t *clearance, rnd_cardinal_t *via_proto, rnd_bool undoable);

/* Change the name. Returns 0 on success. */
int pcb_route_style_change_name(pcb_board_t *pcb, int rstidx, const char *new_name, rnd_bool undoable);

/* Returns the index (counted from 0) or -1 on error */
int pcb_route_style_new(pcb_board_t *pcb, const char *name, rnd_bool undoable);

/* Returns 0 on success. */
int pcb_route_style_del(pcb_board_t *pcb, int idx, rnd_bool undoable);

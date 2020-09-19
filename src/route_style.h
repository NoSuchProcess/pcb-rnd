/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2004 Thomas Nau
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "vtroutestyle.h"

/* Parse a single route string into one pcb_route_style_t *slot. Returns 0 on success.  */
int pcb_route_string_parse1(char **str, pcb_route_style_t *routeStyle, const char *default_unit);

/* Parse a ':' separated list of route strings into a styles vector
   The vector is initialized before the call. On error the vector is left empty
   (but still initialized). Returns 0 on success. */
int pcb_route_string_parse(char *s, vtroutestyle_t *styles, const char *default_unit);

char *pcb_route_string_make(vtroutestyle_t *styles);

/* Set design configuration (the pen we draw with) to a given route style */
void pcb_use_route_style(pcb_route_style_t *rst);

/* Same as pcb_use_route_style() but uses one of the styles in a vector;
   returns -1 if idx is out of bounds, 0 on success. */
int pcb_use_route_style_idx(vtroutestyle_t *styles, int idx);

/* Compare supplied parameters to each style in the vector and return the index
   of the first matching style. All non-(-1) parameters need to match to accept
   a style. Return -1 on no match. */
int pcb_route_style_lookup(vtroutestyle_t *styles, rnd_coord_t Thick, rnd_coord_t Diameter, rnd_coord_t Hole, rnd_coord_t Clearance, char *Name);

/* Return 1 if rst matches the style in supplied args. Same matching rules as
   in pcb_route_style_lookup(). */
int pcb_route_style_match(pcb_route_style_t *rst, rnd_coord_t Thick, rnd_coord_t Diameter, rnd_coord_t Hole, rnd_coord_t Clearance, char *Name);

extern pcb_route_style_t pcb_custom_route_style;

/* helper: get route style size for a function and selected object type.
   size_id: 0=main size; 1=2nd size (drill); 2=clearance */
int pcb_get_style_size(int funcid, rnd_coord_t * out, int type, int size_id);


/*** Undoable changes to route styles ***/

/* Change a field. Returns 0 on success. */
int pcb_route_style_change(pcb_board_t *pcb, int rstidx, rnd_coord_t *thick, rnd_coord_t *textt, int *texts, rnd_coord_t *clearance, rnd_cardinal_t *via_proto, rnd_bool undoable);

/* Change the name. Returns 0 on success. */
int pcb_route_style_change_name(pcb_board_t *pcb, int rstidx, const char *new_name, rnd_bool undoable);

/* Returns the index (counted from 0) or -1 on error */
int pcb_route_style_new(pcb_board_t *pcb, const char *name, rnd_bool undoable);

/* Returns 0 on success. */
int pcb_route_style_del(pcb_board_t *pcb, int idx, rnd_bool undoable);

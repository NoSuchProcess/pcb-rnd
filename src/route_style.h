/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#define STYLE_LOOP(top)  do {                                       \
        Cardinal n;                                                 \
        RouteStyleTypePtr style;                                    \
        for (n = 0; n < vtroutestyle_len(&(top)->RouteStyle); n++)  \
        {                                                           \
                style = &(top)->RouteStyle.array[n]

/* Parse a single route string into one RouteStyleTypePtr slot. Returns 0 on success.  */
int ParseRoutingString1(char **str, RouteStyleTypePtr routeStyle, const char *default_unit);

/* Parse a ':' separated list of route strings into a styles vector
   The vector is initialized before the call. On error the vector is left empty
   (but still initialized). Returns 0 on success. */
int ParseRouteString(char *s, vtroutestyle_t *styles, const char *default_unit);

char *make_route_string(vtroutestyle_t *styles);

/* Set design configuration (the pen we draw with) to a given route style */
void pcb_use_route_style(RouteStyleType *rst);

/* Same as pcb_use_route_style() but uses one of the styles in a vector;
   returns -1 if idx is out of bounds, 0 on success. */
int pcb_use_route_style_idx(vtroutestyle_t *styles, int idx);

/* Compare supplied parameters to each style in the vector and return the index
   of the first mathcing style. All non-0 parameters need to match to accept
   a style. Return -1 on no match. */
int pcb_route_style_lookup(vtroutestyle_t *styles, Coord Thick, Coord Diameter, Coord Hole, Coord Clearance, char *Name);

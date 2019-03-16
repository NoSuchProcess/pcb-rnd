/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2004 Thomas Nau
 *  Copyright (C) 2017 Adrian Purser
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

#ifndef PCB_ROUTE_H
#define PCB_ROUTE_H

#define ROUTE_SMALL_DATA_SIZE 4

#include "obj_common.h"
#include "global_typedefs.h"

typedef struct {
	pcb_objtype_t type;
	pcb_point_t point1; /* Line: Start Point, Arc: Center Point */
	pcb_point_t point2; /* Line: End Point */
	pcb_coord_t radius; /* Arc */
	pcb_angle_t start_angle; /* Arc */
	pcb_angle_t delta_angle; /* Arc */
	pcb_layer_id_t layer;
} pcb_route_object_t;

typedef struct {
	pcb_point_t start_point;
	pcb_point_t end_point;
	pcb_coord_t thickness;
	pcb_coord_t clearance;
	pcb_layer_id_t start_layer; /* The ID of the layer that the route started on */
	pcb_layer_id_t end_layer; /* The ID of the layer that the route ended on, usually the same as the start for simple routes */
	pcb_board_t *PCB;
	pcb_flag_t flags;
	int size; /* The number of active objects in the array */
	int capacity; /* The size of the object array */
	pcb_route_object_t *objects; /* Pointer to the object array data */
	pcb_route_object_t small_data[ROUTE_SMALL_DATA_SIZE]; /* Small object array used to avoid allocating memory for small routes */
} pcb_route_t;


void pcb_route_init(pcb_route_t *p_route);
void pcb_route_destroy(pcb_route_t *p_route);
void pcb_route_reset(pcb_route_t *p_route);
void pcb_route_reserve(pcb_route_t *p_route, int size);
void pcb_route_resize(pcb_route_t *p_route, int size);
void pcb_route_start(pcb_board_t *PCB, pcb_route_t *route, pcb_point_t *point, pcb_layer_id_t layer_id, pcb_coord_t thickness, pcb_coord_t clearance, pcb_flag_t flags);

void pcb_route_add_line(pcb_route_t *p_route, pcb_point_t *point1, pcb_point_t *point2, pcb_layer_id_t layer);
void pcb_route_add_arc(pcb_route_t *p_route, pcb_point_t *center, pcb_angle_t start_angle, pcb_angle_t delta, pcb_coord_t radius, pcb_layer_id_t layer);

void pcb_route_calculate(pcb_board_t *PCB, pcb_route_t *route, pcb_point_t *point1, pcb_point_t *point2, pcb_layer_id_t layer_id, pcb_coord_t thickness, pcb_coord_t clearance, pcb_flag_t flags, int mod1, int mod2);
void pcb_route_calculate_to(pcb_route_t *route, pcb_point_t *point, int mod1, int mod2);
void pcb_route_direct(pcb_board_t *PCB, pcb_route_t *p_route, pcb_point_t *point1, pcb_point_t *point2, pcb_layer_id_t layer, pcb_coord_t thickness, pcb_coord_t clearance, pcb_flag_t flags);

int pcb_route_apply(const pcb_route_t *p_route);
int pcb_route_apply_to_line(const pcb_route_t *p_route, pcb_layer_t *Layer, pcb_line_t *apply_to_line);
int pcb_route_apply_to_arc(const pcb_route_t *p_route, pcb_layer_t *apply_to_arc_layer, pcb_arc_t *apply_to_arc);

#endif

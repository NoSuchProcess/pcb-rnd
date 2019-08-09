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
#include "config.h"

#include "compat_misc.h"
#include "conf_core.h"
#include "hidlib_conf.h"
#include "math_helper.h"
#include "board.h"
#include "data.h"
#include "find.h"
#include "polygon.h"
#include "rtree.h"
#include "route.h"
#include "undo.h"
#include "obj_line_draw.h"
#include "obj_arc_draw.h"
#include "route_draw.h"
#include "obj_line.h"
#include "obj_line_op.h"
#include "draw_wireframe.h"

void pcb_route_init(pcb_route_t *p_route)
{
	p_route->size = 0;
	p_route->capacity = ROUTE_SMALL_DATA_SIZE;
	p_route->objects = &p_route->small_data[0];
}

void pcb_route_destroy(pcb_route_t *p_route)
{
	if (p_route->capacity > ROUTE_SMALL_DATA_SIZE)
		free(p_route->objects);
	p_route->size = 0;
	p_route->capacity = ROUTE_SMALL_DATA_SIZE;
	p_route->objects = &p_route->small_data[0];
}

void pcb_route_reset(pcb_route_t *p_route)
{
	/* NOTE:  Currently this function just sets the size back to zero. It does not
	   free any memory used so the capacity will stay the same. 
	 */

	p_route->size = 0;
}

void pcb_route_reserve(pcb_route_t *p_route, int size)
{
	int grow;

	if (size <= p_route->capacity)
		return;

	grow = size - p_route->capacity;
	if (grow < 8)
		grow = 8;

	if (p_route->capacity == ROUTE_SMALL_DATA_SIZE) {
		p_route->capacity += grow;
		p_route->objects = (pcb_route_object_t *) malloc(p_route->capacity * sizeof(pcb_route_object_t));
		memcpy(p_route->objects, &p_route->small_data[0], p_route->size * sizeof(pcb_route_object_t));
	}
	else {
		p_route->capacity += grow;
		p_route->objects = (pcb_route_object_t *) realloc(p_route->objects, p_route->capacity * sizeof(pcb_route_object_t));
	}
}

void pcb_route_resize(pcb_route_t *p_route, int size)
{
	pcb_route_reserve(p_route, size);
	p_route->size = size;
}

pcb_route_object_t *pcb_route_alloc_object(pcb_route_t *p_route)
{
	pcb_route_resize(p_route, p_route->size + 1);
	return &p_route->objects[p_route->size - 1];
}

void pcb_route_add_line(pcb_route_t *p_route, pcb_point_t *point1, pcb_point_t *point2, pcb_layer_id_t layer)
{
	pcb_route_object_t *p_object = pcb_route_alloc_object(p_route);
	if (p_object == NULL)
		return;

	p_object->point1 = *point1;
	p_object->point2 = *point2;
	p_object->layer = layer;
	p_object->type = PCB_OBJ_LINE;
	p_route->end_point = *point2;
}

void pcb_route_add_arc(pcb_route_t *p_route, pcb_point_t *center, pcb_angle_t start_angle, pcb_angle_t delta, pcb_coord_t radius, pcb_layer_id_t layer)
{
	pcb_route_object_t *p_object = pcb_route_alloc_object(p_route);
	if (p_object == NULL)
		return;

	p_object->point1 = *center;
	p_object->start_angle = start_angle;
	p_object->delta_angle = delta;
	p_object->radius = radius;
	p_object->layer = layer;
	p_object->type = PCB_OBJ_ARC;

	p_route->end_point.X = pcb_round((double)center->X - ((double)radius * cos((start_angle + delta) * PCB_M180)));
	p_route->end_point.Y = pcb_round((double)center->Y + ((double)radius * sin((start_angle + delta) * PCB_M180)));
}


void pcb_route_direct(pcb_board_t *PCB, pcb_route_t *route, pcb_point_t *point1, pcb_point_t *point2, pcb_layer_id_t layer, pcb_coord_t thickness, pcb_coord_t clearance, pcb_flag_t flags)
{
	pcb_route_reset(route);
	route->start_point = *point1;
	route->end_point = *point2;
	route->start_layer = layer;
	route->end_layer = layer;
	route->thickness = thickness;
	route->clearance = clearance;
	route->PCB = PCB;
	route->flags = flags;
	pcb_route_add_line(route, point1, point2, layer);
}

/*-----------------------------------------------------------
 * Calculate an arc fitted to a corner.
 *---------------------------------------------------------*/

void pcb_route_calculate_corner_arc(const pcb_point_t *point1, const pcb_point_t *point2, const pcb_point_t *point3, double radius, pcb_route_object_t *p_out_obj, pcb_point_t *p_endpoint1, pcb_point_t *p_endpoint2)
{
	const double r_min = 10.0;

	const pcb_coord_t dx1 = point1->X - point2->X;
	const pcb_coord_t dy1 = point1->Y - point2->Y;
	const pcb_coord_t dx2 = point3->X - point2->X;
	const pcb_coord_t dy2 = point3->Y - point2->Y;

	const double angle1 = atan2(dy1, dx1);
	const double angle2 = atan2(dy2, dx2);

	const double ad = angle2 - angle1;
	const double d = ad > M_PI ? ad - (M_PI * 2.0) : (ad < -M_PI ? ad + (M_PI * 2.0) : ad);

	const double hangle = fabs(d * 0.5);
	const double angle3 = angle1 + (d * 0.5);

	/* Vector from point2 to the other points */
	const double vx1 = cos(angle1);
	const double vy1 = sin(angle1);
	const double vx2 = cos(angle2);
	const double vy2 = sin(angle2);

	/* Distance from point2 to the other points */
	const double l1 = fabs(fabs(vx1) > fabs(vy1) ? dx1 / vx1 : dy1 / vy1);
	const double l2 = fabs(fabs(vx2) > fabs(vy2) ? dx2 / vx2 : dy2 / vy2);

	/* Calculate maximum possible radius */
	const double rmax = (l1 < l2 ? l1 : l2) * tan(hangle);
	const double r = rmax < radius ? rmax : radius;

	if (r >= r_min) {
		/* Calculate arc center coordinates. */
		const double sh = sin(hangle);
		const pcb_coord_t xc = point2->X + (pcb_coord_t) ((cos(angle3) * r) / sh);
		const pcb_coord_t yc = point2->Y + (pcb_coord_t) ((sin(angle3) * r) / sh);

		/* Calculate arc start and delta angles. */
		const double delta = d < 0 ? -(M_PI + d) : M_PI - d;
		const double start = (d < 0.0 ? (M_PI * 0.5) - angle1 : ((M_PI * 0.5) - angle2) - delta);

		p_out_obj->point1.X = xc;
		p_out_obj->point1.Y = yc;
		p_out_obj->start_angle = start * PCB_RAD_TO_DEG; /* Start Angle */
		p_out_obj->delta_angle = delta * PCB_RAD_TO_DEG; /* Delta Angle */
		p_out_obj->radius = r;

		if (p_endpoint1) {
			p_endpoint1->X = xc - (pcb_coord_t) (r * cos(start));
			p_endpoint1->Y = yc + (pcb_coord_t) (r * sin(start));
		}

		if (p_endpoint1) {
			p_endpoint2->X = xc - (pcb_coord_t) (r * cos(start + delta));
			p_endpoint2->Y = yc + (pcb_coord_t) (r * sin(start + delta));
		}
	}
	else {
		if (p_endpoint1)
			*p_endpoint1 = *point2;

		if (p_endpoint2)
			*p_endpoint2 = *point2;
	}
}

void pcb_route_calculate_45(pcb_point_t *start_point, pcb_point_t *target_point)
{

	pcb_coord_t dx, dy, min;
	unsigned direction = 0;
	double m;

	/* first calculate direction of line */
	dx = target_point->X - start_point->X;
	dy = target_point->Y - start_point->Y;

	if (!dx) {
		if (!dy)
			/* zero length line, don't draw anything */
			return;
		else
			direction = dy > 0 ? 0 : 4;
	}
	else {
		m = (double)dy / dx;
		direction = 2;
		if (m > PCB_TAN_30_DEGREE)
			direction = m > PCB_TAN_60_DEGREE ? 0 : 1;
		else if (m < -PCB_TAN_30_DEGREE)
			direction = m < -PCB_TAN_60_DEGREE ? 0 : 3;
	}

	if (dx < 0)
		direction += 4;

	dx = coord_abs(dx);
	dy = coord_abs(dy);
	min = MIN(dx, dy);

	/* now set up the second pair of coordinates */
	switch (direction) {
		case 0:
		case 4:
			target_point->X = start_point->X;
			break;

		case 2:
		case 6:
			target_point->Y = start_point->Y;
			break;

		case 1:
			target_point->X = start_point->X + min;
			target_point->Y = start_point->Y + min;
			break;

		case 3:
			target_point->X = start_point->X + min;
			target_point->Y = start_point->Y - min;
			break;

		case 5:
			target_point->X = start_point->X - min;
			target_point->Y = start_point->Y - min;
			break;

		case 7:
			target_point->X = start_point->X - min;
			target_point->Y = start_point->Y + min;
			break;
	}
}

void pcb_route_start(pcb_board_t *PCB, pcb_route_t *route, pcb_point_t *point, pcb_layer_id_t layer_id, pcb_coord_t thickness, pcb_coord_t clearance, pcb_flag_t flags)
{
	/* Restart the route */
	pcb_route_reset(route);
	route->start_point = *point;
	route->end_point = *point;
	route->thickness = thickness;
	route->clearance = clearance;
	route->start_layer = layer_id;
	route->end_layer = layer_id;
	route->PCB = PCB;
	route->flags = flags;
}

void pcb_route_calculate_to(pcb_route_t *route, pcb_point_t *point, int mod1, int mod2)
{
	TODO("If an external route calculator has been selected then use it instead of this default one.")
	TODO("Add DRC Checking")

	pcb_point_t *point1 = &route->end_point;
	pcb_point_t *point2 = point;
	pcb_layer_id_t layer_id = route->end_layer;

	/* Set radius to 0 for standard 45/90 operation */
	const pcb_coord_t radius = route->thickness * conf_core.editor.route_radius;

	/* If the start point is the same as the end point then add a single 0-length line. 
	 * If a 0-length line is not added then some operations such as moving a line point
	 * could cause the line to disappear.
	 */
	if ((point1->X == point2->X) && (point1->Y == point2->Y)) {
		route->end_point = *point2;
		pcb_route_add_line(route, point1, point2, layer_id);
	}
	/* If Refraction is 0 then add a single line segment that is horizontal, vertical or 45 degrees. 
	 * This line segment might not end under the crosshair.
	 */
	else if (conf_core.editor.line_refraction == 0) {
		pcb_point_t target = *point2;
		pcb_route_calculate_45(point1, &target);
		pcb_route_add_line(route, point1, &target, layer_id);
		route->end_point = target;
	}
	else {
		/* Refraction is non-zero so add multiple lines (horizontal, vertical and/or 45 degrees). */
		pcb_coord_t dx, dy;
		pcb_point_t target;
		pcb_bool way = (conf_core.editor.line_refraction == 1 ? pcb_false : pcb_true);

		/* swap the 'way' if mod1 is set (typically the shift key) */
		if (mod1)
			way = !way;

		dx = point2->X - point1->X;
		dy = point2->Y - point1->Y;

		if (!way) {
			if (coord_abs(dx) > coord_abs(dy)) {
				target.X = point2->X - SGN(dx) * coord_abs(dy);
				target.Y = point1->Y;
			}
			else {
				target.X = point1->X;
				target.Y = point2->Y - SGN(dy) * coord_abs(dx);
			}
		}
		else {
			if (coord_abs(dx) > coord_abs(dy)) {
				target.X = point1->X + SGN(dx) * coord_abs(dy);
				target.Y = point2->Y;
			}
			else {
				target.X = point2->X;
				target.Y = point1->Y + SGN(dy) * coord_abs(dx);;
			}
		}

		if (radius > 0.0) {
			pcb_route_object_t arc;
			pcb_point_t target1;
			pcb_point_t target2;

			pcb_route_calculate_corner_arc(point1, &target, point2, radius, &arc, &target1, &target2);

			if ((point1->X != target1.X) || (point1->Y != target1.Y))
				pcb_route_add_line(route, point1, &target1, layer_id);

			if ((target1.X != target2.X) || (target1.Y != target2.Y))
				pcb_route_add_arc(route, &arc.point1, arc.start_angle, arc.delta_angle, arc.radius, layer_id);

			if ((point2->X != target2.X) || (point2->Y != target2.Y))
				pcb_route_add_line(route, &target2, point2, layer_id);

			route->end_point = *point2;
		}
		else {
			if ((point1->X != target.X) || (point1->Y != target.Y))
				pcb_route_add_line(route, point1, &target, layer_id);
			if ((point2->X != target.X) || (point2->Y != target.Y))
				pcb_route_add_line(route, &target, point2, layer_id);
			route->end_point = *point2;
		}
	}

}

TODO("Pass in other required information such as object flags")
void pcb_route_calculate(pcb_board_t *PCB, pcb_route_t *route, pcb_point_t *point1, pcb_point_t *point2, pcb_layer_id_t layer_id, pcb_coord_t thickness, pcb_coord_t clearance, pcb_flag_t flags, int mod1, int mod2)
{
	/* Set radius to 0 for standard 45/90 operation */
/*	const pcb_coord_t radius = thickness * conf_core.editor.route_radius; - TODO: remove this if not needed */

	pcb_route_start(PCB, route, point1, layer_id, thickness, clearance, flags);

	/* If the line can be drawn directly to the target then add a single line segment. */
	if (conf_core.editor.all_direction_lines) {
		pcb_route_add_line(route, point1, point2, layer_id);
		return;
	}

	pcb_route_calculate_to(route, point2, mod1, mod2);
}

int pcb_route_apply(const pcb_route_t *p_route)
{
	return pcb_route_apply_to_line(p_route, NULL, NULL);
}

int pcb_route_apply_to_line(const pcb_route_t *p_route, pcb_layer_t *apply_to_line_layer, pcb_line_t *apply_to_line)
{
	int i;
	int applied = 0;

	for(i = 0; i < p_route->size; i++) {
		pcb_route_object_t const *p_obj = &p_route->objects[i];
		pcb_layer_t *layer = pcb_loose_subc_layer(PCB, pcb_get_layer(PCB->Data, p_obj->layer), pcb_true);

		switch (p_obj->type) {
			case PCB_OBJ_LINE:
				/*  If the route is being applied to an existing line then the existing
				   line will be used as the first line in the route. This maintains the
				   ID of the line and line points. This is especially useful if the 
				   route contains a single line.
				 */
				if (apply_to_line) {
					/* Move the existing line points to the position of the route line. */
					if ((p_obj->point1.X != apply_to_line->Point1.X) || (p_obj->point1.Y != apply_to_line->Point1.Y))
						pcb_undo_add_obj_to_move(PCB_OBJ_LINE_POINT, apply_to_line_layer, apply_to_line, &apply_to_line->Point1, p_obj->point1.X - apply_to_line->Point1.X, p_obj->point1.Y - apply_to_line->Point1.Y);

					if ((p_obj->point2.X != apply_to_line->Point2.X) || (p_obj->point2.Y != apply_to_line->Point2.Y))
						pcb_undo_add_obj_to_move(PCB_OBJ_LINE_POINT, apply_to_line_layer, apply_to_line, &apply_to_line->Point2, p_obj->point2.X - apply_to_line->Point2.X, p_obj->point2.Y - apply_to_line->Point2.Y);

					/* Move the existing line point/s */
					pcb_line_invalidate_erase(apply_to_line);
					pcb_r_delete_entry(apply_to_line_layer->line_tree, (pcb_box_t *) apply_to_line);
					pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_LINE, apply_to_line_layer, apply_to_line);
					apply_to_line->Point1.X = p_obj->point1.X;
					apply_to_line->Point1.Y = p_obj->point1.Y;
					apply_to_line->Point2.X = p_obj->point2.X;
					apply_to_line->Point2.Y = p_obj->point2.Y;
					pcb_line_bbox(apply_to_line);
					pcb_r_insert_entry(layer->line_tree, (pcb_box_t *) apply_to_line);
					pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_LINE, layer, apply_to_line);
					pcb_line_invalidate_draw(layer, apply_to_line);
					apply_to_line_layer = layer;

					/* The existing line has been used so forget about it. */
					apply_to_line = NULL;
					applied = 1;
				}
				else {
					/* Create a new line */
					pcb_line_t *line = pcb_line_new_merge(layer,
																								p_obj->point1.X,
																								p_obj->point1.Y,
																								p_obj->point2.X,
																								p_obj->point2.Y,
																								p_route->thickness,
																								p_route->clearance,
																								p_route->flags);
					if (line) {
						pcb_added_lines++;
						pcb_obj_add_attribs(line, PCB->pen_attr);
						pcb_line_invalidate_draw(layer, line);
						pcb_undo_add_obj_to_create(PCB_OBJ_LINE, layer, line, line);
						applied = 1;
					}
				}
				break;

			case PCB_OBJ_ARC:
				{
					/* Create a new arc */
					pcb_arc_t *arc = pcb_arc_new(layer,
																			 p_obj->point1.X,
																			 p_obj->point1.Y,
																			 p_obj->radius,
																			 p_obj->radius,
																			 p_obj->start_angle,
																			 p_obj->delta_angle,
																			 p_route->thickness,
																			 p_route->clearance,
																			 p_route->flags, pcb_true);
					if (arc) {
						pcb_added_lines++;
						pcb_obj_add_attribs(arc, PCB->pen_attr);
						pcb_undo_add_obj_to_create(PCB_OBJ_ARC, layer, arc, arc);
						pcb_arc_invalidate_draw(layer, arc);
						applied = 1;
					}
				}
				break;

			default:
				break;
		}
	}

	/* If the existing apply-to-line wasn't updated then it should be deleted */
	/* (This can happen if the route does not contain any lines.)             */
	if (apply_to_line != NULL)
		pcb_line_destroy(apply_to_line_layer, apply_to_line);

	if (applied)
		pcb_subc_as_board_update(PCB);

	return applied;
}

int pcb_route_apply_to_arc(const pcb_route_t *p_route, pcb_layer_t *apply_to_arc_layer, pcb_arc_t *apply_to_arc)
{
	int i;
	int applied = 0;

	for(i = 0; i < p_route->size; i++) {
		pcb_route_object_t const *p_obj = &p_route->objects[i];
		pcb_layer_t *layer = pcb_loose_subc_layer(PCB, pcb_get_layer(PCB->Data, p_obj->layer), pcb_true);

		switch (p_obj->type) {
			case PCB_OBJ_ARC:
				/* If the route is being applied to an existing arc then the existing
				   arc will be used as the first arc in the route. This maintains the
				   ID of the arc.
				 */
				if (apply_to_arc) {
					int changes = 0;

					/* Move the existing line points to the position of the route line. */
					if ((p_obj->radius != apply_to_arc->Width) || (p_obj->radius != apply_to_arc->Height)) {
						pcb_undo_add_obj_to_change_radii(PCB_OBJ_ARC, apply_to_arc, apply_to_arc, apply_to_arc);
						++changes;
					}

					if ((p_obj->start_angle != apply_to_arc->StartAngle) || (p_obj->delta_angle != apply_to_arc->Delta)) {
						pcb_undo_add_obj_to_change_angles(PCB_OBJ_ARC, apply_to_arc, apply_to_arc, apply_to_arc);
						++changes;
					}

					if (p_route->thickness != apply_to_arc->Thickness) {
						pcb_undo_add_obj_to_size(PCB_OBJ_ARC, apply_to_arc_layer, apply_to_arc, apply_to_arc);
						++changes;
					}

					if ((p_obj->point1.X != apply_to_arc->X) || (p_obj->point1.Y != apply_to_arc->Y)) {
						pcb_undo_add_obj_to_move(PCB_OBJ_ARC, apply_to_arc_layer, apply_to_arc, NULL, p_obj->point1.X - apply_to_arc->X, p_obj->point1.Y - apply_to_arc->Y);
						++changes;
					}

					if (changes > 0) {
						/* Modify the existing arc */
						pcb_arc_invalidate_erase(apply_to_arc);

						pcb_r_delete_entry(apply_to_arc_layer->arc_tree, (pcb_box_t *) apply_to_arc);
						pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_ARC, apply_to_arc_layer, apply_to_arc);

						apply_to_arc->X = p_obj->point1.X;
						apply_to_arc->Y = p_obj->point1.Y;
						apply_to_arc->Width = p_obj->radius;
						apply_to_arc->Height = p_obj->radius;
						apply_to_arc->StartAngle = p_obj->start_angle;
						apply_to_arc->Delta = p_obj->delta_angle;
						apply_to_arc->Thickness = p_route->thickness;
						pcb_arc_bbox(apply_to_arc);
						pcb_r_insert_entry(apply_to_arc_layer->arc_tree, (pcb_box_t *) apply_to_arc);
						pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_ARC, apply_to_arc_layer, apply_to_arc);
						pcb_arc_invalidate_draw(layer, apply_to_arc);
						apply_to_arc_layer = layer;
					}

					/* The existing arc has been used so forget about it. */
					apply_to_arc = NULL;
					applied = 1;
				}
				else {
					/* Create a new arc */
					pcb_arc_t *arc = pcb_arc_new(layer,
																			 p_obj->point1.X,
																			 p_obj->point1.Y,
																			 p_obj->radius,
																			 p_obj->radius,
																			 p_obj->start_angle,
																			 p_obj->delta_angle,
																			 p_route->thickness,
																			 p_route->clearance,
																			 p_route->flags, pcb_true);
					if (arc) {
						pcb_added_lines++;
						pcb_obj_add_attribs(arc, PCB->pen_attr);
						pcb_undo_add_obj_to_create(PCB_OBJ_ARC, layer, arc, arc);
						pcb_arc_invalidate_draw(layer, arc);
						applied = 1;
					}
				}
				break;

			case PCB_OBJ_LINE:
				{
					/* Create a new line */
					pcb_line_t *line = pcb_line_new_merge(layer,
																								p_obj->point1.X,
																								p_obj->point1.Y,
																								p_obj->point2.X,
																								p_obj->point2.Y,
																								p_route->thickness,
																								p_route->clearance,
																								p_route->flags);
					if (line) {
						pcb_added_lines++;
						pcb_obj_add_attribs(line, PCB->pen_attr);
						pcb_line_invalidate_draw(layer, line);
						pcb_undo_add_obj_to_create(PCB_OBJ_LINE, layer, line, line);
						applied = 1;
					}
				}
				break;

			default:
				break;
		}
	}

	/* If the existing apply-to-arc wasn't updated then it should be deleted */
	/* (This can happen if the route does not contain any arcs.)             */
	if (apply_to_arc != NULL)
		pcb_arc_destroy(apply_to_arc_layer, apply_to_arc);

	if (applied)
		pcb_subc_as_board_update(PCB);

	return applied;
}

/*=============================================================================
 * 
 *  Route Drawing
 *
 *===========================================================================*/

/*-----------------------------------------------------------
 * Draws the outline of an arc
 *---------------------------------------------------------*/
void pcb_route_draw_arc(pcb_hid_gc_t GC, pcb_coord_t x, pcb_coord_t y, pcb_angle_t start_angle, pcb_angle_t delta, pcb_coord_t radius, pcb_coord_t thickness)
{
	double x1, y1, x2, y2, wid = thickness / 2;

	if (delta < 0) {
		start_angle += delta;
		delta = -delta;
	}

	x1 = x - (cos(PCB_M180 * start_angle) * radius);
	y1 = y + (sin(PCB_M180 * start_angle) * radius);
	x2 = x - (cos(PCB_M180 * (start_angle + delta)) * radius);
	y2 = y + (sin(PCB_M180 * (start_angle + delta)) * radius);

	pcb_render->draw_arc(GC, x, y, radius + wid, radius + wid, start_angle, delta);
	if (wid > pcb_pixel_slop) {
		pcb_render->draw_arc(GC, x, y, radius - wid, radius - wid, start_angle, delta);
		pcb_render->draw_arc(GC, x1, y1, wid, wid, start_angle, -180 * SGN(delta));
		pcb_render->draw_arc(GC, x2, y2, wid, wid, start_angle + delta, 180 * SGN(delta));
	}

}

/*-----------------------------------------------------------
 * Draws the route as outlines
 *---------------------------------------------------------*/
void pcb_route_draw(pcb_route_t *p_route, pcb_hid_gc_t GC)
{
	int i = 0;
	for(i = 0; i < p_route->size; ++i) {
		const pcb_route_object_t *p_obj = &p_route->objects[i];

		pcb_layer_t *layer = pcb_get_layer(PCB->Data, p_obj->layer);
		if (layer)
			pcb_render->set_color(GC, &layer->meta.real.color);

		switch (p_obj->type) {
			case PCB_OBJ_LINE:
				pcb_draw_wireframe_line(GC, p_obj->point1.X, p_obj->point1.Y, p_obj->point2.X, p_obj->point2.Y, p_route->thickness, 0);
				break;

			case PCB_OBJ_ARC:
				pcb_route_draw_arc(GC, p_obj->point1.X, p_obj->point1.Y, p_obj->start_angle, p_obj->delta_angle, p_obj->radius, p_route->thickness);
				break;

			default:
				break;
		}
	}
}

/*-----------------------------------------------------------
 * Draws a drc outline around the route
 *---------------------------------------------------------*/
void pcb_route_draw_drc(pcb_route_t *p_route, pcb_hid_gc_t GC)
{
	pcb_coord_t thickness = p_route->thickness + 2 * conf_core.design.bloat;
	int i;

	pcb_render->set_color(GC, &pcbhl_conf.appearance.color.cross);

	for(i = 0; i < p_route->size; ++i) {
		const pcb_route_object_t *p_obj = &p_route->objects[i];

		switch (p_obj->type) {
			case PCB_OBJ_LINE:
				pcb_draw_wireframe_line(GC, p_obj->point1.X, p_obj->point1.Y, p_obj->point2.X, p_obj->point2.Y, thickness, 0);
				break;

			case PCB_OBJ_ARC:
				pcb_route_draw_arc(GC, p_obj->point1.X, p_obj->point1.Y, p_obj->start_angle, p_obj->delta_angle, p_obj->radius, thickness);
				break;

			default:
				break;
		}
	}
}

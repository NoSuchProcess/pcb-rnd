/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2021 Tibor 'Igor2' Palinkas
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
 */

/* Returns coords of endpoint of obj that is on 'on' */
static int map_seg_get_end_line_line(pcb_line_t *obj, pcb_line_t *on, rnd_coord_t *ox, rnd_coord_t *oy)
{
	rnd_coord_t r = obj->Thickness/2;
	if (pcb_is_point_on_line(obj->Point1.X, obj->Point1.Y, r, on)) {
		*ox = obj->Point1.X; *oy = obj->Point1.Y;
		return 0;
	}
	if (pcb_is_point_on_line(obj->Point2.X, obj->Point2.Y, r, on)) {
		*ox = obj->Point2.X; *oy = obj->Point2.Y;
		return 0;
	}
	return -1;
}

/* Returns coords of endpoint of obj that is on 'on' */
static int map_seg_get_end_arc_line(pcb_arc_t *obj, pcb_line_t *on, rnd_coord_t *ox, rnd_coord_t *oy)
{
	rnd_coord_t r = obj->Thickness/2, ex, ey;
	int n;

	for(n = 0; n < 2; n++) {
		pcb_arc_get_end(obj, n, &ex, &ey);
		if (pcb_is_point_on_line(ex, ey, r, on)) {
			*ox = ex; *oy = ey;
			return 0;
		}
	}
	return -1;
}

/* check which endpoint of obj falls on hub line and return the
   centerline x;y coords of that point */
static int map_seg_get_end_coords_on_line(pcb_any_obj_t *obj, pcb_line_t *hub, rnd_coord_t *ox, rnd_coord_t *oy)
{
	switch(obj->type) {
		case PCB_OBJ_LINE: return map_seg_get_end_line_line((pcb_line_t *)obj, hub, ox, oy);
		case PCB_OBJ_ARC:  return map_seg_get_end_arc_line((pcb_arc_t *)obj, hub, ox, oy);
		default:
			*ox = *oy = 0;
			return -1;
	}
	return 0;
}

/* Returns coords of endpoint of obj that is on 'on' */
static int map_seg_get_end_line_arc(pcb_line_t *obj, pcb_arc_t *on, rnd_angle_t *isc)
{
	rnd_coord_t r = obj->Thickness/2;
	if (pcb_is_point_on_arc(obj->Point1.X, obj->Point1.Y, r, on)) {
		*isc = pcb_arc_get_angle(on, obj->Point1.X, obj->Point1.Y);
		return 0;
	}
	if (pcb_is_point_on_arc(obj->Point2.X, obj->Point2.Y, r, on)) {
		*isc = pcb_arc_get_angle(on, obj->Point2.X, obj->Point2.Y);
		return 0;
	}
	return -1;
}


/* Returns angle (in second arc) of endpoint of obj that is on 'on' */
static int map_seg_get_end_arc_arc(pcb_arc_t *obj, pcb_arc_t *on, rnd_angle_t *isc)
{
	rnd_coord_t r = obj->Thickness/2, ex, ey;
	int n;

	for(n = 0; n < 2; n++) {
		pcb_arc_get_end(obj, n, &ex, &ey);
		if (pcb_is_point_on_arc(ex, ey, r, on)) {
			*isc = pcb_arc_get_angle(on, ex, ey);
			return 0;
		}
	}
	return -1;
}


/* check which endpoint of obj falls on hub arc and return the
   angle of that point */
static int map_seg_get_end_coords_on_arc(pcb_any_obj_t *obj, pcb_arc_t *hub, rnd_angle_t *isc)
{
	switch(obj->type) {
		case PCB_OBJ_LINE: return map_seg_get_end_line_arc((pcb_line_t *)obj, hub, isc);
		case PCB_OBJ_ARC:  return map_seg_get_end_arc_arc((pcb_arc_t *)obj, hub, isc);
		default:
			*isc = 0;
			return -1;
	}
	return 0;
}


/* Loads ox and oy with the coordinate of the intersection between trace objects
   a and b. Also loads enda and endb with the endpoint bit (1 or 2 for ends; 0
   for middle) */
static int map_seg_get_isc_coords(pcb_any_obj_t *a, pcb_any_obj_t *b, rnd_coord_t *ox, rnd_coord_t *oy)
{
	rnd_angle_t ang;

	if ((a->type == PCB_OBJ_LINE) && (map_seg_get_end_coords_on_line(b, (pcb_line_t *)a, ox, oy) == 0))
		return 0;

	if ((b->type == PCB_OBJ_LINE) && (map_seg_get_end_coords_on_line(a, (pcb_line_t *)b, ox, oy) == 0))
		return 0;

	if ((a->type == PCB_OBJ_ARC) && (map_seg_get_end_coords_on_arc(b, (pcb_arc_t *)a, &ang) == 0)) {
		if (pcb_arc_get_xy((pcb_arc_t *)a, ang, ox, oy) != 0)
			return -1;

		return 0;
	}

	if ((b->type == PCB_OBJ_ARC) && (map_seg_get_end_coords_on_arc(a, (pcb_arc_t *)b, &ang) == 0)) {
		if (pcb_arc_get_xy((pcb_arc_t *)b, ang, ox, oy) != 0)
			return -1;

		return 0;
	}

	return -1;
}


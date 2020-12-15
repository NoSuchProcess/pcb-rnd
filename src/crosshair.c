/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>

#include "brave.h"

#include "board.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "draw_wireframe.h"
#include "search.h"
#include "polygon.h"
#include <librnd/core/actions.h>
#include <librnd/core/hid_inlines.h>
#include <librnd/core/compat_misc.h>
#include <librnd/poly/offset.h>
#include "find.h"
#include "undo.h"
#include "event.h"
#include "thermal.h"
#include <librnd/core/grid.h>

#include "obj_line_draw.h"
#include "obj_arc_draw.h"
#include "obj_text_draw.h"
#include "obj_pstk_draw.h"
#include "obj_gfx_draw.h"
#include "route_draw.h"
#include "obj_arc_ui.h"
#include "obj_subc_parent.h"

#include <librnd/core/tool.h>

typedef struct {
	int x, y;
} point;

static char crosshair_cookie[] = "crosshair";

pcb_crosshair_t pcb_crosshair;  /* information about cursor settings */
rnd_mark_t pcb_marked;
pcb_crosshair_note_t pcb_crosshair_note;


static void thindraw_moved_ps(pcb_pstk_t *ps, rnd_coord_t x, rnd_coord_t y)
{
	/* Make a copy of the pin structure, moved to the correct position */
	pcb_pstk_t moved_ps = *ps;
	moved_ps.x += x;
	moved_ps.y += y;

	pcb_pstk_thindraw(NULL, pcb_crosshair.GC, &moved_ps);
}

/* ---------------------------------------------------------------------------
 * creates a tmp polygon with coordinates converted to screen system
 */
void pcb_xordraw_poly(pcb_poly_t *polygon, rnd_coord_t dx, rnd_coord_t dy, int dash_last)
{
	rnd_cardinal_t i;
	for (i = 0; i < polygon->PointN; i++) {
		rnd_cardinal_t next = pcb_poly_contour_next_point(polygon, i);

		if (next == 0) { /* last line: sometimes the implicit closing line */
			if (i == 1) /* corner case: don't draw two lines on top of each other - with XOR it looks bad */
				continue;

			if (dash_last) {
				pcb_draw_dashed_line(NULL, pcb_crosshair.GC,
									 polygon->Points[i].X + dx,
									 polygon->Points[i].Y + dy, polygon->Points[next].X + dx, polygon->Points[next].Y + dy, 5, rnd_false);
				break; /* skip normal line draw below */
			}
		}

		/* normal contour line */
		rnd_render->draw_line(pcb_crosshair.GC,
								 polygon->Points[i].X + dx,
								 polygon->Points[i].Y + dy, polygon->Points[next].X + dx, polygon->Points[next].Y + dy);
	}
}

void pcb_xordraw_pline(rnd_pline_t *pline, rnd_coord_t dx, rnd_coord_t dy)
{
	rnd_vnode_t *n = pline->head;
	do {
		rnd_render->draw_line(pcb_crosshair.GC,
			 n->point[0] + dx, n->point[1] + dy,
			 n->next->point[0] + dx, n->next->point[1] + dy);
		n = n->next;
	} while(n != pline->head);
}

void pcb_xordraw_polyarea(rnd_polyarea_t *parea, rnd_coord_t dx, rnd_coord_t dy)
{
	rnd_polyarea_t *pa = parea;
	rnd_pline_t *pl;
	do {
		for(pl = pa->contours; pl != NULL; pl = pl->next)
			pcb_xordraw_pline(pl, dx, dy);
		pa = pa->f;
	} while(pa != parea);
}

/* ---------------------------------------------------------------------------
 * creates a tmp polygon with coordinates converted to screen system, designed
 * for subc paste xor-draw
 */
void pcb_xordraw_poly_subc(pcb_poly_t *polygon, rnd_coord_t dx, rnd_coord_t dy, rnd_coord_t w, rnd_coord_t h, int mirr)
{
	rnd_cardinal_t i;
	for (i = 0; i < polygon->PointN; i++) {
		rnd_cardinal_t next = pcb_poly_contour_next_point(polygon, i);

		if (next == 0) { /* last line: sometimes the implicit closing line */
			if (i == 1) /* corner case: don't draw two lines on top of each other - with XOR it looks bad */
				continue;
		}

		/* normal contour line */
		rnd_render->draw_line(pcb_crosshair.GC,
			PCB_CSWAP_X(polygon->Points[i].X, w, mirr) + dx,
			PCB_CSWAP_Y(polygon->Points[i].Y, h, mirr) + dy,
			PCB_CSWAP_X(polygon->Points[next].X, w, mirr) + dx,
			PCB_CSWAP_Y(polygon->Points[next].Y, h, mirr) + dy);
	}
}

void pcb_xordraw_attached_arc(rnd_coord_t thick)
{
	pcb_arc_t arc;
	rnd_coord_t wx, wy;
	rnd_angle_t sa, dir;

	wx = pcb_crosshair.X - pcb_crosshair.AttachedBox.Point1.X;
	wy = pcb_crosshair.Y - pcb_crosshair.AttachedBox.Point1.Y;
	if (wx == 0 && wy == 0)
		return;
	arc.X = pcb_crosshair.AttachedBox.Point1.X;
	arc.Y = pcb_crosshair.AttachedBox.Point1.Y;
	if (RND_XOR(pcb_crosshair.AttachedBox.otherway, coord_abs(wy) > coord_abs(wx))) {
		arc.X = pcb_crosshair.AttachedBox.Point1.X + coord_abs(wy) * RND_SGNZ(wx);
		sa = (wx >= 0) ? 0 : 180;
		dir = (RND_SGNZ(wx) == RND_SGNZ(wy)) ? 90 : -90;
	}
	else {
		arc.Y = pcb_crosshair.AttachedBox.Point1.Y + coord_abs(wx) * RND_SGNZ(wy);
		sa = (wy >= 0) ? -90 : 90;
		dir = (RND_SGNZ(wx) == RND_SGNZ(wy)) ? -90 : 90;
		wy = wx;
	}
	wy = coord_abs(wy);
	arc.StartAngle = sa;
	arc.Delta = dir;
	arc.Width = arc.Height = wy;
	arc.Thickness = thick;

	pcb_draw_wireframe_arc(pcb_crosshair.GC, &arc, thick);
}

/* ---------------------------------------------------------------------------
 * draws all visible and attached objects of the pastebuffer
 */
void pcb_xordraw_buffer(pcb_buffer_t *Buffer)
{
	rnd_cardinal_t i;
	rnd_coord_t x, y;

	/* set offset */
	x = pcb_crosshair.AttachedObject.tx - Buffer->X;
	y = pcb_crosshair.AttachedObject.ty - Buffer->Y;

	/* draw all visible layers */
	for (i = 0; i < pcb_max_layer(PCB); i++)
		if (PCB->Data->Layer[i].meta.real.vis) {
			pcb_layer_t *layer = &Buffer->Data->Layer[i];

			PCB_LINE_LOOP(layer);
			{
				pcb_draw_wireframe_line(	pcb_crosshair.GC,
																	x + line->Point1.X, y + line->Point1.Y, 
																	x + line->Point2.X, y + line->Point2.Y,
																	line->Thickness,0 );
			}
			PCB_END_LOOP;
			PCB_ARC_LOOP(layer);
			{
				pcb_arc_t translated_arc = *arc;
				translated_arc.X += x;
				translated_arc.Y += y;
				pcb_draw_wireframe_arc(pcb_crosshair.GC, &translated_arc, arc->Thickness);
			}
			PCB_END_LOOP;
			PCB_TEXT_LOOP(layer);
			{
				pcb_text_draw_xor(text, x, y, 1);
			}
			PCB_END_LOOP;
			/* the tmp polygon has n+1 points because the first
			 * and the last one are set to the same coordinates
			 */
			PCB_POLY_LOOP(layer);
			{
				pcb_xordraw_poly(polygon, x, y, 0);
			}
			PCB_END_LOOP;
			PCB_GFX_LOOP(layer);
			{
				pcb_gfx_draw_xor(gfx, x, y);
			}
			PCB_END_LOOP;
		}

	/* draw subcircuit */
	PCB_SUBC_LOOP(Buffer->Data);
	{
		pcb_xordraw_subc(subc, x, y, Buffer->from_outside);
	}
	PCB_END_LOOP;

	/* and the padstacks */
	if (PCB->pstk_on)
		PCB_PADSTACK_LOOP(Buffer->Data);
	{
		thindraw_moved_ps(padstack, x, y);
	}
	PCB_END_LOOP;
}

/* ---------------------------------------------------------------------------
 * draws the rubberband to insert points into polygons/lines/...
 */
void pcb_xordraw_insert_pt_obj(void)
{
	pcb_line_t *line = (pcb_line_t *) pcb_crosshair.AttachedObject.Ptr2;
	rnd_point_t *point = (rnd_point_t *) pcb_crosshair.AttachedObject.Ptr3;

	if (pcb_crosshair.AttachedObject.Type != PCB_OBJ_VOID) {
		rnd_render->draw_line(pcb_crosshair.GC, point->X, point->Y, line->Point1.X, line->Point1.Y);
		rnd_render->draw_line(pcb_crosshair.GC, point->X, point->Y, line->Point2.X, line->Point2.Y);
	}
}

/* project dx;fy onto an orthogonal to vx;vy and return the offset */
static void proj_extend(double *ox, double *oy, double vx, double vy, double dx, double dy)
{
	double nx, ny, l, offs;

	if ((vx == 0) && (vy == 0)) {
		*ox = *oy = 0;
		return;
	}

	nx = -vy; ny = vx; /* normal vector */

	l = nx*nx+ny*ny;

	 /* normalize n */
	l = sqrt(l);
	nx /= l; ny /= l;

	/* dot product: projected length */
	offs = (dx * nx + dy * ny);

	/* offsets: normal's direction * projected len */
	*ox =nx * offs; *oy = ny * offs;
}


/* extend the pine between px;py and (already moved) x;y knowing the previous
   point beyond px;py (which is ppx;ppy) */
static void perp_extend(rnd_coord_t ppx, rnd_coord_t ppy, rnd_coord_t *px, rnd_coord_t *py, rnd_coord_t x, rnd_coord_t y, rnd_coord_t dx, rnd_coord_t dy)
{
	rnd_coord_t ix, iy;

	pcb_lines_intersect_at(ppx, ppy, *px, *py,   (*px) + dx, (*py) + dy, x, y,  &ix, &iy);

	*px = ix;
	*py = iy;
}


/*** draw the attached object while in tool mode move or copy ***/

/* Some complex objects (padstacks, text, polygons) need to cache their
   clearance outline because it's too expensive to calculate on every
   screen update */
typedef struct {
	rnd_pline_t *pline;
	rnd_polyarea_t *pa;
} xordraw_cache_t;

static xordraw_cache_t xordraw_cache;

RND_INLINE void xordraw_movecopy_pstk(rnd_bool modifier, int *event_sent, rnd_coord_t dx, rnd_coord_t dy)
{
	pcb_pstk_t *ps = (pcb_pstk_t *)pcb_crosshair.AttachedObject.Ptr2;
	thindraw_moved_ps(ps, dx, dy);

	if (conf_core.editor.show_drc) {
		if (xordraw_cache.pa == NULL) {
			static pcb_poly_t in_poly = {0};
			rnd_layergrp_id_t gid;

			/* build an union of all-layer clearances since a padstack affects multiple layers */
			for(gid = 0; gid < PCB->LayerGroups.len; gid++) {
				pcb_layergrp_t *g = &PCB->LayerGroups.grp[gid];
				rnd_polyarea_t *cla, *tmp;
				if (!(g->ltype & PCB_LYT_COPPER) || (g->len < 1))  continue;
				cla = pcb_thermal_area_pstk(PCB, ps, g->lid[0], &in_poly);
				if (xordraw_cache.pa != NULL) {
					rnd_polyarea_boolean_free(xordraw_cache.pa, cla, &tmp, RND_PBO_UNITE);
					xordraw_cache.pa = tmp;
				}
				else
					xordraw_cache.pa = cla;
			}
		}
		if (xordraw_cache.pa != NULL) {
			rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.drc);
			pcb_xordraw_polyarea(xordraw_cache.pa, dx, dy);
			rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.attached);
		}
	}
}

RND_INLINE void xordraw_movecopy_line(rnd_bool modifier, int *event_sent, rnd_coord_t dx, rnd_coord_t dy)
{
	/* We move a local copy of the line - the real line hasn't moved, only the preview. */
	int constrained = 0;
	pcb_line_t line;
	rnd_coord_t dx1 = dx, dx2 = dx;
	rnd_coord_t dy1 = dy, dy2 = dy;
	
	memcpy(&line, (pcb_line_t *)pcb_crosshair.AttachedObject.Ptr2, sizeof(line));

	if(conf_core.editor.rubber_band_keep_midlinedir)
		rnd_event(&PCB->hidlib, PCB_EVENT_RUBBER_CONSTRAIN_MAIN_LINE, "pppppp", &line, &constrained, &dx1, &dy1, &dx2, &dy2);
	rnd_event(&PCB->hidlib, PCB_EVENT_RUBBER_MOVE_DRAW, "icccc", constrained, dx1, dy1, dx2, dy2);

	*event_sent = 1;

	line.Point1.X += dx1;
	line.Point1.Y += dy1;
	line.Point2.X += dx2;
	line.Point2.Y += dy2;

	pcb_draw_wireframe_line(pcb_crosshair.GC,
		line.Point1.X, line.Point1.Y, line.Point2.X, line.Point2.Y,
		line.Thickness, 0);

	/* Draw the DRC outline if it is enabled */
	if (conf_core.editor.show_drc) {
		rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.drc);
		pcb_draw_wireframe_line(pcb_crosshair.GC,
			line.Point1.X, line.Point1.Y, line.Point2.X, line.Point2.Y,
			line.Thickness + 2 * (line.Clearance/2 + 1), 0);
		rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.attached);
	}
}

RND_INLINE void xordraw_movecopy_arc(rnd_bool modifier, int *event_sent, rnd_coord_t dx, rnd_coord_t dy)
{
	/* Make a temporary arc and move it by dx,dy */
	pcb_arc_t arc = *((pcb_arc_t *)pcb_crosshair.AttachedObject.Ptr2);
	rnd_event(&PCB->hidlib, PCB_EVENT_RUBBER_MOVE_DRAW, "icccc", 0, dx, dy, dx, dy);
	*event_sent = 1;
	
	arc.X += dx;
	arc.Y += dy;

	pcb_draw_wireframe_arc(pcb_crosshair.GC, &arc, arc.Thickness);

	/* Draw the DRC outline if it is enabled */
	if (conf_core.editor.show_drc) {
		rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.drc);
		arc.Thickness += 2 * (arc.Clearance/2 + 1);
		pcb_draw_wireframe_arc(pcb_crosshair.GC, &arc, arc.Thickness);
		rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.attached);
	}
}


RND_INLINE void xordraw_movecopy_poly(rnd_bool modifier, int *event_sent, rnd_coord_t dx, rnd_coord_t dy)
{
	pcb_poly_t *polygon = (pcb_poly_t *)pcb_crosshair.AttachedObject.Ptr2;

	/* the tmp polygon has n+1 points because the first and the last one are set to the same coordinates */
	pcb_xordraw_poly(polygon, dx, dy, 0);

	/* if show_drc is enabled and polygon is trying to clear other polygons,
	   calculate the offset poly and cache */
	if ((conf_core.editor.show_drc) && (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLYPOLY, polygon))) {
		if (xordraw_cache.pline == NULL) {
			long n;
			rnd_vector_t v;
			rnd_pline_t *pl;

			for(n = 0; n < polygon->PointN; n++) {
				v[0] = polygon->Points[n].X; v[1] = polygon->Points[n].Y;
				if (n == 0)
					pl = rnd_poly_contour_new(v);
				else
					rnd_poly_vertex_include(pl->head->prev, rnd_poly_node_create(v));
			}
			rnd_poly_contour_pre(pl, 1);
			if (pl->Flags.orient)
				rnd_poly_contour_inv(pl);

			xordraw_cache.pline = rnd_pline_dup_offset(pl, -(polygon->Clearance/2+1));
			rnd_poly_contour_del(&pl);
		}
		
		/* draw the clearance offset poly */
		if (xordraw_cache.pline != NULL) {
			rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.drc);
			pcb_xordraw_pline(xordraw_cache.pline, dx, dy);
			rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.attached);
		}
	}
}

RND_INLINE void xordraw_movecopy_text(rnd_bool modifier, int *event_sent, rnd_coord_t dx, rnd_coord_t dy)
{
	int want_box = 1;
	pcb_text_t *text = (pcb_text_t *)pcb_crosshair.AttachedObject.Ptr2;
	if (conf_core.editor.show_drc) {
		if (xordraw_cache.pa == NULL)
			xordraw_cache.pa = pcb_poly_construct_text_clearance(text);
		if (xordraw_cache.pa != NULL) {
			rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.drc);
			pcb_xordraw_polyarea(xordraw_cache.pa, dx, dy);
			rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.attached);
			want_box = 0;
		}
	}
	pcb_text_draw_xor(text, dx, dy, want_box);
}

RND_INLINE void xordraw_movecopy_gfx(rnd_bool modifier, int *event_sent, rnd_coord_t dx, rnd_coord_t dy)
{
	pcb_gfx_t *gfx = (pcb_gfx_t *)pcb_crosshair.AttachedObject.Ptr2;
	pcb_gfx_draw_xor(gfx, dx, dy);
}

RND_INLINE void xordraw_movecopy_line_point(rnd_bool modifier, int *event_sent, rnd_coord_t dx, rnd_coord_t dy)
{
	pcb_line_t *line;
	rnd_point_t *point, *point1, point2;

	line = (pcb_line_t *)pcb_crosshair.AttachedObject.Ptr2;
	point = (rnd_point_t *)pcb_crosshair.AttachedObject.Ptr3;
	point1 = (point == &line->Point1 ? &line->Point2 : &line->Point1);
	point2 = *point;
	point2.X += dx;
	point2.Y += dy;

	if(conf_core.editor.move_linepoint_uses_route == 0) { /* config setting for selecting new 45/90 method */
		pcb_draw_wireframe_line(pcb_crosshair.GC,point1->X, point1->Y, point2.X, point2.Y, line->Thickness, 0);

		/* Draw the DRC outline if it is enabled */
		if (conf_core.editor.show_drc) {
			rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.drc);
			pcb_draw_wireframe_line(pcb_crosshair.GC,point1->X, point1->Y, point2.X, point2.Y,line->Thickness + line->Clearance, 0);
			rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.attached);
		}
	}
	else {
		pcb_route_t route;
		pcb_route_init(&route);
		pcb_route_calculate(PCB,
			&route, point1, &point2, pcb_layer_id(PCB->Data,(pcb_layer_t *)pcb_crosshair.AttachedObject.Ptr1),
			line->Thickness, line->Clearance, line->Flags,
			rnd_gui->shift_is_pressed(rnd_gui), rnd_gui->control_is_pressed(rnd_gui));
		pcb_route_draw(&route,pcb_crosshair.GC);
		if (conf_core.editor.show_drc) 
			pcb_route_draw_drc(&route,pcb_crosshair.GC);
		pcb_route_destroy(&route);
	}
}

RND_INLINE void xordraw_movecopy_arc_point(rnd_bool modifier, int *event_sent, rnd_coord_t dx, rnd_coord_t dy)
{
	pcb_arc_t arc = *((pcb_arc_t *)pcb_crosshair.AttachedObject.Ptr2);
	rnd_coord_t ox1, ox2, oy1, oy2;
	rnd_coord_t nx1, nx2, ny1, ny2;

	/* Get the initial position of the arc point */
	pcb_arc_get_end(&arc,0, &ox1, &oy1);
	pcb_arc_get_end(&arc,1, &ox2, &oy2);

	/* Update the attached arc point */
	pcb_arc_ui_move_or_copy(&pcb_crosshair);

	/* Update our local arc copy from the attached object */
	if(pcb_crosshair.AttachedObject.radius != 0) {
		arc.Width   = pcb_crosshair.AttachedObject.radius;
		arc.Height  = pcb_crosshair.AttachedObject.radius;
	}
	else {
		arc.StartAngle  = pcb_crosshair.AttachedObject.start_angle;
		arc.Delta       = pcb_crosshair.AttachedObject.delta_angle;
	}

	pcb_draw_wireframe_arc(pcb_crosshair.GC, &arc, arc.Thickness);

	/* Draw the DRC outline if it is enabled */
	if (conf_core.editor.show_drc) {
		rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.drc);
		arc.Thickness += arc.Clearance;
		pcb_draw_wireframe_arc(pcb_crosshair.GC, &arc, arc.Thickness);
		rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.attached);
	}

	/* Get the new arc point positions, calculate the movement deltas and send them 
	   in a rubber_move_draw event */
	pcb_arc_get_end(&arc,0, &nx1, &ny1);
	pcb_arc_get_end(&arc,1, &nx2, &ny2);

	rnd_event(&PCB->hidlib, PCB_EVENT_RUBBER_MOVE_DRAW, "icccc", 0, nx1-ox1, ny1-oy1, nx2-ox2, ny2-oy2);
	*event_sent = 1;
}

RND_INLINE void xordraw_movecopy_poly_point(rnd_bool modifier, int *event_sent, rnd_coord_t dx, rnd_coord_t dy)
{
	pcb_poly_t *polygon;
	rnd_point_t *point;
	rnd_cardinal_t point_idx, prev, next, prev2, next2;
	rnd_coord_t px, py, x, y, nx, ny;

	polygon = (pcb_poly_t *) pcb_crosshair.AttachedObject.Ptr2;
	point = (rnd_point_t *) pcb_crosshair.AttachedObject.Ptr3;
	point_idx = pcb_poly_point_idx(polygon, point);

	/* get previous and following point */
	prev = pcb_poly_contour_prev_point(polygon, point_idx);
	next = pcb_poly_contour_next_point(polygon, point_idx);

	px = polygon->Points[prev].X; py = polygon->Points[prev].Y;
	x = point->X + dx; y = point->Y + dy;
	nx = polygon->Points[next].X; ny = polygon->Points[next].Y;

	/* modified version: angle keeper; run only on every even call to not mess up the xor */
	if (modifier && pcb_crosshair.edit_poly_point_extra.last_active) {
		rnd_coord_t ppx, ppy, nnx, nny;

		pcb_crosshair.edit_poly_point_extra.active = 1;
		pcb_crosshair.edit_poly_point_extra.point[0] = &polygon->Points[prev];
		pcb_crosshair.edit_poly_point_extra.point[1] = &polygon->Points[next];

		prev2 = pcb_poly_contour_prev_point(polygon, prev);
		next2 = pcb_poly_contour_next_point(polygon, next);
		ppx = polygon->Points[prev2].X; ppy = polygon->Points[prev2].Y;
		nnx = polygon->Points[next2].X; nny = polygon->Points[next2].Y;

		perp_extend(ppx, ppy, &px, &py, x, y, dx, dy);
		perp_extend(nnx, nny, &nx, &ny, x, y, dx, dy);

		/* draw the extra two segments on prev-prev and next-next */
		rnd_render->draw_line(pcb_crosshair.GC, ppx, ppy, px, py);
		rnd_render->draw_line(pcb_crosshair.GC, nnx, nny, nx, ny);

		pcb_crosshair.edit_poly_point_extra.dx[0] = px - polygon->Points[prev].X;
		pcb_crosshair.edit_poly_point_extra.dy[0] = py - polygon->Points[prev].Y;
		pcb_crosshair.edit_poly_point_extra.dx[1] = nx - polygon->Points[next].X;
		pcb_crosshair.edit_poly_point_extra.dy[1] = ny - polygon->Points[next].Y;
	}
	else
		pcb_crosshair.edit_poly_point_extra.active = 0;

	pcb_crosshair.edit_poly_point_extra.last_active = modifier;

	/* draw the two segments */
	rnd_render->draw_line(pcb_crosshair.GC, px, py, x, y);
	rnd_render->draw_line(pcb_crosshair.GC, x, y, nx, ny);
}

RND_INLINE void xordraw_movecopy_gfx_point(rnd_bool modifier, int *event_sent, rnd_coord_t dx, rnd_coord_t dy)
{
	pcb_gfx_t *gfx;
	rnd_point_t *point, *prev, *next, *opp;
	int point_idx, i;
	double px, py, nx, ny, cx, cy;

	gfx = (pcb_gfx_t *)pcb_crosshair.AttachedObject.Ptr2;
	point = (rnd_point_t *)pcb_crosshair.AttachedObject.Ptr3;
	point_idx = point - gfx->corner;
	if ((point_idx < 0) || (point_idx > 3))
		return;

	cx = pcb_crosshair.X;
	cy = pcb_crosshair.Y;

	i = point_idx - 1;
	if (i < 0) i = 3;
	prev = gfx->corner + i;

	i = (point_idx + 1) % 4;
	next = gfx->corner + i;

	i = (point_idx + 2) % 4;
	opp = gfx->corner + i;

	proj_extend(&px, &py, point->X - prev->X, point->Y - prev->Y, dx, dy);
	proj_extend(&nx, &ny, point->X - next->X, point->Y - next->Y, dx, dy);

	rnd_render->draw_line(pcb_crosshair.GC, opp->X, opp->Y, prev->X+px, prev->Y+py);
	rnd_render->draw_line(pcb_crosshair.GC, opp->X, opp->Y, next->X+nx, next->Y+ny);
	rnd_render->draw_line(pcb_crosshair.GC, cx, cy, prev->X+px, prev->Y+py);
	rnd_render->draw_line(pcb_crosshair.GC, cx, cy, next->X+nx, next->Y+ny);
}

void pcb_xordraw_movecopy(rnd_bool modifier)
{
	rnd_coord_t dx = pcb_crosshair.AttachedObject.tx - pcb_crosshair.AttachedObject.X;
	rnd_coord_t dy = pcb_crosshair.AttachedObject.ty - pcb_crosshair.AttachedObject.Y;
	int event_sent = 0;
	
	switch (pcb_crosshair.AttachedObject.Type) {
		case PCB_OBJ_PSTK:        xordraw_movecopy_pstk(modifier, &event_sent, dx, dy); break;
		case PCB_OBJ_LINE:        xordraw_movecopy_line(modifier, &event_sent, dx, dy); break;
		case PCB_OBJ_ARC:         xordraw_movecopy_arc(modifier, &event_sent, dx, dy); break;
		case PCB_OBJ_POLY:        xordraw_movecopy_poly(modifier, &event_sent, dx, dy); break;
		case PCB_OBJ_TEXT:        xordraw_movecopy_text(modifier, &event_sent, dx, dy); break;
		case PCB_OBJ_GFX:         xordraw_movecopy_gfx(modifier, &event_sent, dx, dy); break;
		case PCB_OBJ_LINE_POINT:  xordraw_movecopy_line_point(modifier, &event_sent, dx, dy); break;
		case PCB_OBJ_ARC_POINT:   xordraw_movecopy_arc_point(modifier, &event_sent, dx, dy); break;
		case PCB_OBJ_POLY_POINT:  xordraw_movecopy_poly_point(modifier, &event_sent, dx, dy); break;
		case PCB_OBJ_GFX_POINT:   xordraw_movecopy_gfx_point(modifier, &event_sent, dx, dy); break;
		case PCB_OBJ_SUBC:        pcb_xordraw_subc((pcb_subc_t *) pcb_crosshair.AttachedObject.Ptr2, dx, dy, 0); break;
	}

	/* floaters have a link back to their parent subc */
	if ((pcb_crosshair.AttachedObject.Ptr2 != NULL) && PCB_FLAG_TEST(PCB_FLAG_FLOATER, (pcb_any_obj_t *)pcb_crosshair.AttachedObject.Ptr2)) {
		pcb_any_obj_t *obj = pcb_crosshair.AttachedObject.Ptr2;
		if (obj->parent_type == PCB_PARENT_LAYER) {
			pcb_data_t *data = obj->parent.layer->parent.data;
			if ((data != NULL) && (data->parent_type == PCB_PARENT_SUBC)) {
				pcb_subc_t *sc = data->parent.subc;
				rnd_coord_t ox, oy;
				if (pcb_subc_get_origin(sc, &ox, &oy) == 0)
					rnd_render->draw_line(pcb_crosshair.GC, ox, oy, pcb_crosshair.X, pcb_crosshair.Y);
			}
		}
	}

	if(!event_sent)
		rnd_event(&PCB->hidlib, PCB_EVENT_RUBBER_MOVE_DRAW, "icc", 0, dx, dy);
}

void pcb_crosshair_attached_clean(rnd_hidlib_t *hidlib)
{
	if (xordraw_cache.pline != NULL)
		rnd_poly_contour_del(&xordraw_cache.pline);

	if (xordraw_cache.pa != NULL)
		rnd_polyarea_free(&xordraw_cache.pa);
}

void rnd_draw_attached(rnd_hidlib_t *hidlib, rnd_bool inhibit_drawing_mode)
{
	if (!inhibit_drawing_mode) {
		rnd_render->set_drawing_mode(rnd_gui, RND_HID_COMP_RESET, 1, NULL);
		rnd_render->set_drawing_mode(rnd_gui, RND_HID_COMP_POSITIVE_XOR, 1, NULL);
	}

	rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.attached);
	rnd_tool_draw_attached(hidlib);

	/* an attached box does not depend on a special mode */
	if (pcb_crosshair.AttachedBox.State == PCB_CH_STATE_SECOND || pcb_crosshair.AttachedBox.State == PCB_CH_STATE_THIRD) {
		rnd_coord_t x1, y1, x2, y2;

		x1 = pcb_crosshair.AttachedBox.Point1.X;
		y1 = pcb_crosshair.AttachedBox.Point1.Y;
		x2 = pcb_crosshair.AttachedBox.Point2.X;
		y2 = pcb_crosshair.AttachedBox.Point2.Y;
		rnd_render->draw_rect(pcb_crosshair.GC, x1, y1, x2, y2);
	}

	if (!inhibit_drawing_mode)
		rnd_render->set_drawing_mode(rnd_gui, RND_HID_COMP_FLUSH, 1, NULL);
}

void rnd_draw_marks(rnd_hidlib_t *hidlib, rnd_bool inhibit_drawing_mode)
{
	rnd_coord_t ms = conf_core.appearance.mark_size, ms2 = ms / 2;

	if ((pcb_marked.status) || (hidlib->tool_grabbed.status)) {
		if (!inhibit_drawing_mode) {
			rnd_render->set_drawing_mode(rnd_gui, RND_HID_COMP_RESET, 1, NULL);
			rnd_render->set_drawing_mode(rnd_gui, RND_HID_COMP_POSITIVE, 1, NULL);
		}
		rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.mark);
	}

	if (pcb_marked.status) {
		rnd_render->draw_line(pcb_crosshair.GC, pcb_marked.X - ms, pcb_marked.Y - ms, pcb_marked.X + ms, pcb_marked.Y + ms);
		rnd_render->draw_line(pcb_crosshair.GC, pcb_marked.X + ms, pcb_marked.Y - ms, pcb_marked.X - ms, pcb_marked.Y + ms);
	}

	if (hidlib->tool_grabbed.status) {
		rnd_render->draw_line(pcb_crosshair.GC, hidlib->tool_grabbed.X - ms, hidlib->tool_grabbed.Y - ms2, hidlib->tool_grabbed.X + ms, hidlib->tool_grabbed.Y + ms2);
		rnd_render->draw_line(pcb_crosshair.GC, hidlib->tool_grabbed.X + ms2, hidlib->tool_grabbed.Y - ms, hidlib->tool_grabbed.X - ms2, hidlib->tool_grabbed.Y + ms);
	}

	if ((pcb_marked.status) || (hidlib->tool_grabbed.status))
		if (!inhibit_drawing_mode)
			rnd_render->set_drawing_mode(rnd_gui, RND_HID_COMP_FLUSH, 1, NULL);
}


/* ---------------------------------------------------------------------------
 * notify the GUI that data relating to the mark is being changed.
 *
 * The argument passed is rnd_false to notify "changes are about to happen",
 * and rnd_true to notify "changes have finished".
 *
 * Each call with a 'rnd_false' parameter must be matched with a following one
 * with a 'rnd_true' parameter. Unmatched 'rnd_true' calls are currently not permitted,
 * but might be allowed in the future.
 *
 * GUIs should not complain if they receive extra calls with 'rnd_true' as parameter.
 * They should initiate a redraw of the mark - which may (if necessary) mean
 * repainting the whole screen if the GUI hasn't tracked the mark's location.
 */
void pcb_notify_mark_change(rnd_bool changes_complete)
{
	if (rnd_gui->notify_mark_change)
		rnd_gui->notify_mark_change(rnd_gui, changes_complete);
}

static double square(double x)
{
	return x * x;
}

static double crosshair_sq_dist(pcb_crosshair_t * crosshair, rnd_coord_t x, rnd_coord_t y)
{
	return square(x - crosshair->X) + square(y - crosshair->Y);
}

struct snap_data {
	pcb_crosshair_t *crosshair;
	double nearest_sq_dist, max_sq_dist;
	rnd_bool nearest_is_grid;
	rnd_coord_t x, y;
};

/* Snap to a given location if it is the closest thing we found so far.
 * If "prefer_to_grid" is set, the passed location will take preference
 * over a closer grid points we already snapped to UNLESS the user is
 * pressing the SHIFT key. If the SHIFT key is pressed, the closest object
 * (including grid points), is always preferred.
 */
static void check_snap_object(struct snap_data *snap_data, rnd_coord_t x, rnd_coord_t y, rnd_bool prefer_to_grid, pcb_any_obj_t *snapo)
{
	double sq_dist;

	/* avoid snapping to an object if it is in the same subc */
	if ((snapo != NULL) && (rnd_conf.editor.mode == pcb_crosshair.tool_move) && (pcb_crosshair.AttachedObject.Type == PCB_OBJ_SUBC)) {
		pcb_any_obj_t *parent = (pcb_any_obj_t *)pcb_obj_parent_subc(snapo);
		int n;
		rnd_cardinal_t parent_id = snapo->ID;
		if (parent != NULL)
			parent_id = parent->ID;
		for(n = 0; n < pcb_crosshair.drags_len; n++) {
			if ((snapo->ID == pcb_crosshair.drags[n]) || (parent_id == pcb_crosshair.drags[n]))
				return;
		}
	}

	sq_dist = crosshair_sq_dist(snap_data->crosshair, x, y);
	if (sq_dist > snap_data->max_sq_dist)
		return;

	if (sq_dist < snap_data->nearest_sq_dist || (prefer_to_grid && snap_data->nearest_is_grid && !rnd_gui->shift_is_pressed(rnd_gui))) {
		snap_data->x = x;
		snap_data->y = y;
		snap_data->nearest_sq_dist = sq_dist;
		snap_data->nearest_is_grid = rnd_false;
	}
}

static rnd_bool should_snap_offgrid_line(pcb_board_t *pcb, pcb_layer_t *layer, pcb_line_t *line)
{
	/* Allow snapping to off-grid lines when drawing new lines (on
	 * the same layer), and when moving a line end-point
	 * (but don't snap to the same line)
	 */
	if ((rnd_conf.editor.mode == pcb_crosshair.tool_line && PCB_CURRLAYER(pcb) == layer) ||
			(rnd_conf.editor.mode == pcb_crosshair.tool_move
			 && pcb_crosshair.AttachedObject.Type == PCB_OBJ_LINE_POINT
			 && pcb_crosshair.AttachedObject.Ptr1 == layer
			 && pcb_crosshair.AttachedObject.Ptr2 != line))
		return rnd_true;
	else
		return rnd_false;
}

static void check_snap_offgrid_line(pcb_board_t *pcb, struct snap_data *snap_data, rnd_coord_t nearest_grid_x, rnd_coord_t nearest_grid_y)
{
	void *ptr1, *ptr2, *ptr3;
	int ans;
	pcb_line_t *line;
	rnd_coord_t try_x, try_y;
	double dx, dy;
	double dist;

	if (!conf_core.editor.snap_pin)
		return;

	/* Code to snap at some sensible point along a line */
	/* Pick the nearest grid-point in the x or y direction
	 * to align with, then adjust until we hit the line
	 */
	ans = pcb_search_grid_slop(pcb_crosshair.X, pcb_crosshair.Y, PCB_OBJ_LINE, &ptr1, &ptr2, &ptr3);

	if (ans == PCB_OBJ_VOID)
		return;

	line = (pcb_line_t *) ptr2;

	if (!should_snap_offgrid_line(pcb, ptr1, line))
		return;

	dx = line->Point2.X - line->Point1.X;
	dy = line->Point2.Y - line->Point1.Y;

	/* Try snapping along the X axis */
	if (dy != 0.) {
		/* Move in the X direction until we hit the line */
		try_x = (nearest_grid_y - line->Point1.Y) / dy * dx + line->Point1.X;
		try_y = nearest_grid_y;
		check_snap_object(snap_data, try_x, try_y, rnd_true, (pcb_any_obj_t *)line);
	}

	/* Try snapping along the Y axis */
	if (dx != 0.) {
		try_x = nearest_grid_x;
		try_y = (nearest_grid_x - line->Point1.X) / dx * dy + line->Point1.Y;
		check_snap_object(snap_data, try_x, try_y, rnd_true, (pcb_any_obj_t *)line);
	}

	if (dx != dy) {								/* If line not parallel with dX = dY direction.. */
		/* Try snapping diagonally towards the line in the dX = dY direction */

		if (dy == 0)
			dist = line->Point1.Y - nearest_grid_y;
		else
			dist = ((line->Point1.X - nearest_grid_x) - (line->Point1.Y - nearest_grid_y) * dx / dy) / (1 - dx / dy);

		try_x = nearest_grid_x + dist;
		try_y = nearest_grid_y + dist;

		check_snap_object(snap_data, try_x, try_y, rnd_true, (pcb_any_obj_t *)line);
	}

	if (dx != -dy) {							/* If line not parallel with dX = -dY direction.. */
		/* Try snapping diagonally towards the line in the dX = -dY direction */

		if (dy == 0)
			dist = nearest_grid_y - line->Point1.Y;
		else
			dist = ((line->Point1.X - nearest_grid_x) - (line->Point1.Y - nearest_grid_y) * dx / dy) / (1 + dx / dy);

		try_x = nearest_grid_x + dist;
		try_y = nearest_grid_y - dist;

		check_snap_object(snap_data, try_x, try_y, rnd_true, (pcb_any_obj_t *)line);
	}
}

/* ---------------------------------------------------------------------------
 * recalculates the passed coordinates to fit the current grid setting
 */
void pcb_crosshair_grid_fit(pcb_board_t *pcb, rnd_coord_t X, rnd_coord_t Y)
{
	rnd_hidlib_t *hidlib = &pcb->hidlib;
	rnd_coord_t nearest_grid_x, nearest_grid_y, oldx, oldy;
	void *ptr1, *ptr2, *ptr3;
	struct snap_data snap_data;
	int ans, newpos, enfmode;

	oldx = pcb_crosshair.X;
	oldy = pcb_crosshair.Y;
	pcb->hidlib.ch_x = pcb_crosshair.X = RND_CLAMP(X, -pcb->hidlib.size_x/2, pcb->hidlib.size_x*3/2);
	pcb->hidlib.ch_y = pcb_crosshair.Y = RND_CLAMP(Y, -pcb->hidlib.size_y/2, pcb->hidlib.size_y*3/2);

	nearest_grid_x = rnd_grid_fit(pcb_crosshair.X, pcb->hidlib.grid, pcb->hidlib.grid_ox);
	nearest_grid_y = rnd_grid_fit(pcb_crosshair.Y, pcb->hidlib.grid, pcb->hidlib.grid_oy);


	if (pcb_marked.status && conf_core.editor.orthogonal_moves) {
		rnd_coord_t dx = pcb_crosshair.X - hidlib->tool_grabbed.X;
		rnd_coord_t dy = pcb_crosshair.Y - hidlib->tool_grabbed.Y;
		if (RND_ABS(dx) > RND_ABS(dy))
			nearest_grid_y = hidlib->tool_grabbed.Y;
		else
			nearest_grid_x = hidlib->tool_grabbed.X;
	}

	snap_data.crosshair = &pcb_crosshair;
	snap_data.nearest_sq_dist = crosshair_sq_dist(&pcb_crosshair, nearest_grid_x, nearest_grid_y);
	snap_data.nearest_is_grid = rnd_true;
	snap_data.x = nearest_grid_x;
	snap_data.y = nearest_grid_y;
	snap_data.max_sq_dist = RND_MAX(rnd_conf.editor.grid * 1.5, rnd_pixel_slop*25);
	snap_data.max_sq_dist = snap_data.max_sq_dist * snap_data.max_sq_dist;

	ans = PCB_OBJ_VOID;
	if (!pcb->RatDraw)
		ans = pcb_search_grid_slop(X, Y, PCB_OBJ_SUBC, &ptr1, &ptr2, &ptr3);

	hidlib->tool_snapped_obj_bbox = NULL;

	if (ans & PCB_OBJ_SUBC) {
		pcb_subc_t *sc = (pcb_subc_t *) ptr1;
		rnd_coord_t ox, oy;
		if (pcb_subc_get_origin(sc, &ox, &oy) == 0) {
			check_snap_object(&snap_data, ox, oy, rnd_true, (pcb_any_obj_t *)sc);
			hidlib->tool_snapped_obj_bbox = &sc->BoundingBox;
		}
	}

	/*** padstack center ***/
	ans = PCB_OBJ_VOID;
	if (conf_core.editor.snap_pin)
		ans = pcb_search_grid_slop(X, Y, PCB_OBJ_PSTK | PCB_OBJ_SUBC_PART, &ptr1, &ptr2, &ptr3);

	/* Avoid snapping padstack to any other padstack */
	if (rnd_conf.editor.mode == pcb_crosshair.tool_move && pcb_crosshair.AttachedObject.Type == PCB_OBJ_PSTK && (ans & PCB_OBJ_PSTK))
		ans = PCB_OBJ_VOID;

	if (ans != PCB_OBJ_VOID) {
		pcb_pstk_t *ps = (pcb_pstk_t *) ptr2;
		check_snap_object(&snap_data, ps->x, ps->y, rnd_true, (pcb_any_obj_t *)ps);
		hidlib->tool_snapped_obj_bbox = &ps->BoundingBox;
	}

	/*** arc ***/
	ans = PCB_OBJ_VOID;
	if (conf_core.editor.snap_pin)
		ans = pcb_search_grid_slop(X, Y, PCB_OBJ_LINE_POINT | PCB_OBJ_ARC_POINT | PCB_OBJ_SUBC_PART, &ptr1, &ptr2, &ptr3);

	if (ans == PCB_OBJ_ARC_POINT) {
		/* Arc point needs special handling as it's not a real point but has to be calculated */
		rnd_coord_t ex, ey;
		pcb_arc_get_end((pcb_arc_t *)ptr2, (ptr3 != pcb_arc_start_ptr), &ex, &ey);
		check_snap_object(&snap_data, ex, ey, rnd_true, (pcb_any_obj_t *)ptr2);
	}
	else if (ans != PCB_OBJ_VOID) {
		rnd_point_t *pnt = (rnd_point_t *) ptr3;
		check_snap_object(&snap_data, pnt->X, pnt->Y, rnd_true, (pcb_any_obj_t *)ptr2);
	}

	/*** polygon terminal: center ***/
	ans = PCB_OBJ_VOID;
	if (conf_core.editor.snap_pin)
		ans = pcb_search_grid_slop(X, Y, PCB_OBJ_POLY | PCB_OBJ_SUBC_PART, &ptr1, &ptr2, &ptr3);

	if (ans == PCB_OBJ_POLY) {
		pcb_poly_t *p = ptr2;
		if (p->term != NULL) {
			rnd_coord_t cx, cy;
			cx = (p->BoundingBox.X1 + p->BoundingBox.X2)/2;
			cy = (p->BoundingBox.Y1 + p->BoundingBox.Y2)/2;
			if (pcb_poly_is_point_in_p(cx, cy, 1, p))
				check_snap_object(&snap_data, cx, cy, rnd_true, (pcb_any_obj_t *)p);
		}
	}

	/*** gfx ***/
	ans = PCB_OBJ_VOID;
	ans = pcb_search_grid_slop(X, Y, PCB_OBJ_GFX | PCB_OBJ_SUBC_PART, &ptr1, &ptr2, &ptr3);
	if (ans == PCB_OBJ_VOID) {
		ans = pcb_search_grid_slop(X, Y, PCB_OBJ_GFX_POINT | PCB_OBJ_SUBC_PART, &ptr1, &ptr2, &ptr3);
		if (ans != PCB_OBJ_VOID) {
			rnd_point_t *pt = ptr3;
			check_snap_object(&snap_data, pt->X, pt->Y, rnd_true, (pcb_any_obj_t *)NULL);
		}
	}

	/* Snap to offgrid points on lines. */
	if (conf_core.editor.snap_offgrid_line)
		check_snap_offgrid_line(pcb, &snap_data, nearest_grid_x, nearest_grid_y);

	ans = PCB_OBJ_VOID;
	if (conf_core.editor.snap_pin)
		ans = pcb_search_grid_slop(X, Y, PCB_OBJ_POLY_POINT, &ptr1, &ptr2, &ptr3);

	if (ans != PCB_OBJ_VOID) {
		rnd_point_t *pnt = (rnd_point_t *) ptr3;
		check_snap_object(&snap_data, pnt->X, pnt->Y, rnd_true, NULL);
	}

	newpos = (snap_data.x != oldx) || (snap_data.y != oldy);

	pcb->hidlib.ch_x = pcb_crosshair.X = snap_data.x;
	pcb->hidlib.ch_y = pcb_crosshair.Y = snap_data.y;

	if (newpos)
		rnd_event(hidlib, PCB_EVENT_CROSSHAIR_NEW_POS, "pcc", &pcb_crosshair, oldx, oldy);

	if (rnd_conf.editor.mode == pcb_crosshair.tool_arrow) {
		ans = pcb_search_grid_slop(X, Y, PCB_OBJ_LINE_POINT, &ptr1, &ptr2, &ptr3);
		if (ans == PCB_OBJ_VOID) {
			if ((rnd_gui != NULL) && (rnd_gui->point_cursor != NULL))
				rnd_gui->point_cursor(rnd_gui, rnd_false);
		}
		else if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, (pcb_line_t *) ptr2)) {
			if ((rnd_gui != NULL) && (rnd_gui->point_cursor != NULL))
				rnd_gui->point_cursor(rnd_gui, rnd_true);
		}
	}

	enfmode  = (rnd_conf.editor.mode == pcb_crosshair.tool_line) && pcb_crosshair.AttachedLine.State != PCB_CH_STATE_FIRST;
	if (pcb_brave & PCB_BRAVE_ENFORCE_CLR_MOVE)
		enfmode |= (rnd_conf.editor.mode == pcb_crosshair.tool_move) && (pcb_crosshair.AttachedObject.Type == PCB_OBJ_LINE_POINT);
	if (enfmode && conf_core.editor.auto_drc) {
		rnd_coord_t thick;
		
		if (rnd_conf.editor.mode == pcb_crosshair.tool_move)
			thick = pcb_crosshair.AttachedLine.tot_thick;
		else
			thick = conf_core.design.line_thickness + 2 * (conf_core.design.clearance + 1);

		pcb_line_enforce_drc(pcb, thick);
	}

	rnd_gui->set_crosshair(rnd_gui, pcb_crosshair.X, pcb_crosshair.Y, HID_SC_DO_NOTHING);
}

/* ---------------------------------------------------------------------------
 * move crosshair relative (has to be switched off)
 */
void pcb_crosshair_move_relative(rnd_coord_t DeltaX, rnd_coord_t DeltaY)
{
	pcb_crosshair_grid_fit(PCB, pcb_crosshair.X + DeltaX, pcb_crosshair.Y + DeltaY);
}

/* ---------------------------------------------------------------------------
 * move crosshair absolute
 * return rnd_true if the crosshair was moved from its existing position
 */
rnd_bool pcb_crosshair_move_absolute(pcb_board_t *pcb, rnd_coord_t X, rnd_coord_t Y)
{
	rnd_coord_t x, y, z;
	pcb_crosshair.ptr_x = X;
	pcb_crosshair.ptr_y = Y;
	x = pcb_crosshair.X;
	y = pcb_crosshair.Y;
	pcb_crosshair_grid_fit(pcb, X, Y);
	if (pcb_crosshair.X != x || pcb_crosshair.Y != y) {
		/* back up to old position to notify the GUI
		 * (which might want to erase the old crosshair) */
		z = pcb_crosshair.X;
		pcb->hidlib.ch_x = pcb_crosshair.X = x;
		x = z;
		z = pcb_crosshair.Y;
		pcb->hidlib.ch_y = pcb_crosshair.Y = y;
		rnd_hid_notify_crosshair_change(&pcb->hidlib, rnd_false); /* Our caller notifies when it has done */
		/* now move forward again */
		pcb->hidlib.ch_x = pcb_crosshair.X = x;
		pcb->hidlib.ch_y = pcb_crosshair.Y = z;
		return rnd_true;
	}
	return rnd_false;
}

/* ---------------------------------------------------------------------------
 * centers the displayed PCB around the specified point (X,Y)
 */
void pcb_center_display(pcb_board_t *pcb, rnd_coord_t X, rnd_coord_t Y)
{
	rnd_coord_t save_grid = pcb->hidlib.grid;
	pcb->hidlib.grid = 1;
	if (pcb_crosshair_move_absolute(pcb, X, Y))
		rnd_hid_notify_crosshair_change(&pcb->hidlib, rnd_true);
	rnd_gui->set_crosshair(rnd_gui, pcb_crosshair.X, pcb_crosshair.Y, HID_SC_WARP_POINTER);
	pcb->hidlib.grid = save_grid;
}

/* allocate GC only when the GUI is already up and running */
static void pcb_crosshair_gui_init(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pcb_crosshair.GC = rnd_hid_make_gc();

	rnd_render->set_color(pcb_crosshair.GC, &rnd_conf.appearance.color.cross);
	rnd_hid_set_draw_xor(pcb_crosshair.GC, 1);
	rnd_hid_set_line_cap(pcb_crosshair.GC, rnd_cap_round);
	rnd_hid_set_line_width(pcb_crosshair.GC, 1);
}

/* ---------------------------------------------------------------------------
 * initializes crosshair stuff
 * clears the struct
 */
void pcb_crosshair_init(void)
{
	/* clear the mark */
	pcb_marked.status = rnd_false;

	/* Initialise Line Route */
	pcb_route_init(&pcb_crosshair.Route);


	rnd_event_bind(RND_EVENT_GUI_INIT, pcb_crosshair_gui_init, NULL, crosshair_cookie);
}

void pcb_crosshair_pre_init(void)
{
	pcb_crosshair.tool_arrow = pcb_crosshair.tool_move = pcb_crosshair.tool_line = -1;
	pcb_crosshair.tool_arc = pcb_crosshair.tool_poly = pcb_crosshair.tool_poly_hole = -1;
}

void pcb_crosshair_uninit(void)
{
	pcb_poly_free_fields(&pcb_crosshair.AttachedPolygon);
	pcb_route_destroy(&pcb_crosshair.Route);
	if (rnd_render != NULL)
		rnd_hid_destroy_gc(pcb_crosshair.GC);
	rnd_event_unbind_allcookie(crosshair_cookie);
}

void pcb_crosshair_set_local_ref(rnd_coord_t X, rnd_coord_t Y, rnd_bool Showing)
{
	static rnd_mark_t old;
	static int count = 0;

	if (Showing) {
		pcb_notify_mark_change(rnd_false);
		if (count == 0)
			old = pcb_marked;
		pcb_marked.X = X;
		pcb_marked.Y = Y;
		pcb_marked.status = rnd_true;
		count++;
		pcb_notify_mark_change(rnd_true);
	}
	else if (count > 0) {
		pcb_notify_mark_change(rnd_false);
		count = 0;
		pcb_marked = old;
		pcb_notify_mark_change(rnd_true);
	}
}

static void pcb_event_move_crosshair(rnd_coord_t ev_x, rnd_coord_t ev_y)
{
	if (pcb_crosshair_move_absolute(PCB, ev_x, ev_y)) {
		/* update object position and cursor location */
		rnd_tool_adjust_attached(&PCB->hidlib);
		rnd_event(&PCB->hidlib, PCB_EVENT_DRAW_CROSSHAIR_CHATT, NULL);
		rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_true);
	}
}


typedef struct {
	int obj, line, box;
} old_crosshair_t;

void *rnd_hidlib_crosshair_suspend(rnd_hidlib_t *hl)
{
	old_crosshair_t *buf = malloc(sizeof(old_crosshair_t));

	buf->obj = pcb_crosshair.AttachedObject.State;
	buf->line = pcb_crosshair.AttachedLine.State;
	buf->box = pcb_crosshair.AttachedBox.State;
	rnd_hid_notify_crosshair_change(hl, rnd_false);
	pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
	pcb_crosshair.AttachedLine.State = PCB_CH_STATE_FIRST;
	pcb_crosshair.AttachedBox.State = PCB_CH_STATE_FIRST;
	rnd_hid_notify_crosshair_change(hl, rnd_true);
	return buf;
}

void rnd_hidlib_crosshair_restore(rnd_hidlib_t *hl, void *susp_data)
{
	old_crosshair_t *buf = susp_data;

	rnd_hid_notify_crosshair_change(hl, rnd_false);
	pcb_crosshair.AttachedObject.State = buf->obj;
	pcb_crosshair.AttachedLine.State = buf->line;
	pcb_crosshair.AttachedBox.State = buf->box;
	rnd_hid_notify_crosshair_change(hl, rnd_true);

	free(buf);
}


void rnd_hidlib_crosshair_move_to(rnd_hidlib_t *hl, rnd_coord_t abs_x, rnd_coord_t abs_y, int mouse_mot)
{
	if (!mouse_mot) {
		rnd_hid_notify_crosshair_change(hl, rnd_false);
		if (pcb_crosshair_move_absolute((pcb_board_t *)hl, abs_x, abs_y))
			rnd_hid_notify_crosshair_change(hl, rnd_true);
		rnd_hid_notify_crosshair_change(hl, rnd_true);
	}
	else
		pcb_event_move_crosshair(abs_x, abs_y);
}

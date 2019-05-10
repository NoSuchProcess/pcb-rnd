/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  15 Oct 2008 Ineiev: add different crosshair shapes
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
#include "hidlib_conf.h"

#include "board.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "draw_wireframe.h"
#include "search.h"
#include "polygon.h"
#include "actions.h"
#include "hid_inlines.h"
#include "compat_misc.h"
#include "vtonpoint.h"
#include "find.h"
#include "undo.h"
#include "event.h"
#include "macro.h"
#include "grid.h"
#include "stub_stroke.h"

#include "obj_line_draw.h"
#include "obj_arc_draw.h"
#include "obj_text_draw.h"
#include "obj_pstk_draw.h"
#include "route_draw.h"
#include "obj_arc_ui.h"
#include "obj_subc_parent.h"

#include "tool.h"

typedef struct {
	int x, y;
} point;

pcb_crosshair_t pcb_crosshair;  /* information about cursor settings */
pcb_mark_t pcb_marked;          /* a cross-hair mark */

static void thindraw_moved_ps(pcb_pstk_t *ps, pcb_coord_t x, pcb_coord_t y)
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
void pcb_xordraw_poly(pcb_poly_t *polygon, pcb_coord_t dx, pcb_coord_t dy, int dash_last)
{
	pcb_cardinal_t i;
	for (i = 0; i < polygon->PointN; i++) {
		pcb_cardinal_t next = pcb_poly_contour_next_point(polygon, i);

		if (next == 0) { /* last line: sometimes the implicit closing line */
			if (i == 1) /* corner case: don't draw two lines on top of each other - with XOR it looks bad */
				continue;

			if (dash_last) {
				pcb_draw_dashed_line(NULL, pcb_crosshair.GC,
									 polygon->Points[i].X + dx,
									 polygon->Points[i].Y + dy, polygon->Points[next].X + dx, polygon->Points[next].Y + dy, 5, pcb_false);
				break; /* skip normal line draw below */
			}
		}

		/* normal contour line */
		pcb_gui->draw_line(pcb_crosshair.GC,
								 polygon->Points[i].X + dx,
								 polygon->Points[i].Y + dy, polygon->Points[next].X + dx, polygon->Points[next].Y + dy);
	}
}

/* ---------------------------------------------------------------------------
 * creates a tmp polygon with coordinates converted to screen system, designed
 * for subc paste xor-draw
 */
void pcb_xordraw_poly_subc(pcb_poly_t *polygon, pcb_coord_t dx, pcb_coord_t dy, pcb_coord_t w, pcb_coord_t h, int mirr)
{
	pcb_cardinal_t i;
	for (i = 0; i < polygon->PointN; i++) {
		pcb_cardinal_t next = pcb_poly_contour_next_point(polygon, i);

		if (next == 0) { /* last line: sometimes the implicit closing line */
			if (i == 1) /* corner case: don't draw two lines on top of each other - with XOR it looks bad */
				continue;
		}

		/* normal contour line */
		pcb_gui->draw_line(pcb_crosshair.GC,
			PCB_CSWAP_X(polygon->Points[i].X, w, mirr) + dx,
			PCB_CSWAP_Y(polygon->Points[i].Y, h, mirr) + dy,
			PCB_CSWAP_X(polygon->Points[next].X, w, mirr) + dx,
			PCB_CSWAP_Y(polygon->Points[next].Y, h, mirr) + dy);
	}
}

void pcb_xordraw_attached_arc(pcb_coord_t thick)
{
	pcb_arc_t arc;
	pcb_coord_t wx, wy;
	pcb_angle_t sa, dir;

	wx = pcb_crosshair.X - pcb_crosshair.AttachedBox.Point1.X;
	wy = pcb_crosshair.Y - pcb_crosshair.AttachedBox.Point1.Y;
	if (wx == 0 && wy == 0)
		return;
	arc.X = pcb_crosshair.AttachedBox.Point1.X;
	arc.Y = pcb_crosshair.AttachedBox.Point1.Y;
	if (PCB_XOR(pcb_crosshair.AttachedBox.otherway, coord_abs(wy) > coord_abs(wx))) {
		arc.X = pcb_crosshair.AttachedBox.Point1.X + coord_abs(wy) * PCB_SGNZ(wx);
		sa = (wx >= 0) ? 0 : 180;
		dir = (PCB_SGNZ(wx) == PCB_SGNZ(wy)) ? 90 : -90;
	}
	else {
		arc.Y = pcb_crosshair.AttachedBox.Point1.Y + coord_abs(wx) * PCB_SGNZ(wy);
		sa = (wy >= 0) ? -90 : 90;
		dir = (PCB_SGNZ(wx) == PCB_SGNZ(wy)) ? -90 : 90;
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
	pcb_cardinal_t i;
	pcb_coord_t x, y;

	/* set offset */
	x = pcb_crosshair.AttachedObject.tx - Buffer->X;
	y = pcb_crosshair.AttachedObject.ty - Buffer->Y;

	/* draw all visible layers */
	for (i = 0; i < pcb_max_layer; i++)
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
				pcb_text_draw_xor(text, x, y);
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
	pcb_point_t *point = (pcb_point_t *) pcb_crosshair.AttachedObject.Ptr3;

	if (pcb_crosshair.AttachedObject.Type != PCB_OBJ_VOID) {
		pcb_gui->draw_line(pcb_crosshair.GC, point->X, point->Y, line->Point1.X, line->Point1.Y);
		pcb_gui->draw_line(pcb_crosshair.GC, point->X, point->Y, line->Point2.X, line->Point2.Y);
	}
}

/* ---------------------------------------------------------------------------
 * draws the attached object while in PCB_MODE_MOVE or PCB_MODE_COPY
 */
void pcb_xordraw_movecopy(void)
{
	pcb_coord_t dx = pcb_crosshair.AttachedObject.tx - pcb_crosshair.AttachedObject.X;
	pcb_coord_t dy = pcb_crosshair.AttachedObject.ty - pcb_crosshair.AttachedObject.Y;
	int event_sent = 0;
	
	switch (pcb_crosshair.AttachedObject.Type) {

	case PCB_OBJ_PSTK:
		{
			pcb_pstk_t *ps = (pcb_pstk_t *) pcb_crosshair.AttachedObject.Ptr1;
			thindraw_moved_ps(ps, dx, dy);
			break;
		}

	case PCB_OBJ_LINE:
		{
			/* We move a local copy of the line -the real line hasn't moved, 
			 * only the preview.
			 */			
			int constrained = 0;
			pcb_line_t line;
			
			pcb_coord_t dx1 = dx, dx2 = dx;
			pcb_coord_t dy1 = dy, dy2 = dy;
			
			memcpy(&line, (pcb_line_t *) pcb_crosshair.AttachedObject.Ptr2, sizeof(line));

			if(conf_core.editor.rubber_band_keep_midlinedir)
				pcb_event(&PCB->hidlib, PCB_EVENT_RUBBER_CONSTRAIN_MAIN_LINE, "pppppp", &line, &constrained, &dx1, &dy1, &dx2, &dy2);
			pcb_event(&PCB->hidlib, PCB_EVENT_RUBBER_MOVE_DRAW, "icccc", constrained, dx1, dy1, dx2, dy2);

			event_sent = 1;

			line.Point1.X += dx1;
			line.Point1.Y += dy1;
			line.Point2.X += dx2;
			line.Point2.Y += dy2;

			pcb_draw_wireframe_line(pcb_crosshair.GC,
															line.Point1.X, line.Point1.Y,
															line.Point2.X, line.Point2.Y, 
															line.Thickness, 0);
			
			/* Draw the DRC outline if it is enabled */
			if (conf_core.editor.show_drc) {
				pcb_gui->set_color(pcb_crosshair.GC, &pcbhl_conf.appearance.color.cross);
				pcb_draw_wireframe_line(pcb_crosshair.GC,line.Point1.X, line.Point1.Y,
																line.Point2.X, line.Point2.Y,
																line.Thickness + 2 * (conf_core.design.bloat + 1), 0);
				pcb_gui->set_color(pcb_crosshair.GC, &conf_core.appearance.color.crosshair);
			}
			break;
		}

	case PCB_OBJ_ARC:
		{
			/* Make a temporary arc and move it by dx,dy */
			pcb_arc_t arc = *((pcb_arc_t *) pcb_crosshair.AttachedObject.Ptr2);
			pcb_event(&PCB->hidlib, PCB_EVENT_RUBBER_MOVE_DRAW, "icccc", 0, dx, dy, dx, dy);
			event_sent = 1;
			
			arc.X += dx;
			arc.Y += dy;

			pcb_draw_wireframe_arc(pcb_crosshair.GC, &arc, arc.Thickness);

			/* Draw the DRC outline if it is enabled */
			if (conf_core.editor.show_drc) {
				pcb_gui->set_color(pcb_crosshair.GC, &pcbhl_conf.appearance.color.cross);
				arc.Thickness += 2 * (conf_core.design.bloat + 1);
				pcb_draw_wireframe_arc(pcb_crosshair.GC, &arc, arc.Thickness);
				pcb_gui->set_color(pcb_crosshair.GC, &conf_core.appearance.color.crosshair);
			}
			break;
		}

	case PCB_OBJ_POLY:
		{
			pcb_poly_t *polygon = (pcb_poly_t *) pcb_crosshair.AttachedObject.Ptr2;

			/* the tmp polygon has n+1 points because the first
			 * and the last one are set to the same coordinates
			 */
			pcb_xordraw_poly(polygon, dx, dy, 0);
			break;
		}

	case PCB_OBJ_TEXT:
		{
			pcb_text_t *text = (pcb_text_t *)pcb_crosshair.AttachedObject.Ptr2;
			pcb_text_draw_xor(text, dx, dy);
			break;
		}

	case PCB_OBJ_LINE_POINT:
		{
			pcb_line_t *line;
			pcb_point_t *point,*point1,point2;

			line = (pcb_line_t *) pcb_crosshair.AttachedObject.Ptr2;
			point = (pcb_point_t *) pcb_crosshair.AttachedObject.Ptr3;
			point1 = (point == &line->Point1 ? &line->Point2 : &line->Point1);
			point2 = *point;
			point2.X += dx;
			point2.Y += dy;

			if(conf_core.editor.move_linepoint_uses_route == 0) {/* config setting for selecting new 45/90 method */ 
				pcb_draw_wireframe_line(pcb_crosshair.GC,point1->X, point1->Y, point2.X, point2.Y, line->Thickness, 0);

				/* Draw the DRC outline if it is enabled */
				if (conf_core.editor.show_drc) {
					pcb_gui->set_color(pcb_crosshair.GC, &pcbhl_conf.appearance.color.cross);
					pcb_draw_wireframe_line(pcb_crosshair.GC,point1->X, point1->Y, point2.X, 
																	point2.Y,line->Thickness + 2 * (conf_core.design.bloat + 1), 0);
					pcb_gui->set_color(pcb_crosshair.GC, &conf_core.appearance.color.crosshair);
				}
			}
			else {
				pcb_route_t	route;
				pcb_route_init(&route);
				pcb_route_calculate(	PCB,
															&route,
															point1,
															&point2,
															pcb_layer_id(PCB->Data,(pcb_layer_t *)pcb_crosshair.AttachedObject.Ptr1),
															line->Thickness,
															line->Clearance,
															line->Flags,
															pcb_gui->shift_is_pressed(),
															pcb_gui->control_is_pressed() );
				pcb_route_draw(&route,pcb_crosshair.GC);
				if (conf_core.editor.show_drc) 
					pcb_route_draw_drc(&route,pcb_crosshair.GC);
				pcb_route_destroy(&route);		
			}

			break;
		}

	case PCB_OBJ_ARC_POINT:
		{
			pcb_arc_t arc = *((pcb_arc_t *)pcb_crosshair.AttachedObject.Ptr2);
			pcb_coord_t ox1,ox2,oy1,oy2;
			pcb_coord_t nx1,nx2,ny1,ny2;

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
				pcb_gui->set_color(pcb_crosshair.GC, &pcbhl_conf.appearance.color.cross);
				arc.Thickness += 2 * (conf_core.design.bloat + 1);
				pcb_draw_wireframe_arc(pcb_crosshair.GC, &arc, arc.Thickness);
				pcb_gui->set_color(pcb_crosshair.GC, &conf_core.appearance.color.crosshair);
			}

			/* Get the new arc point positions, calculate the movement deltas and send them 
			 * in a rubber_move_draw event */
			pcb_arc_get_end(&arc,0, &nx1, &ny1);
			pcb_arc_get_end(&arc,1, &nx2, &ny2);

			pcb_event(&PCB->hidlib, PCB_EVENT_RUBBER_MOVE_DRAW, "icccc", 0, nx1-ox1,ny1-oy1,nx2-ox2,ny2-oy2);
			event_sent = 1;
			break;
		}

	case PCB_OBJ_POLY_POINT:
		{
			pcb_poly_t *polygon;
			pcb_point_t *point;
			pcb_cardinal_t point_idx, prev, next;

			polygon = (pcb_poly_t *) pcb_crosshair.AttachedObject.Ptr2;
			point = (pcb_point_t *) pcb_crosshair.AttachedObject.Ptr3;
			point_idx = pcb_poly_point_idx(polygon, point);

			/* get previous and following point */
			prev = pcb_poly_contour_prev_point(polygon, point_idx);
			next = pcb_poly_contour_next_point(polygon, point_idx);

			/* draw the two segments */
			pcb_gui->draw_line(pcb_crosshair.GC, polygon->Points[prev].X, polygon->Points[prev].Y, point->X + dx, point->Y + dy);
			pcb_gui->draw_line(pcb_crosshair.GC, point->X + dx, point->Y + dy, polygon->Points[next].X, polygon->Points[next].Y);
			break;
		}

	case PCB_OBJ_SUBC:
		pcb_xordraw_subc((pcb_subc_t *) pcb_crosshair.AttachedObject.Ptr2, dx, dy, 0);
		break;
	}

	/* floaters have a link back to their parent subc */
	if (pcb_crosshair.AttachedObject.Ptr2 != NULL
			&& PCB_FLAG_TEST(PCB_FLAG_FLOATER, (pcb_any_obj_t *)pcb_crosshair.AttachedObject.Ptr2)) {
		pcb_any_obj_t *obj = pcb_crosshair.AttachedObject.Ptr2;
		if (obj->parent_type == PCB_PARENT_LAYER) {
			pcb_data_t *data = obj->parent.layer->parent.data;
			if ((data != NULL) && (data->parent_type == PCB_PARENT_SUBC)) {
				pcb_subc_t *sc = data->parent.subc;
				pcb_coord_t ox, oy;
				if (pcb_subc_get_origin(sc, &ox, &oy) == 0)
					pcb_gui->draw_line(pcb_crosshair.GC, ox, oy, pcb_crosshair.X, pcb_crosshair.Y);
			}
		}
	}

	if(!event_sent)
		pcb_event(&PCB->hidlib, PCB_EVENT_RUBBER_MOVE_DRAW, "icc", 0, dx, dy );
}

void pcbhl_draw_attached(pcb_hidlib_t *hidlib, pcb_bool inhibit_drawing_mode)
{
	if (!inhibit_drawing_mode) {
		pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, 1, NULL);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE_XOR, 1, NULL);
	}

	pcb_tool_draw_attached();

	/* an attached box does not depend on a special mode */
	if (pcb_crosshair.AttachedBox.State == PCB_CH_STATE_SECOND || pcb_crosshair.AttachedBox.State == PCB_CH_STATE_THIRD) {
		pcb_coord_t x1, y1, x2, y2;

		x1 = pcb_crosshair.AttachedBox.Point1.X;
		y1 = pcb_crosshair.AttachedBox.Point1.Y;
		x2 = pcb_crosshair.AttachedBox.Point2.X;
		y2 = pcb_crosshair.AttachedBox.Point2.Y;
		pcb_gui->draw_rect(pcb_crosshair.GC, x1, y1, x2, y2);
	}

	if (!inhibit_drawing_mode)
		pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, 1, NULL);
}


void pcbhl_draw_marks(pcb_hidlib_t *hidlib, pcb_bool inhibit_drawing_mode)
{
	pcb_coord_t ms = conf_core.appearance.mark_size;

	/* Mark is not drawn when it is not set */
	if (!pcb_marked.status)
		return;

	if (!inhibit_drawing_mode) {
		pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, 1, NULL);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, 1, NULL);
	}

	pcb_gui->draw_line(pcb_crosshair.GC, pcb_marked.X - ms, pcb_marked.Y - ms, pcb_marked.X + ms, pcb_marked.Y + ms);
	pcb_gui->draw_line(pcb_crosshair.GC, pcb_marked.X + ms, pcb_marked.Y - ms, pcb_marked.X - ms, pcb_marked.Y + ms);

	if (!inhibit_drawing_mode)
		pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, 1, NULL);
}

/* ---------------------------------------------------------------------------
 * notify the GUI that data relating to the crosshair is being changed.
 *
 * The argument passed is pcb_false to notify "changes are about to happen",
 * and pcb_true to notify "changes have finished".
 *
 * Each call with a 'pcb_false' parameter must be matched with a following one
 * with a 'pcb_true' parameter. Unmatched 'pcb_true' calls are currently not permitted,
 * but might be allowed in the future.
 *
 * GUIs should not complain if they receive extra calls with 'pcb_true' as parameter.
 * They should initiate a redraw of the crosshair attached objects - which may
 * (if necessary) mean repainting the whole screen if the GUI hasn't tracked the
 * location of existing attached drawing.
 */
void pcb_notify_crosshair_change(pcb_bool changes_complete)
{
	if (pcb_gui->notify_crosshair_change)
		pcb_gui->notify_crosshair_change(&PCB->hidlib, changes_complete);
	pcb_event(&PCB->hidlib, PCB_EVENT_CROSSHAIR_MOVE, "i", (int)changes_complete, NULL);
}


/* ---------------------------------------------------------------------------
 * notify the GUI that data relating to the mark is being changed.
 *
 * The argument passed is pcb_false to notify "changes are about to happen",
 * and pcb_true to notify "changes have finished".
 *
 * Each call with a 'pcb_false' parameter must be matched with a following one
 * with a 'pcb_true' parameter. Unmatched 'pcb_true' calls are currently not permitted,
 * but might be allowed in the future.
 *
 * GUIs should not complain if they receive extra calls with 'pcb_true' as parameter.
 * They should initiate a redraw of the mark - which may (if necessary) mean
 * repainting the whole screen if the GUI hasn't tracked the mark's location.
 */
void pcb_notify_mark_change(pcb_bool changes_complete)
{
	if (pcb_gui->notify_mark_change)
		pcb_gui->notify_mark_change(&PCB->hidlib, changes_complete);
}


/*
 * Below is the implementation of the "highlight on endpoint" functionality.
 * This highlights lines and arcs when the crosshair is on of their (two)
 * endpoints.
 */
struct onpoint_search_info {
	pcb_crosshair_t *crosshair;
	pcb_coord_t X;
	pcb_coord_t Y;
};

static pcb_r_dir_t onpoint_line_callback(const pcb_box_t * box, void *cl)
{
	struct onpoint_search_info *info = (struct onpoint_search_info *) cl;
	pcb_crosshair_t *crosshair = info->crosshair;
	pcb_line_t *line = (pcb_line_t *) box;

#ifdef DEBUG_ONPOINT
	printf("X=%ld Y=%ld    X1=%ld Y1=%ld X2=%ld Y2=%ld\n", info->X, info->Y,
				 line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y);
#endif
	if ((line->Point1.X == info->X && line->Point1.Y == info->Y) || (line->Point2.X == info->X && line->Point2.Y == info->Y)) {
		pcb_onpoint_t op;
		op.type = PCB_OBJ_LINE;
		op.obj.line = line;
		vtop_append(&crosshair->onpoint_objs, op);
		PCB_FLAG_SET(PCB_FLAG_ONPOINT, (pcb_any_obj_t *) line);
		pcb_line_invalidate_draw(NULL, line);
		return PCB_R_DIR_FOUND_CONTINUE;
	}
	else {
		return PCB_R_DIR_NOT_FOUND;
	}
}

#define close_enough(v1, v2) (coord_abs((v1)-(v2)) < 10)

static pcb_r_dir_t onpoint_arc_callback(const pcb_box_t * box, void *cl)
{
	struct onpoint_search_info *info = (struct onpoint_search_info *) cl;
	pcb_crosshair_t *crosshair = info->crosshair;
	pcb_arc_t *arc = (pcb_arc_t *) box;
	pcb_coord_t p1x, p1y, p2x, p2y;

	p1x = arc->X - arc->Width * cos(PCB_TO_RADIANS(arc->StartAngle));
	p1y = arc->Y + arc->Height * sin(PCB_TO_RADIANS(arc->StartAngle));
	p2x = arc->X - arc->Width * cos(PCB_TO_RADIANS(arc->StartAngle + arc->Delta));
	p2y = arc->Y + arc->Height * sin(PCB_TO_RADIANS(arc->StartAngle + arc->Delta));

	/* printf("p1=%ld;%ld p2=%ld;%ld info=%ld;%ld\n", p1x, p1y, p2x, p2y, info->X, info->Y); */

	if ((close_enough(p1x, info->X) && close_enough(p1y, info->Y)) || (close_enough(p2x, info->X) && close_enough(p2y, info->Y))) {
		pcb_onpoint_t op;
		op.type = PCB_OBJ_ARC;
		op.obj.arc = arc;
		vtop_append(&crosshair->onpoint_objs, op);
		PCB_FLAG_SET(PCB_FLAG_ONPOINT, (pcb_any_obj_t *) arc);
		pcb_arc_invalidate_draw(NULL, arc);
		return PCB_R_DIR_FOUND_CONTINUE;
	}
	else {
		return PCB_R_DIR_NOT_FOUND;
	}
}

void DrawLineOrArc(int type, void *obj)
{
	switch (type) {
	case PCB_OBJ_LINE_POINT:
		/* Attention: We can use a NULL pointer here for the layer,
		 * because it is not used in the pcb_line_invalidate_draw() function anyways.
		 * ATM pcb_line_invalidate_draw() only calls AddPart() internally, which invalidates
		 * the area specified by the line's bounding box.
		 */
		pcb_line_invalidate_draw(NULL, (pcb_line_t *) obj);
		break;
	case PCB_OBJ_ARC_POINT:
		/* See comment above */
		pcb_arc_invalidate_draw(NULL, (pcb_arc_t *) obj);
		break;
	}
}


#define op_swap(crosshair) \
do { \
	vtop_t __tmp__ = crosshair->onpoint_objs; \
	crosshair->onpoint_objs = crosshair->old_onpoint_objs; \
	crosshair->old_onpoint_objs = __tmp__; \
} while(0)

static void *onpoint_find(vtop_t *vect, void *obj_ptr)
{
	int i;

	for (i = 0; i < vect->used; i++) {
		pcb_onpoint_t *op = &(vect->array[i]);
		if (op->obj.any == obj_ptr)
			return op;
	}
	return NULL;
}

/*
 * Searches for lines or arcs which have points that are exactly
 * at the given coordinates and adds them to the crosshair's
 * object list along with their respective type.
 */
static void onpoint_work(pcb_crosshair_t * crosshair, pcb_coord_t X, pcb_coord_t Y)
{
	pcb_box_t SearchBox = pcb_point_box(X, Y);
	struct onpoint_search_info info;
	int i;
	pcb_bool redraw = pcb_false;

	op_swap(crosshair);

	/* Do not truncate to 0 because that may free the array */
	vtop_truncate(&crosshair->onpoint_objs, 1);
	crosshair->onpoint_objs.used = 0;


	info.crosshair = crosshair;
	info.X = X;
	info.Y = Y;

	for (i = 0; i < pcb_max_layer; i++) {
		pcb_layer_t *layer = &PCB->Data->Layer[i];
		/* Only find points of arcs and lines on currently visible layers. */
		if (!layer->meta.real.vis)
			continue;
		pcb_r_search(layer->line_tree, &SearchBox, NULL, onpoint_line_callback, &info, NULL);
		pcb_r_search(layer->arc_tree, &SearchBox, NULL, onpoint_arc_callback, &info, NULL);
	}

	/* Undraw the old objects */
	for (i = 0; i < crosshair->old_onpoint_objs.used; i++) {
		pcb_onpoint_t *op = &crosshair->old_onpoint_objs.array[i];

		/* only remove and redraw those which aren't in the new list */
		if (onpoint_find(&crosshair->onpoint_objs, op->obj.any) != NULL)
			continue;

		PCB_FLAG_CLEAR(PCB_FLAG_ONPOINT, (pcb_any_obj_t *) op->obj.any);
		DrawLineOrArc(op->type, op->obj.any);
		redraw = pcb_true;
	}

	/* draw the new objects */
	for (i = 0; i < crosshair->onpoint_objs.used; i++) {
		pcb_onpoint_t *op = &crosshair->onpoint_objs.array[i];

		/* only draw those which aren't in the old list */
		if (onpoint_find(&crosshair->old_onpoint_objs, op->obj.any) != NULL)
			continue;
		DrawLineOrArc(op->type, op->obj.any);
		redraw = pcb_true;
	}

	if (redraw) {
		pcb_hid_redraw(PCB);
	}
}

static double square(double x)
{
	return x * x;
}

static double crosshair_sq_dist(pcb_crosshair_t * crosshair, pcb_coord_t x, pcb_coord_t y)
{
	return square(x - crosshair->X) + square(y - crosshair->Y);
}

struct snap_data {
	pcb_crosshair_t *crosshair;
	double nearest_sq_dist;
	pcb_bool nearest_is_grid;
	pcb_coord_t x, y;
};

/* Snap to a given location if it is the closest thing we found so far.
 * If "prefer_to_grid" is set, the passed location will take preference
 * over a closer grid points we already snapped to UNLESS the user is
 * pressing the SHIFT key. If the SHIFT key is pressed, the closest object
 * (including grid points), is always preferred.
 */
static void check_snap_object(struct snap_data *snap_data, pcb_coord_t x, pcb_coord_t y, pcb_bool prefer_to_grid, pcb_any_obj_t *snapo)
{
	double sq_dist;

	/* avoid snapping to an object if it is in the same subc */
	if ((snapo != NULL) && (pcbhl_conf.editor.mode == PCB_MODE_MOVE) && (pcb_crosshair.AttachedObject.Type == PCB_OBJ_SUBC)) {
		pcb_any_obj_t *parent = (pcb_any_obj_t *)pcb_obj_parent_subc(snapo);
		int n;
		pcb_cardinal_t parent_id = snapo->ID;
		if (parent != NULL)
			parent_id = parent->ID;
		for(n = 0; n < pcb_crosshair.drags_len; n++) {
			if ((snapo->ID == pcb_crosshair.drags[n]) || (parent_id == pcb_crosshair.drags[n]))
				return;
		}
	}

	sq_dist = crosshair_sq_dist(snap_data->crosshair, x, y);
	if (sq_dist < snap_data->nearest_sq_dist || (prefer_to_grid && snap_data->nearest_is_grid && !pcb_gui->shift_is_pressed())) {
		snap_data->x = x;
		snap_data->y = y;
		snap_data->nearest_sq_dist = sq_dist;
		snap_data->nearest_is_grid = pcb_false;
	}
}

static void check_snap_offgrid_line(struct snap_data *snap_data, pcb_coord_t nearest_grid_x, pcb_coord_t nearest_grid_y)
{
	void *ptr1, *ptr2, *ptr3;
	int ans;
	pcb_line_t *line;
	pcb_coord_t try_x, try_y;
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

	if (!pcb_tool_should_snap_offgrid_line(ptr1, line))
		return;

	dx = line->Point2.X - line->Point1.X;
	dy = line->Point2.Y - line->Point1.Y;

	/* Try snapping along the X axis */
	if (dy != 0.) {
		/* Move in the X direction until we hit the line */
		try_x = (nearest_grid_y - line->Point1.Y) / dy * dx + line->Point1.X;
		try_y = nearest_grid_y;
		check_snap_object(snap_data, try_x, try_y, pcb_true, (pcb_any_obj_t *)line);
	}

	/* Try snapping along the Y axis */
	if (dx != 0.) {
		try_x = nearest_grid_x;
		try_y = (nearest_grid_x - line->Point1.X) / dx * dy + line->Point1.Y;
		check_snap_object(snap_data, try_x, try_y, pcb_true, (pcb_any_obj_t *)line);
	}

	if (dx != dy) {								/* If line not parallel with dX = dY direction.. */
		/* Try snapping diagonally towards the line in the dX = dY direction */

		if (dy == 0)
			dist = line->Point1.Y - nearest_grid_y;
		else
			dist = ((line->Point1.X - nearest_grid_x) - (line->Point1.Y - nearest_grid_y) * dx / dy) / (1 - dx / dy);

		try_x = nearest_grid_x + dist;
		try_y = nearest_grid_y + dist;

		check_snap_object(snap_data, try_x, try_y, pcb_true, (pcb_any_obj_t *)line);
	}

	if (dx != -dy) {							/* If line not parallel with dX = -dY direction.. */
		/* Try snapping diagonally towards the line in the dX = -dY direction */

		if (dy == 0)
			dist = nearest_grid_y - line->Point1.Y;
		else
			dist = ((line->Point1.X - nearest_grid_x) - (line->Point1.Y - nearest_grid_y) * dx / dy) / (1 + dx / dy);

		try_x = nearest_grid_x + dist;
		try_y = nearest_grid_y - dist;

		check_snap_object(snap_data, try_x, try_y, pcb_true, (pcb_any_obj_t *)line);
	}
}

/* ---------------------------------------------------------------------------
 * recalculates the passed coordinates to fit the current grid setting
 */
void pcb_crosshair_grid_fit(pcb_coord_t X, pcb_coord_t Y)
{
	pcb_coord_t nearest_grid_x, nearest_grid_y;
	void *ptr1, *ptr2, *ptr3;
	struct snap_data snap_data;
	int ans;

	pcb_crosshair.X = PCB_CLAMP(X, pcb_crosshair.MinX, pcb_crosshair.MaxX);
	pcb_crosshair.Y = PCB_CLAMP(Y, pcb_crosshair.MinY, pcb_crosshair.MaxY);

	if (PCB->RatDraw) {
		nearest_grid_x = -PCB_MIL_TO_COORD(6);
		nearest_grid_y = -PCB_MIL_TO_COORD(6);
	}
	else {
		nearest_grid_x = pcb_grid_fit(pcb_crosshair.X, PCB->hidlib.grid, PCB->hidlib.grid_ox);
		nearest_grid_y = pcb_grid_fit(pcb_crosshair.Y, PCB->hidlib.grid, PCB->hidlib.grid_oy);

		if (pcb_marked.status && conf_core.editor.orthogonal_moves) {
			pcb_coord_t dx = pcb_crosshair.X - pcb_marked.X;
			pcb_coord_t dy = pcb_crosshair.Y - pcb_marked.Y;
			if (PCB_ABS(dx) > PCB_ABS(dy))
				nearest_grid_y = pcb_marked.Y;
			else
				nearest_grid_x = pcb_marked.X;
		}

	}

	snap_data.crosshair = &pcb_crosshair;
	snap_data.nearest_sq_dist = crosshair_sq_dist(&pcb_crosshair, nearest_grid_x, nearest_grid_y);
	snap_data.nearest_is_grid = pcb_true;
	snap_data.x = nearest_grid_x;
	snap_data.y = nearest_grid_y;

	ans = PCB_OBJ_VOID;
	if (!PCB->RatDraw)
		ans = pcb_search_grid_slop(pcb_crosshair.X, pcb_crosshair.Y, PCB_OBJ_SUBC, &ptr1, &ptr2, &ptr3);

	if (ans & PCB_OBJ_SUBC) {
		pcb_subc_t *sc = (pcb_subc_t *) ptr1;
		pcb_coord_t ox, oy;
		if (pcb_subc_get_origin(sc, &ox, &oy) == 0)
			check_snap_object(&snap_data, ox, oy, pcb_false, (pcb_any_obj_t *)sc);
	}

	/*** padstack center ***/
	ans = PCB_OBJ_VOID;
	if (conf_core.editor.snap_pin)
		ans = pcb_search_grid_slop(pcb_crosshair.X, pcb_crosshair.Y, PCB_OBJ_PSTK | PCB_OBJ_SUBC_PART, &ptr1, &ptr2, &ptr3);

	/* Avoid snapping padstack to any other padstack */
	if (pcbhl_conf.editor.mode == PCB_MODE_MOVE && pcb_crosshair.AttachedObject.Type == PCB_OBJ_PSTK && (ans & PCB_OBJ_PSTK))
		ans = PCB_OBJ_VOID;

	if (ans != PCB_OBJ_VOID) {
		pcb_pstk_t *ps = (pcb_pstk_t *) ptr2;
		check_snap_object(&snap_data, ps->x, ps->y, pcb_true, (pcb_any_obj_t *)ps);
		pcb_crosshair.snapped_pstk = ps;
	}

	/*** arc ***/
	ans = PCB_OBJ_VOID;
	if (conf_core.editor.snap_pin)
		ans = pcb_search_grid_slop(pcb_crosshair.X, pcb_crosshair.Y, PCB_OBJ_LINE_POINT | PCB_OBJ_ARC_POINT | PCB_OBJ_SUBC_PART, &ptr1, &ptr2, &ptr3);

	if (ans == PCB_OBJ_ARC_POINT) {
		/* Arc point needs special handling as it's not a real point but has to be calculated */
		pcb_coord_t ex, ey;
		pcb_arc_get_end((pcb_arc_t *)ptr2, (ptr3 != pcb_arc_start_ptr), &ex, &ey);
		check_snap_object(&snap_data, ex, ey, pcb_true, (pcb_any_obj_t *)ptr2);
	}
	else if (ans != PCB_OBJ_VOID) {
		pcb_point_t *pnt = (pcb_point_t *) ptr3;
		check_snap_object(&snap_data, pnt->X, pnt->Y, pcb_true, (pcb_any_obj_t *)ptr2);
	}

	/*** polygon terminal: center ***/
	ans = PCB_OBJ_VOID;
	if (conf_core.editor.snap_pin)
		ans = pcb_search_grid_slop(pcb_crosshair.X, pcb_crosshair.Y, PCB_OBJ_POLY | PCB_OBJ_SUBC_PART, &ptr1, &ptr2, &ptr3);

	if (ans == PCB_OBJ_POLY) {
		pcb_poly_t *p = ptr2;
		if (p->term != NULL) {
			pcb_coord_t cx, cy;
			cx = (p->BoundingBox.X1 + p->BoundingBox.X2)/2;
			cy = (p->BoundingBox.Y1 + p->BoundingBox.Y2)/2;
			if (pcb_poly_is_point_in_p(cx, cy, 1, p))
				check_snap_object(&snap_data, cx, cy, pcb_true, (pcb_any_obj_t *)p);
		}
	}

	/*
	 * Snap to offgrid points on lines.
	 */
	if (conf_core.editor.snap_offgrid_line)
		check_snap_offgrid_line(&snap_data, nearest_grid_x, nearest_grid_y);

	ans = PCB_OBJ_VOID;
	if (conf_core.editor.snap_pin)
		ans = pcb_search_grid_slop(pcb_crosshair.X, pcb_crosshair.Y, PCB_OBJ_POLY_POINT, &ptr1, &ptr2, &ptr3);

	if (ans != PCB_OBJ_VOID) {
		pcb_point_t *pnt = (pcb_point_t *) ptr3;
		check_snap_object(&snap_data, pnt->X, pnt->Y, pcb_true, NULL);
	}

	if (snap_data.x >= 0 && snap_data.y >= 0) {
		pcb_crosshair.X = snap_data.x;
		pcb_crosshair.Y = snap_data.y;
	}

	if (conf_core.editor.highlight_on_point)
		onpoint_work(&pcb_crosshair, pcb_crosshair.X, pcb_crosshair.Y);

	if (pcbhl_conf.editor.mode == PCB_MODE_ARROW) {
		ans = pcb_search_grid_slop(pcb_crosshair.X, pcb_crosshair.Y, PCB_OBJ_LINE_POINT, &ptr1, &ptr2, &ptr3);
		if (ans == PCB_OBJ_VOID) {
			if ((pcb_gui != NULL) && (pcb_gui->point_cursor != NULL))
				pcb_gui->point_cursor(pcb_false);
		}
		else if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, (pcb_line_t *) ptr2)) {
			if ((pcb_gui != NULL) && (pcb_gui->point_cursor != NULL))
				pcb_gui->point_cursor(pcb_true);
		}
	}

	if (pcbhl_conf.editor.mode == PCB_MODE_LINE && pcb_crosshair.AttachedLine.State != PCB_CH_STATE_FIRST && conf_core.editor.auto_drc)
		pcb_line_enforce_drc();

	pcb_gui->set_crosshair(pcb_crosshair.X, pcb_crosshair.Y, HID_SC_DO_NOTHING);
}

/* ---------------------------------------------------------------------------
 * move crosshair relative (has to be switched off)
 */
void pcb_crosshair_move_relative(pcb_coord_t DeltaX, pcb_coord_t DeltaY)
{
	pcb_crosshair_grid_fit(pcb_crosshair.X + DeltaX, pcb_crosshair.Y + DeltaY);
}

/* ---------------------------------------------------------------------------
 * move crosshair absolute
 * return pcb_true if the crosshair was moved from its existing position
 */
pcb_bool pcb_crosshair_move_absolute(pcb_coord_t X, pcb_coord_t Y)
{
	pcb_coord_t x, y, z;
	pcb_crosshair.ptr_x = X;
	pcb_crosshair.ptr_y = Y;
	x = pcb_crosshair.X;
	y = pcb_crosshair.Y;
	pcb_crosshair_grid_fit(X, Y);
	if (pcb_crosshair.X != x || pcb_crosshair.Y != y) {
		/* back up to old position to notify the GUI
		 * (which might want to erase the old crosshair) */
		z = pcb_crosshair.X;
		pcb_crosshair.X = x;
		x = z;
		z = pcb_crosshair.Y;
		pcb_crosshair.Y = y;
		pcb_notify_crosshair_change(pcb_false);	/* Our caller notifies when it has done */
		/* now move forward again */
		pcb_crosshair.X = x;
		pcb_crosshair.Y = z;
		return pcb_true;
	}
	return pcb_false;
}

void pcb_crosshair_set_range(pcb_coord_t MinX, pcb_coord_t MinY, pcb_coord_t MaxX, pcb_coord_t MaxY)
{
	pcb_crosshair.MinX = MAX(0, MinX);
	pcb_crosshair.MinY = MAX(0, MinY);
	pcb_crosshair.MaxX = MIN(PCB->hidlib.size_x, MaxX);
	pcb_crosshair.MaxY = MIN(PCB->hidlib.size_y, MaxY);

	/* force update of position */
	pcb_crosshair_move_relative(0, 0);
}

/* ---------------------------------------------------------------------------
 * centers the displayed PCB around the specified point (X,Y)
 */
void pcb_center_display(pcb_coord_t X, pcb_coord_t Y)
{
	pcb_coord_t save_grid = PCB->hidlib.grid;
	PCB->hidlib.grid = 1;
	if (pcb_crosshair_move_absolute(X, Y))
		pcb_notify_crosshair_change(pcb_true);
	pcb_gui->set_crosshair(pcb_crosshair.X, pcb_crosshair.Y, HID_SC_WARP_POINTER);
	PCB->hidlib.grid = save_grid;
}

/* ---------------------------------------------------------------------------
 * initializes crosshair stuff
 * clears the struct, allocates to graphical contexts
 */
void pcb_crosshair_init(void)
{
	pcb_crosshair.GC = pcb_hid_make_gc();

	pcb_gui->set_color(pcb_crosshair.GC, &conf_core.appearance.color.crosshair);
	pcb_hid_set_draw_xor(pcb_crosshair.GC, 1);
	pcb_hid_set_line_cap(pcb_crosshair.GC, pcb_cap_round);
	pcb_hid_set_line_width(pcb_crosshair.GC, 1);

	/* set default limits */
	pcb_crosshair.MinX = pcb_crosshair.MinY = 0;
	pcb_crosshair.MaxX = PCB->hidlib.size_x;
	pcb_crosshair.MaxY = PCB->hidlib.size_y;

	/* Initialize the onpoint data. */
	memset(&pcb_crosshair.onpoint_objs, 0, sizeof(vtop_t));
	memset(&pcb_crosshair.old_onpoint_objs, 0, sizeof(vtop_t));

	/* clear the mark */
	pcb_marked.status = pcb_false;

	/* Initialise Line Route */
	pcb_route_init(&pcb_crosshair.Route);

}

void pcb_crosshair_uninit(void)
{
	pcb_poly_free_fields(&pcb_crosshair.AttachedPolygon);
	pcb_route_destroy(&pcb_crosshair.Route);
	pcb_hid_destroy_gc(pcb_crosshair.GC);
}

void pcb_crosshair_range_to_buffer(void)
{
	if (pcbhl_conf.editor.mode == PCB_MODE_PASTE_BUFFER) {
		if (pcb_set_buffer_bbox(PCB_PASTEBUFFER) == 0) {
			pcb_crosshair_set_range(PCB_PASTEBUFFER->X - PCB_PASTEBUFFER->bbox_naked.X1,
											PCB_PASTEBUFFER->Y - PCB_PASTEBUFFER->bbox_naked.Y1,
											PCB->hidlib.size_x -
											(PCB_PASTEBUFFER->bbox_naked.X2 - PCB_PASTEBUFFER->X),
											PCB->hidlib.size_y - (PCB_PASTEBUFFER->bbox_naked.Y2 - PCB_PASTEBUFFER->Y));
		}
		else /* failed to calculate the bounding box of the buffer, it's probably a single-object move, allow the whole page */
			pcb_crosshair_set_range(0, 0, PCB->hidlib.size_x, PCB->hidlib.size_y);
	}
}

void pcb_crosshair_set_local_ref(pcb_coord_t X, pcb_coord_t Y, pcb_bool Showing)
{
	static pcb_mark_t old;
	static int count = 0;

	if (Showing) {
		pcb_notify_mark_change(pcb_false);
		if (count == 0)
			old = pcb_marked;
		pcb_marked.X = X;
		pcb_marked.Y = Y;
		pcb_marked.status = pcb_true;
		count++;
		pcb_notify_mark_change(pcb_true);
	}
	else if (count > 0) {
		pcb_notify_mark_change(pcb_false);
		count = 0;
		pcb_marked = old;
		pcb_notify_mark_change(pcb_true);
	}
}

static void pcb_event_move_crosshair(pcb_coord_t ev_x, pcb_coord_t ev_y)
{
	if (pcb_mid_stroke)
		pcb_stub_stroke_record(ev_x, ev_y);
	if (pcb_crosshair_move_absolute(ev_x, ev_y)) {
		/* update object position and cursor location */
		pcb_tool_adjust_attached_objects();
		pcb_event(&PCB->hidlib, PCB_EVENT_DRAW_CROSSHAIR_CHATT, NULL);
		pcb_notify_crosshair_change(pcb_true);
	}
}


typedef struct {
	int obj, line, box;
} old_crosshair_t;

void *pcb_hidlib_crosshair_suspend(void)
{
	old_crosshair_t *buf = malloc(sizeof(old_crosshair_t));

	buf->obj = pcb_crosshair.AttachedObject.State;
	buf->line = pcb_crosshair.AttachedLine.State;
	buf->box = pcb_crosshair.AttachedBox.State;
	pcb_notify_crosshair_change(pcb_false);
	pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
	pcb_crosshair.AttachedLine.State = PCB_CH_STATE_FIRST;
	pcb_crosshair.AttachedBox.State = PCB_CH_STATE_FIRST;
	pcb_notify_crosshair_change(pcb_true);
	return buf;
}

void pcb_hidlib_crosshair_restore(void *susp_data)
{
	old_crosshair_t *buf = susp_data;

	pcb_notify_crosshair_change(pcb_false);
	pcb_crosshair.AttachedObject.State = buf->obj;
	pcb_crosshair.AttachedLine.State = buf->line;
	pcb_crosshair.AttachedBox.State = buf->box;
	pcb_notify_crosshair_change(pcb_true);

	free(buf);
}


void pcb_hidlib_crosshair_move_to(pcb_coord_t abs_x, pcb_coord_t abs_y, int mouse_mot)
{
	if (!mouse_mot) {
		pcb_notify_crosshair_change(pcb_false);
		pcb_crosshair_move_absolute(abs_x, abs_y);
		pcb_notify_crosshair_change(pcb_true);
	}
	else
		pcb_event_move_crosshair(abs_x, abs_y);
}

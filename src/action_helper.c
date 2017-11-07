/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

/* action routines for output window */

#include "config.h"

#include "conf_core.h"

#include "action_helper.h"
#include "board.h"
#include "change.h"
#include "copy.h"
#include "data.h"
#include "draw.h"
#include "find.h"
#include "insert.h"
#include "polygon.h"
#include "remove.h"
#include "rotate.h"
#include "search.h"
#include "select.h"
#include "undo.h"
#include "stub_stroke.h"
#include "funchash_core.h"
#include "hid_actions.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "event.h"

#include "obj_pinvia_draw.h"
#include "obj_pad_draw.h"
#include "obj_line_draw.h"
#include "obj_arc_draw.h"
#include "obj_elem_draw.h"
#include "obj_text_draw.h"
#include "obj_rat_draw.h"
#include "obj_poly_draw.h"

#include "tool.h"


static void GetGridLockCoordinates(int type, void *ptr1, void *ptr2, void *ptr3, pcb_coord_t * x, pcb_coord_t * y)
{
	switch (type) {
	case PCB_TYPE_VIA:
		*x = ((pcb_pin_t *) ptr2)->X;
		*y = ((pcb_pin_t *) ptr2)->Y;
		break;
	case PCB_TYPE_LINE:
		*x = ((pcb_line_t *) ptr2)->Point1.X;
		*y = ((pcb_line_t *) ptr2)->Point1.Y;
		break;
	case PCB_TYPE_TEXT:
	case PCB_TYPE_ELEMENT_NAME:
		*x = ((pcb_text_t *) ptr2)->X;
		*y = ((pcb_text_t *) ptr2)->Y;
		break;
	case PCB_TYPE_ELEMENT:
		*x = ((pcb_element_t *) ptr2)->MarkX;
		*y = ((pcb_element_t *) ptr2)->MarkY;
		break;
	case PCB_TYPE_POLY:
		*x = ((pcb_poly_t *) ptr2)->Points[0].X;
		*y = ((pcb_poly_t *) ptr2)->Points[0].Y;
		break;

	case PCB_TYPE_LINE_POINT:
	case PCB_TYPE_POLY_POINT:
		*x = ((pcb_point_t *) ptr3)->X;
		*y = ((pcb_point_t *) ptr3)->Y;
		break;
	case PCB_TYPE_ARC:
		pcb_arc_get_end((pcb_arc_t *) ptr2, 0, x, y);
		break;
	case PCB_TYPE_ARC_POINT:
		if (ptr3 != NULL) /* need to check because: if snap off, there's no known endpoint (leave x;y as is, then) */
			pcb_arc_get_end((pcb_arc_t *) ptr2, ((*(int **)ptr3) != pcb_arc_start_ptr), x, y);
		break;
	}
}

void pcb_attach_for_copy(pcb_coord_t PlaceX, pcb_coord_t PlaceY)
{
	pcb_box_t box;
	pcb_coord_t mx = 0, my = 0;

	pcb_event(PCB_EVENT_RUBBER_RESET, NULL);
	if (!conf_core.editor.snap_pin) {
		/* dither the grab point so that the mark, center, etc
		 * will end up on a grid coordinate
		 */
		GetGridLockCoordinates(pcb_crosshair.AttachedObject.Type,
													 pcb_crosshair.AttachedObject.Ptr1,
													 pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3, &mx, &my);
		mx = pcb_grid_fit(mx, PCB->Grid, PCB->GridOffsetX) - mx;
		my = pcb_grid_fit(my, PCB->Grid, PCB->GridOffsetY) - my;
	}
	pcb_crosshair.AttachedObject.X = PlaceX - mx;
	pcb_crosshair.AttachedObject.Y = PlaceY - my;
	if (!pcb_marked.status || conf_core.editor.local_ref)
		pcb_crosshair_set_local_ref(PlaceX - mx, PlaceY - my, pcb_true);
	pcb_crosshair.AttachedObject.State = PCB_CH_STATE_SECOND;

	/* get boundingbox of object and set cursor range */
	GetObjectBoundingBox(pcb_crosshair.AttachedObject.Type, pcb_crosshair.AttachedObject.Ptr1, pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3, &box);
	pcb_crosshair_set_range(pcb_crosshair.AttachedObject.X - box.X1,
										pcb_crosshair.AttachedObject.Y - box.Y1,
										PCB->MaxWidth - (box.X2 - pcb_crosshair.AttachedObject.X),
										PCB->MaxHeight - (box.Y2 - pcb_crosshair.AttachedObject.Y));

	/* get all attached objects if necessary */
	if ((conf_core.editor.mode != PCB_MODE_COPY) && conf_core.editor.rubber_band_mode)
		pcb_event(PCB_EVENT_RUBBER_LOOKUP_LINES, "ippp", pcb_crosshair.AttachedObject.Type, pcb_crosshair.AttachedObject.Ptr1, pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3);
	if (conf_core.editor.mode != PCB_MODE_COPY &&
			(pcb_crosshair.AttachedObject.Type == PCB_TYPE_ELEMENT ||
			 pcb_crosshair.AttachedObject.Type == PCB_TYPE_VIA ||
			 pcb_crosshair.AttachedObject.Type == PCB_TYPE_LINE || pcb_crosshair.AttachedObject.Type == PCB_TYPE_LINE_POINT))
		pcb_event(PCB_EVENT_RUBBER_LOOKUP_RATS, "ippp", pcb_crosshair.AttachedObject.Type, pcb_crosshair.AttachedObject.Ptr1, pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3);
}


/* --------------------------------------------------------------------------- */

/* %start-doc actions 00delta

Many actions take a @code{delta} parameter as the last parameter,
which is an amount to change something.  That @code{delta} may include
units, as an additional parameter, such as @code{Action(Object,5,mm)}.
If no units are specified, the default is PCB's native units
(currently 1/100 mil).  Also, if the delta is prefixed by @code{+} or
@code{-}, the size is increased or decreased by that amount.
Otherwise, the size size is set to the given amount.

@example
Action(Object,5,mil)
Action(Object,+0.5,mm)
Action(Object,-1)
@end example

Actions which take a @code{delta} parameter which do not accept all
these options will specify what they do take.

%end-doc */

/* %start-doc actions 00objects

Many actions act on indicated objects on the board.  They will have
parameters like @code{ToggleObject} or @code{SelectedVias} to indicate
what group of objects they act on.  Unless otherwise specified, these
parameters are defined as follows:

@table @code

@item Object
@itemx ToggleObject
Affects the object under the mouse pointer.  If this action is invoked
from a menu or script, the user will be prompted to click on an
object, which is then the object affected.

@item Selected
@itemx SelectedObjects

Affects all objects which are currently selected.  At least, all
selected objects for which the given action makes sense.

@item SelectedPins
@itemx SelectedVias
@itemx Selected@var{Type}
@itemx @i{etc}
Affects all objects which are both selected and of the @var{Type} specified.

@end table

%end-doc */

/*  %start-doc actions 00macros

@macro pinshapes

Pins, pads, and vias can have various shapes.  All may be round.  Pins
and pads may be square (obviously "square" pads are usually
rectangular).  Pins and vias may be octagonal.  When you change a
shape flag of an element, you actually change all of its pins and
pads.

Note that the square flag takes precedence over the octagon flag,
thus, if both the square and octagon flags are set, the object is
square.  When the square flag is cleared, the pins and pads will be
either round or, if the octagon flag is set, octagonal.

@end macro

%end-doc */

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
pcb_layer_t *lastLayer;

pcb_action_note_t Note;
int defer_updates = 0;
int defer_needs_update = 0;


pcb_bool saved_mode = pcb_false;


void pcb_clear_warnings()
{
	pcb_rtree_it_t it;
	pcb_box_t *n;
	int li;
	pcb_layer_t *l;

	conf_core.temp.rat_warn = pcb_false;

	for(n = pcb_r_first(PCB->Data->pin_tree, &it); n != NULL; n = pcb_r_next(&it)) {
		if (PCB_FLAG_TEST(PCB_FLAG_WARN, (pcb_any_obj_t *)n)) {
			PCB_FLAG_CLEAR(PCB_FLAG_WARN, (pcb_any_obj_t *)n);
			pcb_pin_invalidate_draw((pcb_pin_t *)n);
		}
	}
	pcb_r_end(&it);

	for(n = pcb_r_first(PCB->Data->via_tree, &it); n != NULL; n = pcb_r_next(&it)) {
		if (PCB_FLAG_TEST(PCB_FLAG_WARN, (pcb_any_obj_t *)n)) {
			PCB_FLAG_CLEAR(PCB_FLAG_WARN, (pcb_any_obj_t *)n);
			pcb_via_invalidate_draw((pcb_pin_t *)n);
		}
	}
	pcb_r_end(&it);

	for(n = pcb_r_first(PCB->Data->pad_tree, &it); n != NULL; n = pcb_r_next(&it)) {
		if (PCB_FLAG_TEST(PCB_FLAG_WARN, (pcb_any_obj_t *)n)) {
			PCB_FLAG_CLEAR(PCB_FLAG_WARN, (pcb_any_obj_t *)n);
			pcb_pad_invalidate_draw((pcb_pad_t *)n);
		}
	}
	pcb_r_end(&it);

	for(li = 0, l = PCB->Data->Layer; li < PCB->Data->LayerN; li++,l++) {
		for(n = pcb_r_first(l->line_tree, &it); n != NULL; n = pcb_r_next(&it)) {
			if (PCB_FLAG_TEST(PCB_FLAG_WARN, (pcb_any_obj_t *)n)) {
				PCB_FLAG_CLEAR(PCB_FLAG_WARN, (pcb_any_obj_t *)n);
				pcb_line_invalidate_draw(l, (pcb_line_t *)n);
			}
		}
		pcb_r_end(&it);

		for(n = pcb_r_first(l->arc_tree, &it); n != NULL; n = pcb_r_next(&it)) {
			if (PCB_FLAG_TEST(PCB_FLAG_WARN, (pcb_any_obj_t *)n)) {
				PCB_FLAG_CLEAR(PCB_FLAG_WARN, (pcb_any_obj_t *)n);
				pcb_arc_invalidate_draw(l, (pcb_arc_t *)n);
			}
		}
		pcb_r_end(&it);

		for(n = pcb_r_first(l->polygon_tree, &it); n != NULL; n = pcb_r_next(&it)) {
			if (PCB_FLAG_TEST(PCB_FLAG_WARN, (pcb_any_obj_t *)n)) {
				PCB_FLAG_CLEAR(PCB_FLAG_WARN, (pcb_any_obj_t *)n);
				pcb_poly_invalidate_draw(l, (pcb_poly_t *)n);
			}
		}
		pcb_r_end(&it);

		for(n = pcb_r_first(l->text_tree, &it); n != NULL; n = pcb_r_next(&it)) {
			if (PCB_FLAG_TEST(PCB_FLAG_WARN, (pcb_any_obj_t *)n)) {
				PCB_FLAG_CLEAR(PCB_FLAG_WARN, (pcb_any_obj_t *)n);
				pcb_text_invalidate_draw(l, (pcb_text_t *)n);
			}
		}
		pcb_r_end(&it);
	}

	pcb_draw();
}

void pcb_release_mode(void)
{
	pcb_box_t box;

	if (Note.Click) {
		pcb_box_t box;

		box.X1 = -PCB_MAX_COORD;
		box.Y1 = -PCB_MAX_COORD;
		box.X2 = PCB_MAX_COORD;
		box.Y2 = PCB_MAX_COORD;

		Note.Click = pcb_false;					/* inhibit timer action */
		pcb_undo_save_serial();
		/* unselect first if shift key not down */
		if (!pcb_gui->shift_is_pressed()) {
			if (pcb_select_block(PCB, &box, pcb_false))
				pcb_board_set_changed_flag(pcb_true);
			if (Note.Moving) {
				Note.Moving = 0;
				Note.Hit = 0;
				return;
			}
		}
		/* Restore the SN so that if we select something the deselect/select combo
		   gets the same SN. */
		pcb_undo_restore_serial();
		if (pcb_select_object(PCB))
			pcb_board_set_changed_flag(pcb_true);
		else
			pcb_undo_inc_serial(); /* We didn't select anything new, so, the deselection should get its  own SN. */
		Note.Hit = 0;
		Note.Moving = 0;
	}
	else if (Note.Moving) {
		pcb_undo_restore_serial();
		pcb_notify_mode();
		pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
		pcb_buffer_set_number(Note.Buffer);
		Note.Moving = pcb_false;
		Note.Hit = 0;
	}
	else if (Note.Hit) {
		pcb_notify_mode();
		Note.Hit = 0;
	}
	else if (conf_core.editor.mode == PCB_MODE_ARROW) {
		box.X1 = pcb_crosshair.AttachedBox.Point1.X;
		box.Y1 = pcb_crosshair.AttachedBox.Point1.Y;
		box.X2 = pcb_crosshair.AttachedBox.Point2.X;
		box.Y2 = pcb_crosshair.AttachedBox.Point2.Y;

		pcb_undo_restore_serial();
		if (pcb_select_block(PCB, &box, pcb_true))
			pcb_board_set_changed_flag(pcb_true);
		else if (pcb_bumped)
			pcb_undo_inc_serial();
		pcb_crosshair.AttachedBox.State = PCB_CH_STATE_FIRST;
	}
	if (saved_mode)
		pcb_crosshair_restore_mode();
	saved_mode = pcb_false;
}

void pcb_adjust_attached_objects(void)
{
	pcb_tool_adjust_attached_objects();
}

void pcb_notify_line(void)
{
	int type = PCB_TYPE_NONE;
	void *ptr1, *ptr2, *ptr3;

	if (!pcb_marked.status || conf_core.editor.local_ref)
		pcb_crosshair_set_local_ref(pcb_crosshair.X, pcb_crosshair.Y, pcb_true);
	switch (pcb_crosshair.AttachedLine.State) {
	case PCB_CH_STATE_FIRST:						/* first point */
		if (PCB->RatDraw && pcb_search_screen(pcb_crosshair.X, pcb_crosshair.Y, PCB_TYPE_PAD | PCB_TYPE_PIN, &ptr1, &ptr1, &ptr1) == PCB_TYPE_NONE) {
			pcb_gui->beep();
			break;
		}
		if (conf_core.editor.auto_drc && conf_core.editor.mode == PCB_MODE_LINE) {
			type = pcb_search_screen(pcb_crosshair.X, pcb_crosshair.Y, PCB_TYPE_PIN | PCB_TYPE_PAD | PCB_TYPE_VIA, &ptr1, &ptr2, &ptr3);
			pcb_lookup_conn(pcb_crosshair.X, pcb_crosshair.Y, pcb_true, 1, PCB_FLAG_FOUND);
		}
		if (type == PCB_TYPE_PIN || type == PCB_TYPE_VIA) {
			pcb_crosshair.AttachedLine.Point1.X = pcb_crosshair.AttachedLine.Point2.X = ((pcb_pin_t *) ptr2)->X;
			pcb_crosshair.AttachedLine.Point1.Y = pcb_crosshair.AttachedLine.Point2.Y = ((pcb_pin_t *) ptr2)->Y;
		}
		else if (type == PCB_TYPE_PAD) {
			pcb_pad_t *pad = (pcb_pad_t *) ptr2;
			double d1 = pcb_distance(pcb_crosshair.X, pcb_crosshair.Y, pad->Point1.X, pad->Point1.Y);
			double d2 = pcb_distance(pcb_crosshair.X, pcb_crosshair.Y, pad->Point2.X, pad->Point2.Y);
			double dm = pcb_distance(pcb_crosshair.X, pcb_crosshair.Y, (pad->Point1.X + pad->Point2.X) / 2, (pad->Point1.Y + pad->Point2.Y)/2);
			if ((dm <= d1) && (dm <= d2)) { /* prefer to snap to the middle of a pin if that's the closest */
				pcb_crosshair.AttachedLine.Point1.X = pcb_crosshair.AttachedLine.Point2.X = pcb_crosshair.X;
				pcb_crosshair.AttachedLine.Point1.Y = pcb_crosshair.AttachedLine.Point2.Y = pcb_crosshair.Y;
			}
			else if (d2 < d1) { /* else select the closest endpoint */
				pcb_crosshair.AttachedLine.Point1 = pcb_crosshair.AttachedLine.Point2 = pad->Point2;
			}
			else {
				pcb_crosshair.AttachedLine.Point1 = pcb_crosshair.AttachedLine.Point2 = pad->Point1;
			}
		}
		else {
			pcb_crosshair.AttachedLine.Point1.X = pcb_crosshair.AttachedLine.Point2.X = pcb_crosshair.X;
			pcb_crosshair.AttachedLine.Point1.Y = pcb_crosshair.AttachedLine.Point2.Y = pcb_crosshair.Y;
		}
		pcb_crosshair.AttachedLine.State = PCB_CH_STATE_SECOND;
		break;

	case PCB_CH_STATE_SECOND:
		/* fall through to third state too */
		lastLayer = CURRENT;
	default:											/* all following points */
		pcb_crosshair.AttachedLine.State = PCB_CH_STATE_THIRD;
		break;
	}
}

void pcb_notify_block(void)
{
	pcb_notify_crosshair_change(pcb_false);
	switch (pcb_crosshair.AttachedBox.State) {
	case PCB_CH_STATE_FIRST:						/* setup first point */
		pcb_crosshair.AttachedBox.Point1.X = pcb_crosshair.AttachedBox.Point2.X = pcb_crosshair.X;
		pcb_crosshair.AttachedBox.Point1.Y = pcb_crosshair.AttachedBox.Point2.Y = pcb_crosshair.Y;
		pcb_crosshair.AttachedBox.State = PCB_CH_STATE_SECOND;
		break;

	case PCB_CH_STATE_SECOND:						/* setup second point */
		pcb_crosshair.AttachedBox.State = PCB_CH_STATE_THIRD;
		break;
	}
	pcb_notify_crosshair_change(pcb_true);
}

void pcb_notify_mode(void)
{
	if (conf_core.temp.rat_warn)
		pcb_clear_warnings();
	pcb_tool_notify_mode();
}

void pcb_event_move_crosshair(int ev_x, int ev_y)
{
	if (pcb_mid_stroke)
		pcb_stub_stroke_record(ev_x, ev_y);
	if (pcb_crosshair_move_absolute(ev_x, ev_y)) {
		/* update object position and cursor location */
		pcb_notify_crosshair_change(pcb_true); /* xor-draw-remove from old loc */
		pcb_adjust_attached_objects();
		pcb_notify_crosshair_change(pcb_true); /* xor-draw-flush to new loc */
	}
}

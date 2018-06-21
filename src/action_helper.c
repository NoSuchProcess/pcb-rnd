/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
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
#include "actions.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "event.h"

#include "obj_line_draw.h"
#include "obj_arc_draw.h"
#include "obj_text_draw.h"
#include "obj_rat_draw.h"
#include "obj_poly_draw.h"
#include "obj_pstk_draw.h"

#include "tool.h"

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

int defer_updates = 0;
int defer_needs_update = 0;


void pcb_clear_warnings()
{
	pcb_rtree_it_t it;
	pcb_box_t *n;
	int li;
	pcb_layer_t *l;

	conf_core.temp.rat_warn = pcb_false;

	for(n = pcb_r_first(PCB->Data->padstack_tree, &it); n != NULL; n = pcb_r_next(&it)) {
		if (PCB_FLAG_TEST(PCB_FLAG_WARN, (pcb_any_obj_t *)n)) {
			PCB_FLAG_CLEAR(PCB_FLAG_WARN, (pcb_any_obj_t *)n);
			pcb_pstk_invalidate_draw((pcb_pstk_t *)n);
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
	pcb_tool_release_mode();

	if (pcb_tool_is_saved)
		pcb_tool_restore();
	pcb_tool_is_saved = pcb_false;
	pcb_draw();
}

void pcb_adjust_attached_objects(void)
{
	pcb_tool_adjust_attached_objects();
}

void pcb_notify_mode(void)
{
	if (conf_core.temp.rat_warn)
		pcb_clear_warnings();
	pcb_tool_notify_mode();
	pcb_draw();
}

void pcb_event_move_crosshair(pcb_coord_t ev_x, pcb_coord_t ev_y)
{
	if (pcb_mid_stroke)
		pcb_stub_stroke_record(ev_x, ev_y);
	if (pcb_crosshair_move_absolute(ev_x, ev_y)) {
		/* update object position and cursor location */
		pcb_adjust_attached_objects();
		pcb_notify_crosshair_change(pcb_true);
	}
}

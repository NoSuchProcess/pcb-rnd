/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */


/* routines to update widgets and global settings
 * (except output window and dialogs)
 */

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "action_helper.h"
#include "board.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "set.h"
#include "undo.h"
#include "compat_nls.h"

static int mode_position = 0;
static int mode_stack[MAX_MODESTACK_DEPTH];

/* ---------------------------------------------------------------------------
 * sets cursor grid with respect to grid offset values
 */
void pcb_board_set_grid(pcb_coord_t Grid, pcb_bool align)
{
	if (Grid >= 1 && Grid <= MAX_GRID) {
		if (align) {
			PCB->GridOffsetX = Crosshair.X % Grid;
			PCB->GridOffsetY = Crosshair.Y % Grid;
		}
		PCB->Grid = Grid;
		conf_set_design("editor/grid", "%$mS", Grid);
		if (conf_core.editor.draw_grid)
			pcb_redraw();
	}
}

/* ---------------------------------------------------------------------------
 * sets a new line thickness
 */
void pcb_board_set_line_width(pcb_coord_t Size)
{
	if (Size >= MIN_LINESIZE && Size <= MAX_LINESIZE) {
		conf_set_design("design/line_thickness", "%$mS", Size);
		if (conf_core.editor.auto_drc)
			pcb_crosshair_grid_fit(Crosshair.X, Crosshair.Y);
	}
}

/* ---------------------------------------------------------------------------
 * sets a new via thickness
 */
void pcb_board_set_via_size(pcb_coord_t Size, pcb_bool Force)
{
	if (Force || (Size <= MAX_PINORVIASIZE && Size >= MIN_PINORVIASIZE && Size >= conf_core.design.via_drilling_hole + MIN_PINORVIACOPPER)) {
		conf_set_design("design/via_thickness", "%$mS", Size);
	}
}

/* ---------------------------------------------------------------------------
 * sets a new via drilling hole
 */
void pcb_board_set_via_drilling_hole(pcb_coord_t Size, pcb_bool Force)
{
	if (Force || (Size <= MAX_PINORVIASIZE && Size >= MIN_PINORVIAHOLE && Size <= conf_core.design.via_thickness - MIN_PINORVIACOPPER)) {
		conf_set_design("design/via_drilling_hole", "%$mS", Size);
	}
}

/* ---------------------------------------------------------------------------
 * sets a clearance width
 */
void pcb_board_set_clearance(pcb_coord_t Width)
{
	if (Width <= MAX_LINESIZE) {
		conf_set_design("design/clearance", "%$mS", Width);
	}
}

/* ---------------------------------------------------------------------------
 * sets a text scaling
 */
void pcb_board_set_text_scale(int Scale)
{
	if (Scale <= MAX_TEXTSCALE && Scale >= MIN_TEXTSCALE) {
		conf_set_design("design/text_scale", "%d", Scale);
	}
}

/* ---------------------------------------------------------------------------
 * sets or resets changed flag and redraws status
 */
void pcb_board_set_changed_flag(pcb_bool New)
{
	if (PCB->Changed != New) {
		PCB->Changed = New;

	}
}

/* ---------------------------------------------------------------------------
 * sets the crosshair range to the current buffer extents 
 */
void pcb_crosshair_range_to_buffer(void)
{
	if (conf_core.editor.mode == PCB_MODE_PASTE_BUFFER) {
		pcb_set_buffer_bbox(PCB_PASTEBUFFER);
		pcb_crosshair_set_range(PCB_PASTEBUFFER->X - PCB_PASTEBUFFER->BoundingBox.X1,
											PCB_PASTEBUFFER->Y - PCB_PASTEBUFFER->BoundingBox.Y1,
											PCB->MaxWidth -
											(PCB_PASTEBUFFER->BoundingBox.X2 - PCB_PASTEBUFFER->X),
											PCB->MaxHeight - (PCB_PASTEBUFFER->BoundingBox.Y2 - PCB_PASTEBUFFER->Y));
	}
}

/* ---------------------------------------------------------------------------
 * sets a new buffer number
 */
void pcb_buffer_set_number(int Number)
{
	if (Number >= 0 && Number < MAX_BUFFER) {
		conf_set_design("editor/buffer_number", "%d", Number);

		/* do an update on the crosshair range */
		pcb_crosshair_range_to_buffer();
	}
}

/* ---------------------------------------------------------------------------
 */

void pcb_crosshair_save_mode(void)
{
	mode_stack[mode_position] = conf_core.editor.mode;
	if (mode_position < MAX_MODESTACK_DEPTH - 1)
		mode_position++;
}

void pcb_crosshair_restore_mode(void)
{
	if (mode_position == 0) {
		pcb_message(PCB_MSG_DEFAULT, "hace: underflow of restore mode\n");
		return;
	}
	pcb_crosshair_set_mode(mode_stack[--mode_position]);
}


/* ---------------------------------------------------------------------------
 * set a new mode and update X cursor
 */
void pcb_crosshair_set_mode(int Mode)
{
	char sMode[32];
	static pcb_bool recursing = pcb_false;
	/* protect the cursor while changing the mode
	 * perform some additional stuff depending on the new mode
	 * reset 'state' of attached objects
	 */
	if (recursing)
		return;
	recursing = pcb_true;
	pcb_notify_crosshair_change(pcb_false);
	addedLines = 0;
	Crosshair.AttachedObject.Type = PCB_TYPE_NONE;
	Crosshair.AttachedObject.State = STATE_FIRST;
	Crosshair.AttachedPolygon.PointN = 0;
	if (PCB->RatDraw) {
		if (Mode == PCB_MODE_ARC || Mode == PCB_MODE_RECTANGLE ||
				Mode == PCB_MODE_VIA || Mode == PCB_MODE_POLYGON ||
				Mode == PCB_MODE_POLYGON_HOLE || Mode == PCB_MODE_TEXT || Mode == PCB_MODE_INSERT_POINT || Mode == PCB_MODE_THERMAL) {
			pcb_message(PCB_MSG_DEFAULT, _("That mode is NOT allowed when drawing ratlines!\n"));
			Mode = PCB_MODE_NO;
		}
	}
	if (conf_core.editor.mode == PCB_MODE_LINE && Mode == PCB_MODE_ARC && Crosshair.AttachedLine.State != STATE_FIRST) {
		Crosshair.AttachedLine.State = STATE_FIRST;
		Crosshair.AttachedBox.State = STATE_SECOND;
		Crosshair.AttachedBox.Point1.X = Crosshair.AttachedBox.Point2.X = Crosshair.AttachedLine.Point1.X;
		Crosshair.AttachedBox.Point1.Y = Crosshair.AttachedBox.Point2.Y = Crosshair.AttachedLine.Point1.Y;
		pcb_adjust_attached_objects();
	}
	else if (conf_core.editor.mode == PCB_MODE_ARC && Mode == PCB_MODE_LINE && Crosshair.AttachedBox.State != STATE_FIRST) {
		Crosshair.AttachedBox.State = STATE_FIRST;
		Crosshair.AttachedLine.State = STATE_SECOND;
		Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X = Crosshair.AttachedBox.Point1.X;
		Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y = Crosshair.AttachedBox.Point1.Y;
		sprintf(sMode, "%d", Mode);
		conf_set(CFR_DESIGN, "editor/mode", -1, sMode, POL_OVERWRITE);
		pcb_adjust_attached_objects();
	}
	else {
		if (conf_core.editor.mode == PCB_MODE_ARC || conf_core.editor.mode == PCB_MODE_LINE)
			pcb_crosshair_set_local_ref(0, 0, pcb_false);
		Crosshair.AttachedBox.State = STATE_FIRST;
		Crosshair.AttachedLine.State = STATE_FIRST;
		if (Mode == PCB_MODE_LINE && conf_core.editor.auto_drc) {
			if (pcb_reset_conns(pcb_true)) {
				IncrementUndoSerialNumber();
				pcb_draw();
			}
		}
	}

	sprintf(sMode, "%d", Mode);
	conf_set(CFR_DESIGN, "editor/mode", -1, sMode, POL_OVERWRITE);

	if (Mode == PCB_MODE_PASTE_BUFFER)
		/* do an update on the crosshair range */
		pcb_crosshair_range_to_buffer();
	else
		pcb_crosshair_set_range(0, 0, PCB->MaxWidth, PCB->MaxHeight);

	recursing = pcb_false;

	/* force a crosshair grid update because the valid range
	 * may have changed
	 */
	pcb_crosshair_move_relative(0, 0);
	pcb_notify_crosshair_change(pcb_true);
}

void pcb_crosshair_set_local_ref(pcb_coord_t X, pcb_coord_t Y, pcb_bool Showing)
{
	static pcb_mark_t old;
	static int count = 0;

	if (Showing) {
		pcb_notify_mark_change(pcb_false);
		if (count == 0)
			old = Marked;
		Marked.X = X;
		Marked.Y = Y;
		Marked.status = pcb_true;
		count++;
		pcb_notify_mark_change(pcb_true);
	}
	else if (count > 0) {
		pcb_notify_mark_change(pcb_false);
		count = 0;
		Marked = old;
		pcb_notify_mark_change(pcb_true);
	}
}

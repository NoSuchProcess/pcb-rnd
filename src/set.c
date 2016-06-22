/* $Id$ */

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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "action_helper.h"
#include "buffer.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "set.h"
#include "undo.h"
#include "hid_actions.h"
#include "route_style.h"

RCSID("$Id$");

static int mode_position = 0;
static int mode_stack[MAX_MODESTACK_DEPTH];

/* ---------------------------------------------------------------------------
 * sets cursor grid with respect to grid offset values
 */
void SetGrid(Coord Grid, bool align)
{
	if (Grid >= 1 && Grid <= MAX_GRID) {
		if (align) {
			PCB->GridOffsetX = Crosshair.X % Grid;
			PCB->GridOffsetY = Crosshair.Y % Grid;
		}
		PCB->Grid = Grid;
		if (conf_core.editor.draw_grid)
			Redraw();
	}
}

/* ---------------------------------------------------------------------------
 * sets a new line thickness
 */
void SetLineSize(Coord Size)
{
	if (Size >= MIN_LINESIZE && Size <= MAX_LINESIZE) {
		conf_set_design("design/line_thickness", "%$mS", Size);
		if (conf_core.editor.auto_drc)
			FitCrosshairIntoGrid(Crosshair.X, Crosshair.Y);
	}
}

/* ---------------------------------------------------------------------------
 * sets a new via thickness
 */
void SetViaSize(Coord Size, bool Force)
{
	if (Force || (Size <= MAX_PINORVIASIZE && Size >= MIN_PINORVIASIZE && Size >= conf_core.design.via_drilling_hole + MIN_PINORVIACOPPER)) {
		conf_set_design("design/via_thickness", "%$mS", Size);
	}
}

/* ---------------------------------------------------------------------------
 * sets a new via drilling hole
 */
void SetViaDrillingHole(Coord Size, bool Force)
{
	if (Force || (Size <= MAX_PINORVIASIZE && Size >= MIN_PINORVIAHOLE && Size <= conf_core.design.via_thickness - MIN_PINORVIACOPPER)) {
		conf_set_design("design/via_drilling_hole", "%$mS", Size);
	}
}

void pcb_use_route_style(RouteStyleType * rst)
{
	conf_set_design("design/line_thickness", "%$mS", rst->Thick);
	conf_set_design("design/via_thickness", "%$mS", rst->Diameter);
	conf_set_design("design/via_drilling_hole", "%$mS", rst->Hole);
	conf_set_design("design/clearance", "%$mS", rst->Clearance);
}

/* ---------------------------------------------------------------------------
 * sets a clearance width
 */
void SetClearanceWidth(Coord Width)
{
	if (Width <= MAX_LINESIZE) {
		conf_set_design("design/clearance", "%$mS", Width);
	}
}

/* ---------------------------------------------------------------------------
 * sets a text scaling
 */
void SetTextScale(int Scale)
{
	if (Scale <= MAX_TEXTSCALE && Scale >= MIN_TEXTSCALE) {
		conf_set_design("design/text_scale", "%d", Scale);
	}
}

/* ---------------------------------------------------------------------------
 * sets or resets changed flag and redraws status
 */
void SetChangedFlag(bool New)
{
	if (PCB->Changed != New) {
		PCB->Changed = New;

	}
}

/* ---------------------------------------------------------------------------
 * sets the crosshair range to the current buffer extents 
 */
void SetCrosshairRangeToBuffer(void)
{
	if (conf_core.editor.mode == PASTEBUFFER_MODE) {
		SetBufferBoundingBox(PASTEBUFFER);
		SetCrosshairRange(PASTEBUFFER->X - PASTEBUFFER->BoundingBox.X1,
											PASTEBUFFER->Y - PASTEBUFFER->BoundingBox.Y1,
											PCB->MaxWidth -
											(PASTEBUFFER->BoundingBox.X2 - PASTEBUFFER->X),
											PCB->MaxHeight - (PASTEBUFFER->BoundingBox.Y2 - PASTEBUFFER->Y));
	}
}

/* ---------------------------------------------------------------------------
 * sets a new buffer number
 */
void SetBufferNumber(int Number)
{
	if (Number >= 0 && Number < MAX_BUFFER) {
		conf_set_design("editor/buffer_number", "%d", Number);

		/* do an update on the crosshair range */
		SetCrosshairRangeToBuffer();
	}
}

/* ---------------------------------------------------------------------------
 */

void SaveMode(void)
{
	mode_stack[mode_position] = conf_core.editor.mode;
	if (mode_position < MAX_MODESTACK_DEPTH - 1)
		mode_position++;
}

void RestoreMode(void)
{
	if (mode_position == 0) {
		Message("hace: underflow of restore mode\n");
		return;
	}
	SetMode(mode_stack[--mode_position]);
}


/* ---------------------------------------------------------------------------
 * set a new mode and update X cursor
 */
void SetMode(int Mode)
{
	char sMode[32];
	static bool recursing = false;
	/* protect the cursor while changing the mode
	 * perform some additional stuff depending on the new mode
	 * reset 'state' of attached objects
	 */
	if (recursing)
		return;
	recursing = true;
	notify_crosshair_change(false);
	addedLines = 0;
	Crosshair.AttachedObject.Type = NO_TYPE;
	Crosshair.AttachedObject.State = STATE_FIRST;
	Crosshair.AttachedPolygon.PointN = 0;
	if (PCB->RatDraw) {
		if (Mode == ARC_MODE || Mode == RECTANGLE_MODE ||
				Mode == VIA_MODE || Mode == POLYGON_MODE ||
				Mode == POLYGONHOLE_MODE || Mode == TEXT_MODE || Mode == INSERTPOINT_MODE || Mode == THERMAL_MODE) {
			Message(_("That mode is NOT allowed when drawing ratlines!\n"));
			Mode = NO_MODE;
		}
	}
	if (conf_core.editor.mode == LINE_MODE && Mode == ARC_MODE && Crosshair.AttachedLine.State != STATE_FIRST) {
		Crosshair.AttachedLine.State = STATE_FIRST;
		Crosshair.AttachedBox.State = STATE_SECOND;
		Crosshair.AttachedBox.Point1.X = Crosshair.AttachedBox.Point2.X = Crosshair.AttachedLine.Point1.X;
		Crosshair.AttachedBox.Point1.Y = Crosshair.AttachedBox.Point2.Y = Crosshair.AttachedLine.Point1.Y;
		AdjustAttachedObjects();
	}
	else if (conf_core.editor.mode == ARC_MODE && Mode == LINE_MODE && Crosshair.AttachedBox.State != STATE_FIRST) {
		Crosshair.AttachedBox.State = STATE_FIRST;
		Crosshair.AttachedLine.State = STATE_SECOND;
		Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X = Crosshair.AttachedBox.Point1.X;
		Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y = Crosshair.AttachedBox.Point1.Y;
		sprintf(sMode, "%d", Mode);
		conf_set(CFR_DESIGN, "editor/mode", -1, sMode, POL_OVERWRITE);
		AdjustAttachedObjects();
	}
	else {
		if (conf_core.editor.mode == ARC_MODE || conf_core.editor.mode == LINE_MODE)
			SetLocalRef(0, 0, false);
		Crosshair.AttachedBox.State = STATE_FIRST;
		Crosshair.AttachedLine.State = STATE_FIRST;
		if (Mode == LINE_MODE && conf_core.editor.auto_drc) {
			if (ResetConnections(true)) {
				IncrementUndoSerialNumber();
				Draw();
			}
		}
	}

	sprintf(sMode, "%d", Mode);
	conf_set(CFR_DESIGN, "editor/mode", -1, sMode, POL_OVERWRITE);

	if (Mode == PASTEBUFFER_MODE)
		/* do an update on the crosshair range */
		SetCrosshairRangeToBuffer();
	else
		SetCrosshairRange(0, 0, PCB->MaxWidth, PCB->MaxHeight);

	recursing = false;

	/* force a crosshair grid update because the valid range
	 * may have changed
	 */
	MoveCrosshairRelative(0, 0);
	notify_crosshair_change(true);
}

void SetRouteStyle(char *name)
{
	char num[10];

	STYLE_LOOP(PCB);
	{
		if (name && NSTRCMP(name, style->name) == 0) {
			sprintf(num, "%d", n + 1);
			hid_actionl("RouteStyle", num, NULL);
			break;
		}
	}
	END_LOOP;
}

void SetLocalRef(Coord X, Coord Y, bool Showing)
{
	static MarkType old;
	static int count = 0;

	if (Showing) {
		notify_mark_change(false);
		if (count == 0)
			old = Marked;
		Marked.X = X;
		Marked.Y = Y;
		Marked.status = true;
		count++;
		notify_mark_change(true);
	}
	else if (count > 0) {
		notify_mark_change(false);
		count = 0;
		Marked = old;
		notify_mark_change(true);
	}
}

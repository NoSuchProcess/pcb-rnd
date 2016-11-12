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


/* functions used to insert points into objects
 */

#include "config.h"
#include "conf_core.h"

#include "board.h"
#include "data.h"
#include "select.h"
#include "undo.h"

#include "obj_line_op.h"
#include "obj_rat_op.h"
#include "obj_poly_op.h"

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static pcb_opfunc_t InsertFunctions = {
	InsertPointIntoLine,
	NULL,
	InsertPointIntoPolygon,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	InsertPointIntoRat
};

/* ---------------------------------------------------------------------------
 * inserts point into objects
 */
void *InsertPointIntoObject(int Type, void *Ptr1, void *Ptr2, pcb_cardinal_t * Ptr3, Coord DX, Coord DY, pcb_bool Force, pcb_bool insert_last)
{
	void *ptr;
	pcb_opctx_t ctx;

	ctx.insert.pcb = PCB;
	ctx.insert.x = DX;
	ctx.insert.y = DY;
	ctx.insert.idx = *Ptr3;
	ctx.insert.last = insert_last;
	ctx.insert.forcible = Force;

	/* the operation insert the points to the undo-list */
	ptr = ObjectOperation(&InsertFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	if (ptr != NULL)
		IncrementUndoSerialNumber();
	return (ptr);
}

/* ---------------------------------------------------------------------------
 *  adjusts the insert point to make 45 degree lines as necessary
 */
pcb_point_t *AdjustInsertPoint(void)
{
	static pcb_point_t InsertedPoint;
	double m;
	Coord x, y, m1, m2;
	pcb_line_t *line = (pcb_line_t *) Crosshair.AttachedObject.Ptr2;

	if (Crosshair.AttachedObject.State == STATE_FIRST)
		return NULL;
	Crosshair.AttachedObject.Ptr3 = &InsertedPoint;
	if (gui->shift_is_pressed()) {
		AttachedLineType myline;
		/* only force 45 degree for nearest point */
		if (Distance(Crosshair.X, Crosshair.Y, line->Point1.X, line->Point1.Y) <
				Distance(Crosshair.X, Crosshair.Y, line->Point2.X, line->Point2.Y))
			myline.Point1 = myline.Point2 = line->Point1;
		else
			myline.Point1 = myline.Point2 = line->Point2;
		FortyFiveLine(&myline);
		InsertedPoint.X = myline.Point2.X;
		InsertedPoint.Y = myline.Point2.Y;
		return &InsertedPoint;
	}
	if (conf_core.editor.all_direction_lines) {
		InsertedPoint.X = Crosshair.X;
		InsertedPoint.Y = Crosshair.Y;
		return &InsertedPoint;
	}
	if (Crosshair.X == line->Point1.X)
		m1 = 2;											/* 2 signals infinite slope */
	else {
		m = (double) (Crosshair.X - line->Point1.X) / (Crosshair.Y - line->Point1.Y);
		m1 = 0;
		if (m > PCB_TAN_30_DEGREE)
			m1 = (m > PCB_TAN_60_DEGREE) ? 2 : 1;
		else if (m < -PCB_TAN_30_DEGREE)
			m1 = (m < -PCB_TAN_60_DEGREE) ? 2 : -1;
	}
	if (Crosshair.X == line->Point2.X)
		m2 = 2;											/* 2 signals infinite slope */
	else {
		m = (double) (Crosshair.X - line->Point1.X) / (Crosshair.Y - line->Point1.Y);
		m2 = 0;
		if (m > PCB_TAN_30_DEGREE)
			m2 = (m > PCB_TAN_60_DEGREE) ? 2 : 1;
		else if (m < -PCB_TAN_30_DEGREE)
			m2 = (m < -PCB_TAN_60_DEGREE) ? 2 : -1;
	}
	if (m1 == m2) {
		InsertedPoint.X = line->Point1.X;
		InsertedPoint.Y = line->Point1.Y;
		return &InsertedPoint;
	}
	if (m1 == 2) {
		x = line->Point1.X;
		y = line->Point2.Y + m2 * (line->Point1.X - line->Point2.X);
	}
	else if (m2 == 2) {
		x = line->Point2.X;
		y = line->Point1.Y + m1 * (line->Point2.X - line->Point1.X);
	}
	else {
		x = (line->Point2.Y - line->Point1.Y + m1 * line->Point1.X - m2 * line->Point2.X) / (m1 - m2);
		y = (m1 * line->Point2.Y - m1 * m2 * line->Point2.X - m2 * line->Point1.Y + m1 * m2 * line->Point1.X) / (m1 - m2);
	}
	InsertedPoint.X = x;
	InsertedPoint.Y = y;
	return &InsertedPoint;
}

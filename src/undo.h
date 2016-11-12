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

/* prototypes for undo routines */

#ifndef	PCB_UNDO_H
#define	PCB_UNDO_H

#include "library.h"

#define DRAW_FLAGS	(PCB_FLAG_RAT | PCB_FLAG_SELECTED \
			| PCB_FLAG_HIDENAME | PCB_FLAG_HOLE | PCB_FLAG_OCTAGON | PCB_FLAG_FOUND | PCB_FLAG_CLEARLINE)

											/* different layers */

int Undo(pcb_bool);
int Redo(pcb_bool);
void IncrementUndoSerialNumber(void);
void SaveUndoSerialNumber(void);
void RestoreUndoSerialNumber(void);
void ClearUndoList(pcb_bool);
void MoveObjectToRemoveUndoList(int, void *, void *, void *);
void AddObjectToRemovePointUndoList(int, void *, void *, pcb_cardinal_t);
void AddObjectToInsertPointUndoList(int, void *, void *, void *);
void AddObjectToRemoveContourUndoList(int, pcb_layer_t *, pcb_polygon_t *);
void AddObjectToInsertContourUndoList(int, pcb_layer_t *, pcb_polygon_t *);
void AddObjectToMoveUndoList(int, void *, void *, void *, pcb_coord_t, pcb_coord_t);
void AddObjectToChangeNameUndoList(int, void *, void *, void *, char *);
void AddObjectToChangePinnumUndoList(int, void *, void *, void *, char *);
void AddObjectToRotateUndoList(int, void *, void *, void *, pcb_coord_t, pcb_coord_t, pcb_uint8_t);
void AddObjectToCreateUndoList(int, void *, void *, void *);
void AddObjectToMirrorUndoList(int, void *, void *, void *, pcb_coord_t);
void AddObjectToMoveToLayerUndoList(int, void *, void *, void *);
void AddObjectToFlagUndoList(int, void *, void *, void *);
void AddObjectToSizeUndoList(int, void *, void *, void *);
void AddObjectTo2ndSizeUndoList(int, void *, void *, void *);
void AddObjectToClearSizeUndoList(int, void *, void *, void *);
void AddObjectToMaskSizeUndoList(int, void *, void *, void *);
void AddObjectToChangeAnglesUndoList(int, void *, void *, void *);
void AddObjectToChangeRadiiUndoList(int, void *, void *, void *);
void AddObjectToClearPolyUndoList(int, void *, void *, void *, pcb_bool);
void AddLayerChangeToUndoList(int, int);
void AddNetlistLibToUndoList(pcb_lib_t *);
void LockUndo(void);
void UnlockUndo(void);
pcb_bool Undoing(void);

/* Publish actions - these may be useful for other actions */
int ActionUndo(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);
int ActionRedo(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);
int ActionAtomic(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);

/* ---------------------------------------------------------------------------
 * define supported types of undo operations
 * note these must be separate bits now
 */
#define	UNDO_CHANGENAME			0x0001	/* change of names */
#define	UNDO_MOVE			0x0002		/* moving objects */
#define	UNDO_REMOVE			0x0004	/* removing objects */
#define	UNDO_REMOVE_POINT		0x0008	/* removing polygon/... points */
#define	UNDO_INSERT_POINT		0x0010	/* inserting polygon/... points */
#define	UNDO_REMOVE_CONTOUR		0x0020	/* removing a contour from a polygon */
#define	UNDO_INSERT_CONTOUR		0x0040	/* inserting a contour from a polygon */
#define	UNDO_ROTATE			0x0080	/* rotations */
#define	UNDO_CREATE			0x0100	/* creation of objects */
#define	UNDO_MOVETOLAYER		0x0200	/* moving objects to */
#define	UNDO_FLAG			0x0400		/* toggling SELECTED flag */
#define	UNDO_CHANGESIZE			0x0800	/* change size of object */
#define	UNDO_CHANGE2NDSIZE		0x1000	/* change 2ndSize of object */
#define	UNDO_MIRROR			0x2000	/* change side of board */
#define	UNDO_CHANGECLEARSIZE		0x4000	/* change clearance size */
#define	UNDO_CHANGEMASKSIZE		0x8000	/* change mask size */
#define	UNDO_CHANGEANGLES	       0x10000	/* change arc angles */
#define	UNDO_LAYERCHANGE	       0x20000	/* layer new/delete/move */
#define	UNDO_CLEAR		       0x40000	/* clear/restore to polygons */
#define	UNDO_NETLISTCHANGE	       0x80000	/* netlist change */
#define	UNDO_CHANGEPINNUM			0x100000	/* change of pin number */
#define	UNDO_CHANGERADII	    0x200000	/* change arc radii */

#endif

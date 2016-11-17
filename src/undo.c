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

/* functions used to undo operations
 *
 * Description:
 * There are two lists which hold
 *   - information about a command
 *   - data of removed objects
 * Both lists are organized as first-in-last-out which means that the undo
 * list can always use the last entry of the remove list.
 * A serial number is incremented whenever an operation is completed.
 * An operation itself may consist of several basic instructions.
 * E.g.: removing all selected objects is one operation with exactly one
 * serial number even if the remove function is called several times.
 *
 * a lock flag ensures that no infinite loops occur
 */

#include "config.h"

#include <assert.h>

#include "board.h"
#include "change.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "insert.h"
#include "polygon.h"
#include "remove.h"
#include "rotate.h"
#include "search.h"
#include "undo.h"
#include "flag_str.h"
#include "conf_core.h"
#include "compat_misc.h"
#include "compat_nls.h"

#include "obj_elem_draw.h"
#include "obj_poly_draw.h"

#define STEP_REMOVELIST 500
#define STEP_UNDOLIST   500


static pcb_bool between_increment_and_restore = pcb_false;
static pcb_bool added_undo_between_increment_and_restore = pcb_false;

/* ---------------------------------------------------------------------------
 * some local data types
 */
typedef struct {								/* information about a change command */
	char *Name;
} ChangeNameType, *ChangeNameTypePtr;

typedef struct {								/* information about a move command */
	pcb_coord_t DX, DY;									/* movement vector */
} MoveType, *MoveTypePtr;

typedef struct {								/* information about removed polygon points */
	pcb_coord_t X, Y;										/* data */
	int ID;
	pcb_cardinal_t Index;								/* index in a polygons array of points */
	pcb_bool last_in_contour;					/* Whether the point was the last in its contour */
} RemovedPointType, *RemovedPointTypePtr;

typedef struct {								/* information about rotation */
	pcb_coord_t CenterX, CenterY;				/* center of rotation */
	pcb_cardinal_t Steps;								/* number of steps */
} RotateType, *RotateTypePtr;

typedef struct {								/* information about moves between layers */
	pcb_cardinal_t OriginalLayer;				/* the index of the original layer */
} MoveToLayer;

typedef struct {								/* information about layer changes */
	int old_index;
	int new_index;
} LayerChangeType, *LayerChangeTypePtr;

typedef struct {								/* information about poly clear/restore */
	pcb_bool Clear;										/* pcb_true was clear, pcb_false was restore */
	pcb_layer_t *Layer;
} ClearPolyType, *ClearPolyTypePtr;

typedef struct {
	pcb_angle_t angle[2];
} AngleChangeType;

typedef struct {								/* information about netlist lib changes */
	pcb_lib_t *old;
	pcb_lib_t *lib;
} NetlistChangeType, *NetlistChangeTypePtr;

typedef struct {								/* holds information about an operation */
	int Serial,										/* serial number of operation */
	  Type,												/* type of operation */
	  Kind,												/* type of object with given ID */
	  ID;													/* object ID */
	union {												/* some additional information */
		ChangeNameType ChangeName;
		MoveType Move;
		RemovedPointType RemovedPoint;
		RotateType Rotate;
		MoveToLayer MoveToLayer;
		pcb_flag_t Flags;
		pcb_coord_t Size;
		LayerChangeType LayerChange;
		ClearPolyType ClearPoly;
		NetlistChangeType NetlistChange;
		long int CopyID;
		AngleChangeType AngleChange;
	} Data;
} UndoListType, *UndoListTypePtr;

/* ---------------------------------------------------------------------------
 * some local variables
 */
static pcb_data_t *RemoveList = NULL;	/* list of removed objects */
static UndoListTypePtr UndoList = NULL;	/* list of operations */
static int Serial = 1,					/* serial number */
	SavedSerial;
static size_t UndoN, RedoN,			/* number of entries */
  UndoMax;
static pcb_bool Locked = pcb_false;			/* do not add entries if */
static pcb_bool andDraw = pcb_true;
										/* flag is set; prevents from */
										/* infinite loops */

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static UndoListTypePtr GetUndoSlot(int, int, int);
static void DrawRecoveredObject(int, void *, void *, void *);
static pcb_bool UndoRotate(UndoListTypePtr);
static pcb_bool UndoChangeName(UndoListTypePtr);
static pcb_bool UndoCopyOrCreate(UndoListTypePtr);
static pcb_bool UndoMove(UndoListTypePtr);
static pcb_bool UndoRemove(UndoListTypePtr);
static pcb_bool UndoRemovePoint(UndoListTypePtr);
static pcb_bool UndoInsertPoint(UndoListTypePtr);
static pcb_bool UndoRemoveContour(UndoListTypePtr);
static pcb_bool UndoInsertContour(UndoListTypePtr);
static pcb_bool UndoMoveToLayer(UndoListTypePtr);
static pcb_bool UndoFlag(UndoListTypePtr);
static pcb_bool UndoMirror(UndoListTypePtr);
static pcb_bool UndoChangeSize(UndoListTypePtr);
static pcb_bool UndoChange2ndSize(UndoListTypePtr);
static pcb_bool UndoChangeAngles(UndoListTypePtr);
static pcb_bool UndoChangeRadii(UndoListTypePtr);
static pcb_bool UndoChangeClearSize(UndoListTypePtr);
static pcb_bool UndoChangeMaskSize(UndoListTypePtr);
static pcb_bool UndoClearPoly(UndoListTypePtr);
static int PerformUndo(UndoListTypePtr);

/* ---------------------------------------------------------------------------
 * adds a command plus some data to the undo list
 */
static UndoListTypePtr GetUndoSlot(int CommandType, int ID, int Kind)
{
	UndoListTypePtr ptr;
	void *ptr1, *ptr2, *ptr3;
	int type;
	size_t limit = ((size_t)conf_core.editor.undo_warning_size) * 1024;

#ifdef DEBUG_ID
	if (pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, ID, Kind) == PCB_TYPE_NONE)
		pcb_message(PCB_MSG_DEFAULT, "hace: ID (%d) and Type (%x) mismatch in AddObject...\n", ID, Kind);
#endif

	/* allocate memory */
	if (UndoN >= UndoMax) {
		size_t size;

		UndoMax += STEP_UNDOLIST;
		size = UndoMax * sizeof(UndoListType);
		UndoList = (UndoListTypePtr) realloc(UndoList, size);
		memset(&UndoList[UndoN], 0, STEP_REMOVELIST * sizeof(UndoListType));

		/* ask user to flush the table because of it's size */
		if (size > limit) {
			size_t l2;
			l2 = (size / limit + 1) * limit;
			pcb_message(PCB_MSG_DEFAULT, _("Size of 'undo-list' exceeds %li kb\n"), (long) (l2 >> 10));
		}
	}

	/* free structures from the pruned redo list */

	for (ptr = &UndoList[UndoN]; RedoN; ptr++, RedoN--)
		switch (ptr->Type) {
		case PCB_UNDO_CHANGENAME:
		case PCB_UNDO_CHANGEPINNUM:
			free(ptr->Data.ChangeName.Name);
			break;
		case PCB_UNDO_REMOVE:
			type = pcb_search_obj_by_id(RemoveList, &ptr1, &ptr2, &ptr3, ptr->ID, ptr->Kind);
			if (type != PCB_TYPE_NONE) {
				pcb_destroy_object(RemoveList, type, ptr1, ptr2, ptr3);
			}
			break;
		default:
			break;
		}

	if (between_increment_and_restore)
		added_undo_between_increment_and_restore = pcb_true;

	/* copy typefield and serial number to the list */
	ptr = &UndoList[UndoN++];
	ptr->Type = CommandType;
	ptr->Kind = Kind;
	ptr->ID = ID;
	ptr->Serial = Serial;
	return (ptr);
}

/* ---------------------------------------------------------------------------
 * redraws the recovered object
 */
static void DrawRecoveredObject(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	if (Type & (PCB_TYPE_LINE | PCB_TYPE_TEXT | PCB_TYPE_POLYGON | PCB_TYPE_ARC)) {
		pcb_layer_t *layer;

		layer = LAYER_PTR(GetLayerNumber(RemoveList, (pcb_layer_t *) Ptr1));
		pcb_draw_obj(Type, (void *) layer, Ptr2);
	}
	else
		pcb_draw_obj(Type, Ptr1, Ptr2);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a 'rotate' operation
 * returns pcb_true if anything has been recovered
 */
static pcb_bool UndoRotate(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;

	/* lookup entry by it's ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_TYPE_NONE) {
		pcb_obj_rotate90(type, ptr1, ptr2, ptr3,
								 Entry->Data.Rotate.CenterX, Entry->Data.Rotate.CenterY, (4 - Entry->Data.Rotate.Steps) & 0x03);
		Entry->Data.Rotate.Steps = (4 - Entry->Data.Rotate.Steps) & 0x03;
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a clear/restore poly operation
 * returns pcb_true if anything has been recovered
 */
static pcb_bool UndoClearPoly(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;

	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_TYPE_NONE) {
		if (Entry->Data.ClearPoly.Clear)
			pcb_poly_restore_to_poly(PCB->Data, type, Entry->Data.ClearPoly.Layer, ptr3);
		else
			pcb_poly_clear_from_poly(PCB->Data, type, Entry->Data.ClearPoly.Layer, ptr3);
		Entry->Data.ClearPoly.Clear = !Entry->Data.ClearPoly.Clear;
		return pcb_true;
	}
	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * recovers an object from a 'change name' operation
 * returns pcb_true if anything has been recovered
 */
static pcb_bool UndoChangeName(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;

	/* lookup entry by it's ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_TYPE_NONE) {
		Entry->Data.ChangeName.Name = (char *) (pcb_chg_obj_name(type, ptr1, ptr2, ptr3, Entry->Data.ChangeName.Name));
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a 'change oinnum' operation
 * returns pcb_true if anything has been recovered
 */
static pcb_bool UndoChangePinnum(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;

	/* lookup entry by it's ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_TYPE_NONE) {
		Entry->Data.ChangeName.Name = (char *) (pcb_chg_obj_pinnum(type, ptr1, ptr2, ptr3, Entry->Data.ChangeName.Name));
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a 2ndSize change operation
 */
static pcb_bool UndoChange2ndSize(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	pcb_coord_t swap;

	/* lookup entry by ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_TYPE_NONE) {
		swap = ((pcb_pin_t *) ptr2)->DrillingHole;
		if (andDraw)
			pcb_erase_obj(type, ptr1, ptr2);
		((pcb_pin_t *) ptr2)->DrillingHole = Entry->Data.Size;
		Entry->Data.Size = swap;
		pcb_draw_obj(type, ptr1, ptr2);
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a ChangeAngles change operation
 */
static pcb_bool UndoChangeAngles(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	double old_sa, old_da;

	/* lookup entry by ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type == PCB_TYPE_ARC) {
		pcb_layer_t *Layer = (pcb_layer_t *) ptr1;
		pcb_arc_t *a = (pcb_arc_t *) ptr2;
		pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) a);
		old_sa = a->StartAngle;
		old_da = a->Delta;
		if (andDraw)
			pcb_erase_obj(type, Layer, a);
		a->StartAngle = Entry->Data.AngleChange.angle[0];
		a->Delta = Entry->Data.AngleChange.angle[1];
		pcb_arc_bbox(a);
		pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) a, 0);
		Entry->Data.AngleChange.angle[0] = old_sa;
		Entry->Data.AngleChange.angle[1] = old_da;
		pcb_draw_obj(type, ptr1, a);
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a ChangeRadii change operation
 */
static pcb_bool UndoChangeRadii(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	pcb_coord_t old_w, old_h;

	/* lookup entry by ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type == PCB_TYPE_ARC) {
		pcb_layer_t *Layer = (pcb_layer_t *) ptr1;
		pcb_arc_t *a = (pcb_arc_t *) ptr2;
		pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) a);
		old_w = a->Width;
		old_h = a->Height;
		if (andDraw)
			pcb_erase_obj(type, Layer, a);
		a->Width = Entry->Data.Move.DX;
		a->Height = Entry->Data.Move.DY;
		pcb_arc_bbox(a);
		pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) a, 0);
		Entry->Data.Move.DX = old_w;
		Entry->Data.Move.DY = old_h;
		pcb_draw_obj(type, ptr1, a);
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a clearance size change operation
 */
static pcb_bool UndoChangeClearSize(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	pcb_coord_t swap;

	/* lookup entry by ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_TYPE_NONE) {
		swap = ((pcb_pin_t *) ptr2)->Clearance;
		pcb_poly_restore_to_poly(PCB->Data, type, ptr1, ptr2);
		if (andDraw)
			pcb_erase_obj(type, ptr1, ptr2);
		((pcb_pin_t *) ptr2)->Clearance = Entry->Data.Size;
		pcb_poly_clear_from_poly(PCB->Data, type, ptr1, ptr2);
		Entry->Data.Size = swap;
		if (andDraw)
			pcb_draw_obj(type, ptr1, ptr2);
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a mask size change operation
 */
static pcb_bool UndoChangeMaskSize(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	pcb_coord_t swap;

	/* lookup entry by ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type & (PCB_TYPE_VIA | PCB_TYPE_PIN | PCB_TYPE_PAD)) {
		swap = (type == PCB_TYPE_PAD ? ((pcb_pad_t *) ptr2)->Mask : ((pcb_pin_t *) ptr2)->Mask);
		if (andDraw)
			pcb_erase_obj(type, ptr1, ptr2);
		if (type == PCB_TYPE_PAD)
			((pcb_pad_t *) ptr2)->Mask = Entry->Data.Size;
		else
			((pcb_pin_t *) ptr2)->Mask = Entry->Data.Size;
		Entry->Data.Size = swap;
		if (andDraw)
			pcb_draw_obj(type, ptr1, ptr2);
		return (pcb_true);
	}
	return (pcb_false);
}


/* ---------------------------------------------------------------------------
 * recovers an object from a Size change operation
 */
static pcb_bool UndoChangeSize(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3, *ptr1e;
	int type;
	pcb_coord_t swap;

	/* lookup entry by ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
		if (type == PCB_TYPE_ELEMENT_NAME)
			ptr1e = NULL;
		else
			ptr1e = ptr1;

	if (type != PCB_TYPE_NONE) {
		/* Wow! can any object be treated as a pin type for size change?? */
		/* pins, vias, lines, and arcs can. Text can't but it has it's own mechanism */
		swap = ((pcb_pin_t *) ptr2)->Thickness;
		pcb_poly_restore_to_poly(PCB->Data, type, ptr1, ptr2);
		if ((andDraw) && (ptr1e != NULL))
			pcb_erase_obj(type, ptr1e, ptr2);
		((pcb_pin_t *) ptr2)->Thickness = Entry->Data.Size;
		Entry->Data.Size = swap;
		pcb_poly_clear_from_poly(PCB->Data, type, ptr1, ptr2);
		if (andDraw)
			pcb_draw_obj(type, ptr1, ptr2);
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a FLAG change operation
 */
static pcb_bool UndoFlag(UndoListTypePtr Entry)
{
	void *ptr1, *ptr1e, *ptr2, *ptr3;
	int type;
	pcb_flag_t swap;
	int must_redraw;

	/* lookup entry by ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_TYPE_NONE) {
		pcb_flag_t f1, f2;
		pcb_pin_t *pin = (pcb_pin_t *) ptr2;

		if ((type == PCB_TYPE_ELEMENT) || (type == PCB_TYPE_ELEMENT_NAME))
			ptr1e = NULL;
		else
			ptr1e = ptr1;

		swap = pin->Flags;

		must_redraw = 0;
		f1 = pcb_flag_mask(pin->Flags, ~DRAW_FLAGS);
		f2 = pcb_flag_mask(Entry->Data.Flags, ~DRAW_FLAGS);

		if (!PCB_FLAG_EQ(f1, f2))
			must_redraw = 1;

		if (andDraw && must_redraw && (ptr1e != NULL))
			pcb_erase_obj(type, ptr1e, ptr2);

		pin->Flags = Entry->Data.Flags;

		Entry->Data.Flags = swap;

		if (andDraw && must_redraw)
			pcb_draw_obj(type, ptr1, ptr2);
		return (pcb_true);
	}
	pcb_message(PCB_MSG_DEFAULT, "hace Internal error: Can't find ID %d type %08x\n", Entry->ID, Entry->Kind);
	pcb_message(PCB_MSG_DEFAULT, "for UndoFlag Operation. Previous flags: %s\n", pcb_strflg_f2s(Entry->Data.Flags, 0));
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a mirror operation
 * returns pcb_true if anything has been recovered
 */
static pcb_bool UndoMirror(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;

	/* lookup entry by ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type == PCB_TYPE_ELEMENT) {
		pcb_element_t *element = (pcb_element_t *) ptr3;
		if (andDraw)
			EraseElement(element);
		pcb_element_mirror(PCB->Data, element, Entry->Data.Move.DY);
		if (andDraw)
			DrawElement(element);
		return (pcb_true);
	}
	pcb_message(PCB_MSG_DEFAULT, "hace Internal error: UndoMirror on object type %d\n", type);
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a 'copy' or 'create' operation
 * returns pcb_true if anything has been recovered
 */
static pcb_bool UndoCopyOrCreate(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;

	/* lookup entry by it's ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_TYPE_NONE) {
		if (!RemoveList)
			RemoveList = pcb_buffer_new();
		if (andDraw)
			pcb_erase_obj(type, ptr1, ptr2);
		/* in order to make this re-doable we move it to the RemoveList */
		pcb_move_obj_to_buffer(RemoveList, PCB->Data, type, ptr1, ptr2, ptr3);
		Entry->Type = PCB_UNDO_REMOVE;
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * recovers an object from a 'move' operation
 * returns pcb_true if anything has been recovered
 */
static pcb_bool UndoMove(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;

	/* lookup entry by it's ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_TYPE_NONE) {
		pcb_move_obj(type, ptr1, ptr2, ptr3, -Entry->Data.Move.DX, -Entry->Data.Move.DY);
		Entry->Data.Move.DX *= -1;
		Entry->Data.Move.DY *= -1;
		return (pcb_true);
	}
	return (pcb_false);
}

/* ----------------------------------------------------------------------
 * recovers an object from a 'remove' operation
 * returns pcb_true if anything has been recovered
 */
static pcb_bool UndoRemove(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;

	/* lookup entry by it's ID */
	type = pcb_search_obj_by_id(RemoveList, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_TYPE_NONE) {
		if (andDraw)
			DrawRecoveredObject(type, ptr1, ptr2, ptr3);
		pcb_move_obj_to_buffer(PCB->Data, RemoveList, type, ptr1, ptr2, ptr3);
		Entry->Type = PCB_UNDO_CREATE;
		return (pcb_true);
	}
	return (pcb_false);
}

/* ----------------------------------------------------------------------
 * recovers an object from a 'move to another layer' operation
 * returns pcb_true if anything has been recovered
 */
static pcb_bool UndoMoveToLayer(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type, swap;

	/* lookup entry by it's ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_TYPE_NONE) {
		swap = GetLayerNumber(PCB->Data, (pcb_layer_t *) ptr1);
		pcb_move_obj_to_layer(type, ptr1, ptr2, ptr3, LAYER_PTR(Entry->Data.MoveToLayer.OriginalLayer), pcb_true);
		Entry->Data.MoveToLayer.OriginalLayer = swap;
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * recovers a removed polygon point
 * returns pcb_true on success
 */
static pcb_bool UndoRemovePoint(UndoListTypePtr Entry)
{
	pcb_layer_t *layer;
	pcb_polygon_t *polygon;
	void *ptr3;
	int type;

	/* lookup entry (polygon not point was saved) by it's ID */
	assert(Entry->Kind == PCB_TYPE_POLYGON);
	type = pcb_search_obj_by_id(PCB->Data, (void **) &layer, (void **) &polygon, &ptr3, Entry->ID, Entry->Kind);
	switch (type) {
	case PCB_TYPE_POLYGON:						/* restore the removed point */
		{
			/* recover the point */
			if (andDraw && layer->On)
				ErasePolygon(polygon);
			pcb_insert_point_in_object(PCB_TYPE_POLYGON, layer, polygon,
														&Entry->Data.RemovedPoint.Index,
														Entry->Data.RemovedPoint.X,
														Entry->Data.RemovedPoint.Y, pcb_true, Entry->Data.RemovedPoint.last_in_contour);

			polygon->Points[Entry->Data.RemovedPoint.Index].ID = Entry->Data.RemovedPoint.ID;
			if (andDraw && layer->On)
				DrawPolygon(layer, polygon);
			Entry->Type = PCB_UNDO_INSERT_POINT;
			Entry->ID = Entry->Data.RemovedPoint.ID;
			Entry->Kind = PCB_TYPE_POLYGON_POINT;
			return (pcb_true);
		}

	default:
		return (pcb_false);
	}
}

/* ---------------------------------------------------------------------------
 * recovers an inserted polygon point
 * returns pcb_true on success
 */
static pcb_bool UndoInsertPoint(UndoListTypePtr Entry)
{
	pcb_layer_t *layer;
	pcb_polygon_t *polygon;
	pcb_point_t *pnt;
	int type;
	pcb_cardinal_t point_idx;
	pcb_cardinal_t hole;
	pcb_bool last_in_contour = pcb_false;

	assert(Entry->Kind == PCB_TYPE_POLYGON_POINT);
	/* lookup entry by it's ID */
	type = pcb_search_obj_by_id(PCB->Data, (void **) &layer, (void **) &polygon, (void **) &pnt, Entry->ID, Entry->Kind);
	switch (type) {
	case PCB_TYPE_POLYGON_POINT:			/* removes an inserted polygon point */
		{
			if (andDraw && layer->On)
				ErasePolygon(polygon);

			/* Check whether this point was at the end of its contour.
			 * If so, we need to flag as such when re-adding the point
			 * so it goes back in the correct place
			 */
			point_idx = pcb_poly_point_idx(polygon, pnt);
			for (hole = 0; hole < polygon->HoleIndexN; hole++)
				if (point_idx == polygon->HoleIndex[hole] - 1)
					last_in_contour = pcb_true;
			if (point_idx == polygon->PointN - 1)
				last_in_contour = pcb_true;
			Entry->Data.RemovedPoint.last_in_contour = last_in_contour;

			Entry->Data.RemovedPoint.X = pnt->X;
			Entry->Data.RemovedPoint.Y = pnt->Y;
			Entry->Data.RemovedPoint.ID = pnt->ID;
			Entry->ID = polygon->ID;
			Entry->Kind = PCB_TYPE_POLYGON;
			Entry->Type = PCB_UNDO_REMOVE_POINT;
			Entry->Data.RemovedPoint.Index = point_idx;
			pcb_destroy_object(PCB->Data, PCB_TYPE_POLYGON_POINT, layer, polygon, pnt);
			if (andDraw && layer->On)
				DrawPolygon(layer, polygon);
			return (pcb_true);
		}

	default:
		return (pcb_false);
	}
}

static pcb_bool UndoSwapCopiedObject(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	void *ptr1b, *ptr2b, *ptr3b;
	pcb_any_obj_t *obj, *obj2;
	int type;
	long int swap_id;

	/* lookup entry by it's ID */
	type = pcb_search_obj_by_id(RemoveList, &ptr1, &ptr2, &ptr3, Entry->Data.CopyID, Entry->Kind);
	if (type == PCB_TYPE_NONE)
		return pcb_false;

	type = pcb_search_obj_by_id(PCB->Data, &ptr1b, &ptr2b, &ptr3b, Entry->ID, Entry->Kind);
	if (type == PCB_TYPE_NONE)
		return pcb_false;

	obj = (pcb_any_obj_t *) ptr2;
	obj2 = (pcb_any_obj_t *) ptr2b;

	swap_id = obj->ID;
	obj->ID = obj2->ID;
	obj2->ID = swap_id;

	pcb_move_obj_to_buffer(RemoveList, PCB->Data, type, ptr1b, ptr2b, ptr3b);

	if (andDraw)
		DrawRecoveredObject(Entry->Kind, ptr1, ptr2, ptr3);

	obj = (pcb_any_obj_t *) pcb_move_obj_to_buffer(PCB->Data, RemoveList, type, ptr1, ptr2, ptr3);
	if (Entry->Kind == PCB_TYPE_POLYGON)
		pcb_poly_init_clip(PCB->Data, (pcb_layer_t *) ptr1b, (pcb_polygon_t *) obj);
	return (pcb_true);
}

/* ---------------------------------------------------------------------------
 * recovers an removed polygon point
 * returns pcb_true on success
 */
static pcb_bool UndoRemoveContour(UndoListTypePtr Entry)
{
	assert(Entry->Kind == PCB_TYPE_POLYGON);
	return UndoSwapCopiedObject(Entry);
}

/* ---------------------------------------------------------------------------
 * recovers an inserted polygon point
 * returns pcb_true on success
 */
static pcb_bool UndoInsertContour(UndoListTypePtr Entry)
{
	assert(Entry->Kind == PCB_TYPE_POLYGON);
	return UndoSwapCopiedObject(Entry);
}

/* ---------------------------------------------------------------------------
 * undo a layer change
 * returns pcb_true on success
 */
static pcb_bool UndoLayerChange(UndoListTypePtr Entry)
{
	LayerChangeTypePtr l = &Entry->Data.LayerChange;
	int tmp;

	tmp = l->new_index;
	l->new_index = l->old_index;
	l->old_index = tmp;

	if (pcb_layer_move(l->old_index, l->new_index))
		return pcb_false;
	else
		return pcb_true;
}

/* ---------------------------------------------------------------------------
 * undo a netlist change
 * returns pcb_true on success
 */
static pcb_bool UndoNetlistChange(UndoListTypePtr Entry)
{
	NetlistChangeTypePtr l = &Entry->Data.NetlistChange;
	unsigned int i, j;
	pcb_lib_t *lib, *saved;

	lib = l->lib;
	saved = l->old;

	/* iterate over each net */
	for (i = 0; i < lib->MenuN; i++) {
		free(lib->Menu[i].Name);
		free(lib->Menu[i].directory);
		free(lib->Menu[i].Style);

		/* iterate over each pin on the net */
		for (j = 0; j < lib->Menu[i].EntryN; j++) {
			if (!lib->Menu[i].Entry[j].ListEntry_dontfree)
				free((char*)lib->Menu[i].Entry[j].ListEntry);

			free((char*)lib->Menu[i].Entry[j].Package);
			free((char*)lib->Menu[i].Entry[j].Value);
			free((char*)lib->Menu[i].Entry[j].Description);
		}
	}

	free(lib->Menu);

	*lib = *saved;

	pcb_netlist_changed(0);
	return pcb_true;
}

/* ---------------------------------------------------------------------------
 * undo of any 'hard to recover' operation
 *
 * returns the bitfield for the types of operations that were undone
 */
int pcb_undo(pcb_bool draw)
{
	UndoListTypePtr ptr;
	int Types = 0;
	int unique;
	pcb_bool error_undoing = pcb_false;

	unique = conf_core.editor.unique_names;
	conf_force_set_bool(conf_core.editor.unique_names, 0);

	andDraw = draw;

	if (Serial == 0) {
		pcb_message(PCB_MSG_DEFAULT, _("ERROR: Attempt to pcb_undo() with Serial == 0\n" "       Please save your work and report this bug.\n"));
		return 0;
	}

	if (UndoN == 0) {
		pcb_message(PCB_MSG_DEFAULT, _("Nothing to undo - buffer is empty\n"));
		return 0;
	}

	Serial--;

	ptr = &UndoList[UndoN - 1];

	if (ptr->Serial > Serial) {
		pcb_message(PCB_MSG_DEFAULT, _("ERROR: Bad undo serial number %d in undo stack - expecting %d or lower\n"
							"       Please save your work and report this bug.\n"), ptr->Serial, Serial);

	/* It is likely that the serial number got corrupted through some bad
		 * use of the pcb_undo_save_serial() / pcb_undo_restore_serial() APIs.
		 *
		 * Reset the serial number to be consistent with that of the last
		 * operation on the undo stack in the hope that this might clear
		 * the problem and  allow the user to hit Undo again.
		 */
		Serial = ptr->Serial + 1;
		return 0;
	}

	pcb_undo_lock();										/* lock undo module to prevent from loops */

	/* Loop over all entries with the correct serial number */
	for (; UndoN && ptr->Serial == Serial; ptr--, UndoN--, RedoN++) {
		int undid = PerformUndo(ptr);
		if (undid == 0)
			error_undoing = pcb_true;
		Types |= undid;
	}

	pcb_undo_unlock();

	if (error_undoing)
		pcb_message(PCB_MSG_DEFAULT, _("ERROR: Failed to undo some operations\n"));

	if (Types && andDraw)
		pcb_draw();

	/* restore the unique flag setting */
	conf_force_set_bool(conf_core.editor.unique_names, unique);

	return Types;
}

static int PerformUndo(UndoListTypePtr ptr)
{
	switch (ptr->Type) {
	case PCB_UNDO_CHANGENAME:
		if (UndoChangeName(ptr))
			return (PCB_UNDO_CHANGENAME);
		break;

	case PCB_UNDO_CHANGEPINNUM:
		if (UndoChangePinnum(ptr))
			return (PCB_UNDO_CHANGEPINNUM);
		break;

	case PCB_UNDO_CREATE:
		if (UndoCopyOrCreate(ptr))
			return (PCB_UNDO_CREATE);
		break;

	case PCB_UNDO_MOVE:
		if (UndoMove(ptr))
			return (PCB_UNDO_MOVE);
		break;

	case PCB_UNDO_REMOVE:
		if (UndoRemove(ptr))
			return (PCB_UNDO_REMOVE);
		break;

	case PCB_UNDO_REMOVE_POINT:
		if (UndoRemovePoint(ptr))
			return (PCB_UNDO_REMOVE_POINT);
		break;

	case PCB_UNDO_INSERT_POINT:
		if (UndoInsertPoint(ptr))
			return (PCB_UNDO_INSERT_POINT);
		break;

	case PCB_UNDO_REMOVE_CONTOUR:
		if (UndoRemoveContour(ptr))
			return (PCB_UNDO_REMOVE_CONTOUR);
		break;

	case PCB_UNDO_INSERT_CONTOUR:
		if (UndoInsertContour(ptr))
			return (PCB_UNDO_INSERT_CONTOUR);
		break;

	case PCB_UNDO_ROTATE:
		if (UndoRotate(ptr))
			return (PCB_UNDO_ROTATE);
		break;

	case PCB_UNDO_CLEAR:
		if (UndoClearPoly(ptr))
			return (PCB_UNDO_CLEAR);
		break;

	case PCB_UNDO_MOVETOLAYER:
		if (UndoMoveToLayer(ptr))
			return (PCB_UNDO_MOVETOLAYER);
		break;

	case PCB_UNDO_FLAG:
		if (UndoFlag(ptr))
			return (PCB_UNDO_FLAG);
		break;

	case PCB_UNDO_CHANGESIZE:
		if (UndoChangeSize(ptr))
			return (PCB_UNDO_CHANGESIZE);
		break;

	case PCB_UNDO_CHANGECLEARSIZE:
		if (UndoChangeClearSize(ptr))
			return (PCB_UNDO_CHANGECLEARSIZE);
		break;

	case PCB_UNDO_CHANGEMASKSIZE:
		if (UndoChangeMaskSize(ptr))
			return (PCB_UNDO_CHANGEMASKSIZE);
		break;

	case PCB_UNDO_CHANGE2NDSIZE:
		if (UndoChange2ndSize(ptr))
			return (PCB_UNDO_CHANGE2NDSIZE);
		break;

	case PCB_UNDO_CHANGEANGLES:
		if (UndoChangeAngles(ptr))
			return (PCB_UNDO_CHANGEANGLES);
		break;

	case PCB_UNDO_CHANGERADII:
		if (UndoChangeRadii(ptr))
			return (PCB_UNDO_CHANGERADII);
		break;

	case PCB_UNDO_LAYERCHANGE:
		if (UndoLayerChange(ptr))
			return (PCB_UNDO_LAYERCHANGE);
		break;

	case PCB_UNDO_NETLISTCHANGE:
		if (UndoNetlistChange(ptr))
			return (PCB_UNDO_NETLISTCHANGE);
		break;

	case PCB_UNDO_MIRROR:
		if (UndoMirror(ptr))
			return (PCB_UNDO_MIRROR);
		break;
	}
	return 0;
}

/* ---------------------------------------------------------------------------
 * redo of any 'hard to recover' operation
 *
 * returns the number of operations redone
 */
int pcb_redo(pcb_bool draw)
{
	UndoListTypePtr ptr;
	int Types = 0;
	pcb_bool error_undoing = pcb_false;

	andDraw = draw;

	if (RedoN == 0) {
		pcb_message(PCB_MSG_DEFAULT, _("Nothing to redo. Perhaps changes have been made since last undo\n"));
		return 0;
	}

	ptr = &UndoList[UndoN];

	if (ptr->Serial < Serial) {
		pcb_message(PCB_MSG_DEFAULT, _("ERROR: Bad undo serial number %d in redo stack - expecting %d or higher\n"
							"       Please save your work and report this bug.\n"), ptr->Serial, Serial);

		/* It is likely that the serial number got corrupted through some bad
		 * use of the pcb_undo_save_serial() / pcb_undo_restore_serial() APIs.
		 *
		 * Reset the serial number to be consistent with that of the first
		 * operation on the redo stack in the hope that this might clear
		 * the problem and  allow the user to hit Redo again.
		 */
		Serial = ptr->Serial;
		return 0;
	}

	pcb_undo_lock();										/* lock undo module to prevent from loops */

	/* and loop over all entries with the correct serial number */
	for (; RedoN && ptr->Serial == Serial; ptr++, UndoN++, RedoN--) {
		int undid = PerformUndo(ptr);
		if (undid == 0)
			error_undoing = pcb_true;
		Types |= undid;
	}

	/* Make next serial number current */
	Serial++;

	pcb_undo_unlock();

	if (error_undoing)
		pcb_message(PCB_MSG_DEFAULT, _("ERROR: Failed to redo some operations\n"));

	if (Types && andDraw)
		pcb_draw();

	return Types;
}

/* ---------------------------------------------------------------------------
 * restores the serial number of the undo list
 */
void pcb_undo_restore_serial(void)
{
	if (added_undo_between_increment_and_restore)
		pcb_message(PCB_MSG_DEFAULT, _("ERROR: Operations were added to the Undo stack with an incorrect serial number\n"));
	between_increment_and_restore = pcb_false;
	added_undo_between_increment_and_restore = pcb_false;
	Serial = SavedSerial;
}

/* ---------------------------------------------------------------------------
 * saves the serial number of the undo list
 */
void pcb_undo_save_serial(void)
{
	Bumped = pcb_false;
	between_increment_and_restore = pcb_false;
	added_undo_between_increment_and_restore = pcb_false;
	SavedSerial = Serial;
}

/* ---------------------------------------------------------------------------
 * increments the serial number of the undo list
 * it's not done automatically because some operations perform more
 * than one request with the same serial #
 */
void pcb_undo_inc_serial(void)
{
	if (!Locked) {
		/* Set the changed flag if anything was added prior to this bump */
		if (UndoN > 0 && UndoList[UndoN - 1].Serial == Serial)
			pcb_board_set_changed_flag(pcb_true);
		Serial++;
		Bumped = pcb_true;
		between_increment_and_restore = pcb_true;
	}
}

/* ---------------------------------------------------------------------------
 * releases memory of the undo- and remove list
 */
void pcb_undo_clear_list(pcb_bool Force)
{
	UndoListTypePtr undo;

	if (UndoN && (Force || gui->confirm_dialog("OK to clear 'undo' buffer?", 0))) {
		/* release memory allocated by objects in undo list */
		for (undo = UndoList; UndoN; undo++, UndoN--) {
			if ((undo->Type == PCB_UNDO_CHANGENAME) || (undo->Type == PCB_UNDO_CHANGEPINNUM))
				free(undo->Data.ChangeName.Name);
		}
		free(UndoList);
		UndoList = NULL;
		if (RemoveList) {
			pcb_data_free(RemoveList);
			free(RemoveList);
			RemoveList = NULL;
		}

		/* reset some counters */
		UndoN = UndoMax = RedoN = 0;
	}

	/* reset counter in any case */
	Serial = 1;
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of clearpoly objects
 */
void pcb_undo_add_obj_to_clear_poly(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_bool clear)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_CLEAR, PCB_OBJECT_ID(Ptr3), Type);
		undo->Data.ClearPoly.Clear = clear;
		undo->Data.ClearPoly.Layer = (pcb_layer_t *) Ptr1;
	}
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of mirrored objects
 */
void pcb_undo_add_obj_to_mirror(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t yoff)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_MIRROR, PCB_OBJECT_ID(Ptr3), Type);
		undo->Data.Move.DY = yoff;
	}
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of rotated objects
 */
void pcb_undo_add_obj_to_rotate(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t CenterX, pcb_coord_t CenterY, pcb_uint8_t Steps)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_ROTATE, PCB_OBJECT_ID(Ptr3), Type);
		undo->Data.Rotate.CenterX = CenterX;
		undo->Data.Rotate.CenterY = CenterY;
		undo->Data.Rotate.Steps = Steps;
	}
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of removed objects and removes it from
 * the current PCB
 */
void pcb_undo_move_obj_to_remove(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	if (Locked)
		return;

	if (!RemoveList)
		RemoveList = pcb_buffer_new();

	GetUndoSlot(PCB_UNDO_REMOVE, PCB_OBJECT_ID(Ptr3), Type);
	pcb_move_obj_to_buffer(RemoveList, PCB->Data, Type, Ptr1, Ptr2, Ptr3);
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of removed polygon/... points
 */
void pcb_undo_add_obj_to_remove_point(int Type, void *Ptr1, void *Ptr2, pcb_cardinal_t index)
{
	UndoListTypePtr undo;
	pcb_polygon_t *polygon = (pcb_polygon_t *) Ptr2;
	pcb_cardinal_t hole;
	pcb_bool last_in_contour = pcb_false;

	if (!Locked) {
		switch (Type) {
		case PCB_TYPE_POLYGON_POINT:
			{
				/* save the ID of the parent object; else it will be
				 * impossible to recover the point
				 */
				undo = GetUndoSlot(PCB_UNDO_REMOVE_POINT, PCB_OBJECT_ID(polygon), PCB_TYPE_POLYGON);
				undo->Data.RemovedPoint.X = polygon->Points[index].X;
				undo->Data.RemovedPoint.Y = polygon->Points[index].Y;
				undo->Data.RemovedPoint.ID = polygon->Points[index].ID;
				undo->Data.RemovedPoint.Index = index;

				/* Check whether this point was at the end of its contour.
				 * If so, we need to flag as such when re-adding the point
				 * so it goes back in the correct place
				 */
				for (hole = 0; hole < polygon->HoleIndexN; hole++)
					if (index == polygon->HoleIndex[hole] - 1)
						last_in_contour = pcb_true;
				if (index == polygon->PointN - 1)
					last_in_contour = pcb_true;
				undo->Data.RemovedPoint.last_in_contour = last_in_contour;
			}
			break;
		}
	}
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of inserted polygon/... points
 */
void pcb_undo_add_obj_to_insert_point(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	if (!Locked)
		GetUndoSlot(PCB_UNDO_INSERT_POINT, PCB_OBJECT_ID(Ptr3), Type);
}

static void CopyObjectToUndoList(int undo_type, int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	UndoListTypePtr undo;
	pcb_any_obj_t *copy;

	if (Locked)
		return;

	if (!RemoveList)
		RemoveList = pcb_buffer_new();

	undo = GetUndoSlot(undo_type, PCB_OBJECT_ID(Ptr2), Type);
	copy = (pcb_any_obj_t *) pcb_copy_obj_to_buffer(RemoveList, PCB->Data, Type, Ptr1, Ptr2, Ptr3);
	undo->Data.CopyID = copy->ID;
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of removed contours
 * (Actually just takes a copy of the whole polygon to restore)
 */
void pcb_undo_add_obj_to_remove_contour(int Type, pcb_layer_t * Layer, pcb_polygon_t * Polygon)
{
	CopyObjectToUndoList(PCB_UNDO_REMOVE_CONTOUR, Type, Layer, Polygon, NULL);
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of insert contours
 * (Actually just takes a copy of the whole polygon to restore)
 */
void pcb_undo_add_obj_to_insert_contour(int Type, pcb_layer_t * Layer, pcb_polygon_t * Polygon)
{
	CopyObjectToUndoList(PCB_UNDO_INSERT_CONTOUR, Type, Layer, Polygon, NULL);
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of moved objects
 */
void pcb_undo_add_obj_to_move(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t DX, pcb_coord_t DY)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_MOVE, PCB_OBJECT_ID(Ptr3), Type);
		undo->Data.Move.DX = DX;
		undo->Data.Move.DY = DY;
	}
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of objects with changed names
 */
void pcb_undo_add_obj_to_change_name(int Type, void *Ptr1, void *Ptr2, void *Ptr3, char *OldName)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_CHANGENAME, PCB_OBJECT_ID(Ptr3), Type);
		undo->Data.ChangeName.Name = OldName;
	}
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of objects with changed pinnums
 */
void pcb_undo_add_obj_to_change_pinnum(int Type, void *Ptr1, void *Ptr2, void *Ptr3, char *OldName)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_CHANGEPINNUM, PCB_OBJECT_ID(Ptr3), Type);
		undo->Data.ChangeName.Name = OldName;
	}
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of objects moved to another layer
 */
void pcb_undo_add_obj_to_move_to_layer(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_MOVETOLAYER, PCB_OBJECT_ID(Ptr3), Type);
		undo->Data.MoveToLayer.OriginalLayer = GetLayerNumber(PCB->Data, (pcb_layer_t *) Ptr1);
	}
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of created objects
 */
void pcb_undo_add_obj_to_create(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	if (!Locked)
		GetUndoSlot(PCB_UNDO_CREATE, PCB_OBJECT_ID(Ptr3), Type);
	pcb_poly_clear_from_poly(PCB->Data, Type, Ptr1, Ptr2);
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of objects with flags changed
 */
void pcb_undo_add_obj_to_flag(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_FLAG, PCB_OBJECT_ID(Ptr2), Type);
		undo->Data.Flags = ((pcb_pin_t *) Ptr2)->Flags;
	}
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of objects with Size changes
 */
void pcb_undo_add_obj_to_size(int Type, void *ptr1, void *ptr2, void *ptr3)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_CHANGESIZE, PCB_OBJECT_ID(ptr2), Type);
		switch (Type) {
		case PCB_TYPE_PIN:
		case PCB_TYPE_VIA:
			undo->Data.Size = ((pcb_pin_t *) ptr2)->Thickness;
			break;
		case PCB_TYPE_LINE:
		case PCB_TYPE_ELEMENT_LINE:
			undo->Data.Size = ((pcb_line_t *) ptr2)->Thickness;
			break;
		case PCB_TYPE_TEXT:
		case PCB_TYPE_ELEMENT_NAME:
			undo->Data.Size = ((pcb_text_t *) ptr2)->Scale;
			break;
		case PCB_TYPE_PAD:
			undo->Data.Size = ((pcb_pad_t *) ptr2)->Thickness;
			break;
		case PCB_TYPE_ARC:
		case PCB_TYPE_ELEMENT_ARC:
			undo->Data.Size = ((pcb_arc_t *) ptr2)->Thickness;
			break;
		}
	}
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of objects with Size changes
 */
void pcb_undo_add_obj_to_clear_size(int Type, void *ptr1, void *ptr2, void *ptr3)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_CHANGECLEARSIZE, PCB_OBJECT_ID(ptr2), Type);
		switch (Type) {
		case PCB_TYPE_PIN:
		case PCB_TYPE_VIA:
			undo->Data.Size = ((pcb_pin_t *) ptr2)->Clearance;
			break;
		case PCB_TYPE_LINE:
			undo->Data.Size = ((pcb_line_t *) ptr2)->Clearance;
			break;
		case PCB_TYPE_PAD:
			undo->Data.Size = ((pcb_pad_t *) ptr2)->Clearance;
			break;
		case PCB_TYPE_ARC:
			undo->Data.Size = ((pcb_arc_t *) ptr2)->Clearance;
			break;
		}
	}
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of objects with Size changes
 */
void pcb_undo_add_obj_to_mask_size(int Type, void *ptr1, void *ptr2, void *ptr3)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_CHANGEMASKSIZE, PCB_OBJECT_ID(ptr2), Type);
		switch (Type) {
		case PCB_TYPE_PIN:
		case PCB_TYPE_VIA:
			undo->Data.Size = ((pcb_pin_t *) ptr2)->Mask;
			break;
		case PCB_TYPE_PAD:
			undo->Data.Size = ((pcb_pad_t *) ptr2)->Mask;
			break;
		}
	}
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of objects with 2ndSize changes
 */
void pcb_undo_add_obj_to_2nd_size(int Type, void *ptr1, void *ptr2, void *ptr3)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_CHANGE2NDSIZE, PCB_OBJECT_ID(ptr2), Type);
		if (Type == PCB_TYPE_PIN || Type == PCB_TYPE_VIA)
			undo->Data.Size = ((pcb_pin_t *) ptr2)->DrillingHole;
	}
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of changed angles.  Note that you must
 * call this before changing the angles, passing the new start/delta.
 */
void pcb_undo_add_obj_to_change_angles(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	UndoListTypePtr undo;
	pcb_arc_t *a = (pcb_arc_t *) Ptr3;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_CHANGEANGLES, PCB_OBJECT_ID(Ptr3), Type);
		undo->Data.AngleChange.angle[0] = a->StartAngle;
		undo->Data.AngleChange.angle[1] = a->Delta;
	}
}

/* ---------------------------------------------------------------------------
 * adds an object to the list of changed radii.  Note that you must
 * call this before changing the radii, passing the new width/height.
 */
void pcb_undo_add_obj_to_change_radii(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	UndoListTypePtr undo;
	pcb_arc_t *a = (pcb_arc_t *) Ptr3;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_CHANGERADII, PCB_OBJECT_ID(Ptr3), Type);
		undo->Data.Move.DX = a->Width;
		undo->Data.Move.DY = a->Height;
	}
}

/* ---------------------------------------------------------------------------
 * adds a layer change (new, delete, move) to the undo list.
 */
void pcb_undo_add_layer_change(int old_index, int new_index)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_LAYERCHANGE, 0, 0);
		undo->Data.LayerChange.old_index = old_index;
		undo->Data.LayerChange.new_index = new_index;
	}
}

/* ---------------------------------------------------------------------------
 * adds a netlist change to the undo list
 */
void pcb_undo_add_netlist_lib(pcb_lib_t *lib)
{
	UndoListTypePtr undo;
	unsigned int i, j;
	pcb_lib_t *old;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_NETLISTCHANGE, 0, 0);
		/* keep track of where the data needs to go */
		undo->Data.NetlistChange.lib = lib;

		/* and what the old data is that we'll need to restore */
		undo->Data.NetlistChange.old = (pcb_lib_t *) malloc(sizeof(pcb_lib_t *));
		old = undo->Data.NetlistChange.old;
		old->MenuN = lib->MenuN;
		old->MenuMax = lib->MenuMax;
		old->Menu = (pcb_lib_menu_t *) malloc(old->MenuMax * sizeof(pcb_lib_menu_t));
		if (old->Menu == NULL) {
			fprintf(stderr, "malloc() failed in AddNetlistLibToUndoList\n");
			exit(1);
		}

		/* iterate over each net */
		for (i = 0; i < lib->MenuN; i++) {
			old->Menu[i].EntryN = lib->Menu[i].EntryN;
			old->Menu[i].EntryMax = lib->Menu[i].EntryMax;

			old->Menu[i].Name = lib->Menu[i].Name ? pcb_strdup(lib->Menu[i].Name) : NULL;

			old->Menu[i].directory = lib->Menu[i].directory ? pcb_strdup(lib->Menu[i].directory) : NULL;

			old->Menu[i].Style = lib->Menu[i].Style ? pcb_strdup(lib->Menu[i].Style) : NULL;


			old->Menu[i].Entry = (pcb_lib_entry_t *) malloc(old->Menu[i].EntryMax * sizeof(pcb_lib_entry_t));
			if (old->Menu[i].Entry == NULL) {
				fprintf(stderr, "malloc() failed in AddNetlistLibToUndoList\n");
				exit(1);
			}

			/* iterate over each pin on the net */
			for (j = 0; j < lib->Menu[i].EntryN; j++) {

				old->Menu[i].Entry[j].ListEntry = lib->Menu[i].Entry[j].ListEntry ? pcb_strdup(lib->Menu[i].Entry[j].ListEntry) : NULL;
				old->Menu[i].Entry[j].ListEntry_dontfree = 0;

				old->Menu[i].Entry[j].Package = lib->Menu[i].Entry[j].Package ? pcb_strdup(lib->Menu[i].Entry[j].Package) : NULL;

				old->Menu[i].Entry[j].Value = lib->Menu[i].Entry[j].Value ? pcb_strdup(lib->Menu[i].Entry[j].Value) : NULL;

				old->Menu[i].Entry[j].Description =
					lib->Menu[i].Entry[j].Description ? pcb_strdup(lib->Menu[i].Entry[j].Description) : NULL;


			}
		}

	}
}

/* ---------------------------------------------------------------------------
 * set lock flag
 */
void pcb_undo_lock(void)
{
	Locked = pcb_true;
}

/* ---------------------------------------------------------------------------
 * reset lock flag
 */
void pcb_undo_unlock(void)
{
	Locked = pcb_false;
}

/* ---------------------------------------------------------------------------
 * return undo lock state
 */
pcb_bool pcb_undoing(void)
{
	return (Locked);
}

#ifndef NDEBUG
static const char *undo_type2str(int type)
{
	static char buff[32];
	switch(type) {
		case PCB_UNDO_CHANGENAME: return "changename";
		case PCB_UNDO_MOVE: return "move";
		case PCB_UNDO_REMOVE: return "remove";
		case PCB_UNDO_REMOVE_POINT: return "remove_point";
		case PCB_UNDO_INSERT_POINT: return "insert_point";
		case PCB_UNDO_REMOVE_CONTOUR: return "remove_contour";
		case PCB_UNDO_INSERT_CONTOUR: return "insert_contour";
		case PCB_UNDO_ROTATE: return "rotate";
		case PCB_UNDO_CREATE: return "create";
		case PCB_UNDO_MOVETOLAYER: return "movetolayer";
		case PCB_UNDO_FLAG: return "flag";
		case PCB_UNDO_CHANGESIZE: return "changesize";
		case PCB_UNDO_CHANGE2NDSIZE: return "change2ndsize";
		case PCB_UNDO_MIRROR: return "mirror";
		case PCB_UNDO_CHANGECLEARSIZE: return "chngeclearsize";
		case PCB_UNDO_CHANGEMASKSIZE: return "changemasksize";
		case PCB_UNDO_CHANGEANGLES: return "changeangles";
		case PCB_UNDO_CHANGERADII: return "changeradii";
		case PCB_UNDO_LAYERCHANGE: return "layerchange";
		case PCB_UNDO_CLEAR: return "clear";
		case PCB_UNDO_NETLISTCHANGE: return "netlistchange";
		case PCB_UNDO_CHANGEPINNUM: return "changepinnum";
	}
	sprintf(buff, "Unknown %d", type);
	return buff;
}

void undo_dump()
{
	size_t n;
	int last_serial = -2;
	for(n = 0; n < UndoN; n++) {
		if (last_serial != UndoList[n].Serial) {
			printf("--- serial=%d\n", UndoList[n].Serial);
			last_serial = UndoList[n].Serial;
		}
		printf(" type=%s kind=%d ID=%d\n", undo_type2str(UndoList[n].Type), UndoList[n].Kind, UndoList[n].ID);
	}
}

#endif

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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
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
#include <libuundo/uundo.h>

#include "board.h"
#include "change.h"
#include "data.h"
#include "draw.h"
#include "move.h"
#include <librnd/core/error.h>
#include "extobj.h"
#include "insert.h"
#include "polygon.h"
#include "remove.h"
#include "rotate.h"
#include "search.h"
#include "undo.h"
#include "undo_old.h"
#include "flag_str.h"
#include "conf_core.h"
#include <librnd/core/compat_misc.h>
#include "netlist.h"

#include "obj_poly_draw.h"
#include "obj_subc_parent.h"
#include "obj_term.h"

#include "brave.h"

#define Locked pcb_undoing()

/*** undo_old_str ***/

typedef struct {              /* information about a change command */
	char *Name;
} ChangeNameType, *ChangeNameTypePtr;

typedef struct {              /* information about a move command */
	rnd_coord_t DX, DY;         /* movement vector */
} MoveType, *MoveTypePtr;

typedef struct {
	long p_subc_id;             /* parent subc ID (0 if parent is not a subc) */
	int p_subc_layer;           /* parent subc layer index, for layer objects */
} Removed;

typedef struct {              /* information about removed polygon points */
	rnd_coord_t X, Y;           /* data */
	int ID;
	rnd_cardinal_t Index;       /* index in a polygons array of points */
	rnd_bool last_in_contour;   /* Whether the point was the last in its contour */
} RemovedPointType, *RemovedPointTypePtr;

typedef struct {                 /* information about rotation */
	rnd_coord_t CenterX, CenterY;  /* center of rotation */
	rnd_cardinal_t Steps;          /* number of steps */
} RotateType, *RotateTypePtr;

typedef struct {                 /* information about moves between layers */
	rnd_cardinal_t OriginalLayer;  /* the index of the original layer */
} MoveToLayer;

typedef struct {                 /* information about poly clear/restore */
	rnd_bool Clear;                /* rnd_true was clear, rnd_false was restore */
	pcb_layer_t *Layer;
} ClearPolyType, *ClearPolyTypePtr;

typedef struct {
	rnd_angle_t angle[2];
} AngleChangeType;

typedef struct {                 /* information about netlist lib changes */
	pcb_net_t *old;
	pcb_net_t *lib;
} NetlistChangeType, *NetlistChangeTypePtr;

typedef struct {                 /* holds information about an operation */
	int Serial;                    /* serial number of operation */
	int Type;                      /* type of operation */
	pcb_objtype_t Kind;            /* type of object with given ID */
	long int ID;                   /* object ID */
	union {                        /* some additional information */
		ChangeNameType ChangeName;
		MoveType Move;
		Removed Removed;
		RemovedPointType RemovedPoint;
		RotateType Rotate;
		MoveToLayer MoveToLayer;
		pcb_flag_t Flags;
		rnd_coord_t Size;
		ClearPolyType ClearPoly;
		NetlistChangeType NetlistChange;
		long int CopyID;
		AngleChangeType AngleChange;
		rnd_angle_t Angle;
	} Data;
} UndoListType, *UndoListTypePtr;


/*** undo_old */


#define DRAW_FLAGS (PCB_FLAG_RAT | PCB_FLAG_SELECTED | PCB_FLAG_HIDENAME | PCB_FLAG_HOLE | PCB_FLAG_OCTAGON | PCB_FLAG_FOUND | PCB_FLAG_CLEARLINE | PCB_FLAG_CLEARPOLY | PCB_FLAG_CLEARPOLYPOLY)
#define CLIP_FLAGS (PCB_FLAG_CLEARLINE | PCB_FLAG_CLEARPOLY | PCB_FLAG_CLEARPOLYPOLY)

static rnd_bool UndoRotate90(UndoListTypePtr);
static rnd_bool UndoRotate(UndoListTypePtr);
static rnd_bool UndoChangeName(UndoListTypePtr);
static rnd_bool UndoCopyOrCreate(UndoListTypePtr);
static rnd_bool UndoMove(UndoListTypePtr);
static rnd_bool UndoRemove(UndoListTypePtr);
static rnd_bool UndoRemovePoint(UndoListTypePtr);
static rnd_bool UndoInsertPoint(UndoListTypePtr);
static rnd_bool UndoRemoveContour(UndoListTypePtr);
static rnd_bool UndoInsertContour(UndoListTypePtr);
static rnd_bool UndoMoveToLayer(UndoListTypePtr);
static rnd_bool UndoFlag(UndoListTypePtr);
static rnd_bool UndoOtherSide(UndoListTypePtr);
static rnd_bool UndoChangeSize(UndoListTypePtr);
static rnd_bool UndoChange2ndSize(UndoListTypePtr);
static rnd_bool UndoChangeAngles(UndoListTypePtr);
static rnd_bool UndoChangeRadii(UndoListTypePtr);
static rnd_bool UndoChangeClearSize(UndoListTypePtr);
static rnd_bool UndoClearPoly(UndoListTypePtr);

#define PCB_OBJECT_ID(p) (((pcb_any_obj_t *) p)->ID)

static void pcb_undo_old_free(void *udata);
static int pcb_undo_old_undo(void *udata);

void pcb_undo_old_print(void *udata, char *dst, size_t dst_len)
{
	UndoListType *slot = udata;
#ifndef NDEBUG
	const char *res = undo_type2str(slot->Type);
	rnd_snprintf(dst, dst_len, "%s ser=%d id=%d", res, slot->Serial, slot->ID);
#else
	sprintf(dst, "%d", slot->Type);
#endif

}

static const uundo_oper_t pcb_undo_old_oper = {
	"core-old",
	pcb_undo_old_free,
	pcb_undo_old_undo,
	pcb_undo_old_undo, /* redo is the same as undo */
	pcb_undo_old_print
};

static UndoListType *GetUndoSlot(int CommandType, long int ID, pcb_objtype_t Kind)
{
	UndoListType *slot = pcb_undo_alloc(PCB, &pcb_undo_old_oper, sizeof(UndoListType));

	memset(slot, 0, sizeof(UndoListType));
	slot->Type = CommandType;
	slot->ID = ID;
	slot->Kind = Kind;

	return slot;
}


/* redraws the recovered object */
static void DrawRecoveredObject(pcb_any_obj_t *obj)
{
	pcb_draw_obj(obj);
}

/* recovers an object from a 90 deg 'rotate' operation returns rnd_true if
   anything has been recovered */
static rnd_bool UndoRotate90(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;

	/* lookup entry by it's ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_OBJ_VOID) {
		if (type == PCB_OBJ_LINE_POINT) /* rubber band, needs ptr3 */
			pcb_point_rotate90(ptr3, Entry->Data.Rotate.CenterX, Entry->Data.Rotate.CenterY, (4 - Entry->Data.Rotate.Steps) & 0x03);
		else
			pcb_obj_rotate90(PCB, ptr2, Entry->Data.Rotate.CenterX, Entry->Data.Rotate.CenterY, (4 - Entry->Data.Rotate.Steps) & 0x03);
		Entry->Data.Rotate.Steps = (4 - Entry->Data.Rotate.Steps) & 0x03;
		return rnd_true;
	}
	return rnd_false;
}

/* recovers an object from an arbitrary angle 'rotate' operation
   returns rnd_true if anything has been recovered */
static rnd_bool UndoRotate(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;

	/* lookup entry by it's ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_OBJ_VOID) {
		pcb_obj_rotate(PCB, ptr2, Entry->Data.Rotate.CenterX, Entry->Data.Rotate.CenterY, -(Entry->Data.Angle));
		Entry->Data.Angle = -(Entry->Data.Angle);
		return rnd_true;
	}
	return rnd_false;
}

/* recovers an object from a clear/restore poly operation
   returns rnd_true if anything has been recovered */
static rnd_bool UndoClearPoly(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;

	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_OBJ_VOID) {
		if (Entry->Data.ClearPoly.Clear)
			pcb_poly_restore_to_poly(PCB->Data, type, Entry->Data.ClearPoly.Layer, ptr3);
		else
			pcb_poly_clear_from_poly(PCB->Data, type, Entry->Data.ClearPoly.Layer, ptr3);
		Entry->Data.ClearPoly.Clear = !Entry->Data.ClearPoly.Clear;
		return rnd_true;
	}
	return rnd_false;
}

/* recovers an object from a 'change name' operation
   returns rnd_true if anything has been recovered */
static rnd_bool UndoChangeName(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;

	/* lookup entry by it's ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_OBJ_VOID) {
		Entry->Data.ChangeName.Name = (char *) (pcb_chg_obj_name(type, ptr1, ptr2, ptr3, Entry->Data.ChangeName.Name));
		return rnd_true;
	}
	return rnd_false;
}

/* recovers an object from a ChangeAngles change operation */
static rnd_bool UndoChangeAngles(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	double old_sa, old_da;

	/* lookup entry by ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type == PCB_OBJ_ARC) {
		pcb_layer_t *Layer = (pcb_layer_t *) ptr1;
		pcb_arc_t *a = (pcb_arc_t *) ptr2;
		pcb_poly_restore_to_poly(PCB->Data, a->type, ptr1, ptr2);
		rnd_r_delete_entry(Layer->arc_tree, (rnd_box_t *) a);
		old_sa = a->StartAngle;
		old_da = a->Delta;
		if (pcb_undo_and_draw)
			pcb_erase_obj(type, Layer, a);
		a->StartAngle = Entry->Data.AngleChange.angle[0];
		a->Delta = Entry->Data.AngleChange.angle[1];
		pcb_arc_bbox(a);
		rnd_r_insert_entry(Layer->arc_tree, (rnd_box_t *) a);
		pcb_poly_clear_from_poly(PCB->Data, a->type, ptr1, ptr2);
		Entry->Data.AngleChange.angle[0] = old_sa;
		Entry->Data.AngleChange.angle[1] = old_da;
		pcb_draw_obj((pcb_any_obj_t *)a);
		return rnd_true;
	}
	return rnd_false;
}

/* recovers an object from a ChangeRadii change operation */
static rnd_bool UndoChangeRadii(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	rnd_coord_t old_w, old_h;

	/* lookup entry by ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type == PCB_OBJ_ARC) {
		pcb_layer_t *Layer = (pcb_layer_t *) ptr1;
		pcb_arc_t *a = (pcb_arc_t *) ptr2;
		rnd_r_delete_entry(Layer->arc_tree, (rnd_box_t *) a);
		old_w = a->Width;
		old_h = a->Height;
		if (pcb_undo_and_draw)
			pcb_erase_obj(type, Layer, a);
		a->Width = Entry->Data.Move.DX;
		a->Height = Entry->Data.Move.DY;
		pcb_arc_bbox(a);
		rnd_r_insert_entry(Layer->arc_tree, (rnd_box_t *) a);
		Entry->Data.Move.DX = old_w;
		Entry->Data.Move.DY = old_h;
		pcb_draw_obj((pcb_any_obj_t *)a);
		return rnd_true;
	}
	return rnd_false;
}

/* recovers an object from a clearance size change operation */
static rnd_bool UndoChangeClearSize(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	rnd_coord_t swap;

	/* lookup entry by ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_OBJ_VOID) {
		swap = ((pcb_line_t *) ptr2)->Clearance;
		pcb_poly_restore_to_poly(PCB->Data, type, ptr1, ptr2);
		if (pcb_undo_and_draw)
			pcb_erase_obj(type, ptr1, ptr2);
		((pcb_line_t *) ptr2)->Clearance = Entry->Data.Size;
		pcb_poly_clear_from_poly(PCB->Data, type, ptr1, ptr2);
		Entry->Data.Size = swap;
		if (pcb_undo_and_draw)
			pcb_draw_obj((pcb_any_obj_t *)ptr2);
		return rnd_true;
	}
	return rnd_false;
}

/* recovers an object from a Size change operation */
static rnd_bool UndoChangeSize(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3, *ptr1e;
	int type;
	rnd_coord_t swap;

	/* lookup entry by ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	ptr1e = ptr1;

	if (type != PCB_OBJ_VOID) {
		/* Size change for lines and arcs can. Text has it's own mechanism */
		swap = ((pcb_line_t *) ptr2)->Thickness;
		pcb_poly_restore_to_poly(PCB->Data, type, ptr1, ptr2);
		if ((pcb_undo_and_draw) && (ptr1e != NULL))
			pcb_erase_obj(type, ptr1e, ptr2);
		((pcb_line_t *) ptr2)->Thickness = Entry->Data.Size;
		Entry->Data.Size = swap;
		pcb_poly_clear_from_poly(PCB->Data, type, ptr1, ptr2);
		if (pcb_undo_and_draw)
			pcb_draw_obj((pcb_any_obj_t *)ptr2);
		return rnd_true;
	}
	return rnd_false;
}

/* recovers an object from a 2nd Size change operation */
static rnd_bool UndoChange2ndSize(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	rnd_coord_t swap;

	/* lookup entry by ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);

	if (type == PCB_OBJ_TEXT) { /* thickness */
		swap = ((pcb_text_t *)ptr2)->thickness;
		pcb_text_pre((pcb_text_t *)ptr2);
		((pcb_text_t *)ptr2)->thickness = Entry->Data.Size;
		Entry->Data.Size = swap;
		pcb_text_post((pcb_text_t *)ptr2);
		return rnd_true;
	}
	return rnd_false;
}

/* recovers an object from a Size change operation */
static rnd_bool UndoChangeRot(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	double swap;
	rnd_bool ret = rnd_false;

	/* lookup entry by ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);

	if (pcb_undo_and_draw)
		pcb_draw_obj((pcb_any_obj_t *)ptr2);

	switch(type) {
		case PCB_OBJ_PSTK:
			swap = ((pcb_pstk_t *)ptr2)->rot;
			pcb_pstk_pre(((pcb_pstk_t *)ptr2));
			((pcb_pstk_t *)ptr2)->rot = Entry->Data.Size;
			Entry->Data.Size = swap;
			pcb_pstk_post(((pcb_pstk_t *)ptr2));
			ret = rnd_true;
			break;

		case PCB_OBJ_TEXT:
			swap = ((pcb_text_t *)ptr2)->rot;
			pcb_text_pre(((pcb_text_t *)ptr2));
			((pcb_text_t *)ptr2)->rot = Entry->Data.Size;
			Entry->Data.Size = swap;
			pcb_text_post(((pcb_text_t *)ptr2));
			ret = rnd_true;
			break;
	}

	if (pcb_undo_and_draw)
		pcb_draw_obj((pcb_any_obj_t *)ptr2);
	return ret;
}

/* recovers an object from a FLAG change operation */
static rnd_bool UndoFlag(UndoListTypePtr Entry)
{
	void *ptr1, *ptr1e, *ptr2, *ptr3, *txt_save;
	int type;
	pcb_flag_t swap;
	int must_redraw = 0, must_reclip = 0;
	unsigned long oldflg;

	/* lookup entry by ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_OBJ_VOID) {
		pcb_flag_t f1, f2;
		pcb_any_obj_t *obj = (pcb_any_obj_t *)ptr2;

		ptr1e = ptr1;
		swap = obj->Flags;

		f1 = pcb_flag_mask(obj->Flags, ~DRAW_FLAGS);
		f2 = pcb_flag_mask(Entry->Data.Flags, ~DRAW_FLAGS);
		if (!PCB_FLAG_EQ(f1, f2))
			must_redraw = 1;

		f1 = pcb_flag_mask(obj->Flags, ~CLIP_FLAGS);
		f2 = pcb_flag_mask(Entry->Data.Flags, ~CLIP_FLAGS);
		if (!PCB_FLAG_EQ(f1, f2))
			must_reclip = 1;

		if (pcb_undo_and_draw && must_redraw && (ptr1e != NULL))
			pcb_erase_obj(type, ptr1e, ptr2);

		if (must_reclip)
			pcb_poly_restore_to_poly(PCB->Data, obj->type, ptr1, ptr2);

		if (obj->type == PCB_OBJ_TEXT) {
			oldflg = obj->Flags.f;
			pcb_text_flagchg_pre((pcb_text_t *)obj, Entry->Data.Flags.f, &txt_save);
		}

		obj->Flags = Entry->Data.Flags;
		Entry->Data.Flags = swap;

		if (must_reclip)
			pcb_poly_clear_from_poly(PCB->Data, obj->type, ptr1, ptr2);

		if (obj->type == PCB_OBJ_TEXT)
			pcb_text_flagchg_post((pcb_text_t *)obj, oldflg, &txt_save);

		if (pcb_undo_and_draw && must_redraw)
			pcb_draw_obj((pcb_any_obj_t *)ptr2);
		return rnd_true;
	}
	rnd_message(RND_MSG_ERROR, "hace Internal error: Can't find ID %d type %08x\n", Entry->ID, Entry->Kind);
	rnd_message(RND_MSG_ERROR, "for UndoFlag Operation. Previous flags: %s\n", pcb_strflg_f2s(Entry->Data.Flags, 0, NULL, 0));
	return rnd_false;
}

/* recovers a subc from an other-side  operation
   returns rnd_true if anything has been recovered */
static rnd_bool UndoOtherSide(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;

	/* lookup entry by ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type == PCB_OBJ_SUBC) {
		pcb_subc_t *subc = (pcb_subc_t *)ptr3;
		if (pcb_undo_and_draw)
			EraseSubc(subc);
		pcb_subc_change_side(subc, Entry->Data.Move.DY);
		if (pcb_undo_and_draw)
			DrawSubc(subc);
		return rnd_true;
	}
	rnd_message(RND_MSG_ERROR, "hace Internal error: UndoOtherside on object type %x\n", type);
	return rnd_false;
}

static void get_subc_parent_data(long subcid, rnd_layer_id_t subclayer, pcb_subc_t **subc, pcb_data_t **data, void **ptr1)
{
	if (subcid > 0) { /* need to use a subc layer - putting back a floater */
		void *p1, *p2, *p3;
		if (pcb_search_obj_by_id(PCB->Data, &p1, &p2, &p3, subcid, PCB_OBJ_SUBC) != 0) {
			*subc = p2;
			if (subclayer < (*subc)->data->LayerN) {
				*data = (*subc)->data;
				*ptr1 = &(*data)->Layer[subclayer];
			}
		}
	}
}

/* recovers an object from a 'copy' or 'create' operation
   returns rnd_true if anything has been recovered */
static rnd_bool UndoCopyOrCreate(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	pcb_subc_t *subc = NULL;
	pcb_data_t *data = PCB->Data;

	/* lookup entry by it's ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_OBJ_VOID) {
		Removed *r = &Entry->Data.Removed;
		get_subc_parent_data(r->p_subc_id, r->p_subc_layer, &subc, &data, &ptr1);
		if (!pcb_removelist)
			pcb_removelist = pcb_buffer_new(NULL);
		if (pcb_undo_and_draw)
			pcb_erase_obj(type, ptr1, ptr2);
		/* in order to make this re-doable we move it to the pcb_removelist */
		pcb_move_obj_to_buffer(PCB, pcb_removelist, data, type, ptr1, ptr2, ptr3);

		if ((subc != NULL) && (((pcb_any_obj_t *)ptr2)->term != NULL))
			pcb_term_del(&subc->terminals, ((pcb_any_obj_t *)ptr2)->term, (pcb_any_obj_t *)ptr2);

		Entry->Type = PCB_UNDO_REMOVE;
		return rnd_true;
	}
	return rnd_false;
}

/* recovers an object from a 'move' operation
   returns rnd_true if anything has been recovered */
static rnd_bool UndoMove(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;

	/* lookup entry by it's ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_OBJ_VOID) {
		pcb_move_obj(type, ptr1, ptr2, ptr3, -Entry->Data.Move.DX, -Entry->Data.Move.DY);
		Entry->Data.Move.DX *= -1;
		Entry->Data.Move.DY *= -1;
		return rnd_true;
	}
	return rnd_false;
}

/* recovers an object from a 'remove' operation
   returns rnd_true if anything has been recovered */
static rnd_bool UndoRemove(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	Removed *r = &Entry->Data.Removed;
	pcb_data_t *data = PCB->Data;
	pcb_subc_t *subc = NULL;

	if (!(pcb_brave & PCB_BRAVE_NOCLIPBATCH))
		pcb_data_clip_inhibit_inc(PCB->Data);

	/* lookup entry by it's ID */
	type = pcb_search_obj_by_id(pcb_removelist, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_OBJ_VOID) {
		get_subc_parent_data(r->p_subc_id, r->p_subc_layer, &subc, &data, &ptr1);
		pcb_move_obj_to_buffer(PCB, data, pcb_removelist, type, ptr1, ptr2, ptr3);
		if (pcb_undo_and_draw)
			DrawRecoveredObject((pcb_any_obj_t *)ptr2);
		Entry->Type = PCB_UNDO_CREATE;

		{ /* if extended object's edit-obj is restored, make sure the corresponding subc is updated */
			pcb_any_obj_t *o = ptr2;
			pcb_extobj_float_pre(o);
			pcb_extobj_float_geo(o);
		}

		if (!(pcb_brave & PCB_BRAVE_NOCLIPBATCH))
			pcb_data_clip_inhibit_dec(PCB->Data, 1);

		return rnd_true;
	}

	if (!(pcb_brave & PCB_BRAVE_NOCLIPBATCH))
		pcb_data_clip_inhibit_dec(PCB->Data, 1);

	return rnd_false;
}

/* recovers an object from a 'move to another layer' operation
   returns rnd_true if anything has been recovered */
static rnd_bool UndoMoveToLayer(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	rnd_layer_id_t swap;

	/* lookup entry by it's ID */
	type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, Entry->ID, Entry->Kind);
	if (type != PCB_OBJ_VOID) {
		pcb_data_t *data;
		pcb_any_obj_t *o = ptr2;
		assert(o->parent_type == PCB_PARENT_LAYER);
		assert(o->parent.layer != NULL);
		assert(o->parent.layer->parent_type == PCB_PARENT_DATA);
		data = o->parent.layer->parent.data;
		swap = pcb_layer_id(data, o->parent.layer);
		pcb_move_obj_to_layer(type, ptr1, ptr2, ptr3, pcb_get_layer(data, Entry->Data.MoveToLayer.OriginalLayer), rnd_true);
		Entry->Data.MoveToLayer.OriginalLayer = swap;
		return rnd_true;
	}
	return rnd_false;
}

/* recovers a removed polygon point
   returns rnd_true on success */
static rnd_bool UndoRemovePoint(UndoListTypePtr Entry)
{
	pcb_layer_t *layer;
	pcb_poly_t *polygon;
	void *ptr3;
	int type;

	/* lookup entry (polygon not point was saved) by it's ID */
	assert(Entry->Kind == PCB_OBJ_POLY);
	type = pcb_search_obj_by_id(PCB->Data, (void **) &layer, (void **) &polygon, &ptr3, Entry->ID, Entry->Kind);
	switch (type) {
	case PCB_OBJ_POLY:						/* restore the removed point */
		{
			/* recover the point */
			if (pcb_undo_and_draw && layer->meta.real.vis)
				pcb_poly_invalidate_erase(polygon);
			pcb_insert_point_in_object(PCB_OBJ_POLY, layer, polygon,
														&Entry->Data.RemovedPoint.Index,
														Entry->Data.RemovedPoint.X,
														Entry->Data.RemovedPoint.Y, rnd_true, Entry->Data.RemovedPoint.last_in_contour);

			polygon->Points[Entry->Data.RemovedPoint.Index].ID = Entry->Data.RemovedPoint.ID;
			if (pcb_undo_and_draw && layer->meta.real.vis)
				pcb_poly_invalidate_draw(layer, polygon);
			Entry->Type = PCB_UNDO_INSERT_POINT;
			Entry->ID = Entry->Data.RemovedPoint.ID;
			Entry->Kind = PCB_OBJ_POLY_POINT;
			return rnd_true;
		}

	default:
		return rnd_false;
	}
}

/* recovers an inserted polygon point
   returns rnd_true on success */
static rnd_bool UndoInsertPoint(UndoListTypePtr Entry)
{
	pcb_layer_t *layer;
	pcb_poly_t *polygon;
	rnd_point_t *pnt;
	int type;
	rnd_cardinal_t point_idx;
	rnd_cardinal_t hole;
	rnd_bool last_in_contour = rnd_false;

	assert((long int)Entry->Kind == PCB_OBJ_POLY_POINT);
	/* lookup entry by it's ID */
	type = pcb_search_obj_by_id(PCB->Data, (void **) &layer, (void **) &polygon, (void **) &pnt, Entry->ID, Entry->Kind);
	switch (type) {
	case PCB_OBJ_POLY_POINT:			/* removes an inserted polygon point */
		{
			if (pcb_undo_and_draw && layer->meta.real.vis)
				pcb_poly_invalidate_erase(polygon);

			/* Check whether this point was at the end of its contour.
			 * If so, we need to flag as such when re-adding the point
			 * so it goes back in the correct place
			 */
			point_idx = pcb_poly_point_idx(polygon, pnt);
			for (hole = 0; hole < polygon->HoleIndexN; hole++)
				if (point_idx == polygon->HoleIndex[hole] - 1)
					last_in_contour = rnd_true;
			if (point_idx == polygon->PointN - 1)
				last_in_contour = rnd_true;
			Entry->Data.RemovedPoint.last_in_contour = last_in_contour;

			Entry->Data.RemovedPoint.X = pnt->X;
			Entry->Data.RemovedPoint.Y = pnt->Y;
			Entry->Data.RemovedPoint.ID = pnt->ID;
			Entry->ID = polygon->ID;
			Entry->Kind = PCB_OBJ_POLY;
			Entry->Type = PCB_UNDO_REMOVE_POINT;
			Entry->Data.RemovedPoint.Index = point_idx;
			pcb_destroy_object(PCB->Data, PCB_OBJ_POLY_POINT, layer, polygon, pnt);
			if (pcb_undo_and_draw && layer->meta.real.vis)
				pcb_poly_invalidate_draw(layer, polygon);
			return rnd_true;
		}

	default:
		return rnd_false;
	}
}

static rnd_bool UndoSwapCopiedObject(UndoListTypePtr Entry)
{
	void *ptr1, *ptr2, *ptr3;
	void *ptr1b, *ptr2b, *ptr3b;
	pcb_any_obj_t *obj, *obj2;
	int type;
	long int swap_id;

	/* lookup entry by it's ID */
	type = pcb_search_obj_by_id(pcb_removelist, &ptr1, &ptr2, &ptr3, Entry->Data.CopyID, Entry->Kind);
	if (type == PCB_OBJ_VOID)
		return rnd_false;

	type = pcb_search_obj_by_id(PCB->Data, &ptr1b, &ptr2b, &ptr3b, Entry->ID, Entry->Kind);
	if (type == PCB_OBJ_VOID)
		return rnd_false;

	obj = (pcb_any_obj_t *) ptr2;
	obj2 = (pcb_any_obj_t *) ptr2b;

	swap_id = obj->ID;
	obj->ID = obj2->ID;
	obj2->ID = swap_id;

	pcb_move_obj_to_buffer(PCB, pcb_removelist, PCB->Data, type, ptr1b, ptr2b, ptr3b);

	if (pcb_undo_and_draw)
		DrawRecoveredObject((pcb_any_obj_t *)ptr2);

	obj = (pcb_any_obj_t *) pcb_move_obj_to_buffer(PCB, PCB->Data, pcb_removelist, type, ptr1, ptr2, ptr3);
	if (Entry->Kind == PCB_OBJ_POLY)
		pcb_poly_init_clip(PCB->Data, (pcb_layer_t *) ptr1b, (pcb_poly_t *) obj);
	return rnd_true;
}

/* recovers an removed polygon point returns rnd_true on success */
static rnd_bool UndoRemoveContour(UndoListTypePtr Entry)
{
	assert(Entry->Kind == PCB_OBJ_POLY);
	return UndoSwapCopiedObject(Entry);
}

/* recovers an inserted polygon point returns rnd_true on success */
static rnd_bool UndoInsertContour(UndoListTypePtr Entry)
{
	assert(Entry->Kind == PCB_OBJ_POLY);
	return UndoSwapCopiedObject(Entry);
}

static int pcb_undo_old_undo(void *ptr_)
{
	UndoListTypePtr ptr = ptr_;
	switch (ptr->Type) {
	case PCB_UNDO_CHANGENAME:
		if (UndoChangeName(ptr))
			return 0;
		break;

	case PCB_UNDO_CREATE:
		if (UndoCopyOrCreate(ptr))
			return 0;
		break;

	case PCB_UNDO_MOVE:
		if (UndoMove(ptr))
			return 0;
		break;

	case PCB_UNDO_REMOVE:
		if (UndoRemove(ptr))
			return 0;
		break;

	case PCB_UNDO_REMOVE_POINT:
		if (UndoRemovePoint(ptr))
			return 0;
		break;

	case PCB_UNDO_INSERT_POINT:
		if (UndoInsertPoint(ptr))
			return 0;
		break;

	case PCB_UNDO_REMOVE_CONTOUR:
		if (UndoRemoveContour(ptr))
			return 0;
		break;

	case PCB_UNDO_INSERT_CONTOUR:
		if (UndoInsertContour(ptr))
			return 0;
		break;

	case PCB_UNDO_ROTATE:
		if (UndoRotate(ptr))
			return 0;
		break;

	case PCB_UNDO_ROTATE90:
		if (UndoRotate90(ptr))
			return 0;
		break;

	case PCB_UNDO_CLEAR:
		if (UndoClearPoly(ptr))
			return 0;
		break;

	case PCB_UNDO_MOVETOLAYER:
		if (UndoMoveToLayer(ptr))
			return 0;
		break;

	case PCB_UNDO_FLAG:
		if (UndoFlag(ptr))
			return 0;
		break;

	case PCB_UNDO_CHANGESIZE:
		if (UndoChangeSize(ptr))
			return 0;
		break;

	case PCB_UNDO_CHANGE2SIZE:
		if (UndoChange2ndSize(ptr))
			return 0;
		break;

	case PCB_UNDO_CHANGEROT:
		if (UndoChangeRot(ptr))
			return 0;
		break;

	case PCB_UNDO_CHANGECLEARSIZE:
		if (UndoChangeClearSize(ptr))
			return 0;
		break;

	case PCB_UNDO_CHANGEANGLES:
		if (UndoChangeAngles(ptr))
			return 0;
		break;

	case PCB_UNDO_CHANGERADII:
		if (UndoChangeRadii(ptr))
			return 0;
		break;

	case PCB_UNDO_OTHERSIDE:
		if (UndoOtherSide(ptr))
			return 0;
		break;
	}
	return -1;
}

/* adds an object to the list of clearpoly objects */
void pcb_undo_add_obj_to_clear_poly(int Type, void *Ptr1, void *Ptr2, void *Ptr3, rnd_bool clear)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_CLEAR, PCB_OBJECT_ID(Ptr3), Type);
		undo->Data.ClearPoly.Clear = clear;
		undo->Data.ClearPoly.Layer = (pcb_layer_t *) Ptr1;
	}
}

/* adds an subc to the list of objects tossed to the other side */
void pcb_undo_add_subc_to_otherside(int Type, void *Ptr1, void *Ptr2, void *Ptr3, rnd_coord_t yoff)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_OTHERSIDE, PCB_OBJECT_ID(Ptr3), Type);
		undo->Data.Move.DY = yoff;
	}
}

/* adds an object to the list of 90-deg rotated objects */
void pcb_undo_add_obj_to_rotate90(int Type, void *Ptr1, void *Ptr2, void *Ptr3, rnd_coord_t CenterX, rnd_coord_t CenterY, unsigned int Steps)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_ROTATE90, PCB_OBJECT_ID(Ptr3), Type);
		undo->Data.Rotate.CenterX = CenterX;
		undo->Data.Rotate.CenterY = CenterY;
		undo->Data.Rotate.Steps = Steps;
	}
}

/* adds an object to the list of rotated objects */
void pcb_undo_add_obj_to_rotate(int Type, void *Ptr1, void *Ptr2, void *Ptr3, rnd_coord_t CenterX, rnd_coord_t CenterY, rnd_angle_t angle)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_ROTATE, PCB_OBJECT_ID(Ptr3), Type);
		undo->Data.Rotate.CenterX = CenterX;
		undo->Data.Rotate.CenterY = CenterY;
		undo->Data.Angle = angle;
	}
}

/* adds an object to the list of removed objects and removes it from the current PCB */
void pcb_undo_move_obj_to_remove(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	UndoListTypePtr Entry;
	Removed *r;
	pcb_any_obj_t *o = Ptr3;
	pcb_subc_t *subc;

	if (Locked)
		return;

	if (!pcb_removelist)
		pcb_removelist = pcb_buffer_new(NULL);

	switch(o->type) {
		case PCB_OBJ_SUBC:
			/* UI special case: if a selected sucircuit is removed, restore selection after
			   the undo placed back the subc on the board; this is needed because
			   the code for subc move-to-buffer will remove selection flags from the
			   subc. Here we really pretend the user has first unselected the subc. */
			if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, o))
				pcb_subc_select(PCB, (pcb_subc_t *)o, PCB_CHGFLG_CLEAR, 0);
			break;
		default:
			break;
	}

	Entry = GetUndoSlot(PCB_UNDO_REMOVE, PCB_OBJECT_ID(Ptr3), Type);
	r = &Entry->Data.Removed;
	r->p_subc_id = 0;

	switch(o->type) {
		case PCB_OBJ_LINE:
		case PCB_OBJ_ARC:
		case PCB_OBJ_TEXT:
		case PCB_OBJ_POLY:
		case PCB_OBJ_GFX:
			subc = pcb_obj_parent_subc(Ptr3);
			if (subc != NULL) {
				pcb_term_del(&subc->terminals, o->term, o);
				r->p_subc_id = subc->ID;
				r->p_subc_layer = o->parent.layer - subc->data->Layer;
			}
			break;
		case PCB_OBJ_PSTK:
			subc = pcb_obj_parent_subc(Ptr3);
			if (subc != NULL) {
				pcb_term_del(&subc->terminals, o->term, o);
				r->p_subc_id = subc->ID;
			}
			break;
TODO("subc: floater subc-in-subc should remember its subc parent too")
		default:
			break;
	}


	pcb_move_obj_to_buffer(PCB, pcb_removelist, PCB->Data, Type, Ptr1, Ptr2, Ptr3);
}

/* adds an object to the list of removed polygon/... points */
void pcb_undo_add_obj_to_remove_point(int Type, void *Ptr1, void *Ptr2, rnd_cardinal_t index)
{
	UndoListTypePtr undo;
	pcb_poly_t *polygon = (pcb_poly_t *) Ptr2;
	rnd_cardinal_t hole;
	rnd_bool last_in_contour = rnd_false;

	if (!Locked) {
		switch (Type) {
		case PCB_OBJ_POLY_POINT:
			{
				/* save the ID of the parent object; else it will be
				 * impossible to recover the point
				 */
				undo = GetUndoSlot(PCB_UNDO_REMOVE_POINT, PCB_OBJECT_ID(polygon), PCB_OBJ_POLY);
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
						last_in_contour = rnd_true;
				if (index == polygon->PointN - 1)
					last_in_contour = rnd_true;
				undo->Data.RemovedPoint.last_in_contour = last_in_contour;
			}
			break;
		}
	}
}

/* adds an object to the list of inserted polygon/... points */
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

	if (!pcb_removelist)
		pcb_removelist = pcb_buffer_new(NULL);

	undo = GetUndoSlot(undo_type, PCB_OBJECT_ID(Ptr2), Type);
	copy = (pcb_any_obj_t *) pcb_copy_obj_to_buffer(PCB, pcb_removelist, PCB->Data, Type, Ptr1, Ptr2, Ptr3);
	undo->Data.CopyID = copy->ID;
}

/* adds an object to the list of removed contours
   (Actually just takes a copy of the whole polygon to restore) */
void pcb_undo_add_obj_to_remove_contour(int Type, pcb_layer_t * Layer, pcb_poly_t * Polygon)
{
	CopyObjectToUndoList(PCB_UNDO_REMOVE_CONTOUR, Type, Layer, Polygon, NULL);
}

/* adds an object to the list of insert contours
   (Actually just takes a copy of the whole polygon to restore) */
void pcb_undo_add_obj_to_insert_contour(int Type, pcb_layer_t * Layer, pcb_poly_t * Polygon)
{
	CopyObjectToUndoList(PCB_UNDO_INSERT_CONTOUR, Type, Layer, Polygon, NULL);
}

/* adds an object to the list of moved objects */
void pcb_undo_add_obj_to_move(int Type, void *Ptr1, void *Ptr2, void *Ptr3, rnd_coord_t DX, rnd_coord_t DY)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_MOVE, PCB_OBJECT_ID(Ptr3), Type);
		undo->Data.Move.DX = DX;
		undo->Data.Move.DY = DY;
	}
}

/* adds an object to the list of objects with changed names */
void pcb_undo_add_obj_to_change_name(int Type, void *Ptr1, void *Ptr2, void *Ptr3, char *OldName)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_CHANGENAME, PCB_OBJECT_ID(Ptr3), Type);
		undo->Data.ChangeName.Name = OldName;
	}
}

/* adds an object to the list of objects moved to another layer */
void pcb_undo_add_obj_to_move_to_layer(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	UndoListTypePtr undo;

	if (!Locked) {
		pcb_layer_t *ly = Ptr1;
		undo = GetUndoSlot(PCB_UNDO_MOVETOLAYER, PCB_OBJECT_ID(Ptr3), Type);
		undo->Data.MoveToLayer.OriginalLayer = pcb_layer_id(ly->parent.data, ly);
	}
}

/* adds an object to the list of created objects */
void pcb_undo_add_obj_to_create(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	if (!Locked)
		GetUndoSlot(PCB_UNDO_CREATE, PCB_OBJECT_ID(Ptr3), Type);
	pcb_poly_clear_from_poly(PCB->Data, Type, Ptr1, Ptr2);
}

void pcb_undo_add_obj_to_create_noclear(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	if (!Locked)
		GetUndoSlot(PCB_UNDO_CREATE, PCB_OBJECT_ID(Ptr3), Type);
}

/* adds an object to the list of objects with flags changed */
void pcb_undo_add_obj_to_flag(void *obj_)
{
	UndoListTypePtr undo;
	pcb_any_obj_t *obj = obj_;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_FLAG, PCB_OBJECT_ID(obj), obj->type);
		undo->Data.Flags = obj->Flags;
	}
}

/* adds an object to the list of objects with Size changes */
void pcb_undo_add_obj_to_size(int Type, void *ptr1, void *ptr2, void *ptr3)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_CHANGESIZE, PCB_OBJECT_ID(ptr2), Type);
		switch (Type) {
		case PCB_OBJ_LINE:
			undo->Data.Size = ((pcb_line_t *) ptr2)->Thickness;
			break;
		case PCB_OBJ_TEXT:
			undo->Data.Size = ((pcb_text_t *) ptr2)->Scale;
			break;
		case PCB_OBJ_ARC:
			undo->Data.Size = ((pcb_arc_t *) ptr2)->Thickness;
			break;
		}
	}
}


/* adds an object to the list of objects with Size changes */
void pcb_undo_add_obj_to_2nd_size(int Type, void *ptr1, void *ptr2, void *ptr3)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_CHANGE2SIZE, PCB_OBJECT_ID(ptr2), Type);
		switch (Type) {
		case PCB_OBJ_TEXT:
			undo->Data.Size = ((pcb_text_t *) ptr2)->thickness;
			break;
		}
	}
}

/* adds an object to the list of objects with rot changes */
void pcb_undo_add_obj_to_rot(int Type, void *ptr1, void *ptr2, void *ptr3)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_CHANGEROT, PCB_OBJECT_ID(ptr2), Type);
		switch (Type) {
		case PCB_OBJ_PSTK:
			undo->Data.Size = ((pcb_pstk_t *) ptr2)->rot;
			break;
		case PCB_OBJ_TEXT:
			undo->Data.Size = ((pcb_text_t *) ptr2)->rot;
			break;
		}
	}
}

/* adds an object to the list of objects with Size changes */
void pcb_undo_add_obj_to_clear_size(int Type, void *ptr1, void *ptr2, void *ptr3)
{
	UndoListTypePtr undo;

	if (!Locked) {
		undo = GetUndoSlot(PCB_UNDO_CHANGECLEARSIZE, PCB_OBJECT_ID(ptr2), Type);
		switch (Type) {
		case PCB_OBJ_LINE:
			undo->Data.Size = ((pcb_line_t *) ptr2)->Clearance;
			break;
		case PCB_OBJ_ARC:
			undo->Data.Size = ((pcb_arc_t *) ptr2)->Clearance;
			break;
		}
	}
}

/* adds an object to the list of changed angles.  Note that you must
   call this before changing the angles, passing the new start/delta. */
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

/* adds an object to the list of changed radii.  Note that you must
   call this before changing the radii, passing the new width/height */
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

#ifndef NDEBUG
const char *undo_type2str(int type)
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
		case PCB_UNDO_ROTATE90: return "rotate90";
		case PCB_UNDO_ROTATE: return "rotate";
		case PCB_UNDO_CREATE: return "create";
		case PCB_UNDO_MOVETOLAYER: return "movetolayer";
		case PCB_UNDO_FLAG: return "flag";
		case PCB_UNDO_CHANGESIZE: return "changesize";
		case PCB_UNDO_CHANGEROT: return "changerot";
		case PCB_UNDO_OTHERSIDE: return "otherside";
		case PCB_UNDO_CHANGECLEARSIZE: return "chngeclearsize";
		case PCB_UNDO_CHANGEANGLES: return "changeangles";
		case PCB_UNDO_CHANGERADII: return "changeradii";
		case PCB_UNDO_CLEAR: return "clear";
	}
	sprintf(buff, "Unknown %d", type);
	return buff;
}

#endif

static void pcb_undo_old_free(void *ptr_)
{
	UndoListTypePtr ptr = ptr_;
	void *ptr1, *ptr2, *ptr3;
	int type;

	switch (ptr->Type) {
		case PCB_UNDO_CHANGENAME:
			free(ptr->Data.ChangeName.Name);
			break;
		case PCB_UNDO_REMOVE:
			type = pcb_search_obj_by_id(pcb_removelist, &ptr1, &ptr2, &ptr3, ptr->ID, ptr->Kind);
			if (type != PCB_OBJ_VOID)
				pcb_destroy_object(pcb_removelist, type, ptr1, ptr2, ptr3);
			break;
		default:
			break;
	}
}

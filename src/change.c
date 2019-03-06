/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#include "config.h"

#include "conf_core.h"

#include "change.h"
#include "board.h"
#include "data.h"
#include "draw.h"
#include "select.h"
#include "undo.h"
#include "actions.h"
#include "macro.h"
#include "obj_pstk_op.h"
#include "obj_subc_parent.h"
#include "obj_term.h"
#include "obj_arc_op.h"
#include "obj_line_op.h"
#include "obj_poly_op.h"
#include "obj_text_op.h"
#include "obj_subc_op.h"

int defer_updates = 0;
int defer_needs_update = 0;

pcb_opfunc_t ChangeSizeFunctions = {
	pcb_lineop_change_size,
	pcb_textop_change_size,
	pcb_polyop_change_clear,
	NULL,
	NULL,
	pcb_arcop_change_size,
	NULL,
	NULL,
	pcb_subcop_change_size,
	pcb_pstkop_change_size
};

pcb_opfunc_t Change1stSizeFunctions = {
	pcb_lineop_change_size,
	pcb_textop_change_size,
	pcb_polyop_change_clear,
	NULL,
	NULL,
	pcb_arcop_change_size,
	NULL,
	NULL,
	pcb_subcop_change_1st_size,
	NULL  /* padstack */
};

pcb_opfunc_t Change2ndSizeFunctions = {
	NULL,
	pcb_textop_change_2nd_size,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	pcb_subcop_change_2nd_size,
	pcb_pstkop_change_2nd_size
};

pcb_opfunc_t ChangeRotFunctions = {
	NULL,
	pcb_textop_change_rot,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	pcb_pstkop_rotate
};

static pcb_opfunc_t ChangeThermalFunctions = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL, /* subc */
	pcb_pstkop_change_thermal
};

pcb_opfunc_t ChangeClearSizeFunctions = {
	pcb_lineop_change_clear_size,
	NULL,
	pcb_polyop_change_clear_size,				/* just to tell the user not to :-) */
	NULL,
	NULL,
	pcb_arcop_change_clear_size,
	NULL,
	NULL,
	pcb_subcop_change_clear_size,
	pcb_pstkop_change_clear_size
};

static pcb_opfunc_t ChangeNameFunctions = {
	NULL,
	pcb_textop_change_name,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	pcb_subcop_change_name,
	NULL  /* padstack */
};

static pcb_opfunc_t ChangeNonetlistFunctions = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	pcb_subcop_change_nonetlist,
	NULL  /* padstack */
};

static pcb_opfunc_t ChangeJoinFunctions = {
	pcb_lineop_change_join,
	pcb_textop_change_join,
	pcb_polyop_change_join,
	NULL,
	NULL,
	pcb_arcop_change_join,
	NULL,
	NULL,
	NULL, /* subc */
	NULL  /* padstack */
};

static pcb_opfunc_t SetJoinFunctions = {
	pcb_lineop_set_join,
	pcb_textop_set_join,
	NULL,
	NULL,
	NULL,
	pcb_arcop_set_join,
	NULL,
	NULL,
	NULL, /* subc */
	NULL  /* padstack */
};

static pcb_opfunc_t ClrJoinFunctions = {
	pcb_lineop_clear_join,
	pcb_textop_clear_join,
	NULL,
	NULL,
	NULL,
	pcb_arcop_clear_join,
	NULL,
	NULL,
	NULL, /* subc */
	NULL  /* padstack */
};

static pcb_opfunc_t ChangeRadiusFunctions = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	pcb_arcop_change_radius,
	NULL,
	NULL,
	NULL, /* subc */
	NULL  /* padstack */
};

static pcb_opfunc_t ChangeAngleFunctions = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	pcb_arcop_change_angle,
	NULL,
	NULL,
	NULL, /* subc */
	NULL  /* padstack */
};

pcb_opfunc_t ChgFlagFunctions = {
	pcb_lineop_change_flag,
	pcb_textop_change_flag,
	pcb_polyop_change_flag,
	NULL,
	NULL,
	pcb_arcop_change_flag,
	NULL,
	NULL,
	pcb_subcop_change_flag,
	pcb_pstkop_change_flag
};

static pcb_opfunc_t InvalLabelFunctions = {
	pcb_lineop_invalidate_label,
	pcb_textop_invalidate_label,
	pcb_polyop_invalidate_label,
	NULL,
	NULL,
	pcb_arcop_invalidate_label,
	NULL,
	NULL,
	/*pcb_subcop_invalidate_flag*/ NULL,
	NULL  /* padstack */
};


/* ----------------------------------------------------------------------
 * changes the thermals on all selected and visible padstacks.
 * Returns pcb_true if anything has changed
 */
pcb_bool pcb_chg_selected_thermals(int types, int therm_style, unsigned long lid)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgtherm.pcb = PCB;
	ctx.chgtherm.style = therm_style;
	ctx.chgtherm.lid = lid;

	change = pcb_selected_operation(PCB, PCB->Data, &ChangeThermalFunctions, &ctx, pcb_false, types, pcb_false);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return change;
}

/* ----------------------------------------------------------------------
 * changes the size of all selected and visible object types
 * returns pcb_true if anything has changed
 */
pcb_bool pcb_chg_selected_size(int types, pcb_coord_t Difference, pcb_bool fixIt)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;
	ctx.chgsize.is_primary = 1;
	ctx.chgsize.is_absolute = fixIt;
	ctx.chgsize.value = Difference;

	change = pcb_selected_operation(PCB, PCB->Data, &ChangeSizeFunctions, &ctx, pcb_false, types, pcb_false);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return change;
}

/* ----------------------------------------------------------------------
 * changes the clearance size of all selected and visible objects
 * returns pcb_true if anything has changed
 */
pcb_bool pcb_chg_selected_clear_size(int types, pcb_coord_t Difference, pcb_bool fixIt)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;
	ctx.chgsize.is_primary = 1;
	ctx.chgsize.is_absolute = fixIt;
	ctx.chgsize.value = Difference;

	change = pcb_selected_operation(PCB, PCB->Data, &ChangeClearSizeFunctions, &ctx, pcb_false, types, pcb_false);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return change;
}

/* --------------------------------------------------------------------------
 * changes the 2nd size (drilling hole) of all selected and visible objects
 * returns pcb_true if anything has changed
 */
pcb_bool pcb_chg_selected_2nd_size(int types, pcb_coord_t Difference, pcb_bool fixIt)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;
	ctx.chgsize.is_primary = 1;
	ctx.chgsize.is_absolute = fixIt;
	ctx.chgsize.value = Difference;

	change = pcb_selected_operation(PCB, PCB->Data, &Change2ndSizeFunctions, &ctx, pcb_false, types, pcb_false);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return change;
}

/* --------------------------------------------------------------------------
 * changes the internal/self rotation all selected and visible objects
 * returns pcb_true if anything has changed
 */
pcb_bool pcb_chg_selected_rot(int types, double Difference, pcb_bool fixIt)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;
	ctx.chgsize.is_primary = 1;
	ctx.chgsize.is_absolute = fixIt;
	ctx.chgsize.value = Difference;

	change = pcb_selected_operation(PCB, PCB->Data, &ChangeRotFunctions, &ctx, pcb_false, types, pcb_false);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return change;
}

/* ----------------------------------------------------------------------
 * changes the clearance flag (join) of all selected and visible lines
 * and/or arcs. Returns pcb_true if anything has changed
 */
pcb_bool pcb_chg_selected_join(int types)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	change = pcb_selected_operation(PCB, PCB->Data, &ChangeJoinFunctions, &ctx, pcb_false, types, pcb_false);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return change;
}

/* ----------------------------------------------------------------------
 * changes the clearance flag (join) of all selected and visible lines
 * and/or arcs. Returns pcb_true if anything has changed
 */
pcb_bool pcb_set_selected_join(int types)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	change = pcb_selected_operation(PCB, PCB->Data, &SetJoinFunctions, &ctx, pcb_false, types, pcb_false);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return change;
}

/* ----------------------------------------------------------------------
 * changes the clearance flag (join) of all selected and visible lines
 * and/or arcs. Returns pcb_true if anything has changed
 */
pcb_bool pcb_clr_selected_join(int types)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	change = pcb_selected_operation(PCB, PCB->Data, &ClrJoinFunctions, &ctx, pcb_false, types, pcb_false);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return change;
}

/* ----------------------------------------------------------------------
 * changes the nonetlist-flag of all selected and visible subcircuits
 * returns pcb_true if anything has changed
 */
pcb_bool pcb_chg_selected_nonetlist(int types)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	change = pcb_selected_operation(PCB, PCB->Data, &ChangeNonetlistFunctions, &ctx, pcb_false, types, pcb_false);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return change;
}

/* ----------------------------------------------------------------------
 * changes the angle of all selected and visible object types
 * returns pcb_true if anything has changed
 */
pcb_bool pcb_chg_selected_angle(int types, int is_start, pcb_angle_t Difference, pcb_bool fixIt)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgangle.pcb = PCB;
	ctx.chgangle.is_primary = is_start;
	ctx.chgangle.is_absolute = fixIt;
	ctx.chgangle.value = Difference;

	change = pcb_selected_operation(PCB, PCB->Data, &ChangeAngleFunctions, &ctx, pcb_false, types, pcb_false);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return change;
}

/* ----------------------------------------------------------------------
 * changes the radius of all selected and visible object types
 * returns pcb_true if anything has changed
 */
pcb_bool pcb_chg_selected_radius(int types, int is_start, pcb_angle_t Difference, pcb_bool fixIt)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;
	ctx.chgsize.is_primary = is_start;
	ctx.chgsize.is_absolute = fixIt;
	ctx.chgsize.value = Difference;

	change = pcb_selected_operation(PCB, PCB->Data, &ChangeRadiusFunctions, &ctx, pcb_false, types, pcb_false);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return change;
}

TODO("subc: check if it is true:")
/* ---------------------------------------------------------------------------
 * changes the size of the passed object; subc size is silk size (TODO: check if it is true)
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_chg_obj_size(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t Difference, pcb_bool fixIt)
{
	pcb_bool change;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;
	ctx.chgsize.is_primary = 1;
	ctx.chgsize.is_absolute = fixIt;
	ctx.chgsize.value = Difference;

	change = (pcb_object_operation(&ChangeSizeFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return change;
}

TODO("subc: check if it is true:")
/* ---------------------------------------------------------------------------
 * changes the size of the passed object; element size is pin ring sizes
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_chg_obj_1st_size(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t Difference, pcb_bool fixIt)
{
	pcb_bool change;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;
	ctx.chgsize.is_primary = 1;
	ctx.chgsize.is_absolute = fixIt;
	ctx.chgsize.value = Difference;

	change = (pcb_object_operation(&Change1stSizeFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return change;
}

/* ---------------------------------------------------------------------------
 * changes the radius of the passed object (e.g. arc width/height)
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_chg_obj_radius(int Type, void *Ptr1, void *Ptr2, void *Ptr3, int is_x, pcb_coord_t r, pcb_bool fixIt)
{
	pcb_bool change;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;
	ctx.chgsize.is_primary = is_x;
	ctx.chgsize.is_absolute = fixIt;
	ctx.chgsize.value = r;

	change = (pcb_object_operation(&ChangeRadiusFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return change;
}

/* ---------------------------------------------------------------------------
 * changes the angles of the passed object (e.g. arc start/ctx->chgsize.value)
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_chg_obj_angle(int Type, void *Ptr1, void *Ptr2, void *Ptr3, int is_start, pcb_angle_t a, pcb_bool fixIt)
{
	pcb_bool change;
	pcb_opctx_t ctx;

	ctx.chgangle.pcb = PCB;
	ctx.chgangle.is_primary = is_start;
	ctx.chgangle.is_absolute = fixIt;
	ctx.chgangle.value = a;

	change = (pcb_object_operation(&ChangeAngleFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return change;
}


/* ---------------------------------------------------------------------------
 * changes the clearance size of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_chg_obj_clear_size(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t Difference, pcb_bool fixIt)
{
	pcb_bool change;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;
	ctx.chgsize.is_primary = 1;
	ctx.chgsize.is_absolute = fixIt;
	ctx.chgsize.value = Difference;

	change = (pcb_object_operation(&ChangeClearSizeFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return change;
}

/* ---------------------------------------------------------------------------
 * changes the thermal of the passed object
 * Returns pcb_true if anything is changed
 *
 */
pcb_bool pcb_chg_obj_thermal(int Type, void *Ptr1, void *Ptr2, void *Ptr3, int therm_type, unsigned long lid)
{
	pcb_bool change;
	pcb_opctx_t ctx;

	ctx.chgtherm.pcb = PCB;
	ctx.chgtherm.style = therm_type;
	ctx.chgtherm.lid = lid;

	change = (pcb_object_operation(&ChangeThermalFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return change;
}

/* ---------------------------------------------------------------------------
 * changes the 2nd size of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_chg_obj_2nd_size(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t Difference, pcb_bool fixIt, pcb_bool incundo)
{
	pcb_bool change;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;
	ctx.chgsize.is_primary = 1;
	ctx.chgsize.is_absolute = fixIt;
	ctx.chgsize.value = Difference;

	change = (pcb_object_operation(&Change2ndSizeFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		pcb_draw();
		if (incundo)
			pcb_undo_inc_serial();
	}
	return change;
}

/* ---------------------------------------------------------------------------
 * changes the internal/self rotation of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_chg_obj_rot(int Type, void *Ptr1, void *Ptr2, void *Ptr3, double Difference, pcb_bool fixIt, pcb_bool incundo)
{
	pcb_bool change;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;
	ctx.chgsize.is_primary = 1;
	ctx.chgsize.is_absolute = fixIt;
	ctx.chgsize.value = Difference;

	change = (pcb_object_operation(&ChangeRotFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		pcb_draw();
		if (incundo)
			pcb_undo_inc_serial();
	}
	return change;
}

/* ---------------------------------------------------------------------------
 * changes the name of the passed object
 * returns the old name
 *
 * The allocated memory isn't freed because the old string is used
 * by the undo module.
 */
void *pcb_chg_obj_name(int Type, void *Ptr1, void *Ptr2, void *Ptr3, char *Name)
{
	void *result;
	pcb_opctx_t ctx;

	ctx.chgname.pcb = PCB;
	ctx.chgname.new_name = Name;

	result = pcb_object_operation(&ChangeNameFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	pcb_draw();
	return result;
}

/* ---------------------------------------------------------------------------
 * changes the clearance-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_chg_obj_join(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	if (pcb_object_operation(&ChangeJoinFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		pcb_draw();
		pcb_undo_inc_serial();
		return pcb_true;
	}
	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * sets the clearance-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_set_obj_join(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	if (pcb_object_operation(&SetJoinFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		pcb_draw();
		pcb_undo_inc_serial();
		return pcb_true;
	}
	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * clears the clearance-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_clr_obj_join(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	if (pcb_object_operation(&ClrJoinFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		pcb_draw();
		pcb_undo_inc_serial();
		return pcb_true;
	}
	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * changes the square-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_chg_obj_nonetlist(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	if (pcb_object_operation(&ChangeNonetlistFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		pcb_draw();
		pcb_undo_inc_serial();
		return pcb_true;
	}
	return pcb_false;
}

void *pcb_chg_obj_name_query(pcb_any_obj_t *obj)
{
	char *name = NULL;
	pcb_subc_t *parent_subc;

	parent_subc = pcb_obj_parent_subc(obj);

	switch(obj->type) {
		case PCB_OBJ_TEXT:
			if ((parent_subc != NULL) && !PCB_FLAG_TEST(PCB_FLAG_FLOATER, obj) && !PCB_FLAG_TEST(PCB_FLAG_DYNTEXT, obj))
				goto term_name; /* special case: dyntext floaters are rarely temrinals */
			if ((parent_subc != NULL) && PCB_FLAG_TEST(PCB_FLAG_DYNTEXT, obj) && (strstr(((pcb_text_t *)obj)->TextString, "%a.parent.refdes%"))) {
				if (pcb_hid_message_box("question", "Text edit: refdes or template?", "Text object seems to be a refdes text", "edit the text (template)", 0, "edit subcircuit refdes", 1, NULL) == 1) {
					obj = (pcb_any_obj_t *)parent_subc;
					goto subc_name;
				}
			}
			name = pcb_hid_prompt_for("Enter text:", PCB_EMPTY(((pcb_text_t *)obj)->TextString), "Change text");
			break;

		case PCB_OBJ_SUBC:
			subc_name:;
			name = pcb_hid_prompt_for("Subcircuit refdes:", PCB_EMPTY(((pcb_subc_t *)obj)->refdes), "Change refdes");
			break;

		default:
			term_name:;
			if (parent_subc != NULL) {
				name = pcb_hid_prompt_for("Enter terminal ID:", PCB_EMPTY(obj->term), "Change terminal ID");
				if (name != NULL) {
					pcb_term_undoable_rename(PCB, obj, name);
					pcb_draw();
				}
				free(name);
				return obj;
			}
			break;
	}

	if (name) {
		/* ChangeObjectName takes ownership of the passed memory; do not free old name, it's kept for undo */
		char *old;
		old = (char *)pcb_chg_obj_name(obj->type, obj->parent.any, obj, obj, name);

		if (old != (char *)-1) {
			pcb_undo_add_obj_to_change_name(obj->type, obj->parent.any, obj, obj, old);
			pcb_undo_inc_serial();
		}
		pcb_draw();
		return obj;
	}
	return NULL;
}

void pcb_flag_change(pcb_board_t *pcb, pcb_change_flag_t how, pcb_flag_values_t flag, int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;

	ctx.chgflag.pcb = pcb;
	ctx.chgflag.how = how;
	ctx.chgflag.flag = flag;

	pcb_object_operation(&ChgFlagFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
}

void *pcb_obj_invalidate_label(pcb_objtype_t Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;
	ctx.noarg.pcb = PCB;
	return pcb_object_operation(&InvalLabelFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
}

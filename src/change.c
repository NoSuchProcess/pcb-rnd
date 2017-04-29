/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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

/* functions used to change object properties
 *
 */

#include "config.h"

#include "conf_core.h"

#include "board.h"
#include "data.h"
#include "draw.h"
#include "select.h"
#include "undo.h"
#include "hid_actions.h"
#include "compat_nls.h"
#include "obj_all_op.h"

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static pcb_opfunc_t ChangeSizeFunctions = {
	ChangeLineSize,
	ChangeTextSize,
	ChangePolyClear,
	ChangeViaSize,
	ChangeElementSize,						/* changes silk screen line width */
	ChangeElementNameSize,
	ChangePinSize,
	ChangePadSize,
	NULL,
	NULL,
	ChangeArcSize,
	NULL,
	NULL
};

static pcb_opfunc_t Change1stSizeFunctions = {
	ChangeLineSize,
	ChangeTextSize,
	ChangePolyClear,
	ChangeViaSize,
	ChangeElement1stSize,
	ChangeElementNameSize,
	ChangePinSize,
	ChangePadSize,
	NULL,
	NULL,
	ChangeArcSize,
	NULL,
	NULL
};

static pcb_opfunc_t Change2ndSizeFunctions = {
	NULL,
	NULL,
	NULL,
	ChangeVia2ndSize,
	ChangeElement2ndSize,
	NULL,
	ChangePin2ndSize,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static pcb_opfunc_t ChangeThermalFunctions = {
	NULL,
	NULL,
	NULL,
	ChangeViaThermal,
	NULL,
	NULL,
	ChangePinThermal,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static pcb_opfunc_t ChangeClearSizeFunctions = {
	ChangeLineClearSize,
	NULL,
	ChangePolygonClearSize,				/* just to tell the user not to :-) */
	ChangeViaClearSize,
	ChangeElementClearSize,
	NULL,
	ChangePinClearSize,
	ChangePadClearSize,
	NULL,
	NULL,
	ChangeArcClearSize,
	NULL,
	NULL
};

static pcb_opfunc_t ChangeNameFunctions = {
	ChangeLineName,
	ChangeTextName,
	NULL,
	ChangeViaName,
	ChangeElementName,
	NULL,
	ChangePinName,
	ChangePadName,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static pcb_opfunc_t ChangePinnumFunctions = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ChangePinNum,
	ChangePadNum,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static pcb_opfunc_t ChangeSquareFunctions = {
	NULL,
	NULL,
	NULL,
	ChangeViaSquare,
	ChangeElementSquare,
	NULL,
	ChangePinSquare,
	ChangePadSquare,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static pcb_opfunc_t ChangeNonetlistFunctions = {
	NULL,
	NULL,
	NULL,
	NULL,
	ChangeElementNonetlist,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static pcb_opfunc_t ChangeJoinFunctions = {
	ChangeLineJoin,
	ChangeTextJoin,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ChangeArcJoin,
	NULL,
	NULL
};

static pcb_opfunc_t ChangeOctagonFunctions = {
	NULL,
	NULL,
	NULL,
	ChangeViaOctagon,
	ChangeElementOctagon,
	NULL,
	ChangePinOctagon,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static pcb_opfunc_t ChangeMaskSizeFunctions = {
	NULL,
	NULL,
	NULL,
	ChangeViaMaskSize,
#if 0
	ChangeElementMaskSize,
#else
	NULL,
#endif
	NULL,
	ChangePinMaskSize,
	ChangePadMaskSize,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static pcb_opfunc_t SetSquareFunctions = {
	NULL,
	NULL,
	NULL,
	NULL,
	SetElementSquare,
	NULL,
	SetPinSquare,
	SetPadSquare,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static pcb_opfunc_t SetJoinFunctions = {
	SetLineJoin,
	SetTextJoin,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	SetArcJoin,
	NULL,
	NULL
};

static pcb_opfunc_t SetOctagonFunctions = {
	NULL,
	NULL,
	NULL,
	SetViaOctagon,
	SetElementOctagon,
	NULL,
	SetPinOctagon,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static pcb_opfunc_t ClrSquareFunctions = {
	NULL,
	NULL,
	NULL,
	NULL,
	ClrElementSquare,
	NULL,
	ClrPinSquare,
	ClrPadSquare,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static pcb_opfunc_t ClrJoinFunctions = {
	ClrLineJoin,
	ClrTextJoin,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ClrArcJoin,
	NULL,
	NULL
};

static pcb_opfunc_t ClrOctagonFunctions = {
	NULL,
	NULL,
	NULL,
	ClrViaOctagon,
	ClrElementOctagon,
	NULL,
	ClrPinOctagon,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static pcb_opfunc_t ChangeRadiusFunctions = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ChangeArcRadius,
	NULL,
	NULL
};

static pcb_opfunc_t ChangeAngleFunctions = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ChangeArcAngle,
	NULL,
	NULL
};

/* ----------------------------------------------------------------------
 * changes the thermals on all selected and visible pins
 * and/or vias. Returns pcb_true if anything has changed
 */
pcb_bool pcb_chg_selected_thermals(int types, int therm_style)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgtherm.pcb = PCB;
	ctx.chgtherm.style = therm_style;

	change = pcb_selected_operation(PCB, &ChangeThermalFunctions, &ctx, pcb_false, types);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
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
	ctx.chgsize.absolute = (fixIt) ? Difference : 0;
	ctx.chgsize.delta = Difference;

	change = pcb_selected_operation(PCB, &ChangeSizeFunctions, &ctx, pcb_false, types);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
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
	ctx.chgsize.absolute = (fixIt) ? Difference : 0;
	ctx.chgsize.delta = Difference;

	if (pcb_mask_on(PCB))
		change = pcb_selected_operation(PCB, &ChangeMaskSizeFunctions, &ctx, pcb_false, types);
	else
		change = pcb_selected_operation(PCB, &ChangeClearSizeFunctions, &ctx, pcb_false, types);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
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
	ctx.chgsize.absolute = (fixIt) ? Difference : 0;
	ctx.chgsize.delta = Difference;

	change = pcb_selected_operation(PCB, &Change2ndSizeFunctions, &ctx, pcb_false, types);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
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

	change = pcb_selected_operation(PCB, &ChangeJoinFunctions, &ctx, pcb_false, types);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
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

	change = pcb_selected_operation(PCB, &SetJoinFunctions, &ctx, pcb_false, types);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
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

	change = pcb_selected_operation(PCB, &ClrJoinFunctions, &ctx, pcb_false, types);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * changes the nonetlist-flag of all selected and visible elements
 * returns pcb_true if anything has changed
 */
pcb_bool pcb_chg_selected_nonetlist(int types)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	change = pcb_selected_operation(PCB, &ChangeNonetlistFunctions, &ctx, pcb_false, types);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
}

#if 0
/* ----------------------------------------------------------------------
 * sets the square-flag of all selected and visible pins or pads
 * returns pcb_true if anything has changed
 */
pcb_bool SetSelectedNonetlist(int types)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	change = pcb_selected_operation(PCB, &SetNonetlistFunctions, &ctx, pcb_false, types);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * clears the square-flag of all selected and visible pins or pads
 * returns pcb_true if anything has changed
 */
pcb_bool ClrSelectedNonetlist(int types)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	change = pcb_selected_operation(PCB, &ClrNonetlistFunctions, &ctx, pcb_false, types);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
}
#endif

/* ----------------------------------------------------------------------
 * changes the square-flag of all selected and visible pins or pads
 * returns pcb_true if anything has changed
 */
pcb_bool pcb_chg_selected_square(int types)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	change = pcb_selected_operation(PCB, &ChangeSquareFunctions, &ctx, pcb_false, types);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
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
	ctx.chgangle.absolute = (fixIt) ? Difference : 0;
	ctx.chgangle.delta = Difference;

	change = pcb_selected_operation(PCB, &ChangeAngleFunctions, &ctx, pcb_false, types);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
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
	ctx.chgsize.absolute = (fixIt) ? Difference : 0;
	ctx.chgsize.delta = Difference;

	change = pcb_selected_operation(PCB, &ChangeRadiusFunctions, &ctx, pcb_false, types);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
}


/* ----------------------------------------------------------------------
 * sets the square-flag of all selected and visible pins or pads
 * returns pcb_true if anything has changed
 */
pcb_bool pcb_set_selected_square(int types)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	change = pcb_selected_operation(PCB, &SetSquareFunctions, &ctx, pcb_false, types);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * clears the square-flag of all selected and visible pins or pads
 * returns pcb_true if anything has changed
 */
pcb_bool pcb_clr_selected_square(int types)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	change = pcb_selected_operation(PCB, &ClrSquareFunctions, &ctx, pcb_false, types);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * changes the octagon-flag of all selected and visible pins and vias
 * returns pcb_true if anything has changed
 */
pcb_bool pcb_chg_selected_octagon(int types)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	change = pcb_selected_operation(PCB, &ChangeOctagonFunctions, &ctx, pcb_false, types);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * sets the octagon-flag of all selected and visible pins and vias
 * returns pcb_true if anything has changed
 */
pcb_bool pcb_set_selected_octagon(int types)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	change = pcb_selected_operation(PCB, &SetOctagonFunctions, &ctx, pcb_false, types);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * clears the octagon-flag of all selected and visible pins and vias
 * returns pcb_true if anything has changed
 */
pcb_bool pcb_clr_selected_octagon(int types)
{
	pcb_bool change = pcb_false;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	change = pcb_selected_operation(PCB, &ClrOctagonFunctions, &ctx, pcb_false, types);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * changes the hole-flag of all selected and visible vias
 * returns pcb_true if anything has changed
 */
pcb_bool pcb_chg_selected_hole(void)
{
	pcb_bool change = pcb_false;

	if (PCB->ViaOn)
		PCB_VIA_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, via))
			change |= pcb_pin_change_hole(via);
	}
	PCB_END_LOOP;
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * changes the no paste-flag of all selected and visible pads
 * returns pcb_true if anything has changed
 */
pcb_bool pcb_chg_selected_paste(void)
{
	pcb_bool change = pcb_false;

	PCB_PAD_ALL_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, pad))
			change |= pcb_pad_change_paste(pad);
	}
	PCB_ENDALL_LOOP;
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
}


/* ---------------------------------------------------------------------------
 * changes the size of the passed object; element size is silk size
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_chg_obj_size(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t Difference, pcb_bool fixIt)
{
	pcb_bool change;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;
	ctx.chgsize.is_primary = 1;
	ctx.chgsize.absolute = (fixIt) ? Difference : 0;
	ctx.chgsize.delta = Difference;

	change = (pcb_object_operation(&ChangeSizeFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
}

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
	ctx.chgsize.absolute = (fixIt) ? Difference : 0;
	ctx.chgsize.delta = Difference;

	change = (pcb_object_operation(&Change1stSizeFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
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
	ctx.chgsize.absolute = (fixIt) ? r : 0;
	ctx.chgsize.delta = r;

	change = (pcb_object_operation(&ChangeRadiusFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
}

/* ---------------------------------------------------------------------------
 * changes the angles of the passed object (e.g. arc start/ctx->chgsize.delta)
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_chg_obj_angle(int Type, void *Ptr1, void *Ptr2, void *Ptr3, int is_start, pcb_angle_t a, pcb_bool fixIt)
{
	pcb_bool change;
	pcb_opctx_t ctx;

	ctx.chgangle.pcb = PCB;
	ctx.chgangle.is_primary = is_start;
	ctx.chgangle.absolute = (fixIt) ? a : 0;
	ctx.chgangle.delta = a;

	change = (pcb_object_operation(&ChangeAngleFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
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
	ctx.chgsize.absolute = (fixIt) ? Difference : 0;
	ctx.chgsize.delta = Difference;

	if (pcb_mask_on(PCB))
		change = (pcb_object_operation(&ChangeMaskSizeFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	else
		change = (pcb_object_operation(&ChangeClearSizeFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
}

/* ---------------------------------------------------------------------------
 * changes the thermal of the passed object
 * Returns pcb_true if anything is changed
 *
 */
pcb_bool pcb_chg_obj_thermal(int Type, void *Ptr1, void *Ptr2, void *Ptr3, int therm_type)
{
	pcb_bool change;
	pcb_opctx_t ctx;

	ctx.chgtherm.pcb = PCB;
	ctx.chgtherm.style = therm_type;

	change = (pcb_object_operation(&ChangeThermalFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
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
	ctx.chgsize.absolute = (fixIt) ? Difference : 0;
	ctx.chgsize.delta = Difference;

	change = (pcb_object_operation(&Change2ndSizeFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		pcb_draw();
		if (incundo)
			pcb_undo_inc_serial();
	}
	return (change);
}

/* ---------------------------------------------------------------------------
 * changes the mask size of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_chg_obj_mask_size(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t Difference, pcb_bool fixIt)
{
	pcb_bool change;
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;
	ctx.chgsize.is_primary = 1;
	ctx.chgsize.absolute = (fixIt) ? Difference : 0;
	ctx.chgsize.delta = Difference;

	change = (pcb_object_operation(&ChangeMaskSizeFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (change);
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
	return (result);
}

/* ---------------------------------------------------------------------------
 * changes the pin number of the passed object
 * returns the old name
 *
 * The allocated memory isn't freed because the old string is used
 * by the undo module.
 */
void *pcb_chg_obj_pinnum(int Type, void *Ptr1, void *Ptr2, void *Ptr3, char *Name)
{
	void *result;
	pcb_opctx_t ctx;

	ctx.chgname.pcb = PCB;
	ctx.chgname.new_name = Name;

	result = pcb_object_operation(&ChangePinnumFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	pcb_draw();
	return (result);
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
		return (pcb_true);
	}
	return (pcb_false);
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
		return (pcb_true);
	}
	return (pcb_false);
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
		return (pcb_true);
	}
	return (pcb_false);
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
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * changes the square-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_chg_obj_square(int Type, void *Ptr1, void *Ptr2, void *Ptr3, int style)
{
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;
	ctx.chgsize.absolute = style;

	if (pcb_object_operation(&ChangeSquareFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		pcb_draw();
		pcb_undo_inc_serial();
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * sets the square-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_set_obj_square(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	if (pcb_object_operation(&SetSquareFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		pcb_draw();
		pcb_undo_inc_serial();
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * clears the square-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_clr_obj_square(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	if (pcb_object_operation(&ClrSquareFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		pcb_draw();
		pcb_undo_inc_serial();
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * changes the octagon-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_chg_obj_octagon(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	if (pcb_object_operation(&ChangeOctagonFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		pcb_draw();
		pcb_undo_inc_serial();
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * sets the octagon-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_set_obj_octagon(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	if (pcb_object_operation(&SetOctagonFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		pcb_draw();
		pcb_undo_inc_serial();
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * clears the octagon-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool pcb_clr_obj_octagon(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;

	ctx.chgsize.pcb = PCB;

	if (pcb_object_operation(&ClrOctagonFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		pcb_draw();
		pcb_undo_inc_serial();
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * queries the user for a new object name and changes it
 *
 * The allocated memory isn't freed because the old string is used
 * by the undo module.
 */
void *pcb_chg_obj_name_query(int Type, void *Ptr1, void *Ptr2, void *Ptr3, int pinnum)
{
	char *name = NULL;
	char msg[513];

	/* if passed an element name, make it an element reference instead */
	if (Type == PCB_TYPE_ELEMENT_NAME) {
		Type = PCB_TYPE_ELEMENT;
		Ptr2 = Ptr1;
		Ptr3 = Ptr1;
	}
	switch (Type) {
	case PCB_TYPE_LINE:
		name = pcb_gui->prompt_for(_("Linename:"), PCB_EMPTY(((pcb_line_t *) Ptr2)->Number));
		break;

	case PCB_TYPE_VIA:
		name = pcb_gui->prompt_for(_("Vianame:"), PCB_EMPTY(((pcb_pin_t *) Ptr2)->Name));
		break;

	case PCB_TYPE_PIN:
		if (pinnum)
			sprintf(msg, _("%s Pin Number:"), PCB_EMPTY(((pcb_pin_t *) Ptr2)->Number));
		else
			sprintf(msg, _("%s Pin Name:"), PCB_EMPTY(((pcb_pin_t *) Ptr2)->Number));
		name = pcb_gui->prompt_for(msg, PCB_EMPTY(((pcb_pin_t *) Ptr2)->Name));
		break;

	case PCB_TYPE_PAD:
		if (pinnum)
			sprintf(msg, _("%s Pad Number:"), PCB_EMPTY(((pcb_pad_t *) Ptr2)->Number));
		else
			sprintf(msg, _("%s Pad Name:"), PCB_EMPTY(((pcb_pad_t *) Ptr2)->Number));
		name = pcb_gui->prompt_for(msg, PCB_EMPTY(((pcb_pad_t *) Ptr2)->Name));
		break;

	case PCB_TYPE_TEXT:
		name = pcb_gui->prompt_for(_("Enter text:"), PCB_EMPTY(((pcb_text_t *) Ptr2)->TextString));
		break;

	case PCB_TYPE_ELEMENT:
		name = pcb_gui->prompt_for(_("Elementname:"), PCB_EMPTY(PCB_ELEM_NAME_VISIBLE(PCB, (pcb_element_t *) Ptr2)));
		break;
	}
	if (name) {
		/* NB: ChangeObjectName takes ownership of the passed memory */
		char *old;
		if (pinnum)
			old = (char *) pcb_chg_obj_pinnum(Type, Ptr1, Ptr2, Ptr3, name);
		else
			old = (char *) pcb_chg_obj_name(Type, Ptr1, Ptr2, Ptr3, name);

		if (old != (char *) -1) {
			if (pinnum)
				pcb_undo_add_obj_to_change_pinnum(Type, Ptr1, Ptr2, Ptr3, old);
			else
				pcb_undo_add_obj_to_change_name(Type, Ptr1, Ptr2, Ptr3, old);
			pcb_undo_inc_serial();
		}
		pcb_draw();
		return (Ptr3);
	}
	return (NULL);
}

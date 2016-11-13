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

	change = SelectedOperation(&ChangeThermalFunctions, &ctx, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = SelectedOperation(&ChangeSizeFunctions, &ctx, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	if (conf_core.editor.show_mask)
		change = SelectedOperation(&ChangeMaskSizeFunctions, &ctx, pcb_false, types);
	else
		change = SelectedOperation(&ChangeClearSizeFunctions, &ctx, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = SelectedOperation(&Change2ndSizeFunctions, &ctx, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = SelectedOperation(&ChangeJoinFunctions, &ctx, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = SelectedOperation(&SetJoinFunctions, &ctx, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = SelectedOperation(&ClrJoinFunctions, &ctx, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = SelectedOperation(&ChangeNonetlistFunctions, &ctx, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = SelectedOperation(&SetNonetlistFunctions, &ctx, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = SelectedOperation(&ClrNonetlistFunctions, &ctx, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = SelectedOperation(&ChangeSquareFunctions, &ctx, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = SelectedOperation(&ChangeAngleFunctions, &ctx, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = SelectedOperation(&ChangeRadiusFunctions, &ctx, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = SelectedOperation(&SetSquareFunctions, &ctx, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = SelectedOperation(&ClrSquareFunctions, &ctx, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = SelectedOperation(&ChangeOctagonFunctions, &ctx, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = SelectedOperation(&SetOctagonFunctions, &ctx, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = SelectedOperation(&ClrOctagonFunctions, &ctx, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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
		VIA_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_SELECTED, via))
			change |= ChangeHole(via);
	}
	END_LOOP;
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	ALLPAD_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_SELECTED, pad))
			change |= ChangePaste(pad);
	}
	ENDALL_LOOP;
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = (ObjectOperation(&ChangeSizeFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = (ObjectOperation(&Change1stSizeFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = (ObjectOperation(&ChangeRadiusFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = (ObjectOperation(&ChangeAngleFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	if (conf_core.editor.show_mask)
		change = (ObjectOperation(&ChangeMaskSizeFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	else
		change = (ObjectOperation(&ChangeClearSizeFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = (ObjectOperation(&ChangeThermalFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	change = (ObjectOperation(&Change2ndSizeFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		Draw();
		if (incundo)
			IncrementUndoSerialNumber();
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

	change = (ObjectOperation(&ChangeMaskSizeFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
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

	result = ObjectOperation(&ChangeNameFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	Draw();
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

	result = ObjectOperation(&ChangePinnumFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	Draw();
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

	if (ObjectOperation(&ChangeJoinFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
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

	if (ObjectOperation(&SetJoinFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
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

	if (ObjectOperation(&ClrJoinFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
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

	if (ObjectOperation(&ChangeNonetlistFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
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

	if (ObjectOperation(&ChangeSquareFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
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

	if (ObjectOperation(&SetSquareFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
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

	if (ObjectOperation(&ClrSquareFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
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

	if (ObjectOperation(&ChangeOctagonFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
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

	if (ObjectOperation(&SetOctagonFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
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

	if (ObjectOperation(&ClrOctagonFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
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
		name = gui->prompt_for(_("Linename:"), EMPTY(((pcb_line_t *) Ptr2)->Number));
		break;

	case PCB_TYPE_VIA:
		name = gui->prompt_for(_("Vianame:"), EMPTY(((pcb_pin_t *) Ptr2)->Name));
		break;

	case PCB_TYPE_PIN:
		if (pinnum)
			sprintf(msg, _("%s Pin Number:"), EMPTY(((pcb_pin_t *) Ptr2)->Number));
		else
			sprintf(msg, _("%s Pin Name:"), EMPTY(((pcb_pin_t *) Ptr2)->Number));
		name = gui->prompt_for(msg, EMPTY(((pcb_pin_t *) Ptr2)->Name));
		break;

	case PCB_TYPE_PAD:
		if (pinnum)
			sprintf(msg, _("%s Pad Number:"), EMPTY(((pcb_pad_t *) Ptr2)->Number));
		else
			sprintf(msg, _("%s Pad Name:"), EMPTY(((pcb_pad_t *) Ptr2)->Number));
		name = gui->prompt_for(msg, EMPTY(((pcb_pad_t *) Ptr2)->Name));
		break;

	case PCB_TYPE_TEXT:
		name = gui->prompt_for(_("Enter text:"), EMPTY(((pcb_text_t *) Ptr2)->TextString));
		break;

	case PCB_TYPE_ELEMENT:
		name = gui->prompt_for(_("Elementname:"), EMPTY(ELEMENT_NAME(PCB, (pcb_element_t *) Ptr2)));
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
				AddObjectToChangePinnumUndoList(Type, Ptr1, Ptr2, Ptr3, old);
			else
				AddObjectToChangeNameUndoList(Type, Ptr1, Ptr2, Ptr3, old);
			IncrementUndoSerialNumber();
		}
		Draw();
		return (Ptr3);
	}
	return (NULL);
}

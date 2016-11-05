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


/* functions used to move pins, elements ...
 */

#include "config.h"
#include "conf_core.h"

#include <setjmp.h>
#include <stdlib.h>

#include "board.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "move.h"
#include "polygon.h"
#include "rtree.h"
#include "search.h"
#include "select.h"
#include "undo.h"
#include "hid_actions.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "layer.h"
#include "box.h"
#include "obj_all_op.h"
#include "obj_line.h"
#include "obj_pinvia.h"

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static pcb_opfunc_t MoveFunctions = {
	MoveLine,
	MoveText,
	MovePolygon,
	MoveVia,
	MoveElement,
	MoveElementName,
	NULL,
	NULL,
	MoveLinePoint,
	MovePolygonPoint,
	MoveArc,
	NULL
}, MoveToLayerFunctions = {
MoveLineToLayer, MoveTextToLayer, MovePolygonToLayer, NULL, NULL, NULL, NULL, NULL, NULL, NULL, MoveArcToLayer, MoveRatToLayer};

/* ---------------------------------------------------------------------------
 * moves the object identified by its data pointers and the type
 * not we don't bump the undo serial number
 */
void *MoveObject(int Type, void *Ptr1, void *Ptr2, void *Ptr3, Coord DX, Coord DY)
{
	void *result;
	pcb_opctx_t ctx;

	ctx.move.pcb = PCB;
	ctx.move.dx = DX;
	ctx.move.dy = DY;
	AddObjectToMoveUndoList(Type, Ptr1, Ptr2, Ptr3, DX, DY);
	result = ObjectOperation(&MoveFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	return (result);
}

/* ---------------------------------------------------------------------------
 * moves the object identified by its data pointers and the type
 * as well as all attached rubberband lines
 */
void *MoveObjectAndRubberband(int Type, void *Ptr1, void *Ptr2, void *Ptr3, Coord DX, Coord DY)
{
	RubberbandTypePtr ptr;
	pcb_opctx_t ctx;
	void *ptr2;

	pcb_draw_inhibit_inc();

	ctx.move.pcb = PCB;
	ctx.move.dx = DX;
	ctx.move.dy = DY;

	if (DX == 0 && DY == 0) {
		int n;

		/* first clear any marks that we made in the line flags */
		for(n = 0, ptr = Crosshair.AttachedObject.Rubberband; n < Crosshair.AttachedObject.RubberbandN; n++, ptr++)
			CLEAR_FLAG(PCB_FLAG_RUBBEREND, ptr->Line);

		return (NULL);
	}

	/* move all the lines... and reset the counter */
	ptr = Crosshair.AttachedObject.Rubberband;
	while (Crosshair.AttachedObject.RubberbandN) {
		/* first clear any marks that we made in the line flags */
		CLEAR_FLAG(PCB_FLAG_RUBBEREND, ptr->Line);
		AddObjectToMoveUndoList(PCB_TYPE_LINE_POINT, ptr->Layer, ptr->Line, ptr->MovedPoint, DX, DY);
		MoveLinePoint(&ctx, ptr->Layer, ptr->Line, ptr->MovedPoint);
		Crosshair.AttachedObject.RubberbandN--;
		ptr++;
	}

	AddObjectToMoveUndoList(Type, Ptr1, Ptr2, Ptr3, DX, DY);
	ptr2 = ObjectOperation(&MoveFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	IncrementUndoSerialNumber();

	pcb_draw_inhibit_dec();
	Draw();

	return (ptr2);
}

/* ---------------------------------------------------------------------------
 * moves the object identified by its data pointers and the type
 * to a new layer without changing it's position
 */
void *MoveObjectToLayer(int Type, void *Ptr1, void *Ptr2, void *Ptr3, LayerTypePtr Target, pcb_bool enmasse)
{
	void *result;
	pcb_opctx_t ctx;

	ctx.move.pcb = PCB;
	ctx.move.dst_layer = Target;
	ctx.move.more_to_come = enmasse;

	result = ObjectOperation(&MoveToLayerFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	IncrementUndoSerialNumber();
	return (result);
}

/* ---------------------------------------------------------------------------
 * moves the selected objects to a new layer without changing their
 * positions
 */
pcb_bool MoveSelectedObjectsToLayer(LayerTypePtr Target)
{
	pcb_bool changed;
	pcb_opctx_t ctx;

	ctx.move.pcb = PCB;
	ctx.move.dst_layer = Target;
	ctx.move.more_to_come = pcb_true;

	changed = SelectedOperation(&MoveToLayerFunctions, &ctx, pcb_true, PCB_TYPEMASK_ALL);
	/* passing pcb_true to above operation causes Undoserial to auto-increment */
	return (changed);
}

/* ---------------------------------------------------------------------------
 * moves the selected layers to a new index in the layer list.
 */

static void move_one_thermal(int old_index, int new_index, PinType * pin)
{
	int t1 = 0, i;
	int oi = old_index, ni = new_index;

	if (old_index != -1)
		t1 = GET_THERM(old_index, pin);

	if (oi == -1)
		oi = MAX_LAYER - 1;					/* inserting a layer */
	if (ni == -1)
		ni = MAX_LAYER - 1;					/* deleting a layer */

	if (oi < ni) {
		for (i = oi; i < ni; i++)
			ASSIGN_THERM(i, GET_THERM(i + 1, pin), pin);
	}
	else {
		for (i = oi; i > ni; i--)
			ASSIGN_THERM(i, GET_THERM(i - 1, pin), pin);
	}

	if (new_index != -1)
		ASSIGN_THERM(new_index, t1, pin);
	else
		ASSIGN_THERM(ni, 0, pin);
}

static void move_all_thermals(int old_index, int new_index)
{
	VIA_LOOP(PCB->Data);
	{
		move_one_thermal(old_index, new_index, via);
	}
	END_LOOP;

	ALLPIN_LOOP(PCB->Data);
	{
		move_one_thermal(old_index, new_index, pin);
	}
	ENDALL_LOOP;
}

static int LastLayerInComponentGroup(int layer)
{
	int cgroup = GetLayerGroupNumberByNumber(max_group + COMPONENT_LAYER);
	int lgroup = GetLayerGroupNumberByNumber(layer);
	if (cgroup == lgroup && PCB->LayerGroups.Number[lgroup] == 2)
		return 1;
	return 0;
}

static int LastLayerInSolderGroup(int layer)
{
	int sgroup = GetLayerGroupNumberByNumber(max_group + SOLDER_LAYER);
	int lgroup = GetLayerGroupNumberByNumber(layer);
	if (sgroup == lgroup && PCB->LayerGroups.Number[lgroup] == 2)
		return 1;
	return 0;
}

int MoveLayer(int old_index, int new_index)
{
	int groups[MAX_LAYER + 2], l, g;
	LayerType saved_layer;
	int saved_group;

	AddLayerChangeToUndoList(old_index, new_index);
	IncrementUndoSerialNumber();

	if (old_index < -1 || old_index >= max_copper_layer) {
		Message(PCB_MSG_DEFAULT, "Invalid old layer %d for move: must be -1..%d\n", old_index, max_copper_layer - 1);
		return 1;
	}
	if (new_index < -1 || new_index > max_copper_layer || new_index >= MAX_LAYER) {
		Message(PCB_MSG_DEFAULT, "Invalid new layer %d for move: must be -1..%d\n", new_index, max_copper_layer);
		return 1;
	}
	if (old_index == new_index)
		return 0;

	if (new_index == -1 && LastLayerInComponentGroup(old_index)) {
		gui->confirm_dialog("You can't delete the last top-side layer\n", "Ok", NULL);
		return 1;
	}

	if (new_index == -1 && LastLayerInSolderGroup(old_index)) {
		gui->confirm_dialog("You can't delete the last bottom-side layer\n", "Ok", NULL);
		return 1;
	}

	for (g = 0; g < MAX_LAYER + 2; g++)
		groups[g] = -1;

	for (g = 0; g < MAX_LAYER; g++)
		for (l = 0; l < PCB->LayerGroups.Number[g]; l++)
			groups[PCB->LayerGroups.Entries[g][l]] = g;

	if (old_index == -1) {
		LayerTypePtr lp;
		if (max_copper_layer == MAX_LAYER) {
			Message(PCB_MSG_DEFAULT, "No room for new layers\n");
			return 1;
		}
		/* Create a new layer at new_index. */
		lp = &PCB->Data->Layer[new_index];
		memmove(&PCB->Data->Layer[new_index + 1],
						&PCB->Data->Layer[new_index], (max_copper_layer - new_index + 2) * sizeof(LayerType));
		memmove(&groups[new_index + 1], &groups[new_index], (max_copper_layer - new_index + 2) * sizeof(int));
		max_copper_layer++;
		memset(lp, 0, sizeof(LayerType));
		lp->On = 1;
		lp->Name = pcb_strdup("New Layer");
		lp->Color = conf_core.appearance.color.layer[new_index];
		lp->SelectedColor = conf_core.appearance.color.layer_selected[new_index];
		for (l = 0; l < max_copper_layer; l++)
			if (LayerStack[l] >= new_index)
				LayerStack[l]++;
		LayerStack[max_copper_layer - 1] = new_index;
	}
	else if (new_index == -1) {
		/* Delete the layer at old_index */
		memmove(&PCB->Data->Layer[old_index],
						&PCB->Data->Layer[old_index + 1], (max_copper_layer - old_index + 2 - 1) * sizeof(LayerType));
		memset(&PCB->Data->Layer[max_copper_layer + 1], 0, sizeof(LayerType));
		memmove(&groups[old_index], &groups[old_index + 1], (max_copper_layer - old_index + 2 - 1) * sizeof(int));
		for (l = 0; l < max_copper_layer; l++)
			if (LayerStack[l] == old_index)
				memmove(LayerStack + l, LayerStack + l + 1, (max_copper_layer - l - 1) * sizeof(LayerStack[0]));
		max_copper_layer--;
		for (l = 0; l < max_copper_layer; l++)
			if (LayerStack[l] > old_index)
				LayerStack[l]--;
	}
	else {
		/* Move an existing layer */
		memcpy(&saved_layer, &PCB->Data->Layer[old_index], sizeof(LayerType));
		saved_group = groups[old_index];
		if (old_index < new_index) {
			memmove(&PCB->Data->Layer[old_index], &PCB->Data->Layer[old_index + 1], (new_index - old_index) * sizeof(LayerType));
			memmove(&groups[old_index], &groups[old_index + 1], (new_index - old_index) * sizeof(int));
		}
		else {
			memmove(&PCB->Data->Layer[new_index + 1], &PCB->Data->Layer[new_index], (old_index - new_index) * sizeof(LayerType));
			memmove(&groups[new_index + 1], &groups[new_index], (old_index - new_index) * sizeof(int));
		}
		memcpy(&PCB->Data->Layer[new_index], &saved_layer, sizeof(LayerType));
		groups[new_index] = saved_group;
	}

	move_all_thermals(old_index, new_index);

	for (g = 0; g < MAX_LAYER; g++)
		PCB->LayerGroups.Number[g] = 0;
	for (l = 0; l < max_copper_layer + 2; l++) {
		int i;
		g = groups[l];
		if (g >= 0) {
			i = PCB->LayerGroups.Number[g]++;
			PCB->LayerGroups.Entries[g][i] = l;
		}
	}

	for (g = 0; g < MAX_LAYER; g++)
		if (PCB->LayerGroups.Number[g] == 0) {
			memmove(&PCB->LayerGroups.Number[g],
							&PCB->LayerGroups.Number[g + 1], (MAX_LAYER - g - 1) * sizeof(PCB->LayerGroups.Number[g]));
			memmove(&PCB->LayerGroups.Entries[g],
							&PCB->LayerGroups.Entries[g + 1], (MAX_LAYER - g - 1) * sizeof(PCB->LayerGroups.Entries[g]));
		}

	hid_action("LayersChanged");
	gui->invalidate_all();
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char movelayer_syntax[] = "MoveLayer(old,new)";

static const char movelayer_help[] = "Moves/Creates/Deletes Layers.";

/* %start-doc actions MoveLayer

Moves a layer, creates a new layer, or deletes a layer.

@table @code

@item old
The is the layer number to act upon.  Allowed values are:
@table @code

@item c
Currently selected layer.

@item -1
Create a new layer.

@item number
An existing layer number.

@end table

@item new
Specifies where to move the layer to.  Allowed values are:
@table @code
@item -1
Deletes the layer.

@item up
Moves the layer up.

@item down
Moves the layer down.

@item c
Creates a new layer.

@end table

@end table

%end-doc */

int MoveLayerAction(int argc, const char **argv, Coord x, Coord y)
{
	int old_index, new_index;
	int new_top = -1;

	if (argc != 2) {
		Message(PCB_MSG_DEFAULT, "Usage; MoveLayer(old,new)");
		return 1;
	}

	if (strcmp(argv[0], "c") == 0)
		old_index = INDEXOFCURRENT;
	else
		old_index = atoi(argv[0]);

	if (strcmp(argv[1], "c") == 0) {
		new_index = INDEXOFCURRENT;
		if (new_index < 0)
			new_index = 0;
	}
	else if (strcmp(argv[1], "up") == 0) {
		new_index = INDEXOFCURRENT - 1;
		if (new_index < 0)
			return 1;
		new_top = new_index;
	}
	else if (strcmp(argv[1], "down") == 0) {
		new_index = INDEXOFCURRENT + 1;
		if (new_index >= max_copper_layer)
			return 1;
		new_top = new_index;
	}
	else
		new_index = atoi(argv[1]);

	if (MoveLayer(old_index, new_index))
		return 1;

	if (new_index == -1) {
		new_top = old_index;
		if (new_top >= max_copper_layer)
			new_top--;
		new_index = new_top;
	}
	if (old_index == -1)
		new_top = new_index;

	if (new_top != -1)
		ChangeGroupVisibility(new_index, 1, 1);

	return 0;
}

HID_Action move_action_list[] = {
	{"MoveLayer", 0, MoveLayerAction,
	 movelayer_help, movelayer_syntax}
};

REGISTER_ACTIONS(move_action_list, NULL)

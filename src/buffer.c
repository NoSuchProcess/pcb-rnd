/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* functions used by paste- and move/copy buffer
 */
#include "config.h"
#include "conf_core.h"

#include "buffer.h"
#include "board.h"
#include "move.h"
#include "data.h"
#include "plug_io.h"
#include "polygon.h"
#include "rotate.h"
#include "remove.h"
#include "select.h"
#include "draw.h"
#include "undo.h"
#include "funchash_core.h"
#include "compat_misc.h"
#include "obj_arc_op.h"
#include "obj_line_op.h"
#include "obj_text_op.h"
#include "obj_subc_op.h"
#include "obj_poly_op.h"
#include "obj_pstk_op.h"
#include "obj_rat_op.h"
#include "obj_pstk.h"
#include "layer_grp.h"
#include "event.h"
#include "safe_fs.h"
#include "actions.h"

static pcb_opfunc_t AddBufferFunctions = {
	pcb_lineop_add_to_buffer,
	pcb_textop_add_to_buffer,
	pcb_polyop_add_to_buffer,
	NULL,
	NULL,
	pcb_arcop_add_to_buffer,
	pcb_ratop_add_to_buffer,
	NULL,
	pcb_subcop_add_to_buffer,
	pcb_pstkop_add_to_buffer,
};

static pcb_opfunc_t MoveBufferFunctions = {
	pcb_lineop_move_buffer,
	pcb_textop_move_buffer,
	pcb_polyop_move_buffer,
	NULL,
	NULL,
	pcb_arcop_move_buffer,
	pcb_ratop_move_buffer,
	NULL,
	pcb_subcop_move_buffer,
	pcb_pstkop_move_buffer,
};

int pcb_set_buffer_bbox(pcb_buffer_t *Buffer)
{
	pcb_box_t tmp, *box;
	int res = 0;
	
	box = pcb_data_bbox(&tmp, Buffer->Data, pcb_false);
	if (box)
		Buffer->BoundingBox = *box;
	else
		res = -1;

	box = pcb_data_bbox_naked(&tmp, Buffer->Data, pcb_false);
	if (box)
		Buffer->bbox_naked = *box;
	else
		res = -1;

	return res;
}

static void pcb_buffer_clear_(pcb_board_t *pcb, pcb_buffer_t *Buffer, pcb_bool bind)
{
	if (Buffer && Buffer->Data) {
		void *old_parent = Buffer->Data->parent.any;
		pcb_parenttype_t old_pt = Buffer->Data->parent_type;

		pcb_data_uninit(Buffer->Data);
		pcb_data_init(Buffer->Data);

		Buffer->Data->parent.any = old_parent;
		Buffer->Data->parent_type = old_pt;
		if ((pcb != NULL) && (bind))
			pcb_data_bind_board_layers(pcb, Buffer->Data, 0);
	}
	Buffer->from_outside = 0;
}

void pcb_buffer_clear(pcb_board_t *pcb, pcb_buffer_t *Buffer)
{
	pcb_buffer_clear_(pcb, Buffer, pcb_true);
}

/* add (or move) selected */
static void pcb_buffer_toss_selected(pcb_opfunc_t *fnc, pcb_board_t *pcb, pcb_buffer_t *Buffer, pcb_coord_t X, pcb_coord_t Y, pcb_bool LeaveSelected, pcb_bool on_locked_too)
{
	pcb_opctx_t ctx;

	ctx.buffer.pcb = pcb;

	/* switch crosshair off because adding objects to the pastebuffer
	 * may change the 'valid' area for the cursor
	 */
	if (!LeaveSelected)
		ctx.buffer.extraflg = PCB_FLAG_SELECTED;
	else
		ctx.buffer.extraflg =  0;

	pcb_notify_crosshair_change(pcb_false);
	ctx.buffer.src = pcb->Data;
	ctx.buffer.dst = Buffer->Data;
	pcb_selected_operation(pcb, pcb->Data, fnc, &ctx, pcb_false, PCB_OBJ_ANY & (~PCB_OBJ_SUBC_PART), on_locked_too);

	/* set origin to passed or current position */
	if (X || Y) {
		Buffer->X = X;
		Buffer->Y = Y;
	}
	else {
		Buffer->X = pcb_crosshair.X;
		Buffer->Y = pcb_crosshair.Y;
	}
	Buffer->from_outside = 0;
	pcb_notify_crosshair_change(pcb_true);
}

void pcb_buffer_add_selected(pcb_board_t *pcb, pcb_buffer_t *Buffer, pcb_coord_t X, pcb_coord_t Y, pcb_bool LeaveSelected)
{
	pcb_buffer_toss_selected(&AddBufferFunctions, pcb, Buffer, X, Y, LeaveSelected, pcb_false);
}

static const char pcb_acts_LoadFootprint[] = "pcb_load_footprint(filename[,refdes,value])";
static const char pcb_acth_LoadFootprint[] = "Loads a single footprint by name.";
/* DOC: loadfootprint.html */
fgw_error_t pcb_act_LoadFootprint(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *name, *refdes = NULL, *value = NULL;
	pcb_subc_t *s;
	pcb_cardinal_t len;

	
	PCB_ACT_CONVARG(1, FGW_STR, LoadFootprint, name = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, LoadFootprint, refdes = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, LoadFootprint, value = argv[3].val.str);

	if (!pcb_buffer_load_footprint(PCB_PASTEBUFFER, name, NULL)) {
		PCB_ACT_IRES(1);
		return 0;
	}

	len = pcb_subclist_length(&PCB_PASTEBUFFER->Data->subc);

	if (len == 0) {
		pcb_message(PCB_MSG_ERROR, "Footprint %s contains no subcircuits", name);
		PCB_ACT_IRES(1);
		return 0;
	}
	if (len > 1) {
		pcb_message(PCB_MSG_ERROR, "Footprint %s contains multiple subcircuits", name);
		PCB_ACT_IRES(1);
		return 0;
	}

	s = pcb_subclist_first(&PCB_PASTEBUFFER->Data->subc);
	pcb_attribute_put(&s->Attributes, "refdes", refdes);
	pcb_attribute_put(&s->Attributes, "footprint", name);
	pcb_attribute_put(&s->Attributes, "value", value);

	PCB_ACT_IRES(0);
	return 0;
}

pcb_bool pcb_buffer_load_layout(pcb_board_t *pcb, pcb_buffer_t *Buffer, const char *Filename, const char *fmt)
{
	pcb_board_t *newPCB = pcb_board_new_(pcb_false);

	/* new data isn't added to the undo list */
	if (!pcb_parse_pcb(newPCB, Filename, fmt, CFR_invalid, 0)) {
		/* clear data area and replace pointer */
		pcb_buffer_clear(pcb, Buffer);
		free(Buffer->Data);
		Buffer->Data = newPCB->Data;
		newPCB->Data = NULL;
		Buffer->X = 0;
		Buffer->Y = 0;
		PCB_CLEAR_PARENT(Buffer->Data);
		pcb_data_make_layers_bound(newPCB, Buffer->Data);
		pcb_data_binding_update(pcb, Buffer->Data);
		pcb_board_remove(newPCB);
		Buffer->from_outside = 1;
		pcb_event(&newPCB->hidlib, PCB_EVENT_LAYERS_CHANGED, NULL); /* undo the events generated on load */
		return pcb_true;
	}

	/* release unused memory */
	pcb_board_remove(newPCB);
	if (Buffer->Data != NULL)
		PCB_CLEAR_PARENT(Buffer->Data);
	pcb_event(&pcb->hidlib, PCB_EVENT_LAYERS_CHANGED, NULL); /* undo the events generated on load */
	Buffer->from_outside = 0;
	return pcb_false;
}

void pcb_buffer_rotate90(pcb_buffer_t *Buffer, unsigned int Number)
{
	pcb_undo_freeze_serial();
	pcb_undo_freeze_add();

	PCB_PADSTACK_LOOP(Buffer->Data);
	{
		pcb_pstk_rotate90(padstack, Buffer->X, Buffer->Y, Number);
	}
	PCB_END_LOOP;

	PCB_SUBC_LOOP(Buffer->Data);
	{
		pcb_obj_rotate90(PCB_OBJ_SUBC, subc, subc, subc, Buffer->X, Buffer->Y, Number);
	}
	PCB_END_LOOP;

	/* all layer related objects */
	PCB_LINE_ALL_LOOP(Buffer->Data);
	{
		if (layer->line_tree != NULL)
			pcb_r_delete_entry(layer->line_tree, (pcb_box_t *) line);
		pcb_line_rotate90(line, Buffer->X, Buffer->Y, Number);
		if (layer->line_tree != NULL)
			pcb_r_insert_entry(layer->line_tree, (pcb_box_t *) line);
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(Buffer->Data);
	{
		if (layer->arc_tree != NULL)
			pcb_r_delete_entry(layer->arc_tree, (pcb_box_t *) arc);
		pcb_arc_rotate90(arc, Buffer->X, Buffer->Y, Number);
		if (layer->arc_tree != NULL)
			pcb_r_insert_entry(layer->arc_tree, (pcb_box_t *) arc);
	}
	PCB_ENDALL_LOOP;
	PCB_TEXT_ALL_LOOP(Buffer->Data);
	{
		if (layer->text_tree != NULL)
			pcb_r_delete_entry(layer->text_tree, (pcb_box_t *) text);
		pcb_text_rotate90(text, Buffer->X, Buffer->Y, Number);
		if (layer->text_tree != NULL)
			pcb_r_insert_entry(layer->text_tree, (pcb_box_t *) text);
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_ALL_LOOP(Buffer->Data);
	{
		if (layer->polygon_tree != NULL)
			pcb_r_delete_entry(layer->polygon_tree, (pcb_box_t *) polygon);
		pcb_poly_rotate90(polygon, Buffer->X, Buffer->Y, Number);
		if (layer->polygon_tree != NULL)
			pcb_r_insert_entry(layer->polygon_tree, (pcb_box_t *) polygon);
	}
	PCB_ENDALL_LOOP;

	/* finally the origin and the bounding box */
	PCB_COORD_ROTATE90(Buffer->X, Buffer->Y, Buffer->X, Buffer->Y, Number);
	pcb_box_rotate90(&Buffer->BoundingBox, Buffer->X, Buffer->Y, Number);

	pcb_undo_unfreeze_add();
	pcb_undo_unfreeze_serial();
}

void pcb_buffer_rotate(pcb_buffer_t *Buffer, pcb_angle_t angle)
{
	double cosa, sina;

	pcb_undo_freeze_serial();
	pcb_undo_freeze_add();

	cosa = cos(angle * M_PI / 180.0);
	sina = sin(angle * M_PI / 180.0);

	PCB_PADSTACK_LOOP(Buffer->Data);
	{
		pcb_pstk_rotate(padstack, Buffer->X, Buffer->Y, cosa, sina, angle);
	}
	PCB_END_LOOP;

	PCB_SUBC_LOOP(Buffer->Data);
	{
		pcb_subc_rotate(subc, Buffer->X, Buffer->Y, cosa, sina, angle);
	}
	PCB_END_LOOP;

	/* all layer related objects */
	PCB_LINE_ALL_LOOP(Buffer->Data);
	{
		pcb_line_rotate(layer, line, Buffer->X, Buffer->Y, cosa, sina);
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(Buffer->Data);
	{
		pcb_arc_rotate(layer, arc, Buffer->X, Buffer->Y, cosa, sina, angle);
	}
	PCB_ENDALL_LOOP;
	PCB_TEXT_ALL_LOOP(Buffer->Data);
	{
		pcb_text_rotate(text, Buffer->X, Buffer->Y, cosa, sina, angle);
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_ALL_LOOP(Buffer->Data);
	{
		pcb_poly_rotate(layer, polygon, Buffer->X, Buffer->Y, cosa, sina);
	}
	PCB_ENDALL_LOOP;

	pcb_set_buffer_bbox(Buffer);
	pcb_undo_unfreeze_add();
	pcb_undo_unfreeze_serial();
}

pcb_data_t *pcb_buffer_new(pcb_board_t *pcb)
{
	return pcb_data_new(pcb);
}


static const char pcb_acts_FreeRotateBuffer[] = "FreeRotateBuffer([Angle])";
static const char pcb_acth_FreeRotateBuffer[] =
	"Rotates the current paste buffer contents by the specified angle.  The\n"
	"angle is given in degrees.  If no angle is given, the user is prompted\n" "for one.\n";
/* DOC: freerotatebuffer */
fgw_error_t pcb_act_FreeRotateBuffer(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *angle_s = NULL;
	double ang;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, FreeRotateBuffer, angle_s = pcb_strdup(argv[1].val.str));
	PCB_ACT_IRES(0);

	if (angle_s == NULL)
		angle_s = pcb_hid_prompt_for("Enter Rotation (degrees, CCW):", "0", "Rotation angle");

	if ((angle_s == NULL) || (*angle_s == '\0')) {
		free(angle_s);
		PCB_ACT_IRES(-1);
		return 0;
	}

	PCB_ACT_IRES(0);
	ang = strtod(angle_s, 0);
	free(angle_s);
	if (ang == 0) {
		PCB_ACT_IRES(-1);
		return 0;
	}

	if ((ang < -360000) || (ang > +360000)) {
		pcb_message(PCB_MSG_ERROR, "Angle too large\n");
		PCB_ACT_IRES(-1);
		return 0;
	}

	pcb_notify_crosshair_change(pcb_false);
	pcb_buffer_rotate(PCB_PASTEBUFFER, ang);
	pcb_notify_crosshair_change(pcb_true);
	return 0;
}

static const char pcb_acts_ScaleBuffer[] = "ScaleBuffer(x [,y [,thickness [,subc]]])";
static const char pcb_acth_ScaleBuffer[] =
	"Scales the buffer by multiplying all coordinates by a floating point number.\n"
	"If only x is given, it is also used for y and thickness too. If subc is not\n"
	"empty, subcircuits are also scaled\n";
fgw_error_t pcb_act_ScaleBuffer(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *sx = NULL;
	double x, y, th;
	int recurse = 0;
	char *end;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, ScaleBuffer, sx = pcb_strdup(argv[1].val.str));

	if (sx == NULL)
		sx = pcb_hid_prompt_for("Enter scaling factor (unitless multiplier):", "1.0", "scaling factor");
	if ((sx == NULL) || (*sx == '\0')) {
		free(sx);
		PCB_ACT_IRES(-1);
		return 0;
	}
	x = strtod(sx, &end);
	if (*end != '\0') {
		free(sx);
		PCB_ACT_IRES(-1);
		return 0;
	}
	free(sx);
	y = th = x;

	PCB_ACT_MAY_CONVARG(2, FGW_DOUBLE, ScaleBuffer, y = argv[2].val.nat_double);
	PCB_ACT_MAY_CONVARG(3, FGW_DOUBLE, ScaleBuffer, th = argv[3].val.nat_double);
	PCB_ACT_MAY_CONVARG(4, FGW_STR, ScaleBuffer, recurse = (argv[4].val.str != NULL));

	PCB_ACT_IRES(0);


	pcb_notify_crosshair_change(pcb_false);
	pcb_buffer_scale(PCB_PASTEBUFFER, x, y, th, recurse);
	pcb_notify_crosshair_change(pcb_true);
	return 0;
}


void pcb_init_buffers(pcb_board_t *pcb)
{
	int i;

	for (i = 0; i < PCB_MAX_BUFFER; i++)
		pcb_buffers[i].Data = pcb_buffer_new(NULL);
}

void pcb_uninit_buffers(pcb_board_t *pcb)
{
	int i;

	for (i = 0; i < PCB_MAX_BUFFER; i++) {
		pcb_buffer_clear_(pcb, pcb_buffers+i, pcb_false);
		pcb_data_uninit(pcb_buffers[i].Data);
		free(pcb_buffers[i].Data);
	}
}

void pcb_buffer_mirror(pcb_board_t *pcb, pcb_buffer_t *Buffer)
{
	int i, num_layers;

	num_layers = PCB_PASTEBUFFER->Data->LayerN;
	if (num_layers == 0) /* some buffers don't have layers, just simple objects */
		num_layers = pcb->Data->LayerN;

	for (i = 0; i < num_layers; i++) {
		pcb_layer_t *layer = Buffer->Data->Layer + i;
		if (textlist_length(&layer->Text)) {
			pcb_message(PCB_MSG_ERROR, "You can't mirror a buffer that has text!\n");
			return;
		}
	}
	/* set buffer offset to 'mark' position */
	Buffer->X = PCB_SWAP_X(Buffer->X);
	Buffer->Y = PCB_SWAP_Y(Buffer->Y);
	pcb_undo_freeze_add();
	pcb_data_mirror(Buffer->Data, 0, PCB_TXM_COORD, pcb_false);
	pcb_undo_unfreeze_add();
	pcb_set_buffer_bbox(Buffer);
}

void pcb_buffer_scale(pcb_buffer_t *Buffer, double sx, double sy, double sth, int recurse)
{
	pcb_data_scale(Buffer->Data, sx, sy, sth, recurse);
	Buffer->X = pcb_round((double)Buffer->X * sx);
	Buffer->Y = pcb_round((double)Buffer->Y * sy);
	pcb_set_buffer_bbox(Buffer);
}

void pcb_buffer_flip_side(pcb_board_t *pcb, pcb_buffer_t *Buffer)
{
	int j, k;
	pcb_layer_id_t SLayer = -1, CLayer = -1;
	pcb_layergrp_id_t sgroup, cgroup;
	pcb_layer_t swap;

#if 0
/* this results in saving flipped (bottom-side) footprints whenlooking at the board from the bottom */
	PCB_SUBC_LOOP(Buffer->Data);
	{
		pcb_subc_change_side(subc, /*2 * pcb_crosshair.Y - PCB->hidlib.size_y*/0);
	}
	PCB_END_LOOP;
#endif

	/* set buffer offset to 'mark' position */
	Buffer->X = PCB_SWAP_X(Buffer->X);
	Buffer->Y = PCB_SWAP_Y(Buffer->Y);

	PCB_PADSTACK_LOOP(Buffer->Data);
	{
		pcb_pstk_mirror(padstack, PCB_PSTK_DONT_MIRROR_COORDS, 1, 0);
	}
	PCB_END_LOOP;
	PCB_LINE_ALL_LOOP(Buffer->Data);
	{
		pcb_line_flip_side(layer, line);
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(Buffer->Data);
	{
		pcb_arc_flip_side(layer, arc);
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_ALL_LOOP(Buffer->Data);
	{
		pcb_poly_flip_side(layer, polygon);
	}
	PCB_ENDALL_LOOP;
	PCB_TEXT_ALL_LOOP(Buffer->Data);
	{
		pcb_text_flip_side(layer, text, 0);
	}
	PCB_ENDALL_LOOP;

	/* swap silkscreen layers */
	pcb_layer_list(pcb, PCB_LYT_BOTTOM | PCB_LYT_SILK, &SLayer, 1);
	pcb_layer_list(pcb, PCB_LYT_TOP | PCB_LYT_SILK, &CLayer, 1);
	assert(SLayer != -1);
	assert(CLayer != -1);
	swap = Buffer->Data->Layer[SLayer];
	Buffer->Data->Layer[SLayer] = Buffer->Data->Layer[CLayer];
	Buffer->Data->Layer[CLayer] = swap;

	/* swap layer groups when balanced */
	sgroup = pcb_layer_get_group(pcb, SLayer);
	cgroup = pcb_layer_get_group(pcb, CLayer);
TODO("layer: revise this code for the generic physical layer support; move this code to layer*.c")
	if (pcb->LayerGroups.grp[cgroup].len == pcb->LayerGroups.grp[sgroup].len) {
		for (j = k = 0; j < pcb->LayerGroups.grp[sgroup].len; j++) {
			pcb_layer_id_t cnumber = pcb->LayerGroups.grp[cgroup].lid[k];
			pcb_layer_id_t snumber = pcb->LayerGroups.grp[sgroup].lid[j];

			if ((pcb_layer_flags(pcb, snumber)) & PCB_LYT_SILK)
				continue;
			swap = Buffer->Data->Layer[snumber];

			while ((pcb_layer_flags(pcb, cnumber)) & PCB_LYT_SILK) {
				k++;
				cnumber = pcb->LayerGroups.grp[cgroup].lid[k];
			}
			Buffer->Data->Layer[snumber] = Buffer->Data->Layer[cnumber];
			Buffer->Data->Layer[cnumber] = swap;
			k++;
		}
	}
	pcb_set_buffer_bbox(Buffer);
}

void pcb_buffers_flip_side(pcb_board_t *pcb)
{
	int i;

	for (i = 0; i < PCB_MAX_BUFFER; i++)
		pcb_buffer_flip_side(pcb, &pcb_buffers[i]);
	pcb_crosshair_range_to_buffer();
}

void *pcb_move_obj_to_buffer(pcb_board_t *pcb, pcb_data_t *Destination, pcb_data_t *Src, int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;

	/* setup local identifiers used by move operations */
	ctx.buffer.pcb = pcb;
	ctx.buffer.dst = Destination;
	ctx.buffer.src = Src;
	return (pcb_object_operation(&MoveBufferFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3));
}

void *pcb_copy_obj_to_buffer(pcb_board_t *pcb, pcb_data_t *Destination, pcb_data_t *Src, int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;

	ctx.buffer.pcb = pcb;
	ctx.buffer.dst = Destination;
	ctx.buffer.src = Src;
	return (pcb_object_operation(&AddBufferFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3));
}

pcb_bool pcb_buffer_copy_to_layout(pcb_board_t *pcb, pcb_coord_t X, pcb_coord_t Y)
{
	pcb_cardinal_t i;
	pcb_bool changed = pcb_false;
	pcb_opctx_t ctx;
	int num_layers;

#ifdef DEBUG
	printf("Entering CopyPastebufferToLayout.....\n");
#endif

	pcb_data_clip_inhibit_inc(pcb->Data);

	/* set movement vector */
	ctx.copy.pcb = pcb;
	ctx.copy.DeltaX = X - PCB_PASTEBUFFER->X;
	ctx.copy.DeltaY = Y - PCB_PASTEBUFFER->Y;
	ctx.copy.from_outside = PCB_PASTEBUFFER->from_outside;

	/* paste all layers */
	num_layers = PCB_PASTEBUFFER->Data->LayerN;
	if (num_layers != 0) /* some buffers don't have layers, just global objects */
	for (i = 0; i < num_layers; i++) {
		pcb_layer_t *sourcelayer = &PCB_PASTEBUFFER->Data->Layer[i];
		pcb_layer_t *destlayer = pcb_layer_resolve_binding(pcb, sourcelayer);

		if (destlayer == NULL) {
			if ((!(sourcelayer->meta.bound.type & PCB_LYT_VIRTUAL)) && (!pcb_layer_is_pure_empty(sourcelayer))) {
				const char *src_name = sourcelayer->name;
				if ((src_name == NULL) || (*src_name == '\0'))
					src_name = "<anonymous>";
				pcb_message(PCB_MSG_WARNING, "Couldn't resolve buffer layer #%d (%s) on the current board\n", i, src_name);
			}
			continue;
		}

		destlayer = pcb_loose_subc_layer(PCB, destlayer, pcb_true);

		if (destlayer->meta.real.vis) {
			PCB_LINE_LOOP(sourcelayer);
			{
				if (pcb_lineop_copy(&ctx, destlayer, line))
					changed = 1;
			}
			PCB_END_LOOP;
			PCB_ARC_LOOP(sourcelayer);
			{
				if (pcb_arcop_copy(&ctx, destlayer, arc))
					changed = 1;
			}
			PCB_END_LOOP;
			PCB_TEXT_LOOP(sourcelayer);
			{
				if (pcb_textop_copy(&ctx, destlayer, text))
					changed = 1;
			}
			PCB_END_LOOP;
			PCB_POLY_LOOP(sourcelayer);
			{
				if (pcb_polyop_copy(&ctx, destlayer, polygon))
					changed = 1;
			}
			PCB_END_LOOP;
		}
	}

	/* paste subcircuits */
	PCB_SUBC_LOOP(PCB_PASTEBUFFER->Data);
	{
		if (pcb->is_footprint) {
			pcb_message(PCB_MSG_WARNING, "Can not paste subcircuit in the footprint edit mode\n");
			break;
		}
		pcb_subcop_copy(&ctx, subc);
		changed = pcb_true;
	}
	PCB_END_LOOP;

	/* finally: padstacks */
	if (pcb->pstk_on) {
		pcb_board_t dummy;
		changed |= (padstacklist_length(&(PCB_PASTEBUFFER->Data->padstack)) != 0);

		/* set up a dummy board to work around that pcb_pstkop_copy() requires a
		   pcb_board_t instead of a pcb_data_t */
TODO("subc: fix this after the element removal")
		if (pcb->is_footprint) {
			pcb_subc_t *sc = pcb_subclist_first(&pcb->Data->subc);
			if (sc != NULL) {
				ctx.copy.pcb = &dummy;
				memset(&dummy, 0, sizeof(dummy));
				dummy.Data = sc->data;
			}
		}

		PCB_PADSTACK_LOOP(PCB_PASTEBUFFER->Data);
		{
			pcb_pstkop_copy(&ctx, padstack);
		}
		PCB_END_LOOP;
	}

	if (changed) {
		pcb_subc_as_board_update(PCB);
		pcb_draw();
		pcb_undo_inc_serial();
	}

#ifdef DEBUG
	printf("  .... Leaving CopyPastebufferToLayout.\n");
#endif

	pcb_data_clip_inhibit_dec(pcb->Data, pcb_true);

	return changed;
}

void pcb_buffer_set_number(int Number)
{
	if (Number >= 0 && Number < PCB_MAX_BUFFER) {
		conf_set_design("editor/buffer_number", "%d", Number);

		/* do an update on the crosshair range */
		pcb_crosshair_range_to_buffer();
	}
}

/* loads footprint data from file/library into buffer (as subcircuit)
 * returns pcb_false on error
 * if successful, update some other stuff and reposition the pastebuffer */
pcb_bool pcb_buffer_load_footprint(pcb_buffer_t *Buffer, const char *Name, const char *fmt)
{
	pcb_buffer_clear(PCB, Buffer);
	if (!pcb_parse_footprint(Buffer->Data, Name, fmt)) {
		if (conf_core.editor.show_solder_side)
			pcb_buffer_flip_side(PCB, Buffer);
		pcb_set_buffer_bbox(Buffer);

		Buffer->X = 0;
		Buffer->Y = 0;
		Buffer->from_outside = 1;

		if (pcb_subclist_length(&Buffer->Data->subc)) {
			pcb_subc_t *subc = pcb_subclist_first(&Buffer->Data->subc);
			pcb_subc_get_origin(subc, &Buffer->X, &Buffer->Y);
		}
		return pcb_true;
	}

	/* release memory which might have been acquired */
	pcb_buffer_clear(PCB, Buffer);
	return pcb_false;
}


static const char pcb_acts_PasteBuffer[] =
	"PasteBuffer(AddSelected|Clear|1..PCB_MAX_BUFFER)\n"
	"PasteBuffer(Rotate, 1..3)\n"
	"PasteBuffer(Convert|Restore|Mirror)\n"
	"PasteBuffer(ToLayout, X, Y, units)\n"
	"PasteBuffer(ToLayout, crosshair)\n"
	"PasteBuffer(Save, Filename, [format], [force])\n"
	"PasteBuffer(Push)\n"
	"PasteBuffer(Pop)\n"
	;
static const char pcb_acth_PasteBuffer[] = "Various operations on the paste buffer.";
/* DOC: pastebuffer.html */
static fgw_error_t pcb_act_PasteBuffer(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op, force;
	const char *sbufnum = "", *fmt = NULL, *forces = NULL, *name, *tmp;
	static char *default_file = NULL;
	pcb_bool free_name = pcb_false;
	static int stack[32];
	static int sp = 0;
	int number;

	PCB_ACT_CONVARG(1, FGW_STR, PasteBuffer, tmp = argv[1].val.str);
	number = atoi(tmp);
	PCB_ACT_CONVARG(1, FGW_KEYWORD, PasteBuffer, op = fgw_keyword(&argv[1]));
	PCB_ACT_MAY_CONVARG(2, FGW_STR, PasteBuffer, sbufnum = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, PasteBuffer, fmt = argv[3].val.str);
	PCB_ACT_MAY_CONVARG(4, FGW_STR, PasteBuffer, forces = argv[4].val.str);

	force = (forces != NULL) && ((*forces == '1') || (*forces == 'y') || (*forces == 'Y'));

	pcb_notify_crosshair_change(pcb_false);
	switch (op) {
			/* clear contents of paste buffer */
		case F_Clear:
			pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
			break;

		case F_Push:
			if (sp < sizeof(stack) / sizeof(stack[0]))
				stack[sp++] = conf_core.editor.buffer_number;
			else
				pcb_message(PCB_MSG_ERROR, "Paste buffer stack overflow on push.\n");
			break;

		case F_Pop:
			if (sp > 0)
				pcb_buffer_set_number(stack[--sp]);
			else
				pcb_message(PCB_MSG_ERROR, "Paste buffer stack underflow on pop.\n");
			break;

			/* copies objects to paste buffer */
		case F_AddSelected:
			pcb_buffer_add_selected(PCB, PCB_PASTEBUFFER, 0, 0, pcb_false);
			if (pcb_data_is_empty(PCB_PASTEBUFFER->Data)) {
				pcb_message(PCB_MSG_WARNING, "Nothing buffer-movable is selected, nothing copied to the paste buffer\n");
				goto error;
			}
			break;

			/* moves objects to paste buffer: kept for compatibility with old menu files */
		case F_MoveSelected:
			pcb_buffer_add_selected(PCB, PCB_PASTEBUFFER, 0, 0, pcb_false);
			if (pcb_data_is_empty(PCB_PASTEBUFFER->Data)) {
				pcb_message(PCB_MSG_WARNING, "Nothing buffer-movable is selected, nothing moved to the paste buffer\n");
				goto error;
			}
			pcb_actionl("RemoveSelected", NULL);
			break;

			/* converts buffer contents into a subcircuit */
		case F_Convert:
		case F_ConvertSubc:
			pcb_subc_convert_from_buffer(PCB_PASTEBUFFER);
			break;

			/* break up subcircuit for editing */
		case F_Restore:
			if (!pcb_subc_smash_buffer(PCB_PASTEBUFFER))
				pcb_message(PCB_MSG_ERROR, "Error breaking up subcircuits - only one can be broken up at a time\n");
			break;

			/* Mirror buffer */
		case F_Mirror:
			pcb_buffer_mirror(PCB, PCB_PASTEBUFFER);
			break;

		case F_Rotate:
			if (sbufnum) {
				int numtmp = atoi(sbufnum);
				if (numtmp < 0)
					numtmp = 4-numtmp;
				pcb_buffer_rotate90(PCB_PASTEBUFFER, (unsigned int)numtmp);
				pcb_crosshair_range_to_buffer();
			}
			break;

		case F_Save:
			free_name = pcb_false;
			if (argc <= 1) {
				name = pcb_gui->fileselect("Save Paste Buffer As ...",
															 "Choose a file to save the contents of the\n"
																 "paste buffer to.\n", default_file, ".fp", NULL, "footprint", 0, NULL);

				if (default_file) {
					free(default_file);
					default_file = NULL;
				}
				if (name && *name) {
					default_file = pcb_strdup(name);
				}
				free_name = pcb_true;
			}

			else
				name = sbufnum;

			{
				FILE *exist;

				if ((!force) && ((exist = pcb_fopen(&PCB->hidlib, name, "r")))) {
					fclose(exist);
					if (pcb_hid_message_box("warning", "Buffer: overwrite file", "File exists!  Ok to overwrite?", "cancel", 0, "yes", 1, NULL) == 1)
						pcb_save_buffer_elements(name, fmt);
				}
				else
					pcb_save_buffer_elements(name, fmt);

				if (free_name && name)
					free((char*)name);
			}
			break;

		case F_ToLayout:
			{
				static pcb_coord_t oldx = 0, oldy = 0;
				pcb_coord_t x, y;
				pcb_bool absolute;
				if (argc == 2) {
					x = y = 0;
				}
				else if (strcmp(sbufnum, "crosshair") == 0) {
					x = pcb_crosshair.X;
					y = pcb_crosshair.Y;
				}
				else if (argc == 4 || argc == 5) {
					x = pcb_get_value(sbufnum, forces, &absolute, NULL);
					if (!absolute)
						x += oldx;
					y = pcb_get_value(fmt, forces, &absolute, NULL);
					if (!absolute)
						y += oldy;
				}

				else {
					pcb_notify_crosshair_change(pcb_true);
					PCB_ACT_FAIL(PasteBuffer);
				}

				oldx = x;
				oldy = y;
				if (pcb_buffer_copy_to_layout(PCB, x, y))
					pcb_board_set_changed_flag(pcb_true);
			}
			break;

		case F_Normalize:
			pcb_set_buffer_bbox(PCB_PASTEBUFFER);
			PCB_PASTEBUFFER->X = pcb_round(((double)PCB_PASTEBUFFER->BoundingBox.X1 + (double)PCB_PASTEBUFFER->BoundingBox.X2) / 2.0);
			PCB_PASTEBUFFER->Y = pcb_round(((double)PCB_PASTEBUFFER->BoundingBox.Y1 + (double)PCB_PASTEBUFFER->BoundingBox.Y2) / 2.0);
			pcb_crosshair_range_to_buffer();
			break;

			/* set number */
		default:
			{
				

				/* correct number */
				if (number)
					pcb_buffer_set_number(number - 1);
			}
	}

	pcb_notify_crosshair_change(pcb_true);
	PCB_ACT_IRES(0);
	return 0;

	error:;
	pcb_notify_crosshair_change(pcb_true);
	PCB_ACT_IRES(-1);
	return 0;
}

/* --------------------------------------------------------------------------- */

pcb_action_t buffer_action_list[] = {
	{"FreeRotateBuffer", pcb_act_FreeRotateBuffer, pcb_acth_FreeRotateBuffer, pcb_acts_FreeRotateBuffer},
	{"ScaleBuffer", pcb_act_ScaleBuffer, pcb_acth_ScaleBuffer, pcb_acts_ScaleBuffer},
	{"LoadFootprint", pcb_act_LoadFootprint, pcb_acth_LoadFootprint, pcb_acts_LoadFootprint},
	{"PasteBuffer", pcb_act_PasteBuffer, pcb_acth_PasteBuffer, pcb_acts_PasteBuffer}
};

PCB_REGISTER_ACTIONS(buffer_action_list, NULL)

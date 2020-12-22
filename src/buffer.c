/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
#include <librnd/core/compat_misc.h>
#include <librnd/core/pixmap.h>
#include "obj_arc_op.h"
#include "obj_line_op.h"
#include "obj_text_op.h"
#include "obj_subc_op.h"
#include "obj_poly_op.h"
#include "obj_pstk_op.h"
#include "obj_gfx_op.h"
#include "obj_rat_op.h"
#include "obj_pstk.h"
#include "layer_grp.h"
#include <librnd/core/event.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/actions.h>
#include <librnd/core/tool.h>
#include "extobj.h"
#include <librnd/core/hid_dad.h>

static int move_buffer_pre(pcb_opctx_t *ctx, pcb_any_obj_t *ptr2, void *ptr3);
static void move_buffer_post(pcb_opctx_t *ctx, pcb_any_obj_t *ptr2, void *ptr3);
static int add_buffer_pre(pcb_opctx_t *ctx, pcb_any_obj_t *ptr2, void *ptr3);

static pcb_opfunc_t AddBufferFunctions = {
	add_buffer_pre,
	NULL, /* common_post */
	pcb_lineop_add_to_buffer,
	pcb_textop_add_to_buffer,
	pcb_polyop_add_to_buffer,
	NULL,
	NULL,
	pcb_arcop_add_to_buffer,
	pcb_gfxop_add_to_buffer,
	pcb_ratop_add_to_buffer,
	NULL,
	pcb_subcop_add_to_buffer,
	pcb_pstkop_add_to_buffer,
	0 /* extobj_inhibit_regen */
};

static pcb_opfunc_t MoveBufferFunctions = {
	move_buffer_pre,
	move_buffer_post,
	pcb_lineop_move_buffer,
	pcb_textop_move_buffer,
	pcb_polyop_move_buffer,
	NULL,
	NULL,
	pcb_arcop_move_buffer,
	pcb_gfxop_move_buffer,
	pcb_ratop_move_buffer,
	NULL,
	pcb_subcop_move_buffer,
	pcb_pstkop_move_buffer,
	0 /* extobj_inhibit_regen */
};

static int move_buffer_pre(pcb_opctx_t *ctx, pcb_any_obj_t *obj, void *ptr3)
{
	ctx->buffer.post_subc = pcb_extobj_get_floater_subc(obj);

	if (ctx->buffer.removing)
		return 0; /* don't do anything extobj-special for removing, it's already been done */

	/* when an edit-object is moved to buffer, the corresponding subc obj needs to be moved too */
	if (ctx->buffer.post_subc != NULL) {
		pcb_subcop_move_buffer(ctx, ctx->buffer.post_subc);
		return 1;
	}
	return 0;
}

static void move_buffer_post(pcb_opctx_t *ctx, pcb_any_obj_t *obj, void *ptr3)
{
	if ((!ctx->buffer.removing) || (ctx->buffer.post_subc == NULL))
		return;

	/* when an edit-object is moved to buffer, the corresponding subc obj needs to be moved too */
	if (ctx->buffer.post_subc != NULL) {
		pcb_extobj_subc_geo(ctx->buffer.post_subc);
		ctx->buffer.post_subc = NULL;
	}
}

static int add_buffer_pre(pcb_opctx_t *ctx, pcb_any_obj_t *obj, void *ptr3)
{
	pcb_subc_t *subc =  pcb_extobj_get_floater_subc(obj);

	/* when an edit-object is moved to buffer, the corresponding subc obj needs to be moved too */
	if (subc != NULL) {
		/* make sure the same subc is not copied twice by detecting a copy already done from the same subc to the same buffer */
		pcb_subc_t *bsc;
		gdl_iterator_t it;
		subclist_foreach(&ctx->buffer.dst->subc, &it, bsc) {
			if (bsc->copied_from_ID == subc->ID)
				return 1;
		}
		pcb_subcop_add_to_buffer(ctx, subc);
		return 1;
	}
	return 0;

}

int pcb_set_buffer_bbox(pcb_buffer_t *Buffer)
{
	rnd_box_t tmp, *box;
	int res = 0;
	
	box = pcb_data_bbox(&tmp, Buffer->Data, rnd_false);
	if (box)
		Buffer->BoundingBox = *box;
	else
		res = -1;

	box = pcb_data_bbox_naked(&tmp, Buffer->Data, rnd_false);
	if (box)
		Buffer->bbox_naked = *box;
	else
		res = -1;

	return res;
}

static void pcb_buffer_clear_(pcb_board_t *pcb, pcb_buffer_t *Buffer, rnd_bool bind)
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
	free(Buffer->source_path); Buffer->source_path = NULL;
	Buffer->X = Buffer->Y = 0;
}

void pcb_buffer_clear(pcb_board_t *pcb, pcb_buffer_t *Buffer)
{
	pcb_buffer_clear_(pcb, Buffer, rnd_true);
}

/* add (or move) selected */
static void pcb_buffer_toss_selected(pcb_opfunc_t *fnc, pcb_board_t *pcb, pcb_buffer_t *Buffer, rnd_coord_t X, rnd_coord_t Y, rnd_bool LeaveSelected, rnd_bool on_locked_too, rnd_bool keep_id)
{
	rnd_hidlib_t *hidlib = (rnd_hidlib_t *)pcb;
	pcb_opctx_t ctx;

	ctx.buffer.pcb = pcb;
	ctx.buffer.keep_id = keep_id;
	ctx.buffer.removing = 0;

	/* switch crosshair off because adding objects to the pastebuffer
	   may change the 'valid' area for the cursor */
	if (!LeaveSelected)
		ctx.buffer.extraflg = PCB_FLAG_SELECTED;
	else
		ctx.buffer.extraflg =  0;

	rnd_hid_notify_crosshair_change(hidlib, rnd_false);
	ctx.buffer.src = pcb->Data;
	ctx.buffer.dst = Buffer->Data;
	pcb_selected_operation(pcb, pcb->Data, fnc, &ctx, rnd_false, PCB_OBJ_ANY & (~PCB_OBJ_SUBC_PART), (on_locked_too ? PCB_OP_ON_LOCKED : 0) | PCB_OP_NO_SUBC_PART);

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
	free(Buffer->source_path); Buffer->source_path = NULL;
	rnd_hid_notify_crosshair_change(hidlib, rnd_true);
}

void pcb_buffer_add_selected(pcb_board_t *pcb, pcb_buffer_t *Buffer, rnd_coord_t X, rnd_coord_t Y, rnd_bool LeaveSelected, rnd_bool keep_id)
{
	pcb_buffer_toss_selected(&AddBufferFunctions, pcb, Buffer, X, Y, LeaveSelected, rnd_false, keep_id);
}

extern pcb_opfunc_t pcb_RemoveFunctions;
void pcb_buffer_move_selected(pcb_board_t *pcb, pcb_buffer_t *Buffer, rnd_coord_t X, rnd_coord_t Y, rnd_bool LeaveSelected, rnd_bool keep_id)
{
	pcb_buffer_toss_selected(&AddBufferFunctions, pcb, Buffer, X, Y, LeaveSelected, rnd_false, keep_id);
	pcb_buffer_toss_selected(&pcb_RemoveFunctions, pcb, Buffer, X, Y, LeaveSelected, rnd_false, keep_id);
	pcb_undo_inc_serial();
}

static const char pcb_acts_LoadFootprint[] = "LoadFootprint(filename[,refdes,value])";
static const char pcb_acth_LoadFootprint[] = "Loads a single footprint by name.";
/* DOC: loadfootprint.html */
fgw_error_t pcb_act_LoadFootprint(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *name, *refdes = NULL, *value = NULL;
	pcb_subc_t *s;
	rnd_cardinal_t len;

	
	RND_ACT_CONVARG(1, FGW_STR, LoadFootprint, name = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, LoadFootprint, refdes = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, LoadFootprint, value = argv[3].val.str);

	if (!pcb_buffer_load_footprint(PCB_PASTEBUFFER, name, NULL)) {
		RND_ACT_IRES(1);
		return 0;
	}

	len = pcb_subclist_length(&PCB_PASTEBUFFER->Data->subc);

	if (len == 0) {
		rnd_message(RND_MSG_ERROR, "Footprint %s contains no subcircuits", name);
		RND_ACT_IRES(1);
		return 0;
	}
	if (len > 1) {
		rnd_message(RND_MSG_ERROR, "Footprint %s contains multiple subcircuits", name);
		RND_ACT_IRES(1);
		return 0;
	}

	s = pcb_subclist_first(&PCB_PASTEBUFFER->Data->subc);
	pcb_attribute_put(&s->Attributes, "refdes", refdes);
	pcb_attribute_put(&s->Attributes, "footprint", name);
	pcb_attribute_put(&s->Attributes, "value", value);

	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_LoadPixmap[] = "LoadPixmap([filename])";
static const char pcb_acth_LoadPixmap[] = "Loads a pixmap image from disk and creates a gfx object in buffer.";
fgw_error_t pcb_act_LoadPixmap(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fn = NULL;
	double dpi = 600.0;
	rnd_pixmap_t *pxm;
	pcb_gfx_t *g;

	RND_ACT_MAY_CONVARG(1, FGW_STR, LoadPixmap, fn = argv[1].val.str);

	if (fn == NULL) {
		static char *default_file = NULL;
		char *nfn;
		nfn = rnd_gui->fileselect(rnd_gui, "Load pixmap into buffer gfx...",
			"Choose a file to load the pixmap\nfor the gfx object to be created\nin the buffer.\n",
			default_file, ".*", NULL, "gfx", RND_HID_FSD_READ | RND_HID_FSD_MAY_NOT_EXIST, NULL);
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
		if (nfn == NULL) { /* cancel */
			RND_ACT_IRES(-1);
			return 0;
		}
		fn = default_file = nfn;
	}

	pxm = rnd_pixmap_load(RND_ACT_HIDLIB, fn);
	if (pxm == NULL) {
		rnd_message(RND_MSG_ERROR, "Failed to load pixmap from '%s' - is that file a pixmap in any of the supported formats?\n", fn);
		RND_ACT_IRES(-1);
		return 0;
	}

	g = pcb_gfx_new(&PCB_PASTEBUFFER->Data->Layer[PCB_CURRLID(pcb)], 0, 0,
		RND_MIL_TO_COORD((double)pxm->sx / dpi * 1000.0), RND_MIL_TO_COORD((double)pxm->sy / dpi * 1000.0),
		0, pcb_flag_make(0));

	pcb_gfx_set_pixmap_free(g, pxm, 0);

	RND_ACT_IRES(0);
	return 0;
}

rnd_bool pcb_buffer_load_layout(pcb_board_t *pcb, pcb_buffer_t *Buffer, const char *Filename, const char *fmt)
{
	pcb_board_t *orig, *newPCB = pcb_board_new_(rnd_false);

	orig = PCB;
	PCB = newPCB;
	pcb_layergrp_inhibit_inc();

	/* new data isn't added to the undo list */
	if (!pcb_parse_pcb(newPCB, Filename, fmt, RND_CFR_invalid, 0)) {
		pcb_data_t *tmpdata;

		/* clear data area and replace pointer */
		pcb_buffer_clear(pcb, Buffer);
		tmpdata = Buffer->Data;
		Buffer->Data = newPCB->Data;
		newPCB->Data = tmpdata;
		Buffer->X = 0;
		Buffer->Y = 0;
		PCB_CLEAR_PARENT(Buffer->Data);
		pcb_data_make_layers_bound(newPCB, Buffer->Data);
		pcb_data_binding_update(pcb, Buffer->Data);
		pcb_board_free(newPCB);
		free(newPCB);
		Buffer->from_outside = 0; /* always place matching top-to-top, don't swap sides only because the user is viewing the board from the bottom */
		free(Buffer->source_path); Buffer->source_path = rnd_strdup(Filename);
		PCB = orig;
		pcb_layergrp_inhibit_dec();
		return rnd_true;
	}

	/* release unused memory */
	pcb_board_free(newPCB);
	free(newPCB);
	if (Buffer->Data != NULL)
		PCB_CLEAR_PARENT(Buffer->Data);
	PCB = orig;
	pcb_layergrp_inhibit_dec();
	Buffer->from_outside = 0;
	free(Buffer->source_path); Buffer->source_path = NULL;
	return rnd_false;
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
		pcb_obj_rotate90(PCB, (pcb_any_obj_t *)subc, Buffer->X, Buffer->Y, Number);
	}
	PCB_END_LOOP;

	/* all layer related objects */
	PCB_LINE_ALL_LOOP(Buffer->Data);
	{
		if (layer->line_tree != NULL)
			rnd_r_delete_entry(layer->line_tree, (rnd_box_t *) line);
		pcb_line_rotate90(line, Buffer->X, Buffer->Y, Number);
		if (layer->line_tree != NULL)
			rnd_r_insert_entry(layer->line_tree, (rnd_box_t *) line);
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(Buffer->Data);
	{
		if (layer->arc_tree != NULL)
			rnd_r_delete_entry(layer->arc_tree, (rnd_box_t *) arc);
		pcb_arc_rotate90(arc, Buffer->X, Buffer->Y, Number);
		if (layer->arc_tree != NULL)
			rnd_r_insert_entry(layer->arc_tree, (rnd_box_t *) arc);
	}
	PCB_ENDALL_LOOP;
	PCB_TEXT_ALL_LOOP(Buffer->Data);
	{
		if (layer->text_tree != NULL)
			rnd_r_delete_entry(layer->text_tree, (rnd_box_t *) text);
		pcb_text_rotate90(text, Buffer->X, Buffer->Y, Number);
		if (layer->text_tree != NULL)
			rnd_r_insert_entry(layer->text_tree, (rnd_box_t *) text);
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_ALL_LOOP(Buffer->Data);
	{
		if (layer->polygon_tree != NULL)
			rnd_r_delete_entry(layer->polygon_tree, (rnd_box_t *) polygon);
		pcb_poly_rotate90(polygon, Buffer->X, Buffer->Y, Number);
		if (layer->polygon_tree != NULL)
			rnd_r_insert_entry(layer->polygon_tree, (rnd_box_t *) polygon);
	}
	PCB_ENDALL_LOOP;

	/* finally the origin and the bounding box */
	RND_COORD_ROTATE90(Buffer->X, Buffer->Y, Buffer->X, Buffer->Y, Number);
	rnd_box_rotate90(&Buffer->BoundingBox, Buffer->X, Buffer->Y, Number);

	pcb_undo_unfreeze_add();
	pcb_undo_unfreeze_serial();
}

void pcb_buffer_rotate(pcb_buffer_t *Buffer, rnd_angle_t angle)
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

	RND_ACT_MAY_CONVARG(1, FGW_STR, FreeRotateBuffer, angle_s = rnd_strdup(argv[1].val.str));
	RND_ACT_IRES(0);

	if (angle_s == NULL)
		angle_s = rnd_hid_prompt_for(RND_ACT_HIDLIB, "Enter Rotation (degrees, CCW):", "0", "Rotation angle");

	if ((angle_s == NULL) || (*angle_s == '\0')) {
		free(angle_s);
		RND_ACT_IRES(-1);
		return 0;
	}

	RND_ACT_IRES(0);
	ang = strtod(angle_s, 0);
	free(angle_s);
	if (ang == 0) {
		RND_ACT_IRES(-1);
		return 0;
	}

	if ((ang < -360000) || (ang > +360000)) {
		rnd_message(RND_MSG_ERROR, "Angle too large\n");
		RND_ACT_IRES(-1);
		return 0;
	}

	rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_false);
	pcb_buffer_rotate(PCB_PASTEBUFFER, ang);
	rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_true);
	return 0;
}

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	int wx, wy, wlock, wth, wrecurse;
} scale_t;

/* make sure x;y values are synced when lock is active */
static void scale_chg_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	scale_t *ctx = caller_data;
	int wid = attr - ctx->dlg, locked = ctx->dlg[ctx->wlock].val.lng;
	rnd_hid_attr_val_t hv;

	if (!locked)
		return;

	if (wid == ctx->wy) { /* copy y into x */
		hv.dbl = ctx->dlg[ctx->wy].val.dbl;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wx, &hv);
	}
	else { /* copy x into y */
		hv.dbl = ctx->dlg[ctx->wx].val.dbl;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wy, &hv);
	}
}

/* returns 0 on OK */
static int pcb_actgui_ScaleBuffer(rnd_hidlib_t *hidlib, double *x, double *y, double *th, int *recurse)
{
	scale_t ctx;
	int res;

	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {"Apply", 0}, {NULL, 0}};

	memset(&ctx, 0, sizeof(ctx));
	RND_DAD_BEGIN_VBOX(ctx.dlg);
		RND_DAD_COMPFLAG(ctx.dlg, RND_HATF_EXPFILL);
		RND_DAD_BEGIN_TABLE(ctx.dlg, 2);
			RND_DAD_COMPFLAG(ctx.dlg, RND_HATF_EXPFILL);

			RND_DAD_LABEL(ctx.dlg, "scale X");
			RND_DAD_REAL(ctx.dlg);
				ctx.wx = RND_DAD_CURRENT(ctx.dlg);
				RND_DAD_DEFAULT_NUM(ctx.dlg, 1);
				RND_DAD_CHANGE_CB(ctx.dlg, scale_chg_cb);

			RND_DAD_LABEL(ctx.dlg, "same");
			RND_DAD_BOOL(ctx.dlg);
				ctx.wlock = RND_DAD_CURRENT(ctx.dlg);
				RND_DAD_DEFAULT_NUM(ctx.dlg, 1);
				RND_DAD_CHANGE_CB(ctx.dlg, scale_chg_cb);

			RND_DAD_LABEL(ctx.dlg, "scale Y");
			RND_DAD_REAL(ctx.dlg);
				ctx.wy = RND_DAD_CURRENT(ctx.dlg);
				RND_DAD_DEFAULT_NUM(ctx.dlg, 1);
				RND_DAD_CHANGE_CB(ctx.dlg, scale_chg_cb);

			RND_DAD_LABEL(ctx.dlg, "scale thickness");
			RND_DAD_REAL(ctx.dlg);
				ctx.wth = RND_DAD_CURRENT(ctx.dlg);
				RND_DAD_DEFAULT_NUM(ctx.dlg, 1);

			RND_DAD_LABEL(ctx.dlg, "subcircuits");
			RND_DAD_BOOL(ctx.dlg);
				ctx.wrecurse = RND_DAD_CURRENT(ctx.dlg);
				RND_DAD_DEFAULT_NUM(ctx.dlg, 0);

		RND_DAD_BUTTON_CLOSES(ctx.dlg, clbtn);
	RND_DAD_END(ctx.dlg);

	RND_DAD_NEW("buffer_scale", ctx.dlg, "Buffer scale factors", &ctx, rnd_true, NULL);
	res = RND_DAD_RUN(ctx.dlg);
	*x = ctx.dlg[ctx.wx].val.dbl;
	*y = ctx.dlg[ctx.wy].val.dbl;
	*th = ctx.dlg[ctx.wth].val.dbl;
	*recurse = ctx.dlg[ctx.wrecurse].val.lng;
	RND_DAD_FREE(ctx.dlg);
	return res;
}

static const char pcb_acts_ScaleBuffer[] = "ScaleBuffer(x [,y [,thickness [,subc]]])";
static const char pcb_acth_ScaleBuffer[] =
	"Scales the buffer by multiplying all coordinates by a floating point number.\n"
	"If only x is given, it is also used for y and thickness too. If subc is not\n"
	"empty, subcircuits are also scaled\n";
fgw_error_t pcb_act_ScaleBuffer(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *sx = NULL;
	double x, y, th;
	int recurse = 0;
	char *end;

	RND_ACT_MAY_CONVARG(1, FGW_STR, ScaleBuffer, sx = argv[1].val.str);

	if (sx != NULL) {
		x = strtod(sx, &end);
		if (*end != '\0') {
			RND_ACT_IRES(-1);
			return 0;
		}
		y = th = x;
	}
	else {
		if (pcb_actgui_ScaleBuffer(RND_ACT_HIDLIB, &x, &y, &th, &recurse) != 0) {
			RND_ACT_IRES(-1);
			return 0;
		}
	}

	RND_ACT_MAY_CONVARG(2, FGW_DOUBLE, ScaleBuffer, y = argv[2].val.nat_double);
	RND_ACT_MAY_CONVARG(3, FGW_DOUBLE, ScaleBuffer, th = argv[3].val.nat_double);
	RND_ACT_MAY_CONVARG(4, FGW_STR, ScaleBuffer, recurse = (argv[4].val.str != NULL));

	if ((x <= 0) || (y <= 0)) {
		RND_ACT_IRES(-1);
		return 0;
	}

	RND_ACT_IRES(0);

	rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_false);
	pcb_buffer_scale(PCB_PASTEBUFFER, x, y, th, recurse);
	rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_true);
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
		pcb_buffer_clear_(pcb, pcb_buffers+i, rnd_false);
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
			rnd_message(RND_MSG_ERROR, "You can't mirror a buffer that has text!\n");
			return;
		}
	}
	/* set buffer offset to 'mark' position */
	Buffer->X = PCB_SWAP_X(Buffer->X);
	Buffer->Y = PCB_SWAP_Y(Buffer->Y);
	pcb_undo_freeze_add();
	pcb_data_mirror(Buffer->Data, 0, PCB_TXM_COORD, rnd_false, 0);
	pcb_undo_unfreeze_add();
	pcb_set_buffer_bbox(Buffer);
}

void pcb_buffer_scale(pcb_buffer_t *Buffer, double sx, double sy, double sth, int recurse)
{
	pcb_data_scale(Buffer->Data, sx, sy, sth, recurse);
	Buffer->X = rnd_round((double)Buffer->X * sx);
	Buffer->Y = rnd_round((double)Buffer->Y * sy);
	pcb_set_buffer_bbox(Buffer);
}

void pcb_buffer_flip_side(pcb_board_t *pcb, pcb_buffer_t *Buffer)
{
#if 0
/* this results in saving flipped (bottom-side) footprints whenlooking at the board from the bottom */
	PCB_SUBC_LOOP(Buffer->Data);
	{
		pcb_subc_change_side(subc, /*2 * pcb_crosshair.Y - pcb->hidlib.size_y*/0);
	}
	PCB_END_LOOP;
#endif

	/* set buffer offset to 'mark' position */
	Buffer->X = PCB_SWAP_X(Buffer->X);
	Buffer->Y = PCB_SWAP_Y(Buffer->Y);

	PCB_PADSTACK_LOOP(Buffer->Data);
	{
		pcb_pstk_mirror(padstack, PCB_PSTK_DONT_MIRROR_COORDS, 1, 0, 0);
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
		pcb_text_flip_side(layer, text, 0, 0);
	}
	PCB_ENDALL_LOOP;

	pcb_set_buffer_bbox(Buffer);
}

void pcb_buffers_flip_side(pcb_board_t *pcb)
{
	int i;

	for (i = 0; i < PCB_MAX_BUFFER; i++)
		pcb_buffer_flip_side(pcb, &pcb_buffers[i]);
}

void *pcb_move_obj_to_buffer(pcb_board_t *pcb, pcb_data_t *Destination, pcb_data_t *Src, int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;

	/* setup local identifiers used by move operations */
	ctx.buffer.pcb = pcb;
	ctx.buffer.dst = Destination;
	ctx.buffer.src = Src;
	ctx.buffer.removing = (Destination == pcb_removelist);
	return (pcb_object_operation(&MoveBufferFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3));
}

void *pcb_copy_obj_to_buffer(pcb_board_t *pcb, pcb_data_t *Destination, pcb_data_t *Src, int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;

	ctx.buffer.pcb = pcb;
	ctx.buffer.dst = Destination;
	ctx.buffer.src = Src;
	ctx.buffer.removing = 0;
	return (pcb_object_operation(&AddBufferFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3));
}

rnd_bool pcb_buffer_copy_to_layout(pcb_board_t *pcb, rnd_coord_t X, rnd_coord_t Y, rnd_bool keep_id)
{
	rnd_cardinal_t i;
	rnd_bool changed = rnd_false;
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
	ctx.copy.keep_id = keep_id;

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
				rnd_message(RND_MSG_WARNING, "Couldn't resolve buffer layer #%d (%s) on the current board\n", i, src_name);
			}
			continue;
		}

		destlayer = pcb_loose_subc_layer(PCB, destlayer, rnd_true);

		if (destlayer->meta.real.vis) {
			PCB_LINE_LOOP(sourcelayer);
			{
				pcb_line_t *nline = pcb_lineop_copy(&ctx, destlayer, line);
				if (nline != NULL) {
					if (keep_id)
						pcb_extobj_float_geo((pcb_any_obj_t *)nline);
					changed = 1;
				}
			}
			PCB_END_LOOP;
			PCB_ARC_LOOP(sourcelayer);
			{
				pcb_arc_t *narc = pcb_arcop_copy(&ctx, destlayer, arc);
				if (narc != NULL) {
					if (keep_id)
						pcb_extobj_float_geo((pcb_any_obj_t *)narc);
					changed = 1;
				}
			}
			PCB_END_LOOP;
			PCB_TEXT_LOOP(sourcelayer);
			{
				pcb_text_t *ntext = pcb_textop_copy(&ctx, destlayer, text);
				if (ntext != NULL) {
					if (keep_id)
						pcb_extobj_float_geo((pcb_any_obj_t *)ntext);
					changed = 1;
				}
			}
			PCB_END_LOOP;
			PCB_POLY_LOOP(sourcelayer);
			{
				pcb_poly_t *npoly = pcb_polyop_copy(&ctx, destlayer, polygon);
				if (npoly != NULL) {
					if (keep_id)
						pcb_extobj_float_geo((pcb_any_obj_t *)npoly);
					changed = 1;
				}
			}
			PCB_END_LOOP;
			PCB_GFX_LOOP(sourcelayer);
			{
				pcb_gfx_t *ngfx = pcb_gfxop_copy(&ctx, destlayer, gfx);
				if (ngfx != NULL) {
					if (keep_id)
						pcb_extobj_float_geo((pcb_any_obj_t *)ngfx);
					changed = 1;
				}
			}
			PCB_END_LOOP;
		}
	}

	/* paste subcircuits */
	PCB_SUBC_LOOP(PCB_PASTEBUFFER->Data);
	{
		pcb_subc_t *nsubc;
		if (pcb->is_footprint) {
			rnd_message(RND_MSG_WARNING, "Can not paste subcircuit in the footprint edit mode\n");
			break;
		}
		nsubc = pcb_subcop_copy(&ctx, subc);
		if (nsubc != NULL) {
			if (keep_id)
				pcb_extobj_float_geo((pcb_any_obj_t *)nsubc);
			changed = 1;
		}
	}
	PCB_END_LOOP;

	/* finally: padstacks */
	if (pcb->pstk_on) {
		pcb_board_t dummy;
		changed |= (padstacklist_length(&(PCB_PASTEBUFFER->Data->padstack)) != 0);

		/* set up a dummy board to work around that pcb_pstkop_copy() requires a
		   pcb_board_t instead of a pcb_data_t */
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
			pcb_pstk_t *nps;
			nps = pcb_pstkop_copy(&ctx, padstack);
			if (nps != NULL) {
				if (keep_id)
					pcb_extobj_float_geo((pcb_any_obj_t *)nps);
				changed = 1;
			}
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

	pcb_data_clip_inhibit_dec(pcb->Data, rnd_true);

	return changed;
}

void pcb_buffer_set_number(int Number)
{
	if (Number >= 0 && Number < PCB_MAX_BUFFER)
		rnd_conf_set_design("editor/buffer_number", "%d", Number);
}

/* loads footprint data from file/library into buffer (as subcircuit)
 * returns rnd_false on error
 * if successful, update some other stuff and reposition the pastebuffer */
rnd_bool pcb_buffer_load_footprint(pcb_buffer_t *Buffer, const char *Name, const char *fmt)
{
	pcb_buffer_clear(PCB, Buffer);
	if (!pcb_parse_footprint(Buffer->Data, Name, fmt)) {
		if (conf_core.editor.show_solder_side)
			pcb_buffer_flip_side(PCB, Buffer);
		pcb_set_buffer_bbox(Buffer);

		Buffer->X = 0;
		Buffer->Y = 0;
		Buffer->from_outside = 1;
		free(Buffer->source_path); Buffer->source_path = rnd_strdup(Name);

		if (pcb_subclist_length(&Buffer->Data->subc)) {
			pcb_subc_t *subc = pcb_subclist_first(&Buffer->Data->subc);
			pcb_subc_get_origin(subc, &Buffer->X, &Buffer->Y);
		}

		/* the loader may have created new layers */
		pcb_data_binding_update(PCB, Buffer->Data);

		return rnd_true;
	}

	/* release memory which might have been acquired */
	pcb_buffer_clear(PCB, Buffer);
	return rnd_false;
}


static const char pcb_acts_PasteBuffer[] =
	"PasteBuffer(AddSelected|MoveSelected|Clear|1..PCB_MAX_BUFFER)\n"
	"PasteBuffer(Rotate, 1..3)\n"
	"PasteBuffer(Convert|Restore|Mirror)\n"
	"PasteBuffer(ToLayout, X, Y, units)\n"
	"PasteBuffer(ToLayout, crosshair)\n"
	"PasteBuffer(Save, Filename, [format], [force])\n"
	"PasteBuffer(SaveAll, Filename, [format])\n"
	"PasteBuffer(LoadAll, Filename)\n"
	"PasteBuffer(Push)\n"
	"PasteBuffer(Pop)\n"
	"PasteBuffer(GetSource, [1..PCB_MAX_BUFFER])\n"
	;
static const char pcb_acth_PasteBuffer[] = "Various operations on the paste buffer.";
/* DOC: pastebuffer.html */
static fgw_error_t pcb_act_PasteBuffer(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op, force;
	const char *sbufnum = "", *fmt = NULL, *forces = NULL, *name, *tmp;
	static char *default_file = NULL;
	rnd_bool free_name = rnd_false;
	static int stack[32];
	static int sp = 0;
	int number, rv;

	RND_ACT_CONVARG(1, FGW_STR, PasteBuffer, tmp = argv[1].val.str);
	number = atoi(tmp);
	RND_ACT_CONVARG(1, FGW_KEYWORD, PasteBuffer, op = fgw_keyword(&argv[1]));
	RND_ACT_MAY_CONVARG(2, FGW_STR, PasteBuffer, sbufnum = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, PasteBuffer, fmt = argv[3].val.str);
	RND_ACT_MAY_CONVARG(4, FGW_STR, PasteBuffer, forces = argv[4].val.str);

	force = (forces != NULL) && ((*forces == '1') || (*forces == 'y') || (*forces == 'Y'));

	rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_false);
	switch (op) {
			/* clear contents of paste buffer */
		case F_Clear:
			pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
			break;

		case F_Push:
			if (sp < sizeof(stack) / sizeof(stack[0]))
				stack[sp++] = conf_core.editor.buffer_number;
			else
				rnd_message(RND_MSG_ERROR, "Paste buffer stack overflow on push.\n");
			break;

		case F_Pop:
			if (sp > 0)
				pcb_buffer_set_number(stack[--sp]);
			else
				rnd_message(RND_MSG_ERROR, "Paste buffer stack underflow on pop.\n");
			break;

			/* copies objects to paste buffer */
		case F_AddSelected:
			pcb_buffer_add_selected(PCB, PCB_PASTEBUFFER, 0, 0, rnd_false, rnd_false);
			if (pcb_data_is_empty(PCB_PASTEBUFFER->Data)) {
				rnd_message(RND_MSG_WARNING, "Nothing buffer-movable is selected, nothing copied to the paste buffer\n");
				goto error;
			}
			break;

			/* cuts objects to paste buffer; it's different from AddSelected+RemoveSelected in extobj side effects */
		case F_MoveSelected:
			pcb_buffer_move_selected(PCB, PCB_PASTEBUFFER, 0, 0, rnd_false, rnd_false);
			if (pcb_data_is_empty(PCB_PASTEBUFFER->Data)) {
				rnd_message(RND_MSG_WARNING, "Nothing buffer-movable is selected, nothing moved to the paste buffer\n");
				goto error;
			}
			break;

			/* converts buffer contents into a subcircuit */
		case F_Convert:
		case F_ConvertSubc:
			pcb_subc_convert_from_buffer(PCB_PASTEBUFFER);
			break;

			/* break up subcircuit for editing */
		case F_Restore:
			if (!pcb_subc_smash_buffer(PCB_PASTEBUFFER))
				rnd_message(RND_MSG_ERROR, "Error breaking up subcircuits - only one can be broken up at a time\n");
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
			}
			break;

		case F_SaveAll:
			free_name = rnd_false;
			if (argc <= 2) {
				name = rnd_gui->fileselect(rnd_gui, "Save Paste Buffer As ...",
					"Choose a file to save the contents of the\npaste buffer to.\n",
					default_file, ".lht", NULL, "buffer", 0, NULL);

				if (default_file) {
					free(default_file);
					default_file = NULL;
				}
				if (name && *name)
					default_file = rnd_strdup(name);
				free_name = rnd_true;
			}
			else
				name = sbufnum;

			{
				FILE *exist;

				if ((!force) && ((exist = rnd_fopen(RND_ACT_HIDLIB, name, "r")))) {
					fclose(exist);
					if (rnd_hid_message_box(RND_ACT_HIDLIB, "warning", "Buffer: overwrite file", "File exists!  Ok to overwrite?", "cancel", 0, "yes", 1, NULL) == 1)
						pcb_save_buffer(name, fmt);
				}
				else
					pcb_save_buffer(name, fmt);

				if (free_name && name)
					free((char*)name);
			}
			break;

		case F_LoadAll:
			free_name = rnd_false;
			if (argc <= 2) {
				name = rnd_gui->fileselect(rnd_gui, "Load Paste Buffer ...",
					"Choose a file to load the contents of the\npaste buffer from.\n",
					default_file, ".lht", NULL, "buffer", RND_HID_FSD_READ | RND_HID_FSD_MAY_NOT_EXIST, NULL);

				if (default_file) {
					free(default_file);
					default_file = NULL;
				}
				if (name && *name)
					default_file = rnd_strdup(name);
				free_name = rnd_true;
			}
			else
				name = sbufnum;

			if (pcb_load_buffer(RND_ACT_HIDLIB, PCB_PASTEBUFFER, name, NULL) != 0) {
				rnd_message(RND_MSG_ERROR, "Failed to load buffer from %s\n", name);
				RND_ACT_IRES(-1);
			}
			else
				rnd_tool_select_by_name(RND_ACT_HIDLIB, "buffer");
			break;

		case F_Save:
			name = sbufnum;
			rv = rnd_actionva(RND_ACT_HIDLIB, "SaveTo", "PasteBuffer", name, fmt, NULL);
			rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_true);
			return rv;

		case F_ToLayout:
			{
				static rnd_coord_t oldx = 0, oldy = 0;
				rnd_coord_t x, y;
				rnd_bool absolute;
				if (argc == 2) {
					x = y = 0;
				}
				else if (strcmp(sbufnum, "crosshair") == 0) {
					x = pcb_crosshair.X;
					y = pcb_crosshair.Y;
				}
				else if (argc == 4 || argc == 5) {
					x = rnd_get_value(sbufnum, forces, &absolute, NULL);
					if (!absolute)
						x += oldx;
					y = rnd_get_value(fmt, forces, &absolute, NULL);
					if (!absolute)
						y += oldy;
				}

				else {
					rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_true);
					RND_ACT_FAIL(PasteBuffer);
				}

				oldx = x;
				oldy = y;
				if (pcb_buffer_copy_to_layout(PCB, x, y, rnd_false))
					pcb_board_set_changed_flag(PCB, rnd_true);
			}
			break;

		case F_Normalize:
			pcb_set_buffer_bbox(PCB_PASTEBUFFER);
			PCB_PASTEBUFFER->X = rnd_round(((double)PCB_PASTEBUFFER->BoundingBox.X1 + (double)PCB_PASTEBUFFER->BoundingBox.X2) / 2.0);
			PCB_PASTEBUFFER->Y = rnd_round(((double)PCB_PASTEBUFFER->BoundingBox.Y1 + (double)PCB_PASTEBUFFER->BoundingBox.Y2) / 2.0);
			break;

		case F_GetSource:
			if ((sbufnum != NULL) && (*sbufnum != '\0')) {
				char *end;
				number = strtol(sbufnum, &end, 10);
				if (*end != 0) {
					rnd_message(RND_MSG_ERROR, "invalid buffer number '%s': should be an integer\n", sbufnum);
					rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_true);
					return FGW_ERR_ARG_CONV;
				}
				number--;
				if ((number < 0) || (number >= PCB_MAX_BUFFER)) {
					rnd_message(RND_MSG_ERROR, "invalid buffer number '%d': out of range 1..%d\n", number+1, PCB_MAX_BUFFER);
					rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_true);
					return FGW_ERR_ARG_CONV;
				}
			}
			else
				number = conf_core.editor.buffer_number;
			res->type = FGW_STR;
			res->val.str = pcb_buffers[number].source_path;
			rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_true);
			return 0;

			/* set number */
		default:
			{
				

				/* correct number */
				if (number)
					pcb_buffer_set_number(number - 1);
			}
	}

	rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_true);
	RND_ACT_IRES(0);
	return 0;

	error:;
	rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_true);
	RND_ACT_IRES(-1);
	return 0;
}

/* --------------------------------------------------------------------------- */

static rnd_action_t buffer_action_list[] = {
	{"FreeRotateBuffer", pcb_act_FreeRotateBuffer, pcb_acth_FreeRotateBuffer, pcb_acts_FreeRotateBuffer},
	{"ScaleBuffer", pcb_act_ScaleBuffer, pcb_acth_ScaleBuffer, pcb_acts_ScaleBuffer},
	{"LoadFootprint", pcb_act_LoadFootprint, pcb_acth_LoadFootprint, pcb_acts_LoadFootprint},
	{"LoadPixmap", pcb_act_LoadPixmap, pcb_acth_LoadPixmap, pcb_acts_LoadPixmap},
	{"PasteBuffer", pcb_act_PasteBuffer, pcb_acth_PasteBuffer, pcb_acts_PasteBuffer}
};

void pcb_buffer_init2(void)
{
	RND_REGISTER_ACTIONS(buffer_action_list, NULL);
}




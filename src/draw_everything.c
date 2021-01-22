/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2003, 2004 Thomas Nau
 *  Copyright (C) 2017,2018,2020 Tibor 'Igor2' Palinkas
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

#define DRW_MAX_ARG 8

#define DA_FAR_SIDE 0x81
#define DA_THIS_SIDE 0x82

typedef struct {
	/* silk color tune */
	pcb_layergrp_t *backsilk_grp;
	rnd_color_t old_silk_color[PCB_MAX_LAYERGRP];

	pcb_layer_type_t vside, ivside;
	rnd_layergrp_id_t component, solder, side_copper_grp;
	rnd_layergrp_id_t backsilk_gid;


	/* copper ordering and virtual layers */
	legacy_vlayer_t lvly;
	char do_group[PCB_MAX_LAYERGRP]; /* This is the list of layer groups we will draw.  */
	rnd_layergrp_id_t drawn_groups[PCB_MAX_LAYERGRP]; /* This is the reverse of the order in which we draw them.  */
	int ngroups;
	char lvly_inited;
	char do_group_inited;

	/* when scripted: per call args */
	int argc;
	long argv[DRW_MAX_ARG];
} draw_everything_t;



typedef struct {
	int (*cmd)(pcb_draw_info_t *info, draw_everything_t *de);
	int argc;
} draw_call_t;

typedef enum {
	DI_IF,
	DI_AND,
	DI_NOT,
	DI_THEN,
	DI_STOP,
	DI_CALL,
	DI_ARG,
	DI_NL,

	/* fake expressions */
	DI_GUI,
	DI_CHECK_PLANES
} draw_inst_t;

typedef struct {
	draw_inst_t inst;
	draw_call_t call;
	long arg;
} draw_stmt_t;



#define GVT(x) vtdrw_ ## x
#define GVT_ELEM_TYPE draw_stmt_t
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 256
#define GVT_START_SIZE 64
#define GVT_FUNC RND_INLINE
#define GVT_SET_NEW_BYTES_TO 0

#include <genvector/genvector_impl.h>

#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)

#include <genvector/genvector_impl.c>

static vtdrw_t drw_script = {0};

static void draw_compile(const char *src);

static const char *draw_everything_recompile = NULL; /* set to the new script (pointing into a conf val) to get it compiled on next render */
static int draw_everything_error = 0; /* set to 1 on execution error so subsequent renders don't attempt to execute a failing script */


/*** Calls ***/

/* temporarily change the color of the other-side silk */
static int drw_silk_tune_color(pcb_draw_info_t *info, draw_everything_t *de)
{
	if (de->backsilk_grp != NULL) {
		rnd_cardinal_t n;
		for(n = 0; n < de->backsilk_grp->len; n++) {
			pcb_layer_t *ly = pcb_get_layer(PCB->Data, de->backsilk_grp->lid[n]);
			if (ly != NULL) {
				de->old_silk_color[n] = ly->meta.real.color;
				ly->meta.real.color = conf_core.appearance.color.invisible_objects;
			}
		}
	}
	return 0;
}

/* set back the color of the other-side silk */
static int drw_silk_restore_color(pcb_draw_info_t *info, draw_everything_t *de)
{
	if (de->backsilk_grp != NULL) {
		rnd_cardinal_t n;
		for(n = 0; n < de->backsilk_grp->len; n++) {
			pcb_layer_t *ly = pcb_get_layer(PCB->Data, de->backsilk_grp->lid[n]);
			if (ly != NULL)
				ly->meta.real.color = de->old_silk_color[n];
		}
	}
	return 0;
}

/* copper groups will be drawn in the order they were last selected
   in the GUI; build an array of their order */
static int drw_copper_order_UI(pcb_draw_info_t *info, draw_everything_t *de)
{
	int i;

	memset(de->do_group, 0, sizeof(de->do_group));

	de->lvly.top_fab = -1;
	de->lvly.top_assy = de->lvly.bot_assy = -1;

	for (de->ngroups = 0, i = 0; i < pcb_max_layer(PCB); i++) {
		pcb_layer_t *l = PCB_STACKLAYER(PCB, i);
		rnd_layergrp_id_t group = pcb_layer_get_group(PCB, pcb_layer_stack[i]);
		pcb_layergrp_t *grp = pcb_get_layergrp(PCB, group);
		unsigned int gflg = 0;

		if (grp != NULL)
			gflg = grp->ltype;

		if ((gflg & PCB_LYT_DOC) && (grp->purpi == F_fab)) {
			de->lvly.top_fab = group;
			de->lvly.top_fab_enable = has_auto(grp);
		}

		if ((gflg & PCB_LYT_DOC) && (grp->purpi == F_assy)) {
			if (gflg & PCB_LYT_TOP) {
				de->lvly.top_assy = group;
				de->lvly.top_assy_enable = has_auto(grp);
			}
			else if (gflg & PCB_LYT_BOTTOM) {
				de->lvly.bot_assy = group;
				de->lvly.bot_assy_enable = has_auto(grp);
			}
		}

		if ((gflg & PCB_LYT_SILK) || (gflg & PCB_LYT_DOC) || (gflg & PCB_LYT_MASK) || (gflg & PCB_LYT_PASTE) || (gflg & PCB_LYT_BOUNDARY) || (gflg & PCB_LYT_MECH)) /* do not draw silk, mask, paste and boundary here, they'll be drawn separately */
			continue;

		if (l->meta.real.vis && !de->do_group[group]) {
			de->do_group[group] = 1;
			de->drawn_groups[de->ngroups++] = group;
		}
	}
	de->lvly_inited = 1;
	de->do_group_inited = 1;
	return 0;
}

/* draw all layers in layerstack order */
static int drw_copper(pcb_draw_info_t *info, draw_everything_t *de)
{
	int i;

	if (!de->do_group_inited) {
		rnd_message(RND_MSG_ERROR, "draw script: drw_copper called without prior copper ordering\nCan not draw copper layers. Fix the script.\n");
		return -1;
	}

	for (i = de->ngroups - 1; i >= 0; i--) {
		rnd_layergrp_id_t group = de->drawn_groups[i];

		if (pcb_layer_gui_set_glayer(PCB, group, 0, &info->xform_exporter)) {
			int is_current = 0;
			rnd_layergrp_id_t cgrp = PCB_CURRLAYER(PCB)->meta.real.grp;

			if ((cgrp == de->solder) || (cgrp == de->component)) {
				/* current group is top or bottom: visibility depends on side we are looking at */
				if (group == de->side_copper_grp)
					is_current = 1;
			}
			else {
				/* internal layer displayed on top: current group is solid, others are "invisible" */
				if (group == cgrp)
					is_current = 1;
			}

			pcb_draw_layer_grp(info, group, is_current);
			rnd_render->end_layer(rnd_render);
		}
	}
	return 0;
}

/* draw all 'invisible' (other-side) layers */
static int drw_invis(pcb_draw_info_t *info, draw_everything_t *de)
{
	if ((info->xform_exporter != NULL) && !info->xform_exporter->check_planes && pcb_layer_gui_set_vlayer(PCB, PCB_VLY_INVISIBLE, 0, &info->xform_exporter)) {
		pcb_draw_silk_doc(info, de->ivside, PCB_LYT_DOC, 0, 1);
		pcb_draw_silk_doc(info, de->ivside, PCB_LYT_SILK, 0, 1);

		TODO("remove this if nobody reports a bug...");
		if (0) {
			rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_RESET, pcb_draw_out.direct, info->drawn_area);
			rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_POSITIVE, pcb_draw_out.direct, info->drawn_area);
			rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_FLUSH, pcb_draw_out.direct, info->drawn_area);
			rnd_render->end_layer(rnd_render);
		}
	}
	return 0;
}

static int drw_pstk(pcb_draw_info_t *info, draw_everything_t *de)
{
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_RESET, pcb_draw_out.direct, info->drawn_area);
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_POSITIVE, pcb_draw_out.direct, info->drawn_area);
	if (rnd_render->gui) {
		rnd_xform_t tmp;
		xform_setup(info, &tmp, NULL);
		pcb_draw_ppv(info, info->xform->show_solder_side ? de->solder : de->component);
		info->xform = NULL; info->layer = NULL;
	}
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_FLUSH, pcb_draw_out.direct, info->drawn_area);
	return 0;
}


/* Draw the solder mask if turned on */
static int drw_mask(pcb_draw_info_t *info, draw_everything_t *de)
{
	rnd_layergrp_id_t gid;

	gid = pcb_layergrp_get_top_mask();
	if ((gid >= 0) && (pcb_layer_gui_set_glayer(PCB, gid, 0, &info->xform_exporter))) {
		pcb_draw_mask(info, PCB_COMPONENT_SIDE);
		rnd_render->end_layer(rnd_render);
	}

	gid = pcb_layergrp_get_bottom_mask();
	if ((gid >= 0) && (pcb_layer_gui_set_glayer(PCB, gid, 0, &info->xform_exporter))) {
		pcb_draw_mask(info, PCB_SOLDER_SIDE);
		rnd_render->end_layer(rnd_render);
	}

	return 0;
}

static int drw_hole(pcb_draw_info_t *info, draw_everything_t *de)
{
	int plated, unplated;

	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_RESET, pcb_draw_out.direct, info->drawn_area); 
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_POSITIVE, pcb_draw_out.direct, info->drawn_area);

	pcb_board_count_holes(PCB, &plated, &unplated, info->drawn_area);

	if (plated && pcb_layer_gui_set_vlayer(PCB, PCB_VLY_PLATED_DRILL, 0, &info->xform_exporter)) {
		pcb_draw_pstk_holes(info, de->side_copper_grp, PCB_PHOLE_PLATED);
		rnd_render->end_layer(rnd_render);
	}

	if (unplated && pcb_layer_gui_set_vlayer(PCB, PCB_VLY_UNPLATED_DRILL, 0, &info->xform_exporter)) {
		pcb_draw_pstk_holes(info, de->side_copper_grp, PCB_PHOLE_UNPLATED);
		rnd_render->end_layer(rnd_render);
	}

	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_FLUSH, pcb_draw_out.direct, info->drawn_area);
	return 0;
}

static int drw_paste(pcb_draw_info_t *info, draw_everything_t *de)
{
	rnd_layergrp_id_t gid;
	rnd_bool paste_empty;

	gid = pcb_layergrp_get_top_paste();
	if (gid >= 0)
		paste_empty = pcb_layergrp_is_empty(PCB, gid);
	if ((gid >= 0) && (pcb_layer_gui_set_glayer(PCB, gid, paste_empty, &info->xform_exporter))) {
		pcb_draw_paste(info, PCB_COMPONENT_SIDE);
		rnd_render->end_layer(rnd_render);
	}

	gid = pcb_layergrp_get_bottom_paste();
	if (gid >= 0)
		paste_empty = pcb_layergrp_is_empty(PCB, gid);
	if ((gid >= 0) && (pcb_layer_gui_set_glayer(PCB, gid, paste_empty, &info->xform_exporter))) {
		pcb_draw_paste(info, PCB_SOLDER_SIDE);
		rnd_render->end_layer(rnd_render);
	}

	return 0;
}

static int drw_boundary_mech(pcb_draw_info_t *info, draw_everything_t *de)
{
	pcb_draw_boundary_mech(info);
	return 0;
}

static int drw_virtual(pcb_draw_info_t *info, draw_everything_t *de)
{
	draw_virtual_layers(info, &de->lvly);
	if ((rnd_render->gui) || (!info->xform_caller->omit_overlay)) {
		rnd_xform_t tmp;
		xform_setup(info, &tmp, NULL);
		draw_rats(info, info->drawn_area);
		draw_pins_and_pads(info, de->component, de->solder);
	}

	return 0;
}

static int drw_marks(pcb_draw_info_t *info, draw_everything_t *de)
{
	if (rnd_render->gui) {
		rnd_xform_t tmp;
		xform_setup(info, &tmp, NULL);
		draw_xor_marks(info);
	}
	return 0;
}

static int drw_layers(pcb_draw_info_t *info, draw_everything_t *de)
{
	pcb_layer_type_t loc, typ = de->argv[1];
	int setgrp = 1, invis = 0;
	TODO("error handling, return value");
	if (de->argc < 2)
		return -1;

	if ((de->argc > 2) && de->argv[2])
		invis = 1;

	if (de->argv[0] == DA_FAR_SIDE) loc = de->ivside;
	else if (de->argv[0] == DA_THIS_SIDE) loc = de->vside;
	else loc = de->argv[0];

	pcb_draw_silk_doc(info, loc, typ, setgrp, invis);
	return 0;
}

static int drw_ui_layers(pcb_draw_info_t *info, draw_everything_t *de)
{
	draw_ui_layers(info);
	return 0;
}

/*** Execute ***/

/* Fallback: the original C implementation, just in case the script fails */
RND_INLINE void draw_everything_hardwired(pcb_draw_info_t *info, draw_everything_t *de)
{
	drw_silk_tune_color(info, de);
	drw_copper_order_UI(info, de);
	drw_invis(info, de);

	/* Draw far side doc and silks */
	pcb_draw_silk_doc(info, de->ivside, PCB_LYT_SILK, 1, 0);
	pcb_draw_silk_doc(info, de->ivside, PCB_LYT_DOC, 1, 0);

	drw_copper(info, de);

	if (conf_core.editor.check_planes && rnd_render->gui)
		return;

	drw_pstk(info, de);
	drw_mask(info, de);

	/* Draw doc and silks */
	pcb_draw_silk_doc(info, PCB_LYT_INTERN, PCB_LYT_SILK, 1, 0);
	pcb_draw_silk_doc(info, PCB_LYT_INTERN, PCB_LYT_DOC, 1, 0);
	pcb_draw_silk_doc(info, 0, PCB_LYT_DOC, 1, 0);
	pcb_draw_silk_doc(info, de->vside, PCB_LYT_SILK, 1, 0);
	pcb_draw_silk_doc(info, de->vside, PCB_LYT_DOC, 1, 0);

	/* holes_after: draw holes after copper, silk and mask, to make sure it punches through everything. */
	drw_hole(info, de);

	drw_paste(info, de);

	drw_boundary_mech(info, de);

	drw_virtual(info, de);
	drw_ui_layers(info, de);

	drw_marks(info, de);
}

#define IF_NOT(cond) (not ? (cond) : !(cond))

RND_INLINE int draw_everything_scripted(pcb_draw_info_t *info, draw_everything_t *de)
{
	draw_stmt_t *s, *end, *call;
	int in_if, false_if = 0, not = 0;

	/* compile on first render */
	if (draw_everything_recompile != NULL) {
		draw_compile(draw_everything_recompile);
		draw_everything_recompile = NULL;
		draw_everything_error = 0;
	}

	if (draw_everything_error || (drw_script.used == 0))
		return -1;

	/* bytecode execution */
	end = drw_script.array + drw_script.used;
	for(s = drw_script.array; s < end; s++) {
		switch(s->inst) {
			case DI_STOP: if (!false_if) return 0; break;

			/* call with arg processing */
			case DI_CALL: call = s; de->argc = 0; break;
			case DI_ARG:  de->argv[de->argc++] = s->arg; break;
			case DI_NL:
				if ((call != NULL) && !false_if) {
					if (call->call.cmd(info, de) != 0)
						return -1;
				}
				call = NULL;
				in_if = 0; /* just in case 'then' is missing */
				false_if = 0;
				break;

			/* if with fake expression processing */
			case DI_IF:           in_if = 1; false_if = 0; break;
			case DI_AND:          if (!in_if) return -1; break;
			case DI_NOT:          if (!in_if) return -1; not = 1; break;
			case DI_THEN:         if (!in_if) return -1; in_if = 0; break;
			case DI_GUI:          if (IF_NOT(rnd_render->gui)) false_if = 1; not = 0; break;
			case DI_CHECK_PLANES: if (IF_NOT((info->xform_exporter != NULL) && (info->xform_exporter->check_planes))) false_if = 1; not = 0; break;
		}
	}

	return 0;
}

static void draw_everything(pcb_draw_info_t *info)
{
	draw_everything_t de;
	rnd_xform_t tmp;

	setup_again:;

	de.backsilk_grp = NULL;
	de.lvly_inited = 0;
	de.do_group_inited = 0;

	xform_setup(info, &tmp, NULL);
	rnd_render->render_burst(rnd_render, RND_HID_BURST_START, info->drawn_area);
	de.solder = de.component = -1;
	pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &de.solder, 1);
	pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &de.component, 1);
	de.side_copper_grp = info->xform->show_solder_side ? de.solder : de.component;
	de.ivside = PCB_LYT_INVISIBLE_SIDE();
	de.vside = PCB_LYT_VISIBLE_SIDE();
	de.backsilk_gid = ((!info->xform->show_solder_side) ? pcb_layergrp_get_bottom_silk() : pcb_layergrp_get_top_silk());
	de.backsilk_grp = pcb_get_layergrp(PCB, de.backsilk_gid);

	if (draw_everything_scripted(info, &de) < 0) {
		if (!draw_everything_error) {
			rnd_message(RND_MSG_ERROR, "render_script: runtime error; falling back to hardwired C rendering\n");
			draw_everything_error = 1; /* let the error happen only once per compilation */
			goto setup_again; /* a broken rendering may have left invalid states behind */
		}
		draw_everything_hardwired(info, &de);
	}

	drw_silk_restore_color(info, &de);
}


/*** Compile ***/
typedef struct {
	const char *full;
	int hash;

	draw_inst_t inst;
	int (*cmd)(pcb_draw_info_t *info, draw_everything_t *de);
	long arg;
} drw_kw_t;

#define HASH(s) ((s[0] << 8) | (s[1]))
#define KW_MAXLEN 24

static drw_kw_t drw_kw[] = {
	{"if",              0, DI_IF, NULL, 0},
	{"and",             0, DI_AND, NULL, 0},
	{"not",             0, DI_NOT, NULL, 0},
	{"then",            0, DI_THEN, NULL, 0},
	{"stop",            0, DI_STOP, NULL, 0},

	{"GUI",             0, DI_GUI, NULL, 0},
	{"check_planes",    0, DI_CHECK_PLANES, NULL, 0},

	{"silk_tune_color", 0, DI_CALL, drw_silk_tune_color, 0},
	{"copper_order_UI", 0, DI_CALL, drw_copper_order_UI, 0},
	{"drw_copper",      0, DI_CALL, drw_copper, 0},
	{"drw_invis",       0, DI_CALL, drw_invis, 0},
	{"drw_pstk",        0, DI_CALL, drw_pstk, 0},
	{"drw_mask",        0, DI_CALL, drw_mask, 0},
	{"drw_layers",      0, DI_CALL, drw_layers, 0},
	{"drw_hole",        0, DI_CALL, drw_hole, 0},
	{"drw_paste",       0, DI_CALL, drw_paste, 0},
	{"drw_virtual",     0, DI_CALL, drw_virtual, 0},
	{"drw_ui_layers",   0, DI_CALL, drw_ui_layers, 0},
	{"drw_marks",       0, DI_CALL, drw_marks, 0},
	{"drw_boundary_mech",0,DI_CALL, drw_boundary_mech, 0},


	{"global",          0, DI_ARG,  NULL, 0},
	{"this_side",       0, DI_ARG,  NULL, DA_THIS_SIDE},
	{"far_side",        0, DI_ARG,  NULL, DA_FAR_SIDE},

	{NULL, 0, 0, NULL }
};

static void draw_compile(const char *src)
{
	const char *start, *next;
	int hash = 0, line = 1, nst = 0, cmd_idx = -1;
	char kw0[KW_MAXLEN+2];

	/* calculate hash values for keywords on first execution */
	if (drw_kw[0].hash == 0) {
		drw_kw_t *k;
		for(k = drw_kw; k->full != NULL; k++)
			k->hash = HASH(k->full);
	}

	/* clear the script */
	drw_script.used = 0;

	/* parse word by word */
	for(start = src; start != NULL; start = next) {
		draw_stmt_t *stmt;
		const drw_kw_t *k;
		drw_kw_t karg;
		int h, len, bad;

		/* skip leading whitespace */
		while((*start == ' ') || (*start == '\t')) start++;

		if (*start == '\0')
			break;

		/* ignore comments up to the next newline */
		if (*start == '#')
			for(start++; (*start != '\n') && (*start != '\r') && (*start != '\0'); start++) ;

		/* newline is a special case as it is an instruction */
		if ((*start == '\n') || (*start == '\r')) {
			cmd_idx = -1;
			if ((drw_script.used > 0) && (drw_script.array[drw_script.used - 1].inst != DI_NL)) {
				stmt = vtdrw_alloc_append(&drw_script, 1);
				stmt->inst = DI_NL;
				nst++;
			}
			if (*start == '\n')
				line++;
			next = start + 1;
			continue;
		}

		/* seek end of the current word in 'next' */
		next = start;
		while(!isspace(*next) && (*next != '\0')) next++;

		/* keywords are at least 2 chars long, required for the hash */
		len = next - start;
		if (len < 2) {
			strncpy(kw0, start, len); kw0[len] = '\0';
			rnd_message(RND_MSG_ERROR, "render_script: syntax error in line %d: keyword '%s' too short\n", line, kw0);
			continue;
		}
		if (len > KW_MAXLEN) {
			memcpy(kw0, start, KW_MAXLEN);
			kw0[KW_MAXLEN] = '\0';
			rnd_message(RND_MSG_ERROR, "render_script: syntax error in line %d: keyword %s... too long\n", line, kw0);
			continue;
		}

		/* look up the keyword speeding up the process with a hash comparison that skips the strncmp() most of the time */
		h = HASH(start);
		for(k = drw_kw; k->full != NULL; k++)
			if ((k->hash == h) && (strncmp(k->full, start, len) == 0) && (k->full[len] == '\0'))
				break;

		bad = (k->full == NULL);

		/* not a known keyword, could be a number */
		if (bad) {
			if ((start[0] == '+') || (start[0] == '-') || isdigit(start[0])) {
				char *end;
				long l = strtol(start, &end, 10);
				if (isspace(*end) || (*end == '\0')) {
					k = &karg;
					karg.inst = DI_ARG;
					karg.arg = l;
					bad = 0;
				}
			}
		}

		/* not a known keyword or number, could be a layer type */
		if (bad) {
			pcb_layer_type_t lyt;

			strncpy(kw0, start, len); kw0[len] = '\0';
			lyt = pcb_layer_type_str2bit(kw0);
			if (lyt > 0) {
				k = &karg;
				karg.inst = DI_ARG;
				karg.arg = lyt;
				bad = 0;
			}
		}

		if (bad) {
			strncpy(kw0, start, len); kw0[len] = '\0';
			rnd_message(RND_MSG_ERROR, "render_script: keyword '%s' not found in line %d\n", kw0, line);
			continue;
		}

		stmt = vtdrw_alloc_append(&drw_script, 1);
		stmt->inst = k->inst;
		stmt->call.cmd = k->cmd;
		stmt->arg = k->arg;

		/* remember last command or update last command's argc */
		if (cmd_idx == -1) {
			cmd_idx = drw_script.used - 1;
			drw_script.array[cmd_idx].call.argc = 0;
		}
		else
			drw_script.array[cmd_idx].call.argc++;
TODO("check max argc");
	}

	rnd_message(RND_MSG_INFO, "render_script: compiled %ld instructions in %d statements\n", drw_script.used, nst);
}

#undef KW_MAXLEN
#undef HASH
#undef DRW_MAX_ARG
#undef DA_FAR_SIDE
#undef DA_THIS_SIDE

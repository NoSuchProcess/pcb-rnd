/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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
 */

#include "config.h"

#include <genht/htsp.h>
#include <genht/htsi.h>
#include <genht/hash.h>
#include <genvector/gds_char.h>

#include "actions.h"
#include "board.h"
#include "buffer.h"
#include "conf_core.h"
#include "data.h"
#include "draw.h"
#include "obj_subc.h"
#include "plug_footprint.h"
#include "tool.h"

#include "hid.h"
#include "hid_dad.h"
#include "hid_dad_tree.h"
#include "hid_init.h"

#include "dlg_library.h"

#define MAX_PARAMS 128

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	int wtree, wpreview, wtags, wfilt;

	int active; /* already open - allow only one instance */

	pcb_subc_t *sc;
	pcb_board_t *prev_pcb; /* sc must be in here so buffer changes don't ruin it */


 /* for the parametric */
	int pactive; /* already open - allow only one instance */
	int pwdesc;
	PCB_DAD_DECL_NOINIT(pdlg)
	pcb_fplibrary_t *last_l;
	char *example, *help_params;
	htsi_t param_names;     /* param_name -> param_idx */
	int pwid[MAX_PARAMS];   /* param_idx -> widget_idx (for the input field widget) */
	int num_params, first_optional;
	gds_t descr;
} library_ctx_t;

library_ctx_t library_ctx;

#include "dlg_library_param.c"

/* XPM */
static const char *xpm_edit_param[] = {
"16 16 4 1",
"@ c #000000",
"* c #7A8584",
"+ c #6EA5D7",
"  c None",
"                ",
"  @          @  ",
"  @ ++++++++ @  ",
" @  +      +  @ ",
" @  + @@@@ +  @ ",
" @  +      +  @ ",
"@   + @@   +   @",
"@   +      +   @",
"@   + @ @ @+   @",
"@   +      +   @",
" @  +   @@ +  @ ",
" @  + @@@@ +  @ ",
" @  +      +  @ ",
"  @ ++++++++ @  ",
"  @          @  ",
"                "
};


/* XPM */
static const char *xpm_refresh[] = {
"16 16 4 1",
"@ c #000000",
"* c #7A8584",
"+ c #6EA5D7",
"  c None",
"   @            ",
"   @@@@@@@      ",
"   @@@    @@    ",
"   @@@@     @   ",
"             @  ",
"             @  ",
"              @ ",
" @            @ ",
" @            @ ",
" @              ",
"  @             ",
"  @             ",
"   @     @@@@   ",
"    @@    @@@   ",
"      @@@@@@@   ",
"            @   ",
};

static void library_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	library_ctx_t *ctx = caller_data;

	pcb_board_free(ctx->prev_pcb);
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(library_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void create_lib_tree_model_recurse(pcb_hid_attribute_t *attr, pcb_fplibrary_t *parent_lib, pcb_hid_row_t *parent_row)
{
	char *cell[2];
	pcb_fplibrary_t *l;
	int n;

	cell[1] = NULL;
	for(l = parent_lib->data.dir.children.array, n = 0; n < parent_lib->data.dir.children.used; l++, n++) {
		pcb_hid_row_t *row;

		cell[0] = pcb_strdup(l->name);
		row = pcb_dad_tree_append_under(attr, parent_row, cell);
		row->user_data = l;
		if (l->type == LIB_DIR)
			create_lib_tree_model_recurse(attr, l, row);
	}
}


static void library_lib2dlg(library_ctx_t *ctx)
{
	pcb_hid_attribute_t *attr;
	pcb_hid_tree_t *tree;
	pcb_hid_row_t *r;
	char *cursor_path = NULL;

	attr = &ctx->dlg[ctx->wtree];
	tree = (pcb_hid_tree_t *)attr->enumerations;

	/* remember cursor */
	r = pcb_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = pcb_strdup(r->cell[0]);

	/* remove existing items */
	pcb_dad_tree_clear(tree);

	/* add all items recursively */
	create_lib_tree_model_recurse(attr, &pcb_library, NULL);

	/* restore cursor */
	if (cursor_path != NULL) {
		pcb_hid_attr_val_t hv;
		hv.str_value = cursor_path;
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wtree, &hv);
		free(cursor_path);
	}
}

static void library_update_preview(library_ctx_t *ctx, pcb_subc_t *sc, pcb_fplibrary_t *l)
{
	pcb_box_t bbox;
	pcb_hid_attr_val_t hv;
	gds_t tmp;

	if (ctx->sc != NULL) {
		pcb_subc_remove(ctx->sc);
		ctx->sc = NULL;
	}

	gds_init(&tmp);
	if (sc != NULL) {
		ctx->sc = pcb_subc_dup_at(ctx->prev_pcb, ctx->prev_pcb->Data, sc, 0, 0, 1);
		pcb_data_bbox(&bbox, ctx->sc->data, 0);
		pcb_dad_preview_zoomto(&ctx->dlg[ctx->wpreview], &bbox);
	}

	if (l != NULL) {
		void **t;

TODO("Use rich text for this with explicit wrap marks\n");
		/* update tags */
		for (t = l->data.fp.tags; ((t != NULL) && (*t != NULL)); t++) {
			const char *name = pcb_fp_tagname(*t);
			if (name != NULL) {
				gds_append_str(&tmp, "\n  ");
				gds_append_str(&tmp, name);
			}
		}
		gds_append_str(&tmp, "\nLocation:\n ");
		gds_append_str(&tmp, l->data.fp.loc_info);
		gds_append_str(&tmp, "\n");

		hv.str_value = tmp.array;
	}
	else
		hv.str_value = "";

	pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wtags, &hv);
	gds_uninit(&tmp);
}

static void library_select(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	pcb_hid_attr_val_t hv;
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attrib->enumerations;
	library_ctx_t *ctx = tree->user_ctx;
	int close_param = 1;


	library_update_preview(ctx, NULL, NULL);
	if (row != NULL) {
		pcb_fplibrary_t *l = row->user_data;
		if ((l != NULL) && (l->type == LIB_FOOTPRINT)) {
			if (l->data.fp.type == PCB_FP_PARAMETRIC) {
				library_param_dialog(ctx, l);
				close_param = 0;
			}
			else {
				if (pcb_buffer_load_footprint(PCB_PASTEBUFFER, l->data.fp.loc_info, NULL)) {
					pcb_tool_select_by_id(&PCB->hidlib, PCB_MODE_PASTE_BUFFER);
					if (pcb_subclist_length(&PCB_PASTEBUFFER->Data->subc) != 0)
						library_update_preview(ctx, pcb_subclist_first(&PCB_PASTEBUFFER->Data->subc), l);
					pcb_gui->invalidate_all(&PCB->hidlib);
				}
			}
		}
	}

	if (close_param)
		library_param_dialog(ctx, NULL);

	hv.str_value = NULL;
	pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wpreview, &hv);

}

static void library_expose(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	library_ctx_t *ctx = prv->user_ctx;
	int orig_po = pcb_draw_force_termlab;
	if (ctx->sc != NULL) {
		pcb_draw_force_termlab = pcb_false;
		pcb_subc_draw_preview(ctx->sc, &e->view);
		pcb_draw_force_termlab = orig_po;
	}
}

static void library_filter_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr_inp)
{
	library_ctx_t *ctx = caller_data;
	pcb_hid_attribute_t *attr;
	pcb_hid_tree_t *tree;
	const char *otext;
	char *text, *sep;
	int have_filter_text;

	attr = &ctx->dlg[ctx->wtree];
	tree = (pcb_hid_tree_t *)attr->enumerations;
	otext = attr_inp->default_val.str_value;
	text = pcb_strdup(otext);
	have_filter_text = (*text != '\0');

	sep = strpbrk(text, " ()\t\r\n");
	if (sep != NULL)
		*sep = '\0';

	/* hide or unhide everything */
	pcb_dad_tree_hide_all(tree, &tree->rows, have_filter_text);

	if (have_filter_text) /* unhide hits and all their parents */
		pcb_dad_tree_unhide_filter(tree, &tree->rows, 0, text);

	pcb_dad_tree_expcoll(attr, NULL, 1, 1);
	pcb_dad_tree_update_hide(attr);

	/* parametric footprints need to be refreshed on edit */
	if (strchr(otext, ')') != NULL) {
		pcb_fplibrary_t *l = pcb_fp_lib_search(&pcb_library, text);
pcb_trace("l=%p otext='%s' text='%s'\n", l, otext, text);
		if ((l != NULL) && (pcb_buffer_load_footprint(PCB_PASTEBUFFER, otext, NULL))) {
			pcb_tool_select_by_id(&PCB->hidlib, PCB_MODE_PASTE_BUFFER);
			if (pcb_subclist_length(&PCB_PASTEBUFFER->Data->subc) != 0)
				library_update_preview(ctx, pcb_subclist_first(&PCB_PASTEBUFFER->Data->subc), l);
			pcb_gui->invalidate_all(&PCB->hidlib);
		}
	}

	free(text);
}



static pcb_bool library_mouse(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_false;
}

static void pcb_dlg_library(void)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	const pcb_dflgmap_t *g;
	int n;

	if (library_ctx.active)
		return; /* do not open another */

	library_ctx.prev_pcb = pcb_board_new_(1);

	for(g = pcb_dflgmap, n = 0; g->name != NULL; g++,n++)
		pcb_layergrp_set_dflgly(library_ctx.prev_pcb, &(library_ctx.prev_pcb->LayerGroups.grp[n]), g, g->name, g->name);
	library_ctx.prev_pcb->LayerGroups.len = n;

	PCB_DAD_BEGIN_VBOX(library_ctx.dlg);
		PCB_DAD_COMPFLAG(library_ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_HPANE(library_ctx.dlg);
			/* left */
			PCB_DAD_BEGIN_VBOX(library_ctx.dlg);
				PCB_DAD_COMPFLAG(library_ctx.dlg, PCB_HATF_EXPFILL);
				PCB_DAD_TREE(library_ctx.dlg, 1, 1, NULL);
					PCB_DAD_COMPFLAG(library_ctx.dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
					PCB_DAD_TREE_SET_CB(library_ctx.dlg, selected_cb, library_select);
					PCB_DAD_TREE_SET_CB(library_ctx.dlg, ctx, &library_ctx);
					library_ctx.wtree = PCB_DAD_CURRENT(library_ctx.dlg);
				PCB_DAD_BEGIN_HBOX(library_ctx.dlg);
					PCB_DAD_STRING(library_ctx.dlg);
						PCB_DAD_COMPFLAG(library_ctx.dlg, PCB_HATF_EXPFILL);
						PCB_DAD_HELP(library_ctx.dlg, "filter: display only footprints matching this text\n(if empty: display all)");
						PCB_DAD_CHANGE_CB(library_ctx.dlg, library_filter_cb);
						library_ctx.wfilt = PCB_DAD_CURRENT(library_ctx.dlg);
					PCB_DAD_PICBUTTON(library_ctx.dlg, xpm_edit_param);
						PCB_DAD_HELP(library_ctx.dlg, "open GUI to edit the parameters\nof a parametric footprint");
					PCB_DAD_PICBUTTON(library_ctx.dlg, xpm_refresh);
						PCB_DAD_HELP(library_ctx.dlg, "reload and refresh the current\nmain tree of the library");
				PCB_DAD_END(library_ctx.dlg);
			PCB_DAD_END(library_ctx.dlg);

			/* right */
			PCB_DAD_BEGIN_VPANE(library_ctx.dlg);
				PCB_DAD_COMPFLAG(library_ctx.dlg, PCB_HATF_EXPFILL | PCB_HATF_FRAME);
				/* right top */
				PCB_DAD_PREVIEW(library_ctx.dlg, library_expose, library_mouse, NULL, NULL, 200, 200, &library_ctx);
					library_ctx.wpreview = PCB_DAD_CURRENT(library_ctx.dlg);

				/* right bottom */
				TODO("rich text label");
				PCB_DAD_LABEL(library_ctx.dlg, "");
					library_ctx.wtags = PCB_DAD_CURRENT(library_ctx.dlg);
			PCB_DAD_END(library_ctx.dlg);
		PCB_DAD_END(library_ctx.dlg);

		/* bottom */
		PCB_DAD_BUTTON_CLOSES(library_ctx.dlg, clbtn);
	PCB_DAD_END(library_ctx.dlg);

	/* set up the context */
	library_ctx.active = 1;

	PCB_DAD_NEW("library", library_ctx.dlg, "pcb-rnd Footprint Library", &library_ctx, pcb_false, library_close_cb);

	library_lib2dlg(&library_ctx);
}

const char pcb_acts_LibraryDialog[] = "libraryDialog()\n";
const char pcb_acth_LibraryDialog[] = "Open the library dialog.";
fgw_error_t pcb_act_LibraryDialog(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	pcb_dlg_library();
	return 0;
}

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
#include <genregex/regex_sei.h>

#include "actions.h"
#include "board.h"
#include "buffer.h"
#include "conf_core.h"
#include "data.h"
#include "draw.h"
#include "event.h"
#include "obj_subc.h"
#include "plug_footprint.h"
#include "tool.h"
#include "undo.h"

#include "hid.h"
#include "hid_dad.h"
#include "hid_dad_tree.h"
#include "hid_init.h"

#include "dlg_library.h"

static const char *library_cookie = "dlg_library";

#define MAX_PARAMS 128

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	int wtree, wpreview, wtags, wfilt, wpend, wedit;

	int active; /* already open - allow only one instance */

	pcb_subc_t *sc;
	pcb_board_t *prev_pcb; /* sc must be in here so buffer changes don't ruin it */

	pcb_hidval_t timer;
	int timer_active;

 /* for the parametric */
	int pactive; /* already open - allow only one instance */
	int pwdesc;
	PCB_DAD_DECL_NOINIT(pdlg)
	pcb_fplibrary_t *last_l;
	char *example, *help_params;
	htsi_t param_names;     /* param_name -> param_idx */
	int pwid[MAX_PARAMS];   /* param_idx -> widget_idx (for the input field widget) */
	char *pnames[MAX_PARAMS]; /* param_idx -> parameter_name (also key stored in the param_names hash */
	int num_params, first_optional;
	gds_t descr;
} library_ctx_t;

library_ctx_t library_ctx;


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

static void library_update_preview(library_ctx_t *ctx, pcb_subc_t *sc, pcb_fplibrary_t *l)
{
	pcb_box_t bbox;
	pcb_hid_attr_val_t hv;
	gds_t tmp;

	if (ctx->sc != NULL) {
		pcb_undo_freeze_add();
		pcb_subc_remove(ctx->sc);
		pcb_undo_unfreeze_add();
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

static void timed_update_preview_cb(pcb_hidval_t user_data)
{
	library_ctx_t *ctx = user_data.ptr;
	const char *otext = ctx->dlg[ctx->wfilt].default_val.str_value;

	if (pcb_buffer_load_footprint(PCB_PASTEBUFFER, otext, NULL)) {
		pcb_tool_select_by_id(&PCB->hidlib, PCB_MODE_PASTE_BUFFER);
		if (pcb_subclist_length(&PCB_PASTEBUFFER->Data->subc) != 0)
			library_update_preview(ctx, pcb_subclist_first(&PCB_PASTEBUFFER->Data->subc), NULL);
		pcb_gui->invalidate_all(pcb_gui, &PCB->hidlib);
	}
	ctx->timer_active = 0;
	pcb_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wpend, 1);
}

static void timed_update_preview(library_ctx_t *ctx, int active)
{
	if (ctx->timer_active) {
		pcb_gui->stop_timer(pcb_gui, ctx->timer);
		ctx->timer_active = 0;
		pcb_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wpend, 1);
	}

	if (active) {
		pcb_hidval_t user_data;
		user_data.ptr = ctx;
		ctx->timer = pcb_gui->add_timer(pcb_gui, timed_update_preview_cb, 500, user_data);
		ctx->timer_active = 1;
		pcb_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wpend, 0);
	}
}

static void update_edit_button(library_ctx_t *ctx)
{
	const char *otext = ctx->dlg[ctx->wfilt].default_val.str_value;
	pcb_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->wedit, !ctx->pactive && (otext != NULL) && (strchr(otext, '(') != NULL));
}

#include "dlg_library_param.c"

static void library_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	library_ctx_t *ctx = caller_data;

	timed_update_preview(ctx, 0);
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

static void library_select(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	pcb_hid_attr_val_t hv;
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attrib->enumerations;
	library_ctx_t *ctx = tree->user_ctx;
	int close_param = 1;

	timed_update_preview(ctx, 0);

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
					pcb_gui->invalidate_all(pcb_gui, &PCB->hidlib);
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

static int tag_match(pcb_fplibrary_t *l, vtp0_t *taglist)
{
	size_t n;

	if (taglist->used == 0)
		return 1;

	if (l->data.fp.tags == NULL)
		return 0;

	for(n = 0; n < taglist->used; n++) {
		void **t;
		const void *need = pcb_fp_tag((const char *)taglist->array[n], 0);
		int found = 0;
		for(t = l->data.fp.tags, found = 0; *t != NULL; t++) {
			if (*t == need) {
				found = 1;
				break;
			}
		}
		
		if (!found)
			return 0;
	}

	return 1;
}

static void library_tree_unhide(pcb_hid_tree_t *tree, gdl_list_t *rowlist, re_sei_t *preg, vtp0_t *taglist)
{
	pcb_hid_row_t *r, *pr;

	for(r = gdl_first(rowlist); r != NULL; r = gdl_next(rowlist, r)) {
		if (((preg == NULL) || (re_sei_exec(preg, r->cell[0]))) && tag_match(r->user_data, taglist)) {
			pcb_dad_tree_hide_all(tree, &r->children, 0); /* if this is a node with children, show all children */
			for(pr = r; pr != NULL; pr = pcb_dad_tree_parent_row(tree, pr)) /* also show all parents so it is visible */
				pr->hide = 0;
		}
		library_tree_unhide(tree, &r->children, preg, taglist);
	}
}

static void library_filter_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr_inp)
{
	library_ctx_t *ctx = caller_data;
	pcb_hid_attribute_t *attr;
	pcb_hid_tree_t *tree;
	const char *otext;
	char *text, *sep;
	int have_filter_text, is_para;

	attr = &ctx->dlg[ctx->wtree];
	tree = (pcb_hid_tree_t *)attr->enumerations;
	otext = attr_inp->default_val.str_value;
	text = pcb_strdup(otext);
	have_filter_text = (*text != '\0');

	is_para = (strchr(otext, '(') != NULL);

	sep = strpbrk(text, " ()\t\r\n");
	if (sep != NULL)
		*sep = '\0';

	/* hide or unhide everything */
	pcb_dad_tree_hide_all(tree, &tree->rows, have_filter_text);

	if (have_filter_text) { /* unhide hits and all their parents */
		char *tag, *next, *tags = NULL;
		vtp0_t taglist;
		re_sei_t *regex = NULL;

		if (!is_para) {
			tags = strchr(otext, ' ');
			if (tags != NULL) {
				*tags = '\0';
				tags++;
				while(isspace(*tags))
					tags++;
				if (*tags == '\0')
					tags = NULL;
			}
		}

		vtp0_init(&taglist);
		if (tags != NULL) {
			tags = pcb_strdup(tags);
			for (tag = tags; tag != NULL; tag = next) {
				next = strpbrk(tag, " \t\r\n");
				if (next != NULL) {
					*next = '\0';
					next++;
					while (isspace(*next))
						next++;
				}
				vtp0_append(&taglist, tag);
			}
		}

		if ((text != NULL) && (*text != '\0'))
			regex = re_sei_comp(text);

		library_tree_unhide(tree, &tree->rows, regex, &taglist);

		if (regex != NULL)
			re_sei_free(regex);
		vtp0_uninit(&taglist);
		free(tags);
	}

	pcb_dad_tree_expcoll(attr, NULL, 1, 1);
	pcb_dad_tree_update_hide(attr);

	/* parametric footprints need to be refreshed on edit */
	if (is_para)
		timed_update_preview(ctx, 1);

	update_edit_button(ctx);

	free(text);
}

static pcb_hid_row_t *find_fp_prefix_(pcb_hid_tree_t *tree, gdl_list_t *rowlist, const char *name, int namelen)
{
	pcb_hid_row_t *r, *pr;

	for(r = gdl_first(rowlist); r != NULL; r = gdl_next(rowlist, r)) {
		pcb_fplibrary_t *l = r->user_data;
		if ((pcb_strncasecmp(r->cell[0], name, namelen) == 0) && (l->type == LIB_FOOTPRINT) && (l->data.fp.type == PCB_FP_PARAMETRIC))
			return r;
		pr = find_fp_prefix_(tree, &r->children, name, namelen);
		if (pr != NULL)
			return pr;
	}
	return NULL;
}

static pcb_hid_row_t *find_fp_prefix(library_ctx_t *ctx, const char *name, int namelen)
{
	pcb_hid_attribute_t *attr;
	pcb_hid_tree_t *tree;

	attr = &ctx->dlg[ctx->wtree];
	tree = (pcb_hid_tree_t *)attr->enumerations;

	return find_fp_prefix_(tree, &tree->rows, name, namelen);
}


static void library_edit_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr_inp)
{
	library_ctx_t *ctx = caller_data;
	pcb_hid_attribute_t *attr;
	pcb_hid_row_t *r, *rnew;
	const char *otext = ctx->dlg[ctx->wfilt].default_val.str_value;
	char *name, *sep;
	int namelen;

	name = pcb_strdup(otext);
	sep = strchr(name, '(');
	if (sep != NULL)
		*sep = '\0';
	namelen = strlen(name);


	attr = &ctx->dlg[ctx->wtree];
	r = pcb_dad_tree_get_selected(attr);

	if ((r == NULL) || (pcb_strncasecmp(name, r->cell[0], namelen) != 0)) {
		/* no selection or wrong selection: go find the right one */
		rnew = find_fp_prefix(ctx, name, namelen);
	}
	else
		rnew = r;

	if (rnew != NULL) {
		if (r != rnew)
			pcb_dad_tree_jumpto(attr, rnew);
		library_param_dialog(ctx, rnew->user_data);
	}
	else
		pcb_message(PCB_MSG_ERROR, "No such parametric footprint: '%s'\n", name);

	free(name);
}

static void library_refresh_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr_inp)
{
	library_ctx_t *ctx = caller_data;
	pcb_hid_row_t *r = pcb_dad_tree_get_selected(&(ctx->dlg[ctx->wtree]));
	pcb_fplibrary_t *l;
	char *oname;

	if (r == NULL)
		return;

	l = r->user_data;
	if ((l == NULL) || (l->parent == NULL))
		return;

	while((l->parent != NULL) && (l->parent->parent != NULL)) l = l->parent;

	oname = pcb_strdup(l->name); /* need to save the name because refresh invalidates l */

	if (pcb_fp_rehash(&PCB->hidlib, l) == 0)
		pcb_message(PCB_MSG_INFO, "Refreshed library '%s'\n", oname);
	else
		pcb_message(PCB_MSG_ERROR, "Failed to refresh library '%s'\n", oname);

	free(oname);
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
						PCB_DAD_CHANGE_CB(library_ctx.dlg, library_edit_cb);
						library_ctx.wedit = PCB_DAD_CURRENT(library_ctx.dlg);
					PCB_DAD_PICBUTTON(library_ctx.dlg, xpm_refresh);
						PCB_DAD_HELP(library_ctx.dlg, "reload and refresh the current\nmain tree of the library");
						PCB_DAD_CHANGE_CB(library_ctx.dlg, library_refresh_cb);
				PCB_DAD_END(library_ctx.dlg);
			PCB_DAD_END(library_ctx.dlg);

			/* right */
			PCB_DAD_BEGIN_VPANE(library_ctx.dlg);
				PCB_DAD_COMPFLAG(library_ctx.dlg, PCB_HATF_EXPFILL | PCB_HATF_FRAME);
				/* right top */
				PCB_DAD_PREVIEW(library_ctx.dlg, library_expose, library_mouse, NULL, NULL, 100, 100, &library_ctx);
					library_ctx.wpreview = PCB_DAD_CURRENT(library_ctx.dlg);

				/* right bottom */
				PCB_DAD_BEGIN_VBOX(library_ctx.dlg);
					PCB_DAD_LABEL(library_ctx.dlg, "Pending refresh...");
						PCB_DAD_COMPFLAG(library_ctx.dlg, PCB_HATF_HIDE);
						library_ctx.wpend = PCB_DAD_CURRENT(library_ctx.dlg);
					TODO("rich text label");
					PCB_DAD_LABEL(library_ctx.dlg, "");
						library_ctx.wtags = PCB_DAD_CURRENT(library_ctx.dlg);
				PCB_DAD_END(library_ctx.dlg);
			PCB_DAD_END(library_ctx.dlg);
		PCB_DAD_END(library_ctx.dlg);

		/* bottom */
		PCB_DAD_BUTTON_CLOSES(library_ctx.dlg, clbtn);
	PCB_DAD_END(library_ctx.dlg);

	/* set up the context */
	library_ctx.active = 1;

	PCB_DAD_NEW("library", library_ctx.dlg, "pcb-rnd Footprint Library", &library_ctx, pcb_false, library_close_cb);

	library_lib2dlg(&library_ctx);
	pcb_gui->attr_dlg_widget_state(library_ctx.dlg_hid_ctx, library_ctx.wedit, 0);
}

const char pcb_acts_LibraryDialog[] = "libraryDialog()\n";
const char pcb_acth_LibraryDialog[] = "Open the library dialog.";
fgw_error_t pcb_act_LibraryDialog(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	if (strcmp(pcb_gui->name, "lesstif") == 0)
		pcb_actionl("DoWindows", "library");
	else
		pcb_dlg_library();
	return 0;
}

static void library_changed_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (library_ctx.active)
		library_lib2dlg(&library_ctx);
}

void pcb_dlg_library_uninit(void)
{
	pcb_event_unbind_allcookie(library_cookie);
}

void pcb_dlg_library_init(void)
{
	pcb_event_bind(PCB_EVENT_LIBRARY_CHANGED, library_changed_ev, NULL, library_cookie);
}

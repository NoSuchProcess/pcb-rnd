/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019,2020 Tibor 'Igor2' Palinkas
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

#include <librnd/core/actions.h>
#include "board.h"
#include "buffer.h"
#include "conf_core.h"
#include "data.h"
#include "draw.h"
#include "event.h"
#include "obj_subc.h"
#include "plug_footprint.h"
#include <librnd/core/tool.h>
#include "undo.h"

#include <librnd/core/hid.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/hid_dad_tree.h>
#include <librnd/core/hid_init.h>

#include "dlg_library.h"

static const char *library_cookie = "dlg_library";

#define MAX_PARAMS 128

typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	int wtree, wpreview, wtags, wfilt, wpend, wnopend, wedit;

	int active; /* already open - allow only one instance */

	pcb_subc_t *sc;
	pcb_board_t *prev_pcb; /* sc must be in here so buffer changes don't ruin it */

	rnd_hidval_t timer;
	int timer_active;

 /* for the parametric */
	int pactive; /* already open - allow only one instance */
	int pwdesc;
	RND_DAD_DECL_NOINIT(pdlg)
	pcb_fplibrary_t *last_l;
	char *example, *help_params;
	htsi_t param_names;     /* param_name -> param_idx */
	int pwid[MAX_PARAMS];   /* param_idx -> widget_idx (for the input field widget) */
	char *pnames[MAX_PARAMS]; /* param_idx -> parameter_name (also key stored in the param_names hash */
	int num_params, first_optional;
	unsigned last_clicked:1; /* 1 if the last user action was a click in the tree, 0 if it was an edit of the filter text */
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
	rnd_box_t bbox;
	rnd_hid_attr_val_t hv;
	gds_t tmp;

	if (ctx->sc != NULL) {
		pcb_undo_freeze_add();
		pcb_subc_remove(ctx->sc);
		pcb_undo_unfreeze_add();
		ctx->sc = NULL;
	}

	gds_init(&tmp);
	if (sc != NULL) {
		ctx->sc = pcb_subc_dup_at(ctx->prev_pcb, ctx->prev_pcb->Data, sc, 0, 0, 1, rnd_false);
		pcb_data_bbox(&bbox, ctx->sc->data, 0);
		rnd_dad_preview_zoomto(&ctx->dlg[ctx->wpreview], &bbox);
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

		hv.str = tmp.array;
	}
	else
		hv.str = "";

	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wtags, &hv);
	gds_uninit(&tmp);
}

static void timed_update_preview_(library_ctx_t *ctx, const char *otext)
{
	if (pcb_buffer_load_footprint(PCB_PASTEBUFFER, otext, NULL)) {
		rnd_tool_select_by_name(&PCB->hidlib, "buffer");
		if (pcb_subclist_length(&PCB_PASTEBUFFER->Data->subc) != 0)
			library_update_preview(ctx, pcb_subclist_first(&PCB_PASTEBUFFER->Data->subc), NULL);
		rnd_gui->invalidate_all(rnd_gui);
	}
	ctx->timer_active = 0;
	rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wpend, 1);
	rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wnopend, 0);
}

static void timed_update_preview_cb(rnd_hidval_t user_data)
{
	library_ctx_t *ctx = user_data.ptr;
	const char *otext = ctx->dlg[ctx->wfilt].val.str;
	timed_update_preview_(ctx, otext);
}

static void timed_update_preview(library_ctx_t *ctx, int active)
{
	if (ctx->timer_active) {
		rnd_gui->stop_timer(rnd_gui, ctx->timer);
		ctx->timer_active = 0;
		rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wpend, 1);
		rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wnopend, 0);
	}

	if (active) {
		rnd_hidval_t user_data;
		user_data.ptr = ctx;
		ctx->timer = rnd_gui->add_timer(rnd_gui, timed_update_preview_cb, 500, user_data);
		ctx->timer_active = 1;
		rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wpend, 0);
		rnd_gui->attr_dlg_widget_hide(ctx->dlg_hid_ctx, ctx->wnopend, 1);
	}
}

static void update_edit_button(library_ctx_t *ctx)
{
	const char *otext = ctx->dlg[ctx->wfilt].val.str;
	int param_entered = 0, param_selected = 0;
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(&(ctx->dlg[ctx->wtree]));

	if (row != NULL) {
		pcb_fplibrary_t *l = row->user_data;
		param_selected = (l != NULL) && (l->type == PCB_LIB_FOOTPRINT) && (l->data.fp.type == PCB_FP_PARAMETRIC);
	}

	param_entered = !ctx->pactive && (otext != NULL) && (strchr(otext, '(') != NULL);

	rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->wedit, param_selected || param_entered);
}

#include "dlg_library_param.c"

static void library_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	library_ctx_t *ctx = caller_data;

	if (ctx->pactive) {
		ctx->pactive = 0;
		RND_DAD_FREE(ctx->pdlg);
	}

	timed_update_preview(ctx, 0);
	pcb_board_free(ctx->prev_pcb);
	RND_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(library_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void create_lib_tree_model_recurse(rnd_hid_attribute_t *attr, pcb_fplibrary_t *parent_lib, rnd_hid_row_t *parent_row)
{
	char *cell[2];
	pcb_fplibrary_t *l;
	int n;

	cell[1] = NULL;
	for(l = parent_lib->data.dir.children.array, n = 0; n < parent_lib->data.dir.children.used; l++, n++) {
		rnd_hid_row_t *row;

		cell[0] = rnd_strdup(l->name);
		row = rnd_dad_tree_append_under(attr, parent_row, cell);
		row->user_data = l;
		if (l->type == PCB_LIB_DIR)
			create_lib_tree_model_recurse(attr, l, row);
	}
}


static void library_lib2dlg(library_ctx_t *ctx)
{
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	rnd_hid_row_t *r;
	char *cursor_path = NULL;

	attr = &ctx->dlg[ctx->wtree];
	tree = attr->wdata;

	/* remember cursor */
	r = rnd_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = rnd_strdup(r->cell[0]);

	/* remove existing items */
	rnd_dad_tree_clear(tree);

	/* add all items recursively */
	create_lib_tree_model_recurse(attr, &pcb_library, NULL);

	/* restore cursor */
	if (cursor_path != NULL) {
		rnd_hid_attr_val_t hv;
		hv.str = cursor_path;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wtree, &hv);
		free(cursor_path);
	}
}

static void library_select_show_param_example(library_ctx_t *ctx, pcb_fplibrary_t *l)
{
	char line[1024], *arg, *cmd, *end;
	FILE *f = library_param_get_help(ctx, l);
	while(fgets(line, sizeof(line), f) != NULL) {
		cmd = strchr(line, '@');
		if ((cmd == NULL) || (cmd[1] != '@'))
			continue;
		cmd+=2;
		arg = strpbrk(cmd, " \t\r\n");
		if (arg != NULL) {
			*arg = '\0';
			arg++;
			while(isspace(*arg)) arg++;
		}
		if (strcmp(cmd, "example") == 0) {
			if ((arg != NULL) && (*arg != '\0')) {
				end = strpbrk(arg, "\r\n");
				if (end != NULL)
					*end = '\0';
				timed_update_preview_(ctx, arg);
				break;
			}
		}
	}
	rnd_pclose(f);
}

static void library_select(rnd_hid_attribute_t *attrib, void *hid_ctx, rnd_hid_row_t *row)
{
	rnd_hid_attr_val_t hv;
	rnd_hid_tree_t *tree = attrib->wdata;
	library_ctx_t *ctx = tree->user_ctx;
	int close_param = 1;
	static pcb_fplibrary_t *last = NULL;

	ctx->last_clicked = 1;

	timed_update_preview(ctx, 0);

	library_update_preview(ctx, NULL, NULL);
	if (row != NULL) {
		pcb_fplibrary_t *l = row->user_data;
		if ((l != NULL) && (l->type == PCB_LIB_FOOTPRINT)) {
			if ((l->data.fp.type == PCB_FP_PARAMETRIC)) {
				if (last != l) { /* first click */
					library_select_show_param_example(ctx, l);
					update_edit_button(ctx);
				}
				else { /* second click */
					library_param_dialog(ctx, l);
					close_param = 0;
				}
			}
			else {
				if (pcb_buffer_load_footprint(PCB_PASTEBUFFER, l->data.fp.loc_info, NULL)) {
					rnd_tool_select_by_name(&PCB->hidlib, "buffer");
					if (pcb_subclist_length(&PCB_PASTEBUFFER->Data->subc) != 0)
						library_update_preview(ctx, pcb_subclist_first(&PCB_PASTEBUFFER->Data->subc), l);
					update_edit_button(ctx);
					rnd_gui->invalidate_all(rnd_gui);
				}
			}
		}
		last = l;
	}

	if (close_param)
		library_param_dialog(ctx, NULL);

	hv.str = NULL;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wpreview, &hv);

}

static void library_expose(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_gc_t gc, const rnd_hid_expose_ctx_t *e)
{
	library_ctx_t *ctx = prv->user_ctx;
	int orig_po = pcb_draw_force_termlab;
	if (ctx->sc != NULL) {
		pcb_draw_force_termlab = rnd_false;
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

static void library_tree_unhide(rnd_hid_tree_t *tree, gdl_list_t *rowlist, re_sei_t *preg, vtp0_t *taglist)
{
	rnd_hid_row_t *r, *pr;

	for(r = gdl_first(rowlist); r != NULL; r = gdl_next(rowlist, r)) {
		if (((preg == NULL) || (re_sei_exec(preg, r->cell[0]))) && tag_match(r->user_data, taglist)) {
			rnd_dad_tree_hide_all(tree, &r->children, 0); /* if this is a node with children, show all children */
			for(pr = r; pr != NULL; pr = rnd_dad_tree_parent_row(tree, pr)) /* also show all parents so it is visible */
				pr->hide = 0;
		}
		library_tree_unhide(tree, &r->children, preg, taglist);
	}
}

static void library_filter_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_inp)
{
	library_ctx_t *ctx = caller_data;
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	const char *otext;
	char *text, *sep, *para_start;
	int have_filter_text, is_para, is_para_closed = 0;

	ctx->last_clicked = 0;

	attr = &ctx->dlg[ctx->wtree];
	tree = attr->wdata;
	otext = attr_inp->val.str;
	text = rnd_strdup(otext);
	have_filter_text = (*text != '\0');

	para_start = strchr(otext, '(');
	is_para = (para_start != NULL);
	if (is_para)
		is_para_closed = (strchr(para_start, ')') != NULL);

	sep = strpbrk(text, " ()\t\r\n");
	if (sep != NULL)
		*sep = '\0';

	/* if an '(' is entered, stop new filtering, keep filter as is */
	if (is_para)
		goto skip_filter;

	/* hide or unhide everything */

	if (have_filter_text) {
		/* need to unhide for expand to work */
		rnd_dad_tree_hide_all(tree, &tree->rows, 0);
		rnd_dad_tree_update_hide(attr);
		rnd_dad_tree_expcoll(attr, NULL, 1, 1);
		rnd_dad_tree_hide_all(tree, &tree->rows, 1);
	}
	else
		rnd_dad_tree_hide_all(tree, &tree->rows, 0);

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
			tags = rnd_strdup(tags);
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

	rnd_dad_tree_update_hide(attr);

	skip_filter:;

	/* parametric footprints need to be refreshed on edit */
	if (is_para_closed)
		timed_update_preview(ctx, 1);

	update_edit_button(ctx);

	free(text);
}

static rnd_hid_row_t *find_fp_prefix_(rnd_hid_tree_t *tree, gdl_list_t *rowlist, const char *name, int namelen)
{
	rnd_hid_row_t *r, *pr;

	for(r = gdl_first(rowlist); r != NULL; r = gdl_next(rowlist, r)) {
		pcb_fplibrary_t *l = r->user_data;
		if ((rnd_strncasecmp(r->cell[0], name, namelen) == 0) && (l->type == PCB_LIB_FOOTPRINT) && (l->data.fp.type == PCB_FP_PARAMETRIC))
			return r;
		pr = find_fp_prefix_(tree, &r->children, name, namelen);
		if (pr != NULL)
			return pr;
	}
	return NULL;
}

static rnd_hid_row_t *find_fp_prefix(library_ctx_t *ctx, const char *name, int namelen)
{
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;

	attr = &ctx->dlg[ctx->wtree];
	tree = attr->wdata;

	return find_fp_prefix_(tree, &tree->rows, name, namelen);
}


static void library_edit_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_inp)
{
	library_ctx_t *ctx = caller_data;
	rnd_hid_attribute_t *attr;
	rnd_hid_row_t *r, *rnew;
	const char *otext = ctx->dlg[ctx->wfilt].val.str;
	char *name, *sep;
	int namelen;

	attr = &ctx->dlg[ctx->wtree];
	r = rnd_dad_tree_get_selected(attr);

	if (!ctx->last_clicked && (otext != NULL)) {
		name = rnd_strdup(otext);
		sep = strchr(name, '(');
		if (sep != NULL)
			*sep = '\0';
	}
	else {
		pcb_fplibrary_t *l = r->user_data;
		name = rnd_strdup(l->name);
		if (name != NULL) {
			rnd_hid_attr_val_t hv;
			hv.str = name;
			rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wfilt, &hv);
		}
	}

	if ((name == NULL) || (*name == '\0')) {
		rnd_message(RND_MSG_ERROR, "Filed to figure the name of the parametric footprint\n");
		return;
	}
	namelen = strlen(name);

	if ((r == NULL) || (rnd_strncasecmp(name, r->cell[0], namelen) != 0)) {
		/* no selection or wrong selection: go find the right one */
		rnew = find_fp_prefix(ctx, name, namelen);
	}
	else
		rnew = r;

	if (rnew != NULL) {
		if (r != rnew)
			rnd_dad_tree_jumpto(attr, rnew);
		library_param_dialog(ctx, rnew->user_data);
	}
	else
		rnd_message(RND_MSG_ERROR, "No such parametric footprint: '%s'\n", name);

	free(name);
}

static void library_refresh_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_inp)
{
	library_ctx_t *ctx = caller_data;
	rnd_hid_row_t *r = rnd_dad_tree_get_selected(&(ctx->dlg[ctx->wtree]));
	pcb_fplibrary_t *l;
	char *oname;

	if (r == NULL)
		return;

	l = r->user_data;
	if ((l == NULL) || (l->parent == NULL))
		return;

	while((l->parent != NULL) && (l->parent->parent != NULL)) l = l->parent;

	oname = rnd_strdup(l->name); /* need to save the name because refresh invalidates l */

	if (pcb_fp_rehash(&PCB->hidlib, l) == 0)
		rnd_message(RND_MSG_INFO, "Refreshed library '%s'\n", oname);
	else
		rnd_message(RND_MSG_ERROR, "Failed to refresh library '%s'\n", oname);

	free(oname);
}


static rnd_bool library_mouse(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_mouse_ev_t kind, rnd_coord_t x, rnd_coord_t y)
{
	return rnd_false;
}

static void pcb_dlg_library(void)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	const pcb_dflgmap_t *g;
	int n;

	if (library_ctx.active)
		return; /* do not open another */

	library_ctx.prev_pcb = pcb_board_new_(1);

	for(g = pcb_dflgmap, n = 0; g->name != NULL; g++,n++)
		pcb_layergrp_set_dflgly(library_ctx.prev_pcb, &(library_ctx.prev_pcb->LayerGroups.grp[n]), g, g->name, g->name);
	library_ctx.prev_pcb->LayerGroups.len = n;

	RND_DAD_BEGIN_VBOX(library_ctx.dlg);
		RND_DAD_COMPFLAG(library_ctx.dlg, RND_HATF_EXPFILL);
		RND_DAD_BEGIN_HPANE(library_ctx.dlg);
			/* left */
			RND_DAD_BEGIN_VBOX(library_ctx.dlg);
				RND_DAD_COMPFLAG(library_ctx.dlg, RND_HATF_EXPFILL);
				RND_DAD_TREE(library_ctx.dlg, 1, 1, NULL);
					RND_DAD_COMPFLAG(library_ctx.dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
					RND_DAD_TREE_SET_CB(library_ctx.dlg, selected_cb, library_select);
					RND_DAD_TREE_SET_CB(library_ctx.dlg, ctx, &library_ctx);
					library_ctx.wtree = RND_DAD_CURRENT(library_ctx.dlg);
				RND_DAD_BEGIN_HBOX(library_ctx.dlg);
					RND_DAD_STRING(library_ctx.dlg);
						RND_DAD_COMPFLAG(library_ctx.dlg, RND_HATF_EXPFILL);
						RND_DAD_HELP(library_ctx.dlg, "filter: display only footprints matching this text\n(if empty: display all)");
						RND_DAD_CHANGE_CB(library_ctx.dlg, library_filter_cb);
						library_ctx.wfilt = RND_DAD_CURRENT(library_ctx.dlg);
					RND_DAD_PICBUTTON(library_ctx.dlg, xpm_edit_param);
						RND_DAD_HELP(library_ctx.dlg, "open GUI to edit the parameters\nof a parametric footprint");
						RND_DAD_CHANGE_CB(library_ctx.dlg, library_edit_cb);
						library_ctx.wedit = RND_DAD_CURRENT(library_ctx.dlg);
					RND_DAD_PICBUTTON(library_ctx.dlg, xpm_refresh);
						RND_DAD_HELP(library_ctx.dlg, "reload and refresh the current\nmain tree of the library");
						RND_DAD_CHANGE_CB(library_ctx.dlg, library_refresh_cb);
				RND_DAD_END(library_ctx.dlg);
			RND_DAD_END(library_ctx.dlg);

			/* right */
			RND_DAD_BEGIN_VPANE(library_ctx.dlg);
				RND_DAD_COMPFLAG(library_ctx.dlg, RND_HATF_EXPFILL | RND_HATF_FRAME);
				/* right top */
				RND_DAD_PREVIEW(library_ctx.dlg, library_expose, library_mouse, NULL, NULL, 100, 100, &library_ctx);
					library_ctx.wpreview = RND_DAD_CURRENT(library_ctx.dlg);

				/* right bottom */
				RND_DAD_BEGIN_VBOX(library_ctx.dlg);
					RND_DAD_COMPFLAG(library_ctx.dlg, RND_HATF_EXPFILL);
					RND_DAD_LABEL(library_ctx.dlg, "Refreshing");
						RND_DAD_COMPFLAG(library_ctx.dlg, RND_HATF_HIDE);
						library_ctx.wpend = RND_DAD_CURRENT(library_ctx.dlg);
					RND_DAD_LABEL(library_ctx.dlg, " ");
						library_ctx.wnopend = RND_DAD_CURRENT(library_ctx.dlg);
					TODO("rich text label");
					RND_DAD_LABEL(library_ctx.dlg, "");
						library_ctx.wtags = RND_DAD_CURRENT(library_ctx.dlg);
				RND_DAD_END(library_ctx.dlg);
			RND_DAD_END(library_ctx.dlg);
		RND_DAD_END(library_ctx.dlg);

		/* bottom */
		RND_DAD_BUTTON_CLOSES(library_ctx.dlg, clbtn);
	RND_DAD_END(library_ctx.dlg);

	/* set up the context */
	library_ctx.active = 1;

	RND_DAD_NEW("library", library_ctx.dlg, "pcb-rnd Footprint Library", &library_ctx, rnd_false, library_close_cb);

	library_lib2dlg(&library_ctx);
	rnd_gui->attr_dlg_widget_state(library_ctx.dlg_hid_ctx, library_ctx.wedit, 0);
}

const char pcb_acts_LibraryDialog[] = "libraryDialog()\n";
const char pcb_acth_LibraryDialog[] = "Open the library dialog.";
fgw_error_t pcb_act_LibraryDialog(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	pcb_dlg_library();
	return 0;
}

static void library_changed_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if (library_ctx.active)
		library_lib2dlg(&library_ctx);
}

void pcb_dlg_library_uninit(void)
{
	rnd_event_unbind_allcookie(library_cookie);
}

void pcb_dlg_library_init(void)
{
	rnd_event_bind(PCB_EVENT_LIBRARY_CHANGED, library_changed_ev, NULL, library_cookie);
}

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  hand assembly assistant GUI
 *  pcb-rnd Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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

#include <stdio.h>
#include <genvector/gds_char.h>
#include <genlist/gendlist.h>

#include "board.h"
#include "data.h"
#include "plugins.h"
#include "actions.h"
#include "compat_misc.h"
#include "obj_subc.h"
#include "pcb-printf.h"
#include "hid_dad.h"
#include "hid_dad_tree.h"
#include "search.h"
#include "draw.h"
#include "select.h"
#include "asm_conf.h"
#include "../src_plugins/asm/conf_internal.c"

conf_asm_t conf_asm;
#define ASM_CONF_FN "asm.conf"


static const char *asm_cookie = "asm plugin";

/*** internal list of all parts, grouped; have to be arrays for qsort(), so can't
     avoid it and just use tree-table list based trees ***/
typedef enum {
	TT_ATTR,
	TT_SIDE,
	TT_X,
	TT_Y
} ttype_t;

typedef struct {
	ttype_t type;
	const char *key;
	gdl_elem_t link;
} template_t;

typedef struct {
	int is_grp;
	char *name;
	pcb_hid_row_t *row;
	vtp0_t parts;
} group_t;

typedef struct {
	int is_grp;
	char *name;
	long int id;
	int done;
	pcb_hid_row_t *row;
	group_t *parent;
} part_t;

/* dialog context */
typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	vtp0_t grps;
	vtclr_t layer_colors; /* saved before greying out */
	int wtbl, wskipg, wdoneg, wskipp, wdonep;
	int active; /* already open - allow only one instance */
} asm_ctx_t;

asm_ctx_t asm_ctx;

/*** template compiling ***/
static void templ_append(gdl_list_t *dst, ttype_t type, const char *key)
{
	template_t *t = calloc(sizeof(template_t), 1);
	t->type = type;
	t->key = key;
	gdl_append(dst, t, link);
}

static char *templ_compile(gdl_list_t *dst, const char *src_)
{
	char *s, *next, *src = pcb_strdup(src_);

	for(s = src; (s != NULL) && (*s != '\0'); s = next) {
		while(isspace(*s) || (*s == ',')) s++;
		next = strpbrk(s, " \t\r\n,");
		if (next != NULL) {
			*next = '\0';
			next++;
		}
		if ((s[0] == 'a') && (s[1] == '.')) {
			s+=2;
			templ_append(dst, TT_ATTR, s);
		}
		else if (strcmp(s, "side") == 0)
			templ_append(dst, TT_SIDE, NULL);
		else if (strcmp(s, "x") == 0)
			templ_append(dst, TT_X, NULL);
		else if (strcmp(s, "y") == 0)
			templ_append(dst, TT_Y, NULL);
		else
			pcb_message(PCB_MSG_ERROR, "Ignoring unknown asm template entry: '%s'\n", s);
	}
	return src;
}

/* allocate a string and build a sortable text summary of a subc using the template */
static char *templ_exec(pcb_subc_t *subc, gdl_list_t *temp)
{
	gds_t s;
	template_t *t;
	int n, bot, have_coords = 0;
	char *tmp, buf[64];
	pcb_coord_t x = 0, y = 0;

	gds_init(&s);
	for(n = 0, t = gdl_first(temp); t != NULL; n++, t = gdl_next(temp, t)) {
		if (n != 0)
			gds_append(&s, ',');
		switch(t->type) {
			case TT_ATTR:
				tmp = pcb_attribute_get(&subc->Attributes, t->key);
				if (tmp != NULL)
					gds_append_str(&s, tmp);
				break;
			case TT_SIDE:
				bot = 0;
				pcb_subc_get_side(subc, &bot);
				gds_append_str(&s, (bot ? "1/bottom" : "0/top"));
				break;
			case TT_X:
				if (!have_coords) {
					pcb_subc_get_origin(subc, &x, &y);
					have_coords = 1;
				}
				pcb_sprintf(buf, "%.08mm", x);
				break;
			case TT_Y:
				if (!have_coords) {
					pcb_subc_get_origin(subc, &x, &y);
					have_coords = 1;
				}
				pcb_sprintf(buf, "%.08mm", y);
				break;
		}
	}
	return s.array;
}

static void templ_free(char *tmp, gdl_list_t *dst)
{
	template_t *t;

	for(t = gdl_first(dst); t != NULL; t = gdl_first(dst)) {
		gdl_remove(dst, t, link);
		free(t);
	}

	free(tmp);
}

static group_t *group_lookup(vtp0_t *grps, htsp_t *gh, char *name, int alloc)
{
	group_t *g = htsp_get(gh, name);
	if (g != NULL) {
		free(name);
		return g;
	}

	g = calloc(sizeof(group_t), 1);
	g->is_grp = 1;
	g->name = name;

	vtp0_append(grps, g);
	htsp_set(gh, name, g);
	return g;
}

static void part_append(group_t *g, char *sortstr, long int id)
{
	part_t *p = malloc(sizeof(part_t));
	p->is_grp = 0;
	p->name = sortstr;
	p->id = id;
	p->done = 0;
	p->parent = g;
	vtp0_append(&g->parts, p);
}

/* Extract the part list from data */
static void asm_extract(vtp0_t *dst, pcb_data_t *data, const char *group_template, const char *sort_template)
{
	gdl_list_t cgroup, csort;
	char *tmp_group, *tmp_sort;
	htsp_t gh;

	memset(&cgroup, 0, sizeof(cgroup));
	memset(&csort, 0, sizeof(csort));
	tmp_group = templ_compile(&cgroup, group_template);
	tmp_sort = templ_compile(&csort, sort_template);

	htsp_init(&gh, strhash, strkeyeq);

	PCB_SUBC_LOOP(data);
	{
		char *grp, *srt;
		group_t *g;

		grp = templ_exec(subc, &cgroup);
		srt = templ_exec(subc, &csort);
		g = group_lookup(dst, &gh, grp, 1);
		part_append(g, srt, subc->ID);
		/* no need to free grp or srt, they are stored in the group and part structs */
	}
	PCB_END_LOOP;

	htsp_uninit(&gh);
	templ_free(tmp_group, &cgroup);
	templ_free(tmp_sort, &csort);
}

/*** Sort the part list ***/
static int group_cmp(const void *ga_, const void *gb_)
{
	const group_t * const *ga = ga_;
	const group_t * const *gb = gb_;
	return strcmp((*ga)->name, (*gb)->name);
}

static int part_cmp(const void *pa_, const void *pb_)
{
	const part_t * const *pa = pa_;
	const part_t * const *pb = pb_;
	return strcmp((*pa)->name, (*pb)->name);
}

static void asm_sort(vtp0_t *gv)
{
	group_t **g;
	long n;

	qsort(gv->array, gv->used, sizeof(void *), group_cmp);
	for(g = (group_t **)gv->array, n = 0; n < gv->used; g++,n++)
		qsort((*g)->parts.array, (*g)->parts.used, sizeof(void *), part_cmp);
}

/*** Gray out all layers to make selection stick out more ***/
static int fade(int c, int factor)
{
	if (c > 127)
		return 127 + (c - 127) / factor;
	return 127 - (127 - c) / factor;
}

static void asm_greyout(int grey)
{
	int n;
	pcb_data_t *data = PCB->Data;
	pcb_layer_t *ly;

	if (grey) {
		vtclr_init(&asm_ctx.layer_colors);
		vtclr_enlarge(&asm_ctx.layer_colors, data->LayerN);
		for(n = 0, ly = data->Layer; n < data->LayerN; n++,ly++) {
			int r, g, b;

			vtclr_set(&asm_ctx.layer_colors, n, ly->meta.real.color);
			r = ly->meta.real.color.r;
			g = ly->meta.real.color.g;
			b = ly->meta.real.color.b;
			r = fade(r, 4);
			g = fade(g, 4);
			b = fade(b, 4);
			pcb_color_load_int(&ly->meta.real.color, r, g, b, 255);
		}
	}
	else {
		for(n = 0, ly = data->Layer; n < data->LayerN; n++,ly++)
			ly->meta.real.color = asm_ctx.layer_colors.array[n];
		vtclr_uninit(&asm_ctx.layer_colors);
	}
	pcb_hid_redraw(PCB);
}

/*** UI callbacks ***/
static void asm_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	group_t **g;
	part_t **p;
	long i, n;
	asm_ctx_t *ctx = caller_data;
	asm_greyout(0);

	for(g = (group_t **)asm_ctx.grps.array, n = 0; n < asm_ctx.grps.used; g++,n++) {
		for(p = (part_t **)(*g)->parts.array, i = 0; i < (*g)->parts.used; p++,i++)
			free(*p);
		vtp0_uninit(&(*g)->parts);
		free(*g);
	}
	vtp0_uninit(&asm_ctx.grps);

	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(asm_ctx_t));
}

static void select_part(part_t *p)
{
	void *r1, *r2, *r3;
	pcb_subc_t *sc;
	pcb_objtype_t type;
	pcb_box_t view;

	type = pcb_search_obj_by_id_(PCB->Data, &r1, &r2, &r3, p->id, PCB_OBJ_SUBC);
	if (type != PCB_OBJ_SUBC)
		return;

	sc = (pcb_subc_t *)r2;
	pcb_subc_select(PCB, sc, PCB_CHGFLG_SET, 1);

	/* if the part is out of the screen, pan there */
	pcb_gui->view_get(pcb_gui, &view);
	if ((view.X2 < sc->BoundingBox.X1) || (view.X1 > sc->BoundingBox.X2) || (view.Y2 < sc->BoundingBox.Y1) || (view.Y1 > sc->BoundingBox.Y2))
		pcb_gui->pan(pcb_gui, (sc->BoundingBox.X1+sc->BoundingBox.X2)/2, (sc->BoundingBox.Y1+sc->BoundingBox.Y2)/2, 0);
}

static void asm_row_selected(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	long n;
	int isgrp = 0, ispart = 0;
	pcb_box_t box;

	/* unselect all */
	box.X1 = -PCB_MAX_COORD;
	box.Y1 = -PCB_MAX_COORD;
	box.X2 = PCB_MAX_COORD;
	box.Y2 = PCB_MAX_COORD;
	if (pcb_select_block(PCB, &box, pcb_false, pcb_false, pcb_false))
			pcb_board_set_changed_flag(pcb_true);

	if (row == NULL) {
		goto skip;
	}
	if (*(int *)row->user_data) {
		group_t *g = row->user_data;
		part_t **p;
		isgrp = 1;
		for(n = 0, p = (part_t **)g->parts.array; n < g->parts.used; n++,p++)
			select_part(*p);
	}
	else {
		part_t *p = row->user_data;
		ispart = 1;
		select_part(p);
	}

	skip:;
	pcb_gui->attr_dlg_widget_state(hid_ctx, asm_ctx.wskipg, isgrp | ispart);
	pcb_gui->attr_dlg_widget_state(hid_ctx, asm_ctx.wdoneg, isgrp | ispart);
	pcb_gui->attr_dlg_widget_state(hid_ctx, asm_ctx.wskipp, ispart);
	pcb_gui->attr_dlg_widget_state(hid_ctx, asm_ctx.wdonep, ispart);
	pcb_hid_redraw(PCB); /* for displaying the new selection */
}

static void skip(void *hid_ctx, int pick_grp, pcb_hid_row_t *row)
{
	pcb_hid_row_t *nr = NULL;
	int is_grp = *(int *)row->user_data;

	if (pick_grp && !is_grp) {
		/* special case: next group from a part: skip the whole group */
		goto skip_parent;
	}
	else if (!pick_grp || is_grp) {
		/* try to pick the next row first */
		nr = row->link.next;
	}

	if (nr == NULL) {
		part_t *p;
		if (is_grp)
			goto last; /* skipping from the last group -> unselect all */
		/* skipping from last part in a group -> jump to next group's first part */
		skip_parent:;
		p = row->user_data;
		nr = p->parent->row;
		nr = nr->link.next;
		if (nr == NULL)
			goto last; /* skipping from the last part of the last group -> unselect all */
		nr = gdl_first(&nr->children);
	}
	pcb_dad_tree_jumpto(&asm_ctx.dlg[asm_ctx.wtbl], nr);
	return;

	last:;
	/* what happens after the last */
	pcb_dad_tree_jumpto(&asm_ctx.dlg[asm_ctx.wtbl], NULL);
	return;
}

static void group_progress_update(void *hid_ctx, group_t *grp)
{
	long n, done = 0, total = vtp0_len(&grp->parts);
	char *tmp;

	for(n = 0; n < total; n++) {
		part_t *p = grp->parts.array[n];
		if (p->done)
			done++;
	}

	tmp = pcb_strdup_printf("%d/%d", done, total);
	pcb_dad_tree_modify_cell(&asm_ctx.dlg[asm_ctx.wtbl], grp->row, 5, tmp);
}

static void done(void *hid_ctx, part_t *part, int done)
{
	part->done = done;
	if (done)
		pcb_dad_tree_modify_cell(&asm_ctx.dlg[asm_ctx.wtbl], part->row, 5, "yes");
	else
		pcb_dad_tree_modify_cell(&asm_ctx.dlg[asm_ctx.wtbl], part->row, 5, "-");
	group_progress_update(hid_ctx, part->parent);
}

static void asm_done_part(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_row_t *row = pcb_dad_tree_get_selected(&asm_ctx.dlg[asm_ctx.wtbl]);
	if (*(int *)row->user_data)
		return;
	done(hid_ctx, row->user_data, 1);
	skip(hid_ctx, 0, row);
}

static void asm_undo_part(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_row_t *row = pcb_dad_tree_get_selected(&asm_ctx.dlg[asm_ctx.wtbl]);
	if (*(int *)row->user_data)
		return;
	done(hid_ctx, row->user_data, 0);
	skip(hid_ctx, 0, row);
}

static void asm_skip_part(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_row_t *row = pcb_dad_tree_get_selected(&asm_ctx.dlg[asm_ctx.wtbl]);
	if (*(int *)row->user_data)
		return;
	skip(hid_ctx, 0, row);
}

static void asm_done_group_(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr, int dn)
{
	long n;
	group_t *g;
	pcb_hid_row_t *row = pcb_dad_tree_get_selected(&asm_ctx.dlg[asm_ctx.wtbl]);

	if (!*(int *)row->user_data) {
		part_t *p = row->user_data;
		g = p->parent;
	}
	else
		g = row->user_data;

	for(n = 0; n < g->parts.used; n++)
		done(hid_ctx, g->parts.array[n], dn);
	skip(hid_ctx, 1, row);
}

static void asm_done_group(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	asm_done_group_(hid_ctx, caller_data, attr, 1);
}

static void asm_undo_group(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	asm_done_group_(hid_ctx, caller_data, attr, 0);
}


static void asm_skip_group(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_row_t *row = pcb_dad_tree_get_selected(&asm_ctx.dlg[asm_ctx.wtbl]);

	skip(hid_ctx, 1, row);
}


static const char pcb_acts_asm[] = "asm()";
static const char pcb_acth_asm[] = "Interactive assembly assistant";
fgw_error_t pcb_act_asm(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *hdr[] = { "name", "refdes", "footprint", "value", "comments", "done", NULL };
	char *row[7];
	group_t **g;
	part_t **p;
	long n, i;
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	if (asm_ctx.active) {
		PCB_ACT_IRES(0);
		return 0;
	}

	vtp0_init(&asm_ctx.grps);
	asm_extract(&asm_ctx.grps, PCB->Data, conf_asm.plugins.asm1.group_template, conf_asm.plugins.asm1.sort_template);
	asm_sort(&asm_ctx.grps);

	asm_greyout(1);

	PCB_DAD_BEGIN_VBOX(asm_ctx.dlg);
		PCB_DAD_COMPFLAG(asm_ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_TREE(asm_ctx.dlg, 6, 1, hdr);
			asm_ctx.wtbl = PCB_DAD_CURRENT(asm_ctx.dlg);
			PCB_DAD_COMPFLAG(asm_ctx.dlg, PCB_HATF_SCROLL);
			for(g = (group_t **)asm_ctx.grps.array, n = 0; n < asm_ctx.grps.used; g++,n++) {
				pcb_hid_row_t *parent, *child;
				row[0] = (*g)->name;
				row[1] = "";
				row[2] = "";
				row[3] = "";
				row[4] = "";
				row[5] = "";
				row[6] = NULL;
				parent = PCB_DAD_TREE_APPEND(asm_ctx.dlg, NULL, row);
				parent->user_data = *g;
				(*g)->row = parent;
				for(p = (part_t **)(*g)->parts.array, i = 0; i < (*g)->parts.used; p++,i++) {
					void *r1, *r2, *r3;
					pcb_subc_t *sc;
					pcb_objtype_t type;

					type = pcb_search_obj_by_id_(PCB->Data, &r1, &r2, &r3, (*p)->id, PCB_OBJ_SUBC);
					sc = r2;

					row[0] = (*p)->name;
					if (type == PCB_OBJ_SUBC) {
						int m;
						row[1] = (char *)sc->refdes;
						row[2] = pcb_attribute_get(&sc->Attributes, "footprint");
						row[3] = pcb_attribute_get(&sc->Attributes, "value");
						row[4] = pcb_attribute_get(&sc->Attributes, "asm::comment");
						row[5] = "";
						for(m = 1; m < 6; m++)
							if (row[m] == NULL)
								row[m] = "";
					}
					else {
						row[1] = "";
						row[2] = "";
						row[3] = "";
						row[4] = "";
						row[5] = "";
					}
					row[6] = NULL;
					child = PCB_DAD_TREE_APPEND_UNDER(asm_ctx.dlg, parent, row);
					child->user_data = *p;
					(*p)->row = child;
				}
			}
			PCB_DAD_TREE_SET_CB(asm_ctx.dlg, selected_cb, asm_row_selected);
		PCB_DAD_BEGIN_HBOX(asm_ctx.dlg);
			PCB_DAD_BUTTON(asm_ctx.dlg, "skip part");
				asm_ctx.wskipp = PCB_DAD_CURRENT(asm_ctx.dlg);
				PCB_DAD_HELP(asm_ctx.dlg, "Do not populate this part,\ncontinue with the next part");
				PCB_DAD_CHANGE_CB(asm_ctx.dlg, asm_skip_part);
			PCB_DAD_BUTTON(asm_ctx.dlg, "skip group");
				asm_ctx.wskipg = PCB_DAD_CURRENT(asm_ctx.dlg);
				PCB_DAD_HELP(asm_ctx.dlg, "Stop populating this group,\ncontinue with the next group");
				PCB_DAD_CHANGE_CB(asm_ctx.dlg, asm_skip_group);
			PCB_DAD_BUTTON(asm_ctx.dlg, "done part");
				asm_ctx.wdonep = PCB_DAD_CURRENT(asm_ctx.dlg);
				PCB_DAD_HELP(asm_ctx.dlg, "Mark current part done,\ncontinue with the next part");
				PCB_DAD_CHANGE_CB(asm_ctx.dlg, asm_done_part);
			PCB_DAD_BUTTON(asm_ctx.dlg, "undo part");
				asm_ctx.wdonep = PCB_DAD_CURRENT(asm_ctx.dlg);
				PCB_DAD_HELP(asm_ctx.dlg, "Remove the done mark from the current part,\njump to the next part");
				PCB_DAD_CHANGE_CB(asm_ctx.dlg, asm_undo_part);
			PCB_DAD_BUTTON(asm_ctx.dlg, "done group");
				asm_ctx.wdoneg = PCB_DAD_CURRENT(asm_ctx.dlg);
				PCB_DAD_HELP(asm_ctx.dlg, "Mark all parts in this group done,\ncontinue with the next group");
				PCB_DAD_CHANGE_CB(asm_ctx.dlg, asm_done_group);
			PCB_DAD_BUTTON(asm_ctx.dlg, "undo group");
				asm_ctx.wdoneg = PCB_DAD_CURRENT(asm_ctx.dlg);
				PCB_DAD_HELP(asm_ctx.dlg, "Remove the done mark from all parts in this group done,\ncontinue with the next group");
				PCB_DAD_CHANGE_CB(asm_ctx.dlg, asm_undo_group);
		PCB_DAD_END(asm_ctx.dlg);
		PCB_DAD_BUTTON_CLOSES(asm_ctx.dlg, clbtn);
	PCB_DAD_END(asm_ctx.dlg);

	asm_ctx.active = 1;
	PCB_DAD_NEW("asm", asm_ctx.dlg, "Hand assembly with pcb-rnd", &asm_ctx, pcb_false, asm_close_cb);

	/* expand all groups by default */
	for(g = (group_t **)asm_ctx.grps.array, n = 0; n < asm_ctx.grps.used; g++,n++)
		pcb_dad_tree_expcoll(&asm_ctx.dlg[asm_ctx.wtbl], (*g)->row, 1, 0);

	PCB_ACT_IRES(0);
	return 0;
}

static pcb_action_t asm_action_list[] = {
	{"asm", pcb_act_asm, pcb_acth_asm, pcb_acts_asm}
};

PCB_REGISTER_ACTIONS(asm_action_list, asm_cookie)

int pplg_check_ver_asm(int ver_needed) { return 0; }

void pplg_uninit_asm(void)
{
	pcb_remove_actions_by_cookie(asm_cookie);
	pcb_conf_unreg_file(ASM_CONF_FN, asm_conf_internal);
	pcb_conf_unreg_fields("plugins/asm1/");
}


#include "dolists.h"
int pplg_init_asm(void)
{
	PCB_API_CHK_VER;
	pcb_conf_reg_file(ASM_CONF_FN, asm_conf_internal);
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	pcb_conf_reg_field(conf_asm, field,isarray,type_name,cpath,cname,desc,flags);
#include "asm_conf_fields.h"

	PCB_REGISTER_ACTIONS(asm_action_list, asm_cookie)

	return 0;
}

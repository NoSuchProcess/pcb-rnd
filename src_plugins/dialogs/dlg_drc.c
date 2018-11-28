/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#include <ctype.h>
#include <genht/htsp.h>
#include <genht/hash.h>

#include "actions.h"
#include "conf_core.h"
#include "drc.h"
#include "draw.h"
#include "hid_inlines.h"
#include "hid_dad.h"
#include "hid_dad_tree.h"

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	pcb_board_t *pcb;
	pcb_drc_list_t drc;
	int alloced, active;

	unsigned long int selected;

	int wlist, wcount, wprev, wexplanation, wmeasure;
} drc_ctx_t;

drc_ctx_t drc_ctx;

static void drc_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	drc_ctx_t *ctx = caller_data;


	PCB_DAD_FREE(ctx->dlg);
	if (ctx->alloced)
		free(ctx);
}

void drc2dlg(drc_ctx_t *ctx)
{
	char tmp[32];
	pcb_drc_violation_t *v;
	pcb_hid_attribute_t *attr;
	pcb_hid_tree_t *tree;
	pcb_hid_row_t *r;
	char *cell[2], *cursor_path = NULL;

	sprintf(tmp, "%d", pcb_drc_list_length(&ctx->drc));
	PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wcount, str_value, tmp);

	attr = &ctx->dlg[ctx->wlist];
	tree = (pcb_hid_tree_t *)attr->enumerations;

	/* remember cursor */
	r = pcb_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = pcb_strdup(r->cell[0]);

	/* remove existing items */
	for(r = gdl_first(&tree->rows); r != NULL; r = gdl_first(&tree->rows))
		pcb_dad_tree_remove(attr, r);

	/* add all items */
	for(v = pcb_drc_list_first(&ctx->drc); v != NULL; v = pcb_drc_list_next(v)) {
		cell[0] = pcb_strdup_printf("%lu", v->uid);
		cell[1] = pcb_strdup(v->title);
		pcb_dad_tree_append(attr, NULL, cell);
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		pcb_hid_attr_val_t hv;
		hv.str_value = cursor_path;
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wlist, &hv);
		free(cursor_path);
	}
}

static char *re_wrap(char *inp, int len)
{
	int cnt;
	char *s, *lastspc = NULL;
	for(s = inp, cnt = 0; *s != '\0'; s++,cnt++) {
		if (*s == '\n')
			*s = ' ';
		if ((cnt >= len) && (lastspc != NULL)) {
			cnt = 0;
			*lastspc = '\n';
			lastspc = NULL;
		}
		else if (isspace(*s))
			lastspc = s;
	}
	return inp;
}

static void drc_select(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	pcb_hid_attr_val_t hv;
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attrib->enumerations;
	drc_ctx_t *ctx = tree->user_ctx;
	pcb_drc_violation_t *v = NULL;

	if (row != NULL) {
		ctx->selected = strtoul(row->cell[0], NULL, 10);
		v = pcb_drc_by_uid(&ctx->drc, ctx->selected);
		if (v != NULL) {
			pcb_drc_goto(v);
			PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wexplanation, str_value, re_wrap(pcb_strdup(v->explanation), 32));
			if (v->have_measured)
				PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wmeasure, str_value, pcb_strdup_printf("%m+required: %$ms\nmeasured: %$ms\n", conf_core.editor.grid_unit->allow, v->required_value, v->measured_value));
			else
				PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wmeasure, str_value, pcb_strdup_printf("%m+required: %$ms\n", conf_core.editor.grid_unit->allow, v->required_value));
		}
	}

	if (v == NULL) {
		ctx->selected = 0;
		PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wexplanation, str_value, pcb_strdup(""));
		PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wmeasure, str_value, pcb_strdup(""));
	}

	pcb_dad_preview_zoomto(&ctx->dlg[ctx->wprev], &v->bbox);
}

static void drc_expose_cb(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	int old_termlab;
	/* NOTE: zoom box was already set on select */

	/* draw the board */
	old_termlab = pcb_draw_force_termlab;
	pcb_draw_force_termlab = 1;
	pcb_hid_expose_all(pcb_gui, e, NULL);
	pcb_draw_force_termlab = old_termlab;
}


static pcb_bool drc_mouse_cb(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_false; /* don't redraw */
}


static void pcb_dlg_drc(drc_ctx_t *ctx, const char *title)
{
	const char *hdr[] = { "ID", "title", NULL };


	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_BEGIN_HPANE(ctx->dlg);

			/* left */
			PCB_DAD_BEGIN_VBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);

				PCB_DAD_BEGIN_HBOX(ctx->dlg);
					PCB_DAD_LABEL(ctx->dlg, "Number of violations:");
					PCB_DAD_LABEL(ctx->dlg, "n/a");
					ctx->wcount = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_END(ctx->dlg);

				PCB_DAD_TREE(ctx->dlg, 2, 0, hdr);
					PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_SCROLL | PCB_HATF_EXPFILL);
					PCB_DAD_TREE_SET_CB(ctx->dlg, selected_cb, drc_select);
					PCB_DAD_TREE_SET_CB(ctx->dlg, ctx, ctx);
					ctx->wlist = PCB_DAD_CURRENT(ctx->dlg);

				PCB_DAD_BEGIN_HBOX(ctx->dlg);
					PCB_DAD_BUTTON(ctx->dlg, "Copy");
					PCB_DAD_BUTTON(ctx->dlg, "Cut");
					PCB_DAD_BUTTON(ctx->dlg, "Del");
				PCB_DAD_END(ctx->dlg);

				PCB_DAD_BEGIN_HBOX(ctx->dlg);
					PCB_DAD_BUTTON(ctx->dlg, "Paste before");
					PCB_DAD_BUTTON(ctx->dlg, "Paste after");
				PCB_DAD_END(ctx->dlg);
			PCB_DAD_END(ctx->dlg);

			/* right */
			PCB_DAD_BEGIN_VBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_PREVIEW(ctx->dlg, drc_expose_cb, drc_mouse_cb, NULL, NULL, ctx);
					ctx->wprev = PCB_DAD_CURRENT(ctx->dlg);
					PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_LABEL(ctx->dlg, "(explanation)");
					ctx->wexplanation = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "(measure)");
					ctx->wmeasure = PCB_DAD_CURRENT(ctx->dlg);
			PCB_DAD_END(ctx->dlg);
		PCB_DAD_END(ctx->dlg);

		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_BUTTON(ctx->dlg, "Refresh");
					PCB_DAD_BEGIN_HBOX(ctx->dlg);
						PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
					PCB_DAD_END(ctx->dlg);
					PCB_DAD_BUTTON(ctx->dlg, "Close");
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);

	PCB_DAD_NEW(ctx->dlg, title, "", ctx, pcb_false, drc_close_cb);

	ctx->active = 1;
}

const char pcb_acts_DRC[] = "DRC()\n";
const char pcb_acth_DRC[] = "Execute drc checks";
fgw_error_t pcb_act_DRC(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	static drc_ctx_t ctx = {0};

	if (!ctx.active)
		pcb_dlg_drc(&ctx, "DRC violations");

	pcb_drc_all(&ctx.drc);
	drc2dlg(&ctx);

	return 0;
}

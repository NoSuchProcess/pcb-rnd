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
#include <genht/hash.h>

#include "actions.h"
#include "board.h"
#include "draw.h"
#include "obj_subc.h"
#include "plug_footprint.h"

#include "hid.h"
#include "hid_dad.h"
#include "hid_dad_tree.h"
#include "hid_init.h"

#include "dlg_library.h"

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	int wtree;

	int active; /* already open - allow only one instance */
	pcb_subc_t *sc;
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
"  @*        *@  ",
" *@ ++++++++ @* ",
" @* +      + *@ ",
" @  + @@@@ +  @ ",
"*@  +      +  @*",
"@*  + @@*  +  *@",
"@   +      +   @",
"@   + @ @*@+   @",
"@*  +      +  *@",
"*@  +   @@ +  @*",
" @  + @@@@ +  @ ",
" @* +      + *@ ",
" *@ ++++++++ @* ",
"  @*        *@  ",
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
"   @@*@@@@      ",
"   @@@*   @@    ",
"   @@@@     @   ",
"            *@  ",
"             @  ",
" *           *@ ",
" @            @ ",
" @            @ ",
" @*           * ",
"  @             ",
"  @*            ",
"   @     @@@@   ",
"    @@   *@@@   ",
"      @@@@*@@   ",
"            @   ",
};

static void library_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	library_ctx_t *ctx = caller_data;

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
	char *cell[3], *cursor_path = NULL;

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

static pcb_bool library_mouse(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_false;
}

static void pcb_dlg_library(void)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	if (library_ctx.active)
		return; /* do not open another */

	PCB_DAD_BEGIN_VBOX(library_ctx.dlg);
		PCB_DAD_COMPFLAG(library_ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_HPANE(library_ctx.dlg);
			/* left */
			PCB_DAD_BEGIN_VBOX(library_ctx.dlg);
				PCB_DAD_COMPFLAG(library_ctx.dlg, PCB_HATF_EXPFILL);
				PCB_DAD_TREE(library_ctx.dlg, 1, 1, NULL);
					PCB_DAD_COMPFLAG(library_ctx.dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
					library_ctx.wtree = PCB_DAD_CURRENT(library_ctx.dlg);
				PCB_DAD_BEGIN_HBOX(library_ctx.dlg);
					PCB_DAD_STRING(library_ctx.dlg);
						PCB_DAD_COMPFLAG(library_ctx.dlg, PCB_HATF_EXPFILL);
					PCB_DAD_PICBUTTON(library_ctx.dlg, xpm_edit_param);
					PCB_DAD_PICBUTTON(library_ctx.dlg, xpm_refresh);
				PCB_DAD_END(library_ctx.dlg);
			PCB_DAD_END(library_ctx.dlg);

			/* right */
			PCB_DAD_BEGIN_VPANE(library_ctx.dlg);
				PCB_DAD_COMPFLAG(library_ctx.dlg, PCB_HATF_EXPFILL | PCB_HATF_FRAME);
				/* right top */
				PCB_DAD_PREVIEW(library_ctx.dlg, library_expose, library_mouse, NULL, NULL, 200, 200, &library_ctx);
				/* right bottom */
				TODO("rich text label");
				PCB_DAD_LABEL(library_ctx.dlg, "tags...");
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

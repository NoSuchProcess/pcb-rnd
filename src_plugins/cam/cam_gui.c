/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  cam export jobs plugin: GUI dialog
 *  pcb-rnd Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include <genht/htsp.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/hid_dad_tree.h>

static const char *OUTFILE_HELP = "Output file name sample, which will\nbe split to prefix and base name (%base%)\nto be used in file name templates";
static const char *PREFIX_HELP = "File name prefix: every output file\npath will start with this prefix.\nIt is derived from outfile.";

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	cam_ctx_t cam;
	int wjobs, wdigest, wtxt, woutfile, wprefix, wopts;
} cam_dlg_t;

static void cam_gui_jobs2dlg(cam_dlg_t *ctx)
{
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	rnd_hid_row_t *r;
	char *cell[2], *cursor_path = NULL;
	rnd_conf_native_t *cn;

	attr = &ctx->dlg[ctx->wjobs];
	tree = attr->wdata;

	/* remember cursor */
	r = rnd_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = rnd_strdup(r->cell[0]);

	/* remove existing items */
	rnd_dad_tree_clear(tree);

	/* add all new items */
	cn = rnd_conf_get_field("plugins/cam/jobs");
	if (cn != NULL) {
		rnd_conf_listitem_t *item;
		int idx;

		cell[1] = NULL;
		rnd_conf_loop_list(cn->val.list, item, idx) {
			cell[0] = rnd_strdup(item->name);
			rnd_dad_tree_append(attr, NULL, cell);
		}
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		rnd_hid_attr_val_t hv;
		hv.str = cursor_path;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wjobs, &hv);
	}
}

static void cam_gui_digest2dlg(cam_dlg_t *ctx)
{
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	pcb_cam_code_t *c, *plugin = NULL;
	char *cell[4], tmp[1024];
	int n;

	attr = &ctx->dlg[ctx->wdigest];
	tree = attr->wdata;

	/* remove existing items */
	rnd_dad_tree_clear(tree);

	/* add all new items */
	for(n = 0, c = ctx->cam.code.array; n < ctx->cam.code.used; n++,c++) {
		switch(c->inst) {
			case PCB_CAM_DESC:
			case PCB_CAM_PARTIAL:
				break;
			case PCB_CAM_PLUGIN:
				plugin = c;
				break;
			case PCB_CAM_WRITE:
				strncpy(tmp, c->op.write.arg, sizeof(tmp));

				cell[0] = tmp;
				cell[2] = strchr(tmp, '=');
				if (cell[2] != NULL) {
					*cell[2] = '\0';
					(cell[2])++;
				}
				else
					cell[2] = "<none>";

				if (plugin != NULL)
					cell[1] = rnd_strdup(plugin->op.plugin.exporter->name);
				else
					cell[1] = "<NO PLUGIN>";

				cell[3] = NULL;
				rnd_dad_tree_append(attr, NULL, cell);
				break;
		}
	}
}


static void cam_gui_opts2dlg(cam_dlg_t *ctx)
{
	htsp_t *vars = ctx->cam.vars;
	htsp_entry_t *e;
	rnd_hid_attr_val_t hv;
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	rnd_hid_row_t *r;
	char *cell[3], *cursor_path = NULL;

	cam_parse_opt_outfile(&ctx->cam, ctx->dlg[ctx->woutfile].val.str);
	hv.str = ctx->cam.prefix == NULL ? "" : ctx->cam.prefix;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wprefix, &hv);

	attr = &ctx->dlg[ctx->wopts];
	tree = attr->wdata;

	/* remember cursor */
	r = rnd_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = rnd_strdup(r->cell[0]);

	/* remove existing items */
	rnd_dad_tree_clear(tree);

	/* add all new items */
	for(e = htsp_first(vars); e != NULL; e = htsp_next(vars, e)) {
		cell[0] = rnd_strdup(e->key);
		cell[1] = rnd_strdup(e->value);
		cell[2] = NULL;
		rnd_dad_tree_append(attr, NULL, cell);
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		rnd_hid_attr_val_t hv;
		hv.str = cursor_path;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wopts, &hv);
	}
}

static void cam_gui_outfile_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_btn)
{
	cam_dlg_t *ctx = caller_data;
	cam_gui_opts2dlg(ctx);
}

static void cam_gui_filter_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_inp)
{
	cam_dlg_t *ctx = caller_data;
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	const char *text;

	attr = &ctx->dlg[ctx->wjobs];
	tree = attr->wdata;
	text = attr_inp->val.str;

	rnd_dad_tree_hide_all(tree, &tree->rows, 1);
	rnd_dad_tree_unhide_filter(tree, &tree->rows, 0, text);
	rnd_dad_tree_update_hide(attr);
}

static void cam_gui_export_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_btn)
{
	cam_dlg_t *ctx = caller_data;
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wjobs];
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(attr);

	if (row != NULL) {
		const char *outfile = ctx->dlg[ctx->woutfile].val.str;
		char *tmp = pcb_strdup_printf("outfile=%s", outfile);
		rnd_actionva(ctx->cam.hidlib, "cam", "call", row->cell[0], tmp, NULL);
		free(tmp);
	}
}

static char *kill_tabs(const char *str_in)
{
	char *res, *o;
	res = rnd_strdup(str_in);
	for(o = res; *o != '\0'; o++)
		if (*o == '\t')
			*o = ' ';
	return res;
}

static void cam_job_select_cb(rnd_hid_attribute_t *attrib, void *hid_ctx, rnd_hid_row_t *row)
{
	rnd_hid_tree_t *tree = attrib->wdata;
	cam_dlg_t *ctx = tree->user_ctx;

	if (row != NULL) {
		char *script = kill_tabs(cam_find_job(row->cell[0]));
		rnd_hid_attribute_t *atxt = &ctx->dlg[ctx->wtxt];
		rnd_hid_text_t *txt = atxt->wdata;

		txt->hid_set_text(atxt, hid_ctx, RND_HID_TEXT_REPLACE, script);

		cam_free_code(&ctx->cam);
		if (script != NULL)
			cam_compile(&ctx->cam, script);
		cam_gui_digest2dlg(ctx);

		free(script);
	}
}

static void cam_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	cam_dlg_t *ctx = caller_data;
	cam_uninit_inst(&ctx->cam);
	RND_DAD_FREE(ctx->dlg);
	free(ctx);
}

/* center aligned label */
static void header_label(cam_dlg_t *ctx, const char *text)
{
	RND_DAD_BEGIN_HBOX(ctx->dlg);
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
		RND_DAD_END(ctx->dlg);
		RND_DAD_LABEL(ctx->dlg, text);
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
		RND_DAD_END(ctx->dlg);
	RND_DAD_END(ctx->dlg);
}

static int cam_gui(rnd_hidlib_t *hidlib, const char *arg)
{
	cam_dlg_t *ctx = calloc(sizeof(cam_dlg_t), 1);
	const char *opt_hdr[] = {"key", "option value", NULL};
	const char *script_tabs[] = {"digest", "raw", NULL};
	const char *digest_hdr[] = {"file", "plugin", "layer groups", NULL};
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	ctx->cam.hidlib = hidlib;
	ctx->cam.vars = pcb_cam_vars_alloc();

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
		RND_DAD_BEGIN_HPANE(ctx->dlg);

			RND_DAD_BEGIN_VBOX(ctx->dlg); /* left */
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_TREE(ctx->dlg, 1, 0, NULL);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
					RND_DAD_TREE_SET_CB(ctx->dlg, selected_cb, cam_job_select_cb);
					RND_DAD_TREE_SET_CB(ctx->dlg, ctx, ctx);
					ctx->wjobs = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_BEGIN_HBOX(ctx->dlg); /* command section */
					RND_DAD_STRING(ctx->dlg);
						RND_DAD_HELP(ctx->dlg, "Filter text:\nlist jobs with matching name only");
						RND_DAD_CHANGE_CB(ctx->dlg, cam_gui_filter_cb);
					RND_DAD_BEGIN_VBOX(ctx->dlg); /* filler */
						RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
					RND_DAD_END(ctx->dlg);
					RND_DAD_BUTTON(ctx->dlg, "export!");
						RND_DAD_CHANGE_CB(ctx->dlg, cam_gui_export_cb);
						RND_DAD_HELP(ctx->dlg, "Export the current board using the above selected CAM job\nand the options set on the right");
				RND_DAD_END(ctx->dlg);
			RND_DAD_END(ctx->dlg);

			RND_DAD_BEGIN_VBOX(ctx->dlg); /* right */
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_BEGIN_VPANE(ctx->dlg);
					RND_DAD_BEGIN_VBOX(ctx->dlg); /* top */
						RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
						header_label(ctx, "CAM job script");
						RND_DAD_BEGIN_TABBED(ctx->dlg, script_tabs);
							RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);

							RND_DAD_TREE(ctx->dlg, 3, 0, digest_hdr);
								RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
								ctx->wdigest = RND_DAD_CURRENT(ctx->dlg);

							RND_DAD_TEXT(ctx->dlg, ctx);
								RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
								ctx->wtxt = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_END(ctx->dlg);
					RND_DAD_END(ctx->dlg);
					RND_DAD_BEGIN_VBOX(ctx->dlg); /* bottom */
						RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
						header_label(ctx, "CAM options");
						RND_DAD_BEGIN_TABLE(ctx->dlg, 2); /* special options */
							RND_DAD_LABEL(ctx->dlg, "outfile");
								RND_DAD_HELP(ctx->dlg, OUTFILE_HELP);
							RND_DAD_STRING(ctx->dlg);
								RND_DAD_CHANGE_CB(ctx->dlg, cam_gui_outfile_cb);
								ctx->woutfile = RND_DAD_CURRENT(ctx->dlg);
								RND_DAD_HELP(ctx->dlg, OUTFILE_HELP);
							RND_DAD_LABEL(ctx->dlg, "prefix");
							RND_DAD_HELP(ctx->dlg, PREFIX_HELP);
							RND_DAD_LABEL(ctx->dlg, "");
								ctx->wprefix = RND_DAD_CURRENT(ctx->dlg);
								RND_DAD_HELP(ctx->dlg, PREFIX_HELP);
						RND_DAD_END(ctx->dlg);

						RND_DAD_TREE(ctx->dlg, 2, 0, opt_hdr); /* option table */
							RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
							ctx->wopts = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_END(ctx->dlg);
				RND_DAD_END(ctx->dlg);
				RND_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
			RND_DAD_END(ctx->dlg);
		RND_DAD_END(ctx->dlg);
	RND_DAD_END(ctx->dlg);

	RND_DAD_NEW("cam", ctx->dlg, "CAM export", ctx, pcb_false, cam_close_cb);

	{ /* set default outfile */
		rnd_hid_attr_val_t hv;
		hv.str = pcb_derive_default_filename_(PCB->hidlib.filename, "");
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->woutfile, &hv);
		free((char *)hv.str);
		cam_gui_opts2dlg(ctx);
	}

	{ /* set right top text read-only */
		rnd_hid_attribute_t *atxt = &ctx->dlg[ctx->wtxt];
		rnd_hid_text_t *txt = atxt->wdata;
		txt->hid_set_readonly(atxt, ctx->dlg_hid_ctx, 1);
	}

	cam_gui_jobs2dlg(ctx);

	return 0;
}

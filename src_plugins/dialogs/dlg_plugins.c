/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2015,2018 Tibor 'Igor2' Palinkas
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

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
	int wtree;
} plugins_ctx_t;

plugins_ctx_t plugins_ctx;

static void plugins_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	plugins_ctx_t *ctx = caller_data;
	RND_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(plugins_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static int plugin_cmp(const void *v1, const void *v2)
{
	const pup_plugin_t **p1 = (const pup_plugin_t **)v1, **p2 = (const pup_plugin_t **)v2;
	return strcmp((*p1)->name, (*p2)->name);
}

static void plugins2dlg(plugins_ctx_t *ctx)
{
	rnd_hid_attribute_t *attr= &ctx->dlg[ctx->wtree];
	rnd_hid_tree_t *tree = attr->wdata;
	char *cell[4];
	pup_plugin_t *p;
	vtp0_t tmp;
	long n;

	rnd_dad_tree_clear(tree);

	/* sort plugins */
	vtp0_init(&tmp);
	for(p = rnd_pup.plugins; p != NULL; p = p->next)
		vtp0_append(&tmp, p);

	qsort(tmp.array, tmp.used, sizeof(pup_plugin_t *), plugin_cmp);


	/* add all items */
	for(n = 0; n < tmp.used; n++) {
		rnd_hid_row_t *row;

		p = tmp.array[n];
		cell[0] = rnd_strdup(p->name);
		cell[1] = rnd_strdup((p->flags & PUP_FLG_STATIC) ? "buildin" : "plugin");
		cell[2] = rnd_strdup_printf("%d", p->references);
		row = rnd_dad_tree_append(attr, NULL, cell);
		row->user_data = p;
	}

	vtp0_uninit(&tmp);
}

static void unload_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	plugins_ctx_t *ctx = caller_data;
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(&(ctx->dlg[ctx->wtree]));
	pup_plugin_t *p;

	if (row == NULL)
		return;

	p = row->user_data;
	if (p->flags & PUP_FLG_STATIC) {
		rnd_message(RND_MSG_ERROR, "Can not unload '%s', it is builtin (static linked into the executable)\n", p->name);
		return;
	}

	if (p->references > 1) {
		rnd_message(RND_MSG_ERROR, "Can not unload '%s' while other active plugins still depend on it\n", p->name);
		return;
	}

	pup_unload(&rnd_pup, p, NULL);

	/* rebuild the whole tree since we may have unloaded more than one plugins */
	plugins2dlg(ctx);
}

static const char pcb_acts_ManagePlugins[] = "ManagePlugins()\n";
static const char pcb_acth_ManagePlugins[] = "Manage plugins dialog.";
static fgw_error_t pcb_act_ManagePlugins(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	static const char *hdr[] = {"plugin", "type", "refco", NULL};
	plugins_ctx_t *ctx = &plugins_ctx;

	RND_ACT_IRES(0);
	if (ctx->active)
		return 0;

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
		RND_DAD_TREE(ctx->dlg, 3, 0, hdr);
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
/*			RND_DAD_TREE_SET_CB(ctx->dlg, selected_cb, library_select);
			RND_DAD_TREE_SET_CB(ctx->dlg, ctx, &library_ctx);*/
			ctx->wtree = RND_DAD_CURRENT(ctx->dlg);

		RND_DAD_BEGIN_HBOX(ctx->dlg);
			RND_DAD_BUTTON(ctx->dlg, "Unload selected");
			RND_DAD_CHANGE_CB(ctx->dlg, unload_cb);
			RND_DAD_BEGIN_VBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
			RND_DAD_END(ctx->dlg);
			RND_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
		RND_DAD_END(ctx->dlg);
	RND_DAD_END(ctx->dlg);

	RND_DAD_DEFSIZE(ctx->dlg, 300, 400);

	plugins2dlg(ctx);

	RND_DAD_NEW("plugins", ctx->dlg, "Manage plugins", ctx, rnd_false, plugins_close_cb);

	ctx->active = 1;
	return 0;
}

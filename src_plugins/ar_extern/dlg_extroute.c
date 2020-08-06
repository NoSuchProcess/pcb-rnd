/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  auto routing with external router process: dialog box
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

#include <librnd/core/hid_dad.h>

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
	vts0_t tabs;
	int whatever;
} ar_ctx_t;

static ar_ctx_t ar_ctx;

static void ar_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	long n;
	ar_ctx_t *ctx = caller_data;
	RND_DAD_FREE(ctx->dlg);
	vts0_uninit(&ctx->tabs);
	memset(ctx, 0, sizeof(ar_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void extroute_gui(pcb_board_t *pcb)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	long an, mn;

	if (ar_ctx.active)
		return; /* do not open another */

	extroute_query_conf(pcb);

	for(an = 0; an < router_apis.used; an++) {
		router_api_t *a = router_apis.array[an];
		for(mn = 0; mn < a->num_methods; mn++)
			vts0_append(&ar_ctx.tabs, a->methods[mn].name);
	}

	RND_DAD_BEGIN_VBOX(ar_ctx.dlg);
		RND_DAD_BEGIN_TABBED(ar_ctx.dlg, ar_ctx.tabs.array);
			RND_DAD_COMPFLAG(ar_ctx.dlg, RND_HATF_LEFT_TAB|RND_HATF_EXPFILL);
			for(an = 0; an < router_apis.used; an++) {
				router_api_t *a = router_apis.array[an];
				for(mn = 0; mn < a->num_methods; mn++) {
					router_method_t *m = &a->methods[mn];
					char *title;
					int *wid;
					rnd_hid_attr_val_t *val;
					rnd_export_opt_t *cfg;

					RND_DAD_BEGIN_VBOX(ar_ctx.dlg);
						if (m->len > 10)
							RND_DAD_COMPFLAG(ar_ctx.dlg, RND_HATF_SCROLL);

						title = rnd_concat(m->router->name, " / ", m->name, NULL);
						RND_DAD_LABEL(ar_ctx.dlg, title);
						RND_DAD_LABEL(ar_ctx.dlg, m->desc);
						free(title);

						for(cfg = m->confkeys, wid = m->w, val = m->val; cfg->name != NULL; cfg++, wid++, val++) {
							RND_DAD_BEGIN_HBOX(ar_ctx.dlg);
							switch(cfg->type) {
								case RND_HATT_BOOL:
									RND_DAD_BOOL(ar_ctx.dlg);
									RND_DAD_DEFAULT_NUM(ar_ctx.dlg, val->lng);
									break;
								case RND_HATT_INTEGER:
									RND_DAD_INTEGER(ar_ctx.dlg);
									RND_DAD_MINMAX(ar_ctx.dlg, cfg->min_val, cfg->max_val);
									RND_DAD_DEFAULT_NUM(ar_ctx.dlg, val->lng);
									break;
								case RND_HATT_REAL:
									RND_DAD_REAL(ar_ctx.dlg);
									RND_DAD_MINMAX(ar_ctx.dlg, cfg->min_val, cfg->max_val);
									RND_DAD_DEFAULT_NUM(ar_ctx.dlg, val->dbl);
									break;
								case RND_HATT_COORD:
									RND_DAD_COORD(ar_ctx.dlg);
									RND_DAD_MINMAX(ar_ctx.dlg, cfg->min_val, cfg->max_val);
									RND_DAD_DEFAULT_NUM(ar_ctx.dlg, val->crd);
									break;
								case RND_HATT_STRING:
									RND_DAD_STRING(ar_ctx.dlg);
									RND_DAD_DEFAULT_PTR(ar_ctx.dlg, val->str);
									break;
								default:
									break;
						}
						*wid = RND_DAD_CURRENT(ar_ctx.dlg);
						RND_DAD_LABEL(ar_ctx.dlg, cfg->name);
						RND_DAD_END(ar_ctx.dlg);
					}
					RND_DAD_END(ar_ctx.dlg);
				}
			}
		RND_DAD_END(ar_ctx.dlg);

		RND_DAD_BEGIN_HBOX(ar_ctx.dlg);
			RND_DAD_BUTTON(ar_ctx.dlg, "Route");
			RND_DAD_BUTTON(ar_ctx.dlg, "Re-route");
			RND_DAD_BUTTON(ar_ctx.dlg, "Save");
			RND_DAD_BUTTON(ar_ctx.dlg, "Load");
			RND_DAD_BEGIN_HBOX(ar_ctx.dlg);
				RND_DAD_COMPFLAG(ar_ctx.dlg, RND_HATF_EXPFILL);
				RND_DAD_PROGRESS(ar_ctx.dlg);
			RND_DAD_END(ar_ctx.dlg);
			RND_DAD_BUTTON_CLOSES(ar_ctx.dlg, clbtn);
		RND_DAD_END(ar_ctx.dlg);
	RND_DAD_END(ar_ctx.dlg);

	/* set up the context */
	ar_ctx.active = 1;

	RND_DAD_NEW("external_autorouter", ar_ctx.dlg, "External autorouter", &ar_ctx, rnd_false, ar_close_cb);
}

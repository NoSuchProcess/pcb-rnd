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

#include <librnd/core/safe_fs.h>
#include <librnd/core/hid_dad.h>

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
	vts0_t tabs;
	int wtabs;
} ar_ctx_t;

static ar_ctx_t ar_ctx;

/* copy values from the dialog box to the memory representation (router_apis) */
static void dlg2mem(void)
{
	int an, mn;

	if (!ar_ctx.active)
		return;

	for(an = 0; an < router_apis.used; an++) {
		router_api_t *a = router_apis.array[an];
		for(mn = 0; mn < a->num_methods; mn++) {
			router_method_t *m = &a->methods[mn];
			int *wid;
			rnd_hid_attr_val_t *val;
			rnd_export_opt_t *cfg;

			for(cfg = m->confkeys, wid = m->w, val = m->val; cfg->name != NULL; cfg++, wid++, val++)
				*val = ar_ctx.dlg[*wid].val;
		}
	}
}

static void save_conf(FILE *f)
{
	int an, mn;

	dlg2mem();
	fprintf(f, "ha:autorouter-settings-v1 {\n");
	fprintf(f, " ha:confkeys {\n");

	for(an = 0; an < router_apis.used; an++) {
		router_api_t *a = router_apis.array[an];
		fprintf(f, "  ha:%s {\n", a->router->name);
		for(mn = 0; mn < a->num_methods; mn++) {
			router_method_t *m = &a->methods[mn];
			int *wid;
			rnd_hid_attr_val_t *val;
			rnd_export_opt_t *cfg;

			fprintf(f, "   ha:%s {\n", m->name);
			for(cfg = m->confkeys, wid = m->w, val = m->val; cfg->name != NULL; cfg++, wid++, val++) {
				switch(cfg->type) {
					case RND_HATT_BOOL:
					case RND_HATT_INTEGER:
						fprintf(f, "    %s=%ld\n", cfg->name, val->lng);
						break;
					case RND_HATT_REAL:
						fprintf(f, "    %s=%f\n", cfg->name, val->dbl);
						break;
					case RND_HATT_COORD:
						rnd_fprintf(f, "    %s=%$mH", cfg->name, val->crd);
						fprintf(f, "\n");
						break;
					case RND_HATT_STRING:
						fprintf(f, "    %s={%s}\n", cfg->name, val->str);
						break;
					default: break;
				}
			}
			fprintf(f, "   }\n");
		}
		fprintf(f, "  }\n");
	}
	fprintf(f, " }\n");
	fprintf(f, "}\n");
}

static void load_conf(FILE *f, const char *fn)
{
	char *errmsg = NULL;
	int an, mn;
	lht_doc_t *doc = lht_dom_load(fn, &errmsg);
	lht_node_t *croot, *nrouter, *nmethod, *n;
	
	if (doc == NULL) {
		rnd_message(RND_MSG_ERROR, "Failed to parse autorouter settings: %s\n", errmsg);
		free(errmsg);
		return;
	}
	free(errmsg);

	if ((doc->root == NULL) || (doc->root->type != LHT_HASH) || (strcmp(doc->root->name, "autorouter-settings-v1") != 0)) {
		rnd_message(RND_MSG_ERROR, "Failed to load autorouter settings from %s: invalid root node (expected ha:autorouter-settings-v1)\n", fn);
		return;
	}

	croot = lht_dom_hash_get(doc->root, "confkeys");
	if ((croot == NULL) || (croot->type != LHT_HASH)) {
		rnd_message(RND_MSG_ERROR, "Failed to load autorouter settings from %s: no confkeys under\n", fn);
		return;
	}


	for(an = 0; an < router_apis.used; an++) {
		router_api_t *a = router_apis.array[an];
		nrouter = lht_dom_hash_get(croot, a->router->name);
		if ((nrouter == NULL) || (nrouter->type != LHT_HASH))
			continue;
		for(mn = 0; mn < a->num_methods; mn++) {
			router_method_t *m = &a->methods[mn];
			int *wid;
			rnd_hid_attr_val_t *val;
			rnd_export_opt_t *cfg;

			nmethod = lht_dom_hash_get(nrouter, m->name);
			if ((nmethod == NULL) || (nmethod->type != LHT_HASH))
				continue;

			for(cfg = m->confkeys, wid = m->w, val = m->val; cfg->name != NULL; cfg++, wid++, val++) {
				char *end;
				long l;
				double d;
				rnd_bool succ;

				n = lht_dom_hash_get(nmethod, cfg->name);
				if ((n == NULL) || (n->type != LHT_TEXT))
					continue;

				switch(cfg->type) {
					case RND_HATT_BOOL:
					case RND_HATT_INTEGER:
						l = strtol(n->data.text.value, &end, 10);
						if (*end == '\0')
							val->lng = l;
						break;
					case RND_HATT_REAL:
						d = strtod(n->data.text.value, &end);
						if (*end == '\0')
							val->dbl = d;
						break;
					case RND_HATT_COORD:
						d = rnd_get_value(n->data.text.value, NULL, NULL, &succ);
						if (succ)
							val->crd = d;
						break;
					case RND_HATT_STRING:
						free((char *)val->str);
						val->str = n->data.text.value;
						n->data.text.value = NULL; /* took over ownership */
						break;
					default: break;
				}
				rnd_gui->attr_dlg_set_value(ar_ctx.dlg_hid_ctx, *wid, val);
			}
		}
	}

	lht_dom_uninit(doc);
}

static void save_conf_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	FILE *f;
	char *fname;
	rnd_hidlib_t *hl = rnd_gui->get_dad_hidlib(hid_ctx);

	fname = rnd_gui->fileselect(rnd_gui, "Save autoroute settings to...",
		"Pick a file for saving autoroute settings to.\n",
		"autoroute.cfg.lht", ".lht", NULL, "ar_extern", RND_HID_FSD_MAY_NOT_EXIST, NULL);

	if (fname == NULL)
		return;

	f = rnd_fopen(hl, fname, "w");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "Failed to open '%s' for write\n", fname);
		return;
	}

	save_conf(f);
	fclose(f);
	free(fname);
}

static void load_conf_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	FILE *f;
	char *fname;
	rnd_hidlib_t *hl = rnd_gui->get_dad_hidlib(hid_ctx);

	fname = rnd_gui->fileselect(rnd_gui, "Load autoroute settings from...",
		"Pick a file for loading autoroute settings from.\n",
		"autoroute.cfg.lht", ".lht", NULL, "ar_extern", 0, NULL);

	if (fname == NULL)
		return;

	f = rnd_fopen(hl, fname, "r");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "Failed to open '%s' for read\n", fname);
		return;
	}

	load_conf(f, fname);
	fclose(f);
	free(fname);
}

static int val_eq(rnd_hid_attr_type_t type, const rnd_hid_attr_val_t *v1, const rnd_hid_attr_val_t *v2)
{
	switch(type) {
		case RND_HATT_BOOL:
		case RND_HATT_INTEGER: return v1->lng == v2->lng;
		case RND_HATT_REAL:    return v1->dbl == v2->dbl;
		case RND_HATT_COORD:   return v1->crd == v2->crd;
		case RND_HATT_STRING:  return strcmp(v1->str, v2->str) == 0;
		default:               rnd_message(RND_MSG_ERROR, "external router: internal error: unhandled val type - report this bug!\n");
	}
	return 0; /* better stay with the default */
}

static void route_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	int  argc, an, mn, cnt = 0, target = ar_ctx.dlg[ar_ctx.wtabs].val.lng;
	rnd_export_opt_t *cfg;
	router_method_t *m;
	rnd_hid_attr_val_t *val;
	router_api_t *a;
	fgw_arg_t *argv;

	/* count with cnt until reaching target; mn is then the method */
	for(an = 0; an < router_apis.used; an++) {
		a = router_apis.array[an];
		for(mn = 0; mn < a->num_methods; mn++)
			if (cnt++ == target)
				goto out;
	}

	rnd_message(RND_MSG_ERROR, "external router: can't find the router for this tab (%d/%d; internal error)\n", cnt, target);

	out:;

	if (a->router->route == NULL) {
		rnd_message(RND_MSG_ERROR, "external router: internal error: this router does not implement the ->route call\n");
		return;
	}

	dlg2mem();
	m = &a->methods[mn];
	argc = 0;
	argv = calloc(sizeof(fgw_arg_t), m->len);
	for(cfg = m->confkeys, val = m->val; cfg->name != NULL; cfg++, val++) {
		char *s = NULL;

		if (val_eq(cfg->type, val, &cfg->default_val))
			continue;

		switch(cfg->type) {
			case RND_HATT_BOOL:
			case RND_HATT_INTEGER:
				s = rnd_strdup_printf("%s=%d", cfg->name, val->lng);
				break;
			case RND_HATT_REAL:
				s = rnd_strdup_printf("%s=%f", cfg->name, val->dbl);
				break;
			case RND_HATT_COORD:
				s = rnd_strdup_printf("%s=%.06mm", cfg->name, val->crd);
				break;
			case RND_HATT_STRING:
				s = rnd_concat(cfg->name, "=", val->str, NULL);
				break;
			default: break;
		}
		argv[argc].type = FGW_STR; argv[argc].val.str = s;
		argc++;
	}

	a->router->route(PCB, ERSC_BOARD, m->name, argc, argv);

	fgw_argv_free(&rnd_fgw, argc, argv);
	free(argv);
}

static void reroute_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	TODO("remove all auto-routed objects");
	route_cb(hid_ctx, caller_data, attr);
}



static void ar_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	ar_ctx_t *ctx = caller_data;
	dlg2mem();
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
			ar_ctx.wtabs = RND_DAD_CURRENT(ar_ctx.dlg);
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
				RND_DAD_CHANGE_CB(ar_ctx.dlg, route_cb);
			RND_DAD_BUTTON(ar_ctx.dlg, "Re-route");
				RND_DAD_CHANGE_CB(ar_ctx.dlg, reroute_cb);
			RND_DAD_BUTTON(ar_ctx.dlg, "Save");
				RND_DAD_CHANGE_CB(ar_ctx.dlg, save_conf_cb);
			RND_DAD_BUTTON(ar_ctx.dlg, "Load");
				RND_DAD_CHANGE_CB(ar_ctx.dlg, load_conf_cb);
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

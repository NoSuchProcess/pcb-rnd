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
#include "safe_fs.h"

static const char *place_cookie = "dialogs/place";

typedef struct {
	int x, y, w, h;
} wingeo_t;

wingeo_t wingeo_invalid = {0, 0, 0, 0};

typedef const char *htsw_key_t;
typedef wingeo_t htsw_value_t;
#define HT(x) htsw_ ## x
#define HT_INVALID_VALUE wingeo_invalid;
#include <genht/ht.h>
#include <genht/ht.c>
#undef HT
#include <genht/hash.h>

static htsw_t wingeo;

static void pcb_dialog_store(const char *id, int x, int y, int w, int h)
{
	htsw_entry_t *e;
	wingeo_t wg;

/*	pcb_trace("dialog place set: '%s' %d;%d  %d*%d\n", id, x, y, w, h);*/

	e = htsw_getentry(&wingeo, (char *)id);
	if (e != NULL) {
		e->value.x = x;
		e->value.y = y;
		e->value.w = w;
		e->value.h = h;
		return;
	}

	wg.x = x;
	wg.y = y;
	wg.w = w;
	wg.h = h;
	htsw_set(&wingeo, pcb_strdup(id), wg);
}


static void pcb_dialog_place(void *user_data, int argc, pcb_event_arg_t argv[])
{
	const char *id;
	int *geo;
	htsw_entry_t *e;

	if ((argc < 3) || (argv[1].type != PCB_EVARG_PTR) || (argv[2].type != PCB_EVARG_STR))
		return;

	id = argv[2].d.s;
	geo = argv[3].d.p;

	e = htsw_getentry(&wingeo, (char *)id);
	if (e != NULL) {
		geo[0] = e->value.x;
		geo[1] = e->value.y;
		geo[2] = e->value.w;
		geo[3] = e->value.h;
	}
/*	pcb_trace("dialog place: %p '%s'\n", hid_ctx, id);*/
}

static void pcb_dialog_resize(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((argc < 7) || (argv[1].type != PCB_EVARG_PTR) || (argv[2].type != PCB_EVARG_STR))
		return;

/*	hid_ctx = argv[1].d.p;*/
	pcb_dialog_store(argv[2].d.s, argv[3].d.i, argv[4].d.i, argv[5].d.i, argv[6].d.i);
}

static vtp0_t cleanup_later;
static char *str_cleanup_later(const char *path)
{
	char *s = pcb_strdup(path);
	vtp0_append(&cleanup_later, s);
	return s;
}

static void place_conf_set(conf_role_t role, const char *path, int val)
{
	static int dummy;

	if (conf_get_field(path) == NULL)
		conf_reg_field_(&dummy, 1, CFN_INTEGER, str_cleanup_later(path), "", 0);
	conf_setf(role, path, -1, "%d", val);
}

static void place_conf_load(conf_role_t role, const char *path, int *val)
{
	conf_native_t *nat = conf_get_field(path);
	conf_role_t crole;
	static int dummy;

	if (conf_get_field(path) == NULL) {
		conf_reg_field_(&dummy, 1, CFN_INTEGER, str_cleanup_later(path), "", 0);
		conf_update(path, -1);
	}

	nat = conf_get_field(path);
	if ((nat == NULL) || (nat->prop->src == NULL) || (nat->prop->src->type != LHT_TEXT)) {
		pcb_message(PCB_MSG_ERROR, "Can not load window geometry from invalid node for %s\n", path);
		return;
	}

	/* there are priorities which is hanled by conf merging. To make sure
	   only the final value is loaded, check if the final native's source
	   role matches the role that's being loaded. Else the currently loading
	   role is lower prio and didn't contribute to the final values and should
	   be ignored. */
	crole = conf_lookup_role(nat->prop->src);
	if (crole != role)
		return;

	/* need to atoi() directly from the lihata node because the native value
	   is dummy and shared among all nodes - cheaper to atoi than to do
	   a proper allocation for the native value. */
	*val = atoi(nat->prop->src->data.text.value);
}

#define BASEPATH "plugins/dialogs/window_geometry/"
void pcb_wplc_load(conf_role_t role)
{
	char *end, *end2, path[128 + sizeof(BASEPATH)];
	lht_node_t *nd, *root;
	lht_dom_iterator_t it;
	int x, y, w, h;

	strcpy(path, BASEPATH);
	end = path + strlen(BASEPATH);

	root = conf_lht_get_at(role, path, 0);
	if (root == NULL)
		return;

	for(nd = lht_dom_first(&it, root); nd != NULL; nd = lht_dom_next(&it)) {
		int len;
		if (nd->type != LHT_HASH)
			continue;
		len = strlen(nd->name);
		if (len > 64)
			continue;
		memcpy(end, nd->name, len);
		end[len] = '/';
		end2 = end + len+1;

		x = y = -1;
		w = h = 0;
		strcpy(end2, "x"); place_conf_load(role, path, &x);
		strcpy(end2, "y"); place_conf_load(role, path, &y);
		strcpy(end2, "width"); place_conf_load(role, path, &w);
		strcpy(end2, "height"); place_conf_load(role, path, &h);
		pcb_dialog_store(nd->name, x, y, w, h);
	}
}


static void place_maybe_save(conf_role_t role, int force)
{
	htsw_entry_t *e;
	char path[128 + sizeof(BASEPATH)];
	char *end, *end2;

	switch(role) {
		case CFR_USER:    if (!force && !conf_dialogs.plugins.dialogs.auto_save_window_geometry.to_user) return; break;
		case CFR_DESIGN:  if (!force && !conf_dialogs.plugins.dialogs.auto_save_window_geometry.to_design) return; break;
		case CFR_PROJECT: if (!force && !conf_dialogs.plugins.dialogs.auto_save_window_geometry.to_project) return; break;
		default: return;
	}

	strcpy(path, BASEPATH);
	end = path + strlen(BASEPATH);
	for(e = htsw_first(&wingeo); e != NULL; e = htsw_next(&wingeo, e)) {
		int len = strlen(e->key);
		if (len > 64)
			continue;
		memcpy(end, e->key, len);
		end[len] = '/';
		end2 = end + len+1;

		strcpy(end2, "x"); place_conf_set(role, path, e->value.x);
		strcpy(end2, "y"); place_conf_set(role, path, e->value.y);
		strcpy(end2, "width"); place_conf_set(role, path, e->value.w);
		strcpy(end2, "height"); place_conf_set(role, path, e->value.h);
	}


	if (role != CFR_DESIGN) {
		int r = conf_save_file(NULL, (PCB == NULL ? NULL : PCB->Filename), role, NULL);
		if (r != 0)
			pcb_message(PCB_MSG_ERROR, "Failed to save window geometry in %s\n", conf_role_name(role));
	}
}

/* event handlers that run before the current pcb is saved to save win geo
   in the board conf and after loading a new board to fetch window placement
   info. */
static void place_save_pre(void *user_data, int argc, pcb_event_arg_t argv[])
{
	place_maybe_save(CFR_PROJECT, 0);
	place_maybe_save(CFR_DESIGN, 0);
}

static void place_load_post(void *user_data, int argc, pcb_event_arg_t argv[])
{
	pcb_wplc_load(CFR_PROJECT);
	pcb_wplc_load(CFR_DESIGN);
}

void pcb_wplc_save_to_role(conf_role_t role)
{
	place_maybe_save(role, 1);
}

int pcb_wplc_save_to_file(const char *fn)
{
	htsw_entry_t *e;
	FILE *f;

	f = pcb_fopen(fn, "w");
	if (f == NULL)
		return -1;

	fprintf(f, "li:pcb-rnd-conf-v1 {\n");
	fprintf(f, " ha:overwrite {\n");
	fprintf(f, "  ha:plugins {\n");
	fprintf(f, "   ha:dialogs {\n");
	fprintf(f, "    ha:window_geometry {\n");


	for(e = htsw_first(&wingeo); e != NULL; e = htsw_next(&wingeo, e)) {
		fprintf(f, "     ha:%s {\n", e->key);
		fprintf(f, "      x=%d\n", e->value.x);
		fprintf(f, "      y=%d\n", e->value.x);
		fprintf(f, "      width=%d\n", e->value.w);
		fprintf(f, "      height=%d\n", e->value.h);
		fprintf(f, "     }\n");
	}

	fprintf(f, "    }\n");
	fprintf(f, "   }\n");
	fprintf(f, "  }\n");
	fprintf(f, " }\n");
	fprintf(f, "}\n");
	fclose(f);
	return 0;
}

static void pcb_dialog_place_uninit(void)
{
	htsw_entry_t *e;
	int n;

	place_maybe_save(CFR_USER, 0);

	for(e = htsw_first(&wingeo); e != NULL; e = htsw_next(&wingeo, e))
		free((char *)e->key);
	htsw_uninit(&wingeo);
	pcb_event_unbind_allcookie(place_cookie);

	for(n = 0; n < cleanup_later.used; n++)
		free(cleanup_later.array[n]);
}

static void pcb_dialog_place_init(void)
{
	htsw_init(&wingeo, strhash, strkeyeq);
	pcb_event_bind(PCB_EVENT_SAVE_PRE, place_save_pre, NULL, place_cookie);
	pcb_event_bind(PCB_EVENT_LOAD_POST, place_load_post, NULL, place_cookie);
	pcb_wplc_load(CFR_SYSTEM);
	pcb_wplc_load(CFR_USER);
}

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

#include "board.h"
#include "data.h"
#include "plugins.h"
#include "pcb-printf.h"

#include "../src_plugins/propedit/props.h"
#include "../src_plugins/propedit/propsel.h"

#include "lib_vfs.h"

static void vfs_list_props(gds_t *path, pcb_propedit_t *pctx, pcb_vfs_list_cb cb, void *ctx)
{
	htsp_entry_t *e;
	size_t ou, orig_used;

	pcb_propsel_map_core(pctx);

	orig_used = path->used;
	gds_append(path, '/');
	ou = path->used;
	for(e = htsp_first(&pctx->props); e != NULL; e = htsp_next(&pctx->props, e)) {
		path->used = ou;
		gds_append_str(path, e->key);
		cb(ctx, path->array, 0);
	}
	path->used = orig_used;
}

static int vfs_access_prop(pcb_propedit_t *pctx, const char *path, gds_t *data, int wr, int *isdir)
{
	pcb_props_t *pt;
	pcb_propval_t *pv;
	htprop_entry_t *e;

	if (*path == '\0') {
		if (isdir != NULL)
			*isdir = 1;
		if (wr == 0)
			return 0;
		return -1;
	}

	pcb_propsel_map_core(pctx);
	pt = htsp_get(&pctx->props, path);

	if (pt == NULL)
		return -1;

	if (wr) {
		pcb_propset_ctx_t sctx;
		sctx.s = data->array;
		TODO("convert the other fields as well");
		return pcb_propsel_set(pctx, path, &sctx) == 1;
	}


	e = htprop_first(&pt->values);
	pv = &e->key;

	if (data != NULL) {
		free(data->array);
		data->array = pcb_propsel_printval(pt->type, pv);
		data->used = data->alloced = strlen(data->array);
		gds_append(data, '\n');
	}

	return 0;
}

static void vfs_list_obj(pcb_board_t *pcb, gds_t *path, pcb_any_obj_t *obj, pcb_vfs_list_cb cb, void *ctx)
{
	pcb_propedit_t pctx;
	pcb_idpath_t *idp;
	htsp_entry_t *e;
	size_t ou, orig_used;

	idp = pcb_obj2idpath(obj);
	if (idp == NULL)
		return;

	pcb_props_init(&pctx, pcb);
	pcb_idpath_list_append(&pctx.objs, idp);
	pcb_propsel_map_core(&pctx);

	orig_used = path->used;
	pcb_append_printf(path, "/%s/%ld/", pcb_obj_type_name(obj->type), obj->ID);
	ou = path->used;
	for(e = htsp_first(&pctx.props); e != NULL; e = htsp_next(&pctx.props, e)) {
		path->used = ou;
		gds_append_str(path, e->key);
		cb(ctx, path->array, 0);
	}
	path->used = orig_used;

	pcb_props_uninit(&pctx);
	pcb_idpath_destroy(idp);
}

static int vfs_access_obj(pcb_board_t *pcb, pcb_any_obj_t *obj, const char *path, gds_t *data, int wr, int *isdir)
{
	pcb_propedit_t pctx;
	pcb_idpath_t *idp;
	int res;

	idp = pcb_obj2idpath(obj);
	if (idp == NULL)
		return -1;

	if (*path == '\0') {
		if (isdir != NULL)
			*isdir = 1;
		if (wr == 0)
			return 0;
		return -1;
	}

	pcb_props_init(&pctx, pcb);
	pcb_idpath_list_append(&pctx.objs, idp);
	res = vfs_access_prop(&pctx, path, data, wr, isdir);
	pcb_props_uninit(&pctx);
	pcb_idpath_destroy(idp);

	return res;
}

static void vfs_list_layers(pcb_board_t *pcb, pcb_vfs_list_cb cb, void *ctx)
{
	gds_t path;
	pcb_layer_id_t lid;
	size_t orig_used;

	cb(ctx, "data/layers", 1);
	gds_init(&path);
	gds_append_str(&path, "data/layers/");
	orig_used = path.used;

	for(lid = 0; lid < pcb->Data->LayerN; lid++) {
		pcb_propedit_t pctx;

		path.used = orig_used;
		pcb_append_printf(&path, "%ld", lid);
		cb(ctx, path.array, 1);

		{
			int ou = path.used;
			gds_append_str(&path, "/p"); cb(ctx, path.array, 0); path.used = ou;
			gds_append_str(&path, "/a"); cb(ctx, path.array, 0); path.used = ou;
			gds_append_str(&path, "/line"); cb(ctx, path.array, 0); path.used = ou;
			gds_append_str(&path, "/poly"); cb(ctx, path.array, 0); path.used = ou;
			gds_append_str(&path, "/text"); cb(ctx, path.array, 0); path.used = ou;
			gds_append_str(&path, "/arc"); cb(ctx, path.array, 0); path.used = ou;
		}

		pcb_props_init(&pctx, PCB);
		vtl0_append(&pctx.layers, lid);
		vfs_list_props(&path, &pctx, cb, ctx);
		pcb_props_uninit(&pctx);

		{
			pcb_layer_t *layer = pcb_get_layer(pcb->Data, lid);
			pcb_line_t *l;
			pcb_arc_t *a;
			pcb_poly_t *p;
			pcb_text_t *t;
			gdl_iterator_t it;

			linelist_foreach(&layer->Line, &it, l)
				vfs_list_obj(pcb, &path, (pcb_any_obj_t *)l, cb, ctx);
			polylist_foreach(&layer->Polygon, &it, p)
				vfs_list_obj(pcb, &path, (pcb_any_obj_t *)p, cb, ctx);
			textlist_foreach(&layer->Text, &it, t)
				vfs_list_obj(pcb, &path, (pcb_any_obj_t *)t, cb, ctx);
			arclist_foreach(&layer->Arc, &it, a)
				vfs_list_obj(pcb, &path, (pcb_any_obj_t *)a, cb, ctx);
		}
	}
	gds_uninit(&path);
}

static int vfs_access_layer(pcb_board_t *pcb, const char *path, gds_t *data, int wr, int *isdir)
{
	pcb_propedit_t pctx;
	char *end;
	pcb_layer_id_t lid = strtol(path, &end, 10);
	int res;

	if (*end == '\0') {
		pcb_layer_t *ly = pcb_get_layer(pcb->Data, lid);
		if (ly == NULL)
			return -1;
		goto ret_dir;
	}

	if (*end != '/')
		return -1;
	path=end+1;

	if (path[1] == '/') { /* direct layer access */
		pcb_props_init(&pctx, pcb);
		vtl0_append(&pctx.layers, lid);
		res = vfs_access_prop(&pctx, path, data, wr, isdir);
		pcb_props_uninit(&pctx);
	}
	else {
		char *sep = strchr(path, '/');
		long oid;
		pcb_any_obj_t *obj = NULL;
		pcb_objtype_t ty;

		if (sep == NULL)
			goto ret_dir;

		sep = strtol(sep+1, &end, 10);

		if ((*end != '/') && (*end != '\0'))
			return -1;

		if ((strncmp(path, "line/", 5) == 0) || (strcmp(path, "line") == 0))
			ty = PCB_OBJ_LINE;
		else if ((strncmp(path, "poly/", 5) == 0) || (strcmp(path, "poly") == 0))
			ty = PCB_OBJ_POLY;
		else if ((strncmp(path, "text/", 5) == 0) || (strcmp(path, "text") == 0))
			ty = PCB_OBJ_TEXT;
		else if ((strncmp(path, "arc/", 4) == 0) || (strcmp(path, "arc") == 0))
			ty = PCB_OBJ_ARC;
		else
			return -1;

		if (*end == '\0')
			path = end;
		else
			path=end+1;
		obj = htip_get(&pcb->Data->id2obj, oid);
		if ((obj == NULL) || (obj->type != ty))
			return -1;
		if ((obj->parent_type != PCB_PARENT_LAYER) || (obj->parent.layer != pcb_get_layer(pcb->Data, lid)))
			return -1;
		res = vfs_access_obj(pcb, obj, path, data, wr, isdir);
	}

	return res;

	ret_dir:;
	if (isdir != NULL)
		*isdir = 1;
	return 0;
}

static void vfs_list_layergrps(pcb_board_t *pcb, pcb_vfs_list_cb cb, void *ctx)
{
	gds_t path;
	pcb_layergrp_id_t gid;
	size_t orig_used;

	cb(ctx, "layer_groups", 1);
	gds_init(&path);
	gds_append_str(&path, "layer_groups/");
	orig_used = path.used;

	for(gid = 0; gid < pcb->LayerGroups.len; gid++) {
		pcb_propedit_t pctx;

		path.used = orig_used;
		pcb_append_printf(&path, "%ld", gid);
		cb(ctx, path.array, 1);

		pcb_props_init(&pctx, PCB);
		vtl0_append(&pctx.layergrps, gid);
		vfs_list_props(&path, &pctx, cb, ctx);
		pcb_props_uninit(&pctx);
	}
	gds_uninit(&path);
}

static int vfs_access_layergrp(pcb_board_t *pcb, const char *path, gds_t *data, int wr, int *isdir)
{
	pcb_propedit_t pctx;
	char *end;
	pcb_layergrp_id_t gid = strtol(path, &end, 10);
	int res;

	if ((*end != '/') && (*end != '\0'))
		return -1;

	if (*end == '\0')
		path = end;
	else
		path=end+1;

	pcb_props_init(&pctx, pcb);
	vtl0_append(&pctx.layergrps, gid);
	res = vfs_access_prop(&pctx, path, data, wr, isdir);
	pcb_props_uninit(&pctx);

	return res;
}

int pcb_vfs_list(pcb_board_t *pcb, pcb_vfs_list_cb cb, void *ctx)
{
	vfs_list_layergrps(pcb, cb, ctx);
	cb(ctx, "data", 1);
	vfs_list_layers(pcb, cb, ctx);
	cb(ctx, "route_styles", 1);
	cb(ctx, "netlist", 1);

	return 0;
}



int pcb_vfs_access(pcb_board_t *pcb, const char *path, gds_t *data, int wr, int *isdir)
{
	if ((strcmp(path, "data") == 0) || (strcmp(path, "data/layers") == 0)) {
		if (isdir != NULL)
			*isdir = 1;
		if (!wr)
			return 0;
	}
	if (strncmp(path, "data/layers/", 12) == 0)
		return vfs_access_layer(pcb, path+12, data, wr, isdir);

	if (strcmp(path, "layer_groups") == 0) {
		if (isdir != NULL)
			*isdir = 1;
		if (!wr)
			return 0;
	}
	if (strncmp(path, "layer_groups/", 13) == 0)
		return vfs_access_layergrp(pcb, path+13, data, wr, isdir);

	return -1;
}


int pplg_check_ver_lib_vfs(int ver_needed) { return 0; }

void pplg_uninit_lib_vfs(void)
{
}

int pplg_init_lib_vfs(void)
{
	PCB_API_CHK_VER;
	return 0;
}

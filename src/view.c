/*
 *
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */
#include "config.h"

#include "idpath.h"
#include <genlist/gentdlist_undef.h>

#define TDL_DONT_UNDEF
#include "view.h"
#include <genlist/gentdlist_impl.c>
#undef TDL_DONT_UNDEF
#include <genlist/gentdlist_undef.h>

#include <liblihata/dom.h>

#include <assert.h>

#include "actions.h"
#include "compat_misc.h"
#include "error.h"
#include "pcb-printf.h"

static unsigned long int pcb_view_next_uid = 0;

void pcb_view_free(pcb_view_t *item)
{
	pcb_view_list_remove(item);
	pcb_idpath_list_clear(&item->objs[0]);
	pcb_idpath_list_clear(&item->objs[1]);
	free(item->title);
	free(item->description);
	free(item);
}

void pcb_view_list_free_fields(pcb_view_list_t *lst)
{
	for(;;) {
		pcb_view_t *item = pcb_view_list_first(lst);
		if (item == NULL)
			break;
		pcb_view_free(item);
	}
}

void pcb_view_list_free(pcb_view_list_t *lst)
{
	pcb_view_list_free_fields(lst);
	free(lst);
}

pcb_view_t *pcb_view_by_uid(const pcb_view_list_t *lst, unsigned long int uid)
{
	pcb_view_t *v;

	for(v = pcb_view_list_first((pcb_view_list_t *)lst); v != NULL; v = pcb_view_list_next(v))
		if (v->uid == uid)
			return v;

	return NULL;
}

pcb_view_t *pcb_view_by_uid_cnt(const pcb_view_list_t *lst, unsigned long int uid, long *cnt)
{
	pcb_view_t *v;
	long c = 0;

	for(v = pcb_view_list_first((pcb_view_list_t *)lst); v != NULL; v = pcb_view_list_next(v), c++) {
		if (v->uid == uid) {
			*cnt = c;
			return v;
		}
	}

	*cnt = -1;
	return NULL;
}


void pcb_view_goto(pcb_view_t *item)
{
	if (item->have_bbox) {
		fgw_arg_t res, argv[5];

		argv[1].type = FGW_COORD; fgw_coord(&argv[1]) = item->bbox.X1;
		argv[2].type = FGW_COORD; fgw_coord(&argv[2]) = item->bbox.Y1;
		argv[3].type = FGW_COORD; fgw_coord(&argv[3]) = item->bbox.X2;
		argv[4].type = FGW_COORD; fgw_coord(&argv[4]) = item->bbox.Y2;
		pcb_actionv_bin("zoom", &res, 5, argv);
	}
}

pcb_view_t *pcb_view_new(const char *type, const char *title, const char *description)
{
	pcb_view_t *v = calloc(sizeof(pcb_view_t), 1);

	pcb_view_next_uid++;
	v->uid = pcb_view_next_uid;

	if (type == NULL) type = "";
	if (title == NULL) title = "";
	if (description == NULL) description = "";

	v->type = pcb_strdup(type);
	v->title = pcb_strdup(title);
	v->description = pcb_strdup(description);

	return v;
}

void pcb_view_append_obj(pcb_view_t *view, int grp, pcb_any_obj_t *obj)
{
	pcb_idpath_t *idp;

	assert((grp == 0) || (grp == 1));

	switch(obj->type) {
		case PCB_OBJ_TEXT:
		case PCB_OBJ_SUBC:
		case PCB_OBJ_LINE:
		case PCB_OBJ_ARC:
		case PCB_OBJ_POLY:
		case PCB_OBJ_PSTK:
		case PCB_OBJ_RAT:
			idp = pcb_obj2idpath(obj);
			if (idp == NULL)
				pcb_message(PCB_MSG_ERROR, "Internal error in pcb_drc_append_obj: can not resolve object id path\n");
			else
				pcb_idpath_list_append(&view->objs[grp], idp);
			break;
		default:
			pcb_message(PCB_MSG_ERROR, "Internal error in pcb_drc_append_obj: unknown object type %i\n", obj->type);
	}
}

void pcb_view_set_bbox_by_objs(pcb_data_t *data, pcb_view_t *v)
{
	int g;
	pcb_box_t b;
	pcb_any_obj_t *obj;
	pcb_idpath_t *idp;

	/* special case: no object - leave coords unloaded/invalid */
	if ((pcb_idpath_list_length(&v->objs[0]) < 1) && (pcb_idpath_list_length(&v->objs[1]) < 1))
		return;

	/* special case: single objet in group A, use the center */
	if (pcb_idpath_list_length(&v->objs[0]) == 1) {
		idp = pcb_idpath_list_first(&v->objs[0]);
		obj = pcb_idpath2obj_in(data, idp);
		if (obj != NULL) {
			v->have_bbox = 1;
			pcb_obj_center(obj, &v->x, &v->y);
			memcpy(&v->bbox, &obj->BoundingBox, sizeof(obj->BoundingBox));
			pcb_box_enlarge(&v->bbox, 0.25, 0.25);
			return;
		}
	}

	b.X1 = b.Y1 = PCB_MAX_COORD;
	b.X2 = b.Y2 = -PCB_MAX_COORD;
	for(g = 0; g < 2; g++) {
		for(idp = pcb_idpath_list_first(&v->objs[g]); idp != NULL; idp = pcb_idpath_list_next(idp)) {
			obj = pcb_idpath2obj_in(data, idp);
			if (obj != NULL) {
				v->have_bbox = 1;
				pcb_box_bump_box(&b, &obj->BoundingBox);
			}
		}
	}

	if (v->have_bbox) {
		v->x = (b.X1 + b.X2)/2;
		v->y = (b.Y1 + b.Y2)/2;
		memcpy(&v->bbox, &b, sizeof(b));
		pcb_box_enlarge(&v->bbox, 0.25, 0.25);
	}
}

static void view_append_str(gds_t *dst, const char *prefix, const char *key, const char *value)
{
	const char *s;
	pcb_append_printf(dst, "%s  %s = {", prefix, key);
	for(s = value; *s != '\0'; s++) {
		switch(*s) {
			case '\\':
			case '}':
				gds_append(dst, '\\');
				break;
		}
		gds_append(dst, *s);
	}
	gds_append_str(dst, "}\n");
}

void pcb_view_save_list_begin(gds_t *dst, const char *prefix)
{
	if (prefix != NULL)
		gds_append_str(dst, prefix);
	gds_append_str(dst, "li:view-list-v1 {\n");
}

void pcb_view_save_list_end(gds_t *dst, const char *prefix)
{
	if (prefix != NULL)
		gds_append_str(dst, prefix);
	gds_append_str(dst, "}\n");
}

void pcb_view_save(pcb_view_t *v, gds_t *dst, const char *prefix)
{
	int g, n;

	if (prefix == NULL)
		prefix = "";

	pcb_append_printf(dst, "%sha:view.%lu {\n", prefix, v->uid);
	view_append_str(dst, prefix, "type", v->type);
	view_append_str(dst, prefix, "title", v->title);
	view_append_str(dst, prefix, "description", v->description);
	if (v->have_bbox)
		pcb_append_printf(dst, "%s  li:bbox = {%.08$$mm; %.08$$mm; %.08$$mm; %.08$$mm;}\n", prefix, v->bbox.X1, v->bbox.Y1, v->bbox.X2, v->bbox.Y2);
	if (v->have_xy)
		pcb_append_printf(dst, "%s  li:xy = {%.08$$mm; %.08$$mm;}\n", prefix, v->x, v->y);

	for(g = 0; g < 2; g++) {
		if (pcb_idpath_list_length(&v->objs[g]) > 0) {
			pcb_idpath_t *i;
			pcb_append_printf(dst, "%s  li:objs.%d {\n", prefix, g);
			for(i = pcb_idpath_list_first(&v->objs[g]); i != NULL; i = pcb_idpath_list_next(i)) {
				pcb_append_printf(dst, "%s    li:id {", prefix);
				for(n = 0; n < i->len; n++)
					pcb_append_printf(dst, "%ld;", i->id[n]);
				pcb_append_printf(dst, "}\n", prefix);
			}
			pcb_append_printf(dst, "%s  }\n", prefix);
		}
	}

	switch(v->data_type) {
		case PCB_VIEW_PLAIN:
			pcb_append_printf(dst, "%s  data_type = plain\n", prefix);
			break;
		case PCB_VIEW_DRC:
			pcb_append_printf(dst, "%s  data_type = drc\n", prefix);
			pcb_append_printf(dst, "%s  ha:data {\n", prefix);
			pcb_append_printf(dst, "%s    required_value = %.08$$mm\n", prefix, v->data.drc.required_value);
			if (v->data.drc.have_measured)
				pcb_append_printf(dst, "%s    measured_value = %.08$$mm\n", prefix, v->data.drc.measured_value);
			pcb_append_printf(dst, "%s  }\n", prefix);
			break;
	}

	pcb_append_printf(dst, "%s}\n", prefix);
}

typedef struct {
	lht_doc_t *doc;
	lht_node_t *next;
} load_ctx_t;

void pcb_view_load_end(void *load_ctx)
{
	load_ctx_t *ctx = load_ctx;
	lht_dom_uninit(ctx->doc);
	free(ctx);
}

static void *view_load_post(load_ctx_t *ctx)
{
	if (ctx->doc->root == NULL)
		goto error;

	if (ctx->doc->root->type == LHT_LIST) {
		if (strcmp(ctx->doc->root->name, "view-list-v1") != 0)
			goto error;
		ctx->next = ctx->doc->root->data.list.first;
		return ctx;
	}

	if (ctx->doc->root->type == LHT_HASH) {
		if (strncmp(ctx->doc->root->name, "view.", 5) != 0)
			goto error;
		ctx->next = ctx->doc->root;
		return ctx;
	}

	error:;
	pcb_view_load_end(ctx);
	return NULL;
}

void *pcb_view_load_start_str(const char *src)
{
	load_ctx_t *ctx = malloc(sizeof(load_ctx_t));

	ctx->doc = lht_dom_init();

	for(; *src != '\0'; src++) {
		lht_err_t err = lht_dom_parser_char(ctx->doc, *src);
		if ((err != LHTE_SUCCESS) && (err != LHTE_STOP)) {
			pcb_view_load_end(ctx);
			return NULL;
		}
	}
	return view_load_post(ctx);
}

void *pcb_view_load_start_file(FILE *f)
{
	int c;
	load_ctx_t *ctx = malloc(sizeof(load_ctx_t));

	ctx->doc = lht_dom_init();

	while((c = fgetc(f)) != EOF) {
		lht_err_t err = lht_dom_parser_char(ctx->doc, c);
		if ((err != LHTE_SUCCESS) && (err != LHTE_STOP)) {
			pcb_view_load_end(ctx);
			return NULL;
		}
	}
	return view_load_post(ctx);
}

#define LOADERR "Error loading view: "

static void pcb_view_load_objs(pcb_view_t *dst, int grp, lht_node_t *olist)
{
	lht_node_t *n, *m;
	for(n = olist->data.list.first; n != NULL; n = n->next) {
		if ((n->type == LHT_LIST) && (strcmp(n->name, "id") == 0)) {
			pcb_idpath_t *i;
			int len, cnt;

			for(m = n->data.list.first, len = 0; m != NULL; m = m->next) {
				if (m->type != LHT_TEXT) {
					pcb_message(PCB_MSG_ERROR, LOADERR "Invalid object id (non-text)\n");
					goto nope;
				}
				len++;
			}

			i = pcb_idpath_alloc(len);
			for(m = n->data.list.first, cnt = 0; m != NULL; m = m->next, cnt++)
				i->id[cnt] = strtol(m->data.text.value, NULL, 10);

			pcb_idpath_list_append(&dst->objs[grp], i);
		}
		else
			pcb_message(PCB_MSG_ERROR, LOADERR "Invalid object id-path\n");
		nope:;
	}
}

pcb_view_t *pcb_view_load_next(void *load_ctx, pcb_view_t *dst)
{
	load_ctx_t *ctx = load_ctx;
	lht_node_t *n, *c;
	unsigned long int uid;
	char *end;
	pcb_view_type_t data_type;
	pcb_bool succ;

	if (ctx->next == NULL)
		return NULL;

	if ((ctx->next->type != LHT_HASH) || (strncmp(ctx->next->name, "view.", 5) != 0))
		return NULL;

	uid = strtoul(ctx->next->name + 5, &end, 10);
	if (*end != '\0')
		return NULL;

	data_type = PCB_VIEW_PLAIN;
	n = lht_dom_hash_get(ctx->next, "data_type");
	if ((n != NULL) && (n->type == LHT_TEXT)) {
		if (strcmp(n->data.text.value, "drc") == 0)
			data_type = PCB_VIEW_DRC;
		else if (strcmp(n->data.text.value, "plain") == 0)
			data_type = PCB_VIEW_PLAIN;
		else {
			pcb_message(PCB_MSG_ERROR, LOADERR "Invalid data type: '%s'\n", n->data.text.value);
			return NULL;
		}
	}

	if (dst == NULL)
		dst = calloc(sizeof(pcb_view_t), 1);

	dst->uid = uid;
	dst->data_type= data_type;

	n = lht_dom_hash_get(ctx->next, "type");
	if ((n != NULL) && (n->type == LHT_TEXT)) {
		dst->type = n->data.text.value;
		n->data.text.value = NULL;
	}

	n = lht_dom_hash_get(ctx->next, "title");
	if ((n != NULL) && (n->type == LHT_TEXT)) {
		dst->title = n->data.text.value;
		n->data.text.value = NULL;
	}

	n = lht_dom_hash_get(ctx->next, "description");
	if ((n != NULL) && (n->type == LHT_TEXT)) {
		dst->description = n->data.text.value;
		n->data.text.value = NULL;
	}

	n = lht_dom_hash_get(ctx->next, "bbox");
	if ((n != NULL) && (n->type == LHT_LIST)) {
		int ok = 0;

		c = n->data.list.first;
		if ((c != NULL) && (c->type == LHT_TEXT)) {
			dst->bbox.X1 = pcb_get_value(c->data.text.value, NULL, NULL, &succ);
			if (succ) ok++;
			c = c->next;
		}
		if ((c != NULL) && (c->type == LHT_TEXT)) {
			dst->bbox.Y1 = pcb_get_value(c->data.text.value, NULL, NULL, &succ);
			if (succ) ok++;
			c = c->next;
		}
		if ((c != NULL) && (c->type == LHT_TEXT)) {
			dst->bbox.X2 = pcb_get_value(c->data.text.value, NULL, NULL, &succ);
			if (succ) ok++;
			c = c->next;
		}
		if ((c != NULL) && (c->type == LHT_TEXT)) {
			dst->bbox.Y2 = pcb_get_value(c->data.text.value, NULL, NULL, &succ);
			if (succ) ok++;
			c = c->next;
		}
		if ((c == NULL) && (ok == 4))
			dst->have_bbox = 1;
		else
			pcb_message(PCB_MSG_ERROR, LOADERR "Invalid bbox values\n");
	}

	n = lht_dom_hash_get(ctx->next, "xy");
	if ((n != NULL) && (n->type == LHT_LIST)) {
		int ok = 0;

		c = n->data.list.first;
		if ((c != NULL) && (c->type == LHT_TEXT)) {
			dst->x = pcb_get_value(c->data.text.value, NULL, NULL, &succ);
			if (succ) ok++;
			c = c->next;
		}
		if ((c != NULL) && (c->type == LHT_TEXT)) {
			dst->y = pcb_get_value(c->data.text.value, NULL, NULL, &succ);
			if (succ) ok++;
			c = c->next;
		}
		if ((c == NULL) && (ok == 2))
			dst->have_bbox = 1;
		else
			pcb_message(PCB_MSG_ERROR, LOADERR "Invalid xy values\n");
	}

	n = lht_dom_hash_get(ctx->next, "objs.0");
	if ((n != NULL) && (n->type == LHT_LIST))
		pcb_view_load_objs(dst, 0, n);

	n = lht_dom_hash_get(ctx->next, "objs.1");
	if ((n != NULL) && (n->type == LHT_LIST))
		pcb_view_load_objs(dst, 1, n);

	switch(dst->data_type) {
		case PCB_VIEW_PLAIN: break;
		case PCB_VIEW_DRC:
			n = lht_dom_hash_get(ctx->next, "data");
			if ((n != NULL) && (n->type == LHT_HASH)) {
				c = lht_dom_hash_get(n, "required_value");
				if ((c != NULL) && (c->type == LHT_TEXT)) {
					dst->data.drc.required_value = pcb_get_value(c->data.text.value, NULL, NULL, &succ);
					if (!succ)
						pcb_message(PCB_MSG_ERROR, LOADERR "invalid drc required value: '%s'\n", c->data.text.value);
				}
				c = lht_dom_hash_get(n, "measured_value");
				if ((c != NULL) && (c->type == LHT_TEXT)) {
					dst->data.drc.measured_value = pcb_get_value(c->data.text.value, NULL, NULL, &succ);
					if (succ)
						dst->data.drc.have_measured = 1;
					else
						pcb_message(PCB_MSG_ERROR, LOADERR "invalid drc measured value: '%s'\n", c->data.text.value);
				}
			}
			break;
	}

	ctx->next = ctx->next->next;
	return dst;
}


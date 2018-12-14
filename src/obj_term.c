/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include <stdlib.h>
#include <ctype.h>
#include <genht/htsp.h>
#include <genvector/vtp0.h>

#include "change.h"
#include "compat_misc.h"
#include "obj_common.h"
#include "obj_term.h"
#include "obj_subc_parent.h"
#include "pcb-printf.h"
#include "undo.h"
#include "macro.h"
#include "polygon.h"

static const char core_term_cookie[] = "core-term";

static int term_name_invalid(const char *tname)
{
	if ((tname == NULL) || (*tname == '\0'))
		return 1;
	for(;*tname != '\0'; tname++)
		if ((!isalnum(*tname)) && (*tname != '_') && (*tname != '-'))
			return 1;
	return 0;
}

pcb_term_err_t pcb_term_name_is_valid(const char *tname)
{
	if (term_name_invalid(tname))
		return PCB_TERM_ERR_INVALID_NAME;

	return PCB_TERM_ERR_SUCCESS;
}

pcb_term_err_t pcb_term_add(htsp_t *terminals, const char *tname, pcb_any_obj_t *obj)
{
	htsp_entry_t *e;
	vtp0_t *v;

	if (obj->term != NULL)
		return PCB_TERM_ERR_ALREADY_TERMINAL;

	if (term_name_invalid(tname))
		return PCB_TERM_ERR_INVALID_NAME;

	e = htsp_getentry(terminals, tname);
	if (e == NULL) {
		/* allocate new terminal */
		tname = pcb_strdup(tname);
		v = malloc(sizeof(vtp0_t));
		vtp0_init(v);
		htsp_set(terminals, (char *)tname, v);
	}
	else {
		/* need to use the ones from the hash to avoid extra allocation/leak */
		v = e->value;
		tname = e->key;
	}

	obj->term = tname;
	vtp0_append(v, obj);

	return PCB_TERM_ERR_SUCCESS;
}

pcb_term_err_t pcb_term_del(htsp_t *terminals, pcb_any_obj_t *obj)
{
	vtp0_t *v;
	size_t n;

	if (obj->term == NULL)
		return PCB_TERM_ERR_NOT_IN_TERMINAL;

	v = htsp_get(terminals, obj->term);
	if (v == NULL)
		return PCB_TERM_ERR_TERM_NOT_FOUND;

	for(n = 0; n < v->used; n++) {
		if (v->array[n] == obj) {
			vtp0_remove(v, n, 1);
			if (v->used == 0)
				pcb_term_remove(terminals, obj->term);
			obj->term = NULL;
			return PCB_TERM_ERR_SUCCESS;
		}
	}

	return PCB_TERM_ERR_NOT_IN_TERMINAL;
}

static pcb_term_err_t pcb_term_remove_entry(htsp_t *terminals, htsp_entry_t *e)
{
	vtp0_t *v = e->value;
	char *name = e->key;
	size_t n;

	/* unlink all objects from this terminal */
	for(n = 0; n < v->used; n++) {
		pcb_any_obj_t *obj = v->array[n];
		obj->term = NULL;
	}

	htsp_delentry(terminals, e);
	free(name);
	vtp0_uninit(v);
	free(v);

	return PCB_TERM_ERR_SUCCESS;
}

pcb_term_err_t pcb_term_remove(htsp_t *terminals, const char *tname)
{
	htsp_entry_t *e;

	e = htsp_getentry(terminals, tname);
	if (e == NULL)
		return PCB_TERM_ERR_TERM_NOT_FOUND;

	return pcb_term_remove_entry(terminals, e);
}

pcb_term_err_t pcb_term_init(htsp_t *terminals)
{
	htsp_init(terminals, strhash, strkeyeq);
	return PCB_TERM_ERR_SUCCESS;
}

pcb_term_err_t pcb_term_uninit(htsp_t *terminals)
{
	htsp_entry_t *e;

	for (e = htsp_first(terminals); e; e = htsp_next(terminals, e))
		pcb_term_remove_entry(terminals, e);
	htsp_uninit(terminals);
	return PCB_TERM_ERR_SUCCESS;
}

/*** undoable term rename ***/

typedef struct {
	pcb_any_obj_t *obj;
	pcb_flag_t Flags;
	char str[1]; /* must be the last item, spans longer than 1 */
} term_rename_t;

static int undo_term_rename_swap(void *udata)
{
	char *old_term = NULL;
	pcb_subc_t *subc;
	term_rename_t *r = udata;
	int res = 0;
	pcb_flag_t ftmp;

	subc = pcb_obj_parent_subc(r->obj);
	if (subc == NULL) {
		pcb_message(PCB_MSG_ERROR, "Undo error: terminal rename: object %ld not part of a terminal\n", r->obj->ID);
		return -1;
	}

	/* remove from previous terminal */
	if (r->obj->term != NULL) {
		old_term = pcb_strdup(r->obj->term);
		res |= pcb_term_del(&subc->terminals, r->obj);
		pcb_obj_invalidate_label(r->obj->type, r->obj->parent.any, r->obj, r->obj);
		r->obj->term = NULL;
	}

	/* add to new terminal */
	if (*r->str != '\0') {
		res |= pcb_term_add(&subc->terminals, r->str, r->obj);
		pcb_obj_invalidate_label(r->obj->type, r->obj->parent.any, r->obj, r->obj);
	}

	/* swap name: redo & undo are symmetric; we made sure to have enough room for either old or new name */
	if (old_term == NULL)
		*r->str = '\0';
	else
		strcpy(r->str, old_term);

	free(old_term);

	/* swap flags: redo & undo are symmetric */
	ftmp = r->obj->Flags;
	r->obj->Flags = r->Flags;
	r->Flags = ftmp;

	/* Update the attributes */
	if (r->obj->term != NULL)
		pcb_attribute_put(&r->obj->Attributes, "term", r->obj->term);
	else
		pcb_attribute_remove(&r->obj->Attributes, "term");

	if (r->obj->type == PCB_OBJ_POLY)
		pcb_poly_init_clip(r->obj->parent.layer->parent.data, r->obj->parent.layer, (pcb_poly_t *)r->obj);

	return res;
}

static void undo_term_rename_print(void *udata, char *dst, size_t dst_len)
{
	term_rename_t *r = udata;
	pcb_snprintf(dst, dst_len, "term_rename: %s #%ld to '%s'\n",
		pcb_obj_type_name(r->obj->type), r->obj->ID, r->str);
}

static const uundo_oper_t undo_term_rename = {
	core_term_cookie,
	NULL, /* free */
	undo_term_rename_swap,
	undo_term_rename_swap,
	undo_term_rename_print
};

pcb_term_err_t pcb_term_undoable_rename(pcb_board_t *pcb, pcb_any_obj_t *obj, const char *new_name)
{
	int nname_len = 0, oname_len = 0, len;
	term_rename_t *r;
	pcb_subc_t *subc;

	if ((new_name == NULL) && (obj->term == NULL))
		return PCB_TERM_ERR_NO_CHANGE;

	if (((new_name != NULL) && (obj->term != NULL)) && (strcmp(new_name, obj->term) == 0))
		return PCB_TERM_ERR_NO_CHANGE;

	subc = pcb_obj_parent_subc(obj);
	if (subc == NULL)
		return PCB_TERM_ERR_NOT_IN_SUBC;

	if (new_name != NULL)
		nname_len = strlen(new_name);

	if (obj->term != NULL)
		oname_len = strlen(obj->term);

	len = nname_len > oname_len ? nname_len : oname_len; /* +1 for the terminator is implied by sizeof(->str) */

	r = pcb_undo_alloc(pcb, &undo_term_rename, sizeof(term_rename_t) + len);
	r->obj = obj;
	memcpy(r->str, new_name, nname_len+1);
	r->Flags = obj->Flags;
	PCB_FLAG_CLEAR(PCB_FLAG_CLEARPOLY, r);
	undo_term_rename_swap(r);

	if (obj->type == PCB_OBJ_POLY)
		pcb_poly_init_clip(obj->parent.layer->parent.data, obj->parent.layer, (pcb_poly_t *)obj);

	pcb_undo_inc_serial();
	return PCB_TERM_ERR_SUCCESS;
}

#define CHECK_TERM_LY(ob) \
	do { \
		if (PCB_NSTRCMP(term_name, ob->term) == 0 && (!same || !PCB_FLAG_TEST(PCB_FLAG_DRC, ob))) { \
			if (parent_out != NULL) *parent_out = subc; \
			if (gid_out != NULL) *gid_out = pcb_layer_get_group_(layer); \
			return (pcb_any_obj_t *)ob; \
		} \
	} while(0)

#define CHECK_TERM_GL(ob) \
	do { \
		if (PCB_NSTRCMP(term_name, ob->term) == 0 && (!same || !PCB_FLAG_TEST(PCB_FLAG_DRC, ob))) { \
			if (parent_out != NULL) *parent_out = subc; \
			if (gid_out != NULL) { \
				*gid_out = -1; \
				pcb_layergrp_list(pcb, lyt, gid_out, 1); \
			} \
			return (pcb_any_obj_t *)ob; \
		} \
	} while(0)

pcb_any_obj_t *pcb_term_find_name(pcb_board_t *pcb, pcb_data_t *data, pcb_layer_type_t lyt, const char *subc_name, const char *term_name, pcb_bool same, pcb_subc_t **parent_out, pcb_layergrp_id_t *gid_out)
{
	pcb_subc_t *subc;
	pcb_layer_t *layer;
	int l;

	if (lyt == 0)
		return pcb_false;

	if ((subc = pcb_subc_by_refdes(data, subc_name)) == NULL)
		return NULL;

	if (PCB_FLAG_TEST(PCB_FLAG_NONETLIST, subc))
		return NULL;

	/* search for global objects: padstack */
	if (lyt & (PCB_LYT_COPPER | PCB_LYT_MASK | PCB_LYT_MECH)) {
		PCB_PADSTACK_LOOP(subc->data) {
			CHECK_TERM_GL(padstack);
		} PCB_END_LOOP;
	}

	/* search for layer objects */
	layer = subc->data->Layer;
	for (l = 0; l < subc->data->LayerN; l++, layer++) {
		if ((pcb_layer_flags_(layer) & lyt) == 0)
			continue;
		PCB_LINE_LOOP(layer) {
			CHECK_TERM_LY(line);
		} PCB_END_LOOP;

		PCB_ARC_LOOP(layer) {
			CHECK_TERM_LY(arc);
		} PCB_END_LOOP;

		PCB_POLY_LOOP(layer) {
			CHECK_TERM_LY(polygon);
		} PCB_END_LOOP;


		PCB_TEXT_LOOP(layer) {
			CHECK_TERM_LY(text);
		} PCB_END_LOOP;
	}

	return NULL;
}

#undef CHECK_TERM_LY
#undef CHECK_TERM_GL


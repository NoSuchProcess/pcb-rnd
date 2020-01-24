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

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <librnd/core/unit.h>
#include <librnd/poly/rtree.h>

#include <genrtree/genrtree_impl.h>
#include <genrtree/genrtree_search.h>
#include <genrtree/genrtree_delete.h>
#include <genrtree/genrtree_debug.h>
#include "rtree2_compat.h"

/* Temporary compatibility layer for the transition */
pcb_rtree_t *pcb_r_create_tree(void)
{
	pcb_rtree_t *root = malloc(sizeof(pcb_rtree_t));
	pcb_rtree_init(root);
	return root;
}

void pcb_r_destroy_tree(pcb_rtree_t **tree)
{
	pcb_rtree_uninit(*tree);
	free(*tree);
	*tree = NULL;
}

void pcb_r_insert_entry(pcb_rtree_t *rtree, const pcb_box_t *which)
{
	pcb_any_obj_t *obj = (pcb_any_obj_t *)which; /* assumes first field is the bounding box */
	assert(obj != NULL);
	pcb_rtree_insert(rtree, obj, (pcb_rtree_box_t *)which);
}

void pcb_r_insert_array(pcb_rtree_t *rtree, const pcb_box_t *boxlist[], pcb_cardinal_t len)
{
	pcb_cardinal_t n;

	if (len == 0)
		return;

	assert(boxlist != 0);

	for(n = 0; n < len; n++)
		pcb_r_insert_entry(rtree, boxlist[n]);
}

pcb_bool pcb_r_delete_entry(pcb_rtree_t *rtree, const pcb_box_t *which)
{
	pcb_any_obj_t *obj = (pcb_any_obj_t *)which; /* assumes first field is the bounding box */
	assert(obj != NULL);
	return pcb_rtree_delete(rtree, obj, (pcb_rtree_box_t *)which) == 0;
}

pcb_bool pcb_r_delete_entry_free_data(pcb_rtree_t *rtree, const pcb_box_t *box, void (*free_data)(void *d))
{
	pcb_any_obj_t *obj = (pcb_any_obj_t *)box; /* assumes first field is the bounding box */
	assert(obj != NULL);
	if (pcb_rtree_delete(rtree, obj, (pcb_rtree_box_t *)box) != 0)
		return pcb_false;
	free_data(obj);
	return pcb_true;
}

typedef struct {
	pcb_r_dir_t (*region_in_search)(const pcb_box_t *region, void *closure);
	pcb_r_dir_t (*rectangle_in_region)(const pcb_box_t *box, void *closure);
	void *clo;
} r_cb_t;

static pcb_rtree_dir_t r_cb_node(void *ctx_, void *obj, const pcb_rtree_box_t *box)
{
	r_cb_t *ctx = (r_cb_t *)ctx_;
	return ctx->region_in_search((const pcb_box_t *)box, ctx->clo);
}

static pcb_rtree_dir_t r_cb_obj(void *ctx_, void *obj, const pcb_rtree_box_t *box)
{
	r_cb_t *ctx = (r_cb_t *)ctx_;
	return ctx->rectangle_in_region((const pcb_box_t *)obj, ctx->clo);
}


pcb_r_dir_t pcb_r_search(pcb_rtree_t *rtree, const pcb_box_t *query,
	pcb_r_dir_t (*region_in_search)(const pcb_box_t *region, void *closure),
	pcb_r_dir_t (*rectangle_in_region)(const pcb_box_t *box, void *closure),
	void *closure, int *num_found)
{
	pcb_r_dir_t res;
	pcb_rtree_cardinal_t out_cnt;
	r_cb_t ctx;
	ctx.region_in_search = region_in_search;
	ctx.rectangle_in_region = rectangle_in_region;
	ctx.clo = closure;

	res = pcb_rtree_search_any(rtree, (const pcb_rtree_box_t *)query,
		(ctx.region_in_search != NULL) ? r_cb_node : NULL,
		(ctx.rectangle_in_region != NULL) ? r_cb_obj : NULL,
		&ctx, &out_cnt);

	if (num_found != NULL)
		*num_found = out_cnt;

	return res;
}

int pcb_r_region_is_empty(pcb_rtree_t *rtree, const pcb_box_t *region)
{
	return pcb_rtree_is_box_empty(rtree, (const pcb_rtree_box_t *)region);
}

static void r_print_obj(FILE *f, void *obj)
{
	fprintf(f, "<obj %p>\n", obj);
}

void pcb_r_dump_tree(pcb_rtree_t *root, int unused)
{
	pcb_rtree_dump_text(stdout, root, r_print_obj);
}

void pcb_r_free_tree_data(pcb_rtree_t *rtree, void (*free)(void *ptr))
{
	pcb_rtree_it_t it;
	void *o;

	for(o = pcb_rtree_all_first(&it, rtree); o != NULL; o = pcb_rtree_all_next(&it))
		free(o);
}

pcb_box_t *pcb_r_first(pcb_rtree_t *tree, pcb_rtree_it_t *it)
{
	if (tree == NULL)
		return NULL;
	return (pcb_box_t *)pcb_rtree_all_first(it, tree);
}

pcb_box_t *pcb_r_next(pcb_rtree_it_t *it)
{
	return (pcb_box_t *)pcb_rtree_all_next(it);
}

void pcb_r_end(pcb_rtree_it_t *it)
{
}

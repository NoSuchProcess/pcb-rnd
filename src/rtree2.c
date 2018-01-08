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
  */

#include "config.h"

#include <assert.h>
#include <string.h>

#include "unit.h"
#include "rtree2.h"

/* Instantiate an rtree */
#define RTR(n)  pcb_rtree_ ## n
#define RTRU(n) pcb_RTREE_ ## n
#define pcb_rtree_privfunc static
#define pcb_rtree_size 6

#include <genrtree/genrtree_impl.h>
#include <genrtree/genrtree_search.h>
#include <genrtree/genrtree_delete.h>

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


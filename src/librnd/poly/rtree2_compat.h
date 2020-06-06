/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1998,1999,2000,2001 harry eaton
 *
 *  this file, rtree.h, was written and is
 *  Copyright (c) 2004 harry eaton, it's based on C. Scott's kdtree.h template
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
 *
 *
 *  Old contact info:
 *  harry eaton, 6697 Buttonhole Ct, Columbia, MD 21044 USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

/* Compatibility layer between the new rtree and the old rtree for the
   period of transition */

#ifndef RND_RTREE2_COMPAT_H
#define RND_RTREE2_COMPAT_H

#include <librnd/poly/rtree.h>

/* callback direction to the search engine */
typedef enum rnd_r_dir_e {
	RND_R_DIR_NOT_FOUND = 0,         /* object not found or not accepted */
	RND_R_DIR_FOUND_CONTINUE = 1,    /* object found or accepted, go on searching */
	RND_R_DIR_CANCEL = 2             /* cancel the search and return immediately */
} rnd_r_dir_t;

rnd_rtree_t *rnd_r_create_tree(void);
void rnd_r_destroy_tree(rnd_rtree_t **tree);
void rnd_r_free_tree_data(rnd_rtree_t *rtree, void (*free)(void *ptr));

void rnd_r_insert_entry(rnd_rtree_t *rtree, const rnd_box_t *which);
void rnd_r_insert_array(rnd_rtree_t *rtree, const rnd_box_t *boxlist[], rnd_cardinal_t len);

rnd_bool rnd_r_delete_entry(rnd_rtree_t *rtree, const rnd_box_t *which);
rnd_bool rnd_r_delete_entry_free_data(rnd_rtree_t *rtree, rnd_box_t *box, void (*free_data)(void *d));

/* generic search routine */
/* region_in_search should return rnd_true if "what you're looking for" is
 * within the specified region; 
 * rectangle_in_region should return rnd_true if the given rectangle is
 * "what you're looking for".
 * The search will find all rectangles matching the criteria given
 * by region_in_search and rectangle_in_region and return a count of
 * how many things rectangle_in_region returned rnd_true for. closure is
 * used to abort the search if desired from within rectangle_in_region
 * Look at the implementation of r_region_is_empty for how to
 * abort the search if that is the desired behavior.
 */
rnd_r_dir_t rnd_r_search(rnd_rtree_t *rtree, const rnd_box_t *query,
	rnd_r_dir_t (*region_in_search)(const rnd_box_t *region, void *closure),
	rnd_r_dir_t (*rectangle_in_region)(const rnd_box_t *box, void *closure),
	void *closure, int *num_found);

/* return 0 if there are any rectangles in the given region. */
int rnd_r_region_is_empty(rnd_rtree_t *rtree, const rnd_box_t *region);

void rnd_r_dump_tree(rnd_rtree_t *root, int unused);

#define RND_RTREE_EMPTY(rt) (((rt) == NULL) || ((rt)->size == 0))

/* -- Iterate through an rtree; DO NOT modify the tree while iterating -- */

/* Get the first item, get fields of iterator set up; return can be casted to an object; returns NULL if rtree is empty */
rnd_box_t *rnd_r_first(rnd_rtree_t *tree, rnd_rtree_it_t *it);

/* Get the next item, return can be casted to an object; returns NULL if no more items */
rnd_box_t *rnd_r_next(rnd_rtree_it_t *it);

/* Free fields of the iterator - not needed anymore, will be removed */
void rnd_r_end(rnd_rtree_it_t *it);

#endif

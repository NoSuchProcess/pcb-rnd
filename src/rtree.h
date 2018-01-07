/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Contact addresses for paper mail and Email:
 *  harry eaton, 6697 Buttonhole Ct, Columbia, MD 21044 USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

/* r-tree: efficient lookup of screenobjects */

#ifndef PCB_RTREE_H
#define PCB_RTREE_H

#include "global_typedefs.h"

struct pcb_rtree_s {
	struct rtree_node *root;
	int size;											/* number of entries in tree */
};

/* the number of entries in each rtree node
 * 4 - 7 seem to be pretty good settings
 */
#define PCB_RTREE_SIZE 6


struct pcb_rtree_it_s {
	struct rtree_node **open;
	int used, alloced;
	pcb_box_t *ready[PCB_RTREE_SIZE + 1];
	int num_ready;
};

/* callback direction to the search engine */
typedef enum pcb_r_dir_e {
	PCB_R_DIR_NOT_FOUND = 0,         /* object not found or not accepted */
	PCB_R_DIR_FOUND_CONTINUE = 1,    /* object found or accepted, go on searching */
	PCB_R_DIR_CANCEL                 /* cancel the search and return immediately */
} pcb_r_dir_t;

pcb_rtree_t *pcb_r_create_tree(void);
void pcb_r_destroy_tree(pcb_rtree_t **rtree);

/* insert an unsorted list of boxes into an existing r-tree the r-tree will
   keep pointers into it, so don't free the box list until you've called
   r_destroy_tree. If you set 'manage' to pcb_true, r_destroy_tree will
   free your boxlist. */
void pcb_r_insert_array(pcb_rtree_t *rtree, const pcb_box_t *boxlist[], int N);

pcb_bool pcb_r_delete_entry(pcb_rtree_t * rtree, const pcb_box_t * which);

/* also free leaf data using free_data() */
pcb_bool __r_delete_free_data(struct rtree_node *node, const pcb_box_t * query, void (*free_data)(void *data));
pcb_bool pcb_r_delete_entry_free_data(pcb_rtree_t * rtree, const pcb_box_t * box, void (*free_data)(void *d));


void pcb_r_insert_entry(pcb_rtree_t * rtree, const pcb_box_t * which);


/* generic search routine */
/* region_in_search should return pcb_true if "what you're looking for" is
 * within the specified region; regions, like rectangles, are closed on
 * top and left and open on bottom and right.
 * rectangle_in_region should return pcb_true if the given rectangle is
 * "what you're looking for".
 * The search will find all rectangles matching the criteria given
 * by region_in_search and rectangle_in_region and return a count of
 * how many things rectangle_in_region returned pcb_true for. closure is
 * used to abort the search if desired from within rectangle_in_region
 * Look at the implementation of r_region_is_empty for how to
 * abort the search if that is the desired behavior.
 */

pcb_r_dir_t pcb_r_search(pcb_rtree_t * rtree, const pcb_box_t * starting_region,
						 pcb_r_dir_t (*region_in_search) (const pcb_box_t * region, void *cl),
						 pcb_r_dir_t (*rectangle_in_region) (const pcb_box_t * box, void *cl), void *closure,
						 int *num_found);

/* -- special-purpose searches build upon r_search -- */
/* return 0 if there are any rectangles in the given region. */
int pcb_r_region_is_empty(pcb_rtree_t * rtree, const pcb_box_t * region);

void pcb_r_dump_tree(struct rtree_node *, int);

#define PCB_RTREE_EMPTY(rt) (((rt) == NULL) || ((rt)->size == 0))


/* -- Iterate through an rtree; DO NOT modify the tree while iterating -- */

/* Get the first item, get fields of iterator set up; return can be casted to an object; returns NULL if rtree is empty */
pcb_box_t *pcb_r_first(pcb_rtree_t *tree, pcb_rtree_it_t *it);

/* Get the next item, return can be casted to an object; returns NULL if no more items */
pcb_box_t *pcb_r_next(pcb_rtree_it_t *it);

/* Free fields of the iterator */
void pcb_r_end(pcb_rtree_it_t *it);

/* Recursively call free() on all leaf data */
void pcb_r_free_tree_data(pcb_rtree_t *rtree, void (*free)(void *ptr));

#endif

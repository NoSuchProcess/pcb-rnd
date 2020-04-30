/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1998,1999,2000,2001 harry eaton
 *
 *  this file, heap.h, was written and is
 *  Copyright (c) 2001 C. Scott Ananian.
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

/* Heap used by the polygon code and autoroute */

#ifndef RND_HEAP_H
#define RND_HEAP_H

#include <librnd/config.h>

/* type of heap costs */
typedef double rnd_heap_cost_t;
/* what a heap looks like */
typedef struct rnd_heap_s rnd_heap_t;

/* create an empty heap */
rnd_heap_t *rnd_heap_create();
/* destroy a heap */
void rnd_heap_destroy(rnd_heap_t ** heap);
/* free all elements in a heap */
void rnd_heap_free(rnd_heap_t * heap, void (*funcfree) (void *));

/* -- mutation -- */
void rnd_heap_insert(rnd_heap_t * heap, rnd_heap_cost_t cost, void *data);
void *rnd_heap_remove_smallest(rnd_heap_t * heap);
/* replace the smallest item with a new item and return the smallest item.
 * (if the new item is the smallest, than return it, instead.) */
void *rnd_heap_replace(rnd_heap_t * heap, rnd_heap_cost_t cost, void *data);

/* -- interrogation -- */
int rnd_heap_is_empty(rnd_heap_t * heap);
int rnd_heap_size(rnd_heap_t * heap);

#endif /* RND_HEAP_H */

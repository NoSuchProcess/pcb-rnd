/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1998,1999,2000,2001 harry eaton
 *
 *  this file, heap.c, was written and is
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


/* operations on heaps. */

#include <stdlib.h>
#include <assert.h>
#include <librnd/config.h>
#include <librnd/core/heap.h>

/* define this for more thorough self-checking of data structures */
#undef SLOW_ASSERTIONS

struct heap_element {
	rnd_heap_cost_t cost;
	void *data;
};
struct rnd_heap_s {
	struct heap_element *element;
	int size, max;
};

static rnd_heap_cost_t MIN_COST = 0;

/* ---------------------------------------------------------------------------
 * functions.
 */
/* helper functions for assertions */
#ifndef NDEBUG
#ifdef SLOW_ASSERTIONS
static int __heap_is_good_slow(rnd_heap_t * heap)
{
	int i;
	/* heap condition: key in each node should be smaller than in its children */
	/* alternatively (and this is what we check): key in each node should be
	 * larger than (or equal to) key of its parent. */
	/* note that array element[0] is not used (except as a sentinel) */
	for (i = 2; i < heap->size; i++)
		if (heap->element[i].cost < heap->element[i / 2].cost)
			return 0;
	return 1;
}
#endif /* SLOW_ASSERTIONS */
static int __heap_is_good(rnd_heap_t * heap)
{
	return heap && (heap->max == 0 || heap->element) &&
		(heap->max >= 0) && (heap->size >= 0) && (heap->max == 0 || heap->size < heap->max) &&
#ifdef SLOW_ASSERTIONS
		__heap_is_good_slow(heap) &&
#endif
		1;
}
#endif /* ! NDEBUG */

/* create an empty heap */
rnd_heap_t *rnd_heap_create()
{
	rnd_heap_t *heap;
	/* initialize MIN_COST if necessary */
	if (MIN_COST == 0)
		MIN_COST = -1e23;
	assert(MIN_COST < 0);
	/* okay, create empty heap */
	heap = (rnd_heap_t *) calloc(1, sizeof(*heap));
	assert(heap);
	assert(__heap_is_good(heap));
	return heap;
}

/* destroy a heap */
void rnd_heap_destroy(rnd_heap_t ** heap)
{
	assert(heap && *heap);
	assert(__heap_is_good(*heap));
	if ((*heap)->element)
		free((*heap)->element);
	free(*heap);
	*heap = NULL;
}

/* free all elements in the heap */
void rnd_heap_free(rnd_heap_t * heap, void (*freefunc) (void *))
{
	assert(heap);
	assert(__heap_is_good(heap));
	for (; heap->size; heap->size--) {
		if (heap->element[heap->size].data)
			freefunc(heap->element[heap->size].data);
	}
}

/* -- mutation -- */
static void __upheap(rnd_heap_t * heap, int k)
{
	struct heap_element v;

	assert(heap && heap->size < heap->max);
	assert(k <= heap->size);

	v = heap->element[k];
	heap->element[0].cost = MIN_COST;
	for (v = heap->element[k]; heap->element[k / 2].cost > v.cost; k = k / 2)
		heap->element[k] = heap->element[k / 2];
	heap->element[k] = v;
}

void rnd_heap_insert(rnd_heap_t * heap, rnd_heap_cost_t cost, void *data)
{
	assert(heap && __heap_is_good(heap));
	assert(cost >= MIN_COST);

	/* determine whether we need to grow the heap */
	if (heap->size + 1 >= heap->max) {
		heap->max *= 2;
		if (heap->max == 0)
			heap->max = 256;					/* default initial heap size */
		heap->element = (struct heap_element *) realloc(heap->element, heap->max * sizeof(*heap->element));
	}
	heap->size++;
	assert(heap->size < heap->max);
	heap->element[heap->size].cost = cost;
	heap->element[heap->size].data = data;
	__upheap(heap, heap->size);		/* fix heap condition violation */
	assert(__heap_is_good(heap));
	return;
}

/* this procedure moves down the heap, exchanging the node at position
 * k with the smaller of its two children as necessary and stopping when
 * the node at k is smaller than both children or the bottom is reached.
 */
static void __downheap(rnd_heap_t * heap, int k)
{
	struct heap_element v;

	assert(heap && heap->size < heap->max);
	assert(k <= heap->size);

	v = heap->element[k];
	while (k <= heap->size / 2) {
		int j = k + k;
		if (j < heap->size && heap->element[j].cost > heap->element[j + 1].cost)
			j++;
		if (v.cost < heap->element[j].cost)
			break;
		heap->element[k] = heap->element[j];
		k = j;
	}
	heap->element[k] = v;
}

void *rnd_heap_remove_smallest(rnd_heap_t * heap)
{
	struct heap_element v;
	assert(heap && __heap_is_good(heap));
	assert(heap->size > 0);
	assert(heap->max > 1);

	v = heap->element[1];
	heap->element[1] = heap->element[heap->size--];
	if (heap->size > 0)
		__downheap(heap, 1);

	assert(__heap_is_good(heap));
	return v.data;
}

void *rnd_heap_replace(rnd_heap_t * heap, rnd_heap_cost_t cost, void *data)
{
	assert(heap && __heap_is_good(heap));

	if (rnd_heap_is_empty(heap))
		return data;

	assert(heap->size > 0);
	assert(heap->max > 1);

	heap->element[0].cost = cost;
	heap->element[0].data = data;
	__downheap(heap, 0);					/* ooh, tricky! */

	assert(__heap_is_good(heap));
	return heap->element[0].data;
}

/* -- interrogation -- */
int rnd_heap_is_empty(rnd_heap_t * heap)
{
	assert(__heap_is_good(heap));
	return heap->size == 0;
}

/* -- size -- */
int rnd_heap_size(rnd_heap_t * heap)
{
	assert(__heap_is_good(heap));
	return heap->size;
}

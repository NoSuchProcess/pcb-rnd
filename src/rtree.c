/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1998,1999,2000,2001,2002,2003,2004 harry eaton
 *
 *  this file, rtree.c, was written and is
 *  Copyright (c) 2004, harry eaton
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  harry eaton, 6697 Buttonhole Ct, Columbia, MD 21044 USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */


/* implements r-tree structures.
 * these should be much faster for the auto-router
 * because the recursive search is much more efficient
 * and that's where the auto-router spends all its time.
 */
#include "config.h"

#include "rtree.h"

#if PCB_USE_GENRTREE
#include "rtree2.c"
#else

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <setjmp.h>

#include "math_helper.h"
#include "compat_cc.h"
#include "obj_common.h"
#include "macro.h"

#define SLOW_ASSERTS
/* All rectangles are closed on the bottom left and open on the
 * top right. i.e. they contain one corner point, but not the other.
 * This requires that the corner points not be equal!
 */

#define M_SIZE PCB_RTREE_SIZE

/* it seems that sorting the leaf order slows us down
 * but sometimes gives better routes
 */
#undef SORT
#define SORT_NONLEAF

#define DELETE_BY_POINTER

typedef struct {
	const pcb_box_t *bptr;					/* pointer to the box */
	pcb_box_t bounds;								/* copy of the box for locality of reference */
} Rentry;

struct rtree_node {
	pcb_box_t box;									/* bounds rectangle of this node */
	struct rtree_node *parent;		/* parent of this node, NULL = root */
	struct {
		unsigned is_leaf:1;					/* this is a leaf node */
		unsigned unused:31;					/* pcb_true==should free 'rect.bptr' if node is destroyed */
	} flags;
	union {
		struct rtree_node *kids[M_SIZE + 1];	/* when not leaf */
		Rentry rects[M_SIZE + 1];		/* when leaf */
	} u;
};

#ifndef NDEBUG
#ifdef SLOW_ASSERTS
static int __r_node_is_good(struct rtree_node *node)
{
	int i, flag = 1;
	int kind = -1;
	pcb_bool last = pcb_false;

	if (node == NULL)
		return 1;
	for (i = 0; i < M_SIZE; i++) {
		if (node->flags.is_leaf) {
			if (!node->u.rects[i].bptr) {
				last = pcb_true;
				continue;
			}
			/* check that once one entry is empty, all the rest are too */
			if (node->u.rects[i].bptr && last)
				assert(0);
			/* check that the box makes sense */
			if (node->box.X1 > node->box.X2)
				assert(0);
			if (node->box.Y1 > node->box.Y2)
				assert(0);
			/* check that bounds is the same as the pointer */
			if (node->u.rects[i].bounds.X1 != node->u.rects[i].bptr->X1)
				assert(0);
			if (node->u.rects[i].bounds.Y1 != node->u.rects[i].bptr->Y1)
				assert(0);
			if (node->u.rects[i].bounds.X2 != node->u.rects[i].bptr->X2)
				assert(0);
			if (node->u.rects[i].bounds.Y2 != node->u.rects[i].bptr->Y2)
				assert(0);
			/* check that entries are within node bounds */
			if (node->u.rects[i].bounds.X1 < node->box.X1)
				assert(0);
			if (node->u.rects[i].bounds.X2 > node->box.X2)
				assert(0);
			if (node->u.rects[i].bounds.Y1 < node->box.Y1)
				assert(0);
			if (node->u.rects[i].bounds.Y2 > node->box.Y2)
				assert(0);
		}
		else {
			if (!node->u.kids[i]) {
				last = pcb_true;
				continue;
			}
			/* make sure all children are the same type */
			if (kind == -1)
				kind = node->u.kids[i]->flags.is_leaf;
			else if (kind != node->u.kids[i]->flags.is_leaf)
				assert(0);
			/* check that once one entry is empty, all the rest are too */
			if (node->u.kids[i] && last)
				assert(0);
			/* check that entries are within node bounds */
			if (node->u.kids[i]->box.X1 < node->box.X1)
				assert(0);
			if (node->u.kids[i]->box.X2 > node->box.X2)
				assert(0);
			if (node->u.kids[i]->box.Y1 < node->box.Y1)
				assert(0);
			if (node->u.kids[i]->box.Y2 > node->box.Y2)
				assert(0);
		}
		flag <<= 1;
	}
	/* check that we're completely in the parent's bounds */
	if (node->parent) {
		if (node->parent->box.X1 > node->box.X1)
			assert(0);
		if (node->parent->box.X2 < node->box.X2)
			assert(0);
		if (node->parent->box.Y1 > node->box.Y1)
			assert(0);
		if (node->parent->box.Y2 < node->box.Y2)
			assert(0);
	}
	/* make sure overflow is empty */
	if (!node->flags.is_leaf && node->u.kids[i])
		assert(0);
	if (node->flags.is_leaf && node->u.rects[i].bptr)
		assert(0);
	return 1;
}

/* check the whole tree from this node down for consistency */
static pcb_bool __r_tree_is_good(struct rtree_node *node)
{
	int i;

	if (!node)
		return 1;
	if (!__r_node_is_good(node))
		assert(0);
	if (node->flags.is_leaf)
		return 1;
	for (i = 0; i < M_SIZE; i++) {
		if (!__r_tree_is_good(node->u.kids[i]))
			return 0;
	}
	return 1;
}
#endif
#endif

#ifndef NDEBUG
/* print out the tree */
void pcb_r_dump_tree(struct rtree_node *node, int depth)
{
	int i, j;
	static int count;
	static double area;

	if (depth == 0) {
		area = 0;
		count = 0;
	}
	area += (node->box.X2 - node->box.X1) * (double) (node->box.Y2 - node->box.Y1);
	count++;
	for (i = 0; i < depth; i++)
		printf("  ");
	if (!node->flags.is_leaf) {
		printf("p=0x%p node X(%d, %d) Y(%d, %d)\n", (void *) node, node->box.X1, node->box.X2, node->box.Y1, node->box.Y2);
	}
	else {
		printf("p=0x%p leaf X(%d, %d) Y(%d, %d)\n", (void *) node, node->box.X1, node->box.X2, node->box.Y1, node->box.Y2);
		for (j = 0; j < M_SIZE; j++) {
			if (!node->u.rects[j].bptr)
				break;
			area +=
				(node->u.rects[j].bounds.X2 -
				 node->u.rects[j].bounds.X1) * (double) (node->u.rects[j].bounds.Y2 - node->u.rects[j].bounds.Y1);
			count++;
			for (i = 0; i < depth + 1; i++)
				printf("  ");
			printf("entry 0x%p X(%d, %d) Y(%d, %d)\n",
						 (void *) (node->u.rects[j].bptr),
						 node->u.rects[j].bounds.X1, node->u.rects[j].bounds.X2, node->u.rects[j].bounds.Y1, node->u.rects[j].bounds.Y2);
		}
		return;
	}
	for (i = 0; i < M_SIZE; i++)
		if (node->u.kids[i])
			pcb_r_dump_tree(node->u.kids[i], depth + 1);
	if (depth == 0)
		printf("average box area is %g\n", area / count);
}
#endif

/* Sort the children or entries of a node
 * according to the largest side.
 */
#ifdef SORT
static int cmp_box(const pcb_box_t * a, const pcb_box_t * b)
{
	/* compare two box coordinates so that the __r_search
	 * will fail at the earliest comparison possible.
	 * It needs to see the biggest X1 first, then the
	 * smallest X2, the biggest Y1 and smallest Y2
	 */
	if (a->X1 < b->X1)
		return 0;
	if (a->X1 > b->X1)
		return 1;
	if (a->X2 > b->X2)
		return 0;
	if (a->X2 < b->X2)
		return 1;
	if (a->Y1 < b->Y1)
		return 0;
	if (a->Y1 > b->Y1)
		return 1;
	if (a->Y2 > b->Y2)
		return 0;
	return 1;
}

static void sort_node(struct rtree_node *node)
{
	if (node->flags.is_leaf) {
		register Rentry *r, *i, temp;

		for (r = &node->u.rects[1]; r->bptr; r++) {
			temp = *r;
			i = r - 1;
			while (i >= &node->u.rects[0]) {
				if (cmp_box(&i->bounds, &r->bounds))
					break;
				*(i + 1) = *i;
				i--;
			}
			*(i + 1) = temp;
		}
	}
#ifdef SORT_NONLEAF
	else {
		register struct rtree_node **r, **i, *temp;

		for (r = &node->u.kids[1]; *r; r++) {
			temp = *r;
			i = r - 1;
			while (i >= &node->u.kids[0]) {
				if (cmp_box(&(*i)->box, &(*r)->box))
					break;
				*(i + 1) = *i;
				i--;
			}
			*(i + 1) = temp;
		}
	}
#endif
}
#else
#define sort_node(x)
#endif

/* set the node bounds large enough to encompass all
 * of the children's rectangles
 */
static void adjust_bounds(struct rtree_node *node)
{
	int i;

	assert(node);
	assert(node->u.kids[0]);
	if (node->flags.is_leaf) {
		node->box = node->u.rects[0].bounds;
		for (i = 1; i < M_SIZE + 1; i++) {
			if (!node->u.rects[i].bptr)
				return;
			PCB_MAKE_MIN(node->box.X1, node->u.rects[i].bounds.X1);
			PCB_MAKE_MAX(node->box.X2, node->u.rects[i].bounds.X2);
			PCB_MAKE_MIN(node->box.Y1, node->u.rects[i].bounds.Y1);
			PCB_MAKE_MAX(node->box.Y2, node->u.rects[i].bounds.Y2);
		}
	}
	else {
		node->box = node->u.kids[0]->box;
		for (i = 1; i < M_SIZE + 1; i++) {
			if (!node->u.kids[i])
				return;
			PCB_MAKE_MIN(node->box.X1, node->u.kids[i]->box.X1);
			PCB_MAKE_MAX(node->box.X2, node->u.kids[i]->box.X2);
			PCB_MAKE_MIN(node->box.Y1, node->u.kids[i]->box.Y1);
			PCB_MAKE_MAX(node->box.Y2, node->u.kids[i]->box.Y2);
		}
	}
}

pcb_rtree_t *pcb_r_create_tree(void)
{
	pcb_rtree_t *rtree;
	struct rtree_node *node;

	rtree = calloc(1, sizeof(*rtree));

	/* start with a single empty leaf node */
	node = calloc(1, sizeof(*node));
	node->flags.is_leaf = 1;
	node->parent = NULL;
	rtree->root = node;
	return rtree;
}

void pcb_r_insert_array(pcb_rtree_t *rtree, const pcb_box_t *boxlist[], int N)
{
	int i;

	assert(N >= 0);

	/* simple, just insert all of the boxes! */
	for (i = 0; i < N; i++) {
		assert(boxlist[i]);
		pcb_r_insert_entry(rtree, boxlist[i]);
	}
#ifdef SLOW_ASSERTS
	assert(__r_tree_is_good(rtree->root));
#endif
}


static void __r_destroy_tree(struct rtree_node *node)
{
	int i, flag = 1;

	if (node->flags.is_leaf)
		for (i = 0; i < M_SIZE; i++) {
			if (!node->u.rects[i].bptr)
				break;
			flag = flag << 1;
		}
	else
		for (i = 0; i < M_SIZE; i++) {
			if (!node->u.kids[i])
				break;
			__r_destroy_tree(node->u.kids[i]);
		}
	free(node);
}

/* free the memory associated with an rtree. */
void pcb_r_destroy_tree(pcb_rtree_t **rtree)
{
	__r_destroy_tree((*rtree)->root);
	free(*rtree);
	*rtree = NULL;
}

static void pcb_r_free_tree_data_(struct rtree_node *node, void (*free)(void *ptr))
{
	int i;

	if (node->flags.is_leaf) {
		for (i = 0; i < M_SIZE; i++) {
			if (!node->u.rects[i].bptr)
				break;
			free((void *) node->u.rects[i].bptr);
			node->u.rects[i].bptr = NULL;
		}
	}
	else {
		for (i = 0; i < M_SIZE; i++) {
			if (!node->u.kids[i])
				break;
			pcb_r_free_tree_data_(node->u.kids[i], free);
		}
	}
}

void pcb_r_free_tree_data(pcb_rtree_t *rtree, void (*free)(void *ptr))
{
	pcb_r_free_tree_data_(rtree->root, free);
}

typedef struct {
	pcb_r_dir_t (*check_it) (const pcb_box_t * region, void *cl);
	pcb_r_dir_t (*found_it) (const pcb_box_t * box, void *cl);
	void *closure;
	int cancel;
} r_arg;

/* most of the auto-routing time is spent in this routine
 * so some careful thought has been given to maximizing the speed
 *
 */
int __r_search(struct rtree_node *node, const pcb_box_t * query, r_arg * arg)
{
	pcb_r_dir_t res;

	assert(node);
	/** assert that starting_region is well formed */
	assert(query->X1 <= query->X2 && query->Y1 <= query->Y2);
	assert(node->box.X1 < query->X2 && node->box.X2 > query->X1 && node->box.Y1 < query->Y2 && node->box.Y2 > query->Y1);
#ifdef SLOW_ASSERTS
	/** assert that node is well formed */
	assert(__r_node_is_good(node));
#endif
	/* the check for bounds is done before entry. This saves the overhead
	 * of building/destroying the stack frame for each bounds that fails
	 * to intersect, which is the most common condition.
	 */
	if (node->flags.is_leaf) {
		register int i;
		if (arg->found_it) {				/* test this once outside of loop */
			register int seen = 0;
			for (i = 0; node->u.rects[i].bptr; i++) {
				if ((node->u.rects[i].bounds.X1 < query->X2) &&
						(node->u.rects[i].bounds.X2 > query->X1) &&
						(node->u.rects[i].bounds.Y1 < query->Y2) &&
						(node->u.rects[i].bounds.Y2 > query->Y1)) {
					res = arg->found_it(node->u.rects[i].bptr, arg->closure);
					if (res == PCB_R_DIR_CANCEL) {
						arg->cancel = 1;
						return seen;
					}
					if (res != PCB_R_DIR_NOT_FOUND)
						seen++;
				}
			}
			return seen;
		}
		else {
			register int seen = 0;
			for (i = 0; node->u.rects[i].bptr; i++) {
				if ((node->u.rects[i].bounds.X1 < query->X2) &&
						(node->u.rects[i].bounds.X2 > query->X1) &&
						(node->u.rects[i].bounds.Y1 < query->Y2) && (node->u.rects[i].bounds.Y2 > query->Y1))
					seen++;
			}
			return seen;
		}
	}

	/* not a leaf, recurse on lower nodes */
	if (arg->check_it != NULL) {
		int seen = 0;
		struct rtree_node **n;
		for (n = &node->u.kids[0]; *n; n++) {
			if ((*n)->box.X1 >= query->X2 ||
					(*n)->box.X2 <= query->X1 ||
					(*n)->box.Y1 >= query->Y2 || (*n)->box.Y2 <= query->Y1)
				continue;
			res = arg->check_it(&(*n)->box, arg->closure);
			if (res == PCB_R_DIR_CANCEL) {
				arg->cancel = 1;
				return seen;
			}
			if (!res)
				continue;
			seen += __r_search(*n, query, arg);
			if (arg->cancel)
				break;
		}
		return seen;
	}
	else {
		int seen = 0;
		struct rtree_node **n;
		for (n = &node->u.kids[0]; *n; n++) {
			if ((*n)->box.X1 >= query->X2 || (*n)->box.X2 <= query->X1 || (*n)->box.Y1 >= query->Y2 || (*n)->box.Y2 <= query->Y1)
				continue;
			seen  += __r_search(*n, query, arg);
			if (arg->cancel)
				break;
		}
		return seen;
	}
}

/* Parameterized search in the rtree.
 * Sets num_found to the number of rectangles found.
 * calls found_rectangle for each intersection seen
 * and calls check_region with the current sub-region
 * to see whether deeper searching is desired
 * Returns how the search ended.
 */
pcb_r_dir_t
pcb_r_search(pcb_rtree_t * rtree, const pcb_box_t * query,
				 pcb_r_dir_t (*check_region) (const pcb_box_t * region, void *cl),
				 pcb_r_dir_t (*found_rectangle) (const pcb_box_t * box, void *cl), void *cl,
				 int *num_found)
{
	r_arg arg;
	int res = 0;

	arg.cancel = 0;

	if (!rtree || rtree->size < 1)
		goto ret;
	if (query) {
#ifdef SLOW_ASSERTS
		assert(__r_tree_is_good(rtree->root));
#endif
#ifdef DEBUG
		if (query->X2 <= query->X1 || query->Y2 <= query->Y1)
			goto ret;
#endif
		/* check this box. If it's not touched we're done here */
		if (rtree->root->box.X1 >= query->X2 ||
				rtree->root->box.X2 <= query->X1 || rtree->root->box.Y1 >= query->Y2 || rtree->root->box.Y2 <= query->Y1)
			goto ret;
		arg.check_it = check_region;
		arg.found_it = found_rectangle;
		arg.closure = cl;

		res = __r_search(rtree->root, query, &arg);
	}
	else {
		arg.check_it = check_region;
		arg.found_it = found_rectangle;
		arg.closure = cl;
		res = __r_search(rtree->root, &rtree->root->box, &arg);
	}

ret:;
	if (num_found != NULL)
		*num_found = res;
	if (arg.cancel)
		return PCB_R_DIR_CANCEL;
	if (res == 0)
		return PCB_R_DIR_NOT_FOUND;
	return PCB_R_DIR_FOUND_CONTINUE;
}

/*------ r_region_is_empty ------*/
static pcb_r_dir_t __r_region_is_empty_rect_in_reg(const pcb_box_t * box, void *cl)
{
	jmp_buf *envp = (jmp_buf *) cl;
	longjmp(*envp, 1);						/* found one! */
}

/* return 0 if there are any rectangles in the given region. */
int pcb_r_region_is_empty(pcb_rtree_t * rtree, const pcb_box_t * region)
{
	jmp_buf env;
	int r;

	if (setjmp(env))
		return 0;
	pcb_r_search(rtree, region, NULL, __r_region_is_empty_rect_in_reg, &env, &r);
	assert(r == 0);								/* otherwise longjmp would have been called */
	return 1;											/* no rectangles found */
}

struct centroid {
	float x, y, area;
};

/* split the node into two nodes putting clusters in each
 * use the k-means clustering algorithm
 */
struct rtree_node *find_clusters(struct rtree_node *node)
{
	float total_a, total_b;
	float a_X, a_Y, b_X, b_Y;
	pcb_bool belong[M_SIZE + 1];
	struct centroid center[M_SIZE + 1];
	int clust_a, clust_b, tries;
	int i, old_ax, old_ay, old_bx, old_by;
	struct rtree_node *new_node;
	pcb_box_t *b;

	for (i = 0; i < M_SIZE + 1; i++) {
		if (node->flags.is_leaf)
			b = &(node->u.rects[i].bounds);
		else
			b = &(node->u.kids[i]->box);
		center[i].x = 0.5 * (b->X1 + b->X2);
		center[i].y = 0.5 * (b->Y1 + b->Y2);
		/* adding 1 prevents zero area */
		center[i].area = 1. + (float) (b->X2 - b->X1) * (float) (b->Y2 - b->Y1);
	}
	/* starting 'A' cluster center */
	a_X = center[0].x;
	a_Y = center[0].y;
	/* starting 'B' cluster center */
	b_X = center[M_SIZE].x;
	b_Y = center[M_SIZE].y;
	/* don't allow the same cluster centers */
	if (b_X == a_X && b_Y == a_Y) {
		b_X += 10000;
		a_Y -= 10000;
	}
	for (tries = 0; tries < M_SIZE; tries++) {
		old_ax = (int) a_X;
		old_ay = (int) a_Y;
		old_bx = (int) b_X;
		old_by = (int) b_Y;
		clust_a = clust_b = 0;
		for (i = 0; i < M_SIZE + 1; i++) {
			float dist1, dist2;

			dist1 = PCB_SQUARE(a_X - center[i].x) + PCB_SQUARE(a_Y - center[i].y);
			dist2 = PCB_SQUARE(b_X - center[i].x) + PCB_SQUARE(b_Y - center[i].y);
			if (dist1 * (clust_a + M_SIZE / 2) < dist2 * (clust_b + M_SIZE / 2)) {
				belong[i] = pcb_true;
				clust_a++;
			}
			else {
				belong[i] = pcb_false;
				clust_b++;
			}
		}
		/* kludge to fix degenerate cases */
		if (clust_a == M_SIZE + 1)
			belong[M_SIZE / 2] = pcb_false;
		else if (clust_b == M_SIZE + 1)
			belong[M_SIZE / 2] = pcb_true;
		/* compute new center of gravity of clusters */
		total_a = total_b = 0;
		a_X = a_Y = b_X = b_Y = 0;
		for (i = 0; i < M_SIZE + 1; i++) {
			if (belong[i]) {
				a_X += center[i].x * center[i].area;
				a_Y += center[i].y * center[i].area;
				total_a += center[i].area;
			}
			else {
				b_X += center[i].x * center[i].area;
				b_Y += center[i].y * center[i].area;
				total_b += center[i].area;
			}
		}
		a_X /= total_a;
		a_Y /= total_a;
		b_X /= total_b;
		b_Y /= total_b;
		if (old_ax == (int) a_X && old_ay == (int) a_Y && old_bx == (int) b_X && old_by == (int) b_Y)
			break;
	}
	/* Now 'belong' has the partition map */
	new_node = (struct rtree_node *) calloc(1, sizeof(*new_node));
	new_node->parent = node->parent;
	new_node->flags.is_leaf = node->flags.is_leaf;
	clust_a = clust_b = 0;
	if (node->flags.is_leaf) {
		int flag, a_flag, b_flag;
		flag = a_flag = b_flag = 1;
		for (i = 0; i < M_SIZE + 1; i++) {
			if (belong[i]) {
				node->u.rects[clust_a++] = node->u.rects[i];
				a_flag <<= 1;
			}
			else {
				new_node->u.rects[clust_b++] = node->u.rects[i];
				b_flag <<= 1;
			}
			flag <<= 1;
		}
	}
	else {
		for (i = 0; i < M_SIZE + 1; i++) {
			if (belong[i])
				node->u.kids[clust_a++] = node->u.kids[i];
			else {
				node->u.kids[i]->parent = new_node;
				new_node->u.kids[clust_b++] = node->u.kids[i];
			}
		}
	}
	assert(clust_a != 0);
	assert(clust_b != 0);
	if (node->flags.is_leaf)
		for (; clust_a < M_SIZE + 1; clust_a++)
			node->u.rects[clust_a].bptr = NULL;
	else
		for (; clust_a < M_SIZE + 1; clust_a++)
			node->u.kids[clust_a] = NULL;
	adjust_bounds(node);
	sort_node(node);
	adjust_bounds(new_node);
	sort_node(new_node);
	return new_node;
}

/* split a node according to clusters
 */
static void split_node(struct rtree_node *node)
{
	int i;
	struct rtree_node *new_node;

	assert(node);
	assert(node->flags.is_leaf ? (void *) node->u.rects[M_SIZE].bptr : (void *) node->u.kids[M_SIZE]);
	new_node = find_clusters(node);
	if (node->parent == NULL) {		/* split root node */
		struct rtree_node *second;

		second = (struct rtree_node *) calloc(1, sizeof(*second));
		*second = *node;
		if (!second->flags.is_leaf)
			for (i = 0; i < M_SIZE; i++)
				if (second->u.kids[i])
					second->u.kids[i]->parent = second;
		node->flags.is_leaf = 0;
		node->flags.unused = 0;
		second->parent = new_node->parent = node;
		node->u.kids[0] = new_node;
		node->u.kids[1] = second;
		for (i = 2; i < M_SIZE + 1; i++)
			node->u.kids[i] = NULL;
		adjust_bounds(node);
		sort_node(node);
#ifdef SLOW_ASSERTS
		assert(__r_tree_is_good(node));
#endif
		return;
	}
	for (i = 0; i < M_SIZE; i++)
		if (!node->parent->u.kids[i])
			break;
	node->parent->u.kids[i] = new_node;
#ifdef SLOW_ASSERTS
	assert(__r_node_is_good(node));
	assert(__r_node_is_good(new_node));
#endif
	if (i < M_SIZE) {
#ifdef SLOW_ASSERTS
		assert(__r_node_is_good(node->parent));
#endif
		sort_node(node->parent);
		return;
	}
	split_node(node->parent);
}

static inline int contained(struct rtree_node *node, const pcb_box_t * query)
{
	if (node->box.X1 > query->X1 || node->box.X2 < query->X2 || node->box.Y1 > query->Y1 || node->box.Y2 < query->Y2)
		return 0;
	return 1;
}


static inline double penalty(struct rtree_node *node, const pcb_box_t * query)
{
	double score;

	/* Compute the area penalty for inserting here and return.
	 * The penalty is the increase in area necessary
	 * to include the query->
	 */
	score = (MAX(node->box.X2, query->X2) - MIN(node->box.X1, query->X1));
	score *= (MAX(node->box.Y2, query->Y2) - MIN(node->box.Y1, query->Y1));
	score -= (double) (node->box.X2 - node->box.X1) * (double) (node->box.Y2 - node->box.Y1);
	return score;
}

static void __r_insert_node(struct rtree_node *node, const pcb_box_t * query, pcb_bool force)
{

#ifdef SLOW_ASSERTS
	assert(__r_node_is_good(node));
#endif
	/* Ok, at this point we must already enclose the query or we're forcing
	 * this node to expand to enclose it, so if we're a leaf, simply store
	 * the query here
	 */

	if (node->flags.is_leaf) {
		register int i;
		for (i = 0; i < M_SIZE; i++)
			if (!node->u.rects[i].bptr)
				break;

		/* the node always has an extra space available */
		node->u.rects[i].bptr = query;
		node->u.rects[i].bounds = *query;
		/* first entry in node determines initial bounding box */
		if (i == 0)
			node->box = *query;
		else if (force) {
			PCB_MAKE_MIN(node->box.X1, query->X1);
			PCB_MAKE_MAX(node->box.X2, query->X2);
			PCB_MAKE_MIN(node->box.Y1, query->Y1);
			PCB_MAKE_MAX(node->box.Y2, query->Y2);
		}
		if (i < M_SIZE) {
			sort_node(node);
			return;
		}
		/* we must split the node */
		split_node(node);
		return;
	}
	else {
		int i;
		struct rtree_node *best_node;
		double score, best_score;

		if (force) {
			PCB_MAKE_MIN(node->box.X1, query->X1);
			PCB_MAKE_MAX(node->box.X2, query->X2);
			PCB_MAKE_MIN(node->box.Y1, query->Y1);
			PCB_MAKE_MAX(node->box.Y2, query->Y2);
		}

		/* this node encloses it, but it's not a leaf, so descend the tree */

		/* First check if any children actually encloses it */
		assert(node->u.kids[0]);
		for (i = 0; i < M_SIZE; i++) {
			if (!node->u.kids[i])
				break;
			if (contained(node->u.kids[i], query)) {
				__r_insert_node(node->u.kids[i], query, pcb_false);
				sort_node(node);
				return;
			}
		}

		/* see if there is room for a new leaf node */
		if (node->u.kids[0]->flags.is_leaf && i < M_SIZE) {
			struct rtree_node *new_node;
			new_node = (struct rtree_node *) calloc(1, sizeof(*new_node));
			new_node->parent = node;
			new_node->flags.is_leaf = pcb_true;
			node->u.kids[i] = new_node;
			new_node->u.rects[0].bptr = query;
			new_node->u.rects[0].bounds = *query;
			new_node->box = *query;
			sort_node(node);
			return;
		}

		/* Ok, so we're still here - look for the best child to push it into */
		best_score = penalty(node->u.kids[0], query);
		best_node = node->u.kids[0];
		for (i = 1; i < M_SIZE; i++) {
			if (!node->u.kids[i])
				break;
			score = penalty(node->u.kids[i], query);
			if (score < best_score) {
				best_score = score;
				best_node = node->u.kids[i];
			}
		}
		__r_insert_node(best_node, query, pcb_true);
		sort_node(node);
		return;
	}
}

void pcb_r_insert_entry(pcb_rtree_t * rtree, const pcb_box_t * which)
{
	assert(which);
	assert(which->X1 <= which->X2);
	assert(which->Y1 <= which->Y2);
	/* recursively search the tree for the best leaf node */
	assert(rtree->root);
	__r_insert_node(rtree->root, which,
									rtree->root->box.X1 > which->X1
									|| rtree->root->box.X2 < which->X2 || rtree->root->box.Y1 > which->Y1 || rtree->root->box.Y2 < which->Y2);
	rtree->size++;
}

pcb_bool __r_delete_free_data(struct rtree_node *node, const pcb_box_t * query, void (*free_data)(void *data))
{
	int i, mask, a;

	/* the tree might be inconsistent during delete */
	if (query->X1 < node->box.X1 || query->Y1 < node->box.Y1 || query->X2 > node->box.X2 || query->Y2 > node->box.Y2)
		return pcb_false;
	if (!node->flags.is_leaf) {
		for (i = 0; i < M_SIZE; i++) {
			/* if this is us being removed, free and copy over */
			if (node->u.kids[i] == (struct rtree_node *) query) {
				free((void *) query);
				for (; i < M_SIZE; i++) {
					node->u.kids[i] = node->u.kids[i + 1];
					if (!node->u.kids[i])
						break;
				}
				/* nobody home here now ? */
				if (!node->u.kids[0]) {
					if (!node->parent) {
						/* wow, the root is empty! */
						node->flags.is_leaf = 1;
						/* changing type of node, be sure it's all zero */
						for (i = 1; i < M_SIZE + 1; i++)
							node->u.rects[i].bptr = NULL;
						return pcb_true;
					}
					return __r_delete_free_data(node->parent, &node->box, free_data);
				}
				else
					/* propagate boundary adjust upward */
					while (node) {
						adjust_bounds(node);
						node = node->parent;
					}
				return pcb_true;
			}
			if (node->u.kids[i]) {
				if (__r_delete_free_data(node->u.kids[i], query, free_data))
					return pcb_true;
			}
			else
				break;
		}
		return pcb_false;
	}
	/* leaf node here */
	mask = 0;
	a = 1;
	for (i = 0; i < M_SIZE; i++) {
#ifdef DELETE_BY_POINTER
		if (!node->u.rects[i].bptr || node->u.rects[i].bptr == query)
#else
		if (node->u.rects[i].bounds.X1 == query->X1 &&
				node->u.rects[i].bounds.X2 == query->X2 &&
				node->u.rects[i].bounds.Y1 == query->Y1 && node->u.rects[i].bounds.Y2 == query->Y2)
#endif
			break;
		mask |= a;
		a <<= 1;
	}
	if (!node->u.rects[i].bptr)
		return pcb_false;								/* not at this leaf */
	if (free_data != NULL) {
		free_data((void *) node->u.rects[i].bptr);
		node->u.rects[i].bptr = NULL;
	}
	/* remove the entry */
	for (; i < M_SIZE; i++) {
		node->u.rects[i] = node->u.rects[i + 1];
		if (!node->u.rects[i].bptr)
			break;
	}
	if (!node->u.rects[0].bptr) {
		if (node->parent)
			__r_delete_free_data(node->parent, &node->box, free_data);
		return pcb_true;
	}
	else
		/* propagate boundary adjustment upward */
		while (node) {
			adjust_bounds(node);
			node = node->parent;
		}
	return pcb_true;
}

pcb_bool __r_delete(struct rtree_node *node, const pcb_box_t * query)
{
	return __r_delete_free_data(node, query, NULL);
}

pcb_bool pcb_r_delete_entry_free_data(pcb_rtree_t * rtree, const pcb_box_t * box, void (*free_data)(void *d))
{
	pcb_bool r;

	assert(box);
	assert(rtree);
	r = __r_delete_free_data(rtree->root, box, free_data);
	if (r)
		rtree->size--;
#ifdef SLOW_ASSERTS
	assert(__r_tree_is_good(rtree->root));
#endif
	return r;
}

pcb_bool pcb_r_delete_entry(pcb_rtree_t * rtree, const pcb_box_t * box)
{
	return pcb_r_delete_entry_free_data(rtree, box, NULL);
}


static pcb_box_t *pcb_r_next_(pcb_rtree_it_t *it, struct rtree_node *curr)
{
	int n;

	/* as long as we have boxes ready, ignore the tree */
	if (it->num_ready > 0) {
/*		printf("rdy1 get [%d]: %p\n", it->num_ready-1, it->ready[it->num_ready-1]);*/
		return it->ready[--it->num_ready];
	}

	if (curr == NULL) {

		got_filled:;

		/* Next: get an element from the open list */
		if (it->used <= 0)
			return NULL;

		curr = it->open[--it->used];
/*		printf("open get [%d]: %p\n", it->used, curr);*/
	}

	if (curr->flags.is_leaf) {
		/* current is leaf: copy children to the ready list and return the first */
		for(it->num_ready = 0; it->num_ready < M_SIZE; it->num_ready++) {
			if (curr->u.rects[it->num_ready].bptr == NULL)
				break;
			it->ready[it->num_ready] = (pcb_box_t *)curr->u.rects[it->num_ready].bptr;
/*			printf("rdy  add [%d]: %p\n", it->num_ready, it->ready[it->num_ready]);*/
		}
		if (it->num_ready > 0) {
/*			printf("rdy2 get [%d]: %p\n", it->num_ready-1, it->ready[it->num_ready-1]);*/
			return it->ready[--it->num_ready];
		}
		
		/* empty leaf: ignore */
		curr = NULL;
		goto got_filled;
	}

	/* current is a level, add to the open list and retry until a leaf is found */
	for(n = 0; n < M_SIZE; n++) {
		if (curr->u.kids[n] != NULL) {
			if (it->used >= it->alloced) {
				it->alloced += 128;
				it->open = realloc(it->open, it->alloced * sizeof(struct rtree_node *));
			}
			it->open[it->used] = curr->u.kids[n];
/*			printf("open add [%d]: %p\n", it->used, it->open[it->used]);*/
			it->used++;
		}
	}
	goto got_filled;
}

pcb_box_t *pcb_r_first(pcb_rtree_t *tree, pcb_rtree_it_t *it)
{
	it->open = NULL;
	it->alloced = it->used = 0;
	it->num_ready = 0;
	if (tree == NULL)
		return NULL;
	return pcb_r_next_(it, tree->root);
}

pcb_box_t *pcb_r_next(pcb_rtree_it_t *it)
{
	return pcb_r_next_(it, NULL);
}

void pcb_r_end(pcb_rtree_it_t *it)
{
	free(it->open);
	it->open = NULL;
	it->alloced = it->used = 0;
}

#endif

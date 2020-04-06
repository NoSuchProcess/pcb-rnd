/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  BXL IO plugin - file format read, parser
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2020)
 *
 *  Algorithm based on BXL2text/SourceBuffer.cc by Erich S. Heinzle
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

/*#include "config.h"*/

#include <stdlib.h>
#include <string.h>
#include "bxl_decode.h"

/* Frees a whole subtree */
static void hnode_free(hnode_t *n)
{
	if (n->left != NULL)
		hnode_free(n->left);
	if (n->right != NULL)
		hnode_free(n->right);
	free(n);
}

/* Allocates a new node under parent (parent is NULL for root) */
static hnode_t *hnode_alloc(hnode_t *parent, int symbol)
{
	hnode_t *n = calloc(sizeof(hnode_t), 1);

	if (parent != NULL) {
		n->parent = parent;
		n->level = parent->level+1;
	}
	if (n->level > 7)
		n->symbol = symbol;
	else
		n->symbol = -1;
	return n;
}

static hnode_t *hnode_add_child(hnode_t *n, int symbol)
{
	hnode_t *ret;

	if (n->level < 7) {
		if (n->right != NULL) {
			ret = hnode_add_child(n->right, symbol);
			if (ret != NULL)
				return ret;
		}
		if (n->left != NULL) {
			ret = hnode_add_child(n->left, symbol);
			if (ret != NULL)
				return ret;
		}
		if (n->right == NULL) { /* fill from the right side */
			n->right = hnode_alloc(n, -1);
			return n->right;
		}
		if (n->left == NULL) {
			n->left = hnode_alloc(n, -1);
			return n->left;
		}
		return NULL;
	}

	if (n->right == NULL) {
		n->right = hnode_alloc(n, symbol);
		return n->right;
	}
	if (n->left == NULL) {
		n->left = hnode_alloc(n, symbol);
		return n->left;
	}

	return NULL; /* leaves are filled */
}

#define is_leaf(n) ((n)->level > 7)

#define sibling(parent, n) (((n) == (parent)->right) ? (parent)->left : (parent)->right)

#define inc_weight(n) (n)->weight++

#define need_swapping(n) \
	((n)->parent != NULL && (n)->parent->parent != NULL && (n)->weight > (n)->parent->weight)

static void hnode_swap(hnode_t *n1, hnode_t *n2, hnode_t *n3)
{
	if (n3 != NULL)
		n3->parent = n1;

	if (n1->right == n2)
		n1->right = n3;
	else if (n1->left == n2)
		n1->left = n3;
}


static void htree_free(htree_t *t)
{
	hnode_free(t->root);
	free(t);
}

static htree_t *htree_alloc(void)
{
	int leaf_count = 0;
	htree_t *t = calloc(sizeof(htree_t), 1);
	hnode_t *node;

	node = t->root = hnode_alloc(NULL, 0);

	/* fill levels */
	while(node != NULL) {
		node = hnode_add_child(t->root, leaf_count);
		if ((node != NULL) && (is_leaf(node)))
			leaf_count++;
	}
	return t;
}

static void htree_update(hnode_t *current)
{
	hnode_t *parent, *grand_parent, *parent_sibling;
	if ((current == NULL) || (!need_swapping(current)))
		return;

	parent = current->parent;
	grand_parent = parent->parent;
	parent_sibling = sibling(grand_parent, parent);

	hnode_swap(grand_parent, parent,  current);
	hnode_swap(grand_parent, parent_sibling, parent);
	hnode_swap(parent, current, parent_sibling);

	parent->weight = parent->right->weight + parent->left->weight;
	grand_parent->weight = current->weight + parent->weight;

	htree_update(parent);
	htree_update(grand_parent);
	htree_update(current);
}

static int next_bit(hdecode_t *ctx)
{
	if (ctx->bitpos < 0) {
		ctx->bitpos = 7;
		return -1; /* read the next */
	}
	return ctx->chr & (1 << (ctx->bitpos--));
}

static void decode_run(hdecode_t *ctx)
{
	for(;;) {
		while(!is_leaf(ctx->node)) {
			int bit = next_bit(ctx);
			if (bit < 0)
				return; /* wait for the next byte */
			if (bit)
				ctx->node = ctx->node->left;
			else
				ctx->node = ctx->node->right;
		}

		/* produce the next output byte */
		ctx->out[ctx->out_len++] = ctx->node->symbol;
		inc_weight(ctx->node);
		htree_update(ctx->node);
		ctx->node = ctx->tree->root;
	}
}

int pcb_bxl_decode_char(hdecode_t *ctx, int inchr)
{
	ctx->out_len = 0;
	ctx->chr = inchr;
	decode_run(ctx);
	return ctx->out_len;
}

void pcb_bxl_decode_init(hdecode_t *ctx)
{
	memset(ctx, 0, sizeof(hdecode_t));
	ctx->tree = htree_alloc();
	ctx->node = ctx->tree->root;
	decode_run(ctx);
}

void pcb_bxl_decode_uninit(hdecode_t *ctx)
{
	htree_free(ctx->tree);
	ctx->tree = NULL;
}


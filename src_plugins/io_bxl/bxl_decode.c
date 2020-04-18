/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  BXL IO plugin - file format read, binary decoder
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

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "bxl_decode.h"

/* Allocates a new node under parent (parent is NULL for root) */
static hnode_t *hnode_alloc(htree_t *tree, hnode_t *parent, int symbol)
{
	hnode_t *n;

	assert(tree->pool_used < sizeof(tree->pool) / sizeof(tree->pool[0]));

	n = &tree->pool[tree->pool_used++];

	n->weight = 0;

	if (parent != NULL) {
		n->parent = parent;
		n->level = parent->level+1;
	}
	else {
		n->parent = NULL;
		n->level = 0;
	}

	if (n->level > 7)
		n->symbol = symbol;
	else
		n->symbol = -1;
	return n;
}

static hnode_t *hnode_add_child(htree_t *tree, hnode_t *n, int symbol)
{
	hnode_t *ret;

	if (n->level < 7) {
		if (n->right != NULL) {
			ret = hnode_add_child(tree, n->right, symbol);
			if (ret != NULL)
				return ret;
		}
		if (n->left != NULL) {
			ret = hnode_add_child(tree, n->left, symbol);
			if (ret != NULL)
				return ret;
		}
		if (n->right == NULL) { /* fill from the right side */
			n->right = hnode_alloc(tree, n, -1);
			return n->right;
		}
		if (n->left == NULL) {
			n->left = hnode_alloc(tree, n, -1);
			return n->left;
		}
		return NULL;
	}

	if (n->right == NULL) {
		n->right = hnode_alloc(tree, n, symbol);
		return n->right;
	}
	if (n->left == NULL) {
		n->left = hnode_alloc(tree, n, symbol);
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

static htree_t *htree_init(htree_t *t)
{
	int leaf_count = 0;
	hnode_t *node;

	t->pool_used = 0;
	node = t->root = hnode_alloc(t, NULL, 0);

	/* fill levels */
	while(node != NULL) {
		node = hnode_add_child(t, t->root, leaf_count);
		if ((node != NULL) && (is_leaf(node))) {
			t->nodeList[leaf_count] = node;
			leaf_count++;
		}
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
		if (ctx->opos < ctx->plain_len)
			ctx->out[ctx->out_len++] = ctx->node->symbol;
		ctx->opos++;
		inc_weight(ctx->node);
		htree_update(ctx->node);
		ctx->node = ctx->tree.root;
	}
}

/* Swap the bits in a byte */
static unsigned long int bitswap(unsigned int i)
{
	int n;
	unsigned long int o = 0;

	for(n = 0; n < 8; n++) {
		o <<= 1;
		o |= (i & 1);
		i >>= 1;
	}

	return o;
}

int pcb_bxl_decode_char(hdecode_t *ctx, int inchr)
{
	if (ctx->hdr_pos < 4) {
		/* Read the header; the header is A B C D, where A is the LSB and
		   D is the MSB of uncompressed file length in bytes; each header
		   byte is binary mirrored for some very good but unfortunately
		   undocumented reason.
		   
		   Yet another twist: the input binary file often (always?) ends in
		   \r\n. Thus the plain text output must be truncated at the length
		   extracted from the header, else there would be noise causing syntax
		   error at the end of the decoded file.
		*/
		ctx->hdr[ctx->hdr_pos] = inchr;
		ctx->hdr_pos++;
		if (ctx->hdr_pos == 4)
			ctx->plain_len = bitswap(ctx->hdr[3]) << 24 | bitswap(ctx->hdr[2]) << 16 | bitswap(ctx->hdr[1]) << 8 | bitswap(ctx->hdr[0]);
		return 0;
	}

	if (ctx->opos >= ctx->plain_len)
		return 0;

	ctx->out_len = 0;
	ctx->chr = inchr;
	decode_run(ctx);
	return ctx->out_len;
}

void pcb_bxl_decode_init(hdecode_t *ctx)
{
	memset(ctx, 0, sizeof(hdecode_t));
	htree_init(&ctx->tree);
	ctx->node = ctx->tree.root;
	decode_run(ctx);
}

static void append(hdecode_t *ctx, int bitval)
{
	ctx->chr <<= 1;
	ctx->chr |= bitval;
	ctx->bitpos++;
	if (ctx->bitpos == 8) {
		ctx->out[ctx->out_len++] = ctx->chr;
		ctx->chr = ctx->bitpos = 0;
	}
}

int pcb_bxl_encode_char(hdecode_t *ctx, int inchr)
{
	int depth = 0;
	int encoded[257]; /* we need to account for very asymmetric tree topologies */
	hnode_t *node = ctx->tree.nodeList[inchr];

	ctx->plain_len++;
	ctx->out_len = 0;

	/* output 4 dummy bytes that will be the length "header" */
	if (!ctx->after_first_bit)
		ctx->out_len = 4;

	inc_weight(node);

	while (node->level != 0) {
		if (node == node->parent->left)
			encoded[256-depth] = 1; /* left of parent */
		else
			encoded[256-depth] = 0; /* right of parent */
		depth++;
		node = node->parent;
	}

	for(; depth > 0; depth--) {
		if (ctx->after_first_bit) {
			if (encoded[257-depth])
				append(ctx, 1);
			else
				append(ctx, 0);
		}
		ctx->after_first_bit = 1;
	}

	htree_update(ctx->tree.nodeList[inchr]);
	return ctx->out_len;
}

int pcb_bxl_encode_eof(hdecode_t *ctx)
{
	ctx->out_len = 0;
	if (ctx->bitpos == 0)
		return 0;

	/* pad the last byte */
	while(ctx->out_len == 0)
		append(ctx, 0);

	return 1;
}


void pcb_bxl_encode_init(hdecode_t *ctx)
{
	pcb_bxl_decode_init(ctx);
	ctx->bitpos = 0;
}


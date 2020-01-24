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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
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
 */

/* Eagle binary tree store */

#include "config.h"
#include <assert.h>
#include <stdlib.h>
#include <genht/hash.h>
#include "egb_tree.h"
#include <librnd/core/compat_misc.h>

egb_node_t *egb_node_alloc(int id, const char *id_name)
{
	egb_node_t *nd = calloc(sizeof(egb_node_t), 1);
	nd->id = id;
	nd->id_name = id_name;
	return nd;
}

egb_node_t *egb_node_append(egb_node_t *parent, egb_node_t *node)
{
	node->parent = parent;
	node->next = NULL;

	if (parent->last_child == NULL) {
		parent->last_child = node;
		parent->first_child = node;
	}
	else {
		parent->last_child->next = node;
		parent->last_child = node;
	}
	return node;
}


void egb_node_prop_set(egb_node_t *node, const char *key, const char *val)
{
	if (node->props.table == NULL)
		htss_init(&node->props, strhash, strkeyeq);
	htss_set(&node->props, pcb_strdup(key), pcb_strdup(val));
}

char *egb_node_prop_get(egb_node_t *node, const char *key)
{
	if (node->props.table == NULL)
		return NULL;
	return htss_get(&node->props, key);
}

void egb_node_free(egb_node_t *node)
{
	egb_node_t *n;

	for(n = node->first_child; n != NULL; n = n->next)
		egb_node_free(n);

	if (node->props.table != NULL) {
		htss_entry_t *e;
		for (e = htss_first(&node->props); e; e = htss_next(&node->props, e)) {
			free(e->key);
			free(e->value);
		}
		htss_uninit(&node->props);
	}
	free(node);
}

egb_node_t *egb_node_unlink(egb_node_t *parent, egb_node_t *prev, egb_node_t *node)
{
	if (parent->first_child == node) {
		assert(prev == NULL);
		parent->first_child = node->next;
	}
	else {
		assert(prev != NULL);
		assert(prev->next == node);
	}

	if (parent->last_child == node)
		parent->last_child = prev;

	if (prev != NULL)
		prev->next = node->next;

	node->parent = NULL;
	node->next = NULL;
	return node;
}


static char inds[] = "                                                               ";
static void egb_dump_(FILE *f, int ind, egb_node_t *node)
{
	htss_entry_t *e;
	int i;
	egb_node_t *n;

	if (ind >= sizeof(inds)-1)
		ind = sizeof(inds)-1;
	inds[ind] = '\0';
	fprintf(f, "%s%s/%04x [", inds, node->id_name, node->id);
	inds[ind] = ' ';

	for (e = htss_first(&node->props), i = 0; e; e = htss_next(&node->props, e), i++)
		fprintf(f, "%s%s=\"%s\"", (i > 0 ? " " : ""), e->key, e->value);
	fprintf(f, "]\n");

	for(n = node->first_child; n != NULL; n = n->next)
		egb_dump_(f, ind+1, n);
}

void egb_dump(FILE *f, egb_node_t *node)
{
	egb_dump_(f, 0, node);
}


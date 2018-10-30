/*
 *														COPYRIGHT
 *
 *	pcb-rnd, interactive printed circuit board design
 *	Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* Eagle binary tree store */

#ifndef PCB_EGB_TREE_H
#define PCB_EGB_TREE_H
#include <stdio.h>
#include <genht/htss.h>

typedef struct egb_node_s egb_node_t;

struct egb_node_s {
	int id;
	const char *id_name;
	htss_t props;
	egb_node_t *parent;
	egb_node_t *next;
	egb_node_t *first_child, *last_child;
	void *user_data;
};


/* Allocate a new floating node */
egb_node_t *egb_node_alloc(int id, const char *id_name);

/* Link a floating node into the tree, appending it as the last
   child of parent. Returns the node. */
egb_node_t *egb_node_append(egb_node_t *parent, egb_node_t *node);

/* Unlink a node from its parent, making the node a floating node.
   Returns the node */
egb_node_t *egb_node_unlink(egb_node_t *parent, egb_node_t *prev, egb_node_t *node);

/* Free a subtree (without unlinking) */
void egb_node_free(egb_node_t *node);

/* Set a named property of a node to val */
void egb_node_prop_set(egb_node_t *node, const char *key, const char *val);

/* Return the value of a named property of a node */
char *egb_node_prop_get(egb_node_t *node, const char *key);

/* Print the indented tree in text to f */
void egb_dump(FILE *f, egb_node_t *node);

#endif

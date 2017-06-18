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
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *	Contact addresses for paper mail and Email:
 *	Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *	Thomas.Nau@rz.uni-ulm.de
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
};


egb_node_t *egb_node_alloc(int id, const char *id_name);
egb_node_t *egb_node_append(egb_node_t *parent, egb_node_t *node);
void egb_node_free(egb_node_t *node);

void egb_node_prop_set(egb_node_t *node, const char *key, const char *val);
char *egb_node_prop_get(egb_node_t *node, const char *key);

void egb_dump(FILE *f, egb_node_t *node);

#endif

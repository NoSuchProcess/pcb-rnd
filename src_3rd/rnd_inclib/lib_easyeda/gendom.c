/*
    gendom - generic DOM
    Copyright (C) 2023 Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include <stdlib.h>
#include <string.h>
#include <genht/hash.h>
#include "gendom.h"

gdom_node_t *gdom_alloc(long name, gdom_node_type_t type)
{
	gdom_node_t *nd = calloc(sizeof(gdom_node_t), 1);

	nd->type = type;
	nd->name = name;
	if (type == GDOM_HASH)
		htip_init(&nd->value.hash, longhash, longkeyeq);

	return nd;
}

void gdom_free(gdom_node_t *nd)
{
	long n;
	htip_entry_t *e;

	switch(nd->type) {
		case GDOM_ARRAY:
			for(n = 0; n < nd->value.array.used; n++)
				gdom_free(nd->value.array.child[n]);
			free(nd->value.array.child);
			nd->value.array.child = NULL;
			nd->value.array.used = nd->value.array.alloced = 0; /* for easier detection of use-after-free */
			break;

		case GDOM_HASH:
			for(e = htip_first(&nd->value.hash); e != NULL; e = htip_next(&nd->value.hash, e))
				gdom_free(e->value);
			htip_uninit(&nd->value.hash);
			break;

		case GDOM_STRING:
			free(nd->value.str);
			nd->value.str = NULL; /* for easier detection of use-after-free */
			break;

		case GDOM_DOUBLE:
		case GDOM_LONG:
			break;

	}

	free(nd->name_str);
	free(nd);
}

int gdom_hash_put(gdom_node_t *parent, gdom_node_t *child)
{
	if (parent->type != GDOM_HASH)
		return -1;
	if (child->parent != NULL)
		return -2;
	if (htip_has(&parent->value.hash, child->name))
		return -3;

	htip_set(&parent->value.hash, child->name, child);
	child->parent = parent;
	return 0;
}

gdom_node_t *gdom_hash_get(gdom_node_t *parent, long name)
{
	if (parent->type != GDOM_HASH)
		return NULL;

	return htip_get(&parent->value.hash, name);
}


int gdom_append(gdom_node_t *parent, gdom_node_t *child)
{
	if (parent->type == GDOM_HASH)
		return gdom_hash_put(parent, child);
	if (parent->type == GDOM_ARRAY)
		return gdom_array_append(parent, child);
	return -4;
}


int gdom_array_append(gdom_node_t *parent, gdom_node_t *child)
{
	if (parent->type != GDOM_ARRAY)
		return -1;
	if (child->parent != NULL)
		return -2;

	/* grow the array */
	if (parent->value.array.used >= parent->value.array.alloced) {
		if (parent->value.array.alloced == 0)
			parent->value.array.alloced = 16;
		else if (parent->value.array.alloced < 1024)
			parent->value.array.alloced *= 2;
		else
			parent->value.array.alloced += 512;
		parent->value.array.child = realloc(parent->value.array.child, parent->value.array.alloced * sizeof(void *));
	}

	parent->value.array.child[parent->value.array.used++] = child;
	child->parent = parent;

	return 0;
}

static void gdom_ind(FILE *f, int ind)
{
	long n;

	for(n = 0; n < ind; n++)
		fputc(' ', f);
}

void gdom_dump(FILE *f, gdom_node_t *nd, int ind, const char *(*name2str)(long name))
{
	long n;
	htip_entry_t *e;

	gdom_ind(f, ind);

	switch(nd->type) {
		case GDOM_ARRAY:
			fprintf(f, "array '%s' {\n", name2str(nd->name));
			for(n = 0; n < nd->value.array.used; n++)
				gdom_dump(f, nd->value.array.child[n], ind+1, name2str);
			gdom_ind(f, ind);
			fprintf(f, "}\n");
			break;

		case GDOM_HASH:
			fprintf(f, "hash '%s' {\n", name2str(nd->name));
			for(e = htip_first(&nd->value.hash); e != NULL; e = htip_next(&nd->value.hash, e))
				gdom_dump(f, e->value, ind+1, name2str);
			gdom_ind(f, ind);
			fprintf(f, "}\n");
			break;

		case GDOM_STRING:
			fprintf(f, "string '%s'='%s'\n", name2str(nd->name), nd->value.str);
			break;

		case GDOM_DOUBLE:
			fprintf(f, "double '%s'=%f\n", name2str(nd->name), nd->value.dbl);
			break;

		case GDOM_LONG:
			fprintf(f, "long '%s'=%ld\n", name2str(nd->name), nd->value.lng);
			break;
	}
}

char *gdom_strdup(const char *s)
{
	int l = strlen(s);
	char *o = malloc(l+1);
	if (o != NULL)
		memcpy(o, s, l+1);
	return o;
}

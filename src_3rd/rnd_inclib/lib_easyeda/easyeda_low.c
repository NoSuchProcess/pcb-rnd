/*
    easyeda file format parser
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
#include "gendom_json.h"


typedef struct str_tab_s str_tab_t;

struct str_tab_s {
	long name;
	gdom_node_type_t type;
};

/* Parse a string separated by sep into one node per field, appended to parent.
   If parent is a hash, expect a tab[] with the same number of fields,
   terminated with {-1}. If parent is an array, use only tab[0] and read
   as many fields as available in str. */
static int parse_str_by_tab(char *str, gdom_node_t *parent, const str_tab_t *tab, char sep)
{
	char *curr, *next;
	gdom_node_t *nd;

	for(curr = str; (curr != NULL) && (tab->name != -1); curr = next) {
		next = strchr(curr, sep);
		if (next != NULL) {
			*next = '\0';
			next++;
		}

		nd = gdom_alloc(tab->name, tab->type);
		switch(tab->type) {
			case GDOM_STRING: nd->value.str = gdom_strdup(curr); break;
			case GDOM_LONG:   nd->value.lng = strtol(curr, NULL, 10); break;
			case GDOM_DOUBLE: nd->value.dbl = strtod(curr, NULL); break;
			default:
				abort();
		}

		gdom_append(parent, nd);
		if (parent->type == GDOM_HASH)
			tab++;
	}

	return 0;
}

/* replace dst with src, gdom_free() old dst */
static void replace_node(gdom_node_t *dst, gdom_node_t *src)
{
	gdom_node_t save, *orig_parent = dst->parent;
	long lineno = src->lineno > 0 ? src->lineno : dst->lineno;
	long col = src->col > 0 ? src->lineno : dst->col; /* prefer src's if available, fall back to dst's */

	/* can't replace with a differently named node within a hash */
	if (dst->parent->type == GDOM_HASH) {
		if (dst->name != src->name)
			abort();
	}

	memcpy(&save, dst, sizeof(gdom_node_t));
	memcpy(dst, src, sizeof(gdom_node_t));
	memcpy(src, &save, sizeof(gdom_node_t));
	gdom_free(src);

	/* replacing in-place means we need to preserve the original parent node link */
	dst->parent = orig_parent;
	dst->lineno = lineno;
	dst->col = col;
}


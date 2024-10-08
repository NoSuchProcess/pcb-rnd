/*
    gendom - generic DOM - json glue
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

#include "gendom_json.h"
#include <libnanojson/nanojson.h>

#define GDOM_ALLOC_POST(nd) \
do { \
	nd->lineno = ctx.lineno+1; \
	nd->col = ctx.col+1; \
	nd->name_str = name_str; \
	name_str = NULL; \
} while(0)


gdom_node_t *gdom_json_parse_any(void *uctx, int (*getchr)(void *uctx), long (*str2name)(void *uctx, gdom_node_t *parent, const char *str))
{
	njson_ctx_t ctx = {0};
	long line = 1, col = 1;
	int chr, kw;
	gdom_node_t *curr = NULL, *root = NULL, *nd;
	long name = -1;
	char *name_str = NULL;

	for(;;) {
		njson_ev_t ev;

		chr = getchr(uctx);

		/* count location */
		if (chr == '\n') {
			line++;
			col = 1;
		}
		else
			col++;

		ev = njson_push(&ctx, chr);
		switch(ev) {
			case NJSON_EV_OBJECT_BEGIN:
				nd = gdom_alloc(name, (str2name == NULL) ? GDOM_ARRAY : GDOM_HASH);
				GDOM_ALLOC_POST(nd);

				if (root == NULL) {
					root = nd;
				}
				else if (gdom_append(curr, nd) != 0) {
					fprintf(stderr, "Failed to append gdom node (1)\n");
					goto error;
				}
				curr = nd;
				name = -1;
				break;

			case NJSON_EV_ARRAY_BEGIN:
				nd = gdom_alloc(name, GDOM_ARRAY);
				GDOM_ALLOC_POST(nd);

				if (root == NULL) {
					root = nd;
				}
				else if (gdom_append(curr, nd) != 0) {
					fprintf(stderr, "Failed to append gdom node (2)\n");
					goto error;
				}
				curr = nd;
				name = -1;
				break;


			case NJSON_EV_OBJECT_END:
			case NJSON_EV_ARRAY_END:
				curr = curr->parent;
				break;

			case NJSON_EV_NAME:
				if (str2name != NULL)
					name = str2name(uctx, curr, ctx.value.string);

				if ((str2name == NULL) || (name == GDOM_CUSTOM_NAME)) {
					if (name_str != NULL)
						free(name_str);
					name_str = gdom_strdup(ctx.value.string);
				}
				break;

			case NJSON_EV_STRING:
				nd = gdom_alloc(name, GDOM_STRING);
				GDOM_ALLOC_POST(nd);

				if (curr == NULL) { fprintf(stderr, "error 'root is not an object or array' at %ld:%ld\n", line, col); goto error; }
				if (gdom_append(curr, nd) != 0) {
					fprintf(stderr, "Failed to append gdom node (3)\n");
					goto error;
				}
				name = -1;
				nd->value.str = gdom_strdup(ctx.value.string);
				break;

			case NJSON_EV_NUMBER:
				nd = gdom_alloc(name, GDOM_DOUBLE);
				GDOM_ALLOC_POST(nd);

				if (curr == NULL) { fprintf(stderr, "error 'root is not an object or array' at %ld:%ld\n", line, col); goto error; }
				if (gdom_append(curr, nd) != 0) {
					fprintf(stderr, "Failed to append gdom node (4)\n");
					goto error;
				}
				name = -1;
				nd->value.dbl = ctx.value.number;
				break;

			case NJSON_EV_TRUE:
				kw = 1;
				goto append_kw;
			case NJSON_EV_FALSE:
				kw = 0;
				goto append_kw;
			case NJSON_EV_NULL:
				kw = -1;
				append_kw:;
				nd = gdom_alloc(name, GDOM_DOUBLE);
				GDOM_ALLOC_POST(nd);

				if (curr == NULL) { fprintf(stderr, "error 'root is not an object or array' at %ld:%ld\n", line, col); goto error; }
				if (gdom_append(curr, nd) != 0) {
					fprintf(stderr, "Failed to append gdom node (5)\n");
					goto error;
				}
				name = -1;
				nd->value.dbl = kw;
				break;

			case NJSON_EV_error:  fprintf(stderr, "error '%s' at %ld:%ld\n", ctx.error, line, col); goto error;
			case NJSON_EV_eof:    njson_uninit(&ctx); free(name_str); return root;
			case NJSON_EV_more:   break;
		}
	}

	error:;
	njson_uninit(&ctx);
	if (root != NULL)
		gdom_free(root);
	free(name_str);
	return NULL;
}

gdom_node_t *gdom_json_parse(FILE *f, long (*str2name)(void *uctx, gdom_node_t *parent, const char *str))
{
	return gdom_json_parse_any(f, (int (*)(void *))fgetc, str2name);
}

static int gdom_str_getc(void *uctx)
{
	const char **str = uctx;
	int res = **str;

	if (res != 0)
		(*str)++;
	else
		res = EOF;

	return res;
}

gdom_node_t *gdom_json_parse_str(const char *str, long (*str2name)(void *uctx, gdom_node_t *parent, const char *str))
{
	return gdom_json_parse_any(&str, gdom_str_getc, str2name);
}


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


gdom_node_t *gdom_json_parse(FILE *f, long (*str2name)(const char *str))
{
	njson_ctx_t ctx = {0};
	long line = 1, col = 1;
	int chr, kw;
	gdom_node_t *curr = NULL, *root = NULL, *nd;
	long name = -1;

	for(;;) {
		njson_ev_t ev;

		chr = fgetc(f);

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
				nd = gdom_alloc(name, GDOM_HASH);
				nd->lineno = ctx.lineno+1;
				nd->col = ctx.col+1;

				if (root == NULL) {
					root = nd;
				}
				else if (gdom_append(curr, nd) != 0) {
					fprintf(stderr, "Failed to append gdom node\n");
					goto error;
				}
				curr = nd;
				name = -1;
				break;

			case NJSON_EV_ARRAY_BEGIN:
				nd = gdom_alloc(name, GDOM_ARRAY);
				nd->lineno = ctx.lineno+1;
				nd->col = ctx.col+1;

				if (root == NULL) {
					root = nd;
				}
				else if (gdom_append(curr, nd) != 0) {
					fprintf(stderr, "Failed to append gdom node\n");
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
				name = str2name(ctx.value.string);
				break;

			case NJSON_EV_STRING:
				nd = gdom_alloc(name, GDOM_STRING);
				nd->lineno = ctx.lineno+1;
				nd->col = ctx.col+1;

				if (curr == NULL) { fprintf(stderr, "error 'root isnot an object or array' at %ld:%ld\n", line, col); goto error; }
				if (gdom_append(curr, nd) != 0) {
					fprintf(stderr, "Failed to append gdom node\n");
					goto error;
				}
				name = -1;
				nd->value.str = gdom_strdup(ctx.value.string);
				break;

			case NJSON_EV_NUMBER:
				nd = gdom_alloc(name, GDOM_DOUBLE);
				nd->lineno = ctx.lineno+1;
				nd->col = ctx.col+1;

				if (curr == NULL) { fprintf(stderr, "error 'root isnot an object or array' at %ld:%ld\n", line, col); goto error; }
				if (gdom_append(curr, nd) != 0) {
					fprintf(stderr, "Failed to append gdom node\n");
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
				nd->lineno = ctx.lineno+1;
				nd->col = ctx.col+1;

				if (curr == NULL) { fprintf(stderr, "error 'root isnot an object or array' at %ld:%ld\n", line, col); goto error; }
				if (gdom_append(curr, nd) != 0) {
					fprintf(stderr, "Failed to append gdom node\n");
					goto error;
				}
				name = -1;
				nd->value.dbl = kw;
				break;

			case NJSON_EV_error:  fprintf(stderr, "error '%s' at %ld:%ld\n", ctx.error, line, col); goto error;
			case NJSON_EV_eof:    njson_uninit(&ctx); return root;
			case NJSON_EV_more:   break;
		}
	}

	error:;
	njson_uninit(&ctx);
	gdom_free(root);
	return NULL;
}

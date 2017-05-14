/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This module, io_mentor_cell, was written and is Copyright (C) 2016 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <qparse/qparse.h>

#include "board.h"
#include "conf_core.h"
#include "plug_io.h"
#include "error.h"

typedef struct node_s node_t;

struct node_s {
	char **argv;
	int argc;
	int level;
	node_t *parent, *next;
	node_t *first_child, *last_child;
};

typedef struct {
	pcb_board_t *pcb;
	char *unit;
	node_t *root;
} hkp_ctx_t;

static void dump(node_t *nd)
{
	int n;
	for(n = 0; n < nd->level; n++)
		printf(" ");
	if (nd->argc > 0) {
		printf("%s", nd->argv[0]);
		for(n = 1; n < nd->argc; n++)
			printf("|%s", nd->argv[n]);
	}
	else
		printf("<empty>");
	printf("\n");
	for(nd = nd->first_child; nd != NULL; nd = nd->next)
		dump(nd);
}

static void destroy(node_t *nd)
{
	node_t *n, *next;

	qparse_free(nd->argc, &nd->argv);

	for(n = nd->first_child; n != NULL; n = next) {
		next = n->next;
		destroy(n);
	}

	free(nd);
}


int io_mentor_cell_read_pcb(pcb_plug_io_t *pctx, pcb_board_t *pcb, const char *fn, conf_role_t settings_dest)
{
	hkp_ctx_t ctx;
	char *s, **argv;
	int argc;
	FILE *f;
	char line[1024];
	node_t *curr = NULL, *nd, *last;

	f = fopen(fn, "r");
	if (f == NULL)
		goto err;

	ctx.pcb = pcb;
	ctx.unit = "mm";
	curr = ctx.root = calloc(sizeof(node_t), 1);

	while(fgets(line, sizeof(line), f) != NULL) {
		int level = 0;
		s = line;
		while(isspace(*s)) s++;
		while(*s == '.') { s++; level++; };
		if (level == 0)
			continue;
		nd = calloc(sizeof(node_t), 1);
		nd->argc = qparse(s, &nd->argv);
		nd->level = level;

		if (level == curr->level) { /* sibling */
			sibling:;
			nd->parent = curr->parent;
			curr->next = nd;
			nd->parent->last_child = nd;
		}
		else if (level == curr->level+1) { /* first child */
			curr->first_child = curr->last_child = nd;
			nd->parent = curr;
		}
		else if (level < curr->level) { /* step back to a previous level */
			while(level < curr->level) curr = curr->parent;
			goto sibling;
		}
		curr = nd;
	}


	err:;
	dump(ctx.root);
	destroy(ctx.root);
	fclose(f);
	return -1;
}

int io_mentor_cell_test_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, FILE *f)
{
	char line[1024], *s;

	while(!(feof(f))) {
		if (fgets(line, sizeof(line), f) != NULL) {
			s = line;
			while(isspace(*s)) s++; /* strip leading whitespace */
			if (strncmp(s, ".FILETYPE CELL_LIBRARY", 22) == 0) /* valid root */
				return 1;
			if ((*s == '\r') || (*s == '\n') || (*s == '\0')) /* ignore empty lines and comments */
				continue;
			/* non-comment, non-empty line - and we don't have our root -> it's not an s-expression */
			return 0;
		}
	}

	/* hit eof before seeing a valid root -> bad */
	return 0;
}

/*
 *														COPYRIGHT
 *
 *	pcb-rnd, interactive printed circuit board design
 *	Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *	Copyright (C) 2016 Erich S. Heinzle
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
#include <math.h>
#include <gensexpr/gsxl.h>
#include "plug_io.h"
#include "error.h"
#include "data.h"
#include "read.h"
#include "layer.h"
#include "const.h"
#include "netlist.h"

typedef struct {
	PCBTypePtr PCB;
	const char *Filename;
	conf_role_t settings_dest;
	gsxl_dom_t dom;
} read_state_t;

typedef struct {
	const char *node_name;
	int (*parser)(read_state_t *st, gsxl_node_t *subtree);
} dispatch_t;

/* Search the dispatcher table for subtree->str, execute the parser on match
   with the children ("parameters") of the subtree */
static int kicad_dispatch(read_state_t *st, gsxl_node_t *subtree, const dispatch_t *disp_table)
{
	const dispatch_t *d;

	/* do not tolerate empty/NIL node */
	if (subtree->str == NULL)
		return -1;

	for(d = disp_table; d->node_name != NULL; d++)
		if (strcmp(d->node_name, subtree->str) == 0)
			return d->parser(st, subtree->children);

	/* node name not found in the dispatcher table */
	return -1;
}

/* Ignore the subtree */
static int kicad_parse_nop(read_state_t *st, gsxl_node_t *subtree)
{
	return 0;
}

/* kicad_pcb/version */
static int kicad_parse_version(read_state_t *st, gsxl_node_t *subtree)
{
	if (subtree->str != NULL) {
		int ver = atoi(subtree->str);
		if (ver == 3) /* accept version 3 */
			return 0;
	}
	return -1;
}

/* Parse a board from &st->dom into st->PCB */
static int kicad_parse_pcb(read_state_t *st)
{
	gsxl_node_t *n;
	static const dispatch_t disp[] = { /* possible children of root */
		{"version",    kicad_parse_version},
		{"host",       kicad_parse_nop},
		{"general",    kicad_parse_nop},
		{"page",       kicad_parse_nop},
		{"layers",     kicad_parse_nop},
		{"setup",      kicad_parse_nop},
		{"net",        kicad_parse_nop},
		{"net_class",  kicad_parse_nop},
		{"module",     kicad_parse_nop},
		{NULL, NULL}
	};

	/* require the root node to be kicad_pcb */
	if ((st->dom.root->str == NULL) || (strcmp(st->dom.root->str, "kicad_pcb") != 0))
		return -1; 

	/* Call the corresponding subtree parser for each child node; if any of them
	   fail, parse fails */
	for(n = st->dom.root->children; n != NULL; n = n->next)
		if (kicad_dispatch(st, n, disp) != 0)
			return -1;

	return 0; /* success */
}

int io_kicad_read_pcb(plug_io_t *ctx, PCBTypePtr Ptr, const char *Filename, conf_role_t settings_dest)
{
	int c, readres = 0;
	read_state_t st;
	gsx_parse_res_t res;

	/* set up the parse context */
	st.PCB = Ptr;
	st.Filename = Filename;
	st.settings_dest = settings_dest;

	/* load the file into the dom */
	gsxl_init(&st.dom, gsxl_node_t);
	do {
		c = getchar();
	} while((res = gsxl_parse_char(&st.dom, c)) == GSX_RES_NEXT);

	if (res == GSX_RES_EOE) {
		/* compact and simplify the tree */
		gsxl_compact_tree(&st.dom);

		/* recursively parse the dom */
		readres = kicad_parse_pcb(&st);
	}
	else
		readres = -1;

	/* clean up */
	gsxl_uninit(&st.dom);


	return readres;
}

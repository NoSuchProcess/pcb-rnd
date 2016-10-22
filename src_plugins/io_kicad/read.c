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

	printf("Unknown node: '%s'\n", subtree->str);
	/* node name not found in the dispatcher table */
	return -1;
}

/* Take each children of tree and execute them using kicad_dispatch
   Useful for procssing nodes that may host various subtrees of different
   nodes ina  flexible way. Return non-zero if any subtree processor failed. */
static int kicad_foreach_dispatch(read_state_t *st, gsxl_node_t *tree, const dispatch_t *disp_table)
{
	gsxl_node_t *n;

	for(n = tree; n != NULL; n = n->next)
		if (kicad_dispatch(st, n, disp_table) != 0)
			return -1;

	return 0; /* success */
}


/* No-op: ignore the subtree */
static int kicad_parse_nop(read_state_t *st, gsxl_node_t *subtree)
{
	return 0;
}

/* kicad_pcb/version */
static int kicad_parse_version(read_state_t *st, gsxl_node_t *subtree)
{
	if (subtree->str != NULL) {
		int ver = atoi(subtree->str);
		printf("kicad version: '%s' == %d\n", subtree->str, ver);
		if (ver == 3) /* accept version 3 */
			return 0;
	}
	return -1;
}

static int kicad_parse_pcb_element(read_state_t *st);

static int kicad_parse_at(read_state_t *st, gsxl_node_t *subtree);
static int kicad_parse_start(read_state_t *st, gsxl_node_t *subtree);
static int kicad_parse_end(read_state_t *st, gsxl_node_t *subtree);
static int kicad_parse_thickness(read_state_t *st, gsxl_node_t *subtree);
static int kicad_parse_layer_definitions(read_state_t *st, gsxl_node_t *subtree);
static int kicad_parse_element_layers(read_state_t *st, gsxl_node_t *subtree);
static int kicad_parse_width(read_state_t *st, gsxl_node_t *subtree);
static int kicad_parse_angle(read_state_t *st, gsxl_node_t *subtree);
static int kicad_parse_justify(read_state_t *st, gsxl_node_t *subtree);
static int kicad_parse_size(read_state_t *st, gsxl_node_t *subtree);
static int kicad_parse_drill(read_state_t *st, gsxl_node_t *subtree);
static int kicad_parse_tstamp(read_state_t *st, gsxl_node_t *subtree);
static int kicad_parse_net(read_state_t *st, gsxl_node_t *subtree);
static int kicad_parse_element_layer(read_state_t *st, gsxl_node_t *subtree);
static int kicad_parse_font(read_state_t *st, gsxl_node_t *subtree);
/* static int kicad_parse_effects(read_state_t *st, gsxl_node_t *subtree); */

/* kicad_pcb/gr_text */
static int kicad_parse_gr_text(read_state_t *st, gsxl_node_t *subtree)
{

	gsxl_node_t *n, *m;
	int i;
	int tally = 0;
	int retval = 0;

	if (subtree->str != NULL) {
		printf("gr_text element being parsed: '%s'\n", subtree->str);		
		for(n = subtree,i = 0; n != NULL; n = n->next, i++) {
			if (strcmp("at", n->str) == 0) {
				retval += kicad_parse_at(st, n);
				tally += 1;
			} else if (strcmp("layer", n->str) == 0) {
				retval += kicad_parse_element_layer(st, n);
				tally += 2;
			} else if (strcmp("effects", n->str) == 0) {
				for(m = n->children; m != NULL; m = m->next) {
					/*printf("stepping through effects def, looking at %s\n", m->str); */
					if (strcmp("font", m->str) == 0) {
						retval += kicad_parse_font(st, m);
						tally += 4;
					} else if (strcmp("justify", m->str) == 0) {
						retval += kicad_parse_justify(st, m);
						tally += 8;
					}
				}
			}
		}
	}
	if ((tally > 2) && (retval == 0)) { /* has location, layer and stroke thickness at a minimum */
		return 0;
	}
	return -1;
}
	
/* kicad_pcb  parse (font  ) */
static int kicad_parse_font(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *n;
	int tally = 0;
	int retval = 0;

	if (subtree->str != NULL) {
		printf("gr_text (font ) element being parsed: '%s'\n", subtree->str);		
		for(n = subtree->children; n != NULL; n = n->next) {
			if (strcmp("size", n->str) == 0) {
				retval += kicad_parse_size(st, n);
				tally += 1;
			} else if (strcmp("thickness", n->str) == 0) {
				retval += kicad_parse_thickness(st, n);
				tally += 2;
			}
		}
	}
	if ((tally > 2) && (retval == 0)) { /* has location, layer and stroke thickness at a minimum */
		return 0;
	}
	return -1;
}

/* kicad_pcb  parse (size  ) */
static int kicad_parse_size(read_state_t *st, gsxl_node_t *subtree)
{
		if (subtree != NULL && subtree->str != NULL) {
			pcb_printf("arg: '%s'\n", subtree->str);
		}
		if (subtree->children != NULL && subtree->children->str != NULL) {
			pcb_printf("sizeX: '%s'\n", (subtree->children->str));
		}
		if (subtree->children->next != NULL && subtree->children->next->str != NULL) {
			pcb_printf("sizeY: '%s'\n", (subtree->children->next->str));
		}
		return 0;
}

/* kicad_pcb  parse (thickness  ) */
static int kicad_parse_thickness(read_state_t *st, gsxl_node_t *subtree)
{
		if (subtree != NULL && subtree->str != NULL) {
			pcb_printf("arg: '%s'\n", subtree->str);
		}
		if (subtree->children != NULL && subtree->children->str != NULL) {
			pcb_printf("thickness: '%s'\n", (subtree->children->str));
		}
		return 0;
}

/* kicad_pcb  parse (justify  ) */
static int kicad_parse_justify(read_state_t *st, gsxl_node_t *subtree)
{
		if (subtree != NULL && subtree->str != NULL) {
			pcb_printf("arg: '%s'\n", subtree->str);
		}
		if (subtree->children != NULL && subtree->children->str != NULL) {
			pcb_printf("justification: '%s'\n", (subtree->children->str));
		}
		return 0;
}

/* kicad_pcb/gr_line */
static int kicad_parse_gr_line(read_state_t *st, gsxl_node_t *subtree)
{
	if (subtree->str != NULL) {

		printf("gr_line element being parsed: '%s'\n", subtree->str);

/*		if (subtree != NULL) {
			kicad_parse_start(st, subtree);
		}
		if (subtree->next != NULL) {
			kicad_parse_end(st, subtree->next);
		}
		if (subtree->next->next->next->next == NULL) {
			if (subtree->next->next != NULL) {
				kicad_parse_layer(st, subtree->next->next);
			}
			if (subtree->next->next->next != NULL) {
				kicad_parse_width(st, subtree->next->next->next);
			}
		} else {
			if (subtree->next->next != NULL) {
				kicad_parse_angle(st, subtree->next->next);
			}
			if (subtree->next->next->next != NULL) {
				kicad_parse_layer(st, subtree->next->next->next);
			}
			if (subtree->next->next->next->next != NULL) {
				kicad_parse_width(st, subtree->next->next->next->next);
			}
		} */
		return 0;
	}
	return -1;
}

/* kicad_pcb/gr_arc */
static int kicad_parse_gr_arc(read_state_t *st, gsxl_node_t *subtree)
{
	if (subtree->str != NULL) {

		printf("gr_arc: '%s'\n", subtree->str);

		if (subtree != NULL) {
			kicad_parse_start(st, subtree);
		}
		if (subtree->next != NULL) {
			kicad_parse_end(st, subtree->next);
		}
		if (subtree->next->next != NULL) {
			kicad_parse_angle(st, subtree->next->next);
		}
		if (subtree->next->next->next != NULL) {
			kicad_parse_element_layer(st, subtree->next->next->next);
		}
		if (subtree->next->next->next->next != NULL) {
			kicad_parse_width(st, subtree->next->next->next->next);
		}

		return 0;
	}
	return -1;
}

/* kicad_pcb/via */
static int kicad_parse_via(read_state_t *st, gsxl_node_t *subtree)
{
	if (subtree->str != NULL) {

		printf("via: '%s'\n", subtree->str);

		if (subtree != NULL) {
			kicad_parse_at(st, subtree);
		}
		if (subtree->next != NULL) {
			kicad_parse_size(st, subtree->next);
		}
		if (subtree->next->next != NULL) {
			kicad_parse_element_layers(st, subtree->next->next);
		}
/*		if (subtree->next->next->next->next != NULL) {
 *			kicad_parse_net(st, subtree->next->next->next->next);
 *		}
 *  we don't need net info for a via
 */
		return 0;
	}
	return -1;
}

/* kicad_pcb/segment */
static int kicad_parse_segment(read_state_t *st, gsxl_node_t *subtree)
{
	if (subtree->str != NULL) {

		printf("segment: '%s'\n", subtree->str);

		if (subtree != NULL) {
			kicad_parse_start(st, subtree);
		}
		if (subtree->next != NULL) {
			kicad_parse_end(st, subtree->next);
		}
		if (subtree->next->next != NULL) {
			kicad_parse_element_layer(st, subtree->next->next);
		}
		if (subtree->next->next->next != NULL) {
			kicad_parse_width(st, subtree->next->next->next);
		}
		if (subtree->next->next->next->next != NULL) {
			kicad_parse_net(st, subtree->next->next->next->next);
		}
/*   although we don't really care about nets for trackwork, unlike kicad */
		return 0;
	}
	return -1;
}


/* kicad_pcb  parse (at  ) */
static int kicad_parse_at(read_state_t *st, gsxl_node_t *subtree)
{
		if (subtree != NULL && subtree->str != NULL) {
			pcb_printf("arg: '%s'\n", subtree->str);
		}
		if (subtree->children != NULL && subtree->children->str != NULL) {
			pcb_printf("at   x: '%s'\n", (subtree->children->str));
		}
		if (subtree->children->next != NULL && subtree->children->next->str != NULL) {
			pcb_printf("at   y: '%s'\n", (subtree->children->next->str));
		}
		if (subtree->children->next->next != NULL && subtree->children->next->next->str != NULL) {
			pcb_printf("rotation: '%s'\n", (subtree->children->next->next->str));
		}
		return 0;
}

/* kicad_pcb  parse (start  ) */
static int kicad_parse_start(read_state_t *st, gsxl_node_t *subtree)
{
		if (subtree != NULL && subtree->str != NULL) {
			pcb_printf("arg: '%s'\n", subtree->str);
		}
		if (subtree->children != NULL && subtree->children->str != NULL) {
			pcb_printf("start at   x1: '%s'\n", (subtree->children->str));
		}
		if (subtree->children->next != NULL && subtree->children->next->str != NULL) {
			pcb_printf("start at   y1: '%s'\n", (subtree->children->next->str));
		}
		return 0;
}

/* kicad_pcb  parse (end  ) */
static int kicad_parse_end(read_state_t *st, gsxl_node_t *subtree)
{
		if (subtree != NULL && subtree->str != NULL) {
			pcb_printf("arg: '%s'\n", subtree->str);
		}
		if (subtree->children != NULL && subtree->children->str != NULL) {
			pcb_printf("end at   x2: '%s'\n", (subtree->children->str));
		}
		if (subtree->children->next != NULL && subtree->children->next->str != NULL) {
			pcb_printf("end at   y2 '%s'\n", (subtree->children->next->str));
		}
		return 0;
}

/* kicad_pcb  parse (width  ) */
static int kicad_parse_width(read_state_t *st, gsxl_node_t *subtree)
{
		if (subtree != NULL && subtree->str != NULL) {
			pcb_printf("arg: '%s'\n", subtree->str);
		}
		if (subtree->children != NULL && subtree->children->str != NULL) {
			pcb_printf("width: '%s'\n", (subtree->children->str));
		}
		return 0;
}

/* kicad_pcb  parse (layer  ) */
static int kicad_parse_element_layer(read_state_t *st, gsxl_node_t *subtree)
{
		if (subtree != NULL && subtree->str != NULL) {
			pcb_printf("arg: '%s'\n", subtree->str);
		}
		if (subtree->children != NULL && subtree->children->str != NULL) {
			pcb_printf("layer: '%s'\n", (subtree->children->str));
		}
		return 0;
}

/* kicad_pcb  parse (layers  )  - either board layer defintions, or module pad/via layer defs */
static int kicad_parse_layer_definitions(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *n;
	int i;
		if (strcmp(subtree->parent->parent->str, "kicad_pcb") != 0) { /* test if deeper in tree than layer definitions for entire board */  
			for(n = subtree,i = 0; n != NULL; n = n->next, i++)
				pcb_printf("element on layers %d: '%s'\n", i, n->str);
			return 0;
		} else { /* we are just below the top level or root of the tree, so this must be a layer definitions section */
			printf("Probably in the layer definition block, need an iterator...\n"); /* need to assemble the layer list here*/
			return 0;
		}
}

/* kicad_pcb  parse (layers  )  - module pad/via layer defs */
static int kicad_parse_element_layers(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *n;
	int i;
		if (strcmp(subtree->parent->parent->str, "kicad_pcb") != 0) { /* test if deeper in tree than layer definitions for entire board */  
			for(n = subtree,i = 0; n != NULL; n = n->next, i++)
				pcb_printf("element on layers %d: '%s'\n", i, n->str);
			return 0;
		} 
	return -1;
}



/* kicad_pcb  parse (angle  ) */
static int kicad_parse_angle(read_state_t *st, gsxl_node_t *subtree)
{
		if (subtree != NULL && subtree->str != NULL) {
			pcb_printf("arg: '%s'\n", subtree->str);
		}
		if (subtree->children != NULL && subtree->children->str != NULL) {
			pcb_printf("angle: '%s'\n", (subtree->children->str));
		}
		return 0;
}

/* kicad_pcb  parse (drill  ) */
static int kicad_parse_drill(read_state_t *st, gsxl_node_t *subtree)
{
		if (subtree != NULL && subtree->str != NULL) {
			pcb_printf("arg: '%s'\n", subtree->str);
		}
		if (subtree->children != NULL && subtree->children->str != NULL) {
			pcb_printf("drill: '%s'\n", (subtree->children->str));
		}
		return 0;
}

/* kicad_pcb  parse (tstamp  ) ;   not really used*/
static int kicad_parse_tstamp(read_state_t *st, gsxl_node_t *subtree)
{
		if (subtree != NULL && subtree->str != NULL) {
			pcb_printf("arg: '%s'\n", subtree->str);
		}
		if (subtree->children != NULL && subtree->children->str != NULL) {
			pcb_printf("tstamp: '%s'\n", (subtree->children->str));
		}
		return 0;
}

/* kicad_pcb  parse (net  ) ;   */
static int kicad_parse_net(read_state_t *st, gsxl_node_t *subtree)
{
		if (subtree != NULL && subtree->str != NULL) {
			pcb_printf("net number: '%s'\n", subtree->str);
		}
		if (subtree->next != NULL && subtree->next->str != NULL) {
			pcb_printf("and the net label is: '%s'\n", (subtree->next->str));
		}
		return 0;
}

/* kicad_pcb  parse (net  ) ;   */
static int kicad_parse_element_net(read_state_t *st, gsxl_node_t *subtree)
{
		if (strcmp(subtree->parent->parent->str, "kicad_pcb") != 0) { /* test if deeper in tree than net definitions for entire board */		
			if (subtree != NULL && subtree->str != NULL) {
				pcb_printf("arg: '%s'\n", subtree->str);
			}
			if (subtree->next != NULL && subtree->next->str != NULL) {
				pcb_printf("element net: '%s'\n", (subtree->next->str));
			}
			return 0;
		} 
		return -1;
}

static int kicad_parse_module(read_state_t *st, gsxl_node_t *subtree)
{
	static const dispatch_t disp[] = { /* possible children of a module node */
		{"fp_line",    kicad_parse_nop},
		{"fp_arc",     kicad_parse_nop},
		{"fp_circle",  kicad_parse_nop},
		{"fp_text",    kicad_parse_nop},
		{NULL, NULL}
	};

	return kicad_foreach_dispatch(st, subtree, disp);
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
		{"layers",     kicad_parse_layer_definitions}, /* board layer defs, or, module pad/pin layers, or via layers*/
		{"setup",      kicad_parse_nop},
		{"net",        kicad_parse_net}, /* net labels if child of root, otherwise net attribute of element */
		{"net_class",  kicad_parse_nop},
		{"module",     kicad_parse_nop}, /* module},  for footprints */
		{"gr_line",     kicad_parse_gr_line},
		{"gr_arc",     kicad_parse_gr_arc},
		{"gr_text",    kicad_parse_gr_text},
		{"via",     kicad_parse_via},
		{"segment",     kicad_parse_segment},
		{"zone",     kicad_parse_nop}, /* polygonal zones*/

		{"pad",    kicad_parse_nop}, /* for modules, encompasses pad, pin  */

		{"font",    kicad_parse_nop}, /* for font attr lists */
		{"size",    kicad_parse_nop}, /* used for font char size */
/*		{"effects",    kicad_parse_effects}, /* mostly for fonts in modules*/ 
		{"justify",    kicad_parse_justify}, /* mostly for mirrored text on the bottom layer */

		{"at",    kicad_parse_at},
		{"layer",    kicad_parse_element_layer},
		{"xy",    kicad_parse_nop}, /* for polygonal zone vertices*/
		{"hatch",    kicad_parse_nop},  /* a polygonal zone fill type*/
		{"descr",    kicad_parse_nop}, /* for modules, i.e. TO220 */
		{"tedit",    kicad_parse_nop}, /* not really used, time stamp related */
		{"tstamp",    kicad_parse_tstamp}, /* not really used */
		{NULL, NULL}
	};

	/* require the root node to be kicad_pcb */
	if ((st->dom.root->str == NULL) || (strcmp(st->dom.root->str, "kicad_pcb") != 0))
		return -1; 

	/* Call the corresponding subtree parser for each child node of the root
	   node; if any of them fail, parse fails */
	return kicad_foreach_dispatch(st, st->dom.root->children, disp);
}

int io_kicad_read_pcb(plug_io_t *ctx, PCBTypePtr Ptr, const char *Filename, conf_role_t settings_dest)
{
	int c, readres = 0;
	read_state_t st;
	gsx_parse_res_t res;
	FILE *FP;

	FP = fopen(Filename, "r");
	if (FP == NULL)
		return -1;

	/* set up the parse context */
	st.PCB = Ptr;
	st.Filename = Filename;
	st.settings_dest = settings_dest;

	/* load the file into the dom */
	gsxl_init(&st.dom, gsxl_node_t);
	do {
		c = fgetc(FP);
	} while((res = gsxl_parse_char(&st.dom, c)) == GSX_RES_NEXT);
	fclose(FP);

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

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

static int kicad_parse_layer_definitions(read_state_t *st, gsxl_node_t *subtree);
static int kicad_parse_element_layers(read_state_t *st, gsxl_node_t *subtree);
static int kicad_parse_drill(read_state_t *st, gsxl_node_t *subtree);
static int kicad_parse_net(read_state_t *st, gsxl_node_t *subtree);

#define SEEN_NO_DUP(bucket, bit) \
do { \
	int __mask__ = (1<<(bit)); \
	if ((bucket) & __mask__) \
		return -1; \
	bucket |= __mask__; \
} while(0)

#define SEEN(bucket, bit) \
do { \
	int __mask__ = (1<<(bit)); \
	bucket |= __mask__; \
} while(0)

#define BV(bit) (1<<(bit))

/* kicad_pcb/gr_text */
static int kicad_parse_gr_text(read_state_t *st, gsxl_node_t *subtree)
{

	gsxl_node_t *l, *n, *m;
	int i;
	unsigned long tally = 0, required;
	int retval = 0;

	if (subtree->str != NULL) {
		printf("gr_text element being parsed: '%s'\n", subtree->str);		
		for(n = subtree,i = 0; n != NULL; n = n->next, i++) {
			if (n->str != NULL && strcmp("at", n->str) == 0) {
					SEEN_NO_DUP(tally, 0);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("text at x: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 1); /* same as ^= 1 was */
					} else {
						return -1;
					}
					if (n->children->next != NULL && n->children->next->str != NULL) {
						pcb_printf("text at y: '%s'\n", (n->children->next->str));
						SEEN_NO_DUP(tally, 2);	
						if (n->children->next->next != NULL && n->children->next->next->str != NULL) {
							pcb_printf("text rotation: '%s'\n", (n->children->next->next->str));
							SEEN_NO_DUP(tally, 3);
						} 
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("layer", n->str) == 0) {
				SEEN_NO_DUP(tally, 4);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("text layer: '%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("effects", n->str) == 0) {
				SEEN_NO_DUP(tally, 5);
				for(m = n->children; m != NULL; m = m->next) {
					/*printf("stepping through effects def, looking at %s\n", m->str);*/ 
					if (m->str != NULL && strcmp("font", m->str) == 0) {
						SEEN_NO_DUP(tally, 6);
						for(l = m->children; l != NULL; l = l->next) {
							if (m->str != NULL && strcmp("size", l->str) == 0) {
								SEEN_NO_DUP(tally, 7);
								if (l->children != NULL && l->children->str != NULL) {
									pcb_printf("font sizeX: '%s'\n", (l->children->str));
								} else {
									return -1;
								}
								if (l->children->next != NULL && l->children->next->str != NULL) {
									pcb_printf("font sizeY: '%s'\n", (l->children->next->str));
								} else {
									return -1;
								}
							} else if (strcmp("thickness", l->str) == 0) {
								SEEN_NO_DUP(tally, 8);
								if (l->children != NULL && l->children->str != NULL) {
									pcb_printf("font thickness: '%s'\n", (l->children->str));
								} else {
									return -1;
								}
							}
						}
					} else if (m->str != NULL && strcmp("justify", m->str) == 0) {
						SEEN_NO_DUP(tally, 9);
						if (m->children != NULL && m->children->str != NULL) {
							pcb_printf("text justification: '%s'\n", (m->children->str));
						} else {
							return -1;
						}
					} else {
						if (m->str != NULL) {
							printf("Unknown effects argument %s:", m->str);
						}
						return -1;
					}
				}
			} 				
		}
	}
	required = 1; /*BV(2) | BV(3) | BV(4) | BV(7) | BV(8); */
	if ((tally & required) == required) { /* has location, layer, size and stroke thickness at a minimum */
		return 0;
	}
	return -1;
}

/* kicad_pcb/gr_line */
static int kicad_parse_gr_line(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *n;
	unsigned long tally = 0;

	if (subtree->str != NULL) {
		printf("gr_line being parsed: '%s'\n", subtree->str);		
		for(n = subtree; n != NULL; n = n->next) {
			if (n->str != NULL && strcmp("start", n->str) == 0) {
					SEEN_NO_DUP(tally, 0);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("gr_line start at x: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 1); /* same as ^= 1 was */
					} else {
						return -1;
					}
					if (n->children->next != NULL && n->children->next->str != NULL) {
						pcb_printf("gr_line start at y: '%s'\n", (n->children->next->str));
						SEEN_NO_DUP(tally, 2);	
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("end", n->str) == 0) {
					SEEN_NO_DUP(tally, 3);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("gr_line end at x: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 4);
					} else {
						return -1;
					}
					if (n->children->next != NULL && n->children->next->str != NULL) {
						pcb_printf("gr_line end at y: '%s'\n", (n->children->next->str));
						SEEN_NO_DUP(tally, 5);	
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("layer", n->str) == 0) {
					SEEN_NO_DUP(tally, 6);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("gr_line layer: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 7);
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("width", n->str) == 0) {
					SEEN_NO_DUP(tally, 8);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("gr_line width: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 9);
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("angle", n->str) == 0) { /* unlikely to be used or seen */
					SEEN_NO_DUP(tally, 10);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("gr_line angle: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 11);
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("net", n->str) == 0) { /* unlikely to be used or seen */
					SEEN_NO_DUP(tally, 12);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("gr_line net: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 13);
					} else {
						return -1;
					}
			} else {
				if (n->str != NULL) {
					printf("Unknown gr_line argument %s:", n->str);
				}
				return -1;
			}
		}
	}
	if (tally >= 0) { /* need start, end, layer, thickness at a minimum */
		return 0;
	}
	return -1;
}

/* kicad_pcb/gr_arc */
static int kicad_parse_gr_arc(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *n;
	unsigned long tally = 0;

	if (subtree->str != NULL) {
		printf("gr_arc being parsed: '%s'\n", subtree->str);		
		for(n = subtree; n != NULL; n = n->next) {
			if (n->str != NULL && strcmp("start", n->str) == 0) {
					SEEN_NO_DUP(tally, 0);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("gr_arc centre at x: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 1); /* same as ^= 1 was */
					} else {
						return -1;
					}
					if (n->children->next != NULL && n->children->next->str != NULL) {
						pcb_printf("gr_arc centre at y: '%s'\n", (n->children->next->str));
						SEEN_NO_DUP(tally, 2);	
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("end", n->str) == 0) {
					SEEN_NO_DUP(tally, 3);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("gr_arc end at x: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 4);
					} else {
						return -1;
					}
					if (n->children->next != NULL && n->children->next->str != NULL) {
						pcb_printf("gr_arc end at y: '%s'\n", (n->children->next->str));
						SEEN_NO_DUP(tally, 5);	
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("layer", n->str) == 0) {
					SEEN_NO_DUP(tally, 6);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("gr_arc layer: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 7);
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("width", n->str) == 0) {
					SEEN_NO_DUP(tally, 8);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("gr_arc width: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 9);
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("angle", n->str) == 0) {
					SEEN_NO_DUP(tally, 10);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("gr_arc angle CW rotation: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 11);
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("net", n->str) == 0) { /* unlikely to be used or seen */
					SEEN_NO_DUP(tally, 12);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("gr_arc net: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 13);
					} else {
						return -1;
					}
			} else {
				if (n->str != NULL) {
					printf("Unknown gr_arc argument %s:", n->str);
				}
				return -1;
			}
		}
	}
	if (tally >= 0) { /* need start, end, layer, thickness at a minimum */
		return 0;
	}
	return -1;
}

/* kicad_pcb/via */
static int kicad_parse_via(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *m, *n;
	unsigned long tally = 0;

	if (subtree->str != NULL) {
		printf("via being parsed: '%s'\n", subtree->str);		
		for(n = subtree; n != NULL; n = n->next) {
			if (n->str != NULL && strcmp("at", n->str) == 0) {
					SEEN_NO_DUP(tally, 0);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("via at x: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 1); /* same as ^= 1 was */
					} else {
						return -1;
					}
					if (n->children->next != NULL && n->children->next->str != NULL) {
						pcb_printf("via at y: '%s'\n", (n->children->next->str));
						SEEN_NO_DUP(tally, 2);	
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("size", n->str) == 0) {
					SEEN_NO_DUP(tally, 3);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("via size: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 4);
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("layers", n->str) == 0) {
					SEEN_NO_DUP(tally, 5);
					for(m = n->children; m != NULL; m = m->next) {
						if (m->str != NULL) {
							pcb_printf("via layer: '%s'\n", (m->str));
						} else {
							return -1;
						}
					}
			} else if (n->str != NULL && strcmp("net", n->str) == 0) {
					SEEN_NO_DUP(tally, 6);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("segment net: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 7);
					} else {
						return -1;
					}
			} else {
				if (n->str != NULL) {
					printf("Unknown via argument %s:", n->str);
				}
				return -1;
			}
		}
	}
	if (tally >= 0) { /* need start, end, layer, thickness at a minimum */
		return 0;
	}
	return -1;
}

/* kicad_pcb/segment */
static int kicad_parse_segment(read_state_t *st, gsxl_node_t *subtree)
{

	gsxl_node_t *n;
	unsigned long tally = 0;

	if (subtree->str != NULL) {
		printf("segment being parsed: '%s'\n", subtree->str);		
		for(n = subtree; n != NULL; n = n->next) {
			if (n->str != NULL && strcmp("start", n->str) == 0) {
					SEEN_NO_DUP(tally, 0);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("segment start at x: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 1); /* same as ^= 1 was */
					} else {
						return -1;
					}
					if (n->children->next != NULL && n->children->next->str != NULL) {
						pcb_printf("segment start at y: '%s'\n", (n->children->next->str));
						SEEN_NO_DUP(tally, 2);	
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("end", n->str) == 0) {
					SEEN_NO_DUP(tally, 3);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("segment end at x: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 4);
					} else {
						return -1;
					}
					if (n->children->next != NULL && n->children->next->str != NULL) {
						pcb_printf("segment end at y: '%s'\n", (n->children->next->str));
						SEEN_NO_DUP(tally, 5);	
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("layer", n->str) == 0) {
					SEEN_NO_DUP(tally, 6);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("segment layer: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 7);
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("width", n->str) == 0) {
					SEEN_NO_DUP(tally, 8);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("segment width: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 9);
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("net", n->str) == 0) {
					SEEN_NO_DUP(tally, 10);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("segment net: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 11);
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("tstamp", n->str) == 0) { /* not likely to be used */
					SEEN_NO_DUP(tally, 12);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("segment timestamp: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 13);
					} else {
						return -1;
					}
			} else {
				if (n->str != NULL) {
					printf("Unknown segment argument %s:", n->str);
				}
				return -1;
			}
		}
	}
	if (tally >= 0) { /* need start, end, layer, thickness at a minimum */
		return 0;
	}
	return -1;
}

/* kicad_pcb  parse (layers  )  - either board layer defintions, or module pad/via layer defs */
static int kicad_parse_layer_definitions(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *m, *n;
	int i;
		if (strcmp(subtree->parent->parent->str, "kicad_pcb") != 0) { /* test if deeper in tree than layer definitions for entire board */  
			pcb_printf("layer definitions encountered in unexpected place\n");
			return -1;
		} else { /* we are just below the top level or root of the tree, so this must be a layer definitions section */
			pcb_printf("Board layer descriptions:");
			for(n = subtree,i = 0; n != NULL; n = n->next, i++) {
				if ((n->str != NULL) && (n->children->str != NULL) && (n->children->next != NULL) && (n->children->next->str != NULL) ) {
					pcb_printf("layer #%d LAYERNUM found:\t%s\n", i, n->str);
					pcb_printf("layer #%d layer label found:\t%s\n", i, n->children->str);
					pcb_printf("layer #%d layer description/type found:\t%s\n", i, n->children->next->str);
				} else {
					printf("unexpected board layer definition(s) encountered.\n");
					return -1;
				}
			}
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

/*		{"font",    kicad_parse_nop}, for font attr lists
 *		{"size",    kicad_parse_nop},  used for font char size
 *		{"effects",    kicad_parse_effects}, /* mostly for fonts in modules 
 *		{"justify",    kicad_parse_justify},  mostly for mirrored text on the bottom layer
 *
 *		{"at",    kicad_parse_at},
 *		{"layer",    kicad_parse_element_layer},
 *		{"xy",    kicad_parse_nop}, for polygonal zone vertices
 *		{"hatch",    kicad_parse_nop},   a polygonal zone fill type
 *		{"descr",    kicad_parse_nop},  for modules, i.e. TO220 
 *		{"tedit",    kicad_parse_nop}, not really used, time stamp related
 *		{"tstamp",    kicad_parse_tstamp},  not really used */
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

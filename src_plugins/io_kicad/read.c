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
#include <assert.h>
#include <math.h>
#include <gensexpr/gsxl.h>
#include <genht/htsi.h>
#include "compat_misc.h"
#include "plug_io.h"
#include "error.h"
#include "data.h"
#include "read.h"
#include "layer.h"
#include "const.h"
#include "netlist.h"
#include "create.h"
#include "misc.h" /* for flag setting */

typedef struct {
	PCBTypePtr PCB;
	const char *Filename;
	conf_role_t settings_dest;
	gsxl_dom_t dom;
	htsi_t layer_k2i; /* layer name-to-index hash; name is the kicad name, index is the pcb-rnd layer index */
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

static int kicad_parse_layer_definitions(read_state_t *st, gsxl_node_t *subtree); /* for layout layer definitions */
static int kicad_parse_net(read_state_t *st, gsxl_node_t *subtree); /* describes netlists for the layout */
static int kicad_get_layeridx(read_state_t *st, const char *kicad_name);

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


/* kicad_pcb/parse_page */
static int kicad_parse_page_size(read_state_t *st, gsxl_node_t *subtree)
{

	gsxl_node_t *n;
	unsigned long tally = 0, required;

	if (subtree != NULL && subtree->str != NULL) {
		printf("page setting being parsed: '%s'\n", subtree->str);		
			if (strcmp("A4", subtree->str) == 0) {
				st->PCB->MaxWidth = PCB_MM_TO_COORD(297.0);
				st->PCB->MaxHeight = PCB_MM_TO_COORD(210.0);
			} else if (strcmp("A3", subtree->str) == 0) {
				st->PCB->MaxWidth = PCB_MM_TO_COORD(420.0);
				st->PCB->MaxHeight = PCB_MM_TO_COORD(297.0);
			} else if (strcmp("A2", subtree->str) == 0) {
				st->PCB->MaxWidth = PCB_MM_TO_COORD(594.0);
				st->PCB->MaxHeight = PCB_MM_TO_COORD(420.0);
			} else if (strcmp("A1", subtree->str) == 0) {
				st->PCB->MaxWidth = PCB_MM_TO_COORD(841.0);
				st->PCB->MaxHeight = PCB_MM_TO_COORD(594.0);
			} else if (strcmp("A0", subtree->str) == 0) {
				st->PCB->MaxWidth = PCB_MM_TO_COORD(1189.0);
				st->PCB->MaxHeight = PCB_MM_TO_COORD(841.0);
			} else { /* default to A0 */
				st->PCB->MaxWidth = PCB_MM_TO_COORD(1189.0);
				st->PCB->MaxHeight = PCB_MM_TO_COORD(841.0);
			}
			return 0;
	}
	return -1;
}


/* kicad_pcb/gr_text */
static int kicad_parse_gr_text(read_state_t *st, gsxl_node_t *subtree)
{

	gsxl_node_t *l, *n, *m;
	int i;
	unsigned long tally = 0, required;

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

/* kicad_pcb/fp_circle */
static int kicad_parse_fp_circle(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *n;
	unsigned long tally = 0;

	if (subtree->str != NULL) {
		printf("fp_circle being parsed: '%s'\n", subtree->str);		
		for(n = subtree; n != NULL; n = n->next) {
			if (n->str != NULL && strcmp("center", n->str) == 0) {
					SEEN_NO_DUP(tally, 0);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("fp_circle centre at x: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 1); /* same as ^= 1 was */
					} else {
						return -1;
					}
					if (n->children->next != NULL && n->children->next->str != NULL) {
						pcb_printf("fp_circle centre at y: '%s'\n", (n->children->next->str));
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
					printf("Unknown fp_circle argument %s:", n->str);
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

	char *end, *name; /* not using via name for now */
	double val;
	Coord X, Y, Thickness, Clearance, Mask, Drill; /* not sure what to do with mask */
	FlagType Flags = MakeFlags(0); /* start with something bland here */
	int PCBLayer; /* not used for now */

	Clearance = Mask = PCB_MM_TO_COORD(0.250); /* start with something bland here */
	Drill = PCB_MM_TO_COORD(0.300); /* start with something sane */
	name = "";

	if (subtree->str != NULL) {
		printf("via being parsed: '%s'\n", subtree->str);		
		for(n = subtree; n != NULL; n = n->next) {
			if (n->str != NULL && strcmp("at", n->str) == 0) {
					SEEN_NO_DUP(tally, 0);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("via at x: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 1); /* same as ^= 1 was */
						val = strtod(n->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							X = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
					if (n->children->next != NULL && n->children->next->str != NULL) {
						pcb_printf("via at y: '%s'\n", (n->children->next->str));
						SEEN_NO_DUP(tally, 2);	
						val = strtod(n->children->next->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							Y = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("size", n->str) == 0) {
					SEEN_NO_DUP(tally, 3);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("via size: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 4);
						val = strtod(n->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							Thickness = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("layers", n->str) == 0) {
					SEEN_NO_DUP(tally, 5);
					for(m = n->children; m != NULL; m = m->next) {
						if (m->str != NULL) {
							pcb_printf("via layer: '%s'\n", (m->str));
/*							PCBLayer = kicad_get_layeridx(st, n->children->str);
 *							if (PCBLayer == -1) {
 *								return -1;
 *							}   via layers not currently used in PCB
 */   
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
		CreateNewVia( st->PCB->Data, X, Y, Thickness, Clearance, Mask, Drill, name, Flags);
		return 0;
	}
	return -1;
}

/* kicad_pcb/segment */
static int kicad_parse_segment(read_state_t *st, gsxl_node_t *subtree)
{

	gsxl_node_t *n;
	unsigned long tally = 0;

	char *end;
	double val;
	Coord X1, Y1, X2, Y2, Thickness, Clearance;
	FlagType Flags = MakeFlags(0); /* start with something bland here */
	int PCBLayer;

	Clearance = PCB_MM_TO_COORD(0.250); /* start with something bland here */

	if (subtree->str != NULL) {
		printf("segment being parsed: '%s'\n", subtree->str);		
		for(n = subtree; n != NULL; n = n->next) {
			if (n->str != NULL && strcmp("start", n->str) == 0) {
					SEEN_NO_DUP(tally, 0);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("segment start at x: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 1); /* same as ^= 1 was */
						val = strtod(n->children->str, &end);
						printf("The number (double) is %lf\n", val);
						printf("The leftover (string) is %s\n", end);
						if (*end != 0) {
							return -1;
						} else {
							X1 = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
					if (n->children->next != NULL && n->children->next->str != NULL) {
						pcb_printf("segment start at y: '%s'\n", (n->children->next->str));
						SEEN_NO_DUP(tally, 2);	
						val = strtod(n->children->next->str, &end);
						printf("The number (double) is %lf\n", val);
						printf("The leftover (string) is %s\n", end);
						if (*end != 0) {
							return -1;
						} else {
							Y1 = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("end", n->str) == 0) {
					SEEN_NO_DUP(tally, 3);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("segment end at x: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 4);
						val = strtod(n->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							X2 = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
					if (n->children->next != NULL && n->children->next->str != NULL) {
						pcb_printf("segment end at y: '%s'\n", (n->children->next->str));
						SEEN_NO_DUP(tally, 5);	
						val = strtod(n->children->next->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							Y2 = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("layer", n->str) == 0) {
					SEEN_NO_DUP(tally, 6);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("segment layer: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 7);
						PCBLayer = kicad_get_layeridx(st, n->children->str);
						if (PCBLayer == -1) {
							return -1;
						}
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("width", n->str) == 0) {
					SEEN_NO_DUP(tally, 8);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("segment width: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 9);
						val = strtod(n->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							Thickness = PCB_MM_TO_COORD(val);
						}
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
		CreateNewLineOnLayer( &st->PCB->Data->Layer[PCBLayer], X1, Y1, X2, Y2, Thickness, Clearance, Flags);
		pcb_printf("***new line on layer created [ %mm %mm %mm %mm %mm %mm flags]", X1, Y1, X2, Y2, Thickness, Clearance);
		return 0;
	}
	return -1;
}

/* Parse a layer definition and do all the administration needed for the layer */
static int kicad_create_layer(read_state_t *st, int lnum, const char *lname, const char *ltype)
{
	int id = -1;
	switch(lnum) {
		case 0:   id = SOLDER_LAYER; break;
		case 15:  id = COMPONENT_LAYER; break;
		default:
			if (strcmp(lname, "Edge.Cuts") == 0) {
				/* Edge must be the outline */
				id = pcb_layer_create(PCB_LYT_OUTLINE, 0, 0, "outline");
			}
			else if ((strcmp(ltype, "signal") == 0) || (strncmp(lname, "Dwgs.", 4) == 0) || (strncmp(lname, "Cmts.", 4) == 0) || (strncmp(lname, "Eco", 3) == 0)) {
				/* Create a new inner layer for signals and for emulating misc layers */
				id = pcb_layer_create(PCB_LYT_INTERN | PCB_LYT_COPPER, 0, 0, lname);
			}
			else if ((lname[1] == '.') && ((lname[0] == 'F') || (lname[0] == 'B'))) {
				/* F. or B. layers */
				if (strcmp(lname+2, "SilkS") == 0) return 0; /* silk layers are implicit */
				if (strcmp(lname+2, "Adhes") == 0) return 0; /* pcb-rnd has no adhesive support */
				if (strcmp(lname+2, "Paste") == 0) return 0; /* pcb-rnd has no custom paste support */
				if (strcmp(lname+2, "Mask") == 0)  return 0; /* pcb-rnd has no custom mask support */
				return -1; /* unknown F. or B. layer -> error */
			}
			else
				return -1; /* unknown field */
	}

/* valid layer, save it in the hash */
	assert(id >= 0);
	htsi_set(&st->layer_k2i, pcb_strdup(lname), id);
}

/* Register a kicad layer in the layer hash after looking up the pcb-rnd equivalent */
static unsigned int kicad_reg_layer(read_state_t *st, const char *kicad_name, unsigned int mask)
{
	int id;
	if (pcb_layer_list(mask, &id, 1) != 1)
		return 1;
	htsi_set(&st->layer_k2i, pcb_strdup(kicad_name), id);
	return 0;
}

/* Returns the pcb-rnd layer index for a kicad_name, or -1 if not found */
static int kicad_get_layeridx(read_state_t *st, const char *kicad_name)
{
	htsi_entry_t *e;
	e = htsi_getentry(&st->layer_k2i, kicad_name);
	if (e == NULL)
		return -1;
	return e->value;
}

/* kicad_pcb  parse (layers  )  - either board layer defintions, or module pad/via layer defs */
static int kicad_parse_layer_definitions(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *m, *n;
	int i;
	unsigned int res;

		if (strcmp(subtree->parent->parent->str, "kicad_pcb") != 0) { /* test if deeper in tree than layer definitions for entire board */  
			pcb_printf("layer definitions encountered in unexpected place in kicad layout\n");
			return -1;
		} else { /* we are just below the top level or root of the tree, so this must be a layer definitions section */
			pcb_layers_reset();
			pcb_printf("Board layer descriptions:\n");
			for(n = subtree,i = 0; n != NULL; n = n->next, i++) {
				if ((n->str != NULL) && (n->children->str != NULL) && (n->children->next != NULL) && (n->children->next->str != NULL) ) {
					int lnum = atoi(n->str);
					const char *lname = n->children->str, *ltype = n->children->next->str;
					pcb_printf("layer #%d LAYERNUM found:\t%s\n", i, n->str);
					pcb_printf("layer #%d layer label found:\t%s\n", i, lname);
					pcb_printf("layer #%d layer description/type found:\t%s\n", i, ltype);
					if (kicad_create_layer(st, lnum, lname, ltype) < 0) {
						Message(PCB_MSG_ERROR, "Unrecognized layer: %d, %s, %s\n", lnum, lname, ltype);
						return -1;
					}
				} else {
					printf("unexpected board layer definition(s) encountered.\n");
					return -1;
				}
			}
			pcb_layers_finalize();

			/* set up the hash for implicit layers */
			res = 0;
			res |= kicad_reg_layer(st, "F.SilkS", PCB_LYT_SILK | PCB_LYT_TOP);
			res |= kicad_reg_layer(st, "B.SilkS", PCB_LYT_SILK | PCB_LYT_BOTTOM);

			/*
			We don't have custom mask layers yet
			res |= kicad_reg_layer(st, "F.Mask",  PCB_LYT_MASK | PCB_LYT_TOP);
			res |= kicad_reg_layer(st, "B.Mask",  PCB_LYT_MASK | PCB_LYT_BOTTOM);
			*/

			if (res != 0) {
				Message(PCB_MSG_ERROR, "Internal error: can't find a silk or mask layer\n");
				return -1;
			}

			return 0;
		}
}

/* kicad_pcb  parse (net  ) ;   used for net descriptions for the entire layout */
static int kicad_parse_net(read_state_t *st, gsxl_node_t *subtree)
{
		if (subtree != NULL && subtree->str != NULL) {
			pcb_printf("net number: '%s'\n", subtree->str);
		} else {
			pcb_printf("missing net number in net descriptors");
			return -1;
		}
		if (subtree->next != NULL && subtree->next->str != NULL) {
			pcb_printf("and the net label is: '%s'\n", (subtree->next->str));
		} else {
			pcb_printf("missing net label in net descriptors");
			return -1;
		}
		return 0;
}

static int kicad_parse_module(read_state_t *st, gsxl_node_t *subtree)
{

	gsxl_node_t *l, *n, *m;
	int i;
	unsigned long tally = 0, required;

	if (subtree->str != NULL) {
		printf("Name of module element being parsed: '%s'\n", subtree->str);		
		for(n = subtree->next,i = 0; n != NULL; n = n->next, i++) {
			if (n->str != NULL && strcmp("tedit", n->str) == 0) {
				SEEN_NO_DUP(tally, 0);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("module tedit: '%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("tstamp", n->str) == 0) {
				SEEN_NO_DUP(tally, 1);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("module tstamp: '%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("at", n->str) == 0) {
				SEEN_NO_DUP(tally, 2);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("text at x: '%s'\n", (n->children->str));
					SEEN_NO_DUP(tally, 3); /* same as ^= 1 was */
				} else {
					return -1;
				}
				if (n->children->next != NULL && n->children->next->str != NULL) {
					pcb_printf("text at y: '%s'\n", (n->children->next->str));
					SEEN_NO_DUP(tally, 4);	
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("layer", n->str) == 0) {
				SEEN_NO_DUP(tally, 5);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("text layer: '%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("descr", n->str) == 0) {
				SEEN_NO_DUP(tally, 6);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("module descr: '%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("tags", n->str) == 0) {
				SEEN_NO_DUP(tally, 7);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("module tags: '%s'\n", (n->children->str)); /* maye be more than one? */
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("path", n->str) == 0) {
				SEEN_NO_DUP(tally, 8);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("module path: '%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("model", n->str) == 0) {
				SEEN_NO_DUP(tally, 9);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("module model provided: '%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("pad", n->str) == 0) {
				if (n->children != 0 && n->children->str != NULL) {
					printf("pad name : %s\n", n->str);
				} else {
					return -1;
				}
				if (n->children->next != NULL && n->children->next->str != NULL) {
					pcb_printf("pad type: '%s'\n", (n->children->next->str));
					if (n->children->next->next != NULL && n->children->next->next->str != NULL) {
						pcb_printf("pad shape: '%s'\n", (n->children->next->next->str));
					} else {
						return -1;
					}
				} else {
					return -1;
				}
				for (m = n->children->next->next->next; m != NULL; m = m->next) {
					printf("stepping through module pad defs, looking at: %s\n", m->str);
					if (m->str != NULL && strcmp("at", m->str) == 0) {
						/*SEEN_NO_DUP(padTally, 1); */
						if (m->children != NULL && m->children->str != NULL) {
							pcb_printf("pad X position:\t'%s'\n", (m->children->str));
							if (m->children->next != NULL && m->children->next->str != NULL) {
								pcb_printf("pad Y position:\t'%s'\n", (m->children->next->str));
							} else {
								return -1;
							}
						} else {
							return -1;
						}
					} else if (m->str != NULL && strcmp("layers", m->str) == 0) {
						/*SEEN_NO_DUP(padTally, 2);*/
						for(l = m->children; l != NULL; l = l->next) {
							if (l->str != NULL) {
								pcb_printf("layer: '%s'\n", (l->str));
							} else {
								return -1;
							}
						}
					} else if (m->str != NULL && strcmp("drill", m->str) == 0) {
						/*SEEN_NO_DUP(padTally, 3);*/
						if (m->children != NULL && m->children->str != NULL) {
							pcb_printf("drill size: '%s'\n", (m->children->str));
						} else {
							return -1;
						}
					} else if (m->str != NULL && strcmp("net", m->str) == 0) {
						/*SEEN_NO_DUP(padTally, 4);*/
						if (m->children != NULL && m->children->str != NULL) {
							pcb_printf("pad's net number:\t'%s'\n", (m->children->str));
							if (m->children->next != NULL && m->children->next->str != NULL) {
								pcb_printf("pad's net name:\t'%s'\n", (m->children->next->str));
							} else {
								return -1;
							}
						} else {
							return -1;
						}
					} else if (m->str != NULL && strcmp("size", m->str) == 0) {
						/*SEEN_NO_DUP(padTally, 5);*/
						if (m->children != NULL && m->children->str != NULL) {
							pcb_printf("pad X size:\t'%s'\n", (m->children->str));
							if (m->children->next != NULL && m->children->next->str != NULL) {
								pcb_printf("pad Y size:\t'%s'\n", (m->children->next->str));
							} else {
								return -1;
							}
						} else {
							return -1;
						}
					} else {
						if (m->str != NULL) {
							printf("Unknown pad argument %s:", m->str);
						}
					} 
				}
			} else if (n->str != NULL && strcmp("fp_line", n->str) == 0) {
					pcb_printf("fp_line found\n");
					kicad_parse_gr_line(st, n->children);
			} else if (n->str != NULL && strcmp("fp_arc", n->str) == 0) {
					pcb_printf("fp_arc found\n");
					kicad_parse_gr_arc(st, n->children);
			} else if (n->str != NULL && strcmp("fp_circle", n->str) == 0) {
					pcb_printf("fp_circle found\n");
					kicad_parse_fp_circle(st, n->children);
			} else if (n->str != NULL && strcmp("fp_text", n->str) == 0) {
					pcb_printf("fp_text found\n");
					kicad_parse_gr_text(st, n->children);
			} else {
				if (n->str != NULL) {
					printf("Unknown pad argument %s:", n->str);
				}
			} 
		}
	} 
	/* required = 1; BV(2) | BV(3) | BV(4) | BV(7) | BV(8);
	if ((tally & required) == required) {  */ /* has location, layer, size and stroke thickness at a minimum */
		return 0;
/*	}
	return -1; */
}

static int kicad_parse_zone(read_state_t *st, gsxl_node_t *subtree)
{

	gsxl_node_t *n, *m;
	int i;
	int polycount = 0;
	long j  = 0;
	unsigned long tally = 0, required;

	if (subtree->str != NULL) {
		printf("Zone element found:\t'%s'\n", subtree->str);		
		for(n = subtree->next,i = 0; n != NULL; n = n->next, i++) {
			if (n->str != NULL && strcmp("net", n->str) == 0) {
				SEEN_NO_DUP(tally, 0);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("zone net number:\t'%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("net_name", n->str) == 0) {
				SEEN_NO_DUP(tally, 1);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("zone net_name:\t'%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("tstamp", n->str) == 0) {
				SEEN_NO_DUP(tally, 2);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("zone tstamp:\t'%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("hatch", n->str) == 0) {
				SEEN_NO_DUP(tally, 3);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("zone hatch_edge:\t'%s'\n", (n->children->str));
					SEEN_NO_DUP(tally, 4); /* same as ^= 1 was */
				} else {
					return -1;
				}
				if (n->children->next != NULL && n->children->next->str != NULL) {
					pcb_printf("zone hatching size:\t'%s'\n", (n->children->next->str));
					SEEN_NO_DUP(tally, 5);	
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("connect_pads", n->str) == 0) {
				SEEN_NO_DUP(tally, 6);
				if (n->children != NULL && n->children->str != NULL && (strcmp("clearance", n->children->str) == 0) && (n->children->children->str != NULL)) {
					pcb_printf("zone clearance:\t'%s'\n", (n->children->children->str));  /* this is if yes/no flag for connected pads is absent */
					SEEN_NO_DUP(tally, 7); /* same as ^= 1 was */
				} else if (n->children != NULL && n->children->str != NULL && n->children->next->str != NULL) {
					pcb_printf("zone connect_pads:\t'%s'\n", (n->children->str));  /* this is if the optional(!) yes or no flag for connected pads is present */
					SEEN_NO_DUP(tally, 7); /* same as ^= 1 was */
					if (n->children->next != NULL && n->children->next->str != NULL && n->children->next->children != NULL && n->children->next->children->str != NULL) {
						if (strcmp("clearance", n->children->next->str) == 0) {
							SEEN_NO_DUP(tally, 8);
							pcb_printf("zone connect_pads clearance: '%s'\n", (n->children->next->children->str));
						} else {
							printf("Unrecognised zone connect_pads option %s\n", n->children->next->str);
						}
					}
				}
			} else if (n->str != NULL && strcmp("layer", n->str) == 0) {
				SEEN_NO_DUP(tally, 9);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("zone layer:\t'%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("polygon", n->str) == 0) {
				printf("Processing polygon [%d] points:\n", polycount);
				polycount++; /*keep track of number of polygons in zone */
				if (n->children != NULL && n->children->str != NULL) {
					if (strcmp("pts", n->children->str) == 0) {
						for (m = n->children->children, j =0; m != NULL; m = m->next, j++) {
							if (m->str != NULL && strcmp("xy", m->str) == 0) {
								if (m->children != NULL && m->children->str != NULL) {
									pcb_printf("vertex X[%d]:\t'%s'\n", j, (m->children->str));
									if (m->children->next != NULL && m->children->next->str != NULL) {
										pcb_printf("vertex Y[%d]:\t'%s'\n", j, (m->children->next->str));
									} else {
										return -1;
									}
								} else {
									return -1;
								}
							}
						}
					} else {
						printf("pts section not found in polygon.\n");
						return -1;
					}
				} else {
					printf("Empty polygon!\n");
					return -1;
				}
			} else if (n->str != NULL && strcmp("fill", n->str) == 0) {
				SEEN_NO_DUP(tally, 10);
				printf("Reading fill settings:\n");
				for (m = n->children; m != NULL; m = m->next) {
					if (m->str != NULL && strcmp("arc_segments", m->str) == 0) {
						if (m->children != NULL && m->children->str != NULL) {
							pcb_printf("zone arc_segments:\t'%s'\n", (m->children->str));
						} else {
							return -1;
						}
					} else if (m->str != NULL && strcmp("thermal_gap", m->str) == 0) {
						if (m->children != NULL && m->children->str != NULL) {
							pcb_printf("zone thermal_gap:\t'%s'\n", (m->children->str));
						} else {
							return -1;
						}
					} else if (m->str != NULL && strcmp("thermal_bridge_width", m->str) == 0) {
						if (m->children != NULL && m->children->str != NULL) {
							pcb_printf("zone thermal_bridge_width:\t'%s'\n", (m->children->str));
						} else {
							return -1;
						}
					} else if (m->str != NULL) {
						printf("Unknown zone fill argument:\t%s\n", m->str);
					}
				}
			} else if (n->str != NULL && strcmp("min_thickness", n->str) == 0) {
				SEEN_NO_DUP(tally, 11);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("zone min_thickness:\t'%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("filled_polygon", n->str) == 0) {
				pcb_printf("Ignoring filled_polygon definition.\n");
			} else {
				if (n->str != NULL) {
					printf("Unknown polygon argument:\t%s\n", n->str);
				}
			} 
		}
	} 
	/* required = 1; BV(2) | BV(3) | BV(4) | BV(7) | BV(8);
	if ((tally & required) == required) {  */ /* has location, layer, size and stroke thickness at a minimum */
		return 0;
/*	}
	return -1; */
}


/* Parse a board from &st->dom into st->PCB */
static int kicad_parse_pcb(read_state_t *st)
{
	gsxl_node_t *n;
	static const dispatch_t disp[] = { /* possible children of root */
		{"version",    kicad_parse_version},
		{"host",       kicad_parse_nop},
		{"general",    kicad_parse_nop},
		{"page",       kicad_parse_page_size},
		{"layers",     kicad_parse_layer_definitions}, /* board layer defs */
		{"setup",      kicad_parse_nop},
		{"net",        kicad_parse_net}, /* net labels if child of root, otherwise net attribute of element */
		{"net_class",  kicad_parse_nop},
		{"module",     kicad_parse_module},  /* for footprints */
		{"gr_line",     kicad_parse_gr_line},
		{"gr_arc",     kicad_parse_gr_arc},
		{"gr_text",    kicad_parse_gr_text},
		{"via",     kicad_parse_via},
		{"segment",     kicad_parse_segment},
		{"zone",     kicad_parse_zone}, /* polygonal zones*/
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
	memset(&st, 0, sizeof(st));
	st.PCB = Ptr;
	st.Filename = Filename;
	st.settings_dest = settings_dest;
	htsi_init(&st.layer_k2i, strhash, strkeyeq);

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

#warning TODO: free the layer hash

printf("readres=%d\n", readres);
	return readres;
}

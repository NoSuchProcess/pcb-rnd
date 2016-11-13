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
#include "config.h"
#include "board.h"
#include "plug_io.h"
#include "error.h"
#include "data.h"
#include "read.h"
#include "layer.h"
#include "const.h"
#include "netlist.h"
#include "polygon.h"
#include "misc_util.h" /* for distance calculations */
#include "conf_core.h"
#include "move.h"
#include "macro.h"
#include "obj_all.h"

typedef struct {
	pcb_board_t *PCB;
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

	char *end, *text;
	double val;
	pcb_coord_t X, Y;
	int scaling = 100;
	int textLength = 0;
	int mirrored = 0;
	double glyphWidth = 1.27; /* a reasonable approximation of pcb glyph width, ~=  5000 centimils */
	unsigned direction = 0; /* default is horizontal */
	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	int PCBLayer = 0; /* sane default value */

	if (subtree->str != NULL) {
		printf("gr_text element being parsed: '%s'\n", subtree->str);
		text = subtree->str;
		for (i = 0; text[i] != 0; i++) {
			textLength++;
		}
		printf("\tgr_text element length: '%d'\n", textLength);
		for(n = subtree,i = 0; n != NULL; n = n->next, i++) {
			if (n->str != NULL && strcmp("at", n->str) == 0) {
					SEEN_NO_DUP(tally, 0);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\ttext at x: '%s'\n", (n->children->str));
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
						pcb_printf("\ttext at y: '%s'\n", (n->children->next->str));
						val = strtod(n->children->next->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							Y = PCB_MM_TO_COORD(val);
						}	
						if (n->children->next->next != NULL && n->children->next->next->str != NULL) {
							pcb_printf("\ttext rotation: '%s'\n", (n->children->next->next->str));
							val = strtod(n->children->next->next->str, &end);
							if (*end != 0) {
								return -1;
							} else {
								direction = 0;  /* default */
								if (val > 45.0 && val <= 135.0) {
									direction = 1;
								} else if (val > 135.0 && val <= 225.0) {
									direction = 2;
								} else if (val > 225.0 && val <= 315.0) {
									direction = 3;
								}
								printf("\tkicad angle: %f,   Direction %d\n", val, direction);
							}
						} 
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("layer", n->str) == 0) {
				SEEN_NO_DUP(tally, 1);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\ttext layer: '%s'\n", (n->children->str));
					PCBLayer = kicad_get_layeridx(st, n->children->str);
					if (PCBLayer == -1) {
						return -1;
					} else if (pcb_layer_flags(PCBLayer) & PCB_LYT_BOTTOM) {
							Flags = pcb_flag_make(PCB_FLAG_ONSOLDER);
					}
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("effects", n->str) == 0) {
				for(m = n->children; m != NULL; m = m->next) {
					printf("\tstepping through text effects definitions\n"); 
					if (m->str != NULL && strcmp("font", m->str) == 0) {
						for(l = m->children; l != NULL; l = l->next) {
							if (m->str != NULL && strcmp("size", l->str) == 0) {
								SEEN_NO_DUP(tally, 2);
								if (l->children != NULL && l->children->str != NULL) {
									pcb_printf("\tfont sizeX: '%s'\n", (l->children->str));
									val = strtod(l->children->str, &end);
									if (*end != 0) {
										return -1;
									} else {
										scaling = (int) (100*val/1.27); /* standard glyph width ~= 1.27mm */
									}
								} else {
									return -1;
								}
								if (l->children->next != NULL && l->children->next->str != NULL) {
									pcb_printf("\tfont sizeY: '%s'\n", (l->children->next->str));
								} else {
									return -1;
								}
							} else if (strcmp("thickness", l->str) == 0) {
								SEEN_NO_DUP(tally, 3);
								if (l->children != NULL && l->children->str != NULL) {
									pcb_printf("\tfont thickness: '%s'\n", (l->children->str));
								} else {
									return -1;
								}
							}
						}
					} else if (m->str != NULL && strcmp("justify", m->str) == 0) {
						SEEN_NO_DUP(tally, 4);
						if (m->children != NULL && m->children->str != NULL) {
							pcb_printf("\ttext justification: '%s'\n", (m->children->str));
							if (strcmp("mirror", m->children->str) == 0) {
								mirrored = 1;
							}
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
	required = BV(0) | BV(1) | BV(2) | BV(3);
	if ((tally & required) == required) { /* has location, layer, size and stroke thickness at a minimum */
		if (&st->PCB->Font == NULL) {
			CreateDefaultFont(st->PCB);
		}

		if (mirrored != 0) {
			if (direction%2 == 0) {
				direction += 2;
				direction = direction%4;
			}
			if (direction == 0 ) {
				X -= PCB_MM_TO_COORD((glyphWidth * textLength)/2.0);
				Y += PCB_MM_TO_COORD(glyphWidth/2.0); /* centre it vertically */
			} else if (direction == 1 ) {
				Y -= PCB_MM_TO_COORD((glyphWidth * textLength)/2.0);
				X -= PCB_MM_TO_COORD(glyphWidth/2.0); /* centre it vertically */
			} else if (direction == 2 ) {
				X += PCB_MM_TO_COORD((glyphWidth * textLength)/2.0);
				Y -= PCB_MM_TO_COORD(glyphWidth/2.0);  /* centre it vertically */
			} else if (direction == 3 ) {
				Y += PCB_MM_TO_COORD((glyphWidth * textLength)/2.0);
				X += PCB_MM_TO_COORD(glyphWidth/2.0); /* centre it vertically */
			}
		} else { /* not back of board text */
			if (direction == 0 ) {
				X -= PCB_MM_TO_COORD((glyphWidth * textLength)/2.0);
				Y -= PCB_MM_TO_COORD(glyphWidth/2.0); /* centre it vertically */
			} else if (direction == 1 ) {
				Y += PCB_MM_TO_COORD((glyphWidth * textLength)/2.0);
				X -= PCB_MM_TO_COORD(glyphWidth/2.0); /* centre it vertically */
			} else if (direction == 2 ) {
				X += PCB_MM_TO_COORD((glyphWidth * textLength)/2.0);
				Y += PCB_MM_TO_COORD(glyphWidth/2.0);  /* centre it vertically */
			} else if (direction == 3 ) {
				Y -= PCB_MM_TO_COORD((glyphWidth * textLength)/2.0);
				X += PCB_MM_TO_COORD(glyphWidth/2.0); /* centre it vertically */
			}
		}

		CreateNewText( &st->PCB->Data->Layer[PCBLayer], &st->PCB->Font, X, Y, direction, scaling, text, Flags);
		return 0; /* create new font */
	}
	return -1;
}

/* kicad_pcb/gr_line */
static int kicad_parse_gr_line(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *n;
	unsigned long tally = 0, required;

	char *end;
	double val;
	pcb_coord_t X1, Y1, X2, Y2, Thickness, Clearance; /* not sure what to do with mask */
	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	int PCBLayer = 0; /* sane default value */

	Clearance = Thickness = PCB_MM_TO_COORD(0.250); /* start with sane defaults */

	printf("gr_line parsing about to commence:\n");

	if (subtree->str != NULL) {
		for(n = subtree; n != NULL; n = n->next) {
			if (n->str != NULL && strcmp("start", n->str) == 0) {
					SEEN_NO_DUP(tally, 0);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tgr_line start at x: '%s'\n", (n->children->str));
						val = strtod(n->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							X1 = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
					if (n->children->next != NULL && n->children->next->str != NULL) {
						pcb_printf("\tgr_line start at y: '%s'\n", (n->children->next->str));
						val = strtod(n->children->next->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							Y1 = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("end", n->str) == 0) {
					SEEN_NO_DUP(tally, 1);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tgr_line end at x: '%s'\n", (n->children->str));
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
						pcb_printf("\tgr_line end at y: '%s'\n", (n->children->next->str));
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
					SEEN_NO_DUP(tally, 2);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tgr_line layer: '%s'\n", (n->children->str));
						PCBLayer = kicad_get_layeridx(st, n->children->str);
						if (PCBLayer == -1) {
							return -1;
						}
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("width", n->str) == 0) {
					SEEN_NO_DUP(tally, 3);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tgr_line width: '%s'\n", (n->children->str));
						val = strtod(n->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							Thickness = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("angle", n->str) == 0) { /* unlikely to be used or seen */
					SEEN_NO_DUP(tally, 4);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tgr_line angle: '%s'\n", (n->children->str));
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("net", n->str) == 0) { /* unlikely to be used or seen */
					SEEN_NO_DUP(tally, 5);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tgr_line net: '%s'\n", (n->children->str));
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
        required = BV(0) | BV(1) | BV(2) | BV(3);
        if ((tally & required) == required) { /* need start, end, layer, thickness at a minimum */
		CreateNewLineOnLayer( &st->PCB->Data->Layer[PCBLayer], X1, Y1, X2, Y2, Thickness, Clearance, Flags);
		pcb_printf("\tnew gr_line on layer created\n");
		return 0;
	}
	return -1;
}

/* kicad_pcb/gr_arc     can also parse gr_cicle*/
static int kicad_parse_gr_arc(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *n;
	unsigned long tally = 0, required;

	char *end;
	double val;
	pcb_coord_t centreX, centreY, endX, endY, width, height, Thickness, Clearance;
	pcb_angle_t startAngle = 0.0;
	pcb_angle_t delta = 360.0; /* these defaults allow a gr_circle to be parsed, which does not specify (angle XXX) */
	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	int PCBLayer = 0; /* sane default value */

	Clearance = Thickness = PCB_MM_TO_COORD(0.250); /* start with sane defaults */

	if (subtree->str != NULL) {
		printf("gr_arc being parsed:\n");		
		for(n = subtree; n != NULL; n = n->next) {
			if (n->str != NULL && strcmp("start", n->str) == 0) {
					SEEN_NO_DUP(tally, 0);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tgr_arc centre at x: '%s'\n", (n->children->str));
						val = strtod(n->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							centreX = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
					if (n->children->next != NULL && n->children->next->str != NULL) {
						pcb_printf("\tgr_arc centre at y: '%s'\n", (n->children->next->str));
						val = strtod(n->children->next->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							centreY = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("center", n->str) == 0) { /* this lets us parse a circle too */
					SEEN_NO_DUP(tally, 0);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tgr_arc centre at x: '%s'\n", (n->children->str));
						val = strtod(n->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							centreX = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
					if (n->children->next != NULL && n->children->next->str != NULL) {
						pcb_printf("\tgr_arc centre at y: '%s'\n", (n->children->next->str));
						val = strtod(n->children->next->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							centreY = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("end", n->str) == 0) {
					SEEN_NO_DUP(tally, 1);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tgr_arc end at x: '%s'\n", (n->children->str));
						val = strtod(n->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							endX = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
					if (n->children->next != NULL && n->children->next->str != NULL) {
						pcb_printf("\tgr_arc end at y: '%s'\n", (n->children->next->str));
						val = strtod(n->children->next->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							endY = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("layer", n->str) == 0) {
					SEEN_NO_DUP(tally, 2);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tgr_arc layer: '%s'\n", (n->children->str));
						PCBLayer = kicad_get_layeridx(st, n->children->str);
						if (PCBLayer == -1) {
							return -1;
						}
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("width", n->str) == 0) {
					SEEN_NO_DUP(tally, 3);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tgr_arc width: '%s'\n", (n->children->str));
						val = strtod(n->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							Thickness = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("angle", n->str) == 0) {
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tgr_arc angle CW rotation: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 4);
						val = strtod(n->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							delta = val;
						}
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("net", n->str) == 0) { /* unlikely to be used or seen */
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tgr_arc net: '%s'\n", (n->children->str));
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
        required = BV(0) | BV(1) | BV(2) | BV(3); /* | BV(4); not needed for circles */
        if ((tally & required) == required) { /* need start, end, layer, thickness at a minimum */
		width = height = Distance(centreX, centreY, endX, endY); /* calculate radius of arc */
		if (width < 1) { /* degenerate case */
			startAngle = 0;
		} else {
			startAngle = 180*atan2(endY - centreY, endX - centreX)/M_PI;
			/* avoid using atan2 with zero parameters */
		}
		CreateNewArcOnLayer( &st->PCB->Data->Layer[PCBLayer], centreX, centreY, width, height, startAngle, -delta, Thickness, Clearance, Flags);
		return 0;
	}
	return -1;
}

/* kicad_pcb/via */
static int kicad_parse_via(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *m, *n;
	unsigned long tally = 0, required;

	char *end, *name; /* not using via name for now */
	double val;
	pcb_coord_t X, Y, Thickness, Clearance, Mask, Drill; /* not sure what to do with mask */
	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
/*	int PCBLayer = 0;   not used for now; no blind or buried vias currently in pcb-rnd */

	Clearance = Mask = PCB_MM_TO_COORD(0.250); /* start with something bland here */
	Drill = PCB_MM_TO_COORD(0.300); /* start with something sane */
	name = "";

	if (subtree->str != NULL) {
		printf("via being parsed:\n");		
		for(n = subtree; n != NULL; n = n->next) {
			if (n->str != NULL && strcmp("at", n->str) == 0) {
					SEEN_NO_DUP(tally, 0);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tvia at x: '%s'\n", (n->children->str));
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
						pcb_printf("\tvia at y: '%s'\n", (n->children->next->str));
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
					SEEN_NO_DUP(tally, 1);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tvia size: '%s'\n", (n->children->str));
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
					SEEN_NO_DUP(tally, 2);
					for(m = n->children; m != NULL; m = m->next) {
						if (m->str != NULL) {
							pcb_printf("\tvia layer: '%s'\n", (m->str));
/*							PCBLayer = kicad_get_layeridx(st, m->str);
 *							if (PCBLayer == -1) {
 *								return -1;
 *							}   via layers not currently used in PCB
 */   
						} else {
							return -1;
						}
					}
			} else if (n->str != NULL && strcmp("net", n->str) == 0) {
					SEEN_NO_DUP(tally, 3);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tvia segment net: '%s'\n", (n->children->str));
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
        required = BV(0) | BV(1);
        if ((tally & required) == required) { /* need start, end, layer, thickness at a minimum */
		CreateNewVia( st->PCB->Data, X, Y, Thickness, Clearance, Mask, Drill, name, Flags);
		return 0;
	}
	return -1;
}

/* kicad_pcb/segment */
static int kicad_parse_segment(read_state_t *st, gsxl_node_t *subtree)
{

	gsxl_node_t *n;
	unsigned long tally = 0, required;

	char *end;
	double val;
	pcb_coord_t X1, Y1, X2, Y2, Thickness, Clearance;
	pcb_flag_t Flags = pcb_flag_make(PCB_FLAG_CLEARLINE); /* we try clearline flag here */
	int PCBLayer = 0; /* sane default value */

	Clearance = PCB_MM_TO_COORD(0.250); /* start with something bland here */

	if (subtree->str != NULL) {
		printf("segment being parsed:\n");		
		for(n = subtree; n != NULL; n = n->next) {
			if (n->str != NULL && strcmp("start", n->str) == 0) {
					SEEN_NO_DUP(tally, 0);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tsegment start at x: '%s'\n", (n->children->str));
						val = strtod(n->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							X1 = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
					if (n->children->next != NULL && n->children->next->str != NULL) {
						pcb_printf("\tsegment start at y: '%s'\n", (n->children->next->str));
						val = strtod(n->children->next->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							Y1 = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("end", n->str) == 0) {
					SEEN_NO_DUP(tally, 1);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tsegment end at x: '%s'\n", (n->children->str));
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
						pcb_printf("\tsegment end at y: '%s'\n", (n->children->next->str));
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
					SEEN_NO_DUP(tally, 2);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tsegment layer: '%s'\n", (n->children->str));
						PCBLayer = kicad_get_layeridx(st, n->children->str);
						if (PCBLayer == -1) {
							return -1;
						}
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("width", n->str) == 0) {
					SEEN_NO_DUP(tally, 3);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tsegment width: '%s'\n", (n->children->str));
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
					SEEN_NO_DUP(tally, 4);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tsegment net: '%s'\n", (n->children->str));
						SEEN_NO_DUP(tally, 11);
					} else {
						return -1;
					}
			} else if (n->str != NULL && strcmp("tstamp", n->str) == 0) { /* not likely to be used */
					SEEN_NO_DUP(tally, 5);
					if (n->children != NULL && n->children->str != NULL) {
						pcb_printf("\tsegment timestamp: '%s'\n", (n->children->str));
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
        required = BV(0) | BV(1) | BV(2) | BV(3);
        if ((tally & required) == required) { /* need start, end, layer, thickness at a minimum */
		CreateNewLineOnLayer( &st->PCB->Data->Layer[PCBLayer], X1, Y1, X2, Y2, Thickness, Clearance, Flags);
		pcb_printf("\tnew segment on layer created\n");
		return 0;
	}
	return -1;
}

/* Parse a layer definition and do all the administration needed for the layer */
static int kicad_create_layer(read_state_t *st, int lnum, const char *lname, const char *ltype)
{
	int id = -1;
	switch(lnum) {
		case 0:
			id = SOLDER_LAYER;
			pcb_layer_rename(id, lname);
			break;
		case 15:
			id = COMPONENT_LAYER;
			pcb_layer_rename(id, lname);
			break;
		default:
			if (strcmp(lname, "Edge.Cuts") == 0) {
				/* Edge must be the outline */
				id = pcb_layer_create(PCB_LYT_OUTLINE, 0, 0, "outline");
			}
			else if ((strcmp(ltype, "signal") == 0) || (strncmp(lname, "Dwgs.", 4) == 0) || (strncmp(lname, "Cmts.", 4) == 0) || (strncmp(lname, "Eco", 3) == 0)) {
				/* Create a new inner layer for signals and for emulating misc layers */
				id = pcb_layer_create(PCB_LYT_INTERN | PCB_LYT_COPPER, 0, 0, lname);
			}
#if 0
			else if ((lname[1] == '.') && ((lname[0] == 'F') || (lname[0] == 'B'))) {
				/* F. or B. layers */
				if (strcmp(lname+2, "SilkS") == 0) return 0; /* silk layers are implicit */
				if (strcmp(lname+2, "Adhes") == 0) return 0; /* pcb-rnd has no adhesive support */
				if (strcmp(lname+2, "Paste") == 0) return 0; /* pcb-rnd has no custom paste support */
				if (strcmp(lname+2, "Mask") == 0)  return 0; /* pcb-rnd has no custom mask support */
				return -1; /* unknown F. or B. layer -> error */
			}
#endif
			else if (lnum > 15) {
				/* HACK/WORKAROUND: remember kicad layers for those that are unsupported */
				htsi_set(&st->layer_k2i, pcb_strdup(lname), -lnum);
				return 0;
			}
			else
				return -1; /* unknown field */
	}

/* valid layer, save it in the hash */
	if (id >= 0) {
	htsi_set(&st->layer_k2i, pcb_strdup(lname), id);
	} else {
		assert(id < -1);
	}
	return 0;
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
	gsxl_node_t *n;
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
					pcb_printf("\tlayer #%d LAYERNUM found:\t%s\n", i, n->str);
					pcb_printf("\tlayer #%d layer label found:\t%s\n", i, lname);
					pcb_printf("\tlayer #%d layer description/type found:\t%s\n", i, ltype);
					if (kicad_create_layer(st, lnum, lname, ltype) < 0) {
						pcb_message(PCB_MSG_ERROR, "Unrecognized layer: %d, %s, %s\n", lnum, lname, ltype);
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
				pcb_message(PCB_MSG_ERROR, "Internal error: can't find a silk or mask layer\n");
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
			pcb_printf("\tcorresponding net label: '%s'\n", (subtree->next->str));
		} else {
			pcb_printf("missing net label in net descriptors");
			return -1;
		}
		return 0;
}

static int kicad_parse_module(read_state_t *st, gsxl_node_t *subtree)
{

	gsxl_node_t *l, *n, *m, *p;
	int i;
	int scaling = 100;
	int textLength = 0;
	int mirrored = 0;
	int moduleDefined = 0;
	int PCBLayer = 0;
	int kicadLayer = 15; /* default = top side */
	int SMD = 0;
	int square = 0;
	int throughHole = 0;
	int foundRefdes = 0;
	int refdesScaling  = 100;
	unsigned long tally = 0, featureTally, required;
	pcb_coord_t moduleX, moduleY, X, Y, X1, Y1, X2, Y2, centreX, centreY, endX, endY, width, height, Thickness, Clearance, padXsize, padYsize, drill, refdesX, refdesY;
	pcb_angle_t startAngle = 0.0;
	pcb_angle_t endAngle = 0.0;
	pcb_angle_t delta = 360.0; /* these defaults allow a fp_circle to be parsed, which does not specify (angle XXX) */
	double val;
	double glyphWidth = 1.27; /* a reasonable approximation of pcb glyph width, ~=  5000 centimils */
	unsigned direction = 0; /* default is horizontal */
	char * end, * textLabel, * text;
	char * moduleName, * moduleRefdes, * moduleValue, * pinName;
	pcb_element_t *newModule;

	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	pcb_flag_t TextFlags = pcb_flag_make(0); /* start with something bland here */
	Clearance = PCB_MM_TO_COORD(0.250); /* start with something bland here */

	moduleName = moduleRefdes = moduleValue = NULL;

	if (subtree->str != NULL) {
		printf("Name of module element being parsed: '%s'\n", subtree->str);
		moduleName = subtree->str;
		SEEN_NO_DUP(tally, 0);
		for(n = subtree->next, i = 0; n != NULL; n = n->next, i++) {
			if (n->str != NULL && strcmp("layer", n->str) == 0) { /* need this to sort out ONSOLDER flags etc... */
				SEEN_NO_DUP(tally, 1);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\tlayer: '%s'\n", (n->children->str));
					PCBLayer = kicad_get_layeridx(st, n->children->str);
					if (PCBLayer == -1) {
						return -1;
					} else if (pcb_layer_flags(PCBLayer) & PCB_LYT_BOTTOM) {
							Flags = pcb_flag_make(PCB_FLAG_ONSOLDER);
							TextFlags = pcb_flag_make(PCB_FLAG_ONSOLDER);
					}
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("tedit", n->str) == 0) {
				SEEN_NO_DUP(tally, 2);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\ttedit: '%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("tstamp", n->str) == 0) {
				SEEN_NO_DUP(tally, 3);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\ttstamp: '%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("at", n->str) == 0) {
				SEEN_NO_DUP(tally, 4);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\tat x: '%s'\n", (n->children->str));
					SEEN_NO_DUP(tally, 5); /* same as ^= 1 was */
						val = strtod(n->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							moduleX = PCB_MM_TO_COORD(val);
						}
				} else {
					return -1;
				}
				if (n->children->next != NULL && n->children->next->str != NULL) {
					pcb_printf("\tat y: '%s'\n", (n->children->next->str));
					SEEN_NO_DUP(tally, 6);	
						val = strtod(n->children->next->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							moduleY = PCB_MM_TO_COORD(val);
						}
				} else {
					return -1;
				}

			} else if (n->str != NULL && strcmp("fp_text", n->str) == 0) {
					pcb_printf("fp_text found\n");
					featureTally = 0;

/* ********************************************************** */


	if (n->children != NULL && n->children->str != NULL) {
		textLabel = n->children->str;
		printf("fp_text element being parsed for %s - label: '%s'\n", moduleName, textLabel);
		if (n->children->next != NULL && n->children->next->str != NULL) {
			text = n->children->next->str;
			foundRefdes = 0;
			if (strcmp("reference", textLabel) == 0) {
				SEEN_NO_DUP(tally, 7);
				printf("\tfp_text reference found: '%s'\n", textLabel);
				moduleRefdes = text;
				foundRefdes = 1;
				for (i = 0; text[i] != 0; i++) {
					textLength++;
				}
				printf("\tmoduleRefdes now: '%s'\n", moduleRefdes);
			} else if (strcmp("value", textLabel) == 0) {
				SEEN_NO_DUP(tally, 8);
				printf("\tfp_text value found: '%s'\n", textLabel);
				moduleValue = text;
				foundRefdes = 0;
				printf("\tmoduleValue now: '%s'\n", moduleValue);
			} else {
				foundRefdes = 0;
			}
		} else {
			text = textLabel; /* just a single string, no reference or value */ 
		}

		printf("\tfp_text element length: '%d'\n", textLength);
		for(l = n->children->next->next, i = 0; l != NULL; l = l->next, i++) { /*fixed this */
			if (l->str != NULL && strcmp("at", l->str) == 0) {
					SEEN_NO_DUP(featureTally, 0);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\ttext at x: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 1);
						val = strtod(l->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							X = PCB_MM_TO_COORD(val);
							if (foundRefdes) {
								refdesX = X;
								pcb_printf("\tRefdesX = %mm", refdesX);

							}
						}
					} else {
						return -1;
					}
					if (l->children->next != NULL && l->children->next->str != NULL) {
						pcb_printf("\ttext at y: '%s'\n", (l->children->next->str));
						SEEN_NO_DUP(featureTally, 2);
						val = strtod(l->children->next->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							Y = PCB_MM_TO_COORD(val);
							if (foundRefdes) {
								refdesY = Y;
								pcb_printf("\tRefdesX = %mm", refdesY);
							}
						}	
						if (l->children->next->next != NULL && l->children->next->next->str != NULL) {
							pcb_printf("\ttext rotation: '%s'\n", (l->children->next->next->str));
							val = strtod(l->children->next->next->str, &end);
							if (*end != 0) {
								return -1;
							} else {
								direction = 0;  /* default */
								if (val > 45.0 && val <= 135.0) {
									direction = 1;
								} else if (val > 135.0 && val <= 225.0) {
									direction = 2;
								} else if (val > 225.0 && val <= 315.0) {
									direction = 3;
								}
								printf("\tkicad angle: %f,   Direction %d\n", val, direction);
							}
							SEEN_NO_DUP(featureTally, 3);
						} 
					} else {
						return -1;
					}
			} else if (l->str != NULL && strcmp("layer", l->str) == 0) {
				SEEN_NO_DUP(featureTally, 4);
				if (l->children != NULL && l->children->str != NULL) {
					pcb_printf("\ttext layer: '%s'\n", (l->children->str));
					PCBLayer = kicad_get_layeridx(st, l->children->str);
					if (PCBLayer == -1) {
						return -1;
					} else if (pcb_layer_flags(PCBLayer) & PCB_LYT_BOTTOM) {
							Flags = pcb_flag_make(PCB_FLAG_ONSOLDER);
					}
				} else {
					return -1;
				}
			} else if (l->str != NULL && strcmp("effects", l->str) == 0) {
				SEEN_NO_DUP(featureTally, 5);
				for(m = l->children; m != NULL; m = m->next) {
					/*printf("\tstepping through effects def, looking at %s\n", m->str);*/ 
					if (m->str != NULL && strcmp("font", m->str) == 0) {
						SEEN_NO_DUP(featureTally, 6);
						for(p = m->children; p != NULL; p = p->next) {
							if (m->str != NULL && strcmp("size", p->str) == 0) {
								SEEN_NO_DUP(featureTally, 7);
								if (p->children != NULL && p->children->str != NULL) {
									pcb_printf("\tfont sizeX: '%s'\n", (p->children->str));
									val = strtod(p->children->str, &end);
									if (*end != 0) {
										return -1;
									} else {
										scaling = (int) (100*val/1.27); /* standard glyph width ~= 1.27mm */
										if (foundRefdes) {
											refdesScaling = scaling;
											foundRefdes = 0;
										}
									}
								} else {
									return -1;
								}
								if (p->children->next != NULL && p->children->next->str != NULL) {
									pcb_printf("\tfont sizeY: '%s'\n", (p->children->next->str));
								} else {
									return -1;
								}
							} else if (strcmp("thickness", p->str) == 0) {
								SEEN_NO_DUP(featureTally, 8);
								if (p->children != NULL && p->children->str != NULL) {
									pcb_printf("\tfont thickness: '%s'\n", (p->children->str));
								} else {
									return -1;
								}
							}
						}
					} else if (m->str != NULL && strcmp("justify", m->str) == 0) {
						SEEN_NO_DUP(featureTally, 9);
						if (m->children != NULL && m->children->str != NULL) {
							pcb_printf("\ttext justification: '%s'\n", (m->children->str));
							if (strcmp("mirror", m->children->str) == 0) {
								mirrored = 1;
							}
						} else {
							return -1;
						}
					} else {
						if (m->str != NULL) {
							printf("Unknown text effects argument %s:", m->str);
						}
						return -1;	
					}
				}
			} 				
		}
	}
	required = BV(0) | BV(1) | BV(4) | BV(7) | BV(8);
	if ((tally & required) == required) { /* has location, layer, size and stroke thickness at a minimum */
		if (&st->PCB->Font == NULL) {
			CreateDefaultFont(st->PCB);
		}

		X = refdesX;
		Y = refdesY;
		glyphWidth = 1.27;
		glyphWidth = glyphWidth * refdesScaling/100.0;

		if (mirrored != 0) {
			if (direction%2 == 0) {
				direction += 2;
				direction = direction%4;
			}
			if (direction == 0 ) {
				X -= PCB_MM_TO_COORD((glyphWidth * textLength)/2.0);
				Y += PCB_MM_TO_COORD(glyphWidth/2.0); /* centre it vertically */
			} else if (direction == 1 ) {
				Y -= PCB_MM_TO_COORD((glyphWidth * textLength)/2.0);
				X -= PCB_MM_TO_COORD(glyphWidth/2.0); /* centre it vertically */
			} else if (direction == 2 ) {
				X += PCB_MM_TO_COORD((glyphWidth * textLength)/2.0);
				Y -= PCB_MM_TO_COORD(glyphWidth/2.0);  /* centre it vertically */
			} else if (direction == 3 ) {
				Y += PCB_MM_TO_COORD((glyphWidth * textLength)/2.0);
				X += PCB_MM_TO_COORD(glyphWidth/2.0); /* centre it vertically */
			}
		} else { /* not back of board text */
			if (direction == 0 ) {
				X -= PCB_MM_TO_COORD((glyphWidth * textLength)/2.0);
				Y -= PCB_MM_TO_COORD(glyphWidth/2.0); /* centre it vertically */
			} else if (direction == 1 ) {
				Y += PCB_MM_TO_COORD((glyphWidth * textLength)/2.0);
				X -= PCB_MM_TO_COORD(glyphWidth/2.0); /* centre it vertically */
			} else if (direction == 2 ) {
				X += PCB_MM_TO_COORD((glyphWidth * textLength)/2.0);
				Y += PCB_MM_TO_COORD(glyphWidth/2.0);  /* centre it vertically */
			} else if (direction == 3 ) {
				Y -= PCB_MM_TO_COORD((glyphWidth * textLength)/2.0);
				X += PCB_MM_TO_COORD(glyphWidth/2.0); /* centre it vertically */
			}
		}

		if (moduleValue != NULL && moduleRefdes != NULL && moduleName != NULL && moduleDefined == 0) {
			moduleDefined = 1;
			printf("now have RefDes %s and Value %s, can now define module/element %s\n", moduleRefdes, moduleValue, moduleName);
			newModule = CreateNewElement(st->PCB->Data, NULL,
								 &st->PCB->Font, Flags,
								 moduleName, moduleRefdes, moduleValue,
								 moduleX, moduleY, direction,
								 refdesScaling, TextFlags,  pcb_false); /*pcb_flag_t TextFlags, pcb_bool uniqueName) */
			MoveObject(PCB_TYPE_ELEMENT_NAME, newModule,  &newModule->Name[NAME_INDEX()],  &newModule->Name[NAME_INDEX()], X, Y);
		}
	}


/* ********************************************************** */

			} else if (n->str != NULL && strcmp("descr", n->str) == 0) {
				SEEN_NO_DUP(tally, 9);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\tmodule descr: '%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("tags", n->str) == 0) {
				SEEN_NO_DUP(tally, 10);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\tmodule tags: '%s'\n", (n->children->str)); /* maye be more than one? */
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("path", n->str) == 0) {
				SEEN_NO_DUP(tally, 11);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\tmodule path: '%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("model", n->str) == 0) {
				SEEN_NO_DUP(tally, 12);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\tmodule model provided: '%s'\n", (n->children->str));
				} else {
					return -1;
				}
			/* pads next  - have thru_hole, circle, rect, roundrect, to think about*/ 
			} else if (n->str != NULL && strcmp("pad", n->str) == 0) {
				featureTally = 0;
				if (n->children != 0 && n->children->str != NULL) {
					printf("pad name found: %s\n", n->children->str);
					pinName = n->children->str;
					SEEN_NO_DUP(featureTally, 0);
					if (n->children->next != NULL && n->children->next->str != NULL) {
						pcb_printf("\tpad type: '%s'\n", (n->children->next->str));
						if (strcmp("thru_hole", n->children->next->str) == 0) {
							SMD = 0;
							throughHole = 1;
						} else {
							SMD = 1;
							throughHole = 0;
						}
						if (n->children->next->next != NULL && n->children->next->next->str != NULL) {
							pcb_printf("\tpad shape: '%s'\n", (n->children->next->next->str));
							if (strcmp("circle", n->children->next->next->str) == 0) {
								square = 0;
							} else {
								square = 1; /* this will catch obround, roundrect, trapezoidal as well. Kicad does not do octagonal pads */
							}
						} else {
							return -1;
						}
					} else {
						return -1;
					}
				} else {
					return -1;
				}
				if (n->children->next->next->next == NULL || n->children->next->next->next->str == NULL) {
					return -1;
				}
				for (m = n->children->next->next->next; m != NULL; m = m->next) {
					if (m != NULL) {
						printf("\tstepping through module pad defs, looking at: %s\n", m->str);
					} else {
						printf("error in pad def\n");
					}
					if (m->str != NULL && strcmp("at", m->str) == 0) {
						SEEN_NO_DUP(featureTally, 1);
						if (m->children != NULL && m->children->str != NULL) {
							pcb_printf("\tpad X position:\t'%s'\n", (m->children->str));
							val = strtod(m->children->str, &end);
							if (*end != 0) {
								return -1;
							} else {
								X = PCB_MM_TO_COORD(val);
							}
							if (m->children->next != NULL && m->children->next->str != NULL) {
								pcb_printf("\tpad Y position:\t'%s'\n", (m->children->next->str));
								val = strtod(m->children->next->str, &end);
								if (*end != 0) {
									return -1;
								} else {
									Y = PCB_MM_TO_COORD(val);
								}
							} else {
								return -1;
							}
						} else {
							return -1;
						}
					} else if (m->str != NULL && strcmp("layers", m->str) == 0) {
						if (SMD) { /* skip testing for pins */
							SEEN_NO_DUP(featureTally, 2);
							kicadLayer = 15;
							for(l = m->children; l != NULL; l = l->next) {
								if (l->str != NULL) {
									PCBLayer = kicad_get_layeridx(st, l->str);
									if (PCBLayer == -1) {
										printf("Unknown layer definition: %s", l->str);
										return -1;
									} else if (PCBLayer < -1) {
										printf("\tUnimplemented layer definition: %s", l->str);
									} else if (pcb_layer_flags(PCBLayer) & PCB_LYT_BOTTOM) {
										Flags = pcb_flag_make(PCB_FLAG_ONSOLDER);
										kicadLayer = 0;
									}
									pcb_printf("\tpad layer: '%s',  PCB layer number %d\n", (l->str), kicad_get_layeridx(st, l->str));
								} else {
									return -1;
								}
							}
						} else {	
							printf("\tIgnoring layer definitions for through hole pin\n");
						}
					} else if (m->str != NULL && strcmp("drill", m->str) == 0) {
						SEEN_NO_DUP(featureTally, 3);
						if (m->children != NULL && m->children->str != NULL) {
							pcb_printf("\tdrill size: '%s'\n", (m->children->str));
							val = strtod(m->children->str, &end);
							if (*end != 0) {
								return -1;
							} else {
								drill = PCB_MM_TO_COORD(val);
							}

						} else {
							return -1;
						}
					} else if (m->str != NULL && strcmp("net", m->str) == 0) { 
						SEEN_NO_DUP(featureTally, 4);
						if (m->children != NULL && m->children->str != NULL) {
							pcb_printf("\tpad's net number:\t'%s'\n", (m->children->str));
							if (m->children->next != NULL && m->children->next->str != NULL) {
								pcb_printf("\tpad's net name:\t'%s'\n", (m->children->next->str));
							} else {
								return -1;
							}
						} else {
							return -1;
						}
					} else if (m->str != NULL && strcmp("size", m->str) == 0) {
						SEEN_NO_DUP(featureTally, 5);
						if (m->children != NULL && m->children->str != NULL) {
							pcb_printf("\tpad X size:\t'%s'\n", (m->children->str));
							val = strtod(m->children->str, &end);
							if (*end != 0) {
								return -1;
							} else {
								padXsize = PCB_MM_TO_COORD(val);
							}
							if (m->children->next != NULL && m->children->next->str != NULL) {
								pcb_printf("\tpad Y size:\t'%s'\n", (m->children->next->str));
								val = strtod(m->children->next->str, &end);
								if (*end != 0) {
									return -1;
								} else {
									padYsize = PCB_MM_TO_COORD(val);
								}
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
					printf("\tFinished stepping through pad args\n");
				}
				printf("\tfinished pad parse\n");
				if (throughHole == 1  &&  newModule != NULL) {
					printf("\tcreating new pin %s in element\n", pinName);
					required = BV(0) | BV(1) | BV(3) | BV(5);
        				if ((featureTally & required) == required) {
						CreateNewPin(newModule, X + moduleX, Y + moduleY, padXsize, Clearance,
								Clearance, drill, pinName, pinName, Flags); /* using clearance value for arg 5 = mask too */
					} else {
						return -1;
					}
				} else if (newModule != NULL) {
					printf("\tcreating new pad %s in element\n", pinName);
                                        required = BV(0) | BV(1) | BV(2) | BV(5);
                                        if ((featureTally & required) == required) {	
						if (padXsize >= padYsize) { /* square pad or rectangular pad, wider than tall */
							Y1 = Y2 = Y;
							X1 = X - (padXsize - padYsize)/2;
							X2 = X + (padXsize - padYsize)/2;
							Thickness = padYsize;
						} else { /* rectangular pad, taller than wide */
							X1 = X2 = X;
							Y1 = Y - (padYsize - padXsize)/2;
							Y2 = Y + (padYsize - padXsize)/2;
							Thickness = padXsize;
						}
						if (square && kicadLayer) {
							Flags = pcb_flag_make(PCB_FLAG_SQUARE);
						} else if (kicadLayer) {
							Flags = pcb_flag_make(0);
						} else if (square && !kicadLayer) {
							Flags = pcb_flag_make(PCB_FLAG_SQUARE | PCB_FLAG_ONSOLDER);
						} else {
							Flags = pcb_flag_make(PCB_FLAG_ONSOLDER);
						}
						CreateNewPad(newModule, X1 + moduleX, Y1 + moduleY, X2 + moduleX, Y2 + moduleY, Thickness, Clearance, 
								Clearance, pinName, pinName, Flags); /* using clearance value for arg 7 = mask too */
					} else {
						return -1;
					}
				} else {
					return -1;
				}

			} else if (n->str != NULL && strcmp("fp_line", n->str) == 0) {
					pcb_printf("fp_line found\n");
					featureTally = 0;

/* ********************************************************** */

	if (n->children->str != NULL) {
		for(l = n->children; l != NULL; l = l->next) {
			printf("\tnow looking at fp_line text: '%s'\n", l->str);
			if (l->str != NULL && strcmp("start", l->str) == 0) {
					SEEN_NO_DUP(featureTally, 0);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_line start at x: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 1); 
						val = strtod(l->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							X1 = PCB_MM_TO_COORD(val) + moduleX;
						}
					} else {
						return -1;
					}
					if (l->children->next != NULL && l->children->next->str != NULL) {
						pcb_printf("\tfp_line start at y: '%s'\n", (l->children->next->str));
						SEEN_NO_DUP(featureTally, 2);	
						val = strtod(l->children->next->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							Y1 = PCB_MM_TO_COORD(val) + moduleY;
						}
					} else {
						return -1;
					}
			} else if (l->str != NULL && strcmp("end", l->str) == 0) {
					SEEN_NO_DUP(featureTally, 3);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_line end at x: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 4);
						val = strtod(l->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							X2 = PCB_MM_TO_COORD(val) + moduleX;
						}
					} else {
						return -1;
					}
					if (l->children->next != NULL && l->children->next->str != NULL) {
						pcb_printf("\tfp_line end at y: '%s'\n", (l->children->next->str));
						SEEN_NO_DUP(featureTally, 5);	
						val = strtod(l->children->next->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							Y2 = PCB_MM_TO_COORD(val) + moduleY;
						}
					} else {
						return -1;
					}
			} else if (l->str != NULL && strcmp("layer", l->str) == 0) {
					SEEN_NO_DUP(featureTally, 6);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_line layer: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 7);
						PCBLayer = kicad_get_layeridx(st, l->children->str);
						if (PCBLayer == -1) {
							return -1;
						}
					} else {
						return -1;
					}
			} else if (l->str != NULL && strcmp("width", l->str) == 0) {
					SEEN_NO_DUP(featureTally, 8);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_line width: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 9);
						val = strtod(l->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							Thickness = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
			} else if (l->str != NULL && strcmp("angle", l->str) == 0) { /* unlikely to be used or seen */
					SEEN_NO_DUP(featureTally, 10);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_line angle: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 11);
					} else {
						return -1;
					}
			} else if (l->str != NULL && strcmp("net", l->str) == 0) { /* unlikely to be used or seen */
					SEEN_NO_DUP(featureTally, 12);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_line net: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 13);
					} else {
						return -1;
					}
			} else {
				if (l->str != NULL) {
					printf("Unknown fp_line argument %s:", l->str);
				}
				return -1;
			}
		}
	}
	required = BV(0) | BV(3) | BV(6) | BV(8);
	if (((featureTally & required) == required) && newModule != NULL) { /* need start, end, layer, thickness at a minimum */
		CreateNewLineInElement(newModule, X1, Y1, X2, Y2, Thickness);
		pcb_printf("\tnew fp_line on layer created\n");
	}

/* ********************************************************** */

			} else if ((n->str != NULL && strcmp("fp_arc", n->str) == 0) || (n->str != NULL && strcmp("fp_circle", n->str) == 0)) {
					pcb_printf("fp_arc or fp_circle found\n");
					featureTally = 0;

/* ********************************************************** */

	if (subtree->str != NULL) {
		printf("fp_arc being parsed: '%s'\n", subtree->str);		
		for(l = n->children; l != NULL; l = l->next) {
			if (l->str != NULL && strcmp("start", l->str) == 0) {
					SEEN_NO_DUP(featureTally, 0);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_arc centre at x: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 1);
						val = strtod(l->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							centreX = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
					if (l->children->next != NULL && l->children->next->str != NULL) {
						pcb_printf("\tfp_arc centre at y: '%s'\n", (l->children->next->str));
						SEEN_NO_DUP(featureTally, 2);	
						val = strtod(l->children->next->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							centreY = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
			} else if (l->str != NULL && strcmp("center", l->str) == 0) { /* this lets us parse a circle too */
					SEEN_NO_DUP(featureTally, 0);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_arc centre at x: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 1);
						val = strtod(l->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							centreX = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
					if (l->children->next != NULL && l->children->next->str != NULL) {
						pcb_printf("\tfp_arc centre at y: '%s'\n", (l->children->next->str));
						SEEN_NO_DUP(featureTally, 2);
						val = strtod(l->children->next->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							centreY = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
			} else if (l->str != NULL && strcmp("end", l->str) == 0) {
					SEEN_NO_DUP(featureTally, 3);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_arc end at x: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 4);
						val = strtod(l->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							endX = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
					if (l->children->next != NULL && l->children->next->str != NULL) {
						pcb_printf("\tfp_arc end at y: '%s'\n", (l->children->next->str));
						SEEN_NO_DUP(featureTally, 5);
						val = strtod(l->children->next->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							endY = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
			} else if (l->str != NULL && strcmp("layer", l->str) == 0) {
					SEEN_NO_DUP(featureTally, 6);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_arc layer: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 7);
						PCBLayer = kicad_get_layeridx(st, l->children->str);
						if (PCBLayer == -1) {
							return -1;
						}
					} else {
						return -1;
					}
			} else if (l->str != NULL && strcmp("width", l->str) == 0) {
					SEEN_NO_DUP(featureTally, 8);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_arc width: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 9);
						val = strtod(l->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							Thickness = PCB_MM_TO_COORD(val);
						}
					} else {
						return -1;
					}
			} else if (l->str != NULL && strcmp("angle", l->str) == 0) {
					SEEN_NO_DUP(featureTally, 10);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_arc angle CW rotation: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 11);
						val = strtod(l->children->str, &end);
						if (*end != 0) {
							return -1;
						} else {
							delta = val;
						}
					} else {
						return -1;
					}
			} else if (l->str != NULL && strcmp("net", l->str) == 0) { /* unlikely to be used or seen */
					SEEN_NO_DUP(featureTally, 12);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_arc net: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 13);
					} else {
						return -1;
					}
			} else {
				if (n->str != NULL) {
					printf("Unknown gr_arc argument %s:", l->str);
				}
				return -1;
			}
		}
	}
        required = BV(0) | BV(6) | BV(8);
        if (((featureTally & required) == required) && newModule != NULL) {
		/* need start, layer, thickness at a minimum */
		width = height = Distance(centreX, centreY, endX, endY); /* calculate radius of arc */
		if (width < 1) { /* degenerate case */
			startAngle = 0;
		} else {
			endAngle = 180*atan2(-(endY - centreY), endX - centreX)/M_PI; /* avoid using atan2 with zero parameters */
			if (endAngle < 0.0) {
				endAngle += 360.0; /*make it 0...360 */
			}
			startAngle = endAngle + delta; /* geda is 180 degrees out of phase with kicad, and opposite direction rotation */
			if (startAngle > 360.0) {
				startAngle -= 360.0;
			}
			if (startAngle < 0.0) {
				startAngle += 360.0;
			}

		}
		CreateNewArcInElement(newModule, moduleX + centreX, moduleY + centreY, width, height, endAngle, delta, Thickness);

	}

/* ********************************************************** */


			} else {
				if (n->str != NULL) {
					printf("Unknown pad argument : %s\n", n->str);
				}
			} 
		}

		if (newModule != NULL) {
			SetElementBoundingBox(PCB->Data, newModule, &PCB->Font);
			return 0; 
		} else {
			return -1;
		}
	} else {
		return -1;
	}
}

static int kicad_parse_zone(read_state_t *st, gsxl_node_t *subtree)
{

	gsxl_node_t *n, *m;
	int i;
	int polycount = 0;
	long j  = 0;
	unsigned long tally = 0;
	unsigned long required;

	pcb_polygon_t *polygon = NULL;
	pcb_flag_t flags = pcb_flag_make(PCB_FLAG_CLEARPOLY);
	char *end;
	double val;
	pcb_coord_t X, Y;
	int PCBLayer = 0;

	if (subtree->str != NULL) {
		printf("Zone element found:\t'%s'\n", subtree->str);
		for(n = subtree->next,i = 0; n != NULL; n = n->next, i++) {
			if (n->str != NULL && strcmp("net", n->str) == 0) {
				SEEN_NO_DUP(tally, 0);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\tzone net number:\t'%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("net_name", n->str) == 0) {
				SEEN_NO_DUP(tally, 1);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\tzone net_name:\t'%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("tstamp", n->str) == 0) {
				SEEN_NO_DUP(tally, 2);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\tzone tstamp:\t'%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("hatch", n->str) == 0) {
				SEEN_NO_DUP(tally, 3);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\tzone hatch_edge:\t'%s'\n", (n->children->str));
					SEEN_NO_DUP(tally, 4); /* same as ^= 1 was */
				} else {
					return -1;
				}
				if (n->children->next != NULL && n->children->next->str != NULL) {
					pcb_printf("\tzone hatching size:\t'%s'\n", (n->children->next->str));
					SEEN_NO_DUP(tally, 5);	
				} else {
					return -1;
			}
			} else if (n->str != NULL && strcmp("connect_pads", n->str) == 0) {
				SEEN_NO_DUP(tally, 6);
				if (n->children != NULL && n->children->str != NULL && (strcmp("clearance", n->children->str) == 0) && (n->children->children->str != NULL)) {
					pcb_printf("\tzone clearance:\t'%s'\n", (n->children->children->str));  /* this is if yes/no flag for connected pads is absent */
					SEEN_NO_DUP(tally, 7); /* same as ^= 1 was */
				} else if (n->children != NULL && n->children->str != NULL && n->children->next->str != NULL) {
					pcb_printf("\tzone connect_pads:\t'%s'\n", (n->children->str));  /* this is if the optional(!) yes or no flag for connected pads is present */
					SEEN_NO_DUP(tally, 8); /* same as ^= 1 was */
					if (n->children->next != NULL && n->children->next->str != NULL && n->children->next->children != NULL && n->children->next->children->str != NULL) {
						if (strcmp("clearance", n->children->next->str) == 0) {
							SEEN_NO_DUP(tally, 9);
							pcb_printf("\tzone connect_pads clearance: '%s'\n", (n->children->next->children->str));
						} else {
							printf("Unrecognised zone connect_pads option %s\n", n->children->next->str);
						}
					}
				}
			} else if (n->str != NULL && strcmp("layer", n->str) == 0) {
				SEEN_NO_DUP(tally, 10);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\tzone layer:\t'%s'\n", (n->children->str));
					PCBLayer = kicad_get_layeridx(st, n->children->str);
					if (PCBLayer == -1) {
						return -1;
					}
					polygon = CreateNewPolygon(&st->PCB->Data->Layer[PCBLayer], flags);
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
									pcb_printf("\tvertex X[%d]:\t'%s'\n", j, (m->children->str));
									val = strtod(m->children->str, &end);
									if (*end != 0) {
										return -1;
									} else {
										X = PCB_MM_TO_COORD(val);
									}
									if (m->children->next != NULL && m->children->next->str != NULL) {
										pcb_printf("\tvertex Y[%d]:\t'%s'\n", j, (m->children->next->str));
										val = strtod(m->children->next->str, &end);
										if (*end != 0) {
											return -1;
										} else {
											Y = PCB_MM_TO_COORD(val);
										}
										if (polygon != NULL) {
											CreateNewPointInPolygon(polygon, X, Y);
										}
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
				SEEN_NO_DUP(tally, 11);
				printf("\tReading fill settings:\n");
				for (m = n->children; m != NULL; m = m->next) {
					if (m->str != NULL && strcmp("arc_segments", m->str) == 0) {
						if (m->children != NULL && m->children->str != NULL) {
							pcb_printf("\tzone arc_segments:\t'%s'\n", (m->children->str));
						} else {
							return -1;
						}
					} else if (m->str != NULL && strcmp("thermal_gap", m->str) == 0) {
						if (m->children != NULL && m->children->str != NULL) {
							pcb_printf("\tzone thermal_gap:\t'%s'\n", (m->children->str));
						} else {
							return -1;
						}
					} else if (m->str != NULL && strcmp("thermal_bridge_width", m->str) == 0) {
						if (m->children != NULL && m->children->str != NULL) {
							pcb_printf("\tzone thermal_bridge_width:\t'%s'\n", (m->children->str));
						} else {
							return -1;
						}
					} else if (m->str != NULL) {
						printf("Unknown zone fill argument:\t%s\n", m->str);
					}
				}
			} else if (n->str != NULL && strcmp("min_thickness", n->str) == 0) {
				SEEN_NO_DUP(tally, 12);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\tzone min_thickness:\t'%s'\n", (n->children->str));
				} else {
					return -1;
				}
			} else if (n->str != NULL && strcmp("filled_polygon", n->str) == 0) {
				pcb_printf("\tIgnoring filled_polygon definition.\n");
			} else {
				if (n->str != NULL) {
					printf("Unknown polygon argument:\t%s\n", n->str);
				}
			} 
		}
	} 

	required = BV(10);
	if ((tally & required) == required) {  /* has location, layer, size and stroke thickness at a minimum */
		if (polygon != NULL) {
			pcb_add_polygon_on_layer(&st->PCB->Data->Layer[PCBLayer], polygon);
			InitClip(st->PCB->Data, &st->PCB->Data->Layer[PCBLayer], polygon);
		}
		return 0;
	}
	return -1;
}


/* Parse a board from &st->dom into st->PCB */
static int kicad_parse_pcb(read_state_t *st)
{
	/* gsxl_node_t *n;  not used */
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
		{"gr_circle",     kicad_parse_gr_arc},
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
	
int io_kicad_read_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, conf_role_t settings_dest)
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

pcb_trace("readres=%d\n", readres);
	return readres;
}

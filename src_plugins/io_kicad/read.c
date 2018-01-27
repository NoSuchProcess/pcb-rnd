/*
 *			COPYRIGHT
 *
 *	pcb-rnd, interactive printed circuit board design
 *	Copyright (C) 2016, 2017 Tibor 'Igor2' Palinkas
 *	Copyright (C) 2016, 2017 Erich S. Heinzle
 *	Copyright (C) 2017 Miloh
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
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
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
#include "rotate.h"
#include "safe_fs.h"
#include "attrib.h"

typedef struct {
	pcb_board_t *pcb;
	const char *Filename;
	conf_role_t settings_dest;
	gsxl_dom_t dom;
	htsi_t layer_k2i; /* layer name-to-index hash; name is the kicad name, index is the pcb-rnd layer index */
} read_state_t;

typedef struct {
	const char *node_name;
	int (*parser) (read_state_t *st, gsxl_node_t *subtree);
} dispatch_t;

static int kicad_error(gsxl_node_t *subtree, char *fmt, ...)
{
	gds_t str;
	va_list ap;


	gds_init(&str);
	pcb_append_printf(&str, "io_kicad parse error at %d.%d: ", subtree->line, subtree->col);

	va_start(ap, fmt);
	pcb_append_vprintf(&str, fmt, ap);
	va_end(ap);

	gds_append(&str, '\n');

	pcb_message(PCB_MSG_ERROR, "%s", str.array);

	gds_uninit(&str);
	return -1;
}

static int kicad_warning(gsxl_node_t *subtree, char *fmt, ...)
{
	gds_t str;
	va_list ap;

	gds_init(&str);
	pcb_append_printf(&str, "io_kicad warning at %d.%d: ", subtree->line, subtree->col);

	va_start(ap, fmt);
	pcb_append_vprintf(&str, fmt, ap);
	va_end(ap);

	gds_append(&str, '\n');

	pcb_message(PCB_MSG_WARNING, "%s", str.array);

	gds_uninit(&str);
	return 0;
}



/* Search the dispatcher table for subtree->str, execute the parser on match
   with the children ("parameters") of the subtree */
static int kicad_dispatch(read_state_t *st, gsxl_node_t *subtree, const dispatch_t *disp_table)
{
	const dispatch_t *d;

	/* do not tolerate empty/NIL node */
	if (subtree->str == NULL)
		return kicad_error(subtree, "unexpected empty/NIL subtree");

	for(d = disp_table; d->node_name != NULL; d++)
		if (strcmp(d->node_name, subtree->str) == 0)
			return d->parser(st, subtree->children);

	/* node name not found in the dispatcher table */
	return kicad_error(subtree, "Unknown node: '%s'", subtree->str);
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
		if (ver == 3 || ver == 4 || ver == 20170123)
			return 0;
	}
	return kicad_error(subtree, "unexpected layout version");
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
		if (strcmp("A4", subtree->str) == 0) {
			st->pcb->MaxWidth = PCB_MM_TO_COORD(297.0);
			st->pcb->MaxHeight = PCB_MM_TO_COORD(210.0);
		}
		else if (strcmp("A3", subtree->str) == 0) {
			st->pcb->MaxWidth = PCB_MM_TO_COORD(420.0);
			st->pcb->MaxHeight = PCB_MM_TO_COORD(297.0);
		}
		else if (strcmp("A2", subtree->str) == 0) {
			st->pcb->MaxWidth = PCB_MM_TO_COORD(594.0);
			st->pcb->MaxHeight = PCB_MM_TO_COORD(420.0);
		}
		else if (strcmp("A1", subtree->str) == 0) {
			st->pcb->MaxWidth = PCB_MM_TO_COORD(841.0);
			st->pcb->MaxHeight = PCB_MM_TO_COORD(594.0);
		}
		else if (strcmp("A0", subtree->str) == 0) {
			st->pcb->MaxWidth = PCB_MM_TO_COORD(1189.0);
			st->pcb->MaxHeight = PCB_MM_TO_COORD(841.0);
		}
		else { /* default to A0 */
			st->pcb->MaxWidth = PCB_MM_TO_COORD(1189.0);
			st->pcb->MaxHeight = PCB_MM_TO_COORD(841.0);
			pcb_message(PCB_MSG_ERROR, "\tUnable to determine layout size. Defaulting to A0 layout size.\n");
		}
		return 0;
	}
	return kicad_error(subtree, "error parsing KiCad layout size.");
}

/* kicad_pcb/parse_title_block */
static int kicad_parse_title_block(read_state_t *st, gsxl_node_t *subtree)
{

	gsxl_node_t *n;
	const char *prefix = "kicad_titleblock_";
	char *name;
	if (subtree->str != NULL) {
		name = pcb_concat(prefix, subtree->str, NULL);
		pcb_attrib_put(st->pcb, name, subtree->children->str);
		free(name);
		for(n = subtree->next; n != NULL; n = n->next) {
			if (n->str != NULL && strcmp("comment", n->str) != 0) {
				name = pcb_concat(prefix, n->str, NULL);
				pcb_attrib_put(st->pcb, name, n->children->str);
				free(name);
			}
			else { /* if comment field has extra children args */
				name = pcb_concat(prefix, n->str, "_", n->children->str, NULL);
				pcb_attrib_put(st->pcb, name, n->children->next->str);
				free(name);
			}
		}
		return 0;
	}
	return kicad_error(subtree, "error parsing KiCad titleblock.");
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
		text = subtree->str;
		for(i = 0; text[i] != 0; i++) {
			textLength++;
		}
		for(n = subtree, i = 0; n != NULL; n = n->next, i++) {
			if (n->str != NULL && strcmp("at", n->str) == 0) {
				SEEN_NO_DUP(tally, 0);
				if (n->children != NULL && n->children->str != NULL) {
					val = strtod(n->children->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "error parsing gr_text X1");
					}
					else {
						X = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL gr_text X1 node");
				}
				if (n->children->next != NULL && n->children->next->str != NULL) {
					val = strtod(n->children->next->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "error parsing gr_text Y1");
					}
					else {
						Y = PCB_MM_TO_COORD(val);
					}
					if (n->children->next->next != NULL && n->children->next->next->str != NULL) {
						val = strtod(n->children->next->next->str, &end);
						if (*end != 0) {
							return kicad_error(subtree, "error parsing gr_text rotation");
						}
						else {
							direction = 0; /* default */
							if (val > 45.0 && val <= 135.0) {
								direction = 1;
							}
							else if (val > 135.0 && val <= 225.0) {
								direction = 2;
							}
							else if (val > 225.0 && val <= 315.0) {
								direction = 3;
							}
						}
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL gr_text Y1 node");
				}
			}
			else if (n->str != NULL && strcmp("layer", n->str) == 0) {
				SEEN_NO_DUP(tally, 1);
				if (n->children != NULL && n->children->str != NULL) {
					PCBLayer = kicad_get_layeridx(st, n->children->str);
					if (PCBLayer < 0) {
						return kicad_error(subtree, "unexpected gr_text layer def < 0");
					}
					else if (pcb_layer_flags(PCB, PCBLayer) & PCB_LYT_BOTTOM) {
						Flags = pcb_flag_make(PCB_FLAG_ONSOLDER);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL gr_text layer node");
				}
			}
			else if (n->str != NULL && strcmp("hide", n->str) == 0) {
				pcb_printf("\tignoring gr_text \"hide\" flag\n");
			}
			else if (n->str != NULL && strcmp("effects", n->str) == 0) {
				for(m = n->children; m != NULL; m = m->next) {
					if (m->str != NULL && strcmp("font", m->str) == 0) {
						for(l = m->children; l != NULL; l = l->next) {
							if (m->str != NULL && strcmp("size", l->str) == 0) {
								SEEN_NO_DUP(tally, 2);
								if (l->children != NULL && l->children->str != NULL) {
									val = strtod(l->children->str, &end);
									if (*end != 0) {
										return kicad_error(subtree, "error parsing gr_text size X");
									}
									else {
										scaling = (int)(100 * val / 1.27); /* standard glyph width ~= 1.27mm */
									}
								}
								else {
									return kicad_error(subtree, "unexpected empty/NULL gr_text font size X node");
								}
								if (l->children->next != NULL && l->children->next->str != NULL) {
									/*pcb_trace("\tfont sizeY: '%s'\n", (l->children->next->str)); */
								}
								else {
									return kicad_error(subtree, "unexpected empty/NULL gr_text font size Y node");
								}
							}
							else if (strcmp("thickness", l->str) == 0) {
								SEEN_NO_DUP(tally, 3);
								if (l->children != NULL && l->children->str != NULL) {
									/*pcb_trace("\tfont thickness: '%s'\n", (l->children->str)); */
								}
								else {
									return kicad_error(subtree, "unexpected empty/NULL gr_text font thickness node");
								}
							}
						}
					}
					else if (m->str != NULL && strcmp("justify", m->str) == 0) {
						/* SEEN_NO_DUP(tally, 4); */
						if (m->children != NULL && m->children->str != NULL) {
							if (strcmp("mirror", m->children->str) == 0) {
								mirrored = 1;
								SEEN_NO_DUP(tally, 4);
							}
							/* ignore right or left justification for now */
						}
						else {
							return kicad_error(subtree, "unexpected empty/NULL gr_text justify node");
						}
					}
					else {
						if (m->str != NULL) {
							/*pcb_trace("Unknown effects argument %s:", m->str); */
						}
						return kicad_error(subtree, "unexpected empty/NULL gr_text effects node");
					}
				}
			}
		}
	}
	required = BV(0) | BV(1) | BV(2) | BV(3);
	if ((tally & required) == required) { /* has location, layer, size and stroke thickness at a minimum */
#warning TODO: this will never be NULL; what are we trying to check here?
		if (&st->pcb->fontkit.dflt == NULL) {
			pcb_font_create_default(st->pcb);
		}

		if (mirrored != 0) {
			if (direction % 2 == 0) {
				direction += 2;
				direction = direction % 4;
			}
			if (direction == 0) {
				X -= PCB_MM_TO_COORD((glyphWidth * textLength) / 2.0);
				Y += PCB_MM_TO_COORD(glyphWidth / 2.0); /* centre it vertically */
			}
			else if (direction == 1) {
				Y -= PCB_MM_TO_COORD((glyphWidth * textLength) / 2.0);
				X -= PCB_MM_TO_COORD(glyphWidth / 2.0); /* centre it vertically */
			}
			else if (direction == 2) {
				X += PCB_MM_TO_COORD((glyphWidth * textLength) / 2.0);
				Y -= PCB_MM_TO_COORD(glyphWidth / 2.0); /* centre it vertically */
			}
			else if (direction == 3) {
				Y += PCB_MM_TO_COORD((glyphWidth * textLength) / 2.0);
				X += PCB_MM_TO_COORD(glyphWidth / 2.0); /* centre it vertically */
			}
		}
		else { /* not back of board text */
			if (direction == 0) {
				X -= PCB_MM_TO_COORD((glyphWidth * textLength) / 2.0);
				Y -= PCB_MM_TO_COORD(glyphWidth / 2.0); /* centre it vertically */
			}
			else if (direction == 1) {
				Y += PCB_MM_TO_COORD((glyphWidth * textLength) / 2.0);
				X -= PCB_MM_TO_COORD(glyphWidth / 2.0); /* centre it vertically */
			}
			else if (direction == 2) {
				X += PCB_MM_TO_COORD((glyphWidth * textLength) / 2.0);
				Y += PCB_MM_TO_COORD(glyphWidth / 2.0); /* centre it vertically */
			}
			else if (direction == 3) {
				Y -= PCB_MM_TO_COORD((glyphWidth * textLength) / 2.0);
				X += PCB_MM_TO_COORD(glyphWidth / 2.0); /* centre it vertically */
			}
		}

		pcb_text_new(&st->pcb->Data->Layer[PCBLayer], pcb_font(st->pcb, 0, 1), X, Y, direction, scaling, text, Flags);
		return 0; /* create new font */
	}
	return kicad_error(subtree, "failed to create gr_text element");
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

	Clearance = Thickness = 1; /* start with sane default of one nanometre */

	if (subtree->str != NULL) {
		for(n = subtree; n != NULL; n = n->next) {
			if (n->str != NULL && strcmp("start", n->str) == 0) {
				SEEN_NO_DUP(tally, 0);
				if (n->children != NULL && n->children->str != NULL) {
					val = strtod(n->children->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "error parsing gr_line X1");
					}
					else {
						X1 = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "null/missing node gr_line X1 coord");
				}
				if (n->children->next != NULL && n->children->next->str != NULL) {
					val = strtod(n->children->next->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "error parsing gr_line Y1");
					}
					else {
						Y1 = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "null/missing node gr_line Y1 coord");
				}
			}
			else if (n->str != NULL && strcmp("end", n->str) == 0) {
				SEEN_NO_DUP(tally, 1);
				if (n->children != NULL && n->children->str != NULL) {
					val = strtod(n->children->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "error parsing gr_line X2");
					}
					else {
						X2 = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "null/missing node gr_line X2 coord");
				}
				if (n->children->next != NULL && n->children->next->str != NULL) {
					val = strtod(n->children->next->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "error parsing gr_line Y2");
					}
					else {
						Y2 = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "null/missing node gr_line Y2 coord");
				}
			}
			else if (n->str != NULL && strcmp("layer", n->str) == 0) {
				SEEN_NO_DUP(tally, 2);
				if (n->children != NULL && n->children->str != NULL) {
					PCBLayer = kicad_get_layeridx(st, n->children->str);
					if (PCBLayer < 0) {
						/*pcb_trace("unsupported gr_line layer ignored.\n"); */
						return 0;
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL gr_line layer.");
				}
			}
			else if (n->str != NULL && strcmp("width", n->str) == 0) {
				SEEN_NO_DUP(tally, 3);
				if (n->children != NULL && n->children->str != NULL) {
					/*pcb_trace("\tgr_line width: '%s'\n", (n->children->str)); */
					val = strtod(n->children->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "error parsing gr_line width");
					}
					else {
						Thickness = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL gr_line width.");
				}
			}
			else if (n->str != NULL && strcmp("angle", n->str) == 0) { /* unlikely to be used or seen */
				SEEN_NO_DUP(tally, 4);
				if (n->children != NULL && n->children->str != NULL) {
					/*pcb_trace("\tgr_line angle: '%s'\n", (n->children->str)); */
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL gr_line angle.");
				}
			}
			else if (n->str != NULL && strcmp("net", n->str) == 0) { /* unlikely to be used or seen */
				SEEN_NO_DUP(tally, 5);
				if (n->children != NULL && n->children->str != NULL) {
					/*pcb_trace("\tgr_line net: '%s'\n", (n->children->str)); */
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL gr_line net.");
				}
			}
			else if (n->str != NULL && strcmp("tstamp", n->str) == 0) {
				/*pcb_trace("\tgr_line tstamp: '%s'\n", (n->children->str)); */
				/* ignore */
			}
			else {
				if (n->str != NULL) {
					/*pcb_trace("Unknown gr_line argument %s:", n->str); */
				}
				return kicad_error(subtree, "unexpected empty/NULL gr_line node.");
			}
		}
	}
	required = BV(0) | BV(1) | BV(2); /* | BV(3); now have 1nm default width, i.e. for edge cut */
	if ((tally & required) == required) { /* need start, end, layer, thickness at a minimum */
		pcb_line_new(&st->pcb->Data->Layer[PCBLayer], X1, Y1, X2, Y2, Thickness, Clearance, Flags);
		/*pcb_trace("\tnew gr_line on layer created\n"); */
		return 0;
	}
	else {
		/*pcb_trace("\tignoring malformed gr_line definition \n"); */
		return 0;
	}
}

/* kicad_pcb/gr_arc     can also parse gr_cicle*/
static int kicad_parse_gr_arc(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *n;
	unsigned long tally = 0, required;

	char *end;
	double val;
	pcb_coord_t centreX, centreY, endX, endY, width, height, Thickness, Clearance, deltaX, deltaY;
	pcb_angle_t startAngle = 0.0;
	pcb_angle_t endAngle = 0.0;
	pcb_angle_t delta = 360.0; /* these defaults allow a gr_circle to be parsed, which does not specify (angle XXX) */
	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	int PCBLayer = 0; /* sane default value */

	Clearance = Thickness = PCB_MM_TO_COORD(0.250); /* start with sane defaults */

	if (subtree->str != NULL) {
		for(n = subtree; n != NULL; n = n->next) {
			if (n->str != NULL && strcmp("start", n->str) == 0) {
				SEEN_NO_DUP(tally, 0);
				if (n->children != NULL && n->children->str != NULL) {
					val = strtod(n->children->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "gr_arc X1 parse error.");
					}
					else {
						centreX = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL gr_arc X1.");
				}
				if (n->children->next != NULL && n->children->next->str != NULL) {
					val = strtod(n->children->next->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "gr_arc Y1 parse error.");
					}
					else {
						centreY = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL gr_arc Y1.");
				}
			}
			else if (n->str != NULL && strcmp("center", n->str) == 0) { /* this lets us parse a circle too */
				SEEN_NO_DUP(tally, 0);
				if (n->children != NULL && n->children->str != NULL) {
					val = strtod(n->children->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "gr_arc centre X parse error.");
					}
					else {
						centreX = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL gr_arc centre X.");
				}
				if (n->children->next != NULL && n->children->next->str != NULL) {
					val = strtod(n->children->next->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "gr_arc centre Y parse error.");
					}
					else {
						centreY = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL gr_arc centre Y.");
				}
			}
			else if (n->str != NULL && strcmp("end", n->str) == 0) {
				SEEN_NO_DUP(tally, 1);
				if (n->children != NULL && n->children->str != NULL) {
					val = strtod(n->children->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "gr_arc end X parse error.");
					}
					else {
						endX = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL gr_arc end X.");
				}
				if (n->children->next != NULL && n->children->next->str != NULL) {
					val = strtod(n->children->next->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "gr_arc end Y parse error.");
					}
					else {
						endY = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL gr_arc end Y.");
				}
			}
			else if (n->str != NULL && strcmp("layer", n->str) == 0) {
				SEEN_NO_DUP(tally, 2);
				if (n->children != NULL && n->children->str != NULL) {
					PCBLayer = kicad_get_layeridx(st, n->children->str);
					if (PCBLayer < 0) {
						return kicad_warning(subtree, "gr_arc: \"%s\" not found", n->children->str);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL gr_arc layer.");
				}
			}
			else if (n->str != NULL && strcmp("width", n->str) == 0) {
				SEEN_NO_DUP(tally, 3);
				if (n->children != NULL && n->children->str != NULL) {
					val = strtod(n->children->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "gr_arc width parse error.");
					}
					else {
						Thickness = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL gr_arc width.");
				}
			}
			else if (n->str != NULL && strcmp("angle", n->str) == 0) {
				if (n->children != NULL && n->children->str != NULL) {
					SEEN_NO_DUP(tally, 4);
					val = strtod(n->children->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "gr_arc angle parse error.");
					}
					else {
						delta = val;
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL gr_arc angle.");
				}
			}
			else if (n->str != NULL && strcmp("net", n->str) == 0) { /* unlikely to be used or seen */
				if (n->children != NULL && n->children->str != NULL) {
					/*pcb_trace("\tgr_arc net: '%s'\n", (n->children->str)); */
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL gr_arc net.");
				}
			}
			else if (n->str != NULL && strcmp("tstamp", n->str) == 0) {
				/*pcb_trace("\tgr_arc tstamp: '%s'\n", (n->children->str)); */
				/* ignore */
			}
			else {
				if (n->str != NULL) {
					/*pcb_trace("Unknown gr_arc argument %s:", n->str); */
				}
				return kicad_error(subtree, "unexpected empty/NULL gr_arc node.");
			}
		}
	}
	required = BV(0) | BV(1) | BV(2) | BV(3); /* | BV(4); not needed for circles */
	if ((tally & required) == required) { /* need start, end, layer, thickness at a minimum */
		width = height = pcb_distance(centreX, centreY, endX, endY); /* calculate radius of arc */
		deltaX = endX - centreX;
		deltaY = endY - centreY;
		if (width < 1) { /* degenerate case */
			startAngle = 0;
		}
		else {
			endAngle = 180 + 180 * atan2(-deltaY, deltaX) / M_PI;
			/* avoid using atan2 with zero parameters */

			if (endAngle < 0.0) {
				endAngle += 360.0; /*make it 0...360 */
			}
			startAngle = (endAngle - delta); /* geda is 180 degrees out of phase with kicad, and opposite direction rotation */
			if (startAngle > 360.0) {
				startAngle -= 360.0;
			}
			if (startAngle < 0.0) {
				startAngle += 360.0;
			}
		}
		pcb_arc_new(&st->pcb->Data->Layer[PCBLayer], centreX, centreY, width, height, startAngle, delta, Thickness, Clearance, Flags);
		return 0;
	}
	return kicad_error(subtree, "unexpected empty/NULL node in gr_arc.");
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
		for(n = subtree; n != NULL; n = n->next) {
			if (n->str != NULL && strcmp("at", n->str) == 0) {
				SEEN_NO_DUP(tally, 0);
				if (n->children != NULL && n->children->str != NULL) {
					val = strtod(n->children->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "error parsing via X");
					}
					else {
						X = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL via X node");
				}
				if (n->children->next != NULL && n->children->next->str != NULL) {
					val = strtod(n->children->next->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "error parsing via Y");
					}
					else {
						Y = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL via Y node");
				}
			}
			else if (n->str != NULL && strcmp("size", n->str) == 0) {
				SEEN_NO_DUP(tally, 1);
				if (n->children != NULL && n->children->str != NULL) {
					val = strtod(n->children->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "error parsing via size");
					}
					else {
						Thickness = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL via size node");
				}
			}
			else if (n->str != NULL && strcmp("layers", n->str) == 0) {
				SEEN_NO_DUP(tally, 2);
				for(m = n->children; m != NULL; m = m->next) {
					if (m->str != NULL) {
						pcb_printf("\tvia layer: '%s'\n", (m->str));
/*							PCBLayer = kicad_get_layeridx(st, m->str);
 *							if (PCBLayer < 0) {
 *								return -1;
 *							}   via layers not currently used... padstacks
 */
					}
					else {
						return kicad_error(subtree, "unexpected empty/NULL via layer node");
					}
				}
			}
			else if (n->str != NULL && strcmp("net", n->str) == 0) {
				SEEN_NO_DUP(tally, 3);
				if (n->children != NULL && n->children->str != NULL) {
					/*pcb_trace("\tvia segment net: '%s'\n", (n->children->str)); */
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL via net node");
				}
			}
			else if (n->str != NULL && strcmp("tstamp", n->str) == 0) {
				SEEN_NO_DUP(tally, 4);
				if (n->children != NULL && n->children->str != NULL) {
					/*pcb_trace("\tvia tstamp: '%s'\n", (n->children->str)); */
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL via tstamp node");
				}
			}
			else if (n->str != NULL && strcmp("drill", n->str) == 0) {
				SEEN_NO_DUP(tally, 5);
				if (n->children != NULL && n->children->str != NULL) {
					val = strtod(n->children->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "error parsing via drill");
					}
					else {
						Drill = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL via drill node");
				}
			}
			else {
				if (n->str != NULL) {
					/*pcb_trace("Unknown via argument %s:", n->str); */
				}
				return kicad_error(subtree, "unexpected empty/NULL via argument node");
			}
		}
	}
	required = BV(0) | BV(1);
	if ((tally & required) == required) { /* need start, end, layer, thickness at a minimum */
		pcb_via_new(st->pcb->Data, X, Y, Thickness, Clearance, Mask, Drill, name, Flags);
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
		for(n = subtree; n != NULL; n = n->next) {
			if (n->str != NULL && strcmp("start", n->str) == 0) {
				SEEN_NO_DUP(tally, 0);
				if (n->children != NULL && n->children->str != NULL) {
					val = strtod(n->children->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "error parsing segment X1");
					}
					else {
						X1 = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL segment X1 node");
				}
				if (n->children->next != NULL && n->children->next->str != NULL) {
					val = strtod(n->children->next->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "error parsing segment Y1");
					}
					else {
						Y1 = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL segment Y1 node");
				}
			}
			else if (n->str != NULL && strcmp("end", n->str) == 0) {
				SEEN_NO_DUP(tally, 1);
				if (n->children != NULL && n->children->str != NULL) {
					val = strtod(n->children->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "error parsing segment X2");
					}
					else {
						X2 = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL segment X2 node");
				}
				if (n->children->next != NULL && n->children->next->str != NULL) {
					val = strtod(n->children->next->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "error parsing segment Y2");
					}
					else {
						Y2 = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL segment Y2 node");
				}
			}
			else if (n->str != NULL && strcmp("layer", n->str) == 0) {
				SEEN_NO_DUP(tally, 2);
				if (n->children != NULL && n->children->str != NULL) {
					PCBLayer = kicad_get_layeridx(st, n->children->str);
					if (PCBLayer < 0) {
						return kicad_warning(subtree, "error parsing segment layer");
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL segment layer node");
				}
			}
			else if (n->str != NULL && strcmp("width", n->str) == 0) {
				SEEN_NO_DUP(tally, 3);
				if (n->children != NULL && n->children->str != NULL) {
					val = strtod(n->children->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "error parsing segment width");
					}
					else {
						Thickness = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL segment width node");
				}
			}
			else if (n->str != NULL && strcmp("net", n->str) == 0) {
				SEEN_NO_DUP(tally, 4);
				if (n->children != NULL && n->children->str != NULL) {
					SEEN_NO_DUP(tally, 11);
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL segment net node");
				}
			}
			else if (n->str != NULL && strcmp("tstamp", n->str) == 0) { /* not likely to be used */
				SEEN_NO_DUP(tally, 5);
				if (n->children != NULL && n->children->str != NULL) {
					SEEN_NO_DUP(tally, 13);
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL segment tstamp node");
				}
			}
			else {
				if (n->str != NULL) {
					/*pcb_trace("Unknown segment argument %s:", n->str); */
				}
				return kicad_error(subtree, "unexpected empty/NULL segment argument node");
			}
		}
	}
	required = BV(0) | BV(1) | BV(2) | BV(3);
	if ((tally & required) == required) { /* need start, end, layer, thickness at a minimum */
		pcb_line_new(&st->pcb->Data->Layer[PCBLayer], X1, Y1, X2, Y2, Thickness, Clearance, Flags);
		return 0;
	}
	return kicad_error(subtree, "failed to create segment on layout");
}

/* Parse a layer definition and do all the administration needed for the layer */
static int kicad_create_layer(read_state_t *st, int lnum, const char *lname, const char *ltype, gsxl_node_t *subtree)
{
	pcb_layer_id_t id = -1;
	pcb_layergrp_id_t gid = -1;
#warning TODO: we should not depend on layer IDs other than 0
	switch (lnum) {
		case 0:
		case 15:
		case 31:
			if (strcmp(lname+1, ".Cu") != 0)
				return kicad_error(subtree, "layer 0 and 15/31 must be F.Cu and B.Cu (.Cu mismatch)");
			if ((lname[0] != 'F') && (lname[0] != 'B'))
				return kicad_error(subtree, "layer 0 and 15/31 must be F.Cu and B.Cu (F or B mismatch)");
			pcb_layergrp_list(st->pcb, PCB_LYT_COPPER | ((lname[0] == 'B') ? PCB_LYT_BOTTOM : PCB_LYT_TOP), &gid, 1);
			id = pcb_layer_create(st->pcb, gid, lname);
/*printf("------------------------------ layer=%s\n", lname);
pcb_hid_actionl("dumpcsect", NULL);*/
			break;
		default:
			if (strcmp(lname, "Edge.Cuts") == 0) {
				/* Edge must be the outline */
				pcb_layergrp_t *g = pcb_get_grp_new_intern(PCB, -1);
				pcb_layergrp_fix_turn_to_outline(g);
				id = pcb_layer_create(st->pcb, g - st->pcb->LayerGroups.grp, lname);
			}
			else if ((strcmp(ltype, "signal") == 0) || (strcmp(ltype, "power") == 0) || (strncmp(lname, "Dwgs.", 4) == 0) || (strncmp(lname, "Cmts.", 4) == 0) || (strncmp(lname, "Eco", 3) == 0)) {
				/* Create a new inner layer for signals and for emulating misc layers */
				pcb_layergrp_t *g = pcb_get_grp_new_intern(PCB, -1);
				id = pcb_layer_create(st->pcb, g - st->pcb->LayerGroups.grp, lname);
			}
			else if ((lname[1] == '.') && ((lname[0] == 'F') || (lname[0] == 'B'))) {
				/* F. or B. layers */
#warning TODO: update this: we have explicit silk, paste and mask now
				if (strcmp(lname + 2, "SilkS") == 0)
					return 0; /* silk layers are implicit */
#if 0
				if (strcmp(lname + 2, "Adhes") == 0)
					return 0; /* pcb-rnd has no adhesive support */
				if (strcmp(lname + 2, "Paste") == 0)
					return 0; /* pcb-rnd has no custom paste support */
				if (strcmp(lname + 2, "Mask") == 0)
					return 0; /* pcb-rnd has no custom mask support */
				return kicad_error(subtree, "unknown F. or B. layer error"); /* unknown F. or B. layer -> error */
#endif
				goto hack1;
			}
			else if (lnum > 15) {
			hack1:;
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
	}
	else {
		assert(id < -1);
	}
	return 0;
}

/* Register a kicad layer in the layer hash after looking up the pcb-rnd equivalent */
static unsigned int kicad_reg_layer(read_state_t *st, const char *kicad_name, unsigned int mask)
{
	pcb_layer_id_t id;
	if (pcb_layer_list(st->pcb, mask, &id, 1) != 1) {
		pcb_layergrp_id_t gid;
		pcb_layergrp_list(PCB, mask, &gid, 1);
		id = pcb_layer_create(st->pcb, gid, kicad_name);
	}
	htsi_set(&st->layer_k2i, pcb_strdup(kicad_name), id);
	return 0;
}

/* Returns the pcb-rnd layer index for a kicad_name, or -1 if not found */
static int kicad_get_layeridx(read_state_t *st, const char *kicad_name)
{
	htsi_entry_t *e;
	e = htsi_getentry(&st->layer_k2i, kicad_name);
	if (e == NULL) {
		if ((kicad_name[0] == 'I') && (kicad_name[1] == 'n')) {
			/* Workaround: specal case InX.Cu, where X is an integer */
			char *end;
			long id = strtol(kicad_name + 2, &end, 10);
			if ((pcb_strcasecmp(end, ".Cu") == 0) && (id >= 1) && (id <= 30)) {
				if (kicad_reg_layer(st, kicad_name, PCB_LYT_COPPER | PCB_LYT_INTERN) == 0) {
					/*pcb_trace("Created implicit copper layer %s as %d\n", kicad_name, id); */
					return kicad_get_layeridx(st, kicad_name);
				}
				/*pcb_trace("Failed to create implicit copper layer %s as %d\n", kicad_name, id); */
			}
		}
		return -1;
	}
	return e->value;
}

/* kicad_pcb  parse (layers  )  - either board layer defintions, or module pad/via layer defs */
static int kicad_parse_layer_definitions(read_state_t *st, gsxl_node_t *subtree)
{
	gsxl_node_t *n;
	int i;
	unsigned int res;

	if (strcmp(subtree->parent->parent->str, "kicad_pcb") != 0) { /* test if deeper in tree than layer definitions for entire board */
		return kicad_error(subtree, "layer definition found in unexpected location in KiCad layout");
	}
	else { /* we are just below the top level or root of the tree, so this must be a layer definitions section */
		pcb_layergrp_inhibit_inc();
		pcb_layer_group_setup_default(&st->pcb->LayerGroups);

		/* set up the hash for implicit layers */
		res = 0;
		res |= kicad_reg_layer(st, "F.SilkS", PCB_LYT_SILK | PCB_LYT_TOP);
		res |= kicad_reg_layer(st, "B.SilkS", PCB_LYT_SILK | PCB_LYT_BOTTOM);

		/* for modules */
		res |= kicad_reg_layer(st, "Top", PCB_LYT_COPPER | PCB_LYT_TOP);
		res |= kicad_reg_layer(st, "Bottom", PCB_LYT_COPPER | PCB_LYT_BOTTOM);

		/*
		   We don't have custom mask layers yet
		   res |= kicad_reg_layer(st, "F.Mask",  PCB_LYT_MASK | PCB_LYT_TOP);
		   res |= kicad_reg_layer(st, "B.Mask",  PCB_LYT_MASK | PCB_LYT_BOTTOM);
		 */

		if (res != 0) {
			pcb_message(PCB_MSG_ERROR, "Internal error: can't find a silk or mask layer\n");
			pcb_layergrp_inhibit_dec();
			return kicad_error(subtree, "Internal error: can't find a silk or mask layer while parsing KiCad layout");
		}

		pcb_printf("Board layer descriptions:\n");
		for(n = subtree, i = 0; n != NULL; n = n->next, i++) {
			if ((n->str != NULL) && (n->children->str != NULL) && (n->children->next != NULL) && (n->children->next->str != NULL)) {
				int lnum = atoi(n->str);
				const char *lname = n->children->str, *ltype = n->children->next->str;
				/*pcb_trace("\tlayer #%d LAYERNUM found:\t%s\n", i, n->str); */
				/*pcb_trace("\tlayer #%d layer label found:\t%s\n", i, lname); */
				/*pcb_trace("\tlayer #%d layer description/type found:\t%s\n", i, ltype); */
				if (kicad_create_layer(st, lnum, lname, ltype, n) < 0) {
					pcb_message(PCB_MSG_ERROR, "Unrecognized layer: %d, %s, %s\n", lnum, lname, ltype);
					pcb_layergrp_inhibit_dec();
					return -1;
				}
			}
			else {
				pcb_message(PCB_MSG_ERROR, "unexpected board layer definition(s) encountered.\n");
				pcb_layergrp_inhibit_dec();
				return -1;
			}
		}

		pcb_layergrp_fix_old_outline(PCB);

		pcb_layergrp_inhibit_dec();
		return 0;
	}
}

/* kicad_pcb  parse (net  ) ;   used for net descriptions for the entire layout */
static int kicad_parse_net(read_state_t *st, gsxl_node_t *subtree)
{
	if (subtree != NULL && subtree->str != NULL) {
		/*pcb_trace("net number: '%s'\n", subtree->str); */
	}
	else {
		return kicad_error(subtree, "missing net number in net descriptors.");
	}
	if (subtree->next != NULL && subtree->next->str != NULL) {
		/*pcb_trace("\tcorresponding net label: '%s'\n", (subtree->next->str)); */
	}
	else {
		return kicad_error(subtree, "missing net label in net descriptors.");
	}
	return 0;
}

static int kicad_make_pad(read_state_t *st, gsxl_node_t *subtree, pcb_subc_t *subc, int throughHole, pcb_coord_t moduleX, pcb_coord_t moduleY, pcb_coord_t X, pcb_coord_t Y, pcb_coord_t padXsize, pcb_coord_t padYsize, unsigned int padRotation, unsigned int moduleRotation, pcb_coord_t Clearance, pcb_coord_t drill, const char *pinName, const char *pad_shape, unsigned long *featureTally, int *moduleEmpty, int kicadLayer)
{
	unsigned long required;
	pcb_coord_t X1, Y1, X2, Y2, Thickness;
	pcb_flag_t Flags;

	int square;

	if (throughHole == 1) { /* "pad" for thru-hole pin */
		/*pcb_trace("\tcreating new pin %s in element\n", pinName); */
		required = BV(0) | BV(1) | BV(3) | BV(5);
		if ((*featureTally & required) == required) {
			*moduleEmpty = 0;

			if (pad_shape == NULL)
				return kicad_error(subtree, "pin with no shape");

			if (strcmp(pad_shape, "circle") == 0) {
				square = 0;
			}
			else {
				square = 1; /* this will catch obround, roundrect, trapezoidal as well. Kicad does not do octagonal pads */
			}

 /* using clearance value for arg 5 = mask too */
/*
			pcb_element_pin_new(subc, X + moduleX, Y + moduleY, padXsize, Clearance, Clearance, drill, pinName, pinName, Flags);
*/
		}
		else {
			return kicad_error(subtree, "malformed module pad/pin definition.");
		}
	}
	else if (subc != NULL) { /* "pad" for smd pad */
		/*pcb_trace("\tcreating new pad %s in element\n", pinName); */
		required = BV(0) | BV(1) | BV(2) | BV(5);
		if ((*featureTally & required) == required) {
			if (padXsize >= padYsize) { /* square pad or rectangular pad, wider than tall */
				Y1 = Y2 = Y;
				X1 = X - (padXsize - padYsize) / 2;
				X2 = X + (padXsize - padYsize) / 2;
				Thickness = padYsize;
			}
			else { /* rectangular pad, taller than wide */
				X1 = X2 = X;
				Y1 = Y - (padYsize - padXsize) / 2;
				Y2 = Y + (padYsize - padXsize) / 2;
				Thickness = padXsize;
			}

			if (square && kicadLayer) {
				Flags = pcb_flag_make(PCB_FLAG_SQUARE);
			}
			else if (kicadLayer) {
				Flags = pcb_flag_make(0);
			}
			else if (square && !kicadLayer) {
				Flags = pcb_flag_make(PCB_FLAG_SQUARE | PCB_FLAG_ONSOLDER);
			}
			else {
				Flags = pcb_flag_make(PCB_FLAG_ONSOLDER);
			}

			*moduleEmpty = 0;
			/* the rotation value describes rotation to the pad
			   versus the original pad orientation, _NOT_ rotation
			   that now needs to be applied, it seems */
			if (padRotation != 0 && padRotation != moduleRotation) {
				padRotation = padRotation / 90; /*ignore rotation != n*90 */
				PCB_COORD_ROTATE90(X1, Y1, X, Y, padRotation);
				PCB_COORD_ROTATE90(X2, Y2, X, Y, padRotation);
			}

			/* using clearance value for arg 7 = mask too */
/*
			pcb_element_pad_new(subc, X1 + moduleX, Y1 + moduleY, X2 + moduleX, Y2 + moduleY, Thickness, Clearance, Clearance, pinName, pinName, Flags);
*/

		}
		else
			return kicad_error(subtree, "error parsing incomplete module definition.");
	}
	else
		return kicad_error(subtree, "error - unable to create incomplete module definition.");
	return 0;
}

pcb_layer_t *kicad_get_subc_layer(read_state_t *st, pcb_subc_t *subc, const char *layer_name, const char *default_layer_name)
{
	int pcb_idx = -1;
	pcb_layer_type_t lyt;
	pcb_layer_combining_t comb;
	const char *lnm;

	if (layer_name != NULL) {
		pcb_idx = kicad_get_layeridx(st, layer_name);
		lnm = layer_name;
	}
	if (pcb_idx < 0) {
		pcb_message(PCB_MSG_ERROR, "\tline layer '%s' not found or not defined for module object, using module layer '%s' instead.\n", layer_name, default_layer_name);
		pcb_idx = kicad_get_layeridx(st, default_layer_name);
		if (pcb_idx < 0)
			return NULL;
		lnm = default_layer_name;
	}

	lyt = pcb_layer_flags(st->pcb, pcb_idx);
	comb = 0;
	return pcb_subc_get_layer(subc, lyt, comb, 1, lnm);
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
	int moduleLayer = 0; /* used in case empty module element layer defs found */
	int kicadLayer = 15; /* default = top side */
	int on_bottom = 0;
	int padLayerDefCount = 0;
	int SMD = 0;
	int square = 0;
	int throughHole = 0;
	int foundRefdes = 0;
	int foundValue = 0;
	int refdesScaling = 100;
	int moduleEmpty = 1;
	unsigned int moduleRotation = 0; /* for rotating modules */
	unsigned int padRotation = 0; /* for rotating pads */
	unsigned long tally = 0, featureTally = 0, required;
	pcb_coord_t moduleX, moduleY, X, Y, X1, Y1, X2, Y2, centreX, centreY, endX, endY, width, height, Thickness, Clearance, padXsize, padYsize, drill, refdesX, refdesY;
	pcb_angle_t startAngle = 0.0;
	pcb_angle_t endAngle = 0.0;
	pcb_angle_t delta = 360.0; /* these defaults allow a fp_circle to be parsed, which does not specify (angle XXX) */
	double val;
	double glyphWidth = 1.27; /* a reasonable approximation of pcb glyph width, ~=  5000 centimils */
	unsigned direction = 0; /* default is horizontal */
	char *end, *textLabel, *text;
	char *pinName, *moduleName;
	const char *subc_layer_str;
	pcb_subc_t *subc;
	pcb_layer_t *subc_layer;

	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	pcb_flag_t TextFlags = pcb_flag_make(0); /* start with something bland here */
	Clearance = PCB_MM_TO_COORD(0.250); /* start with something bland here */

	if (subtree->str != NULL) {
		/*pcb_trace("Name of module element being parsed: '%s'\n", subtree->str); */
		moduleName = subtree->str;
		p = subtree->next;
		if ((p != NULL) && (p->str != NULL) && (strcmp("locked", p->str) == 0)) {
			p = p->next;
			/*pcb_trace("The module is locked, which is being ignored.\n"); */
		}
		SEEN_NO_DUP(tally, 0);
		for(n = p, i = 0; n != NULL; n = n->next, i++) {
			if (n->str != NULL && strcmp("layer", n->str) == 0) { /* need this to sort out ONSOLDER flags etc... */
				SEEN_NO_DUP(tally, 1);
				if (n->children != NULL && n->children->str != NULL) {
					PCBLayer = kicad_get_layeridx(st, n->children->str);
					moduleLayer = PCBLayer;
					subc_layer_str = n->children->str;
					if (PCBLayer < 0)
						return kicad_error(subtree, "module layer error - layer < 0.");
					else if (pcb_layer_flags(PCB, PCBLayer) & PCB_LYT_BOTTOM)
						on_bottom = 1;
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL module layer node");
				}
			}
			else if (n->str != NULL && strcmp("tedit", n->str) == 0) {
				SEEN_NO_DUP(tally, 2);
				if (n->children != NULL && n->children->str != NULL) {
					/*pcb_trace("\ttedit: '%s'\n", (n->children->str)); */
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL module tedit node");
				}
			}
			else if (n->str != NULL && strcmp("tstamp", n->str) == 0) {
				SEEN_NO_DUP(tally, 3);
				if (n->children != NULL && n->children->str != NULL) {
					/*pcb_trace("\ttstamp: '%s'\n", (n->children->str)); */
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL module tstamp node");
				}
			}
			else if (n->str != NULL && strcmp("attr", n->str) == 0) {
				if (n->children != NULL && n->children->str != NULL) {
					/*pcb_trace("\tmodule attribute \"attr\": '%s' (not used)\n", (n->children->str)); */
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL module attr node");
				}
			}
			else if (n->str != NULL && strcmp("at", n->str) == 0) {
				SEEN_NO_DUP(tally, 4);
				if (n->children != NULL && n->children->str != NULL) {
					SEEN_NO_DUP(tally, 5); /* same as ^= 1 was */
					val = strtod(n->children->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "error parsing module X.");
					}
					else {
						moduleX = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL module X node");
				}
				if (n->children->next != NULL && n->children->next->str != NULL) {
					SEEN_NO_DUP(tally, 6);
					val = strtod(n->children->next->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "error parsing module Y.");
					}
					else {
						moduleY = PCB_MM_TO_COORD(val);
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL module Y node");
				}
				if (n->children->next->next != NULL && n->children->next->next->str != NULL) {
					val = strtod(n->children->next->next->str, &end);
					if (*end != 0) {
						return kicad_error(subtree, "error parsing module rotation.");
					}
					else {
						moduleRotation = (int)val;
					}
				}
				else {
					/*pcb_trace("\tno module (at) \"rotation\" value found'\n"); */
				}
				/* if we have been provided with a Module Name and location, create a new Element with default "" and "" for refdes and value fields */
				if (moduleName != NULL && moduleDefined == 0) {
					moduleDefined = 1; /* but might be empty, wait and see */
					/*pcb_trace("Have new module name and location, defining module/element %s\n", moduleName); */
					subc = pcb_subc_new();
					pcb_add_subc_to_data(st->pcb->Data, subc);
					pcb_subc_create_aux(subc, moduleX, moduleY, 0.0, on_bottom);
					pcb_attribute_put(&subc->Attributes, "refdes", "K1");
				}
			}
			else if (n->str != NULL && strcmp("model", n->str) == 0) {
				/*pcb_trace("module 3D model found and ignored\n"); */
			}
			else if (n->str != NULL && strcmp("fp_text", n->str) == 0) {
				/*pcb_trace("fp_text found\n"); */
				featureTally = 0;

/* ********************************************************** */

				if (n->children != NULL && n->children->str != NULL) {
					textLabel = n->children->str;
					if (n->children->next != NULL && n->children->next->str != NULL) {
						text = n->children->next->str;
						if (strcmp("reference", textLabel) == 0) {
							SEEN_NO_DUP(tally, 7);
							pcb_attribute_put(&subc->Attributes, "refdes", text);
							foundRefdes = 1;
							textLength = strlen(text);
							/*pcb_trace("\tmoduleRefdes now: '%s'\n", moduleRefdes); */
						}
						else if (strcmp("value", textLabel) == 0) {
							SEEN_NO_DUP(tally, 8);
							pcb_attribute_put(&subc->Attributes, "value", text);
							foundValue = 1;
							/*pcb_trace("\tmoduleValue now: '%s'\n", moduleValue); */
						}
						else if (strcmp("descr", textLabel) == 0) {
							SEEN_NO_DUP(tally, 12);
							pcb_attribute_put(&subc->Attributes, "footprint", text);
							foundValue = 1;
							/*pcb_trace("\tmoduleValue now: '%s'\n", moduleValue); */
						}
						else if (strcmp("hide", textLabel) == 0) {
							/*pcb_trace("\tignoring fp_text \"hide\" flag\n"); */
						}
					}
					else {
						text = textLabel; /* just a single string, no reference or value */
					}

					for(l = n->children->next->next, i = 0; l != NULL; l = l->next, i++) { /*fixed this */
						if (l->str != NULL && strcmp("at", l->str) == 0) {
							SEEN_NO_DUP(featureTally, 0);
							if (l->children != NULL && l->children->str != NULL) {
								SEEN_NO_DUP(featureTally, 1);
								val = strtod(l->children->str, &end);
								if (*end != 0) {
									return kicad_error(subtree, "error parsing module fp_text X.");
								}
								else {
									X = PCB_MM_TO_COORD(val);
									if (foundRefdes) {
										refdesX = X;
									}
								}
							}
							else {
								return kicad_error(subtree, "unexpected empty/NULL module fp_text X node");
							}
							if (l->children->next != NULL && l->children->next->str != NULL) {
								SEEN_NO_DUP(featureTally, 2);
								val = strtod(l->children->next->str, &end);
								if (*end != 0) {
									return kicad_error(subtree, "error parsing module fp_text Y.");
								}
								else {
									Y = PCB_MM_TO_COORD(val);
									if (foundRefdes) {
										refdesY = Y;
									}
								}
								if (l->children->next->next != NULL && l->children->next->next->str != NULL) {
									val = strtod(l->children->next->next->str, &end);
									if (*end != 0) {
										return kicad_error(subtree, "error parsing module fp_text rotation.");
									}
									else {
										direction = 0; /* default */
										if (val > 45.0 && val <= 135.0) {
											direction = 1;
										}
										else if (val > 135.0 && val <= 225.0) {
											direction = 2;
										}
										else if (val > 225.0 && val <= 315.0) {
											direction = 3;
										}
									}
									SEEN_NO_DUP(featureTally, 3);
								}
							}
							else {
								return kicad_error(subtree, "unexpected empty/NULL module fp_text Y node");
							}
						}
						else if (l->str != NULL && strcmp("layer", l->str) == 0) {
							SEEN_NO_DUP(featureTally, 4);
							if (l->children != NULL && l->children->str != NULL) {
								PCBLayer = kicad_get_layeridx(st, l->children->str);
								if (PCBLayer < 0) {
									/*pcb_trace("\ttext layer not defined for module text, default being used.\n"); */
									Flags = pcb_flag_make(0);
								}
								else if (pcb_layer_flags(PCB, PCBLayer) & PCB_LYT_BOTTOM) {
									Flags = pcb_flag_make(PCB_FLAG_ONSOLDER);
								}
							}
							else {
								return kicad_error(subtree, "unexpected empty/NULL module fp_text layer node");
							}
						}
						else if (l->str != NULL && strcmp("hide", l->str) == 0) {
							/*pcb_trace("\tfp_text hidden flag \"hide\" found and ignored.\n"); */
						}
						else if (l->str != NULL && strcmp("justify", l->str) == 0) { /*this may be unnecessary here */
							/*pcb_trace("\tfp_text justify flag found and ignored.\n"); */
						}
						else if (l->str != NULL && strcmp("effects", l->str) == 0) {
							SEEN_NO_DUP(featureTally, 5);
							for(m = l->children; m != NULL; m = m->next) {
								if (m->str != NULL && strcmp("font", m->str) == 0) {
									SEEN_NO_DUP(featureTally, 6);
									for(p = m->children; p != NULL; p = p->next) {
										if (m->str != NULL && strcmp("size", p->str) == 0) {
											SEEN_NO_DUP(featureTally, 7);
											if (p->children != NULL && p->children->str != NULL) {
												val = strtod(p->children->str, &end);
												if (*end != 0) {
													return kicad_error(subtree, "error parsing module fp_text font size X.");
												}
												else {
													scaling = (int)(100 * val / 1.27); /* standard glyph width ~= 1.27mm */
													if (foundRefdes) {
														refdesScaling = scaling;
													}
												}
											}
											else {
												return kicad_error(subtree, "unexpected empty/NULL module fp_text X size node");
											}
											if (p->children->next != NULL && p->children->next->str != NULL) {
												/*pcb_trace("\tfont sizeY: '%s'\n", (p->children->next->str)); */
											}
											else {
												return kicad_error(subtree, "unexpected empty/NULL module fp_text Y size node");
											}
										}
										else if (strcmp("thickness", p->str) == 0) {
											SEEN_NO_DUP(featureTally, 8);
											if (p->children != NULL && p->children->str != NULL) {
												/*pcb_trace("\tfont thickness: '%s'\n", (p->children->str)); */
											}
											else {
												return kicad_error(subtree, "unexpected empty/NULL module fp_text thickness node");
											}
										}
									}
								}
								else if (m->str != NULL && strcmp("justify", m->str) == 0) {
									SEEN_NO_DUP(featureTally, 9);
									if (m->children != NULL && m->children->str != NULL) {
										/*pcb_trace("\ttext justification: '%s'\n", (m->children->str)); */
										if (strcmp("mirror", m->children->str) == 0) {
											mirrored = 1;
										}
									}
									else {
										return kicad_error(subtree, "unexpected empty/NULL module fp_text justify node");
									}
								}
								else {
									if (m->str != NULL) {
										/*pcb_trace("Unknown text effects argument %s:", m->str); */
									}
									return kicad_error(subtree, "unknown fp_text text effects argument null node");
								}
							}
						}
					}
				}
				required = BV(0) | BV(1) | BV(4) | BV(7) | BV(8);
				if ((tally & required) == required) { /* has location, layer, size and stroke thickness at a minimum */
#warning TODO: this will never be NULL; what are we trying to check here?
					if (&st->pcb->fontkit.dflt == NULL) {
						pcb_font_create_default(st->pcb);
					}

					X = refdesX;
					Y = refdesY;
					glyphWidth = 1.27;
					glyphWidth = glyphWidth * refdesScaling / 100.0;

					if (mirrored != 0) {
						if (direction % 2 == 0) {
							direction += 2;
							direction = direction % 4;
						}
						if (direction == 0) {
							X -= PCB_MM_TO_COORD((glyphWidth * textLength) / 2.0);
							Y += PCB_MM_TO_COORD(glyphWidth / 2.0); /* centre it vertically */
						}
						else if (direction == 1) {
							Y -= PCB_MM_TO_COORD((glyphWidth * textLength) / 2.0);
							X -= PCB_MM_TO_COORD(glyphWidth / 2.0); /* centre it vertically */
						}
						else if (direction == 2) {
							X += PCB_MM_TO_COORD((glyphWidth * textLength) / 2.0);
							Y -= PCB_MM_TO_COORD(glyphWidth / 2.0); /* centre it vertically */
						}
						else if (direction == 3) {
							Y += PCB_MM_TO_COORD((glyphWidth * textLength) / 2.0);
							X += PCB_MM_TO_COORD(glyphWidth / 2.0); /* centre it vertically */
						}
					}
					else { /* not back of board text */
						if (direction == 0) {
							X -= PCB_MM_TO_COORD((glyphWidth * textLength) / 2.0);
							Y -= PCB_MM_TO_COORD(glyphWidth / 2.0); /* centre it vertically */
						}
						else if (direction == 1) {
							Y += PCB_MM_TO_COORD((glyphWidth * textLength) / 2.0);
							X -= PCB_MM_TO_COORD(glyphWidth / 2.0); /* centre it vertically */
						}
						else if (direction == 2) {
							X += PCB_MM_TO_COORD((glyphWidth * textLength) / 2.0);
							Y += PCB_MM_TO_COORD(glyphWidth / 2.0); /* centre it vertically */
						}
						else if (direction == 3) {
							Y -= PCB_MM_TO_COORD((glyphWidth * textLength) / 2.0);
							X += PCB_MM_TO_COORD(glyphWidth / 2.0); /* centre it vertically */
						}
					}

					/* if we  update X, Y for text fields (refdes, value), we would do it here */
				}


/* ********************************************************** */

			}
			else if (n->str != NULL && strcmp("descr", n->str) == 0) {
				SEEN_NO_DUP(tally, 9);
				if (n->children != NULL && n->children->str != NULL) {
					/*pcb_trace("\tmodule descr: '%s'\n", (n->children->str)); */
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL module descr node");
				}
			}
			else if (n->str != NULL && strcmp("tags", n->str) == 0) {
				SEEN_NO_DUP(tally, 10);
				if (n->children != NULL && n->children->str != NULL) {
					/*pcb_trace("\tmodule tags: '%s'\n", (n->children->str)); may be more than one? */
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL module tags node");
				}
			}
			else if (n->str != NULL && strcmp("path", n->str) == 0) {
				SEEN_NO_DUP(tally, 11);
				if (n->children != NULL && n->children->str != NULL) {
					/*pcb_trace("\tmodule path: '%s'\n", (n->children->str)); */
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL module model node");
				}
				/* pads next  - have thru_hole, circle, rect, roundrect, to think about */
			}
			else if (n->str != NULL && strcmp("pad", n->str) == 0) {
				char *pad_shape = NULL;
				featureTally = 0;
				padLayerDefCount = 0;
				padRotation = 0;
				if (n->children != 0 && n->children->str != NULL) {
					pinName = n->children->str;
					SEEN_NO_DUP(featureTally, 0);
					if (n->children->next != NULL && n->children->next->str != NULL) {
						if (strcmp("thru_hole", n->children->next->str) == 0) {
							SMD = 0;
							throughHole = 1;
						}
						else {
							SMD = 1;
							throughHole = 0;
						}
						if (n->children->next->next != NULL && n->children->next->next->str != NULL) {
							pad_shape = n->children->next->next->str;
						}
						else {
							return kicad_error(subtree, "unexpected empty/NULL module pad shape node");
						}
					}
					else {
						return kicad_error(subtree, "unexpected empty/NULL module pad type node");
					}
				}
				else {
					return kicad_error(subtree, "unexpected empty/NULL module pad name  node");
				}
				if (n->children->next->next->next == NULL || n->children->next->next->next->str == NULL) {
					return kicad_error(subtree, "unexpected empty/NULL module node");
				}
				for(m = n->children->next->next->next; m != NULL; m = m->next) {
					if (m != NULL) {
						/*pcb_trace("\tstepping through module pad defs, looking at: %s\n", m->str); */
					}
					else {
						/*pcb_trace("error in pad def\n"); */
					}
					if (m->str != NULL && strcmp("at", m->str) == 0) {
						SEEN_NO_DUP(featureTally, 1);
						if (m->children != NULL && m->children->str != NULL) {
							val = strtod(m->children->str, &end);
							if (*end != 0) {
								return kicad_error(subtree, "error parsing module pad X.");
							}
							else {
								X = PCB_MM_TO_COORD(val);
							}
							if (m->children->next != NULL && m->children->next->str != NULL) {
								val = strtod(m->children->next->str, &end);
								if (*end != 0) {
									return kicad_error(subtree, "error parsing module pad Y.");
								}
								else {
									Y = PCB_MM_TO_COORD(val);
								}
							}
							else {
								return kicad_error(subtree, "unexpected empty/NULL module X node");
							}
							if (m->children->next->next != NULL && m->children->next->next->str != NULL) {
								val = strtod(m->children->next->next->str, &end);
								if (*end != 0) {
									/*pcb_trace("Odd pad rotation def ignored."); */
								}
								else {
									padRotation = (int)val;
								}
							}
						}
						else {
							return kicad_error(subtree, "unexpected empty/NULL module Y node");
						}
					}
					else if (m->str != NULL && strcmp("layers", m->str) == 0) {
						if (SMD) { /* skip testing for pins */
							SEEN_NO_DUP(featureTally, 2);
							kicadLayer = 15;
							for(l = m->children; l != NULL; l = l->next) {
								if (l->str != NULL) {
									PCBLayer = kicad_get_layeridx(st, l->str);
									if (PCBLayer < 0) {
										/* we ignore *.mask, *.paste, etc., if valid layer def already found */
										/*pcb_trace("Unknown layer definition: %s\n", l->str); */
										if (!padLayerDefCount) {
											/*pcb_trace("Default placement of pad is the copper layer defined for module as a whole\n"); */

											/*return -1; */
											if (on_bottom) {
												kicadLayer = 0;
											}
										}
									}
									else if (PCBLayer < -1) {
										/*pcb_trace("\tUnimplemented layer definition: %s\n", l->str); */
									}
									else if (pcb_layer_flags(PCB, PCBLayer) & PCB_LYT_BOTTOM) {
										kicadLayer = 0;
										padLayerDefCount++;
									}
									else if (padLayerDefCount) {
										/*pcb_trace("More than one valid pad layer found, only using the first one found for layer.\n"); */
										padLayerDefCount++;
									}
									else {
										padLayerDefCount++;
										/*pcb_trace("Valid layer defs found for current pad: %d\n", padLayerDefCount); */
									}
									/*pcb_trace("\tpad layer: '%s',  PCB layer number %d\n", (l->str), kicad_get_layeridx(st, l->str)); */
								}
								else {
									return kicad_error(subtree, "unexpected empty/NULL module layer node");
								}
							}
						}
						else {
							/*pcb_trace("\tIgnoring layer definitions for through hole pin\n"); */
						}
					}
					else if (m->str != NULL && strcmp("drill", m->str) == 0) {
						SEEN_NO_DUP(featureTally, 3);
						if (m->children != NULL && m->children->str != NULL) {
							val = strtod(m->children->str, &end);
							if (*end != 0) {
								return kicad_error(subtree, "error parsing module pad drill.");
							}
							else {
								drill = PCB_MM_TO_COORD(val);
							}

						}
						else {
							return kicad_error(subtree, "unexpected empty/NULL module pad drill node");
						}
					}
					else if (m->str != NULL && strcmp("net", m->str) == 0) {
						SEEN_NO_DUP(featureTally, 4);
						if (m->children != NULL && m->children->str != NULL) {
							if (m->children->next != NULL && m->children->next->str != NULL) {
								/*pcb_trace("\tpad's net name:\t'%s'\n", (m->children->next->str)); */
							}
							else {
								return kicad_error(subtree, "unexpected empty/NULL module pad net name node");
							}
						}
						else {
							return kicad_error(subtree, "unexpected empty/NULL module pad net node");
						}
					}
					else if (m->str != NULL && strcmp("size", m->str) == 0) {
						SEEN_NO_DUP(featureTally, 5);
						if (m->children != NULL && m->children->str != NULL) {
							val = strtod(m->children->str, &end);
							if (*end != 0) {
								return kicad_error(subtree, "error parsing module pad size X.");
							}
							else {
								padXsize = PCB_MM_TO_COORD(val);
							}
							if (m->children->next != NULL && m->children->next->str != NULL) {
								val = strtod(m->children->next->str, &end);
								if (*end != 0) {
									return kicad_error(subtree, "error parsing module pad size Y.");
								}
								else {
									padYsize = PCB_MM_TO_COORD(val);
								}
							}
							else {
								return kicad_error(subtree, "unexpected empty/NULL module pad Y size node");
							}
						}
						else {
							return kicad_error(subtree, "unexpected empty/NULL module pad X size node");
						}
					}
					else {
						if (m->str != NULL) {
							/*pcb_trace("Unknown pad argument %s:", m->str); */
						}
					}
				}
				if (subc != NULL)
					if (kicad_make_pad(st, subtree, subc, throughHole, moduleX, moduleY, X, Y, padXsize, padYsize, padRotation, moduleRotation, Clearance, drill, pinName, pad_shape, &featureTally, &moduleEmpty, kicadLayer) != 0)
						return -1;
				pad_shape = NULL;
			}
			else if (n->str != NULL && strcmp("fp_line", n->str) == 0) {
				/*pcb_trace("fp_line found\n"); */
				featureTally = 0;

/* ********************************************************** */

				if (n->children->str != NULL) {
					for(l = n->children; l != NULL; l = l->next) {
						if (l->str != NULL && strcmp("start", l->str) == 0) {
							SEEN_NO_DUP(featureTally, 0);
							if (l->children != NULL && l->children->str != NULL) {
								SEEN_NO_DUP(featureTally, 1);
								val = strtod(l->children->str, &end);
								if (*end != 0) {
									return kicad_error(subtree, "error parsing fp_line start X.");
								}
								else {
									X1 = PCB_MM_TO_COORD(val) + moduleX;
								}
							}
							else {
								return kicad_error(subtree, "unexpected fp_line start X null node.");
							}
							if (l->children->next != NULL && l->children->next->str != NULL) {
								SEEN_NO_DUP(featureTally, 2);
								val = strtod(l->children->next->str, &end);
								if (*end != 0) {
									return kicad_error(subtree, "error parsing fp_line start Y.");
								}
								else {
									Y1 = PCB_MM_TO_COORD(val) + moduleY;
								}
							}
							else {
								return kicad_error(subtree, "unexpected fp_line start Y null node.");
							}
						}
						else if (l->str != NULL && strcmp("end", l->str) == 0) {
							SEEN_NO_DUP(featureTally, 3);
							if (l->children != NULL && l->children->str != NULL) {
								SEEN_NO_DUP(featureTally, 4);
								val = strtod(l->children->str, &end);
								if (*end != 0) {
									return kicad_error(subtree, "error parsing fp_line end X.");
								}
								else {
									X2 = PCB_MM_TO_COORD(val) + moduleX;
								}
							}
							else {
								return kicad_error(subtree, "unexpected fp_line end X null node.");
							}
							if (l->children->next != NULL && l->children->next->str != NULL) {
								SEEN_NO_DUP(featureTally, 5);
								val = strtod(l->children->next->str, &end);
								if (*end != 0) {
									return kicad_error(subtree, "error parsing fp_line end Y.");
								}
								else {
									Y2 = PCB_MM_TO_COORD(val) + moduleY;
								}
							}
							else {
								return kicad_error(subtree, "unexpected fp_line end Y null node.");
							}
						}
						else if (l->str != NULL && strcmp("layer", l->str) == 0) {
							SEEN_NO_DUP(featureTally, 6);
							if (l->children != NULL && l->children->str != NULL) {
								SEEN_NO_DUP(featureTally, 7);
								subc_layer = kicad_get_subc_layer(st, subc, l->children->str, subc_layer_str);
								if (subc_layer == NULL)
									return kicad_error(l, "unable to set subcircuit layer");
							}
							else {
								pcb_message(PCB_MSG_ERROR, "\tusing default module layer for gr_line element\n");
								subc_layer = kicad_get_subc_layer(st, subc, NULL, subc_layer_str); /* default to module layer */
								/* return -1; */
							}
							if (subc_layer == NULL)
								return kicad_error(l, "unable to set subcircuit layer");
						}
						else if (l->str != NULL && strcmp("width", l->str) == 0) {
							SEEN_NO_DUP(featureTally, 8);
							if (l->children != NULL && l->children->str != NULL) {
								SEEN_NO_DUP(featureTally, 9);
								val = strtod(l->children->str, &end);
								if (*end != 0) {
									return kicad_error(subtree, "error parsing fp_line width.");
								}
								else {
									Thickness = PCB_MM_TO_COORD(val);
								}
							}
							else {
								return kicad_error(subtree, "unexpected fp_line width null node.");
							}
						}
						else if (l->str != NULL && strcmp("angle", l->str) == 0) { /* unlikely to be used or seen */
							SEEN_NO_DUP(featureTally, 10);
							if (l->children != NULL && l->children->str != NULL) {
								SEEN_NO_DUP(featureTally, 11);
							}
							else {
								return kicad_error(subtree, "unexpected fp_line angle null node.");
							}
						}
						else if (l->str != NULL && strcmp("net", l->str) == 0) { /* unlikely to be used or seen */
							SEEN_NO_DUP(featureTally, 12);
							if (l->children != NULL && l->children->str != NULL) {
								SEEN_NO_DUP(featureTally, 13);
							}
							else {
								return kicad_error(subtree, "unexpected fp_line net null node.");
							}
						}
						else {
							if (l->str != NULL) {
								/*pcb_trace("Unknown fp_line argument %s:", l->str); */
							}
							return kicad_error(subtree, "unexpected fp_line null node.");
						}
					}
				}
				required = BV(0) | BV(3) | BV(6) | BV(8);
				if (((featureTally & required) == required) && subc_layer != NULL) { /* need start, end, layer, thickness at a minimum */
					moduleEmpty = 0;
					pcb_line_new(subc_layer, X1, Y1, X2, Y2, Thickness, 0, pcb_no_flags());
				}

/* ********************************************************** */

			}
			else if ((n->str != NULL && strcmp("fp_arc", n->str) == 0) || (n->str != NULL && strcmp("fp_circle", n->str) == 0)) {
				/*pcb_trace("fp_arc or fp_circle found\n"); */
				featureTally = 0;

/* ********************************************************** */

				if (subtree->str != NULL) {
					for(l = n->children; l != NULL; l = l->next) {
						if (l->str != NULL && strcmp("start", l->str) == 0) {
							SEEN_NO_DUP(featureTally, 0);
							if (l->children != NULL && l->children->str != NULL) {
								SEEN_NO_DUP(featureTally, 1);
								val = strtod(l->children->str, &end);
								if (*end != 0) {
									return kicad_error(subtree, "error parsing fp_arc X.");
								}
								else {
									centreX = PCB_MM_TO_COORD(val);
								}
							}
							else {
								return kicad_error(subtree, "unexpected fp_arc start X null node.");
							}
							if (l->children->next != NULL && l->children->next->str != NULL) {
								SEEN_NO_DUP(featureTally, 2);
								val = strtod(l->children->next->str, &end);
								if (*end != 0) {
									return kicad_error(subtree, "error parsing fp_arc Y.");
								}
								else {
									centreY = PCB_MM_TO_COORD(val);
								}
							}
							else {
								return kicad_error(subtree, "unexpected fp_arc start Y null node.");
							}
						}
						else if (l->str != NULL && strcmp("center", l->str) == 0) { /* this lets us parse a circle too */
							SEEN_NO_DUP(featureTally, 0);
							if (l->children != NULL && l->children->str != NULL) {
								SEEN_NO_DUP(featureTally, 1);
								val = strtod(l->children->str, &end);
								if (*end != 0) {
									return kicad_error(subtree, "error parsing fp_arc centre X.");
								}
								else {
									centreX = PCB_MM_TO_COORD(val);
								}
							}
							else {
								return kicad_error(subtree, "unexpected fp_arc centre X null node.");
							}
							if (l->children->next != NULL && l->children->next->str != NULL) {
								SEEN_NO_DUP(featureTally, 2);
								val = strtod(l->children->next->str, &end);
								if (*end != 0) {
									return kicad_error(subtree, "error parsing fp_arc centre Y.");
								}
								else {
									centreY = PCB_MM_TO_COORD(val);
								}
							}
							else {
								return kicad_error(subtree, "unexpected fp_arc centre Y null node.");
							}
						}
						else if (l->str != NULL && strcmp("end", l->str) == 0) {
							SEEN_NO_DUP(featureTally, 3);
							if (l->children != NULL && l->children->str != NULL) {
								SEEN_NO_DUP(featureTally, 4);
								val = strtod(l->children->str, &end);
								if (*end != 0) {
									return kicad_error(subtree, "error parsing fp_arc end X.");
								}
								else {
									endX = PCB_MM_TO_COORD(val);
								}
							}
							else {
								return kicad_error(subtree, "unexpected fp_arc end X null node.");
							}
							if (l->children->next != NULL && l->children->next->str != NULL) {
								SEEN_NO_DUP(featureTally, 5);
								val = strtod(l->children->next->str, &end);
								if (*end != 0) {
									return kicad_error(subtree, "error parsing fp_arc end Y.");
								}
								else {
									endY = PCB_MM_TO_COORD(val);
								}
							}
							else {
								return kicad_error(subtree, "unexpected fp_arc end Y null node.");
							}
						}
#warning TODO: this is just code duplication of fp_line code
						else if (l->str != NULL && strcmp("layer", l->str) == 0) {
							SEEN_NO_DUP(featureTally, 6);
							if (l->children != NULL && l->children->str != NULL) {
								SEEN_NO_DUP(featureTally, 7);
								subc_layer = kicad_get_subc_layer(st, subc, l->children->str, subc_layer_str);
								if (subc_layer == NULL)
									return kicad_error(l, "unable to set subcircuit layer");
							}
							else {
								pcb_message(PCB_MSG_ERROR, "\tusing default module layer for gr_line element\n");
								subc_layer = kicad_get_subc_layer(st, subc, NULL, subc_layer_str); /* default to module layer */
								/* return -1; */
							}
							if (subc_layer == NULL)
								return kicad_error(l, "unable to set subcircuit layer");
						}
						else if (l->str != NULL && strcmp("width", l->str) == 0) {
							SEEN_NO_DUP(featureTally, 8);
							if (l->children != NULL && l->children->str != NULL) {
								SEEN_NO_DUP(featureTally, 9);
								val = strtod(l->children->str, &end);
								if (*end != 0) {
									return kicad_error(subtree, "error parsing fp_arc width.");
								}
								else {
									Thickness = PCB_MM_TO_COORD(val);
								}
							}
							else {
								return kicad_error(subtree, "unexpected fp_arc width null node.");
							}
						}
						else if (l->str != NULL && strcmp("angle", l->str) == 0) {
							SEEN_NO_DUP(featureTally, 10);
							if (l->children != NULL && l->children->str != NULL) {
								SEEN_NO_DUP(featureTally, 11);
								val = strtod(l->children->str, &end);
								if (*end != 0) {
									return kicad_error(subtree, "error parsing fp_arc angle.");
								}
								else {
									delta = val;
								}
							}
							else {
								return kicad_error(subtree, "unexpected fp_arc angle null node.");
							}
						}
						else if (l->str != NULL && strcmp("net", l->str) == 0) { /* unlikely to be used or seen */
							SEEN_NO_DUP(featureTally, 12);
							if (l->children != NULL && l->children->str != NULL) {
								SEEN_NO_DUP(featureTally, 13);
							}
							else {
								return kicad_error(subtree, "unexpected fp_arc net null node.");
							}
						}
						else {
							if (n->str != NULL) {
								/*pcb_trace("Unknown gr_arc argument %s:", l->str); */
							}
							return kicad_error(subtree, "unexpected fp_arc null node.");
						}
					}
				}
				required = BV(0) | BV(6) | BV(8);
				if (((featureTally & required) == required) && subc != NULL) {
					moduleEmpty = 0;
					/* need start, layer, thickness at a minimum */
					/* same code used above for gr_arc parsing */
					width = height = pcb_distance(centreX, centreY, endX, endY); /* calculate radius of arc */
					if (width < 1) { /* degenerate case */
						startAngle = 0;
					}
					else {
						endAngle = 180 + 180 * atan2(-(endY - centreY), endX - centreX) / M_PI; /* avoid using atan2 with zero parameters */
						if (endAngle < 0.0) {
							endAngle += 360.0; /*make it 0...360 */
						}
						startAngle = (endAngle - delta); /* geda is 180 degrees out of phase with kicad, and opposite direction rotation */
						if (startAngle > 360.0) {
							startAngle -= 360.0;
						}
						if (startAngle < 0.0) {
							startAngle += 360.0;
						}
					}
					pcb_arc_new(subc_layer, moduleX + centreX, moduleY + centreY, width, height, startAngle, delta, Thickness, 0, pcb_no_flags());
				}

/* ********************************************************** */


			}
			else {
				if (n->str != NULL) {
					/*pcb_trace("Unknown pad argument : %s\n", n->str); */
				}
			}
		}


		if (subc != NULL) {
			if (moduleEmpty) { /* should try and use module empty function here */
#warning TODO: why do we try to do this? an error is an error
				Thickness = PCB_MM_TO_COORD(0.200);
/*				pcb_element_line_new(subc, moduleX, moduleY, moduleX + 1, moduleY + 1, Thickness);*/
				/*pcb_trace("\tEmpty Module!! 1nm line created at module centroid.\n"); */
			}

			pcb_subc_bbox(subc);
			pcb_add_subc_to_data(st->pcb->Data, subc);
			if (st->pcb->Data->subc_tree == NULL)
				st->pcb->Data->subc_tree = pcb_r_create_tree();
			pcb_r_insert_entry(st->pcb->Data->subc_tree, (pcb_box_t *)subc);

			if (moduleRotation != 0) {
#warning TODO: fix rotation code for non-90
				moduleRotation = moduleRotation / 90; /* ignore rotation != n*90 for now */
				pcb_subc_rotate90(subc, moduleX, moduleY, moduleRotation);
				/* can test for rotation != n*90 degrees if necessary, and call
				 * void pcb_element_rotate(pcb_data_t *Data, pcb_element_t *Element,
				 *    pcb_coord_t X, pcb_coord_t Y, double 
				 *    cosa, double sina, pcb_angle_t angle);
				 */
			}
			return 0;
		}
		else {
			return kicad_error(subtree, "unable to create incomplete module.");
		}
	}
	else {
		return kicad_error(subtree, "module parsing failure.");
	}
	return kicad_error(subtree, "Internal error #4");
}

static int kicad_parse_zone(read_state_t *st, gsxl_node_t *subtree)
{

	gsxl_node_t *n, *m;
	int i;
	int polycount = 0;
	long j = 0;
	unsigned long tally = 0;
	unsigned long required;

	pcb_poly_t *polygon = NULL;
	pcb_flag_t flags = pcb_flag_make(PCB_FLAG_CLEARPOLY);
	char *end;
	double val;
	pcb_coord_t X, Y;
	int PCBLayer = 0;

	if (subtree->str != NULL) {
		/*pcb_trace("Zone element found:\t'%s'\n", subtree->str); */
		for(n = subtree, i = 0; n != NULL; n = n->next, i++) {
			if (n->str != NULL && strcmp("net", n->str) == 0) {
				SEEN_NO_DUP(tally, 0);
				if (n->children != NULL && n->children->str != NULL) {
					/*pcb_trace("\tzone net number:\t'%s'\n", (n->children->str)); */
				}
				else {
					return kicad_error(subtree, "unexpected zone net null node.");
				}
			}
			else if (n->str != NULL && strcmp("net_name", n->str) == 0) {
				SEEN_NO_DUP(tally, 1);
				if (n->children != NULL && n->children->str != NULL) {
					/*pcb_trace("\tzone net_name:\t'%s'\n", (n->children->str)); */
				}
				else {
					return kicad_error(subtree, "unexpected zone net_name null node.");
				}
			}
			else if (n->str != NULL && strcmp("tstamp", n->str) == 0) {
				SEEN_NO_DUP(tally, 2);
				if (n->children != NULL && n->children->str != NULL) {
					/*pcb_trace("\tzone tstamp:\t'%s'\n", (n->children->str)); */
				}
				else {
					return kicad_error(subtree, "unexpected zone tstamp null node.");
				}
			}
			else if (n->str != NULL && strcmp("hatch", n->str) == 0) {
				SEEN_NO_DUP(tally, 3);
				if (n->children != NULL && n->children->str != NULL) {
					/*pcb_trace("\tzone hatch_edge:\t'%s'\n", (n->children->str)); */
					SEEN_NO_DUP(tally, 4); /* same as ^= 1 was */
				}
				else {
					return kicad_error(subtree, "unexpected zone hatch null node.");
				}
				if (n->children->next != NULL && n->children->next->str != NULL) {
					/*pcb_trace("\tzone hatching size:\t'%s'\n", (n->children->next->str)); */
					SEEN_NO_DUP(tally, 5);
				}
				else {
					return kicad_error(subtree, "unexpected zone hatching null node.");
				}
			}
			else if (n->str != NULL && strcmp("connect_pads", n->str) == 0) {
				SEEN_NO_DUP(tally, 6);
				if (n->children != NULL && n->children->str != NULL && (strcmp("clearance", n->children->str) == 0) && (n->children->children->str != NULL)) {
					/*pcb_trace("\tzone clearance:\t'%s'\n", (n->children->children->str));  this is if yes/no flag for connected pads is absent */
					SEEN_NO_DUP(tally, 7); /* same as ^= 1 was */
				}
				else if (n->children != NULL && n->children->str != NULL && n->children->next->str != NULL) {
					/*pcb_trace("\tzone connect_pads:\t'%s'\n", (n->children->str)); this is if the optional(!) yes or no flag for connected pads is present */
					SEEN_NO_DUP(tally, 8); /* same as ^= 1 was */
					if (n->children->next != NULL && n->children->next->str != NULL && n->children->next->children != NULL && n->children->next->children->str != NULL) {
						if (strcmp("clearance", n->children->next->str) == 0) {
							SEEN_NO_DUP(tally, 9);
							/*pcb_trace("\tzone connect_pads clearance: '%s'\n", (n->children->next->children->str)); */
						}
						else {
							/*pcb_trace("Unrecognised zone connect_pads option %s\n", n->children->next->str); */
						}
					}
				}
			}
			else if (n->str != NULL && strcmp("layer", n->str) == 0) {
				SEEN_NO_DUP(tally, 10);
				if (n->children != NULL && n->children->str != NULL) {
					PCBLayer = kicad_get_layeridx(st, n->children->str);
					if (PCBLayer < 0) {
						return kicad_warning(subtree, "parse error: zone layer <0.");
					}
					polygon = pcb_poly_new(&st->pcb->Data->Layer[PCBLayer], 0, flags);
				}
				else {
					return kicad_error(subtree, "unexpected zone layer null node.");
				}
			}
			else if (n->str != NULL && strcmp("polygon", n->str) == 0) {
				polycount++; /*keep track of number of polygons in zone */
				if (n->children != NULL && n->children->str != NULL) {
					if (strcmp("pts", n->children->str) == 0) {
						for(m = n->children->children, j = 0; m != NULL; m = m->next, j++) {
							if (m->str != NULL && strcmp("xy", m->str) == 0) {
								if (m->children != NULL && m->children->str != NULL) {
									/*pcb_trace("\tvertex X[%d]:\t'%s'\n", j, (m->children->str)); */
									val = strtod(m->children->str, &end);
									if (*end != 0) {
										return kicad_error(subtree, "error parsing zone vertex X.");
									}
									else {
										X = PCB_MM_TO_COORD(val);
									}
									if (m->children->next != NULL && m->children->next->str != NULL) {
										/*pcb_trace("\tvertex Y[%d]:\t'%s'\n", j, (m->children->next->str)); */
										val = strtod(m->children->next->str, &end);
										if (*end != 0) {
											return kicad_error(subtree, "error parsing zone vertex Y.");
										}
										else {
											Y = PCB_MM_TO_COORD(val);
										}
										if (polygon != NULL) {
											pcb_poly_point_new(polygon, X, Y);
										}
									}
									else {
										return kicad_error(subtree, "unexpected zone vertex coord null node.");
									}
								}
								else {
									return kicad_error(subtree, "unexpected zone vertex null node.");
								}
							}
						}
					}
					else {
						return kicad_error(subtree, "pts section vertices not found in zone polygon.");
					}
				}
				else {
					return kicad_error(subtree, "error parsing empty polygon.");
				}
			}
			else if (n->str != NULL && strcmp("fill", n->str) == 0) {
				SEEN_NO_DUP(tally, 11);
				for(m = n->children; m != NULL; m = m->next) {
					if (m->str != NULL && strcmp("arc_segments", m->str) == 0) {
						if (m->children != NULL && m->children->str != NULL) {
							/*pcb_trace("\tzone arc_segments:\t'%s'\n", (m->children->str)); */
						}
						else {
							return kicad_error(subtree, "unexpected zone arc_segment null node.");
						}
					}
					else if (m->str != NULL && strcmp("thermal_gap", m->str) == 0) {
						if (m->children != NULL && m->children->str != NULL) {
							/*pcb_trace("\tzone thermal_gap:\t'%s'\n", (m->children->str)); */
						}
						else {
							return kicad_error(subtree, "unexpected zone thermal_gap null node.");
						}
					}
					else if (m->str != NULL && strcmp("thermal_bridge_width", m->str) == 0) {
						if (m->children != NULL && m->children->str != NULL) {
							/*pcb_trace("\tzone thermal_bridge_width:\t'%s'\n", (m->children->str)); */
						}
						else {
							return kicad_error(subtree, "unexpected zone thermal_bridge_width null node.");
						}
					}
					else if (m->str != NULL) {
						/*pcb_trace("Unknown zone fill argument:\t%s\n", m->str); */
					}
				}
			}
			else if (n->str != NULL && strcmp("min_thickness", n->str) == 0) {
				SEEN_NO_DUP(tally, 12);
				if (n->children != NULL && n->children->str != NULL) {
					/*pcb_trace("\tzone min_thickness:\t'%s'\n", (n->children->str)); */
				}
				else {
					return kicad_error(subtree, "unexpected zone min_thickness null node.");
				}
			}
			else if (n->str != NULL && strcmp("filled_polygon", n->str) == 0) {
				/*pcb_trace("\tIgnoring filled_polygon definition.\n"); */
			}
			else {
				if (n->str != NULL) {
					/*pcb_trace("Unknown polygon argument:\t%s\n", n->str); */
				}
			}
		}
	}

	required = BV(10);
	if ((tally & required) == required) { /* has location, layer, size and stroke thickness at a minimum */
		if (polygon != NULL) {
			pcb_add_poly_on_layer(&st->pcb->Data->Layer[PCBLayer], polygon);
			pcb_poly_init_clip(st->pcb->Data, &st->pcb->Data->Layer[PCBLayer], polygon);
		}
		return 0;
	}
	return -1;
}


/* Parse a board from &st->dom into st->pcb */
static int kicad_parse_pcb(read_state_t *st)
{
	/* gsxl_node_t *n;  not used */
	static const dispatch_t disp[] = { /* possible children of root */
		{"version", kicad_parse_version},
		{"host", kicad_parse_nop},
		{"general", kicad_parse_nop},
		{"page", kicad_parse_page_size},
		{"title_block", kicad_parse_title_block},
		{"layers", kicad_parse_layer_definitions}, /* board layer defs */
		{"setup", kicad_parse_nop},
		{"net", kicad_parse_net}, /* net labels if child of root, otherwise net attribute of element */
		{"net_class", kicad_parse_nop},
		{"module", kicad_parse_module}, /* for footprints */
		{"dimension", kicad_parse_nop}, /* for dimensioning features */
		{"gr_line", kicad_parse_gr_line},
		{"gr_arc", kicad_parse_gr_arc},
		{"gr_circle", kicad_parse_gr_arc},
		{"gr_text", kicad_parse_gr_text},
		{"via", kicad_parse_via},
		{"segment", kicad_parse_segment},
		{"zone", kicad_parse_zone}, /* polygonal zones */
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

	FP = pcb_fopen(Filename, "r");
	if (FP == NULL)
		return -1;

	/* set up the parse context */
	memset(&st, 0, sizeof(st));
	st.pcb = Ptr;
	st.Filename = Filename;
	st.settings_dest = settings_dest;
	htsi_init(&st.layer_k2i, strhash, strkeyeq);

	/* load the file into the dom */
	gsxl_init(&st.dom, gsxl_node_t);
	st.dom.parse.line_comment_char = '#';
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

	pcb_layer_auto_fixup(Ptr);

	if (pcb_board_normalize(Ptr) > 0)
		pcb_message(PCB_MSG_WARNING, "Had to make changes to the coords so that the design fits the board.\n");


#warning TODO: free the layer hash

	return readres;
}

int io_kicad_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
{
	char line[1024], *s;

	if (typ != PCB_IOT_PCB)
		return 0; /* support only boards for now - kicad footprints are in the legacy format */

	while(!(feof(f))) {
		if (fgets(line, sizeof(line), f) != NULL) {
			s = line;
			while(isspace(*s))
				s++; /* strip leading whitespace */
			if (strncmp(s, "(kicad_pcb", 10) == 0) /* valid root */
				return 1;
			if ((*s == '\r') || (*s == '\n') || (*s == '#') || (*s == '\0')) /* ignore empty lines and comments */
				continue;
			/* non-comment, non-empty line - and we don't have our root -> it's not an s-expression */
			return 0;
		}
	}

	/* hit eof before seeing a valid root -> bad */
	return 0;
}

/*
 *				COPYRIGHT
 *
 *	pcb-rnd, interactive printed circuit board design
 *	Copyright (C) 2016, 2017 Tibor 'Igor2' Palinkas
 *	Copyright (C) 2016, 2017 Erich S. Heinzle
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

#include "config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <genht/htsi.h>
#include <qparse/qparse.h>
#include "compat_misc.h"
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
#include "../src_plugins/boardflip/boardflip.h"
#include "hid_actions.h"


#define MAXREAD 45

/* remove leading whitespace */
#define ltrim(s) while(isspace(*s)) s++

/* remove trailing newline */
#define rtrim(s) \
	do { \
		char *end; \
		for(end = s + strlen(s) - 1; (end >= s) && ((*end == '\r') || (*end == '\n')); end--) \
			*end = '\0'; \
	} while(0)

/* the following sym_attr_t struct, #define... and sym_flush() routine could go in a shared lib */

typedef struct {
	char *refdes;
	char *value;
	char *footprint;
} symattr_t;

#define null_empty(s) ((s) == NULL ? "" : (s))

static void sym_flush(symattr_t *sattr)
{
	if (sattr->refdes != NULL) {
/*	      pcb_trace("protel autotrax sym: refdes=%s val=%s fp=%s\n", sattr->refdes, sattr->value, sattr->footprint);*/
		if (sattr->footprint == NULL)
			pcb_message(PCB_MSG_ERROR, "protel autotrax: not importing refdes=%s: no footprint specified\n", sattr->refdes);
		else
			pcb_hid_actionl("ElementList", "Need", null_empty(sattr->refdes), null_empty(sattr->footprint), null_empty(sattr->value), NULL);
	}
	free(sattr->refdes); sattr->refdes = NULL;
	free(sattr->value); sattr->value = NULL;
	free(sattr->footprint); sattr->footprint = NULL;
}


/* the followinf read_state_t struct could go in a shared lib for parsers doing layer mapping */

typedef struct {
	pcb_board_t *PCB;
	const char *Filename;
	conf_role_t settings_dest;
	htsi_t layer_protel2i; /* protel layer name-to-index hash; name is the protel layer, index is the pcb-rnd layer index */
} read_state_t;

static int autotrax_parse_net(read_state_t *st, FILE *FP); /* describes netlists for the layout */

static int autotrax_get_layeridx(read_state_t *st, const int autotrax_layer);

/* autotrax_free_text/component_text */
static int autotrax_parse_text(read_state_t *st, FILE *FP, pcb_element_t *el)
{
	int heightMil;

	char line[MAXREAD], *t;
	pcb_coord_t X, Y, linewidth;
	int scaling = 100;
	unsigned direction = 0; /* default is horizontal */
	pcb_flag_t Flags = pcb_flag_make(0); /* default */
	int PCBLayer = 0; /* sane default value */

	if (fgets(line, sizeof(line), FP) != NULL) {
		int argc;
		char **argv, *s;

		s = line;
		ltrim(s);
		rtrim(s);
		argc = qparse2(s, &argv, 0);
		if (argc > 5) {
			X = pcb_get_value_ex(argv[0], NULL, NULL, NULL, "mil", NULL);
			pcb_trace("Found free text X : %ld\n", X);
			Y = pcb_get_value_ex(argv[1], NULL, NULL, NULL, "mil", NULL);
			pcb_trace("Found free text Y : %ld\n", Y);
			heightMil = pcb_get_value_ex(argv[2], NULL, NULL, NULL, NULL, NULL);
			scaling = (100*heightMil)/60;
			pcb_trace("Found free text height(mil) : %d, giving scaling: %d\n", heightMil, scaling);
			direction = pcb_get_value_ex(argv[3], NULL, NULL, NULL, NULL, NULL);
			pcb_trace("Found free text rotation : %d\n", direction);
			linewidth = pcb_get_value_ex(argv[4], NULL, NULL, NULL, "mil", NULL);
			pcb_trace("Found free text linewidth : %ld\n", linewidth);
			PCBLayer = autotrax_get_layeridx(st,
					pcb_get_value_ex(argv[5], NULL, NULL, NULL, NULL, NULL)); 
			pcb_trace("Found free text layer : %d\n", PCBLayer);
			/* we ignore the user routed flag */
			qparse_free(argc, &argv);
		} else {
			pcb_trace("error: insufficient free string attribute fields\n");
			qparse_free(argc, &argv);
			return -1;
		}
	}

	if (fgets(line, sizeof(line), FP) == NULL) {
		pcb_trace("error parsing free string text line; empty field\n");
		strcpy(line, "(empty text field)");
	} /* this helps the parser fail more gracefully if excessive newlines, or empty text field */

	pcb_trace("Found text string for display : %s\n", line);
	t = line;
	ltrim(t);
	rtrim(t); /*need to remove trailing '\r' to avoid rendering oddities */

	/* ABOUT HERE, CAN DO ROTATION/DIRECTION CONVERSION */

	if (PCBLayer >= 0) {
		if (el == NULL && st != NULL) {
			pcb_text_new( &st->PCB->Data->Layer[PCBLayer], pcb_font(st->PCB, 0, 1), X, Y, direction, scaling, t, Flags);
			pcb_trace("\tnew free text on layer %d created\n", PCBLayer);
			return 1;
		} else if (el != NULL && st != NULL) {
			pcb_trace("\ttext within component not supported\n");
			/* this may change with subcircuits */
			return 1;
		} else if (strlen(t) == 0){
			pcb_trace("\tempty text not placed on layout\n");
			return 0;
		} 
	}
	return -1;
}

/* autotrax_pcb free_track/component_track */
static int autotrax_parse_track(read_state_t *st, FILE *FP, pcb_element_t *el)
{
	char line[MAXREAD];
	pcb_coord_t X1, Y1, X2, Y2, Thickness, Clearance;
	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	int PCBLayer = 0; /* sane default value */
	int success;
	int valid = 1;

	Clearance = Thickness = PCB_MIL_TO_COORD(10); /* start with sane default of ten mil */

	if (fgets(line, sizeof(line), FP) != NULL) {
		int argc;
		char **argv, *s;

		s = line;
		ltrim(s);
		rtrim(s);
		argc = qparse2(s, &argv, 0);
		if (argc > 5) {
			X1 = pcb_get_value_ex(argv[0], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found tarck X1 : %ld\n", X1);
			Y1 = pcb_get_value_ex(argv[1], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found track Y1 : %ld\n", Y1);
			X2 = pcb_get_value_ex(argv[2], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found track X2 : %ld\n", X2);
			Y2 = pcb_get_value_ex(argv[3], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found track Y2 : %ld\n", Y2);
			Thickness = pcb_get_value_ex(argv[4], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found track linewidth : %ld\n", Thickness);
			PCBLayer = autotrax_get_layeridx(st,
					pcb_get_value_ex(argv[5], NULL, NULL, NULL, NULL, &success));
			valid &= success;
			pcb_trace("Found track layer : %d\n", PCBLayer);
			/* we ignore the user routed flag */
			qparse_free(argc, &argv);
		} else {
			pcb_trace("error: insufficient track attribute fields\n");
			qparse_free(argc, &argv);
			return -1;
		}
	}

	if (! valid) {
		pcb_trace("error: text attributes unable to be parsed:\n\t%s\n", line);
		return -1;
	}

	if (PCBLayer >= 0) {
		if (el == NULL && st != NULL) {
			pcb_line_new( &st->PCB->Data->Layer[PCBLayer], X1, Y1, X2, Y2,
				Thickness, Clearance, Flags);
			pcb_trace("\tnew free line on layer %d created\n", PCBLayer);
			return 1;
		} else if (el != NULL && st != NULL) {
			pcb_element_line_new(el, X1, Y1, X2, Y2, Thickness);
			pcb_trace("\tnew free line in component created\n");
			return 1;
		}
	}
	return -1;
}

/* autotrax_pcb free arc and component arc parser */
static int autotrax_parse_arc(read_state_t *st, FILE *FP, pcb_element_t *el)
{
	char line[MAXREAD];
	int segments = 15; /* full circle by default */ 
	int success;
	int valid = 1;

	pcb_coord_t centreX, centreY, width, height, Thickness, Clearance, radius;
	pcb_angle_t startAngle = 0.0;
	pcb_angle_t delta = 360.0;

	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	int PCBLayer = 0; /* sane default value */

	Clearance = Thickness = PCB_MIL_TO_COORD(10); /* start with sane default of ten mil */

	if (fgets(line, sizeof(line), FP) != NULL) {
		int argc;
		char **argv, *s;
		s = line;
		ltrim(s);
		rtrim(s);
		argc = qparse2(s, &argv, 0);
		if (argc > 5) {
			centreX = pcb_get_value_ex(argv[0], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found arc centreX : %ld\n", centreX);
			centreY = pcb_get_value_ex(argv[1], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found arc centreY : %ld\n", centreY);
			radius = pcb_get_value_ex(argv[2], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found arc radius : %ld\n", radius);
			segments = pcb_get_value_ex(argv[3], NULL, NULL, NULL, NULL, &success);
			valid &= success;
			pcb_trace("Found ard segments : %ld\n", segments);
			Thickness = pcb_get_value_ex(argv[4], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found arc linewidth : %ld\n", Thickness);
			PCBLayer = autotrax_get_layeridx(st,
					pcb_get_value_ex(argv[5], NULL, NULL, NULL, NULL, &success));
			valid &= success;
			pcb_trace("Found arc layer : %d\n", PCBLayer);
			/* we ignore the user routed flag */
			qparse_free(argc, &argv);
		} else { 
			qparse_free(argc, &argv);
			pcb_trace("error: insufficient track attribute fields\n");
			return -1;
		}
	}

	if (! valid) {
		pcb_trace("error: arc attributes unable to be parsed:\n\t%s\n", line);
		return -1;
	}


	width = height = radius;

/* we now need to decrypt the segments value. Some definitions will represent two arcs.
Bit 0 : RU quadrant
Bit 1 : LU quadrant
Bit 2 : LL quadrant
Bit 3 : LR quadrant

TODO: This needs further testing to ensure the refererence
document used reflects actual outputs from protel autotrax
*/
	if (segments == 10) { /* LU + RL quadrants */
		startAngle = 90.0;
		delta = 90.0;
		pcb_arc_new( &st->PCB->Data->Layer[PCBLayer], centreX, centreY, width, height, startAngle, delta, Thickness, Clearance, Flags);
		startAngle = 270.0;
	} else if (segments == 5) { /* RU + LL quadrants */
		startAngle = 0.0;
		delta = 90.0;
		pcb_arc_new( &st->PCB->Data->Layer[PCBLayer], centreX, centreY, width, height, startAngle, delta, Thickness, Clearance, Flags);
		startAngle = 180.0;
	} else if (segments >= 15) { /* whole circle */
		startAngle = 0.0;
		delta = 360.0;
	} else if (segments == 1) { /* RU quadrant */
		startAngle = 180.0;
		delta = 90.0;
	} else if (segments == 2) { /* LU quadrant */
		startAngle = 270.0;
		delta = 90.0;
	} else if (segments == 4) { /* LL quadrant */
		startAngle = 0.0;
		delta = 90.0;
	} else if (segments == 8) { /* RL quadrant */
		startAngle = 90.0;
		delta = 90.0;
	} else if (segments == 3) { /* Upper half */
		startAngle = 180.0;
		delta = -180.0; /* this seems to fix IC notches */
	} else if (segments == 6) { /* Left half */
		startAngle = 270.0;
		delta = 180.0;
	} else if (segments == 12) { /* Lower half */
		startAngle = 0.0;
		delta = -180.0;  /* this seems to fix IC notches */
	} else if (segments == 9) { /* Right half */
		startAngle = 90.0;
		delta = 180.0;
	} else if (segments == 14) { /* not RUQ */
		startAngle = -90.0;
		delta = 270.0;
	} else if (segments == 13) { /* not LUQ */
		startAngle = 0.0;
		delta = 270.0;
	} else if (segments == 11) { /* not LLQ */
		startAngle = 90.0;
		delta = 270.0;
	} else if (segments == 7) { /* not RLQ */
		startAngle = 180.0;
		delta = 270.0;
	}  

	if (PCBLayer >= 0) {
		if (el == NULL && st != NULL) {
			pcb_arc_new( &st->PCB->Data->Layer[PCBLayer], centreX, centreY, width, height, startAngle, delta, Thickness, Clearance, Flags);
			pcb_trace("\tnew free arc on layer %d created\n", PCBLayer);
			return 1;
		} else if (el != NULL && st != NULL) {
			pcb_element_arc_new(el, centreX, centreY, width, height, startAngle, delta, Thickness);
			pcb_trace("\tnew arc in component created\n");
			return 1;
		}
	}
	return -1;
}

/* autotrax_pcb via parser */
static int autotrax_parse_via(read_state_t *st, FILE *FP, pcb_element_t *el)
{
	char line[MAXREAD];
	char * name;
	int success;
	int valid = 1;

	pcb_coord_t X, Y, Thickness, Clearance, Mask, Drill; /* not sure what to do with mask */
	pcb_flag_t Flags = pcb_flag_make(0);

	Clearance = Mask = Thickness = PCB_MIL_TO_COORD(10); /* start with sane default of ten mil */

	Drill = PCB_MM_TO_COORD(0.300); /* start with something sane */


	name = pcb_strdup("unnamed");

	if (fgets(line, sizeof(line), FP) != NULL) {
		int argc;
		char **argv, *s;
		s = line;
		ltrim(s);
		rtrim(s);
		argc = qparse2(s, &argv, 0);
		if (argc >= 4) {
			X = pcb_get_value_ex(argv[0], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found via X : %ld\n", X);
			Y = pcb_get_value_ex(argv[1], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found via Y : %ld\n", Y);
			Thickness = pcb_get_value_ex(argv[2], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found via Thickness : %ld\n", Thickness);
			Drill = pcb_get_value_ex(argv[3], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found via Drill : %ld\n", Drill);
			qparse_free(argc, &argv);
		} else {
			qparse_free(argc, &argv);
			pcb_trace("error: insufficient via attribute fields\n");
			return -1;
		}
	}

	if (! valid) {
		pcb_trace("error: via attributes unable to be parsed:\n\t%s\n", line);
		return -1;
	}

	if (el == NULL) {
		pcb_via_new( st->PCB->Data, X, Y, Thickness, Clearance, Mask, Drill, name, Flags);
		pcb_trace("\tnew free via created\n");
		return 1;
	} else if (el != NULL && st != NULL) {
		pcb_via_new( st->PCB->Data, X, Y, Thickness, Clearance, Mask, Drill, name, Flags);
		pcb_trace("\tnew free via created\n");
		return 1;
	}
	return -1;

}

/* autotrax_pcb free or component pad
x y xsize ysize shape holesize pwr/gnd layer
padname
may need to think about hybrid outputs, like pad + hole, to match possible features in protel autotrax
*/
static int autotrax_parse_pad(read_state_t *st, FILE *FP, pcb_element_t *el)
{

	char line[MAXREAD], *s;
	int Connects = 0;
	int Shape = 0;
	int AutotraxLayer = 0;
	int PCBLayer = 0; /* sane default value */

	int valid = 1;
	int success;

	pcb_coord_t X, Y, Xsize, Ysize, Thickness, Clearance, Mask, Drill; 
	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */

	Clearance = Mask = Thickness = PCB_MIL_TO_COORD(10); /* start with sane default of ten mil */

	Drill = PCB_MM_TO_COORD(0.300); /* start with something sane */



	if (fgets(line, sizeof(line), FP) != NULL) {
		int argc;
		char **argv;
		s = line;
		ltrim(s);
		rtrim(s);
		argc = qparse2(s, &argv, 0);
		if (argc > 6) {
			X = pcb_get_value_ex(argv[0], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found pad X : %ld\n", X);
			Y = pcb_get_value_ex(argv[1], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found pad Y : %ld\n", Y);
			Xsize = pcb_get_value_ex(argv[2], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found pad Xsize : %ld\n", Xsize);
			Ysize = pcb_get_value_ex(argv[3], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found pad Ysize : %ld\n", Ysize);
			Shape = pcb_get_value_ex(argv[4], NULL, NULL, NULL, NULL, &success);
			valid &= success;
			pcb_trace("Found pad Shape : %ld\n", Shape);
			Drill = pcb_get_value_ex(argv[5], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found pad Drill : %ld\n", Drill);
			Connects = pcb_get_value_ex(argv[6], NULL, NULL, NULL, NULL, &success);
			valid &= success; /* which specifies GND or Power connection for pin/pad/via */
			pcb_trace("Found pad Connects PWR/GND : %ld\n", Connects);
			PCBLayer = autotrax_get_layeridx(st,
					pcb_get_value_ex(argv[6], NULL, NULL, NULL, NULL, &success));
			valid &= success;
			pcb_trace("Found pad layer : %d\n", PCBLayer);
			qparse_free(argc, &argv);
		} else {
			qparse_free(argc, &argv);
			pcb_trace("error: insufficient pad attribute fields\n");
			return -1;
		}
	}

	if (! valid) {
		pcb_trace("error: pad attributes unable to be parsed:\n\t%s\n", line);
		return -1;
	}

/* now find name as string on next line and copy it */

	if (fgets(line, sizeof(line), FP) == NULL) {
		pcb_trace("error parsing pad free string text line; empty\n");
		return -1;
	}
	s = line;
	rtrim(s); /* avoid rendering oddities on layout, and netlist matching confusion */
	pcb_trace("Found free pad name : %s\n", line);

	/* these features can have connections to GND and PWR	
	   planes specified in protel autotrax (seems rare though)
	   so we warn the user is this is the case */ 
	switch (Connects) {
		case 1:	pcb_message(PCB_MSG_ERROR, "pin clears PWR/GND.\n");
			break;
		case 2: pcb_message(PCB_MSG_ERROR, "pin requires relief to GND plane.\n");
			break;
		case 4: pcb_message(PCB_MSG_ERROR, "pin requires relief to PWR plane.\n");
			break;
		case 3: pcb_message(PCB_MSG_ERROR, "pin should connect to PWR plane.\n");
			break;
		case 5: pcb_message(PCB_MSG_ERROR, "pin should connect to GND plane.\n");
			break;
	}

	Thickness = MIN(Xsize, Ysize);

/* 	TODO: having fully parsed the free pad, and determined, rectangle vs octagon vs round
	the problem is autotrax can define an SMD pad, but we have to map this to a pin, since a
	discrete element would be needed for a standalone pad. Padstacks may allow more flexibility
	The onsolder flags are redundant for now with vias.

	currently ignore:
	shape:
		5 Cross Hair Target
		6 Moiro Target
*/

	if ((Shape == 5 || Shape == 6 ) && el == NULL) {
		pcb_trace("\tnew free pad not created; Cross Hair/Moiro target not supported\n");
	} else if ((Shape == 5 || Shape == 6) && el != NULL) {
		pcb_trace("\tnew element pad not created; Cross Hair/Moiro targets not supported\n");
	} else if (st != NULL && el == NULL) { /* pad not within element, i.e. a free pad/pin/via */
		if (Shape == 3) {
			Flags = pcb_flag_make(PCB_FLAG_OCTAGON);
		} else if (Shape == 2 || Shape == 4) {	
			Flags = pcb_flag_make(PCB_FLAG_SQUARE);
		}
		/* should this in fact be an SMD pad, +/- a hole in it ? */
		pcb_via_new( st->PCB->Data, X, Y, Thickness, Clearance, Mask, Drill, line, Flags);
		pcb_trace("\tnew free pad/hole created; need to check connects\n");
		return 0;
	} else if (st != NULL && el != NULL) { /* pad within element */
		if ((Shape == 2  || Shape == 4)  && AutotraxLayer == 1) { /* square (2) or rounded rect (4) on top layer */ 
			Flags = pcb_flag_make(PCB_FLAG_SQUARE); /* actually a rectangle, but... */
		} else if ((Shape == 2  || Shape == 4) && AutotraxLayer == 6) { /* bottom layer */ 
			Flags = pcb_flag_make(PCB_FLAG_SQUARE | PCB_FLAG_ONSOLDER); /*actually a rectangle, but... */
		} else if (Shape == 3 && AutotraxLayer == 1) {/* top layer*/ 
			Flags = pcb_flag_make(PCB_FLAG_OCTAGON);
		} else if (Shape == 3 && AutotraxLayer == 6) {  /*bottom layer */
			Flags = pcb_flag_make(PCB_FLAG_OCTAGON | PCB_FLAG_ONSOLDER); 
		}		
/* # TODO  not yet processing SMD pads - need examples to work with */
		if (Shape == 2 && Drill == 0) {/* is probably SMD */
/*			pcb_element_pad_new_rect(el, el->X - Xsize/2, el->Y - Ysize/2, Xsize/2 + el->X, Ysize/2 + el->Y, Clearance, 
				Clearance, line, linee, Flags);*/
/*			pcb_element_pad_new_rect(el, X - Xsize/2, Y - Ysize/2, X + Xsize/2, Ysize/2 + Y, Clearance, 
				Clearance, line, line, Flags);*/
			return 1;
		} else {
/*			pcb_element_pin_new(el, X + el->X, Y + el->Y, Thickness, Clearance, Clearance,  
				Drill, line, line, Flags);*/
			pcb_element_pin_new(el, X, Y , Thickness, Clearance, Clearance,  
				Drill, line, line, Flags);
			return 1;
		}
		pcb_trace("\tnew component pad/hole created; need to check connects\n");
		return 1;
	}
	pcb_trace("\tfailed to parse new pad/hole CP/FP\n");	
	return -1;
}

/* protel autorax free fill (rectangular pour) parser - the closest thing protel autotrax has to a polygon */
static int autotrax_parse_fill(read_state_t *st, FILE *FP, pcb_element_t *el)
{
	int success;
	int valid = 1;
	char line[MAXREAD];
	pcb_polygon_t *polygon = NULL;
	pcb_flag_t flags = pcb_flag_make(PCB_FLAG_CLEARPOLY);
	pcb_coord_t X1, Y1, X2, Y2, Clearance;
	int PCBLayer = 0;

	Clearance = PCB_MIL_TO_COORD(10);

	if (fgets(line, sizeof(line), FP) != NULL) {
		int argc;
		char **argv, *s;
		s = line;
		ltrim(s);
		rtrim(s);
		argc = qparse2(s, &argv, 0);
		if (argc >= 5) {
			X1 = pcb_get_value_ex(argv[0], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found fill X1 : %ld\n", X1);
			Y1 = pcb_get_value_ex(argv[1], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found fill Y1 : %ld\n", Y1);
			X2 = pcb_get_value_ex(argv[2], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found fill X2 : %ld\n", X2);
			Y2 = pcb_get_value_ex(argv[3], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found fill Y2 : %ld\n", Y2);
			PCBLayer = autotrax_get_layeridx(st,
					pcb_get_value_ex(argv[4], NULL, NULL, NULL, NULL, &success));
			valid &= success;
			pcb_trace("Found fill layer : %d\n", PCBLayer);
			qparse_free(argc, &argv);
		} else {
			qparse_free(argc, &argv);
			pcb_trace("error: insufficient fill attribute fields\n");
			return -1;
		}
	}

	if (! valid) {
		pcb_trace("error: fill attributes unable to be parsed:\n\t%s\n", line);
		return -1;
	}

	if (PCBLayer >= 0 && el == NULL) {
		polygon = pcb_poly_new(&st->PCB->Data->Layer[PCBLayer], flags);
	} else if (PCBLayer < 0 && el == NULL){
		pcb_trace("Invalid free fill layer found : %d\n", PCBLayer);
	}

	if (polygon != NULL && el == NULL && st != NULL) { /* a free fill, not in an element */ 
		pcb_poly_point_new(polygon, X1, Y1);
		pcb_poly_point_new(polygon, X2, Y1);
		pcb_poly_point_new(polygon, X2, Y2);
		pcb_poly_point_new(polygon, X1, Y2);
		pcb_add_polygon_on_layer(&st->PCB->Data->Layer[PCBLayer], polygon);
		pcb_poly_init_clip(st->PCB->Data, &st->PCB->Data->Layer[PCBLayer], polygon);
		return 1;
	} else if (polygon == NULL && el != NULL && st != NULL && PCBLayer == 1 ) { /* in an element, top layer copper */
		flags = pcb_flag_make(0);
		pcb_element_pad_new_rect(el, X1, Y1, X2, Y2, Clearance, Clearance, "", "", flags);
		return 1;
	} else if (polygon == NULL && el != NULL && st != NULL && PCBLayer == 6 ) { /* in an element, bottom layer copper */
		flags = pcb_flag_make(0);
		pcb_element_pad_new_rect(el, X1, Y1, X2, Y2, Clearance, Clearance, "", "", flags);				
		return 1;
	}
	return -1;
}

/* the following three functions could potetially go in a shared alien format parser lib */ 
/* Register an autotrax layer in the layer hash after looking up the pcb-rnd equivalent */
static unsigned int autotrax_reg_layer(read_state_t *st, const char *autotrax_layer, unsigned int mask)
{
	pcb_layer_id_t id;
	if (pcb_layer_list(mask, &id, 1) != 1) {
		pcb_layergrp_id_t gid;
		pcb_layergrp_list(PCB, mask, &gid, 1);
		id = pcb_layer_create(gid, autotrax_layer);
	}
	htsi_set(&st->layer_protel2i, pcb_strdup(autotrax_layer), id);
	return 0;
}

/* create a set of default layers, since protel has a static stackup */
static int autotrax_create_layers(read_state_t *st)
{

	unsigned int res;
	pcb_layer_id_t id = -1;
	pcb_layergrp_t *g = pcb_get_grp_new_intern(PCB, -1);

	/* set up the hash for implicit layers */
	res = 0;
	res |= autotrax_reg_layer(st, "TopSilk", PCB_LYT_SILK | PCB_LYT_TOP);
	res |= autotrax_reg_layer(st, "BottomSilk", PCB_LYT_SILK | PCB_LYT_BOTTOM);

	/* for modules */
	res |= autotrax_reg_layer(st, "Top", PCB_LYT_COPPER | PCB_LYT_TOP);
	res |= autotrax_reg_layer(st, "Bottom", PCB_LYT_COPPER | PCB_LYT_BOTTOM);

	if (res != 0) {
		pcb_message(PCB_MSG_ERROR, "Internal error: can't find a silk or mask layer\n");
		pcb_layergrp_inhibit_dec();
		return -1;
	}

	pcb_layergrp_fix_old_outline(PCB);
	
	id = pcb_layer_create(g - PCB->LayerGroups.grp, "Mid1");
	htsi_set(&st->layer_protel2i, pcb_strdup("2"), id);

	g = pcb_get_grp_new_intern(PCB, -1);
	id = pcb_layer_create(g - PCB->LayerGroups.grp, "Mid2");
	htsi_set(&st->layer_protel2i, pcb_strdup("3"), id);

	g = pcb_get_grp_new_intern(PCB, -1);
	id = pcb_layer_create(g - PCB->LayerGroups.grp, "Mid3");
	htsi_set(&st->layer_protel2i, pcb_strdup("4"), id);

	g = pcb_get_grp_new_intern(PCB, -1);
	id = pcb_layer_create(g - PCB->LayerGroups.grp, "Mid4");
	htsi_set(&st->layer_protel2i, pcb_strdup("5"), id);

	g = pcb_get_grp_new_intern(PCB, -1);
	id = pcb_layer_create(g - PCB->LayerGroups.grp, "GND");
	htsi_set(&st->layer_protel2i, pcb_strdup("9"), id);

	g = pcb_get_grp_new_intern(PCB, -1);
	id = pcb_layer_create(g - PCB->LayerGroups.grp, "Power");
	htsi_set(&st->layer_protel2i, pcb_strdup("10"), id);

	g = pcb_get_grp_new_intern(PCB, -1);
	id = pcb_layer_create(g - PCB->LayerGroups.grp, "Substrate");
	htsi_set(&st->layer_protel2i, pcb_strdup("11"), id);

	g = pcb_get_grp_new_misc(PCB);
	id = pcb_layer_create(g - PCB->LayerGroups.grp, "KeepOut");
	htsi_set(&st->layer_protel2i, pcb_strdup("12"), id);

	g = pcb_get_grp_new_misc(PCB);
	id = pcb_layer_create(g - PCB->LayerGroups.grp, "Multi");
	htsi_set(&st->layer_protel2i, pcb_strdup("13"), id);

	pcb_layergrp_fix_old_outline(PCB);

	pcb_layergrp_inhibit_dec();

	return 0;
}

/* Returns the pcb-rnd layer index for a autotrax_layer number, or -1 if not found */
static int autotrax_get_layeridx(read_state_t *st, const int autotrax_layer)
{
	char *textName;
	htsi_entry_t *e;

	switch (autotrax_layer) {
		case 1:	textName = pcb_strdup("Top");
			break;
		case 2:	textName = pcb_strdup("2");
			break;
		case 3:	textName = pcb_strdup("3");
			break;
		case 4:	textName = pcb_strdup("4");
			break;
		case 5:	textName = pcb_strdup("5");
			break;
		case 6:	textName = pcb_strdup("Bottom");
			break;
		case 7:	textName = pcb_strdup("TopSilk");
			break;
		case 8:	textName = pcb_strdup("BottomSilk");
			break;
		case 9:	textName = pcb_strdup("9");
			break;
		case 10:	textName = pcb_strdup("10");
			break;
		case 11:	textName = pcb_strdup("11");
			break;
		case 12:	textName = pcb_strdup("12");
			break;
		default: textName = pcb_strdup("13");
	}

	e = htsi_getentry(&st->layer_protel2i, textName);
	if (e == NULL)
		return -1;
	return e->value;
}

/* autotrax_pcb  autotrax_parse_net ;   used to read net descriptions for the entire layout */
static int autotrax_parse_net(read_state_t *st, FILE *FP)
{
	symattr_t sattr;

	char line[MAXREAD];
	char *netname, *s;
	int length = 0;
	int in_comp = 0;
	int in_net = 1;
	int in_node_table = 0;
	int endpcb = 0;

	netname = NULL;

	memset(&sattr, 0, sizeof(sattr));

	/* next line is the netlist name */
	pcb_trace("About to read netdef section.\n");
	
	if (fgets(line, sizeof(line), FP) != NULL) {
		s = line;
		rtrim(s);
		pcb_trace("new netlist being added: %s.\n", line);
		netname = pcb_strdup(line);
	} else {
		pcb_trace("empty netlist name found.\n");
		return -1;
	}
	fgets(line, sizeof(line), FP);
	s = line;
	rtrim(s);
	pcb_trace("netlist visibility flag: %s.\n", line);
	while (!feof(FP) && !endpcb && in_net) {
		fgets(line, sizeof(line), FP);
		if (strncmp(line, "[", 1) == 0 ) {
			pcb_trace("Entering netlist component definition.\n");
			in_comp = 1;
			while (in_comp) {
				if (fgets(line, sizeof(line), FP) == NULL) {
					pcb_trace("Unexpected empty line in netlist component def.\n");
				} else {
					if (fgets(line, sizeof(line), FP) == NULL) {
						pcb_trace("Unexpected empty REFDES.\n");
					} else {
						s = line;
						rtrim(s);
						sym_flush(&sattr);
						free(sattr.refdes);
						sattr.refdes = pcb_strdup(line);
					}
					if (fgets(line, sizeof(line), FP) == NULL) {
						pcb_trace("Unexpected empty PACKAGE.\n");
						free(sattr.footprint);
						sattr.footprint = pcb_strdup("unknown");
					} else {
						s = line;
						rtrim(s);
						free(sattr.footprint);
						sattr.footprint = pcb_strdup(line);
					}
					if (fgets(line, sizeof(line), FP) == NULL) {
						free(sattr.value);
						sattr.value = pcb_strdup("value");
					} else {
						s = line;
						rtrim(s);
						free(sattr.value);
						sattr.value = pcb_strdup(line);
					}
				}
				while (fgets(line, sizeof(line), FP) == NULL) {
					/* clear empty lines in COMP definition */	
				}					
				if (strncmp(line, "]", 1) == 0 ) {
					in_comp = 0;	
				}
			}
		} else if (strncmp(line, "(", 1) == 0) {
			pcb_trace("Entering netlist node definitions.\n");
			in_net = 1;
 			while (in_net || in_node_table) {
				while (fgets(line, sizeof(line), FP) == NULL) {
					/* skip empty lines */
				}
				if (strncmp(line, ")", 1) == 0) {
					in_net = 0;
				} else if (strncmp(line, "{", 1) == 0) {
					in_node_table = 1;
					in_net = 0;
				} else if (strncmp(line, "}", 1) == 0) {
					in_node_table = 0;
				} else if (!in_node_table) {
					s = line;
					rtrim(s);
					pcb_trace("processing node definition:  %s\n", line);
					if (line != NULL && netname != NULL) {
						pcb_trace("Adding node to net: %s, %s\n",
								line, netname);
						pcb_hid_actionl("Netlist", "Add", netname, line, NULL);
					}
				}
			}
		} else if (length >= 6 && strncmp(line, "ENDPCB", 6) == 0 ) {
			pcb_trace("End of protel Autotrax file found in netlist section?!\n");
			endpcb = 1; /* if we get here, something went wrong */
		}
	}
	sym_flush(&sattr);

	return 0;
}

/* protel autotrax component definition parser */
/* no mark() or location as such it seems */
static int autotrax_parse_component(read_state_t *st, FILE *FP)
{
	int success;
	int valid = 1;
	int refdesScaling = 100;
	pcb_coord_t moduleX, moduleY, Thickness;
	unsigned direction = 0; /* default is horizontal */
	char moduleName[MAXREAD], moduleRefdes[MAXREAD], moduleValue[MAXREAD];
	pcb_element_t *newModule;

	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	pcb_flag_t TextFlags = pcb_flag_make(0); /* start with something bland here */

	char *s, line[MAXREAD];
	int nonempty = 0;
	int length = 0;

	if (fgets(moduleRefdes, sizeof(moduleRefdes), FP) == NULL) {
		strcpy(moduleRefdes, "unknown");
	}
	s = moduleRefdes;
	rtrim(s); /* avoid rendering oddities on layout */
	pcb_trace("Found component refdes : %s\n", moduleRefdes);
	if (fgets(moduleName, sizeof(moduleName), FP) == NULL) {
		strcpy(moduleName, "unknown");
	}
	s = moduleName;
	rtrim(s); /* avoid rendering oddities on layout */
	pcb_trace("Found component name : %s\n", moduleName);
	if (fgets(moduleValue, sizeof(moduleValue), FP) == NULL) {
		strcpy(moduleValue, "unknown");
	}
	s = moduleValue;
	rtrim(s); /* avoid rendering oddities on layout */
	pcb_trace("Found component description : %s\n", moduleValue);

/* with the header read, we now ignore the locations for the text fields in the next two lines... */
/* we also allow up to two empty lines in case of extra newlines */
	if (fgets(line, sizeof(line), FP) == NULL) {
		fgets(line, sizeof(line), FP);
	}
	if (fgets(line, sizeof(line), FP) == NULL) {
		fgets(line, sizeof(line), FP);
	}

	if (fgets(line, sizeof(line), FP) != NULL) {
		int argc;
		char **argv, *s;
		s = line;
		ltrim(s);
		rtrim(s);
		argc = qparse2(s, &argv, 0);
		if (argc >= 2) {
			moduleX = pcb_get_value_ex(argv[0], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found COMP moduleX : %ld\n", moduleX);
			moduleY = pcb_get_value_ex(argv[1], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found COMP moduleY : %ld\n", moduleY);
			qparse_free(argc, &argv);
		} else {
			qparse_free(argc, &argv);
			pcb_trace("error: insufficient COMP attribute fields\n");
			return -1;
		}
	}

	if (! valid) {
		pcb_trace("error: COMP attributes unable to be parsed:\n\t%s\n", line);
		return -1;
	}

	
	pcb_trace("Have new module name and location, defining module/element %s\n", moduleName);
	newModule = pcb_element_new(st->PCB->Data, NULL, 
								pcb_font(st->PCB, 0, 1), Flags,
								moduleName, moduleRefdes, moduleValue,
								moduleX, moduleY, direction,
								refdesScaling, TextFlags, pcb_false);

	while (!feof(FP)) {
		if (fgets(line, sizeof(line), FP) == NULL) {
			fgets(line, sizeof(line), FP);
		}
		nonempty = 0;
		s = line;
		length = strlen(line);
		if (length >= 7) {
			if (strncmp(line, "ENDCOMP", 7) == 0 ) {
				pcb_trace("Finished parsing component\n");
				if (!nonempty) { /* could try and use module empty function here */
					Thickness = PCB_MM_TO_COORD(0.200);
					pcb_element_line_new(newModule, moduleX, moduleY, moduleX+1, moduleY+1, Thickness);
					pcb_trace("\tEmpty Module!! 1nm line created at module centroid.\n");
				}
				pcb_element_bbox(st->PCB->Data, newModule, pcb_font(PCB, 0, 1));
				return 0;
			}
		} else if (length >= 2) {
			if (strncmp(s, "CT",2) == 0 ) {
				pcb_trace("Found component track\n");
				nonempty |= autotrax_parse_track(st, FP, newModule);
			} else if (strncmp(s, "CA",2) == 0 ) {
				pcb_trace("Found component arc\n");
				nonempty |= autotrax_parse_arc(st, FP, newModule);
			} else if (strncmp(s, "CV",2) == 0 ) {
				pcb_trace("Found component via\n");
				nonempty |= autotrax_parse_via(st, FP, newModule);
			} else if (strncmp(s, "CF",2) == 0 ) {
				pcb_trace("Found component fill\n");
				nonempty |= autotrax_parse_fill(st, FP, newModule);
			} else if (strncmp(s, "CP",2) == 0 ) {
				pcb_trace("Found component pad\n");
				nonempty |= autotrax_parse_pad(st, FP, newModule);
			} else if (strncmp(s, "CS",2) == 0 ) {
				pcb_trace("Found component String\n");
				nonempty |= autotrax_parse_text(st, FP, newModule);
			}
		}
	}
	return -1; /* should not get here */ 
}

	
int io_autotrax_read_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, conf_role_t settings_dest)
{
	int readres = 0;
	pcb_box_t boardSize, *box;
	read_state_t st;
	FILE *FP;

	pcb_element_t *el = NULL;

	int length = 0;
	int finished = 0;
	int netdefs = 0;
	char line[1024];
	char *s;

	FP = fopen(Filename, "r");
	if (FP == NULL)
		return -1;

	/* set up the parse context */
	memset(&st, 0, sizeof(st));
	st.PCB = Ptr;
	st.Filename = Filename;
	st.settings_dest = settings_dest;
	htsi_init(&st.layer_protel2i, strhash, strkeyeq);

	while (!feof(FP) && !finished) {
		if (fgets(line, sizeof(line), FP) == NULL) {
			fgets(line, sizeof(line), FP);
		}
		s = line;
		rtrim(s);
		length = strlen(line);
		if (length >= 10) {
			if (strncmp(line, "PCB FILE 4", 10) == 0 ) {
				pcb_trace("Found Protel Autotrax version 4\n");
				autotrax_create_layers(&st);
			} else if (strncmp(line, "PCB FILE 5", 10) == 0 ) {
				pcb_trace("Found Protel Easytrax version 5\n");
				autotrax_create_layers(&st);
			}
		} else if (length >= 6) {
			if (strncmp(s, "ENDPCB",6) == 0 ) {
				pcb_trace("Found end of file\n");
				finished = 1;
			} else if (strncmp(s, "NETDEF",6) == 0 ) {
				pcb_trace("Found net definition\n");
				if (netdefs == 0) {
					pcb_hid_actionl("ElementList", "start", NULL);
					pcb_hid_actionl("Netlist", "Freeze", NULL);
					pcb_hid_actionl("Netlist", "Clear", NULL);
				}
				netdefs |= 1;
				autotrax_parse_net(&st, FP);
			}
		} else if (length >= 4) {
			if (strncmp(line, "COMP",4) == 0 ) {
				pcb_trace("Found component\n");
				autotrax_parse_component(&st, FP);
			}
		} else if (length >= 2) {
			if (strncmp(s, "FT",2) == 0 ) {
				pcb_trace("Found free track\n");
				autotrax_parse_track(&st, FP, el);
			} else if (strncmp(s, "FA",2) == 0 ) {
				pcb_trace("Found free arc\n");
				autotrax_parse_arc(&st, FP, el);
			} else if (strncmp(s, "FV",2) == 0 ) {
				pcb_trace("Found free via\n");
				autotrax_parse_via(&st, FP, el);
			} else if (strncmp(s, "FF",2) == 0 ) {
				pcb_trace("Found free fill\n");
				autotrax_parse_fill(&st, FP, el);
			} else if (strncmp(s, "FP",2) == 0 ) {
				pcb_trace("Found free pad\n");
				autotrax_parse_pad(&st, FP, el);
			} else if (strncmp(s, "FS",2) == 0 ) {
				pcb_trace("Found free String\n");
				autotrax_parse_text(&st, FP, el);
			}
		}
	}
	if (netdefs) {
		pcb_hid_actionl("Netlist", "Sort", NULL);
		pcb_hid_actionl("Netlist", "Thaw", NULL);
		pcb_hid_actionl("ElementList", "Done", NULL);
	}
	fclose(FP);
	box = pcb_data_bbox(&boardSize, Ptr->Data);
	pcb_trace("Maximum X, Y dimensions (mil) of imported Protel autotrax layout: %ld, %ld\n", box->X2, box->Y2);
	Ptr->MaxWidth = box->X2;
	Ptr->MaxHeight = box->Y2;

	/* we now flip the board about the X-axis, to invert the Y coords used by autotrax */	
	pcb_flip_data(Ptr->Data, 0, 1, 0, Ptr->MaxHeight, 0);
	
	/* not sure if this is required: */
	/*pcb_layer_auto_fixup(Ptr);  this crashes things immeditely on load */

#warning TODO: free the layer hash

	return readres;
}

int io_autotrax_test_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, FILE *f)
{
	char line[1024], *s;

	while(!(feof(f))) {
		if (fgets(line, sizeof(line), f) != NULL) {
			s = line;
			while(isspace(*s)) s++; /* strip leading whitespace */
			if (strncmp(s, "PCB FILE 4", 10) == 0 || strncmp(s, "PCB FILE 5", 10) == 0)
				return 1;
			if ((*s == '\r') || (*s == '\n') || (*s == '#') || (*s == '\0'))
				/* ignore empty lines and comments */
				continue;
			return 0;
		}
	}

	return 0;
}

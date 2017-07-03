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


#define MAXREAD 255

/* remove leading whitespace */
#define ltrim(s) while(isspace(*s)) s++

/* remove trailing newline */
#define rtrim(s) \
	do { \
		char *end; \
		for(end = s + strlen(s) - 1; (end >= s) && ((*end == '\r') || (*end == '\n')); end--) \
			*end = '\0'; \
	} while(0)

/* read the next line, increasing the line number lineno */
#define fgetline(s, size, stream, lineno) \
	((lineno)++,fgets((s), (size), (stream)))

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
	pcb_layer_id_t protel_to_stackup[13];
	int lineno;
} read_state_t;

static int autotrax_parse_net(read_state_t *st, FILE *FP); /* describes netlists for the layout */

/* autotrax_free_text/component_text */
static int autotrax_parse_text(read_state_t *st, FILE *FP, pcb_element_t *el)
{
	int height_mil;
	int autotrax_layer = 0;
	char line[MAXREAD], *t;
	int success;
	int valid = 1;
	pcb_coord_t X, Y, linewidth;
	int scaling = 100;
	unsigned direction = 0; /* default is horizontal */
	pcb_flag_t Flags = pcb_flag_make(0); /* default */
	pcb_layer_id_t PCB_layer;

	if (fgetline(line, sizeof(line), FP, st->lineno) != NULL) {
		int argc;
		char **argv, *s;

		s = line;
		ltrim(s);
		rtrim(s);
		argc = qparse2(s, &argv, 0);
		if (argc > 5) {
			X = pcb_get_value_ex(argv[0], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found free text X : %ld\n", X);
			Y = pcb_get_value_ex(argv[1], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found free text Y : %ld\n", Y);
			height_mil = pcb_get_value_ex(argv[2], NULL, NULL, NULL, NULL, &success);
			valid &= success;
			scaling = (100*height_mil)/60;
			pcb_trace("Found free text height(mil) : %d, giving scaling: %d\n", height_mil, scaling);
			direction = pcb_get_value_ex(argv[3], NULL, NULL, NULL, NULL, &success);
			direction = direction%4; /* ignore mirroring */
			valid &= success;
			pcb_trace("Found free text rotation : %d\n", direction);
			linewidth = pcb_get_value_ex(argv[4], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found free text linewidth : %ld\n", linewidth);
			autotrax_layer = pcb_get_value_ex(argv[5], NULL, NULL, NULL, NULL, &success);
			valid &= (success && (autotrax_layer > 0) && (autotrax_layer < 14));
			PCB_layer = st->protel_to_stackup[(int)autotrax_layer];
			pcb_trace("Found free text layer : %d\n", PCB_layer);
			/* we ignore the user routed flag */
			qparse_free(argc, &argv);
		} else {
			pcb_message(PCB_MSG_ERROR, "Insufficient free string attribute fields, %s:%d\n", st->Filename, st->lineno);
			qparse_free(argc, &argv);
			return -1;
		}
	}

	if (! valid) {
		pcb_message(PCB_MSG_ERROR, "Failed to parse text attribute fields, %s:%d\n", st->Filename, st->lineno);
		return -1;
	}

	if (fgetline(line, sizeof(line), FP, st->lineno) == NULL) {
		pcb_message(PCB_MSG_ERROR, "Empty free string text field, %s:%d\n", st->Filename, st->lineno);
		strcpy(line, "(empty text field)");
	} /* this helps the parser fail more gracefully if excessive newlines, or empty text field */

	pcb_trace("Found text string for display : %s\n", line);
	t = line;
	ltrim(t);
	rtrim(t); /*need to remove trailing '\r' to avoid rendering oddities */

	/* # TODO - ABOUT HERE, CAN DO ROTATION/DIRECTION CONVERSION */

	if (autotrax_layer == 6 || autotrax_layer == 8) {
		Flags = pcb_flag_make(PCB_FLAG_ONSOLDER | PCB_FLAG_CLEARLINE); 
	} else if ((autotrax_layer >= 1 && autotrax_layer <= 5)
				|| autotrax_layer == 7 || autotrax_layer == 9 || autotrax_layer == 10) {
		Flags = pcb_flag_make(PCB_FLAG_CLEARLINE);
	} /* flags do not seem to be honoured */

	if (PCB_layer >= 0) {
		if (el == NULL && st != NULL) {
			pcb_text_new( &st->PCB->Data->Layer[PCB_layer], pcb_font(st->PCB, 0, 1), X, Y, direction, scaling, t, Flags);
			pcb_trace("\tnew free text on layer %d created\n", PCB_layer);
			return 1;
		} else if (el != NULL && st != NULL) {
			pcb_trace("\ttext within component not supported\n");
			/* this may change with subcircuits */
			return 1;
		} else if (strlen(t) == 0){
			pcb_message(PCB_MSG_ERROR, "Empty free string not placed on layout, %s:%d\n", st->Filename, st->lineno);
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
	pcb_layer_id_t PCB_layer;
	int autotrax_layer = 0;
	int success;
	int valid = 1;

	Clearance = Thickness = PCB_MIL_TO_COORD(10); /* start with sane default of ten mil */

	if (fgetline(line, sizeof(line), FP, st->lineno) != NULL) {
		int argc;
		char **argv, *s;

		s = line;
		ltrim(s);
		rtrim(s);
		argc = qparse2(s, &argv, 0);
		if (argc > 5) {
			X1 = pcb_get_value_ex(argv[0], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found track X1 : %ld\n", X1);
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
			autotrax_layer = pcb_get_value_ex(argv[5], NULL, NULL, NULL, NULL, &success);
			PCB_layer = st->protel_to_stackup[(int)autotrax_layer];
			valid &= (success && (autotrax_layer > 0) && (autotrax_layer < 14));
			pcb_trace("Found track layer : %d\n", PCB_layer);
			/* we ignore the user routed flag */
			qparse_free(argc, &argv);
		} else {
			pcb_message(PCB_MSG_ERROR, "Insufficient track attribute fields, %s:%d\n", st->Filename, st->lineno);
			qparse_free(argc, &argv);
			return -1;
		}
	}

	if (! valid) {
		pcb_message(PCB_MSG_ERROR, "Failed to parse track attribute fields, %s:%d\n", st->Filename, st->lineno);
		return -1;
	}

	if (PCB_layer >= 0) {
		if (el == NULL && st != NULL) {
			pcb_line_new( &st->PCB->Data->Layer[PCB_layer], X1, Y1, X2, Y2,
				Thickness, Clearance, Flags);
			pcb_trace("\tnew free line on layer %d created\n", PCB_layer);
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
	int autotrax_layer = 0;

	pcb_coord_t centreX, centreY, width, height, Thickness, Clearance, radius;
	pcb_angle_t start_angle = 0.0;
	pcb_angle_t delta = 360.0;

	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	pcb_layer_id_t PCB_layer;

	Clearance = Thickness = PCB_MIL_TO_COORD(10); /* start with sane default of ten mil */

	if (fgetline(line, sizeof(line), FP, st->lineno) != NULL) {
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
			autotrax_layer = pcb_get_value_ex(argv[5], NULL, NULL, NULL, NULL, &success);
			PCB_layer = st->protel_to_stackup[(int)autotrax_layer];
			valid &= (success && (autotrax_layer > 0) && (autotrax_layer < 14));
			pcb_trace("Found arc layer : %d\n", PCB_layer);
			/* we ignore the user routed flag */
			qparse_free(argc, &argv);
		} else { 
			qparse_free(argc, &argv);
			pcb_message(PCB_MSG_ERROR, "Insufficient arc attribute fields, %s:%d\n", st->Filename, st->lineno);
			return -1;
		}
	}

	if (! valid) {
		pcb_message(PCB_MSG_ERROR, "Unable to parse arc attribute fields, %s:%d\n", st->Filename, st->lineno);
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
		start_angle = 90.0;
		delta = 90.0;
		pcb_arc_new( &st->PCB->Data->Layer[PCB_layer], centreX, centreY, width, height, start_angle, delta, Thickness, Clearance, Flags);
		start_angle = 270.0;
	} else if (segments == 5) { /* RU + LL quadrants */
		start_angle = 0.0;
		delta = 90.0;
		pcb_arc_new( &st->PCB->Data->Layer[PCB_layer], centreX, centreY, width, height, start_angle, delta, Thickness, Clearance, Flags);
		start_angle = 180.0;
	} else if (segments >= 15) { /* whole circle */
		start_angle = 0.0;
		delta = 360.0;
	} else if (segments == 1) { /* RU quadrant */
		start_angle = 180.0;
		delta = 90.0;
	} else if (segments == 2) { /* LU quadrant */
		start_angle = 270.0;
		delta = 90.0;
	} else if (segments == 4) { /* LL quadrant */
		start_angle = 0.0;
		delta = 90.0;
	} else if (segments == 8) { /* RL quadrant */
		start_angle = 90.0;
		delta = 90.0;
	} else if (segments == 3) { /* Upper half */
		start_angle = 180.0;
		delta = -180.0; /* this seems to fix IC notches */
	} else if (segments == 6) { /* Left half */
		start_angle = 270.0;
		delta = 180.0;
	} else if (segments == 12) { /* Lower half */
		start_angle = 0.0;
		delta = -180.0;  /* this seems to fix IC notches */
	} else if (segments == 9) { /* Right half */
		start_angle = 90.0;
		delta = 180.0;
	} else if (segments == 14) { /* not RUQ */
		start_angle = -90.0;
		delta = 270.0;
	} else if (segments == 13) { /* not LUQ */
		start_angle = 0.0;
		delta = 270.0;
	} else if (segments == 11) { /* not LLQ */
		start_angle = 90.0;
		delta = 270.0;
	} else if (segments == 7) { /* not RLQ */
		start_angle = 180.0;
		delta = 270.0;
	}  

	if (PCB_layer >= 0) {
		if (el == NULL && st != NULL) {
			pcb_arc_new( &st->PCB->Data->Layer[PCB_layer], centreX, centreY, width, height, start_angle, delta, Thickness, Clearance, Flags);
			pcb_trace("\tnew free arc on layer %d created\n", PCB_layer);
			return 1;
		} else if (el != NULL && st != NULL) {
			pcb_element_arc_new(el, centreX, centreY, width, height, start_angle, delta, Thickness);
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

	if (fgetline(line, sizeof(line), FP, st->lineno) != NULL) {
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
			pcb_message(PCB_MSG_ERROR, "Insufficient via attribute fields, %s:%d\n", st->Filename, st->lineno);
			return -1;
		}
	}

	if (! valid) {
		pcb_message(PCB_MSG_ERROR, "Unable to parse via attribute fields, %s:%d\n", st->Filename, st->lineno);
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
x y X_size Y_size shape holesize pwr/gnd layer
padname
may need to think about hybrid outputs, like pad + hole, to match possible features in protel autotrax
*/
static int autotrax_parse_pad(read_state_t *st, FILE *FP, pcb_element_t *el)
{

	char line[MAXREAD], *s;
	int Connects = 0;
	int Shape = 0;
	int autotrax_layer = 0;
	pcb_layer_id_t PCB_layer;

	int valid = 1;
	int success;

	pcb_coord_t X, Y, X_size, Y_size, Thickness, Clearance, Mask, Drill; 
	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */

	Clearance = Mask = Thickness = PCB_MIL_TO_COORD(10); /* start with sane default of ten mil */

	Drill = PCB_MM_TO_COORD(0.300); /* start with something sane */

	if (fgetline(line, sizeof(line), FP, st->lineno) != NULL) {
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
			X_size = pcb_get_value_ex(argv[2], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found pad X_size : %ld\n", X_size);
			Y_size = pcb_get_value_ex(argv[3], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found pad Y_size : %ld\n", Y_size);
			Shape = pcb_get_value_ex(argv[4], NULL, NULL, NULL, NULL, &success);
			valid &= success;
			pcb_trace("Found pad Shape : %ld\n", Shape);
			Drill = pcb_get_value_ex(argv[5], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found pad Drill : %ld\n", Drill);
			Connects = pcb_get_value_ex(argv[6], NULL, NULL, NULL, NULL, &success);
			valid &= success; /* which specifies GND or Power connection for pin/pad/via */
			pcb_trace("Found pad Connects PWR/GND : %ld\n", Connects);
			autotrax_layer = pcb_get_value_ex(argv[7], NULL, NULL, NULL, NULL, &success);
			PCB_layer = st->protel_to_stackup[(int)autotrax_layer];
			valid &= (success && (autotrax_layer > 0) && (autotrax_layer < 14));
			pcb_trace("Found Autotrax and pcb-rnd pad layer : %d, %d\n", autotrax_layer, PCB_layer);
			qparse_free(argc, &argv);
		} else {
			qparse_free(argc, &argv);
			pcb_message(PCB_MSG_ERROR, "Insufficient pad attribute fields, %s:%d\n", st->Filename, st->lineno);
			return -1;
		}
	}

	if (! valid) {
		pcb_message(PCB_MSG_ERROR, "Insufficient pad attribute fields, %s:%d\n", st->Filename, st->lineno);
		return -1;
	}

/* now find name as string on next line and copy it */

	if (fgetline(line, sizeof(line), FP, st->lineno) == NULL) {
		pcb_message(PCB_MSG_ERROR, "Error parsing pad text field line, %s:%d\n", st->Filename, st->lineno);
		return -1;
	}
	s = line;
	rtrim(s); /* avoid rendering oddities on layout, and netlist matching confusion */
	pcb_trace("Found free pad name : %s\n", line);

	/* these features can have connections to GND and PWR	
	   planes specified in protel autotrax (seems rare though)
	   so we warn the user is this is the case */ 
	switch (Connects) {
		case 1:	pcb_message(PCB_MSG_ERROR, "pin clears PWR/GND, %s:%d.\n", st->Filename, st->lineno);
			break;
		case 2: pcb_message(PCB_MSG_ERROR, "pin requires relief to GND plane, %s:%d.\n", st->Filename, st->lineno);
			break;
		case 4: pcb_message(PCB_MSG_ERROR, "pin requires relief to PWR plane, %s:%d.\n", st->Filename, st->lineno);
			break;
		case 3: pcb_message(PCB_MSG_ERROR, "pin should connect to PWR plane, %s:%d.\n", st->Filename, st->lineno);
			break;
		case 5: pcb_message(PCB_MSG_ERROR, "pin should connect to GND plane, %s:%d.\n", st->Filename, st->lineno);
			break;
	}

	Thickness = MIN(X_size, Y_size);

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
		return 0;
	} else if ((Shape == 5 || Shape == 6) && el != NULL) {
		pcb_trace("\tnew element pad not created; Cross Hair/Moiro targets not supported\n");
		return 0;
	} else if (st != NULL && el == NULL) { /* pad not within element, i.e. a free pad/pin/via */
		if (Shape == 3) {
			Flags = pcb_flag_make(PCB_FLAG_OCTAGON);
		} else if (Shape == 2 || Shape == 4) {	
			Flags = pcb_flag_make(PCB_FLAG_SQUARE);
		}
		/* should this in fact be an SMD pad, +/- a hole in it ? */
		pcb_via_new( st->PCB->Data, X, Y, Thickness, Clearance, Mask, Drill, line, Flags);
		pcb_trace("\tnew free via created; need to check connects\n");
		return 1;
	} else if (st != NULL && el != NULL) { /* pad within element */

		/* first we sort out pad shapes, and layer flags */
		if ((Shape == 2  || Shape == 4)  && autotrax_layer == 6) {
			/* square (2) or rounded rect (4) on top layer */ 
			Flags = pcb_flag_make(PCB_FLAG_SQUARE | PCB_FLAG_ONSOLDER);
			/* actually a rectangle, but... */
		} else if ((Shape == 2  || Shape == 4)) { /* bottom layer */ 
			Flags = pcb_flag_make(PCB_FLAG_SQUARE);
			/*actually a rectangle, but... */
		} else if (Shape == 3 && autotrax_layer == 1) {/* top layer*/ 
			Flags = pcb_flag_make(PCB_FLAG_OCTAGON);
		} else if (Shape == 3 && autotrax_layer == 6) {  /*bottom layer */
			Flags = pcb_flag_make(PCB_FLAG_OCTAGON | PCB_FLAG_ONSOLDER); 
		}
/*Pads
FP
x y xsize ysize shape holesize pwr/gnd layer
padname
*/
		if (Drill == 0) {/* SMD */
			pcb_element_pad_new_rect(el, X + X_size/2, Y + Y_size/2,
						X - X_size/2, Y - Y_size/2,
						Clearance, Clearance, line, line, Flags);
			return 1;
		} else { /* not SMD */
			pcb_element_pin_new(el, X, Y , Thickness, Clearance, Clearance,  
				Drill, line, line, Flags);
			return 1;
		}
	}
	pcb_message(PCB_MSG_ERROR, "Failed to parse new pad, %s:%d\n", st->Filename, st->lineno);
	return -1;
}

/* protel autorax free fill (rectangular pour) parser - the closest thing protel autotrax has to a polygon */
static int autotrax_parse_fill(read_state_t *st, FILE *FP, pcb_element_t *el)
{
	int success;
	int valid = 1;
	int autotrax_layer = 0;
	char line[MAXREAD];
	pcb_polygon_t *polygon = NULL;
	pcb_flag_t flags = pcb_flag_make(PCB_FLAG_CLEARPOLY);
	pcb_coord_t X1, Y1, X2, Y2, Clearance;
	pcb_layer_id_t PCB_layer;

	Clearance = PCB_MIL_TO_COORD(10);

	if (fgetline(line, sizeof(line), FP, st->lineno) != NULL) {
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
			autotrax_layer = pcb_get_value_ex(argv[4], NULL, NULL, NULL, NULL, &success);
			PCB_layer = st->protel_to_stackup[(int)autotrax_layer];
			valid &= (success && (autotrax_layer > 0) && (autotrax_layer < 14));
			pcb_trace("Found fill layer : %d\n", PCB_layer);
			qparse_free(argc, &argv);
		} else {
			qparse_free(argc, &argv);
			pcb_message(PCB_MSG_ERROR, "Insufficient fill attribute fields, %s:%d\n", st->Filename, st->lineno);
			return -1;
		}
	}

	if (! valid) {
		pcb_message(PCB_MSG_ERROR, "Fill attribute fields unable to be parsed, %s:%d\n", st->Filename, st->lineno);
		return -1;
	}

	if (PCB_layer >= 0 && el == NULL) {
		polygon = pcb_poly_new(&st->PCB->Data->Layer[PCB_layer], flags);
	} else if (PCB_layer < 0 && el == NULL){
		pcb_message(PCB_MSG_ERROR, "Invalid free fill layer found, %s:%d\n", st->Filename, st->lineno);
	}

	if (polygon != NULL && el == NULL && st != NULL) { /* a free fill, not in an element */ 
		pcb_poly_point_new(polygon, X1, Y1);
		pcb_poly_point_new(polygon, X2, Y1);
		pcb_poly_point_new(polygon, X2, Y2);
		pcb_poly_point_new(polygon, X1, Y2);
		pcb_add_polygon_on_layer(&st->PCB->Data->Layer[PCB_layer], polygon);
		pcb_poly_init_clip(st->PCB->Data, &st->PCB->Data->Layer[PCB_layer], polygon);
		return 1;
	} else if (polygon == NULL && el != NULL && st != NULL && PCB_layer == 1 ) { /* in an element, top layer copper */
		flags = pcb_flag_make(0);
		pcb_element_pad_new_rect(el, X1, Y1, X2, Y2, Clearance, Clearance, "", "", flags);
		return 1;
	} else if (polygon == NULL && el != NULL && st != NULL && PCB_layer == 6 ) { /* in an element, bottom layer copper */
		flags = pcb_flag_make(0);
		pcb_element_pad_new_rect(el, X1, Y1, X2, Y2, Clearance, Clearance, "", "", flags);				
		return 1;
	}
	return -1;
}

static pcb_layer_id_t autotrax_reg_layer(read_state_t *st, const char *autotrax_layer, unsigned int mask)
{
	pcb_layer_id_t id;
	if (pcb_layer_list(mask, &id, 1) != 1) {
		pcb_layergrp_id_t gid;
		pcb_layergrp_list(PCB, mask, &gid, 1);
		id = pcb_layer_create(gid, autotrax_layer);
	}
	return id;
}

/* create a set of default layers, since protel has a static stackup */
static int autotrax_create_layers(read_state_t *st)
{
	
	pcb_layergrp_t *g;
	pcb_layer_id_t id;

	pcb_layer_group_setup_default(&st->PCB->LayerGroups);

	st->protel_to_stackup[7] = autotrax_reg_layer(st, "top silk", PCB_LYT_SILK | PCB_LYT_TOP);
	st->protel_to_stackup[8] = autotrax_reg_layer(st, "bottom silk", PCB_LYT_SILK | PCB_LYT_BOTTOM);

	st->protel_to_stackup[1] = autotrax_reg_layer(st, "top copper", PCB_LYT_COPPER | PCB_LYT_TOP);
	st->protel_to_stackup[6] = autotrax_reg_layer(st, "bottom copper", PCB_LYT_COPPER | PCB_LYT_BOTTOM);

	if (pcb_layer_list(PCB_LYT_SILK | PCB_LYT_TOP, &id, 1) == 1) {
		pcb_layergrp_id_t gid;
		pcb_layergrp_list(PCB, PCB_LYT_SILK | PCB_LYT_TOP, &gid, 1);
		st->protel_to_stackup[12] = pcb_layer_create(gid, "Keepout");
		pcb_layergrp_list(PCB, PCB_LYT_SILK | PCB_LYT_TOP, &gid, 1);
		st->protel_to_stackup[13] = pcb_layer_create(gid, "Multi");
	} else {
		pcb_message(PCB_MSG_ERROR, "Unable to create Keepout, Multi layers in default top silk group\n");
	}

	g = pcb_get_grp_new_intern(PCB, -1);
	st->protel_to_stackup[2] = pcb_layer_create(g - PCB->LayerGroups.grp, "Mid1");

	g = pcb_get_grp_new_intern(PCB, -1);
	st->protel_to_stackup[3] = pcb_layer_create(g - PCB->LayerGroups.grp, "Mid2");

	g = pcb_get_grp_new_intern(PCB, -1);
	st->protel_to_stackup[4]  = pcb_layer_create(g - PCB->LayerGroups.grp, "Mid3");

	g = pcb_get_grp_new_intern(PCB, -1);
	st->protel_to_stackup[5]  = pcb_layer_create(g - PCB->LayerGroups.grp, "Mid4");

	g = pcb_get_grp_new_intern(PCB, -1);
	st->protel_to_stackup[9]  = pcb_layer_create(g - PCB->LayerGroups.grp, "GND");

	g = pcb_get_grp_new_intern(PCB, -1);
	st->protel_to_stackup[10]  = pcb_layer_create(g - PCB->LayerGroups.grp, "Power");

	pcb_layergrp_fix_old_outline(PCB);
	st->protel_to_stackup[11]  = autotrax_reg_layer(st, "outline", PCB_LYT_OUTLINE);

	pcb_layergrp_inhibit_dec();

	return 0;
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
	
	if (fgetline(line, sizeof(line), FP, st->lineno) != NULL) {
		s = line;
		rtrim(s);
		pcb_trace("new netlist being added: %s.\n", line);
		netname = pcb_strdup(line);
	} else {
		pcb_message(PCB_MSG_ERROR, "Empty netlist name found, %s:%d\n", st->Filename, st->lineno);
		return -1;
	}
	fgetline(line, sizeof(line), FP, st->lineno);
	s = line;
	rtrim(s);
	pcb_trace("netlist visibility flag: %s.\n", line);
	while (!feof(FP) && !endpcb && in_net) {
		fgetline(line, sizeof(line), FP, st->lineno);
		if (strncmp(line, "[", 1) == 0 ) {
			pcb_trace("Entering netlist component definition.\n");
			in_comp = 1;
			while (in_comp) {
				if (fgetline(line, sizeof(line), FP, st->lineno) == NULL) {
					pcb_message(PCB_MSG_ERROR, "Empty line in netlist COMP, %s:%d\n", st->Filename, st->lineno);
				} else {
					if (fgetline(line, sizeof(line), FP, st->lineno) == NULL) {
						pcb_message(PCB_MSG_ERROR, "Empty netlist REFDES, %s:%d\n", st->Filename, st->lineno);
					} else {
						s = line;
						rtrim(s);
						sym_flush(&sattr);
						free(sattr.refdes);
						sattr.refdes = pcb_strdup(line);
					}
					if (fgetline(line, sizeof(line), FP, st->lineno) == NULL) {
						pcb_message(PCB_MSG_ERROR, "Empty NETDEF package, %s:%d\n", st->Filename, st->lineno);
						free(sattr.footprint);
						sattr.footprint = pcb_strdup("unknown");
					} else {
						s = line;
						rtrim(s);
						free(sattr.footprint);
						sattr.footprint = pcb_strdup(line);
					}
					if (fgetline(line, sizeof(line), FP, st->lineno) == NULL) {
						free(sattr.value);
						sattr.value = pcb_strdup("value");
					} else {
						s = line;
						rtrim(s);
						free(sattr.value);
						sattr.value = pcb_strdup(line);
					}
				}
				while (fgetline(line, sizeof(line), FP, st->lineno) == NULL) {
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
				while (fgetline(line, sizeof(line), FP, st->lineno) == NULL) {
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
			pcb_message(PCB_MSG_ERROR, "End of protel Autotrax file found in netlist section?!, %s:%d\n", st->Filename, st->lineno);
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
	int refdes_scaling = 100;
	pcb_coord_t module_X, module_Y;
	unsigned direction = 0; /* default is horizontal */
	char module_name[MAXREAD], module_refdes[MAXREAD], module_value[MAXREAD];
	pcb_element_t *new_module;

	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	pcb_flag_t text_flags = pcb_flag_make(0); /* start with something bland here */

	char *s, line[MAXREAD];
	int nonempty = 0;
	int length = 0;

	if (fgetline(module_refdes, sizeof(module_refdes), FP, st->lineno) == NULL) {
		strcpy(module_refdes, "unknown");
	}
	s = module_refdes;
	rtrim(s); /* avoid rendering oddities on layout */
	pcb_trace("Found component refdes : %s\n", module_refdes);
	if (fgetline(module_name, sizeof(module_name), FP, st->lineno) == NULL) {
		strcpy(module_name, "unknown");
	}
	s = module_name;
	rtrim(s); /* avoid rendering oddities on layout */
	pcb_trace("Found component name : %s\n", module_name);
	if (fgetline(module_value, sizeof(module_value), FP, st->lineno) == NULL) {
		strcpy(module_value, "unknown");
	}
	s = module_value;
	rtrim(s); /* avoid rendering oddities on layout */
	pcb_trace("Found component description : %s\n", module_value);

/* with the header read, we now ignore the locations for the text fields in the next two lines... */
/* we also allow up to two empty lines in case of extra newlines */
	if (fgetline(line, sizeof(line), FP, st->lineno) == NULL) {
		fgetline(line, sizeof(line), FP, st->lineno);
	}
	if (fgetline(line, sizeof(line), FP, st->lineno) == NULL) {
		fgetline(line, sizeof(line), FP, st->lineno);
	}

	if (fgetline(line, sizeof(line), FP, st->lineno) != NULL) {
		int argc;
		char **argv, *s;
		s = line;
		ltrim(s);
		rtrim(s);
		argc = qparse2(s, &argv, 0);
		if (argc >= 2) {
			module_X = pcb_get_value_ex(argv[0], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found COMP module_X : %ld\n", module_X);
			module_Y = pcb_get_value_ex(argv[1], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			pcb_trace("Found COMP module_Y : %ld\n", module_Y);
			qparse_free(argc, &argv);
		} else {
			qparse_free(argc, &argv);
			pcb_message(PCB_MSG_ERROR, "Insufficient COMP attribute fields, %s:%d\n", st->Filename, st->lineno);
			return -1;
		}
	}

	if (! valid) {
		pcb_message(PCB_MSG_ERROR, "Unable to parse COMP attributes, %s:%d\n", st->Filename, st->lineno);
		return -1;
	}

	
	pcb_trace("Have new module name and location, defining module/element %s\n", module_name);
	new_module = pcb_element_new(st->PCB->Data, NULL, 
								pcb_font(st->PCB, 0, 1), Flags,
								module_name, module_refdes, module_value,
								module_X, module_Y, direction,
								refdes_scaling, text_flags, pcb_false);

	nonempty = 0;
	while (!feof(FP)) {
		if (fgetline(line, sizeof(line), FP, st->lineno) == NULL) {
			fgetline(line, sizeof(line), FP, st->lineno);
		}
		s = line;
		length = strlen(line);
		if (length >= 7) {
			if (strncmp(line, "ENDCOMP", 7) == 0 ) {
				pcb_trace("Finished parsing component\n");
				if (nonempty) { /* could try and use module empty function here */
					pcb_element_bbox(st->PCB->Data, new_module, pcb_font(PCB, 0, 1));
					return 0;
				} else {
					pcb_message(PCB_MSG_ERROR, "Empty module/COMP found, not added to layout, %s:%d\n", st->Filename, st->lineno);
					return 0;
				}
			}
		} else if (length >= 2) {
			if (strncmp(s, "CT",2) == 0 ) {
				pcb_trace("Found component track\n");
				nonempty |= autotrax_parse_track(st, FP, new_module);
			} else if (strncmp(s, "CA",2) == 0 ) {
				pcb_trace("Found component arc\n");
				nonempty |= autotrax_parse_arc(st, FP, new_module);
			} else if (strncmp(s, "CV",2) == 0 ) {
				pcb_trace("Found component via\n");
				nonempty |= autotrax_parse_via(st, FP, new_module);
			} else if (strncmp(s, "CF",2) == 0 ) {
				pcb_trace("Found component fill\n");
				nonempty |= autotrax_parse_fill(st, FP, new_module);
			} else if (strncmp(s, "CP",2) == 0 ) {
				pcb_trace("Found component pad\n");
				nonempty |= autotrax_parse_pad(st, FP, new_module);
			} else if (strncmp(s, "CS",2) == 0 ) {
				pcb_trace("Found component String\n");
				nonempty |= autotrax_parse_text(st, FP, new_module);
			}
		}
	}
	return -1; /* should not get here */ 
}

	
int io_autotrax_read_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, conf_role_t settings_dest)
{
	int readres = 0;
	pcb_box_t board_size, *box;
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
	st.lineno = 0;

	while (!feof(FP) && !finished) {
		if (fgetline(line, sizeof(line), FP, st.lineno) == NULL) {
			fgetline(line, sizeof(line), FP, st.lineno);
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
	box = pcb_data_bbox(&board_size, Ptr->Data);
	pcb_trace("Maximum X, Y dimensions (mil) of imported Protel autotrax layout: %ld, %ld\n", box->X2, box->Y2);
	Ptr->MaxWidth = box->X2;
	Ptr->MaxHeight = box->Y2;

	/* we now flip the board about the X-axis, to invert the Y coords used by autotrax */	
	pcb_flip_data(Ptr->Data, 0, 1, 0, Ptr->MaxHeight, 0);
	
	/* still not sure if this is required: */
	pcb_layer_auto_fixup(Ptr);

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

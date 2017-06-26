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
#include "../src_plugins/boardflip/boardflip.h"

typedef struct {
	pcb_board_t *PCB;
	const char *Filename;
	conf_role_t settings_dest;
	gsxl_dom_t dom;
	htsi_t layer_protel2i; /* protel layer name-to-index hash; name is the protel layer, index is the pcb-rnd layer index */
} read_state_t;

typedef struct {
	const char *node_name;
	int (*parser)(read_state_t *st, gsxl_node_t *subtree);
} dispatch_t;

static int autotrax_error(gsxl_node_t *subtree, char *fmt, ...)
{
	gds_t str;
	va_list ap;


	gds_init(&str);
#warning TODO: include location info here:
	pcb_append_printf(&str, "io_autotrax parse error: ");

	va_start(ap, fmt);
	pcb_append_vprintf(&str, fmt, ap);
	va_end(ap);
	
	gds_append(&str, '\n');

	pcb_message(PCB_MSG_ERROR, "%s", str.array);

	gds_uninit(&str);
	return -1;
}



static int autotrax_parse_net(read_state_t *st, gsxl_node_t *subtree); /* describes netlists for the layout */

static int autotrax_get_layeridx(read_state_t *st, const int autotrax_layer);

/* a simple line retrieval routine that ideally has maxread to be 2 less than * line in size */ 
static int read_a_text_line(FILE *FP, char * line, int maxread) {
	int index = 0;
	int c;
	/* we avoid up to one '\n' if it is at the beginning of the line */
	while (!feof(FP) && ((c = fgetc(FP)) != '\n') && (index < (maxread-1))) {
		if ((c == '\t' || c == ' ') && index == 0) {
			/* strip leading tabs, spaces */ 
		} else if ((c == '\t' || c == ' ' ) && line[index-1] != ' ') {
			line[index] = ' '; /* avoid repeated tabs or spaces */
			index++;
		} else if (c != 0x0D && c != ' ' && c != '\t') {
			/*carriage return found in some pathological old files prior to newline*/
			line[index] = c; /* let all other stuff through for now */
			index++;
		}
	}
	line[index] = '\0';
	/*printf("Line returned:\n%s\n", line);*/
	return index;
}

static int autotrax_bring_back_eight_track(FILE * FP, double * results, int numresults) {
	int maxread = 42; /*sufficient for all normal protel autotrax line reads, plus a safety margin */
	char line[43];
	char *current, *next;
	int maxresult = 8; /* no more than 8 fields are present in protel autotrax subtypes */
	int iter = 0;
	int strlen = 0;
	if (numresults > maxresult) {
		pcb_trace("Too many result fields requested from line parser.\n");
		return -1;
	}
	strlen = read_a_text_line(FP, line, maxread);
	if (strlen == 0) {
		strlen = read_a_text_line(FP, line, maxread);
	}
	if (strlen == 0) {
		pcb_trace("Too many newlines in file. Unable to parse properly.\n");
		return -1;
	}
	pcb_trace("segment: %s\n", line);
	current = line;
	/* no leading or double spaces to worry about; taken care of by line retrieval routine */ 
	for (; *current != '\0' && iter < numresults; iter++, next++) {
		next = current + 1;
		while (*next != ' ' && *next != '\0') {
			next ++;
		}
		results[iter] = strtod(current, &next);
		current = next;
	}
	return iter;
}

/* autotrax_free_text/component_text */
static int autotrax_parse_text(read_state_t *st, FILE *FP, pcb_element_t *el)
{
	int textargcount = 6;
	double results[6];
	int maxtext = 32;

	int strlen = 0;
	int heightMil;

	char line[45];
	pcb_coord_t X, Y, linewidth;
	int scaling = 100;
	unsigned direction = 0; /* default is horizontal */
	pcb_flag_t Flags = pcb_flag_make(0); /* default */
	int PCBLayer = 0; /* sane default value */

	if (!autotrax_bring_back_eight_track(FP, results, textargcount)) {
		pcb_trace("error parsing protel text field attributes\n");
		return -1;
	}

	X = PCB_MIL_TO_COORD(results[0]);
	pcb_trace("Found free text X : %ld\n", X);
	Y = PCB_MIL_TO_COORD(results[1]);
	pcb_trace("Found free text Y : %ld\n", Y);
	heightMil = (int)results[2];
	scaling = (100*heightMil)/60;
	pcb_trace("Found free text height(mil) : %d, giving scaling: %d\n", heightMil, scaling);
	direction = (int)results[3];
	pcb_trace("Found free text rotation : %d\n", direction);
	linewidth = PCB_MIL_TO_COORD(results[4]);
	pcb_trace("Found free text linewidth : %ld\n", linewidth);
	PCBLayer = autotrax_get_layeridx(st, (int)results[5]); 
	pcb_trace("Found free text layer : %d\n", PCBLayer);
	/* ignore user routed flag */

	if ((strlen = read_a_text_line(FP, line, maxtext)) == 0) {
		pcb_trace("error parsing free string text line; empty field\n");
		strcpy(line, "(empty text field)");
	} /* this helps the parser fail more gracefully if excessive newlines, or empty text field */

	pcb_trace("Found text string for display : %s\n", line);

	/* ABOUT HERE, CAN DO ROTATION/DIRECTION CONVERSION */

	if (PCBLayer >= 0) {
		if (el == NULL && st != NULL && strlen != 0) {
			pcb_text_new( &st->PCB->Data->Layer[PCBLayer], pcb_font(st->PCB, 0, 1), X, Y, direction, scaling, line, Flags);
			pcb_trace("\tnew free text on layer %d created\n", PCBLayer);
			return 1;
		} else if (el != NULL && st != NULL && strlen != 0) {
			pcb_trace("\ttext within component not supported\n");
			/* this may change with subcircuits */
			return 1;
		} else if (strlen == 0) {
			pcb_trace("\tempty free/component text field ignored\n");
			return 0;
		}
	}
	return -1;
}

/* autotrax_pcb/free_track/component_track */
static int autotrax_parse_track(read_state_t *st, FILE *FP, pcb_element_t *el)
{
	int trackargcount = 7;
	double results[7];

	pcb_coord_t X1, Y1, X2, Y2, Thickness, Clearance;
	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	int PCBLayer = 0; /* sane default value */

	Clearance = Thickness = PCB_MIL_TO_COORD(10); /* start with sane default of ten mil */

	if (!autotrax_bring_back_eight_track(FP, results, trackargcount)) {
		printf("error parsing track text\n");
		return -1;
	}
	X1 = PCB_MIL_TO_COORD(results[0]);
	pcb_trace("Found track X1 : %ml\n", X1);
	Y1 = PCB_MIL_TO_COORD(results[1]);
	pcb_trace("Found track Y1 : %ml\n", Y1);
	X2 = PCB_MIL_TO_COORD(results[2]);
	pcb_trace("Found track X2 : %ml\n", X2);
	Y2 = PCB_MIL_TO_COORD(results[3]);
	pcb_trace("Found track Y2 : %ml\n", Y2);
	Thickness = PCB_MIL_TO_COORD(results[4]);
	pcb_trace("Found track width : %ml\n", Thickness);
	PCBLayer = autotrax_get_layeridx(st, (int)results[5]); 
	pcb_trace("Found track layer : %d\n", PCBLayer);
	/* ignore user routed flag in results[6]*/

	if (PCBLayer >= 0) {
		if (el == NULL && st != NULL) {
			pcb_line_new( &st->PCB->Data->Layer[PCBLayer], X1, Y1, X2, Y2, Thickness, Clearance, Flags);
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
	int arcargcount = 6;
	double results[6];
	int segments = 15; /* full circle by default */ 

	pcb_coord_t centreX, centreY, width, height, Thickness, Clearance, radius; /* radius may in fact be diameter */
	pcb_angle_t startAngle = 0.0;
	pcb_angle_t delta = 360.0;

	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	int PCBLayer = 0; /* sane default value */

	Clearance = Thickness = PCB_MIL_TO_COORD(10); /* start with sane default of ten mil */

	if (!autotrax_bring_back_eight_track(FP, results, arcargcount)) {
		pcb_trace("error parsing arc text attribute line\n");
		return -1;
	}
	centreX = PCB_MIL_TO_COORD(results[0]);
	pcb_trace("Found arc centre X : %ml\n", centreX);
	centreY = PCB_MIL_TO_COORD(results[1]);
	pcb_trace("Found arc Y : %ml\n", centreY);
	radius = PCB_MIL_TO_COORD(results[2]);
	pcb_trace("Found arc radius : %ml\n", radius);
	segments = (int)results[3];
	pcb_trace("Found arc segments : %d\n", segments);
	Thickness = PCB_MIL_TO_COORD(results[4]);
	pcb_trace("Found arc width : %ml\n", Thickness);
	PCBLayer = autotrax_get_layeridx(st, (int)results[5]); 
	pcb_trace("Found arc layer : %d\n", PCBLayer);

	width = height = radius;

/* we now need to decrypt the segments value. Some definitions will represent two arcs.
Bit 0 : RU quadrant
Bit 1 : LU quadrant
Bit 2 : LL quadrant
Bit 3 : LR quadrant

TODO: This needs further testing to ensure the document used reflects actual outputs from protel autotrax
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

/* autotrax_pcb/via */
static int autotrax_parse_via(read_state_t *st, FILE *FP, pcb_element_t *el)
{
	int viaargcount = 4;
	double results[4];
	char * name;

	pcb_coord_t X, Y, Thickness, Clearance, Mask, Drill; /* not sure what to do with mask */
	pcb_flag_t Flags = pcb_flag_make(0);

	Clearance = Mask = Thickness = PCB_MIL_TO_COORD(10); /* start with sane default of ten mil */

	Drill = PCB_MM_TO_COORD(0.300); /* start with something sane */

	if (!autotrax_bring_back_eight_track(FP, results, viaargcount)) {
		pcb_trace("error parsing via attribute text\n");
		return -1;
	}

	name = pcb_strdup("unnamed");

	X = PCB_MIL_TO_COORD(results[0]);
	pcb_trace("Found free via X : %ld\n", X);
	Y = PCB_MIL_TO_COORD(results[1]);
	pcb_trace("Found free via Y : %ld\n", Y);
	Thickness = PCB_MIL_TO_COORD(results[2]);
	pcb_trace("Found free via diameter : %ld\n", Thickness);
	Drill = PCB_MIL_TO_COORD(results[3]);
	pcb_trace("Found free track drill : %ld\n", Drill);

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

/* autotrax_pcb free pad*/
/* FP or CP
x y xsize ysize shape holesize pwr/gnd layer
padname
*/
static int autotrax_parse_pad(read_state_t *st, FILE *FP, pcb_element_t *el)
{
	int padargcount = 7;
	double results[7];
	int maxtext = 32;

	char padname[32];
	int Connects = 0;
	int Shape = 0;
	int AutotraxLayer = 0;
	int PCBLayer = 0; /* sane default value */

	pcb_coord_t X, Y, Xsize, Ysize, Thickness, Clearance, Mask, Drill; 
	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */

	Clearance = Mask = Thickness = PCB_MIL_TO_COORD(10); /* start with sane default of ten mil */

	Drill = PCB_MM_TO_COORD(0.300); /* start with something sane */

	if (!autotrax_bring_back_eight_track(FP, results, padargcount)) {
		pcb_trace("error parsing pad attribute text\n");
		return -1;
	}
	X = PCB_MIL_TO_COORD(results[0]);
	pcb_trace("Found free pad X : %ld\n", X);
	Y = PCB_MIL_TO_COORD(results[1]);
	pcb_trace("Found free pad Y : %ld\n", Y);
	Xsize = PCB_MIL_TO_COORD(results[2]);
	pcb_trace("Found free pad Xsize : %ld\n", Xsize);
	Ysize = PCB_MIL_TO_COORD(results[3]);
	pcb_trace("Found free pad Ysize : %ld\n", Ysize);
	Shape = (int)results[4]; 
	pcb_trace("Found free pad shape : %d\n", Shape);
	Drill = PCB_MIL_TO_COORD(results[5]);
	pcb_trace("Found free pad drill : %ld\n", Drill);
	Connects = (int)results[5];  /* which specifies GND or Power connection for pin/pad/via */
	AutotraxLayer = (int)results[6];
	PCBLayer = autotrax_get_layeridx(st, (int)results[6]); 
	pcb_trace("Found free pad Layer : %d\n", PCBLayer);

/* now find name as string on next line and copy it */

	if (read_a_text_line(FP, padname, maxtext) == 0) {
		pcb_trace("error parsing pad free string text line; empty\n");
		return -1;
	}
	pcb_trace("Found free pad name : %s\n", padname);

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
	The onsolder flags are reduntant for now with vias.

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
		pcb_via_new( st->PCB->Data, X, Y, Thickness, Clearance, Mask, Drill, padname, Flags);
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
/* # TODO  not yet processing SMD pads */
		if (Shape == 2 && Drill == 0) {/* is probably SMD */
/*			pcb_element_pad_new_rect(el, el->X - Xsize/2, el->Y - Ysize/2, Xsize/2 + el->X, Ysize/2 + el->Y, Clearance, 
				Clearance, padname, padname, Flags);*/
/*			pcb_element_pad_new_rect(el, X - Xsize/2, Y - Ysize/2, X + Xsize/2, Ysize/2 + Y, Clearance, 
				Clearance, padname, padname, Flags);*/
			return 1;
		} else {
/*			pcb_element_pin_new(el, X + el->X, Y + el->Y, Thickness, Clearance, Clearance,  
				Drill, padname, padname, Flags);*/
			pcb_element_pin_new(el, X, Y , Thickness, Clearance, Clearance,  
				Drill, padname, padname, Flags);
			return 1;
		}
		pcb_trace("\tnew component pad/hole created; need to check connects\n");
		return 1;
	}
	pcb_trace("\tfailed to parse new pad/hole CP/FP\n");	
	return -1;
}


static int autotrax_parse_fill(read_state_t *st, FILE *FP, pcb_element_t *el)
{
	int fillargcount = 5;
	double results[5];

	pcb_polygon_t *polygon = NULL;
	pcb_flag_t flags = pcb_flag_make(PCB_FLAG_CLEARPOLY);
	pcb_coord_t X1, Y1, X2, Y2, Clearance;
	int PCBLayer = 0;

	Clearance = PCB_MIL_TO_COORD(10);

	if (!autotrax_bring_back_eight_track(FP, results, fillargcount)) {
		pcb_trace("error parsing fill attribute text\n");
		return -1;
	}

	X1 = PCB_MIL_TO_COORD(results[0]);
	pcb_trace("Found fill X1 : %ld\n", X1);
	Y1 = PCB_MIL_TO_COORD(results[1]);
	pcb_trace("Found fill Y1 : %ld\n", Y1);
	X2 = PCB_MIL_TO_COORD(results[2]);
	pcb_trace("Found fill X2 : %ld\n", X2);
	Y2 = PCB_MIL_TO_COORD(results[3]);
	pcb_trace("Found fill Y2 : %ld\n", Y2);
	PCBLayer = autotrax_get_layeridx(st, (int)results[4]);
	pcb_trace("Found fill layer : %d\n", PCBLayer);


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
	/*pcb_layer_id_t stackup[14];*/
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

/* autotrax_pcb  parse (net  ) ;   used for net descriptions for the entire layout */
static int autotrax_parse_net(read_state_t *st, gsxl_node_t *subtree)
{
		if (subtree != NULL && subtree->str != NULL) {
			pcb_trace("net number: '%s'\n", subtree->str);
		} else {
			pcb_trace("missing net number in net descriptors");
			return autotrax_error(subtree, "missing net number in net descriptors.");
		}
		if (subtree->next != NULL && subtree->next->str != NULL) {
			pcb_trace("\tcorresponding net label: '%s'\n", (subtree->next->str));
		} else {
			pcb_trace("missing net label in net descriptors");
			return autotrax_error(subtree, "missing net label in net descriptors.");
		}
		return 0;
}

static int autotrax_parse_component(read_state_t *st, FILE *FP)
{
	int coordresultcount = 2;
	double results[2];
	int refdesScaling = 100;
	pcb_coord_t moduleX, moduleY, Thickness;
	unsigned direction = 0; /* default is horizontal */
	char moduleName[33], moduleRefdes[33], moduleValue[33];
	pcb_element_t *newModule;

	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	pcb_flag_t TextFlags = pcb_flag_make(0); /* start with something bland here */

	int length = 0;
	int maxtext = 33;
	char *s, line[33];
	int nonempty = 0;

	length = read_a_text_line(FP, moduleRefdes, maxtext); /* this breaks mildly with excess newlines */
	if (length == 0) {
		strcpy(moduleRefdes, "unknown");
	}
	pcb_trace("Found component refdes : %s\n", moduleRefdes);
	length = read_a_text_line(FP, moduleName, maxtext);
	if (length == 0) {
		strcpy(moduleName, "unknown");
	}
	pcb_trace("ModuleRefdes: %s\n", moduleRefdes);
	pcb_trace("Found component name : %s\n", moduleName);
	length = read_a_text_line(FP, moduleValue, maxtext);
		if (length == 0) {
		strcpy(moduleValue, "unknown");
	}
	pcb_trace("Found component description : %s\n", moduleValue);

/* with the header read, we now ignore the locations for the text fields in the next two lines... */
/* we also allow up to two empty lines in case of extra newlines */
	length = read_a_text_line(FP, line, maxtext);
	if (length == 0) {
		read_a_text_line(FP, line, maxtext);
	}
	length = read_a_text_line(FP, line, maxtext);
	if (length == 0) {
		read_a_text_line(FP, line, maxtext);
	}

	if (!autotrax_bring_back_eight_track(FP, results, coordresultcount)) {
		pcb_trace("error parsing component coordinates\n");
		return -1;
	}
	moduleX = PCB_MIL_TO_COORD(results[0]);
	pcb_trace("Found moduleX : %ld\n", moduleX);
	moduleY = PCB_MIL_TO_COORD(results[1]);
	pcb_trace("Found moduleY : %ld\n", moduleY);
	
	pcb_trace("Have new module name and location, defining module/element %s\n", moduleName);
	newModule = pcb_element_new(st->PCB->Data, NULL, 
								pcb_font(st->PCB, 0, 1), Flags,
								moduleName, moduleRefdes, moduleValue,
								moduleX, moduleY, direction,
								refdesScaling, TextFlags, pcb_false); /*pcb_flag_t TextFlags, pcb_bool uniqueName) */

	while (!feof(FP)) {
		length = read_a_text_line(FP, line, maxtext);
		if (length == 0) {
			length = read_a_text_line(FP, line, maxtext);
		}
		nonempty = 0;
		s = line;
		if (length >= 7) {
			if (strncmp(line, "ENDCOMP", 7) == 0 ) {
				printf("Finished parsing component\n");
				if (!nonempty) { /* should try and use module empty function here */
					Thickness = PCB_MM_TO_COORD(0.200);
					pcb_element_line_new(newModule, moduleX, moduleY, moduleX+1, moduleY+1, Thickness);
					pcb_printf("\tEmpty Module!! 1nm line created at module centroid.\n");
				}
				pcb_element_bbox(st->PCB->Data, newModule, pcb_font(PCB, 0, 1));
				return 0;
			}
		} else if (length >= 2) {
			if (strncmp(s, "CT",2) == 0 ) {
				printf("Found component track\n");
				nonempty |= autotrax_parse_track(st, FP, newModule);
			} else if (strncmp(s, "CA",2) == 0 ) {
				printf("Found component arc\n");
				nonempty |= autotrax_parse_arc(st, FP, newModule);
			} else if (strncmp(s, "CV",2) == 0 ) {
				printf("Found component via\n");
				nonempty |= autotrax_parse_via(st, FP, newModule);
			} else if (strncmp(s, "CF",2) == 0 ) {
				printf("Found component fill\n");
				nonempty |= autotrax_parse_fill(st, FP, newModule);
			} else if (strncmp(s, "CP",2) == 0 ) {
				printf("Found component pad\n");
				nonempty |= autotrax_parse_pad(st, FP, newModule);
			} else if (strncmp(s, "CS",2) == 0 ) {
				printf("Found component String\n");
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
	int maxtext = 1024;
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
		length = read_a_text_line(FP, line, maxtext);
		if (length == 0) { /* deal with up to one additional newline per line*/
			length = read_a_text_line(FP, line, maxtext);
		}
		s = line;
		if (length >= 10) {
			if (strncmp(line, "PCB FILE 4", 10) == 0 ) {
				printf("Found Protel Autotrax version 4\n");
				autotrax_create_layers(&st);
			} else if (strncmp(line, "PCB FILE 5", 10) == 0 ) {
				printf("Found Protel Easytrax version 5\n");
				autotrax_create_layers(&st);
			}
		} else if (length >= 6) {
			if (strncmp(s, "ENDPCB",6) == 0 ) {
				printf("Found end of file\n");
				finished = 1;
			} else if (strncmp(s, "NETDEF",6) == 0 ) {
				printf("Found net definition\n");
			}
		} else if (length >= 4) {
			if (strncmp(line, "COMP",4) == 0 ) {
				printf("Found component\n");
				autotrax_parse_component(&st, FP);
			}
		} else if (length >= 2) {
			if (strncmp(s, "FT",2) == 0 ) {
				printf("Found free track\n");
				autotrax_parse_track(&st, FP, el);
			} else if (strncmp(s, "FA",2) == 0 ) {
				printf("Found free arc\n");
				autotrax_parse_arc(&st, FP, el);
			} else if (strncmp(s, "FV",2) == 0 ) {
				printf("Found free via\n");
				autotrax_parse_via(&st, FP, el);
			} else if (strncmp(s, "FF",2) == 0 ) {
				printf("Found free fill\n");
				autotrax_parse_fill(&st, FP, el);
			} else if (strncmp(s, "FP",2) == 0 ) {
				printf("Found free pad\n");
				autotrax_parse_pad(&st, FP, el);
			} else if (strncmp(s, "FS",2) == 0 ) {
				printf("Found free String\n");
				autotrax_parse_text(&st, FP, el);
			}
		}
	} 
	fclose(FP);
	box = pcb_data_bbox(&boardSize, Ptr->Data);
	pcb_trace("Maximum X, Y dimensions (mil) of imported Protel autotrax layout: %ld, %ld\n", box->X2, box->Y2);
	Ptr->MaxWidth = box->X2;
	Ptr->MaxHeight = box->Y2;

	if (1) {
		pcb_flip_data(Ptr->Data, 0, 1, 0, Ptr->MaxHeight, 0);
	}

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
			if ((*s == '\r') || (*s == '\n') || (*s == '#') || (*s == '\0')) /* ignore empty lines and comments */
				continue;
			return 0;
		}
	}

	/* hit eof before seeing a valid root -> bad */
	return 0;
}

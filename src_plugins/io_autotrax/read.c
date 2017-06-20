/*
 *														COPYRIGHT
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
	htsi_t layer_k2i; /* layer name-to-index hash; name is the kicad name, index is the pcb-rnd layer index */
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

#warning TODO: do not printf here:
	pcb_message(PCB_MSG_ERROR, "%s", str.array);

	gds_uninit(&str);
	return -1;
}

/* Take each children of tree and execute them using autotrax_dispatch
   Useful for procssing nodes that may host various subtrees of different
   nodes ina  flexible way. Return non-zero if any subtree processor failed. */

/* autotrax_pcb/version */
static int autotrax_parse_version(read_state_t *st, gsxl_node_t *subtree)
{
	if (subtree->str != NULL) {
		int ver = atoi(subtree->str);
		printf("kicad version: '%s' == %d\n", subtree->str, ver);
		if (ver == 3 || ver == 4 || ver == 20170123)
			return 0;
	}
	return autotrax_error(subtree, "unexpected layout version");
}

static int autotrax_parse_net(read_state_t *st, gsxl_node_t *subtree); /* describes netlists for the layout */
static int autotrax_get_layeridx(read_state_t *st, const char *autotrax_name);

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


/* autotrax_free_text */
static int autotrax_parse_free_text(read_state_t *st, FILE *FP)
{

	int i, heightMil;

	char *end;
	double val;
	pcb_coord_t X, Y, linewidth;
	int scaling = 100;
	unsigned direction = 0; /* default is horizontal */
	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	int PCBLayer = 0; /* sane default value */

	int index = 0;
	char line[35]; /* line is 4 x 32000 + layer + 1 + 1 = 33 characters at most */
	char coord[6];

	int c;

	index =0;
	while ((!feof(FP) && (c = fgetc(FP)) != '\n' && index < 34) || (c == '\n' && index == 0) ) {
		line[index] = c;
		index ++;
	}
	if (index > 33) {
		pcb_printf("error parsing free track line; too long\n");
		return -1;
	}
	line[index] = ' ';
	index++;
	line[index] = '\0';
	printf("About to parse autotrax free text: %s\n", line);  
	index = 0;
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	index++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free text X\n");
        	return -1;
	}
	X = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free text X : %ml\n", X);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free text Y\n");
        	return -1;
	}
	Y = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free text Y : %ml\n", Y);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index ++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free text height\n");
        	return -1;
	}
	heightMil = (int)val;
	scaling = (100*heightMil)/60;
	pcb_printf("Found free text height(mil) : %d, giving scaling: %d\n", heightMil, scaling);

/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index ++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free text rotation\n");
        	return -1;
	}
	direction = (int)val;
	pcb_printf("Found free text rotation : %d\n", direction);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free text linewidth\n");
        	return -1;
	}
	linewidth = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free text linewidth : %ml\n", linewidth);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = atoi(coord);
	if (*end != 0) {
		pcb_printf("error parsing free text layer\n");
        	return -1;
	}
	PCBLayer = val; 
	pcb_printf("Found free text layer : %d\n", PCBLayer);
	/* ignore user routed flag */

	index= 0; /* reset index for following text line for display  */
	while ((!feof(FP) && (c = fgetc(FP)) != '\n' && index < 34) || (c == '\n' && index == 0) ) {
		line[index] = c;
		index ++;
	}
	if (index > 32) {
		pcb_printf("error parsing free string text line; too long\n");
		return -1;
	}
	line[index] = '\0';

	pcb_printf("Found free text string for display : %s\n", line);

	/* ABOUT HERE, CAN DO ROTATION/DIRECTION CONVERSION */

	if (line[0] != '\0') {
		pcb_text_new( &st->PCB->Data->Layer[PCBLayer], pcb_font(st->PCB, 0, 1), X, Y, direction, scaling, line, Flags);
	}
	return 0; /* create new font */
}

/* autotrax_pcb/free_track */
static int autotrax_parse_free_track(read_state_t *st, FILE *FP)
{
	int index = 0;
	char line[35]; /* line is 4 x 32000 + layer + 1 + 1 = 33 characters at most */
	char coord[6];

	int i;
	int c;
	char *end;
	double val;

	pcb_coord_t X1, Y1, X2, Y2, Thickness, Clearance; /* not sure what to do with mask */
	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	int PCBLayer = 0; /* sane default value */

	Clearance = Thickness = PCB_MIL_TO_COORD(10); /* start with sane default of ten mil */
	index = 0;

	while ((!feof(FP) && (c = fgetc(FP)) != '\n' && index < 34) || (c == '\n' && index == 0) ) {
		line[index] = c;
		index ++;
	}
	if (index > 33) {
		pcb_printf("error parsing free track line; too long\n");
		return -1;
	}
	line[index] = ' ';
	index++;
	line[index] = '\0';
	printf("About to parse autotrax free track: %s\n", line);  
	index = 0;
	while(line[index] == ' ') { /* skip white space */
		index++;
	}
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	index++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free track X1\n");
        	return -1;
	}
	X1 = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free track X1 : %ml\n", X1);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free track Y1\n");
        	return -1;
	}
	Y1 = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free track Y1 : %ml\n", Y1);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index ++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free track X2\n");
        	return -1;
	}
	X2 = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free track X2 : %ml\n", X2);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free track Y2\n");
        	return -1;
	}
	Y2 = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free track Y2 : %ml\n", Y2);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free track width\n");
        	return -1;
	}
	Thickness = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free track width : %ml\n", Thickness);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = atoi(coord);
	if (*end != 0) {
		pcb_printf("error parsing free track layer\n");
        	return -1;
	}
	PCBLayer = val; 
	pcb_printf("Found free track layer : %d\n", PCBLayer);
	/* ignore user routed flag */

	if (PCBLayer >= 0 || PCBLayer <= 10) {
		pcb_line_new( &st->PCB->Data->Layer[PCBLayer], X1, Y1, X2, Y2, Thickness, Clearance, Flags);
		pcb_printf("\tnew free line on layer %d created\n", PCBLayer);
		return 0;
	} else {
		pcb_printf("Invalid layer found : %d\n", PCBLayer);
	}
	return -1;
}




/* autotrax_pcb free arc parser */
static int autotrax_parse_free_arc(read_state_t *st, FILE *FP)
{
	int index = 0;
	char line[34]; /* line is 3 x 32000 + segments + width + layer = 29 characters at most */
	char coord[6];

	int i;
	int c;
	char *end;
	double val;

	int segments = 15; /* full circle by default */ 

	pcb_coord_t centreX, centreY, width, height, Thickness, Clearance, radius;
	pcb_angle_t startAngle = 0.0;
	pcb_angle_t delta = 360.0;

	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	int PCBLayer = 0; /* sane default value */

	Clearance = Thickness = PCB_MIL_TO_COORD(10); /* start with sane default of ten mil */

	index =0;
	while ((!feof(FP) && (c = fgetc(FP)) != '\n' && index < 34) || (c == '\n' && index == 0) ) {
		line[index] = c;
		index ++;
	}
	if (index > 32) {
		pcb_printf("error parsing free arc line; too long\n");
		return -1;
	}
	line[index] = ' ';
	index++;
	line[index] = '\0';
	printf("About to parse autotrax free arc: %s\n", line);  
	index = 0;
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	index++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free arc centre X\n");
        	return -1;
	}
	centreX = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free arc centre X : %ml\n", centreX);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free arc centre Y\n");
        	return -1;
	}
	centreY = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free arc Y : %ml\n", centreY);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index ++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free arc radius\n");
        	return -1;
	}
	radius = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free arc radius : %ml\n", radius);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = strtod(coord, &end);
	if (*end != 0 || val > 15 || val < 1) {
		pcb_printf("error parsing free arc segments\n");
        	return -1;
	}
	segments = (int)val;
	pcb_printf("Found free track segments : %d\n", segments);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free arc width\n");
        	return -1;
	}
	Thickness = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free arc width : %ml\n", Thickness);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = atoi(coord);
	if (*end != 0) {
		pcb_printf("error parsing free track layer\n");
        	return -1;
	}
	PCBLayer = val; 
	pcb_printf("Found free track layer : %d\n", PCBLayer);

	width = height = 2*radius;

/* we now need to decrypt the segments value. Some definitions will represent two arcs.
Bit 0 : RU quadrant
Bit 1 : LU quadrant
Bit 2 : LL quadrant
Bit 3 : LR quadrant
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
		delta = 180.0;
	} else if (segments == 6) { /* Left half */
		startAngle = 270.0;
		delta = 180.0;
	} else if (segments == 12) { /* Lower half */
		startAngle = 0.0;
		delta = 180.0;
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

	pcb_arc_new( &st->PCB->Data->Layer[PCBLayer], centreX, centreY, width, height, startAngle, delta, Thickness, Clearance, Flags);
	return 0;
}

/* autotrax_pcb/via */
static int autotrax_parse_free_via(read_state_t *st, FILE *FP)
{
	int index = 0;
	char line[30]; /* line is 4 x 32000 = 23 characters at most */
	char coord[6];

	int i;
	int c;
	char *end, *name;
	double val;

	pcb_coord_t X, Y, Thickness, Clearance, Mask, Drill; /* not sure what to do with mask */
	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */

	Clearance = Mask = Thickness = PCB_MIL_TO_COORD(10); /* start with sane default of ten mil */

	Drill = PCB_MM_TO_COORD(0.300); /* start with something sane */

	name = ""; /*not used*/
	index =0;

	while ((!feof(FP) && (c = fgetc(FP)) != '\n' && index < 25) || (c == '\n' && index == 0) ) {
		line[index] = c;
		index ++;
	}
	if (index > 25) {
		pcb_printf("error parsing free via line; too long\n");
		return -1;
	}
	line[index] = ' ';
	index++;
	line[index] = '\0';
	printf("About to parse autotrax free via: %s\n", line);  
	index = 0;
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free via X\n");
        	return -1;
	}
	X = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free via X : %ml\n", X);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free via Y\n");
        	return -1;
	}
	Y = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free via Y : %ml\n", Y);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index ++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free via diameter\n");
        	return -1;
	}
	Thickness = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free via diameter : %ml\n", Thickness);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free via drill\n");
        	return -1;
	}
	Drill = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free track drill : %ml\n", Drill);
/*  */
	pcb_via_new( st->PCB->Data, X, Y, Thickness, Clearance, Mask, Drill, name, Flags);
	pcb_printf("\tnew free via created\n");
	return 0;
}

/* autotrax_pcb free pad*/
/* FP
x y xsize ysize shape holesize pwr/gnd layer
padname
*/
static int autotrax_parse_free_pad(read_state_t *st, FILE *FP)
{
	int index = 0;
	char line[30]; /* line is 4 x 32000 = 23 characters at most */
	char coord[6];

	int i;
	int c;
	char *end, padname[32];
	double val;

	int Shape = 0;
	int Connects = 0;
	int PCBLayer = 0; /* sane default value */

	pcb_coord_t X, Y, Xsize, Ysize, Thickness, Clearance, Mask, Drill; /* not sure what to do with mask */
	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */

	Clearance = Mask = Thickness = PCB_MIL_TO_COORD(10); /* start with sane default of ten mil */

	Drill = PCB_MM_TO_COORD(0.300); /* start with something sane */

/*	padname = "";*/	
	index =0;

	while ((!feof(FP) && (c = fgetc(FP)) != '\n' && index < 45) || (c == '\n' && index == 0) ) {
		line[index] = c;
		index ++;
	}
	if (index > 42) {
		pcb_printf("error parsing free pad line; too long\n");
		return -1;
	}
	line[index] = ' ';
	index++;
	line[index] = '\0';
	printf("About to parse autotrax free pad: %s\n", line);  
	index = 0;
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free pad X\n");
        	return -1;
	}
	X = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free pad X : %ml\n", X);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free pad Y\n");
        	return -1;
	}
	Y = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free pad Y : %ml\n", Y);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free pad Xsize\n");
        	return -1;
	}
	Xsize = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free pad Xsize : %ml\n", Xsize);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free pad Ysize\n");
        	return -1;
	}
	Ysize = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free pad Ysize : %ml\n", Ysize);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	Shape = atoi(coord); 
	pcb_printf("Found free pad shape : %d\n", Shape);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index ++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free pad drill\n");
        	return -1;
	}
	Drill = PCB_MIL_TO_COORD(val);
	pcb_printf("Found free pad drill : %ml\n", Drill);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	Connects = atoi(coord); 
	pcb_printf("Found free pad connections : %d\n", Connects);
/*  */
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	PCBLayer = atoi(coord); 
	pcb_printf("Found free pad Layer : %d\n", PCBLayer);
/* now find name as string on next line and copy it */
	index = 0;
	while (!feof(FP) && ((c = fgetc(FP)) != '\n') && (index < 32)) {
		padname[index] = c;
		index++;
	}
	padname[index] = '\0';
	pcb_printf("Found free pad name : %s\n", padname);

	Thickness = MIN(Xsize, Ysize);
	if ((Shape == 2  || Shape == 4) ) { /* && PCBLayer == 1) { square (2) or rounded rect (4) on top layer */ 
		Flags = pcb_flag_make(PCB_FLAG_SQUARE); /* actually a rectangle, but... */
/*	} else if ((Shape == 2  || Shape == 4) && PCBLayer == 6) { bottom layer 
		Flags = pcb_flag_make(PCB_FLAG_SQUARE | PCB_FLAG_ONSOLDER); actually a rectangle, but... */
	} else if (Shape == 3) { /*  && PCBLayer == 1) top layer */
		Flags = pcb_flag_make(PCB_FLAG_OCTAGON);
/*	} else if (Shape == 3 && PCBLayer == 6) {  bottom layer 
		Flags = pcb_flag_make(PCB_FLAG_OCTAGON | PCB_FLAG_ONSOLDER); */
	}		

/* 	TODO: having fully parsed the free pad, and determined, rectangle vs octagon vs round
	the problem is autotrax can define an SMD pad, but we have to map this to a pin, since a
	discrete element would be needed for a standalone pad. Padstacks may allow more flexibility
	The onsolder flags are reduntant for now with vias.

	currently ignore:
	shape:
		5 Cross Hair Target
		6 Moiro Target
*/
	if (Shape == 5 && Shape == 6) {
		pcb_printf("\tnew free pad not created; as it is a Cross Hair Target or Moiro target\n");
	} else {
		pcb_via_new( st->PCB->Data, X, Y, Thickness, Clearance, Mask, Drill, padname, Flags);
		pcb_printf("\tnew free pad/hole created; need to check connects\n");
	}
	return 0;
}


static int autotrax_parse_free_fill(read_state_t *st, FILE *FP)
{
	int index = 0;
	char line[30]; /* line is 4 x 32000 + layer = 26 characters at most */
	char coord[6];

	int i;
	int c;

	pcb_polygon_t *polygon = NULL;
	pcb_flag_t flags = pcb_flag_make(PCB_FLAG_CLEARPOLY);
	char *end;
	double val;
	pcb_coord_t X1, Y1, X2, Y2;
	int PCBLayer = 0;

	index =0;
	while ((!feof(FP) && (c = fgetc(FP)) != '\n' && index < 29) || (c == '\n' && index == 0) ) {
		line[index] = c;
		index ++;
	}
	if (index > 27) {
		pcb_printf("error parsing free fill line; too long\n");
		return -1;
	}
	line[index] = ' ';
	index++;
	line[index] = '\0';
	index = 0;
	printf("About to parse free fill line: %s\n", line); 
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing free fill X1\n");
        	return -1;
	}
	X1 = PCB_MIL_TO_COORD(val);
	pcb_printf("Found fill X1 : %ml\n", X1);
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	Y1 = PCB_MIL_TO_COORD(atoi(coord));
	pcb_printf("Found fill Y1 : %ml\n", Y1);
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index ++;
	X2 = PCB_MIL_TO_COORD(atoi(coord));
	pcb_printf("Found fill X2 : %ml\n", X2);
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	Y2 = PCB_MIL_TO_COORD(atoi(coord));
	pcb_printf("Found fill Y2 : %ml\n", Y2);

	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	PCBLayer = atoi(coord); 
	pcb_printf("Found fill layer : %d\n", PCBLayer);
	if (PCBLayer >= 0 || PCBLayer <= 10) {
		polygon = pcb_poly_new(&st->PCB->Data->Layer[PCBLayer], flags);
	} else {
		pcb_printf("Invalid free fill layer found : %d\n", PCBLayer);
	}
	if (polygon != NULL) {
		pcb_poly_point_new(polygon, X1, Y1);
		pcb_poly_point_new(polygon, X2, Y1);
		pcb_poly_point_new(polygon, X2, Y2);
		pcb_poly_point_new(polygon, X1, Y2);
		pcb_add_polygon_on_layer(&st->PCB->Data->Layer[PCBLayer], polygon);
		pcb_poly_init_clip(st->PCB->Data, &st->PCB->Data->Layer[PCBLayer], polygon);
		return 0;
	}
	return -1;
}

/* Register a kicad layer in the layer hash after looking up the pcb-rnd equivalent */
static unsigned int autotrax_reg_layer(read_state_t *st, const char *autotrax_name, unsigned int mask)
{
        pcb_layer_id_t id;
        if (pcb_layer_list(mask, &id, 1) != 1) {
                pcb_layergrp_id_t gid;
                pcb_layergrp_list(PCB, mask, &gid, 1);
                id = pcb_layer_create(gid, autotrax_name);
        }
        htsi_set(&st->layer_k2i, pcb_strdup(autotrax_name), id);
        return 0;
}

/* Parse a layer definition and do all the administration needed for the layer */
static int autotrax_create_layers(read_state_t *st)
{
	unsigned int res;
	pcb_layer_id_t id = -1;
	pcb_layergrp_id_t gid = -1;
	pcb_layergrp_t *g = pcb_get_grp_new_intern(PCB, -1);

	/* set up the hash for implicit layers */
	res = 0;
	res |= autotrax_reg_layer(st, "F.SilkS", PCB_LYT_SILK | PCB_LYT_TOP);
	res |= autotrax_reg_layer(st, "B.SilkS", PCB_LYT_SILK | PCB_LYT_BOTTOM);

	/* for modules */
	res |= autotrax_reg_layer(st, "Top", PCB_LYT_COPPER | PCB_LYT_TOP);
	res |= autotrax_reg_layer(st, "Bottom", PCB_LYT_COPPER | PCB_LYT_BOTTOM);


	if (res != 0) {
		pcb_message(PCB_MSG_ERROR, "Internal error: can't find a silk or mask layer\n");
		pcb_layergrp_inhibit_dec();
                return -1;
	}

	pcb_layergrp_fix_old_outline(PCB);
	
	pcb_layergrp_list(PCB, PCB_LYT_COPPER | PCB_LYT_BOTTOM, &gid, 1);
	id = pcb_layer_create(gid, "Bottom");
	htsi_set(&st->layer_k2i, pcb_strdup("Bottom"), id);

	pcb_layergrp_list(PCB, PCB_LYT_COPPER | PCB_LYT_TOP, &gid, 1);
	id = pcb_layer_create(gid, "Top");
	htsi_set(&st->layer_k2i, pcb_strdup("Top"), id);

	id = pcb_layer_create(g - PCB->LayerGroups.grp, "Mid1");
	htsi_set(&st->layer_k2i, pcb_strdup("Mid1"), id);

	g = pcb_get_grp_new_intern(PCB, -1);
	id = pcb_layer_create(g - PCB->LayerGroups.grp, "Mid2");
	htsi_set(&st->layer_k2i, pcb_strdup("Mid2"), id);

	g = pcb_get_grp_new_intern(PCB, -1);
	id = pcb_layer_create(g - PCB->LayerGroups.grp, "Mid3");
	htsi_set(&st->layer_k2i, pcb_strdup("Mid3"), id);

	g = pcb_get_grp_new_intern(PCB, -1);
	id = pcb_layer_create(g - PCB->LayerGroups.grp, "Mid4");
	htsi_set(&st->layer_k2i, pcb_strdup("Mid4"), id);

	pcb_layergrp_fix_old_outline(PCB);

	pcb_layergrp_inhibit_dec();
	
	return 0;
}

/* Returns the pcb-rnd layer index for a autotrax_name, or -1 if not found */
static int autotrax_get_layeridx(read_state_t *st, const char *autotrax_name)
{
	htsi_entry_t *e;
	e = htsi_getentry(&st->layer_k2i, autotrax_name);
	if (e == NULL)
		return -1;
	return e->value;
}

/* autotrax_pcb  parse (net  ) ;   used for net descriptions for the entire layout */
static int autotrax_parse_net(read_state_t *st, gsxl_node_t *subtree)
{
		if (subtree != NULL && subtree->str != NULL) {
			pcb_printf("net number: '%s'\n", subtree->str);
		} else {
			pcb_printf("missing net number in net descriptors");
			return autotrax_error(subtree, "missing net number in net descriptors.");
		}
		if (subtree->next != NULL && subtree->next->str != NULL) {
			pcb_printf("\tcorresponding net label: '%s'\n", (subtree->next->str));
		} else {
			pcb_printf("missing net label in net descriptors");
			return autotrax_error(subtree, "missing net label in net descriptors.");
		}
		return 0;
}

static int autotrax_parse_module(read_state_t *st, gsxl_node_t *subtree)
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
	int moduleOnTop = 1;
	int padLayerDefCount = 0;
	int SMD = 0;
	int square = 0;
	int throughHole = 0;
	int foundRefdes = 0;
	int refdesScaling  = 100;
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
	char * end, * textLabel, * text;
	char * moduleName, * moduleRefdes, * moduleValue, * pinName;
	pcb_element_t *newModule;

	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	pcb_flag_t TextFlags = pcb_flag_make(0); /* start with something bland here */
	Clearance = PCB_MM_TO_COORD(0.250); /* start with something bland here */

	moduleName = NULL;
	moduleRefdes = "";
	moduleValue = "";

	if (subtree->str != NULL) {
		printf("Name of module element being parsed: '%s'\n", subtree->str);
		moduleName = subtree->str;

		SEEN_NO_DUP(tally, 0);
		for(n = subtree->next, i = 0; n != NULL; n = n->next, i++) {
			if (n->str != NULL && strcmp("layer", n->str) == 0) { /* need this to sort out ONSOLDER flags etc... */
				SEEN_NO_DUP(tally, 1);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\tlayer: '%s'\n", (n->children->str));
					PCBLayer = autotrax_get_layeridx(st, n->children->str);
					moduleLayer = PCBLayer;
					if (PCBLayer < 0) {
						return autotrax_error(subtree, "module layer error - layer < 0.");
					} else if (pcb_layer_flags(PCB, PCBLayer) & PCB_LYT_BOTTOM) {
							Flags = pcb_flag_make(PCB_FLAG_ONSOLDER);
							TextFlags = pcb_flag_make(PCB_FLAG_ONSOLDER);
							moduleOnTop = 0;
					}
				} else {
					return autotrax_error(subtree, "unexpected empty/NULL module layer node");
				}
			} else if (n->str != NULL && strcmp("tedit", n->str) == 0) {
				SEEN_NO_DUP(tally, 2);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\ttedit: '%s'\n", (n->children->str));
				} else {
					return autotrax_error(subtree, "unexpected empty/NULL module tedit node");
				}
			} else if (n->str != NULL && strcmp("tstamp", n->str) == 0) {
				SEEN_NO_DUP(tally, 3);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\ttstamp: '%s'\n", (n->children->str));
				} else {
					return autotrax_error(subtree, "unexpected empty/NULL module tstamp node");
				}
			} else if (n->str != NULL && strcmp("attr", n->str) == 0) {
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\tmodule attribute \"attr\": '%s' (not used)\n", (n->children->str));
				} else {
					return autotrax_error(subtree, "unexpected empty/NULL module attr node");
				}
			} else if (n->str != NULL && strcmp("at", n->str) == 0) {
				SEEN_NO_DUP(tally, 4);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\tat x: '%s'\n", (n->children->str));
					SEEN_NO_DUP(tally, 5); /* same as ^= 1 was */
						val = strtod(n->children->str, &end);
						if (*end != 0) {
							return autotrax_error(subtree, "error parsing module X.");
						} else {
							moduleX = PCB_MM_TO_COORD(val);
						}
				} else {
					return autotrax_error(subtree, "unexpected empty/NULL module X node");
				}
				if (n->children->next != NULL && n->children->next->str != NULL) {
					pcb_printf("\tat y: '%s'\n", (n->children->next->str));
					SEEN_NO_DUP(tally, 6);	
						val = strtod(n->children->next->str, &end);
						if (*end != 0) {
							return autotrax_error(subtree, "error parsing module Y.");
						} else {
							moduleY = PCB_MM_TO_COORD(val);
						}
				} else {
					return autotrax_error(subtree, "unexpected empty/NULL module Y node");
				}
				if (n->children->next->next != NULL && n->children->next->next->str != NULL) {
					pcb_printf("\tmodule rotation: '%s'\n", (n->children->next->next->str));
					val = strtod(n->children->next->next->str, &end);
					if (*end != 0) {
						pcb_printf("\tmodule (at) \"rotation\" value error\n");
						return autotrax_error(subtree, "error parsing module rotation.");
					} else {
						moduleRotation = (int)val;
					}
				} else {
					pcb_printf("\tno module (at) \"rotation\" value found'\n");
				}
			/* if we have been provided with a Module Name and location, create a new Element with default "" and "" for refdes and value fields */
				if (moduleName != NULL && moduleDefined == 0) {
					moduleDefined = 1; /* but might be empty, wait and see */
					printf("Have new module name and location, defining module/element %s\n", moduleName);
						newModule = pcb_element_new(st->PCB->Data, NULL,
										 pcb_font(st->PCB, 0, 1), Flags,
										 moduleName, moduleRefdes, moduleValue,
										 moduleX, moduleY, direction,
										 refdesScaling, TextFlags,  pcb_false); /*pcb_flag_t TextFlags, pcb_bool uniqueName) */
					pcb_move_obj(PCB_TYPE_ELEMENT_NAME, newModule,  &newModule->Name[PCB_ELEMNAME_IDX_VISIBLE()],  &newModule->Name[PCB_ELEMNAME_IDX_VISIBLE()], X, Y);
					moduleRefdes = NULL;
					moduleValue = NULL;
				}
			} else if (n->str != NULL && strcmp("model", n->str) == 0) {
				pcb_printf("module 3D model found and ignored\n");
			} else if (n->str != NULL && strcmp("fp_text", n->str) == 0) {
					pcb_printf("fp_text found\n");
					featureTally = 0;

/* ********************************************************** */

	if (n->children != NULL && n->children->str != NULL) {
		textLabel = n->children->str;
		printf("fp_text element being parsed for %s - label: '%s'\n", moduleName, textLabel);
		if (n->children->next != NULL && n->children->next->str != NULL) {
			text = n->children->next->str;
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
				printf("\tmoduleValue now: '%s'\n", moduleValue);
			} else if (strcmp("hide", textLabel) == 0) {
				pcb_printf("\tignoring fp_text \"hide\" flag\n");
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
							return autotrax_error(subtree, "error parsing module fp_text X.");
						} else {
							X = PCB_MM_TO_COORD(val);
							if (foundRefdes) {
								refdesX = X;
								pcb_printf("\tRefdesX = %mm\n", refdesX);
							}
						}
					} else {
						return autotrax_error(subtree, "unexpected empty/NULL module fp_text X node");
					}
					if (l->children->next != NULL && l->children->next->str != NULL) {
						pcb_printf("\ttext at y: '%s'\n", (l->children->next->str));
						SEEN_NO_DUP(featureTally, 2);
						val = strtod(l->children->next->str, &end);
						if (*end != 0) {
							return autotrax_error(subtree, "error parsing module fp_text Y.");
						} else {
							Y = PCB_MM_TO_COORD(val);
							if (foundRefdes) {
								refdesY = Y;
								pcb_printf("\tRefdesY = %mm\n", refdesY);
							}
						}	
						if (l->children->next->next != NULL && l->children->next->next->str != NULL) {
							pcb_printf("\ttext rotation: '%s'\n", (l->children->next->next->str));
							val = strtod(l->children->next->next->str, &end);
							if (*end != 0) {
								return autotrax_error(subtree, "error parsing module fp_text rotation.");
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
						return autotrax_error(subtree, "unexpected empty/NULL module fp_text Y node");
					}
			} else if (l->str != NULL && strcmp("layer", l->str) == 0) {
				SEEN_NO_DUP(featureTally, 4);
				if (l->children != NULL && l->children->str != NULL) {
					pcb_printf("\ttext layer: '%s'\n", (l->children->str));
					PCBLayer = autotrax_get_layeridx(st, l->children->str);
					if (PCBLayer < 0) {
						pcb_printf("\ttext layer not defined for module text, default being used.\n");
						Flags = pcb_flag_make(0);
						/*return -1;*/
					} else if (pcb_layer_flags(PCB, PCBLayer) & PCB_LYT_BOTTOM) {
							Flags = pcb_flag_make(PCB_FLAG_ONSOLDER);
					}
				} else {
					return autotrax_error(subtree, "unexpected empty/NULL module fp_text layer node");
				}
			} else if (l->str != NULL && strcmp("hide", l->str) == 0) {
				pcb_printf("\tfp_text hidden flag \"hide\" found and ignored.\n");
			} else if (l->str != NULL && strcmp("justify", l->str) == 0) {/*this may be unnecessary here*/
				pcb_printf("\tfp_text justify flag found and ignored.\n");
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
										return autotrax_error(subtree, "error parsing module fp_text font size X.");
									} else {
										scaling = (int) (100*val/1.27); /* standard glyph width ~= 1.27mm */
										if (foundRefdes) {
											refdesScaling = scaling;
											/*foundRefdes = 0;*/
										}
									}
								} else {
									return autotrax_error(subtree, "unexpected empty/NULL module fp_text X size node");
								}
								if (p->children->next != NULL && p->children->next->str != NULL) {
									pcb_printf("\tfont sizeY: '%s'\n", (p->children->next->str));
								} else {
									return autotrax_error(subtree, "unexpected empty/NULL module fp_text Y size node");
								}
							} else if (strcmp("thickness", p->str) == 0) {
								SEEN_NO_DUP(featureTally, 8);
								if (p->children != NULL && p->children->str != NULL) {
									pcb_printf("\tfont thickness: '%s'\n", (p->children->str));
								} else {
									return autotrax_error(subtree, "unexpected empty/NULL module fp_text thickness node");
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
							return autotrax_error(subtree, "unexpected empty/NULL module fp_text justify node");
						}
					} else {
						if (m->str != NULL) {
							printf("Unknown text effects argument %s:", m->str);
						}
						return autotrax_error(subtree, "unknown fp_text text effects argument null node");
					}
				}
			} 				
		}
	}
	required = BV(0) | BV(1) | BV(4) | BV(7) | BV(8);
	if ((tally & required) == required) { /* has location, layer, size and stroke thickness at a minimum */
#warning TODO: this will never be NULL; what are we trying to check here?
		if (&st->PCB->fontkit.dflt == NULL) {
			pcb_font_create_default(st->PCB);
		}

		X = refdesX;
		Y = refdesY;

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

		/* if we  update X, Y for text fields (refdes, value), we would do it here */
	}


/* ********************************************************** */

			} else if (n->str != NULL && strcmp("descr", n->str) == 0) {
				SEEN_NO_DUP(tally, 9);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\tmodule descr: '%s'\n", (n->children->str));
				} else {
					return autotrax_error(subtree, "unexpected empty/NULL module descr node");
				}
			} else if (n->str != NULL && strcmp("tags", n->str) == 0) {
				SEEN_NO_DUP(tally, 10);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\tmodule tags: '%s'\n", (n->children->str)); /* maye be more than one? */
				} else {
					return autotrax_error(subtree, "unexpected empty/NULL module tags node");
				}
			} else if (n->str != NULL && strcmp("path", n->str) == 0) {
				SEEN_NO_DUP(tally, 11);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\tmodule path: '%s'\n", (n->children->str));
				} else {
					return autotrax_error(subtree, "unexpected empty/NULL module path node");
				}
			} else if (n->str != NULL && strcmp("model", n->str) == 0) {
				SEEN_NO_DUP(tally, 12);
				if (n->children != NULL && n->children->str != NULL) {
					pcb_printf("\tmodule model provided: '%s'\n", (n->children->str));
				} else {
					return autotrax_error(subtree, "unexpected empty/NULL module model node");
				}
			/* pads next  - have thru_hole, circle, rect, roundrect, to think about*/ 
			} else if (n->str != NULL && strcmp("pad", n->str) == 0) {
				featureTally = 0;
				padLayerDefCount = 0;
				padRotation = 0;
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
							return autotrax_error(subtree, "unexpected empty/NULL module pad shape node");
						}
					} else {
						return autotrax_error(subtree, "unexpected empty/NULL module pad type node");
					}
				} else {
					return autotrax_error(subtree, "unexpected empty/NULL module pad name  node");
				}
				if (n->children->next->next->next == NULL || n->children->next->next->next->str == NULL) {
					return autotrax_error(subtree, "unexpected empty/NULL module node");
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
								return autotrax_error(subtree, "error parsing module pad X.");
							} else {
								X = PCB_MM_TO_COORD(val);
							}
							if (m->children->next != NULL && m->children->next->str != NULL) {
								pcb_printf("\tpad Y position:\t'%s'\n", (m->children->next->str));
								val = strtod(m->children->next->str, &end);
								if (*end != 0) {
									return autotrax_error(subtree, "error parsing module pad Y.");
								} else {
									Y = PCB_MM_TO_COORD(val);
								}
							} else {
								return autotrax_error(subtree, "unexpected empty/NULL module X node");
							}
							if (m->children->next->next != NULL && m->children->next->next->str != NULL) {
								pcb_printf("\tpad rotation:\t'%s'\n", (m->children->next->next->str));
								val = strtod(m->children->next->next->str, &end);
								if (*end != 0) {
									pcb_printf("Odd pad rotation def ignored.");
								} else {
									padRotation = (int) val;
								}
							}
						} else {
							return autotrax_error(subtree, "unexpected empty/NULL module Y node");
						}
					} else if (m->str != NULL && strcmp("layers", m->str) == 0) {
						if (SMD) { /* skip testing for pins */
							SEEN_NO_DUP(featureTally, 2);
							kicadLayer = 15;
							for(l = m->children; l != NULL; l = l->next) {
								if (l->str != NULL) {
									PCBLayer = autotrax_get_layeridx(st, l->str);
									if (PCBLayer < 0) {
										/* we ignore *.mask, *.paste, etc., if valid layer def already found */
										printf("Unknown layer definition: %s\n", l->str);
										if (!padLayerDefCount) {
											printf("Default placement of pad is the copper layer defined for module as a whole\n");

											/*return -1;*/
											if (!moduleOnTop) {
												kicadLayer = 0;
											}
										}
									} else if (PCBLayer < -1) {
										printf("\tUnimplemented layer definition: %s\n", l->str);
									} else if (pcb_layer_flags(PCB, PCBLayer) & PCB_LYT_BOTTOM) {
										kicadLayer = 0;
										padLayerDefCount++;
									} else if (padLayerDefCount) {
										printf("More than one valid pad layer found, only using the first one found for layer.\n");
										padLayerDefCount++;
									} else {
										padLayerDefCount++;
										printf("Valid layer defs found for current pad: %d\n", padLayerDefCount);
									}
									pcb_printf("\tpad layer: '%s',  PCB layer number %d\n", (l->str), autotrax_get_layeridx(st, l->str));
								} else {
									return autotrax_error(subtree, "unexpected empty/NULL module layer node");
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
								return autotrax_error(subtree, "error parsing module pad drill.");
							} else {
								drill = PCB_MM_TO_COORD(val);
							}

						} else {
							return autotrax_error(subtree, "unexpected empty/NULL module pad drill node");
						}
					} else if (m->str != NULL && strcmp("net", m->str) == 0) { 
						SEEN_NO_DUP(featureTally, 4);
						if (m->children != NULL && m->children->str != NULL) {
							pcb_printf("\tpad's net number:\t'%s'\n", (m->children->str));
							if (m->children->next != NULL && m->children->next->str != NULL) {
								pcb_printf("\tpad's net name:\t'%s'\n", (m->children->next->str));
							} else {
								return autotrax_error(subtree, "unexpected empty/NULL module pad net name node");
							}
						} else {
							return autotrax_error(subtree, "unexpected empty/NULL module pad net node");
						}
					} else if (m->str != NULL && strcmp("size", m->str) == 0) {
						SEEN_NO_DUP(featureTally, 5);
						if (m->children != NULL && m->children->str != NULL) {
							pcb_printf("\tpad X size:\t'%s'\n", (m->children->str));
							val = strtod(m->children->str, &end);
							if (*end != 0) {
								return autotrax_error(subtree, "error parsing module pad size X.");
							} else {
								padXsize = PCB_MM_TO_COORD(val);
							}
							if (m->children->next != NULL && m->children->next->str != NULL) {
								pcb_printf("\tpad Y size:\t'%s'\n", (m->children->next->str));
								val = strtod(m->children->next->str, &end);
								if (*end != 0) {
									return autotrax_error(subtree, "error parsing module pad size Y.");
								} else {
									padYsize = PCB_MM_TO_COORD(val);
								}
							} else {
								return autotrax_error(subtree, "unexpected empty/NULL module pad Y size node");
							}
						} else {
							return autotrax_error(subtree, "unexpected empty/NULL module pad X size node");
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
						moduleEmpty = 0;
						pcb_element_pin_new(newModule, X + moduleX, Y + moduleY, padXsize, Clearance,
								Clearance, drill, pinName, pinName, Flags); /* using clearance value for arg 5 = mask too */
					} else {
						return autotrax_error(subtree, "malformed module pad/pin definition.");
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
						moduleEmpty = 0;
						if (padRotation != 0) {
							padRotation = padRotation/90;/*ignore rotation != n*90*/
							PCB_COORD_ROTATE90(X1, Y1, X, Y, padRotation);
							PCB_COORD_ROTATE90(X2, Y2, X, Y, padRotation);
							pcb_printf("\t Pad rotation %d applied\n", padRotation);
						}
						pcb_element_pad_new(newModule, X1 + moduleX, Y1 + moduleY, X2 + moduleX, Y2 + moduleY, Thickness, Clearance, 
								Clearance, pinName, pinName, Flags); /* using clearance value for arg 7 = mask too */
					} else {
						return autotrax_error(subtree, "error parsing incomplete module definition.");
					}
				} else {
					return autotrax_error(subtree, "error - unable to create incomplete module definition.");
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
							return autotrax_error(subtree, "error parsing fp_line start X.");
						} else {
							X1 = PCB_MM_TO_COORD(val) + moduleX;
						}
					} else {
						return autotrax_error(subtree, "unexpected fp_line start X null node.");
					}
					if (l->children->next != NULL && l->children->next->str != NULL) {
						pcb_printf("\tfp_line start at y: '%s'\n", (l->children->next->str));
						SEEN_NO_DUP(featureTally, 2);	
						val = strtod(l->children->next->str, &end);
						if (*end != 0) {
							return autotrax_error(subtree, "error parsing fp_line start Y.");
						} else {
							Y1 = PCB_MM_TO_COORD(val) + moduleY;
						}
					} else {
						return autotrax_error(subtree, "unexpected fp_line start Y null node.");
					}
			} else if (l->str != NULL && strcmp("end", l->str) == 0) {
					SEEN_NO_DUP(featureTally, 3);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_line end at x: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 4);
						val = strtod(l->children->str, &end);
						if (*end != 0) {
							return autotrax_error(subtree, "error parsing fp_line end X.");
						} else {
							X2 = PCB_MM_TO_COORD(val) + moduleX;
						}
					} else {
						return autotrax_error(subtree, "unexpected fp_line end X null node.");
					}
					if (l->children->next != NULL && l->children->next->str != NULL) {
						pcb_printf("\tfp_line end at y: '%s'\n", (l->children->next->str));
						SEEN_NO_DUP(featureTally, 5);	
						val = strtod(l->children->next->str, &end);
						if (*end != 0) {
							return autotrax_error(subtree, "error parsing fp_line end Y.");
						} else {
							Y2 = PCB_MM_TO_COORD(val) + moduleY;
						}
					} else {
						return autotrax_error(subtree, "unexpected fp_line end Y null node.");
					}
			} else if (l->str != NULL && strcmp("layer", l->str) == 0) {
					SEEN_NO_DUP(featureTally, 6);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_line layer: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 7);
						PCBLayer = autotrax_get_layeridx(st, l->children->str);
						if (PCBLayer < 0) {
							pcb_message(PCB_MSG_ERROR, "\tline layer not defined for module line, using module layer.\n");
							PCBLayer = moduleLayer;
							return 0;
						}
					} else {
						pcb_message(PCB_MSG_ERROR, "\tusing default module layer for gr_line element\n");
						PCBLayer = moduleLayer; /* default to module layer */
						/* return -1; */
					}
			} else if (l->str != NULL && strcmp("width", l->str) == 0) {
					SEEN_NO_DUP(featureTally, 8);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_line width: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 9);
						val = strtod(l->children->str, &end);
						if (*end != 0) {
							return autotrax_error(subtree, "error parsing fp_line width.");
						} else {
							Thickness = PCB_MM_TO_COORD(val);
						}
					} else {
						return autotrax_error(subtree, "unexpected fp_line width null node.");
					}
			} else if (l->str != NULL && strcmp("angle", l->str) == 0) { /* unlikely to be used or seen */
					SEEN_NO_DUP(featureTally, 10);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_line angle: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 11);
					} else {
						return autotrax_error(subtree, "unexpected fp_line angle null node.");
					}
			} else if (l->str != NULL && strcmp("net", l->str) == 0) { /* unlikely to be used or seen */
					SEEN_NO_DUP(featureTally, 12);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_line net: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 13);
					} else {
						return autotrax_error(subtree, "unexpected fp_line net null node.");
					}
			} else {
				if (l->str != NULL) {
					printf("Unknown fp_line argument %s:", l->str);
				}
				return autotrax_error(subtree, "unexpected fp_line null node.");
			}
		}
	}
	required = BV(0) | BV(3) | BV(6) | BV(8);
	if (((featureTally & required) == required) && newModule != NULL) { /* need start, end, layer, thickness at a minimum */
		moduleEmpty = 0;
		pcb_element_line_new(newModule, X1, Y1, X2, Y2, Thickness);
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
							return autotrax_error(subtree, "error parsing fp_arc X.");
						} else {
							centreX = PCB_MM_TO_COORD(val);
						}
					} else {
						return autotrax_error(subtree, "unexpected fp_arc start X null node.");
					}
					if (l->children->next != NULL && l->children->next->str != NULL) {
						pcb_printf("\tfp_arc centre at y: '%s'\n", (l->children->next->str));
						SEEN_NO_DUP(featureTally, 2);	
						val = strtod(l->children->next->str, &end);
						if (*end != 0) {
							return autotrax_error(subtree, "error parsing fp_arc Y.");
						} else {
							centreY = PCB_MM_TO_COORD(val);
						}
					} else {
						return autotrax_error(subtree, "unexpected fp_arc start Y null node.");
					}
			} else if (l->str != NULL && strcmp("center", l->str) == 0) { /* this lets us parse a circle too */
					SEEN_NO_DUP(featureTally, 0);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_arc centre at x: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 1);
						val = strtod(l->children->str, &end);
						if (*end != 0) {
							return autotrax_error(subtree, "error parsing fp_arc centre X.");
						} else {
							centreX = PCB_MM_TO_COORD(val);
						}
					} else {
						return autotrax_error(subtree, "unexpected fp_arc centre X null node.");
					}
					if (l->children->next != NULL && l->children->next->str != NULL) {
						pcb_printf("\tfp_arc centre at y: '%s'\n", (l->children->next->str));
						SEEN_NO_DUP(featureTally, 2);
						val = strtod(l->children->next->str, &end);
						if (*end != 0) {
							return autotrax_error(subtree, "error parsing fp_arc centre Y.");
						} else {
							centreY = PCB_MM_TO_COORD(val);
						}
					} else {
						return autotrax_error(subtree, "unexpected fp_arc centre Y null node.");
					}
			} else if (l->str != NULL && strcmp("end", l->str) == 0) {
					SEEN_NO_DUP(featureTally, 3);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_arc end at x: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 4);
						val = strtod(l->children->str, &end);
						if (*end != 0) {
							return autotrax_error(subtree, "error parsing fp_arc end X.");
						} else {
							endX = PCB_MM_TO_COORD(val);
						}
					} else {
						return autotrax_error(subtree, "unexpected fp_arc end X null node.");
					}
					if (l->children->next != NULL && l->children->next->str != NULL) {
						pcb_printf("\tfp_arc end at y: '%s'\n", (l->children->next->str));
						SEEN_NO_DUP(featureTally, 5);
						val = strtod(l->children->next->str, &end);
						if (*end != 0) {
							return autotrax_error(subtree, "error parsing fp_arc end Y.");
						} else {
							endY = PCB_MM_TO_COORD(val);
						}
					} else {
						return autotrax_error(subtree, "unexpected fp_arc end Y null node.");
					}
			} else if (l->str != NULL && strcmp("layer", l->str) == 0) {
					SEEN_NO_DUP(featureTally, 6);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_arc layer: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 7);
						PCBLayer = autotrax_get_layeridx(st, l->children->str);
						if (PCBLayer < 0) {
							pcb_message(PCB_MSG_ERROR, "\tinvalid gr_arc layer def, using module default layer.\n");
							PCBLayer = moduleLayer; /* revert to default */
						}
					} else {
						PCBLayer = moduleLayer;
						pcb_message(PCB_MSG_ERROR, "\tusing default module layer for gr_arc element.\n");
					}
			} else if (l->str != NULL && strcmp("width", l->str) == 0) {
					SEEN_NO_DUP(featureTally, 8);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_arc width: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 9);
						val = strtod(l->children->str, &end);
						if (*end != 0) {
							return autotrax_error(subtree, "error parsing fp_arc width.");
						} else {
							Thickness = PCB_MM_TO_COORD(val);
						}
					} else {
						return autotrax_error(subtree, "unexpected fp_arc width null node.");
					}
			} else if (l->str != NULL && strcmp("angle", l->str) == 0) {
					SEEN_NO_DUP(featureTally, 10);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_arc angle CW rotation: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 11);
						val = strtod(l->children->str, &end);
						if (*end != 0) {
							return autotrax_error(subtree, "error parsing fp_arc angle.");
						} else {
							delta = val;
						}
					} else {
						return autotrax_error(subtree, "unexpected fp_arc angle null node.");
					}
			} else if (l->str != NULL && strcmp("net", l->str) == 0) { /* unlikely to be used or seen */
					SEEN_NO_DUP(featureTally, 12);
					if (l->children != NULL && l->children->str != NULL) {
						pcb_printf("\tfp_arc net: '%s'\n", (l->children->str));
						SEEN_NO_DUP(featureTally, 13);
					} else {
						return autotrax_error(subtree, "unexpected fp_arc net null node.");
					}
			} else {
				if (n->str != NULL) {
					printf("Unknown gr_arc argument %s:", l->str);
				}
				return autotrax_error(subtree, "unexpected fp_arc null node.");
			}
		}
	}
        required = BV(0) | BV(6) | BV(8);
        if (((featureTally & required) == required) && newModule != NULL) {
		moduleEmpty = 0;
		/* need start, layer, thickness at a minimum */
		/* same code used above for gr_arc parsing */
		width = height = pcb_distance(centreX, centreY, endX, endY); /* calculate radius of arc */
		if (width < 1) { /* degenerate case */
			startAngle = 0;
		} else {
			endAngle = 180+180*atan2(-(endY - centreY), endX - centreX)/M_PI; /* avoid using atan2 with zero parameters */
			pcb_printf("\tcalculated end angle: '%f'\n", endAngle);
			if (endAngle < 0.0) {
				endAngle += 360.0; /*make it 0...360 */
				pcb_printf("\tadjusted end angle: '%f'\n", endAngle);
			}
			startAngle = (endAngle - delta); /* geda is 180 degrees out of phase with kicad, and opposite direction rotation */
			pcb_printf("\tcalculated start angle: '%f'\n", startAngle);
			if (startAngle > 360.0) {
				startAngle -= 360.0;
			}
			if (startAngle < 0.0) {
				startAngle += 360.0;
			}
			pcb_printf("\tadjusted start angle: '%f'\n", startAngle);
		}
		pcb_element_arc_new(newModule, moduleX + centreX, moduleY + centreY, width, height, startAngle, delta, Thickness); /* was endAngle */

	}

/* ********************************************************** */


			} else {
				if (n->str != NULL) {
					printf("Unknown pad argument : %s\n", n->str);
				}
			} 
		}


		if (newModule != NULL) {
			if (moduleEmpty) { /* should try and use module empty function here */
				Thickness = PCB_MM_TO_COORD(0.200);
				pcb_element_line_new(newModule, moduleX, moduleY, moduleX+1, moduleY+1, Thickness);
				pcb_printf("\tEmpty Module!! 1nm line created at module centroid.\n");
			}
                        pcb_element_bbox(PCB->Data, newModule, pcb_font(PCB, 0, 1));
			if (moduleRotation != 0) {
				pcb_printf("Applying module rotation of %d here.", moduleRotation);
				moduleRotation = moduleRotation/90;/* ignore rotation != n*90 for now*/
				pcb_element_rotate90(PCB->Data, newModule, moduleX, moduleY, moduleRotation);
				/* can test for rotation != n*90 degrees if necessary, and call
				 * void pcb_element_rotate(pcb_data_t *Data, pcb_element_t *Element,
				 *		pcb_coord_t X, pcb_coord_t Y, double 
				 *		cosa, double sina, pcb_angle_t angle);
				 */
			}
			/*pcb_element_bbox(PCB->Data, newModule, pcb_font(PCB, 0, 1));*/

			/* update the newly created module's refdes field, if available */
			if (moduleDefined && moduleRefdes) {
				printf("have Refdes for module/element %s, updating new module\n", moduleRefdes);
				free(newModule->Name[PCB_ELEMNAME_IDX_REFDES].TextString);
				newModule->Name[PCB_ELEMNAME_IDX_REFDES].TextString = pcb_strdup(moduleRefdes);
			}
			/* update the newly created module's value field, if available */
			if (moduleDefined && moduleValue) {
				printf("have Value for module/element %s, updating new module\n", moduleValue);
				free(newModule->Name[PCB_ELEMNAME_IDX_VALUE].TextString);
				newModule->Name[PCB_ELEMNAME_IDX_VALUE].TextString = pcb_strdup(moduleValue);
			}
			return 0;
		} else {
			return autotrax_error(subtree, "unable to create incomplete module.");
		}
	} else {
		return autotrax_error(subtree, "module parsing failure.");
	}
}

	
int io_autotrax_read_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, conf_role_t settings_dest)
{
	int c, readres = 0;
	pcb_box_t boardSize, *box;
	read_state_t st;
	FILE *FP;

	int index = 0;
	char line[1024], *s;

	FP = fopen(Filename, "r");
	if (FP == NULL)
		return -1;

	/* set up the parse context */
	memset(&st, 0, sizeof(st));
	st.PCB = Ptr;
	st.Filename = Filename;
	st.settings_dest = settings_dest;
	htsi_init(&st.layer_k2i, strhash, strkeyeq);

	while (!feof(FP)) {
		index =0;
		while (!feof(FP) && (c = fgetc(FP)) != '\n' && index < 1023) {
			line[index] = c;
			index ++;
		}
		index++;
		line[index] = '\0';
		s = line;
		if (index > 10) {
			if (strncmp(line, "PCB FILE 4", 10) == 0 ) {
				printf("Found Protel Autotrax version 4\n");
				autotrax_create_layers(&st);
			} else if (strncmp(line, "PCB FILE 5", 10) == 0 ) {
				printf("Found Protel Easytrax version 5\n");
				autotrax_create_layers(&st);
			}
		} else if (index > 6) {
			if (strncmp(s, "ENDPCB",6) == 0 ) {
				printf("Found end of file\n");
			} else if (strncmp(s, "NETDEF",6) == 0 ) {
				printf("Found net definition\n");
			}
		} else if (index > 4) {
			if (strncmp(line, "COMP",4) == 0 ) {
				printf("Found component\n");
			}
		} else if (index > 2) {
			if (strncmp(s, "FT",2) == 0 ) {
				printf("Found free track\n");
				autotrax_parse_free_track(&st, FP);	
			} else if (strncmp(s, "FA",2) == 0 ) {
				printf("Found free arc\n");
				autotrax_parse_free_arc(&st, FP);
			} else if (strncmp(s, "FV",2) == 0 ) {
				printf("Found free via\n");
				autotrax_parse_free_via(&st, FP);
			} else if (strncmp(s, "FF",2) == 0 ) {
				printf("Found free fill\n");
				autotrax_parse_free_fill(&st, FP);
			} else if (strncmp(s, "FP",2) == 0 ) {
				printf("Found free pad\n");
				autotrax_parse_free_pad(&st, FP);
			} else if (strncmp(s, "FS",2) == 0 ) {
				printf("Found free String\n");
				autotrax_parse_free_text(&st, FP);
			}
		}
	} 
	fclose(FP);
	box = pcb_data_bbox(&boardSize, Ptr->Data);
	pcb_printf("Maximum X, Y dimensions (mil) of imported Protel autotrax layout: %.0ml, %.0ml\n", box->X2, box->Y2);
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
			if (strncmp(s, "PCB FILE 4", 10) == 0 || strncmp(s, "PCB FILE 5", 10) == 0) /* valid root */
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

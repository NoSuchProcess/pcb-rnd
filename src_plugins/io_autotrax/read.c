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

static int autotrax_parse_component_track(pcb_element_t *el, FILE *FP)
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
		pcb_element_line_new(el, X1, Y1, X2, Y2, Thickness);
		pcb_printf("\tnew free line in component created\n");
		return 0;
	} else {
		pcb_printf("Invalid component layer found : %d\n", PCBLayer);
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

static int autotrax_parse_component_pad(pcb_element_t *el, FILE *FP)
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

	pcb_coord_t moduleX, moduleY, X, Y, Xsize, Ysize, Thickness, Clearance, Mask, Drill; /* not sure what to do with mask */
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
	} else if ((Shape == 2  || Shape == 4) && PCBLayer == 6) { /* bottom layer */ 
		Flags = pcb_flag_make(PCB_FLAG_SQUARE | PCB_FLAG_ONSOLDER); /*actually a rectangle, but... */
	} else if (Shape == 3) { /*  && PCBLayer == 1) top layer*/ 
		Flags = pcb_flag_make(PCB_FLAG_OCTAGON);
	} else if (Shape == 3 && PCBLayer == 6) {  /*bottom layer */
		Flags = pcb_flag_make(PCB_FLAG_OCTAGON | PCB_FLAG_ONSOLDER); 
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
		if (Shape == 2 && Drill ==0) {/* is probably SMD */
			pcb_element_pad_new_rect(el, moduleX - Xsize/2, moduleY - Ysize/2, Xsize/2 + moduleX, Ysize/2 + moduleY, Clearance, 
				Clearance, padname, padname, Flags);
		} else {
			pcb_element_pin_new(el, X + moduleX, Y + moduleY, Thickness, Clearance, Clearance,  
				Drill, padname, padname, Flags);
		}
		pcb_printf("\tnew component pad/hole created; need to check connects\n");
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

static int autotrax_parse_component(read_state_t *st, FILE *FP)
{
	int i;
	int scaling = 100;
	int moduleDefined = 0;
	int PCBLayer = 0;
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
	char moduleName[33], moduleRefdes[33], moduleValue[33], pinName[33], coord[10];
	pcb_element_t *newModule;

	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	pcb_flag_t TextFlags = pcb_flag_make(0); /* start with something bland here */
	Clearance = PCB_MIL_TO_COORD(10); /* start with something bland here */

	int index = 0;
	int c;
	char *s, line[1024];

	while (!feof(FP) && ((c = fgetc(FP)) != '\n') && (index < 32)) {
		moduleRefdes[index] = c;
		index++;
	}
	moduleRefdes[index] = '\0';
	pcb_printf("Found component refdes : %s\n", moduleRefdes);

	index = 0;
	while (!feof(FP) && ((c = fgetc(FP)) != '\n') && (index < 32)) {
		moduleName[index] = c;
		index++;
	}
	moduleName[index] = '\0';
	pcb_printf("Found component name : %s\n", moduleName);

	index = 0;
	while (!feof(FP) && ((c = fgetc(FP)) != '\n') && (index < 32)) {
		moduleValue[index] = c;
		index++;
	}
	moduleValue[index] = '\0';
	pcb_printf("Found component description : %s\n", moduleValue);

/* with the header read, we now ignore the locations for the text fields... */
	for (index = 0; index < 2; index++) {
		while (!feof(FP) && (c = fgetc(FP) != '\n')) {
			printf("Skipping component text field coordinates, and XY data lines %d\n", index);
		}
	}

	index =0;
	while ((!feof(FP) && (c = fgetc(FP)) != '\n' && index < 29) || (c == '\n' && index == 0) ) {
		line[index] = c;
		index ++;
	}
	if (index > 27) {
		pcb_printf("error parsing component location line; too long\n");
		return -1;
	}
	line[index] = ' ';
	index++;
	line[index] = '\0';
	index = 0;
	printf("About to parse component location line: %s\n", line); 
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	val = strtod(coord, &end);
	if (*end != 0) {
		pcb_printf("error parsing component X location\n");
        	return -1;
	}
	moduleX = PCB_MIL_TO_COORD(val);
	pcb_printf("Found moduleX : %ml\n", moduleX);
	for (i = 0; line[index] != ' '; i++, index++) {
		coord[i] = line[index];
	}
	coord[i] = '\0';
	index++;
	moduleY = PCB_MIL_TO_COORD(atoi(coord));
	pcb_printf("Found moduleY : %ml\n", moduleY);
	
	printf("Have new module name and location, defining module/element %s\n", moduleName);
	newModule = pcb_element_new(st->PCB->Data, NULL, 
								pcb_font(st->PCB, 0, 1), Flags,
								moduleName, moduleRefdes, moduleValue,
								moduleX, moduleY, direction,
								refdesScaling, TextFlags,  pcb_false); /*pcb_flag_t TextFlags, pcb_bool uniqueName) */
	
	while (!feof(FP)) {
		index =0;
		while (!feof(FP) && (c = fgetc(FP)) != '\n' && index < 1023) {
			line[index] = c;
			index ++;
		}
		index++;
		line[index] = '\0';
		s = line;
		if (index > 7) {
			if (strncmp(line, "ENDCOMP", 7) == 0 ) {
				printf("Finished parsing component\n");
				if (moduleEmpty) { /* should try and use module empty function here */
					Thickness = PCB_MM_TO_COORD(0.200);
					pcb_element_line_new(newModule, moduleX, moduleY, moduleX+1, moduleY+1,
 Thickness);
					pcb_printf("\tEmpty Module!! 1nm line created at module centroid.\n");
				}
				pcb_element_bbox(st->PCB->Data, newModule, pcb_font(PCB, 0, 1));
				return 0;
			}
		} else if (index > 2) {
			if (strncmp(s, "CT",2) == 0 ) {
				printf("Found component track\n");
				autotrax_parse_component_track(&newModule, FP);
			} else if (strncmp(s, "CA",2) == 0 ) {
				printf("Found component arc\n");
			} else if (strncmp(s, "CV",2) == 0 ) {
				printf("Found component via\n");
/*				autotrax_parse_component_via(&newModule, FP);*/
			} else if (strncmp(s, "CF",2) == 0 ) {
				printf("Found component fill\n");
			} else if (strncmp(s, "CP",2) == 0 ) {
				printf("Found component pad\n");
				autotrax_parse_component_pad(&newModule, FP);
			} else if (strncmp(s, "CS",2) == 0 ) {
				printf("Found component String\n");
			}
		}
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
				autotrax_parse_component(&st, FP);
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

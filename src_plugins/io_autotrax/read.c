/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016, 2017, 2018 Tibor 'Igor2' Palinkas
 *  Copyright (C) 2016, 2017 Erich S. Heinzle
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
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
#include "netlist.h"
#include "polygon.h"
#include "misc_util.h" /* for distance calculations */
#include "conf_core.h"
#include "move.h"
#include "macro.h"
#include "safe_fs.h"
#include "rotate.h"
#include "actions.h"

#include "../src_plugins/lib_compat_help/pstk_compat.h"
#include "../src_plugins/lib_compat_help/pstk_help.h"
#include "../src_plugins/lib_compat_help/subc_help.h"

#define PCB REPLACE_THIS

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
		if (sattr->footprint == NULL)
			pcb_message(PCB_MSG_ERROR, "protel autotrax: not importing refdes=%s: no footprint specified\n", sattr->refdes);
		else
			pcb_actionl("ElementList", "Need", null_empty(sattr->refdes), null_empty(sattr->footprint), null_empty(sattr->value), NULL);
	}
	free(sattr->refdes);
	sattr->refdes = NULL;
	free(sattr->value);
	sattr->value = NULL;
	free(sattr->footprint);
	sattr->footprint = NULL;
}


/* the followinf read_state_t struct could go in a shared lib for parsers doing layer mapping */

typedef struct {
	pcb_board_t *pcb;
	const char *Filename;
	conf_role_t settings_dest;
	pcb_layer_id_t protel_to_stackup[13];
	int lineno;
	pcb_coord_t mask_clearance;
	pcb_coord_t copper_clearance;
	pcb_coord_t minimum_comp_pin_drill;
	int trax_version;
	int ignored_keepout_element;
	int ignored_layer_zero_element;
} read_state_t;

static int rdax_net(read_state_t *st, FILE *FP); /* describes netlists for the layout */

/* Look up (or even alloc) a layer on the board or in a subc, by autotrax layer number */
static pcb_layer_t *autotrax_get_layer(read_state_t *st, pcb_subc_t *subc, int autotrax_layer, const char *otyp)
{
	int lid, comb;
	pcb_layer_type_t lyt;

	if (autotrax_layer == 12) {
		st->ignored_keepout_element++;
		return NULL;
	}
	if (autotrax_layer == 0) {
		pcb_message(PCB_MSG_ERROR, "Ignored '%s' on easy/autotrax layer zero, %s:%d\n", otyp, st->Filename, st->lineno);
		st->ignored_layer_zero_element++;
		return NULL;
	}

	lid = st->protel_to_stackup[autotrax_layer];
	if (lid < 0) {
		pcb_message(PCB_MSG_ERROR, "Ignored '%s' on easy/autotrax unknown layer %d, %s:%d\n", otyp, autotrax_layer, st->Filename, st->lineno);
		return NULL;
	}

	if (subc == NULL)
		return &st->pcb->Data->Layer[lid];

	lyt = pcb_layer_flags(st->pcb, lid);
	comb = 0;
	return pcb_subc_get_layer(subc, lyt, comb, 1, st->pcb->Data->Layer[lid].name, pcb_true);
}

/* autotrax_free_text/component_text */
static int rdax_text(read_state_t *st, FILE *FP, pcb_subc_t *subc)
{
	int height_mil;
	int autotrax_layer = 0;
	char line[MAXREAD], *t;
	int success;
	int valid = 1;
	pcb_coord_t X, Y, linewidth;
	int scaling = 100;
	unsigned direction = 0; /* default is horizontal */
	pcb_flag_t Flags;
	pcb_layer_t *ly;
	pcb_layer_type_t lyt;

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
			Y = pcb_get_value_ex(argv[1], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			height_mil = pcb_get_value_ex(argv[2], NULL, NULL, NULL, NULL, &success);
			valid &= success;
			scaling = (100 * height_mil) / 60;
			direction = pcb_get_value_ex(argv[3], NULL, NULL, NULL, NULL, &success);
			direction = direction % 4; /* ignore mirroring */
			valid &= success;
			linewidth = pcb_get_value_ex(argv[4], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			autotrax_layer = pcb_get_value_ex(argv[5], NULL, NULL, NULL, NULL, &success);
			valid &= (success && (autotrax_layer > 0) && (autotrax_layer < 14));
			/* we ignore the user routed flag */
			qparse_free(argc, &argv);
		}
		else {
			pcb_message(PCB_MSG_ERROR, "Insufficient free string attribute fields, %s:%d\n", st->Filename, st->lineno);
			qparse_free(argc, &argv);
			return -1;
		}
	}

	if (!valid) {
		pcb_message(PCB_MSG_ERROR, "Failed to parse text attribute fields, %s:%d\n", st->Filename, st->lineno);
		return -1;
	}

	if (fgetline(line, sizeof(line), FP, st->lineno) == NULL) {
		pcb_message(PCB_MSG_ERROR, "Empty free string text field, %s:%d\n", st->Filename, st->lineno);
		strcpy(line, "(empty text field)");
	} /* this helps the parser fail more gracefully if excessive newlines, or empty text field */

	t = line;
	ltrim(t);
	rtrim(t); /*need to remove trailing '\r' to avoid rendering oddities */

	/* # TODO - ABOUT HERE, CAN DO ROTATION/DIRECTION CONVERSION */

	ly = autotrax_get_layer(st, subc, autotrax_layer, "text");
	if (ly == NULL)
		return 0;

	lyt = pcb_layer_flags_(ly);
	if (lyt & PCB_LYT_BOTTOM) Flags = pcb_flag_make(PCB_FLAG_ONSOLDER);
	else Flags = pcb_flag_make(0);

TODO("textrot: is there a real rotation angle available?")
	if (pcb_text_new(ly, pcb_font(st->pcb, 0, 1), X, Y, 90.0*direction, scaling, 0, t, Flags) != 0)
		return 1;
	return -1;

TODO(": do not use strlen() for this, decide where to move this code")
/*
		if (strlen(t) == 0) {
			pcb_message(PCB_MSG_ERROR, "Empty free string not placed on layout, %s:%d\n", st->Filename, st->lineno);
			return 0;
		}
*/
}

/* autotrax_pcb free_track/component_track */
static int rdax_track(read_state_t *st, FILE *FP, pcb_subc_t *subc)
{
	char line[MAXREAD];
	pcb_coord_t X1, Y1, X2, Y2, Thickness, Clearance;
	pcb_flag_t Flags = pcb_flag_make(0);
	int autotrax_layer = 0;
	int success;
	int valid = 1;
	pcb_layer_t *ly;

	Thickness = 0;
	Clearance = st->copper_clearance; /* start with sane default */

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
			Y1 = pcb_get_value_ex(argv[1], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			X2 = pcb_get_value_ex(argv[2], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			Y2 = pcb_get_value_ex(argv[3], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			Thickness = pcb_get_value_ex(argv[4], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			autotrax_layer = pcb_get_value_ex(argv[5], NULL, NULL, NULL, NULL, &success);
			valid &= (success && (autotrax_layer > 0) && (autotrax_layer < 14));
			/* we ignore the user routed flag */
			qparse_free(argc, &argv);
		}
		else {
			pcb_message(PCB_MSG_ERROR, "Insufficient track attribute fields, %s:%d\n", st->Filename, st->lineno);
			qparse_free(argc, &argv);
			return -1;
		}
	}

	ly = autotrax_get_layer(st, subc, autotrax_layer, "line");
	if (ly == NULL)
		return 0;

	if (pcb_line_new(ly, X1, Y1, X2, Y2, Thickness, Clearance, Flags) != NULL)
		return 1;
	return -1;
}

/* autotrax_pcb free arc and component arc parser */
static int rdax_arc(read_state_t *st, FILE *FP, pcb_subc_t *subc)
{
	char line[MAXREAD];
	int segments = 15; /* full circle by default */
	int success;
	int valid = 1;
	int autotrax_layer = 0;
	pcb_layer_t *ly;

	pcb_coord_t centreX, centreY, width, height, Thickness, Clearance, radius;
	pcb_angle_t start_angle = 0.0;
	pcb_angle_t delta = 360.0;

	pcb_flag_t Flags = pcb_flag_make(0); /* start with something bland here */
	pcb_layer_id_t PCB_layer;

	Thickness = 0;
	Clearance = st->copper_clearance; /* start with sane default */

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
			centreY = pcb_get_value_ex(argv[1], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			radius = pcb_get_value_ex(argv[2], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			segments = pcb_get_value_ex(argv[3], NULL, NULL, NULL, NULL, &success);
			valid &= success;
			Thickness = pcb_get_value_ex(argv[4], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			autotrax_layer = pcb_get_value_ex(argv[5], NULL, NULL, NULL, NULL, &success);
			PCB_layer = st->protel_to_stackup[(int)autotrax_layer];
			valid &= (success && (autotrax_layer > 0) && (autotrax_layer < 14));
			/* we ignore the user routed flag */
			qparse_free(argc, &argv);
		}
		else {
			qparse_free(argc, &argv);
			pcb_message(PCB_MSG_ERROR, "Insufficient arc attribute fields, %s:%d\n", st->Filename, st->lineno);
			return -1;
		}
	}

	if (!valid) {
		pcb_message(PCB_MSG_ERROR, "Unable to parse arc attribute fields, %s:%d\n", st->Filename, st->lineno);
		return -1;
	}


	width = height = radius;

/* we now need to decrypt the segments value. Some definitions will represent two arcs.
Bit 0 : RU quadrant
Bit 1 : LU quadrant
Bit 2 : LL quadrant
Bit 3 : LR quadrant

Since the board will be flipped, we should swap upper and lower quadrants

TODO: This needs further testing to ensure the refererence
document used reflects actual outputs from protel autotrax
*/
	if (segments == 10) { /* LU + RL quadrants */
		start_angle = 180.0;
		delta = 90.0;
		pcb_arc_new(&st->pcb->Data->Layer[PCB_layer], centreX, centreY, width, height, start_angle, delta, Thickness, Clearance, Flags, pcb_true);
		start_angle = 0.0;
	}
	else if (segments == 5) { /* RU + LL quadrants */
		start_angle = 270.0;
		delta = 90.0;
		pcb_arc_new(&st->pcb->Data->Layer[PCB_layer], centreX, centreY, width, height, start_angle, delta, Thickness, Clearance, Flags, pcb_true);
		start_angle = 90.0;
	}
	else if (segments >= 15) { /* whole circle */
		start_angle = 0.0;
		delta = 360.0;
	}
	else if (segments == 1) { /* RU quadrant */
		start_angle = 90.0;
		delta = 90.0;
	}
	else if (segments == 2) { /* LU quadrant */
		start_angle = 0.0;
		delta = 90.0;
	}
	else if (segments == 4) { /* LL quadrant */
		start_angle = 270.0;
		delta = 90.0;
	}
	else if (segments == 8) { /* RL quadrant */
		start_angle = 180.0;
		delta = 90.0;
	}
	else if (segments == 3) { /* Upper half */
		start_angle = 0.0;
		delta = 180.0;
	}
	else if (segments == 6) { /* Left half */
		start_angle = 270.0;
		delta = 180.0;
	}
	else if (segments == 12) { /* Lower half */
		start_angle = 180.0;
		delta = 180.0;
	}
	else if (segments == 9) { /* Right half */
		start_angle = 90.0;
		delta = 180.0;
	}
	else if (segments == 14) { /* not RUQ */
		start_angle = 180.0;
		delta = 270.0;
	}
	else if (segments == 13) { /* not LUQ */
		start_angle = 90.0;
		delta = 270.0;
	}
	else if (segments == 11) { /* not LLQ */
		start_angle = 0.0;
		delta = 270.0;
	}
	else if (segments == 7) { /* not RLQ */
		start_angle = 270.0;
		delta = 270.0;
	}

	ly = autotrax_get_layer(st, subc, autotrax_layer, "arc");
	if (ly == NULL)
		return 0;

	if (pcb_arc_new(ly, centreX, centreY, width, height, start_angle, delta, Thickness, Clearance, Flags, pcb_true) != 0)
		return 1;
	return -1;
}

/* autotrax_pcb via parser */
static int rdax_via(read_state_t *st, FILE *FP, pcb_subc_t *subc)
{
	char line[MAXREAD];
	char *name;
	int success;
	int valid = 1;
	pcb_data_t *data = (subc == NULL) ? st->pcb->Data : subc->data;
	pcb_pstk_t *ps;
	pcb_coord_t X, Y, Thickness, Clearance, Mask, Drill; /* not sure what to do with mask */

	Thickness = 0;
	Clearance = st->copper_clearance; /* start with sane default */

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
			Y = pcb_get_value_ex(argv[1], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			Thickness = pcb_get_value_ex(argv[2], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			Drill = pcb_get_value_ex(argv[3], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			qparse_free(argc, &argv);
		}
		else {
			qparse_free(argc, &argv);
			pcb_message(PCB_MSG_ERROR, "Insufficient via attribute fields, %s:%d\n", st->Filename, st->lineno);
			return -1;
		}
	}

	if (!valid) {
		pcb_message(PCB_MSG_ERROR, "Unable to parse via attribute fields, %s:%d\n", st->Filename, st->lineno);
		return -1;
	}

	Mask = Thickness + st->mask_clearance;

	ps = pcb_pstk_new_compat_via(data, -1, X, Y, Drill, Thickness, Clearance, Mask, PCB_PSTK_COMPAT_ROUND, 1);
	return ps != NULL;
}

/* autotrax_pcb free or component pad
x y X_size Y_size shape holesize pwr/gnd layer
padname
may need to think about hybrid outputs, like pad + hole, to match possible features in protel autotrax
*/
static int rdax_pad(read_state_t *st, FILE *FP, pcb_subc_t *subc, int component)
{
	char line[MAXREAD], *s;
	int Connects = 0;
	int Shape = 0;
	int autotrax_layer = 0;
	int valid = 1;
	int success;
	pcb_coord_t X, Y, X_size, Y_size, Thickness, Clearance, Mask, Drill;
	pcb_data_t *data = (subc == NULL) ? st->pcb->Data : subc->data;
	pcb_pstk_t *ps;

	Thickness = 0;
	Clearance = st->copper_clearance; /* start with sane default */

	Drill = PCB_MM_TO_COORD(0.300); /* start with something sane */

	if (fgetline(line, sizeof(line), FP, st->lineno) != NULL) {
		int argc;
		char **argv, *end;
		s = line;
		ltrim(s);
		rtrim(s);
		argc = qparse2(s, &argv, 0);
		if (argc <= 6) {
			qparse_free(argc, &argv);
			pcb_message(PCB_MSG_ERROR, "Insufficient pad attribute fields, %s:%d\n", st->Filename, st->lineno);
			return -1;
		}
		X = pcb_get_value_ex(argv[0], NULL, NULL, NULL, "mil", &success);
		valid &= success;
		Y = pcb_get_value_ex(argv[1], NULL, NULL, NULL, "mil", &success);
		valid &= success;
		X_size = pcb_get_value_ex(argv[2], NULL, NULL, NULL, "mil", &success);
		valid &= success;
		Y_size = pcb_get_value_ex(argv[3], NULL, NULL, NULL, "mil", &success);
		valid &= success;
		Shape = strtol(argv[4], &end, 10);
		if (*end != '\0') valid = 0;
		Drill = pcb_get_value_ex(argv[5], NULL, NULL, NULL, "mil", &success);
		valid &= success;
		Connects = strtol(argv[6], &end, 10);
		if (*end != '\0') valid = 0;
		/* which specifies GND or Power connection for pin/pad/via */
		autotrax_layer = strtol(argv[7], &end, 10);
		if (*end != '\0') valid = 0;
		valid &= (success && (autotrax_layer > 0) && (autotrax_layer < 14));
		qparse_free(argc, &argv);
	}

	if (!valid) {
		pcb_message(PCB_MSG_ERROR, "Insufficient pad attribute fields, %s:%d\n", st->Filename, st->lineno);
		return -1;
	}

/* now find name as string on next line and copy it */
TODO(": can not exit above if we need to read this line")
	if (fgetline(line, sizeof(line), FP, st->lineno) == NULL) {
		pcb_message(PCB_MSG_ERROR, "Error parsing pad text field line, %s:%d\n", st->Filename, st->lineno);
		return -1;
	}
	s = line;
	rtrim(s); /* avoid rendering oddities on layout, and netlist matching confusion */

	if (autotrax_layer == 11) return 1; /* layer 11: "board" layer - looks like an ignore */

	/* these features can have connections to GND and PWR
	   planes specified in protel autotrax (seems rare though)
	   so we warn the user is this is the case */
	switch (Connects) {
		case 1:
			pcb_message(PCB_MSG_ERROR, "pin clears PWR/GND, %s:%d.\n", st->Filename, st->lineno);
			break;
		case 2:
			pcb_message(PCB_MSG_ERROR, "pin requires relief to GND plane, %s:%d.\n", st->Filename, st->lineno);
			break;
		case 4:
			pcb_message(PCB_MSG_ERROR, "pin requires relief to PWR plane, %s:%d.\n", st->Filename, st->lineno);
			break;
		case 3:
			pcb_message(PCB_MSG_ERROR, "pin should connect to PWR plane, %s:%d.\n", st->Filename, st->lineno);
			break;
		case 5:
			pcb_message(PCB_MSG_ERROR, "pin should connect to GND plane, %s:%d.\n", st->Filename, st->lineno);
			break;
	}

	Thickness = MIN(X_size, Y_size);
	Mask = Thickness + st->mask_clearance;

	if (autotrax_layer == 0) {
		pcb_message(PCB_MSG_ERROR, "Ignored pad on easy/autotrax layer zero, %s:%d\n", st->Filename, st->lineno);
		st->ignored_layer_zero_element++;
		return 0;
	}

	/* Easytrax seems to specify zero drill for some component pins, and round free pins/pads */
	if (((st->trax_version == 5) && (X_size == Y_size) && component && (Drill == 0))
			|| ((st->trax_version == 5) && (X_size == Y_size) && (Shape == 1) && (Drill == 0))) {
		Drill = st->minimum_comp_pin_drill; /* ?may be better off using half the thickness for drill */
	}

/* currently ignore:
   shape:
    5 Cross Hair Target
    6 Moiro Target */
	if ((Shape == 5) || (Shape == 6)) {
		pcb_message(PCB_MSG_ERROR, "Unsupported FP target shape %d, %s:%d.\n", Shape, st->Filename, st->lineno);
		return 0;
	}

	{ /* pad not within element, i.e. a free pad/pin/via */
		pcb_pstk_shape_t sh[8];
		int n;

		memset(sh, 0, sizeof(sh));
		sh[0].layer_mask = PCB_LYT_PASTE; sh[0].comb = PCB_LYC_AUTO;
		sh[1].layer_mask = PCB_LYT_MASK; sh[1].comb = PCB_LYC_SUB | PCB_LYC_AUTO;
		sh[2].layer_mask = PCB_LYT_COPPER;
		sh[3].layer_mask = PCB_LYT_COPPER;
		sh[4].layer_mask = PCB_LYT_COPPER;
		sh[5].layer_mask = PCB_LYT_MASK; sh[5].comb = PCB_LYC_SUB | PCB_LYC_AUTO;
		sh[6].layer_mask = PCB_LYT_PASTE; sh[6].comb = PCB_LYC_AUTO;

		switch(autotrax_layer) {
			case 1:
				for(n = 0; n < 3; n++)
					sh[n].layer_mask |= PCB_LYT_TOP;
				sh[3].layer_mask = 0;
				break;
			case 6:
				for(n = 0; n < 3; n++)
					sh[n].layer_mask |= PCB_LYT_BOTTOM;
				sh[3].layer_mask = 0;
				break;
			case 13:
				for(n = 0; n < 3; n++)
					sh[n].layer_mask |= PCB_LYT_TOP;
				sh[3].layer_mask |= PCB_LYT_INTERN;
				for(n = 4; n < 7; n++)
					sh[n].layer_mask |= PCB_LYT_BOTTOM;
				break;
			default:
				pcb_message(PCB_MSG_ERROR, "Unsupported FP layer: %d, %s:%d.\n", autotrax_layer, st->Filename, st->lineno);
				return 0;
		}

		switch(Shape) {
			case 1: /* round */
				for(n = 0; n < 7; n++) {
					pcb_coord_t clr = (sh[n].layer_mask & PCB_LYT_MASK) ? Clearance : 0;
					if (sh[n].layer_mask == 0) break;
					pcb_shape_oval(&sh[n], X_size+clr, Y_size+clr);
				}
				break;
			case 2: /* rect */
			case 4: /* round-rect - for now */
TODO(": generate round-rect")
				for(n = 0; n < 7; n++) {
					pcb_coord_t clr = (sh[n].layer_mask & PCB_LYT_MASK) ? Clearance : 0;
					if (sh[n].layer_mask == 0) break;
					pcb_shape_rect(&sh[n], X_size+clr, Y_size+clr);
				}
				break;
			case 3: /* octa */
TODO(": generate octa")
			default:
				pcb_message(PCB_MSG_ERROR, "Unsupported FP shape: %d, %s:%d.\n", Shape, st->Filename, st->lineno);
				return 0;
		}
		ps = pcb_pstk_new_from_shape(data, X, Y, Drill, 1, Clearance, sh);
		if (ps == NULL)
			pcb_message(PCB_MSG_ERROR, "Failed to convert FP to padstack, %s:%d.\n", st->Filename, st->lineno);

		return (ps != NULL);
	}
}

/* protel autorax free fill (rectangular pour) parser - the closest thing protel autotrax has to a polygon */
static int rdax_fill(read_state_t *st, FILE *FP, pcb_subc_t *subc)
{
	int success;
	int valid = 1;
	int autotrax_layer = 0;
	char line[MAXREAD];
	pcb_poly_t *polygon = NULL;
	pcb_flag_t flags = pcb_flag_make(PCB_FLAG_CLEARPOLY);
	pcb_coord_t X1, Y1, X2, Y2, Clearance;
	pcb_layer_t *ly;

	Clearance = st->copper_clearance; /* start with sane default */

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
			Y1 = pcb_get_value_ex(argv[1], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			X2 = pcb_get_value_ex(argv[2], NULL, NULL, NULL, "mil", &success);
			valid &= success;
			Y2 = pcb_get_value_ex(argv[3], NULL, NULL, NULL, "mil", &success);
			valid &= success;
TODO(": do not use get_value_ex for plain integers (revise the whole file for this)")
			autotrax_layer = pcb_get_value_ex(argv[4], NULL, NULL, NULL, NULL, &success);
			valid &= (success && (autotrax_layer > 0) && (autotrax_layer < 14));
			qparse_free(argc, &argv);
		}
		else {
			qparse_free(argc, &argv);
			pcb_message(PCB_MSG_ERROR, "Insufficient fill attribute fields, %s:%d\n", st->Filename, st->lineno);
			return -1;
		}
	}

	if (!valid) {
		pcb_message(PCB_MSG_ERROR, "Fill attribute fields unable to be parsed, %s:%d\n", st->Filename, st->lineno);
		return -1;
	}

TODO(": figure if autotrax really converts layer 1 and 6 polygons to pads")
	if ((subc == NULL) || ((autotrax_layer != 1) && (autotrax_layer != 6))) {
		ly = autotrax_get_layer(st, subc, autotrax_layer, "polygon");
		if (ly == NULL)
			return 0;

		polygon = pcb_poly_new(ly, 0, flags);
		if (polygon == NULL) {
			pcb_message(PCB_MSG_ERROR, "Failed to allocate polygon, %s:%d\n", st->Filename, st->lineno);
			return -1;
		}

		pcb_poly_point_new(polygon, X1, Y1);
		pcb_poly_point_new(polygon, X2, Y1);
		pcb_poly_point_new(polygon, X2, Y2);
		pcb_poly_point_new(polygon, X1, Y2);
		pcb_add_poly_on_layer(ly, polygon);
		if (subc == NULL)
			pcb_poly_init_clip(st->pcb->Data, ly, polygon);
		return 1;
	}
	else {
		pcb_coord_t w = X2-X1, h = Y2-Y1;
		pcb_pstk_shape_t sh[4];
		pcb_layer_type_t side;
		int n;

		switch(autotrax_layer) {
			case 1: side = PCB_LYT_TOP; break;
			case 6: side = PCB_LYT_BOTTOM; break;
		}

		memset(sh, 0, sizeof(sh));
		sh[0].layer_mask = side | PCB_LYT_PASTE; sh[0].comb = PCB_LYC_AUTO;
		sh[1].layer_mask = side | PCB_LYT_MASK; sh[1].comb = PCB_LYC_SUB | PCB_LYC_AUTO;
		sh[2].layer_mask = side | PCB_LYT_COPPER;
		pcb_shape_rect(&sh[0], w, h);
		pcb_shape_rect(&sh[1], w+Clearance, h+Clearance);
		pcb_shape_rect(&sh[2], w, h);
		if (pcb_pstk_new_from_shape(subc->data, (X1+X2)/2, (Y1+Y2)/2, 0, 0, Clearance, sh) != NULL)
			return 1;
		pcb_message(PCB_MSG_ERROR, "SMD pad: filed to convert from polygon, %s:%d\n", st->Filename, st->lineno);
	}

	return -1;
}

static pcb_layer_id_t autotrax_reg_layer(read_state_t *st, const char *autotrax_layer, unsigned int mask)
{
	pcb_layer_id_t id;
	if (pcb_layer_list(st->pcb, mask, &id, 1) != 1) {
		pcb_layergrp_id_t gid;
		pcb_layergrp_list(st->pcb, mask, &gid, 1);
		id = pcb_layer_create(st->pcb, gid, autotrax_layer);
	}
	return id;
}

/* create a set of default layers, since protel has a static stackup */
static int autotrax_create_layers(read_state_t *st)
{

	pcb_layergrp_t *g;
	pcb_layer_id_t id;

	pcb_layer_group_setup_default(st->pcb);

	st->protel_to_stackup[7] = autotrax_reg_layer(st, "top silk", PCB_LYT_SILK | PCB_LYT_TOP);
	st->protel_to_stackup[8] = autotrax_reg_layer(st, "bottom silk", PCB_LYT_SILK | PCB_LYT_BOTTOM);

	st->protel_to_stackup[1] = autotrax_reg_layer(st, "top copper", PCB_LYT_COPPER | PCB_LYT_TOP);
	st->protel_to_stackup[6] = autotrax_reg_layer(st, "bottom copper", PCB_LYT_COPPER | PCB_LYT_BOTTOM);

	if (pcb_layer_list(st->pcb, PCB_LYT_SILK | PCB_LYT_TOP, &id, 1) == 1) {
		pcb_layergrp_id_t gid;
		pcb_layergrp_list(st->pcb, PCB_LYT_SILK | PCB_LYT_TOP, &gid, 1);
		st->protel_to_stackup[11] = pcb_layer_create(st->pcb, gid, "Board"); /* != outline, cutouts */
		pcb_layergrp_list(st->pcb, PCB_LYT_SILK | PCB_LYT_TOP, &gid, 1);
		st->protel_to_stackup[13] = pcb_layer_create(st->pcb, gid, "Multi");
	}
	else {
		pcb_message(PCB_MSG_ERROR, "Unable to create Keepout, Multi layers in default top silk group\n");
	}

	g = pcb_get_grp_new_intern(st->pcb, -1);
	st->protel_to_stackup[2] = pcb_layer_create(st->pcb, g - st->pcb->LayerGroups.grp, "Mid1");

	g = pcb_get_grp_new_intern(st->pcb, -1);
	st->protel_to_stackup[3] = pcb_layer_create(st->pcb, g - st->pcb->LayerGroups.grp, "Mid2");

	g = pcb_get_grp_new_intern(st->pcb, -1);
	st->protel_to_stackup[4] = pcb_layer_create(st->pcb, g - st->pcb->LayerGroups.grp, "Mid3");

	g = pcb_get_grp_new_intern(st->pcb, -1);
	st->protel_to_stackup[5] = pcb_layer_create(st->pcb, g - st->pcb->LayerGroups.grp, "Mid4");

	g = pcb_get_grp_new_intern(st->pcb, -1);
	st->protel_to_stackup[9] = pcb_layer_create(st->pcb, g - st->pcb->LayerGroups.grp, "GND");

	g = pcb_get_grp_new_intern(st->pcb, -1);
	st->protel_to_stackup[10] = pcb_layer_create(st->pcb, g - st->pcb->LayerGroups.grp, "Power");

	g = pcb_get_grp_new_intern(st->pcb, -1);
	g->name = pcb_strdup("outline"); /* equivalent to keepout = layer 12 in autotrax */
	g->ltype = PCB_LYT_BOUNDARY; /* and includes cutouts */
	pcb_layergrp_set_purpose__(g, pcb_strdup("uroute"));
	st->protel_to_stackup[12] = autotrax_reg_layer(st, "outline", PCB_LYT_BOUNDARY);

	pcb_layergrp_inhibit_dec();

	return 0;
}

/* autotrax_pcb  rdax_net ;   used to read net descriptions for the entire layout */
static int rdax_net(read_state_t *st, FILE *FP)
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

	if (fgetline(line, sizeof(line), FP, st->lineno) != NULL) {
		s = line;
		rtrim(s);
		netname = pcb_strdup(line);
	}
	else {
		pcb_message(PCB_MSG_ERROR, "Empty netlist name found, %s:%d\n", st->Filename, st->lineno);
		return -1;
	}
	fgetline(line, sizeof(line), FP, st->lineno);
	s = line;
	rtrim(s);
	while(!feof(FP) && !endpcb && in_net) {
		fgetline(line, sizeof(line), FP, st->lineno);
		if (strncmp(line, "[", 1) == 0) {
			in_comp = 1;
			while(in_comp) {
				if (fgetline(line, sizeof(line), FP, st->lineno) == NULL) {
					pcb_message(PCB_MSG_ERROR, "Empty line in netlist COMP, %s:%d\n", st->Filename, st->lineno);
				}
				else {
					if (fgetline(line, sizeof(line), FP, st->lineno) == NULL) {
						pcb_message(PCB_MSG_ERROR, "Empty netlist REFDES, %s:%d\n", st->Filename, st->lineno);
					}
					else {
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
					}
					else {
						s = line;
						rtrim(s);
						free(sattr.footprint);
						sattr.footprint = pcb_strdup(line);
					}
					if (fgetline(line, sizeof(line), FP, st->lineno) == NULL) {
						free(sattr.value);
						sattr.value = pcb_strdup("value");
					}
					else {
						s = line;
						rtrim(s);
						free(sattr.value);
						sattr.value = pcb_strdup(line);
					}
				}
				while(fgetline(line, sizeof(line), FP, st->lineno) == NULL) {
					/* clear empty lines in COMP definition */
				}
				if (strncmp(line, "]", 1) == 0) {
					in_comp = 0;
				}
			}
		}
		else if (strncmp(line, "(", 1) == 0) {
			in_net = 1;
			while(in_net || in_node_table) {
				while(fgetline(line, sizeof(line), FP, st->lineno) == NULL) {
					/* skip empty lines */
				}
				if (strncmp(line, ")", 1) == 0) {
					in_net = 0;
				}
				else if (strncmp(line, "{", 1) == 0) {
					in_node_table = 1;
					in_net = 0;
				}
				else if (strncmp(line, "}", 1) == 0) {
					in_node_table = 0;
				}
				else if (!in_node_table) {
					s = line;
					rtrim(s);
					if (line != NULL && netname != NULL) {
						pcb_actionl("Netlist", "Add", netname, line, NULL);
					}
				}
			}
		}
		else if (length >= 6 && strncmp(line, "ENDPCB", 6) == 0) {
			pcb_message(PCB_MSG_ERROR, "End of protel Autotrax file found in netlist section?!, %s:%d\n", st->Filename, st->lineno);
			endpcb = 1; /* if we get here, something went wrong */
		}
	}
	sym_flush(&sattr);

	return 0;
}

/* protel autotrax component definition parser */
/* no mark() or location as such it seems */
static int rdax_component(read_state_t *st, FILE *FP)
{
	int success;
	int valid = 1;
	int refdes_scaling = 100;
	pcb_coord_t module_X, module_Y;
	unsigned direction = 0; /* default is horizontal */
	char module_name[MAXREAD], module_refdes[MAXREAD], module_value[MAXREAD];
	pcb_subc_t *new_module;

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
	if (fgetline(module_name, sizeof(module_name), FP, st->lineno) == NULL) {
		strcpy(module_name, "unknown");
	}
	s = module_name;
	rtrim(s); /* avoid rendering oddities on layout */
	if (fgetline(module_value, sizeof(module_value), FP, st->lineno) == NULL) {
		strcpy(module_value, "unknown");
	}
	s = module_value;
	rtrim(s); /* avoid rendering oddities on layout */

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
			module_Y = pcb_get_value_ex(argv[1], NULL, NULL, NULL, "mil", &success);
			valid &= success;
TODO(": load placement status and apply PCB_FLAG_LOCK if needed")
			qparse_free(argc, &argv);
		}
		else {
			qparse_free(argc, &argv);
			pcb_message(PCB_MSG_ERROR, "Insufficient COMP attribute fields, %s:%d\n", st->Filename, st->lineno);
			return -1;
		}
	}

	if (!valid) {
		pcb_message(PCB_MSG_ERROR, "Unable to parse COMP attributes, %s:%d\n", st->Filename, st->lineno);
		return -1;
	}

	new_module = pcb_subc_alloc();
	pcb_subc_create_aux(new_module, module_X, module_Y, 0.0, 0);
	pcb_attribute_put(&new_module->Attributes, "refdes", "A1");
	pcb_subc_reg(st->pcb->Data, new_module);
	pcb_subc_bind_globals(st->pcb, new_module);

	nonempty = 0;
	while(!feof(FP)) {
		if (fgetline(line, sizeof(line), FP, st->lineno) == NULL) {
			fgetline(line, sizeof(line), FP, st->lineno);
		}
		s = line;
		length = strlen(line);
		if (length >= 7) {
			if (strncmp(line, "ENDCOMP", 7) == 0) {
				if (nonempty) { /* could try and use module empty function here */
					pcb_subc_bbox(new_module);
					break;
				}
				else {
					pcb_message(PCB_MSG_ERROR, "Empty module/COMP found, not added to layout, %s:%d\n", st->Filename, st->lineno);
TODO("TODO safely free new_module")
					return 0;
				}
			}
		}
		else if (length >= 2) {
TODO(": this does not handle return -1")
			if (strncmp(s, "CT", 2) == 0) {
				nonempty |= rdax_track(st, FP, new_module);
			}
			else if (strncmp(s, "CA", 2) == 0) {
				nonempty |= rdax_arc(st, FP, new_module);
			}
			else if (strncmp(s, "CV", 2) == 0) {
				nonempty |= rdax_via(st, FP, new_module);
			}
			else if (strncmp(s, "CF", 2) == 0) {
				nonempty |= rdax_fill(st, FP, new_module);
			}
			else if (strncmp(s, "CP", 2) == 0) {
				nonempty |= rdax_pad(st, FP, new_module, 1); /* flag in COMP */
			}
			else if (strncmp(s, "CS", 2) == 0) {
				nonempty |= rdax_text(st, FP, new_module);
			}
		}
	}
	pcb_subc_bbox(new_module);
	if (st->pcb->Data->subc_tree == NULL)
		st->pcb->Data->subc_tree = pcb_r_create_tree();
	pcb_r_insert_entry(st->pcb->Data->subc_tree, (pcb_box_t *)new_module);
	pcb_subc_rebind(st->pcb, new_module);

	return 0;
}


int io_autotrax_read_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, conf_role_t settings_dest)
{
	int readres = 0;
	pcb_box_t board_size, *box;
	read_state_t st;
	FILE *FP;
	pcb_subc_t *subc = NULL;
	int length = 0;
	int finished = 0;
	int netdefs = 0;
	char line[1024];
	char *s;

	FP = pcb_fopen(Filename, "r");
	if (FP == NULL)
		return -1;

	/* set up the parse context */
	memset(&st, 0, sizeof(st));
	st.pcb = Ptr;
	st.Filename = Filename;
	st.settings_dest = settings_dest;
	st.lineno = 0;
	st.mask_clearance = st.copper_clearance = PCB_MIL_TO_COORD(10); /* sensible default values */
	st.minimum_comp_pin_drill = PCB_MIL_TO_COORD(30); /* Easytrax PCB FILE 5 uses zero in COMP pins */
	st.trax_version = 4; /* assume autotrax, not easytrax */
	st.ignored_keepout_element = 0;
	st.ignored_layer_zero_element = 0;

	while(!feof(FP) && !finished) {
		if (fgetline(line, sizeof(line), FP, st.lineno) == NULL) {
			fgetline(line, sizeof(line), FP, st.lineno);
		}
		s = line;
		rtrim(s);
		length = strlen(line);
		if (length >= 10) {
			if (strncmp(line, "PCB FILE 4", 10) == 0) {
				st.trax_version = 4;
				autotrax_create_layers(&st);
			}
			else if (strncmp(line, "PCB FILE 5", 10) == 0) {
				st.trax_version = 5;
				autotrax_create_layers(&st);
			}
		}
		else if (length >= 6) {
			if (strncmp(s, "ENDPCB", 6) == 0) {
				finished = 1;
			}
			else if (strncmp(s, "NETDEF", 6) == 0) {
				if (netdefs == 0) {
					pcb_actionl("ElementList", "start", NULL);
					pcb_actionl("Netlist", "Freeze", NULL);
					pcb_actionl("Netlist", "Clear", NULL);
				}
				netdefs |= 1;
				rdax_net(&st, FP);
			}
		}
		else if (length >= 4) {
			if (strncmp(line, "COMP", 4) == 0) {
				rdax_component(&st, FP);
			}
		}
		else if (length >= 2) {
			if (strncmp(s, "FT", 2) == 0)      rdax_track(&st, FP, subc);
			else if (strncmp(s, "FA", 2) == 0) rdax_arc(&st, FP, subc);
			else if (strncmp(s, "FV", 2) == 0) rdax_via(&st, FP, subc);
			else if (strncmp(s, "FF", 2) == 0) rdax_fill(&st, FP, subc);
			else if (strncmp(s, "FP", 2) == 0) rdax_pad(&st, FP, subc, 0); /* flag not in a component */
			else if (strncmp(s, "FS", 2) == 0) rdax_text(&st, FP, subc);
		}
	}
	if (netdefs) {
		pcb_actionl("Netlist", "Sort", NULL);
		pcb_actionl("Netlist", "Thaw", NULL);
		pcb_actionl("ElementList", "Done", NULL);
	}
	fclose(FP);
	box = pcb_data_bbox(&board_size, Ptr->Data, pcb_false);
	if (st.ignored_keepout_element) {
		pcb_message(PCB_MSG_ERROR, "Ignored %d keepout track(s) on auto/easytrax layer 12\n", st.ignored_keepout_element);
	}
	if (st.ignored_layer_zero_element) {
		pcb_message(PCB_MSG_ERROR, "Ignored %d auto/easytrax layer zero feature(s)\n", st.ignored_layer_zero_element);
	}

	if (box != NULL) {
		Ptr->MaxWidth = box->X2;
		Ptr->MaxHeight = box->Y2;
	}
	else
		pcb_message(PCB_MSG_ERROR, "Can not determine board extents - empty board?\n");

	/* we now flip the board about the X-axis, to invert the Y coords used by autotrax */
	pcb_undo_freeze_add();
	pcb_data_mirror(Ptr->Data, 0, PCB_TXM_COORD, 0);
	pcb_undo_unfreeze_add();

	/* still not sure if this is required: */
	pcb_layer_auto_fixup(Ptr);

	pcb_board_normalize(Ptr);
	pcb_layer_colors_from_conf(Ptr, 1);

	return readres;
}

int io_autotrax_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
{
	char line[1024], *s;

	if (typ != PCB_IOT_PCB)
		return 0; /* support only boards for now */

	while(!(feof(f))) {
		if (fgets(line, sizeof(line), f) != NULL) {
			s = line;
			while(isspace(*s))
				s++; /* strip leading whitespace */
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

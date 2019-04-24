/*
 * read hyperlynx files
 * Copyright 2016 Koen De Vleeschauwer.
 * Copyright (C) 2018 Tibor 'Igor2' Palinkas
 *
 * This file is part of pcb-rnd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define MAX_STRING 256

#include "parser.h"
#include "hyp_l.h"
#include "hyp_y.h"
#include "error.h"
#include "pcb-printf.h"
#include "flag_str.h"
#include "polygon.h"
#include "board.h"
#include "layer.h"
#include "data.h"
#include "search.h"
#include "rotate.h"
#include "actions.h"
#include "plug_io.h"
#include "compat_misc.h"
#include "safe_fs.h"
#include "../src_plugins/lib_compat_help/pstk_help.h"
#include "../src_plugins/lib_compat_help/subc_help.h"
#include <string.h>

#define MASK_OFFS  +PCB_MIL_TO_COORD(10)
#define PASTE_OFFS -PCB_MIL_TO_COORD(10)


#undef min
#undef max
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

/*
 * the board is shared between all routines.
 */

pcb_data_t *hyp_dest;

/*
 * board outline is a linked list of arcs and line segments.
 */

typedef struct outline_s {
	pcb_coord_t x1;
	pcb_coord_t y1;
	pcb_coord_t x2;
	pcb_coord_t y2;
	pcb_coord_t xc;
	pcb_coord_t yc;
	pcb_coord_t r;
	pcb_bool_t is_arc;						/* arc or line */
	pcb_bool_t used;							/* already included in outline */
	struct outline_s *next;
} outline_t;

outline_t *outline_head;
outline_t *outline_tail;

void hyp_set_origin();					/* set origin so all coordinates are positive */
void hyp_perimeter();						/* add board outline to pcb */
void hyp_draw_polygons();				/* add all hyperlynx polygons to pcb */
static void hyp_subcs_fin(void);
void hyp_resize_board();				/* resize board to fit outline */
pcb_bool_t hyp_is_bottom_layer(char *);	/* true if bottom layer */

/* layer creation */
int layer_count;
pcb_layer_id_t top_layer_id, bottom_layer_id;
void hyp_reset_layers();				/* reset layer stack to minimun */
pcb_layer_id_t hyp_create_layer(char *lname);	/* create new copper layer at bottom of stack */

int hyp_debug;									/* logging on/off switch */

/* Physical constants */
double inches;									/* inches to m */
double copper_imperial_weight;	/* metal thickness in ounces/ft2 */
double copper_metric_weight;		/* metal thickness in grams/cm2 */
double copper_bulk_resistivity;	/* metal resistivity in ohm meter */
double copper_temperature_coefficient;	/* temperature coefficient of bulk resistivity */
double fr4_epsilon_r;						/* dielectric constant of substrate */
double fr4_loss_tangent;				/* loss tangent of substrate */
double conformal_epsilon_r;			/* dielectric constant of conformal coating */

/* Hyperlynx UNIT and OPTIONS */
double unit;										/* conversion factor: pcb length units to meters */
double metal_thickness_unit;		/* conversion factor: metal thickness to meters */

pcb_bool use_die_for_metal;			/* use dielectric constant and loss tangent of dielectric for metal layers */

pcb_coord_t board_clearance;		/* distance between PLANE polygon and copper of different nets; -1 if not set */

/* stackup */
pcb_bool layer_is_plane[PCB_MAX_LAYER];	/* whether layer is signal or plane layer */
pcb_coord_t layer_clearance[PCB_MAX_LAYER];	/* separation between fill copper and signals on layer */

/* net */
char *net_name;									/* name of current copper */
pcb_coord_t net_clearance;			/* distance between PLANE polygon and net copper */

/* netlist */
void hyp_netlist_begin();
void hyp_netlist_add(char *device_name, char *pin_name);
void hyp_netlist_end();

/* devices */

typedef struct device_s {
	char *ref;
	char *name;										/* optional */
	char *value;									/* optional */
	char *layer_name;
	pcb_subc_t *subc;
	struct device_s *next;
} device_t;

device_t *device_head;
int unknown_device_number;
int unknown_pin_number;

/* padstack */

typedef struct padstack_element_s {
	char *layer_name;
	int pad_shape;
	pcb_coord_t pad_sx;
	pcb_coord_t pad_sy;
	double pad_angle;
	pcb_coord_t thermal_clear_sx;
	pcb_coord_t thermal_clear_sy;
	double thermal_clear_angle;
	pad_type_enum pad_type;
	struct padstack_element_s *next;
} padstack_element_t;

padstack_element_t *current_pstk_element;

typedef struct padstack_s {
	char *name;
	pcb_coord_t drill_size;
	struct padstack_element_s *padstack;
	struct padstack_s *next;
} padstack_t;

padstack_t *padstack_head;
padstack_t *current_pstk;

/* polygons */

/* 
 * a hyperlynx polygon is a sequence of line and arc segments.
 *
 * if is_arc is false, draw a line to (x1, y1).
 * if is_arc is true, draw an arc from (x1, y1) to (x2, y2) with center (xc, yc).
 *
 * is_first marks the first vertex of a contour. 
 * The first contour is the outer edge of the polygon; the following contours are holes.
 */

typedef struct hyp_vertex_s {
	pcb_coord_t x1;
	pcb_coord_t y1;
	pcb_coord_t x2;
	pcb_coord_t y2;
	pcb_coord_t xc;
	pcb_coord_t yc;
	pcb_coord_t r;
	pcb_bool_t is_first;					/* true if first vertex of contour */
	pcb_bool_t is_arc;						/* true if arc */
	struct hyp_vertex_s *next;
} hyp_vertex_t;

	/* linked list to store correspondence between hyperlynx polygon id's and pcb polygons. */
typedef struct hyp_polygon_s {
	int hyp_poly_id;							/* hyperlynx polygon/polyline id */
	polygon_type_enum hyp_poly_type;	/* pour, copper, ... */
	pcb_bool_t is_polygon;				/* true if polygon, false if polyline */
	char *layer_name;							/* layer of polygon */
	pcb_coord_t line_width;				/* line width of edge */
	pcb_coord_t clearance;				/* clearance with other copper */
	hyp_vertex_t *vertex;					/* polygon contours as linked list of vertices */
	struct hyp_polygon_s *next;
} hyp_polygon_t;

hyp_polygon_t *polygon_head = NULL;	/* linked list of all polygons */
hyp_vertex_t *current_vertex = NULL;	/* keeps track where to add next polygon segment when parsing polygons */

/* origin. Chosen so all coordinates are positive. */
pcb_coord_t origin_x;
pcb_coord_t origin_y;

/*
 * Conversion from hyperlynx to pcb_coord_t
 */

/* meter to pcb_coord_t */

static pcb_coord_t m2coord(double m)
{
	return ((pcb_coord_t) PCB_MM_TO_COORD(1000.0 * m));
}

/* xy coordinates to pcb_coord_t, without offset */

static pcb_coord_t xy2coord(double f)
{
	return (m2coord(unit * f));
}


/* x coordinates to pcb_coord_t, with offset */

static pcb_coord_t x2coord(double f)
{
	return (m2coord(unit * f) - origin_x);
}

/* y coordinates to pcb_coord_t, with offset */

static pcb_coord_t y2coord(double f)
{
	return (origin_y - m2coord(unit * f));
}

/* z coordinates to pcb_coord_t. No offset needed. */

static pcb_coord_t z2coord(double f)
{
	return (m2coord(metal_thickness_unit * f));
}

/*
 * initialize physical constants 
 */

void hyp_init(void)
{
	int n;

	unit = 1;
	metal_thickness_unit = 1;
	use_die_for_metal = pcb_false;

	inches = 0.0254;							/* inches to m */
	copper_imperial_weight = 1.341;	/* metal thickness in ounces/ft2. 1 oz/ft2 copper = 1.341 mil */
	copper_metric_weight = 0.1116;	/* metal thickness in grams/cm2. 1 gr/cm2 copper = 0.1116 cm */
	copper_bulk_resistivity = 1.724e-8;
	copper_temperature_coefficient = 0.00393;
	fr4_epsilon_r = 4.3;
	fr4_loss_tangent = 0.020;
	conformal_epsilon_r = 3.3;		/* dielectric constant of conformal layer */
	board_clearance = -1;					/* distance between PLANE polygon and copper of different nets; -1 if not set */

	/* empty board outline */
	outline_head = NULL;
	outline_tail = NULL;

	/* clear layer info */
	for (n = 1; n < PCB_MAX_LAYER; n++) {
		layer_is_plane[n] = pcb_false;	/* signal layer */
		layer_clearance[n] = -1;		/* no separation between fill copper and signals on layer */
	}
	layer_count = 0;
	top_layer_id = -1;
	bottom_layer_id = -1;

	/* clear padstack */
	padstack_head = NULL;
	current_pstk = NULL;
	current_pstk_element = NULL;

	/* clear devices */
	device_head = NULL;
	unknown_device_number = 0;
	unknown_pin_number = 0;

	/* clear polygon data */
	polygon_head = NULL;
	current_vertex = NULL;

	/* set origin */
	origin_x = 0;
	origin_y = 0;

	return;
}

/* 
 * called by pcb-rnd to load hyperlynx file 
 */

int hyp_parse(pcb_data_t * dest, const char *fname, int debug)
{
	int retval;

	/* set debug levels */
	hyyset_debug(debug > 2);			/* switch flex logging on */
	hyydebug = (debug > 1);				/* switch bison logging on */
	hyp_debug = (debug > 0);			/* switch hyperlynx logging on */

	hyp_init();

	hyp_netlist_begin();

	hyp_reset_layers();

	/* set shared board */
	hyp_dest = dest;

	/* reset line number to first line */
	hyyset_lineno(1);

	/* parse hyperlynx file */
	hyyin = pcb_fopen(fname, "r");
	if (hyyin == NULL)
		return 1;
	retval = hyyparse();
	fclose(hyyin);

	/* set up polygons */
	hyp_draw_polygons();

	/* postprocess/finalize subcircuits */
	hyp_subcs_fin();

	/* add board outline last */
	hyp_perimeter();

	/* clear */
	hyp_dest = NULL;

	hyp_netlist_end();

	return retval;
}

/* print error message */
void hyp_error(const char *msg)
{
	enum pcb_message_level level;

	if (strstr(msg, "warning"))
		level = PCB_MSG_WARNING;
	else
		level = PCB_MSG_ERROR;

	pcb_message(level, "line %d: %s at '%s'\n", hyylineno, msg, hyytext);
}

/*
 * find padstack by name 
 */
TODO(": use a hash instead")
padstack_t *hyp_pstk_by_name(char *padstack_name)
{
	padstack_t *i;
	for (i = padstack_head; i != NULL; i = i->next)
		if (strcmp(i->name, padstack_name) == 0)
			return i;
	return NULL;
}

/*
 * netlist - assign pin or pad to net
 */

void hyp_netlist_begin()
{
	/* clear netlist */
	pcb_actionl("Netlist", "Freeze", NULL);
	pcb_actionl("Netlist", "Clear", NULL);
	return;
}

/* add pin to net */
void hyp_netlist_add(char *device_name, char *pin_name)
{
	char conn[MAX_STRING];

	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "netlist net: '%s' device: '%s' pin: '%s'\n", net_name, device_name, pin_name);

	if ((net_name != NULL) && (device_name != NULL) && (pin_name != NULL)) {
		pcb_snprintf(conn, sizeof(conn), "%s-%s", device_name, pin_name);
		pcb_actionl("Netlist", "Add", net_name, conn, NULL);
	}
	return;
}

void hyp_netlist_end()
{
	/* sort netlist */
	pcb_actionl("Netlist", "Sort", NULL);
	pcb_actionl("Netlist", "Thaw", NULL);
	return;
}

/*
 * find hyperlynx device by name 
 */
TODO(": convert this into a hash")
device_t *hyp_device_by_name(char *device_name)
{
	device_t *i;
	for (i = device_head; i != NULL; i = i->next)
		if (strcmp(i->ref, device_name) == 0)
			return i;
	return NULL;
}

pcb_subc_t *hyp_create_subc_by_name(char *refdes, pcb_coord_t x, pcb_coord_t y)
{
	pcb_subc_t *subc;
	unsigned int text_direction = 0;
	int text_scale = 100;
	device_t *dev;
	int on_bottom;

	/* does the device already exist? */
	subc = pcb_subc_by_refdes(hyp_dest, refdes);
	if (subc != NULL)
		return subc;

	/* device needs to be created. Search in DEVICE records. */
	dev = hyp_device_by_name(refdes);
	if (dev == NULL) {
		/* no device with this name exists, and no such device has been listed in a DEVICE record. Let's create the device anyhow so we can continue. Assume device is on component side. */
		pcb_message(PCB_MSG_WARNING, "device \"%s\" not specified in DEVICE record. Assuming device is on component side.\n", refdes);

		dev = calloc(sizeof(device_t), 1);
		dev->next = device_head;
		device_head = dev;
	}

	/* device on component or solder side? */
	if (dev->layer_name != NULL)
		on_bottom = hyp_is_bottom_layer(dev->layer_name);
	if (on_bottom)
		text_direction = 2;

	/* create */
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "creating device \"%s\".\n", dev->ref);

	subc = pcb_subc_alloc();
	pcb_subc_create_aux(subc, x, y, 0.0, on_bottom);
	pcb_attribute_put(&subc->Attributes, "refdes", refdes);
	pcb_subc_add_refdes_text(subc, x, y, text_direction, text_scale, on_bottom);
	pcb_subc_reg(hyp_dest, subc);
	pcb_subc_bind_globals(hyp_dest->parent.board, subc);

	dev->subc = subc;

	return subc;
}

static void hyp_subc_fin(pcb_subc_t *subc)
{
	pcb_subc_bbox(subc);
	if (hyp_dest->subc_tree == NULL)
		hyp_dest->subc_tree = pcb_r_create_tree();
	pcb_r_insert_entry(hyp_dest->subc_tree, (pcb_box_t *)subc);
	pcb_subc_rebind(hyp_dest->parent.board, subc);
}

static void hyp_subcs_fin(void)
{
	device_t *d;
	for(d = device_head; d != NULL; d = d->next)
		if (d->subc != NULL)
			hyp_subc_fin(d->subc);
}

/*
 * add segment to board outline
 */

void hyp_perimeter_segment_add(outline_t * s, pcb_bool_t forward)
{
	pcb_layer_id_t outline_id;
	pcb_layer_t *outline_layer;

	/* get outline layer */
	outline_id = pcb_layer_by_name(PCB->Data, "outline");
	if (outline_id < 0) {
		pcb_message(PCB_MSG_ERROR, "no outline layer.\n");
		return;
	}
	outline_layer = pcb_get_layer(PCB->Data, outline_id);
	if (outline_layer == NULL) {
		pcb_message(PCB_MSG_ERROR, "get outline layer failed.\n");
		return;
	}

	/* mark segment as used, so we don't use it twice */
	s->used = pcb_true;

	/* debugging */
	if (hyp_debug) {
		if (forward)
			pcb_message(PCB_MSG_DEBUG, "outline: fwd %s from (%ml, %ml) to (%ml, %ml)\n", s->is_arc ? "arc" : "line", s->x1, s->y1,
									s->x2, s->y2);
		else
			pcb_message(PCB_MSG_DEBUG, "outline: bwd %s from (%ml, %ml) to (%ml, %ml)\n", s->is_arc ? "arc" : "line", s->x2, s->y2, s->x1, s->y1);	/* add segment back to front */
	}

	if (s->is_arc) {
		/* counterclockwise arc from (x1, y1) to (x2, y2) */
		hyp_arc_new(outline_layer, s->x1, s->y1, s->x2, s->y2, s->xc, s->yc, s->r, s->r, pcb_false, 1, 0, pcb_no_flags());
	}
	else
		/* line from (x1, y1) to (x2, y2) */
		pcb_line_new(outline_layer, s->x1, s->y1, s->x2, s->y2, 1, 0, pcb_no_flags());

	return;
}

/* 
 * Check whether point (end_x, end_y) is connected to point (begin_x, begin_y) via un-used segment s and other un-used segments.
 */

pcb_bool_t hyp_segment_connected(pcb_coord_t begin_x, pcb_coord_t begin_y, pcb_coord_t end_x, pcb_coord_t end_y, outline_t * s)
{
	outline_t *i;
	pcb_bool_t connected;

	connected = (begin_x == end_x) && (begin_y == end_y);	/* trivial case */

	if (!connected) {
		/* recursive descent */
		s->used = pcb_true;

		for (i = outline_head; i != NULL; i = i->next) {
			if (i->used)
				continue;
			if ((i->x1 == begin_x) && (i->y1 == begin_y)) {
				connected = ((i->x2 == end_x) && (i->y2 == end_y)) || hyp_segment_connected(i->x2, i->y2, end_x, end_y, i);
				if (connected)
					break;
			}
			/* try back-to-front */
			if ((i->x2 == begin_x) && (i->y2 == begin_y)) {
				connected = ((i->x1 == end_x) && (i->y1 == end_y)) || hyp_segment_connected(i->x1, i->y1, end_x, end_y, i);
				if (connected)
					break;
			}
		}

		s->used = pcb_false;
	}

	return connected;
}

/* 
 * Sets (origin_x, origin_y)
 * Choose origin so that all coordinates are posive. 
 */

void hyp_set_origin()
{
	outline_t *i;

	/* safe initial value */
	if (outline_head != NULL) {
		origin_x = outline_head->x1;
		origin_y = outline_head->y1;
	}
	else {
		origin_x = 0;
		origin_y = 0;
	}

	/* choose upper left corner of outline */
	for (i = outline_head; i != NULL; i = i->next) {
		/* set origin so all coordinates are positive */
		if (i->x1 < origin_x)
			origin_x = i->x1;
		if (i->x2 < origin_x)
			origin_x = i->x2;
		if (i->y1 > origin_y)
			origin_y = i->y1;
		if (i->y2 > origin_y)
			origin_y = i->y2;
		if (i->is_arc) {
			if (i->xc - i->r < origin_x)
				origin_x = i->xc - i->r;
			if (i->yc + i->r > origin_y)
				origin_y = i->yc + i->r;
		}
	}
}

/* 
 * resize board to fit pcb outline.
 */

void hyp_resize_board()
{
	pcb_coord_t x_max, x_min, y_max, y_min;
	pcb_coord_t width, height;
	pcb_coord_t slack = PCB_MM_TO_COORD(1);
	outline_t *i;

	if (PCB == NULL)
		return;
	if (outline_head == NULL)
		return;

	x_min = x_max = outline_head->x1;
	y_min = y_max = outline_head->y1;

	for (i = outline_head; i != NULL; i = i->next) {

		x_max = max(x_max, i->x1);
		x_max = max(x_max, i->x2);
		y_max = max(y_max, i->y1);
		y_max = max(y_max, i->y2);

		x_min = min(x_min, i->x1);
		x_min = min(x_min, i->x2);
		y_min = min(y_min, i->y1);
		y_min = min(y_min, i->y2);

		if (i->is_arc) {
			x_max = max(x_max, i->xc + i->r);
			y_max = max(y_max, i->yc + i->r);

			x_min = min(x_min, i->xc - i->r);
			y_min = min(y_min, i->yc - i->r);
		}
	}

	width = max(PCB->MaxWidth, x_max - x_min + slack);
	height = max(PCB->MaxHeight, y_max - y_min + slack);

	/* resize if board too small */
	if ((width > PCB->MaxWidth) || (height > PCB->MaxHeight))
		pcb_board_resize(width, height);

	return;

}

/* 
 * Draw board perimeter.
 * The first segment is part of the first polygon.
 * The first polygon of the board perimeter is positive, the rest are holes.
 * Segments do not necessarily occur in order.
 */

void hyp_perimeter()
{
	pcb_bool_t warn_not_closed;
	pcb_bool_t segment_found;
	pcb_bool_t polygon_closed;
	pcb_coord_t begin_x, begin_y, last_x, last_y;
	outline_t *i;
	outline_t *j;

	warn_not_closed = pcb_false;

	/* iterate over perimeter segments and adjust origin */
	for (i = outline_head; i != NULL; i = i->next) {
		/* set origin so all coordinates are positive */
		i->x1 = i->x1 - origin_x;
		i->y1 = origin_y - i->y1;
		i->x2 = i->x2 - origin_x;
		i->y2 = origin_y - i->y2;
		if (i->is_arc) {
			i->xc = i->xc - origin_x;
			i->yc = origin_y - i->yc;
		}
	}

	/* iterate over perimeter polygons */
	while (pcb_true) {

		/* find first free segment */
		for (i = outline_head; i != NULL; i = i->next)
			if (i->used == pcb_false)
				break;

		/* exit if no segments found */
		if (i == NULL)
			break;

		/* first point of polygon (begin_x, begin_y) */
		begin_x = i->x1;
		begin_y = i->y1;

		/* last point of polygon (last_x, last_y) */
		last_x = i->x2;
		last_y = i->y2;

		/* add segment */
		hyp_perimeter_segment_add(i, pcb_true);

		/* add polygon segments until the polygon is closed */
		polygon_closed = pcb_false;
		while (!polygon_closed) {

#undef XXX
#ifdef XXX
			pcb_message(PCB_MSG_DEBUG, "perimeter: last_x = %ml last_y = %ml\n", last_x, last_y);
			for (i = outline_head; i != NULL; i = i->next)
				if (!i->used)
					pcb_message(PCB_MSG_DEBUG, "perimeter segments available: %s from (%ml, %ml) to (%ml, %ml)\n",
											i->is_arc ? "arc " : "line", i->x1, i->y1, i->x2, i->y2);
#endif

			/* find segment to add to current polygon */
			segment_found = pcb_false;

			/* XXX prefer closed polygon over open polyline */

			for (i = outline_head; i != NULL; i = i->next) {
				if (i->used)
					continue;

				if ((last_x == i->x1) && (last_y == i->y1)) {
					if (!hyp_segment_connected(i->x2, i->y2, begin_x, begin_y, i))
						continue;
					/* first point of segment is last point of current edge: add segment to edge */
					segment_found = pcb_true;
					hyp_perimeter_segment_add(i, pcb_true);
					last_x = i->x2;
					last_y = i->y2;
				}
				else if ((last_x == i->x2) && (last_y == i->y2)) {
					if (!hyp_segment_connected(i->x1, i->y1, begin_x, begin_y, i))
						continue;
					/* last point of segment is last point of current edge: add segment to edge back to front */
					segment_found = pcb_true;
					/* add segment back to front */
					hyp_perimeter_segment_add(i, pcb_false);
					last_x = i->x1;
					last_y = i->y1;
				}
				if (segment_found)
					break;
			}
			polygon_closed = (begin_x == last_x) && (begin_y == last_y);
			if (!polygon_closed && !segment_found)
				break;									/* can't find anything suitable */
		}
		if (polygon_closed) {
			if (hyp_debug)
				pcb_message(PCB_MSG_DEBUG, "outline: closed\n");
		}
		else {
			if (hyp_debug)
				pcb_message(PCB_MSG_DEBUG, "outline: open\n");
			warn_not_closed = pcb_true;
		}
	}

	/* free segment memory */
	for (i = outline_head; i != NULL; i = j) {
		j = i->next;
		free(i);
	}
	outline_head = outline_tail = NULL;

	if (warn_not_closed)
		pcb_message(PCB_MSG_WARNING, "warning: board outline not closed\n");

	return;
}

/* 
 * reset pcb layer stack 
 */
void hyp_reset_layers()
{

	pcb_layer_id_t id = -1;
	pcb_layergrp_id_t gid = -1;
	pcb_layergrp_t *grp = NULL;

	pcb_layergrp_inhibit_inc();

	pcb_layers_reset(PCB);

	pcb_layer_group_setup_default(PCB);

	/* set up dual layer board: top and bottom copper and silk */

	id = -1;
	if (pcb_layergrp_list(PCB, PCB_LYT_SILK | PCB_LYT_TOP, &gid, 1) == 1)
		id = pcb_layer_create(PCB, gid, "top silk");
	if (id < 0)
		pcb_message(PCB_MSG_ERROR, "failed to create top silk\n");

	id = -1;
	if (pcb_layergrp_list(PCB, PCB_LYT_SILK | PCB_LYT_BOTTOM, &gid, 1) == 1)
		id = pcb_layer_create(PCB, gid, "bottom silk");
	if (id < 0)
		pcb_message(PCB_MSG_ERROR, "failed to create bottom silk\n");

	top_layer_id = -1;
	if (pcb_layergrp_list(PCB, PCB_LYT_COPPER | PCB_LYT_TOP, &gid, 1) == 1)
		top_layer_id = pcb_layer_create(PCB, gid, "");
	if (top_layer_id < 0)
		pcb_message(PCB_MSG_ERROR, "failed to create top copper\n");

	bottom_layer_id = -1;
	if (pcb_layergrp_list(PCB, PCB_LYT_COPPER | PCB_LYT_BOTTOM, &gid, 1) == 1)
		bottom_layer_id = pcb_layer_create(PCB, gid, "");
	if (bottom_layer_id < 0)
		pcb_message(PCB_MSG_ERROR, "failed to create bottom copper\n");

	/* create outline layer */

	id = -1;
	grp = pcb_get_grp_new_intern(PCB, -1);
	if (grp != NULL) {
		id = pcb_layer_create(PCB, grp - PCB->LayerGroups.grp, "outline");
		pcb_layergrp_fix_turn_to_outline(grp);
	}
	if (id < 0)
		pcb_message(PCB_MSG_ERROR, "failed to create outline\n");

	pcb_layergrp_inhibit_dec();

	return;
}

/*
 * Returns the pcb_layer_id of layer "lname". 
 * If layer lname does not exist, a new copper layer with name "lname" is created.
 * If the layer name is NULL, a new copper layer with a new, unused layer name is created.
 */

pcb_layer_id_t hyp_create_layer(char *lname)
{
	pcb_layer_id_t layer_id;
	pcb_layergrp_id_t gid;
	pcb_layergrp_t *grp;
	char new_layer_name[MAX_STRING];
	int n;

	layer_id = -1;
	if (lname != NULL) {
		/* we have a layer name. check whether layer already exists */
		layer_id = pcb_layer_by_name(PCB->Data, lname);
		if (layer_id >= 0)
			return layer_id;					/* found. return existing layer. */
	}
	else {
		/* no layer name given. find unused layer name in range 1..PCB_MAX_LAYER */
		for (n = 1; n < PCB_MAX_LAYER; n++) {
			pcb_sprintf(new_layer_name, "%i", n);
			if (pcb_layer_by_name(PCB->Data, new_layer_name) < 0) {
				lname = new_layer_name;
				break;
			}
		}
		if (lname == NULL)
			return bottom_layer_id;		/* no unused layer name available */
	}

	/* new layer */

	layer_count++;
	switch (layer_count) {
	case 1:
		/* rename top copper and return */
		pcb_layer_rename(PCB->Data, top_layer_id, lname);
		return top_layer_id;
		break;

	case 2:
		/* rename bottom copper and return */
		pcb_layer_rename(PCB->Data, bottom_layer_id, lname);
		return bottom_layer_id;
		break;

	default:

		/* create new bottom layer */
		pcb_layergrp_list(PCB, PCB_LYT_COPPER | PCB_LYT_BOTTOM, &gid, 1);
		layer_id = pcb_layer_create(PCB, gid, lname);

		/* check if new bottom layer valid */
		if (layer_id < 0) {
			/* layer creation failed. return old bottom layer. */
			if (hyp_debug)
				pcb_message(PCB_MSG_DEBUG, "running out of layers\n");
			return bottom_layer_id;
		}

		/* move old bottom layer to internal */
		grp = pcb_get_grp_new_intern(PCB, -1);
		pcb_layer_move_to_group(PCB, bottom_layer_id, grp - PCB->LayerGroups.grp);

		/* created layer becomes new bottom layer */
		bottom_layer_id = layer_id;
		return bottom_layer_id;
	}

	return bottom_layer_id;
}

/*
 * convert hyperlynx layer name to pcb layer
 */

pcb_layer_t *hyp_get_layer(parse_param * h)
{
	return pcb_get_layer(PCB->Data, hyp_create_layer(h->layer_name));
}

/*
 * True if solder side 
 */

pcb_bool_t hyp_is_bottom_layer(char *layer_name)
{
	return ((layer_name != NULL) && (pcb_layer_flags(PCB, pcb_layer_by_name(PCB->Data, layer_name)) & PCB_LYT_BOTTOM));
}

/*
 * Draw arc from (x1, y1) to (x2, y2) with center (xc, yc) and radius r. 
 * Direction of arc is clockwise or counter-clockwise, depending upon value of pcb_bool_t Clockwise.
 */

pcb_arc_t *hyp_arc_new(pcb_layer_t * Layer, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t XC,
											 pcb_coord_t YC, pcb_coord_t Width, pcb_coord_t Height, pcb_bool_t Clockwise, pcb_coord_t Thickness,
											 pcb_coord_t Clearance, pcb_flag_t Flags)
{
	pcb_angle_t start_angle;
	pcb_angle_t end_angle;
	pcb_angle_t delta;
	pcb_arc_t *new_arc;

	if (Width < 1) {
		start_angle = 0.0;
		end_angle = 360.0;
	}
	else {
		/* note: y-axis points down */
		start_angle = 180 + 180 * atan2(YC - Y1, X1 - XC) / M_PI;
		end_angle = 180 + 180 * atan2(YC - Y2, X2 - XC) / M_PI;
	}
	start_angle = pcb_normalize_angle(start_angle);
	end_angle = pcb_normalize_angle(end_angle);

	if (Clockwise)
		while (start_angle < end_angle)
			start_angle += 360;
	else
		while (end_angle <= start_angle)
			end_angle += 360;

	delta = end_angle - start_angle;

	new_arc = pcb_arc_new(Layer, XC, YC, Width, Height, start_angle, delta, Thickness, Clearance, Flags, pcb_true);

	return new_arc;
}

/*
 * Draw an arc from (x1, y1) to (x2, y2) with center (xc, yc) and radius r using a polygonal approximation.
 * Arc may be clockwise or counterclockwise. Note pcb-rnd y-axis points downward.
 */
void hyp_arc2contour(pcb_pline_t * contour, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t xc,
										 pcb_coord_t yc, pcb_coord_t r, pcb_bool_t clockwise)
{
	pcb_coord_t arc_precision = PCB_MM_TO_COORD(0.254);	/* mm */
	int min_circle_segments = 8;	/* 8 seems minimal, 16 seems more than sufficient. */
	int segments;
	int poly_points = 0;
	int i;
	pcb_vector_t v;

	double alpha = atan2(y1 - yc, x1 - xc);
	double beta = atan2(y2 - yc, x2 - xc);

	if (contour == NULL)
		return;

	if (clockwise) {
		/* draw arc clockwise from (x1,y1) to (x2,y2) */
		if (beta < alpha)
			beta = beta + 2 * M_PI;
	}
	else {
		/* draw arc counterclockwise from (x1,y1) to (x2,y2) */
		if (alpha < beta)
			alpha = alpha + 2 * M_PI;
		/* draw a circle if starting and end points are the same */
		if ((x1 == x2) && (y1 == y2))
			beta = alpha + 2 * M_PI;
	}

	/* Calculate number of segments needed for a full circle. */
	segments = min_circle_segments;
	if (arc_precision > 0) {
		/* Increase number of segments until difference between circular arc and polygonal approximation is less than max_arc_error */
		double arc_error;
		do {
			arc_error = r * (1 - cos(M_PI / segments));
			if (arc_error > arc_precision)
				segments += 4;
		}
		while (arc_error > arc_precision);
	}
	else if (arc_precision < 0)
		pcb_message(PCB_MSG_ERROR, "error: negative arc precision\n");

	/* A full circle is drawn using 'segments' segments; a 90 degree arc using segments/4. */
	poly_points = pcb_round(segments * fabs(beta - alpha) / (2 * M_PI));

	/* Sanity checks */
	if (poly_points < 1)
		poly_points = 1;

	/* add first point to contour */
	v[0] = x1;
	v[1] = y1;
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));

	/* intermediate points */
	for (i = 1; i < poly_points; i++) {
		double angle = alpha + (beta - alpha) * i / poly_points;
		v[0] = xc + r * cos(angle);
		v[1] = yc + r * sin(angle);
		pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
	}

	/* last point */
	v[0] = x2;
	v[1] = y2;
	pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));

	return;
}

/*
 * dump polygon structure for debugging.
 */
void hyp_dump_polygons()
{
	hyp_polygon_t *i;
	hyp_vertex_t *v;

	/* loop over all created polygons and draw them */
	for (i = polygon_head; i != NULL; i = i->next) {
		pcb_message(PCB_MSG_DEBUG, "%s id=%i.\n", (i->is_polygon ? "polygon" : "polyline"), i->hyp_poly_id);
		for (v = i->vertex; v != NULL; v = v->next) {
			if (v->is_first)
				pcb_message(PCB_MSG_DEBUG, "  contour\n");
			if (v->is_arc)
				pcb_message(PCB_MSG_DEBUG, "    arc  x1 = %ml y1 = %ml x2 = %ml y2 = %ml xc = %ml yc = %ml r = %ml\n", v->x1, v->y1,
										v->x2, v->y2, v->xc, v->yc, v->r);
			else
				pcb_message(PCB_MSG_DEBUG, "    line x1 = %ml y1 = %ml\n", v->x1, v->y1);
		}
	}
}

/* 
 * draw a single polyline 
 */

void hyp_draw_polyline(hyp_polygon_t * polyline)
{
	pcb_layer_t *layer;
	pcb_coord_t xpos;
	pcb_coord_t ypos;
	hyp_vertex_t *vrtx;

	if ((polyline == NULL) || (polyline->vertex == NULL))
		return;

	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "draw polyline:  drawing poly id=%i.\n", polyline->hyp_poly_id);

	layer = pcb_get_layer(PCB->Data, hyp_create_layer(polyline->layer_name));

	xpos = polyline->vertex->x1;
	ypos = polyline->vertex->y1;

	for (vrtx = polyline->vertex->next; vrtx != NULL; vrtx = vrtx->next) {
		if (vrtx->is_first)
			break;										/* polyvoids in polyline not implemented */
		if (!vrtx->is_arc) {
			/* draw line */
			pcb_line_new(layer, xpos, ypos, vrtx->x1, vrtx->y1, polyline->line_width, polyline->clearance, pcb_no_flags());
			/* move on */
			xpos = vrtx->x1;
			ypos = vrtx->y1;
		}
		else {
			/* draw clockwise arc from (x1, y2) to (x2, y2) */
			hyp_arc_new(layer, vrtx->x1, vrtx->y1, vrtx->x2, vrtx->y2, vrtx->xc, vrtx->yc, vrtx->r, vrtx->r, pcb_false,
									polyline->line_width, polyline->clearance, pcb_no_flags());
			/* move on */
			if ((xpos == vrtx->x1) && (ypos == vrtx->y1)) {
				xpos = vrtx->x2;
				ypos = vrtx->y2;
			}
			else if ((xpos == vrtx->x2) && (ypos == vrtx->y2)) {
				xpos = vrtx->x1;
				ypos = vrtx->y1;
			}
		}
	}
	return;
}

/* 
 * draw a single polygon 
 */

void hyp_draw_polygon(hyp_polygon_t * polygon)
{
	pcb_layer_t *layer;
	pcb_bool_t outer_contour;
	hyp_vertex_t *vrtx;

	pcb_polyarea_t *polyarea = NULL;
	pcb_pline_t *contour = NULL;

	polyarea = pcb_polyarea_create();
	if (polyarea == NULL)
		return;

	if ((polygon == NULL) || (polygon->vertex == NULL))
		return;

	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "draw polygon:   drawing poly id=%i.\n", polygon->hyp_poly_id);

	layer = pcb_get_layer(PCB->Data, hyp_create_layer(polygon->layer_name));

	outer_contour = pcb_true;

	for (vrtx = polygon->vertex; vrtx != NULL; vrtx = vrtx->next) {
		pcb_vector_t v;
		v[0] = vrtx->x1;
		v[1] = vrtx->y1;
		if ((vrtx->is_first) || (vrtx->next == NULL)) {
			if (contour != NULL) {
				/* add contour to polyarea */
				pcb_poly_contour_pre(contour, pcb_false);

				/* check contour valid */
				if (pcb_polyarea_contour_check(contour) && hyp_debug)
					pcb_message(PCB_MSG_WARNING, "draw polygon: bad contour? continuing.\n");

				/* set orientation for outer contour, negative for holes */
				if (contour->Flags.orient != (outer_contour ? PCB_PLF_DIR : PCB_PLF_INV))
					pcb_poly_contour_inv(contour);

				/* add contour to polyarea */
				pcb_polyarea_contour_include(polyarea, contour);

				/* first contour is outer contour, remainder are holes */
				outer_contour = pcb_false;
			}
			/* create new contour */
			contour = pcb_poly_contour_new(v);
			if (contour == NULL)
				return;
		}
		else {
			if (!vrtx->is_arc)
				/* line. add vertex to contour */
				pcb_poly_vertex_include(contour->head.prev, pcb_poly_node_create(v));
			else
				/* arc. pcb polyarea contains line segments, not arc segments. convert arc segment to line segments. */
				hyp_arc2contour(contour, vrtx->x1, vrtx->y1, vrtx->x2, vrtx->y2, vrtx->xc, vrtx->yc, vrtx->r, pcb_false);
		}
	}

	/* add polyarea to pcb */
	if (pcb_poly_valid(polyarea))
		pcb_poly_to_polygons_on_layer(hyp_dest, layer, polyarea, pcb_no_flags());
	else if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "draw polygon: self-intersecting polygon id=%i dropped.\n", polygon->hyp_poly_id);

	return;
}

/*
 * create all polygons 
 */

void hyp_draw_polygons()
{
	hyp_polygon_t *i;
	int l, layer_count = 0;
	pcb_layer_id_t *layer_array = NULL;

#ifdef XXX
	hyp_dump_polygons();					/* more debugging */
#endif

	/* get list of copper layer id's */
	layer_count = pcb_layer_list(PCB, PCB_LYT_COPPER, NULL, 0);
	if (layer_count <= 0)
		return;
	layer_array = malloc(sizeof(pcb_layer_id_t) * layer_count);
	if (layer_array == NULL)
		return;
	layer_count = pcb_layer_list(PCB, PCB_LYT_COPPER, layer_array, layer_count);

	/* loop over all layers */
	for (l = 0; l < layer_count; l++) {
		pcb_layer_id_t layer_id = layer_array[l];
		if (hyp_debug)
			pcb_message(PCB_MSG_DEBUG, "draw polygons: layer %lx \"%s\"\n", layer_id, pcb_layer_name(PCB->Data, layer_id));

		/* loop over all polygons of the layer and draw them */
		for (i = polygon_head; i != NULL; i = i->next) {
			/* check polygon is on layer */
			if (layer_id != hyp_create_layer(i->layer_name))
				continue;
			/* polygon or polyline? */
			if (i->is_polygon)
				hyp_draw_polygon(i);
			else
				hyp_draw_polyline(i);
		}
	}

	return;
}

/*
 * calculate hyperlynx-style clearance
 * 
 * use plane_separation of, in order:
 * - this copper
 * - the net this copper belongs to
 * - the layer this copper is on
 * - the board
 * if neither copper, net, layer nor board have plane_separation set, do not set clearance.
 */

pcb_coord_t hyp_clearance(parse_param * h)
{
	pcb_coord_t clearance;
	pcb_layer_id_t layr_id;

	if (h->layer_name_set)
		layr_id = hyp_create_layer(h->layer_name);
	clearance = -1;

	if (h->plane_separation_set)
		clearance = xy2coord(h->plane_separation);
	else if (net_clearance >= 0)
		clearance = net_clearance;
	else if (h->layer_name_set && (layer_clearance[layr_id] >= 0))
		clearance = layer_clearance[layr_id];
	else if (board_clearance >= 0)
		clearance = board_clearance;
	else
		clearance = 0;

	return clearance;
}

/* exec_* routines are called by parser to interpret hyperlynx file */

/*
 * 'BOARD_FILE' record.
 */

pcb_bool exec_board_file(parse_param * h)
{
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "board:\n");

	return 0;
}

/*
 * 'VERSION' record.
 * Specifies version number.
 */

pcb_bool exec_version(parse_param * h)
{
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "version: vers = %f\n", h->vers);

	if (h->vers < 1.0)
		pcb_message(PCB_MSG_DEBUG, "info: version 1.x deprecated\n");

	return 0;
}

/*
 * 'DATA_MODE' record.
 * If DATA_MODE is DETAILED, model can be used for power and signal simulation.
 * If DATA_MODE is SIMPLIFIED, model can be used for signal simulation only.
 */

pcb_bool exec_data_mode(parse_param * h)
{
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "data_mode: detailed = %i\n", h->detailed);

	return 0;
}

/*
 * 'UNITS' record.
 * Specifies measurement system (english/metric) for the rest of the file.
 */

pcb_bool exec_units(parse_param * h)
{
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "units: unit_system_english = %d metal_thickness_weight = %d\n", h->unit_system_english,
								h->metal_thickness_weight);

	/* convert everything to meter */

	if (h->unit_system_english) {
		unit = inches;							/* lengths in inches. 1 in = 2.54 cm = 0.0254 m */
		if (h->metal_thickness_weight)
			metal_thickness_unit = copper_imperial_weight * unit;	/* metal thickness in ounces/ft2. 1 oz/ft2 copper = 1.341 mil */
		else
			metal_thickness_unit = unit;	/* metal thickness in inches */
	}
	else {
		unit = 0.01;								/* lengths in centimeters. 1 cm = 0.01 m */
		if (h->metal_thickness_weight)
			metal_thickness_unit = copper_metric_weight * unit;	/* metal thickness in grams/cm2. 1 gr/cm2 copper = 0.1116 cm */
		else
			metal_thickness_unit = unit;	/* metal thickness in centimeters */
	}

	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "units: unit = %f metal_thickness_unit = %f\n", unit, metal_thickness_unit);

	return 0;
}

/*
 * 'PLANE_SEP' record.
 * Defines default trace to plane clearance.
 */

pcb_bool exec_plane_sep(parse_param * h)
{
	board_clearance = xy2coord(h->default_plane_separation);

	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "plane_sep: default_plane_separation = %ml\n", board_clearance);

	return 0;
}

/*
 * 'PERIMETER_SEGMENT' subrecord of 'BOARD' record.
 * Draws linear board outline segment from (x1, y1) to (x2, y2).
 */

pcb_bool exec_perimeter_segment(parse_param * h)
{
	outline_t *peri_seg;

	peri_seg = malloc(sizeof(outline_t));

	/* convert coordinates */
	peri_seg->x1 = xy2coord(h->x1);
	peri_seg->y1 = xy2coord(h->y1);
	peri_seg->x2 = xy2coord(h->x2);
	peri_seg->y2 = xy2coord(h->y2);
	peri_seg->xc = 0;
	peri_seg->yc = 0;
	peri_seg->r = 0;
	peri_seg->is_arc = pcb_false;
	peri_seg->used = pcb_false;
	peri_seg->next = NULL;

	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "perimeter_segment: x1 = %ml y1 = %ml x2 = %ml y2 = %ml\n", peri_seg->x1, peri_seg->y1,
								peri_seg->x2, peri_seg->y2);

	/* append at end of doubly linked list */
	if (outline_tail == NULL) {
		outline_head = peri_seg;
		outline_tail = peri_seg;
	}
	else {
		outline_tail->next = peri_seg;
		outline_tail = peri_seg;
	}

	/* set origin so all coordinates are positive */
	hyp_set_origin();

	/* resize board if too small to draw outline */
	hyp_resize_board();

	return 0;
}

/*
 * 'PERIMETER_ARC' subrecord of 'BOARD' record.
 * Draws arc segment of board outline. 
 * Arc drawn counterclockwise from (x1, y1) to (x2, y2) with center (xc, yc) and radius r.
 */

pcb_bool exec_perimeter_arc(parse_param * h)
{
	outline_t *peri_arc;

	peri_arc = malloc(sizeof(outline_t));

	peri_arc->x1 = xy2coord(h->x1);
	peri_arc->y1 = xy2coord(h->y1);
	peri_arc->x2 = xy2coord(h->x2);
	peri_arc->y2 = xy2coord(h->y2);
	peri_arc->xc = xy2coord(h->xc);
	peri_arc->yc = xy2coord(h->yc);
	peri_arc->r = xy2coord(h->r);
	peri_arc->is_arc = pcb_true;
	peri_arc->used = pcb_false;
	peri_arc->next = NULL;

	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "perimeter_arc: x1 = %ml y1 = %ml x2 = %ml y2 = %ml xc = %ml yc = %ml r = %ml\n", peri_arc->x1,
								peri_arc->y1, peri_arc->x2, peri_arc->y2, peri_arc->xc, peri_arc->yc, peri_arc->r);

	/* append at end of doubly linked list */
	if (outline_tail == NULL) {
		outline_head = peri_arc;
		outline_tail = peri_arc;
	}
	else {
		outline_tail->next = peri_arc;
		outline_tail = peri_arc;
	}

	/* set origin so all coordinates are positive */
	hyp_set_origin();

	return 0;
}

/*
 * 'A' attribute subrecord of 'BOARD' record.
 * Defines board attributes as name/value pairs.
 */

pcb_bool exec_board_attribute(parse_param * h)
{
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "board_attribute: name = \"%s\" value = \"%s\"\n", h->name, h->value);

	return 0;
}

/*
 * STACKUP section 
 */

/*
 * Debug output for layers
 */

void hyp_debug_layer(parse_param * h)
{
	if (hyp_debug) {
		if (h->thickness_set)
			pcb_message(PCB_MSG_DEBUG, " thickness = %ml", z2coord(h->thickness));
		if (h->plating_thickness_set)
			pcb_message(PCB_MSG_DEBUG, " plating_thickness = %ml", z2coord(h->plating_thickness));
		if (h->bulk_resistivity_set)
			pcb_message(PCB_MSG_DEBUG, " bulk_resistivity = %f", h->bulk_resistivity);
		if (h->temperature_coefficient_set)
			pcb_message(PCB_MSG_DEBUG, " temperature_coefficient = %f", h->temperature_coefficient);
		if (h->epsilon_r_set)
			pcb_message(PCB_MSG_DEBUG, " epsilon_r = %f", h->epsilon_r);
		if (h->loss_tangent_set)
			pcb_message(PCB_MSG_DEBUG, " loss_tangent = %f", h->loss_tangent);
		if (h->conformal_set)
			pcb_message(PCB_MSG_DEBUG, " conformal = %i", h->conformal);
		if (h->prepreg_set)
			pcb_message(PCB_MSG_DEBUG, " prepreg = %i", h->prepreg);
		if (h->layer_name_set)
			pcb_message(PCB_MSG_DEBUG, " layer_name = \"%s\"", h->layer_name);
		if (h->material_name_set)
			pcb_message(PCB_MSG_DEBUG, " material_name = \"%s\"", h->material_name);
		if (h->plane_separation_set)
			pcb_message(PCB_MSG_DEBUG, " plane_separation = %ml", xy2coord(h->plane_separation));
		pcb_message(PCB_MSG_DEBUG, "\n");
	}

	return;
}

/*
 * 'OPTIONS' subrecord of 'STACKUP' record.
 * Defines dielectric constant and loss tangent of metal layers.
 */

pcb_bool exec_options(parse_param * h)
{
	/* Use dielectric for metal? */
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "options: use_die_for_metal = %f\n", h->use_die_for_metal);
	if (h->use_die_for_metal)
		use_die_for_metal = pcb_true;
	return 0;
}

/*
 * SIGNAL subrecord of STACKUP record.
 * signal layer.
 */

pcb_bool exec_signal(parse_param * h)
{
	pcb_layer_id_t signal_layer_id;

	if ((h->layer_name != NULL) && (pcb_layer_by_name(PCB->Data, h->layer_name) >= 0))
		pcb_message(PCB_MSG_WARNING, "duplicate SIGNAL layer name \"%s\"\n", h->layer_name);

	signal_layer_id = hyp_create_layer(h->layer_name);

	layer_is_plane[signal_layer_id] = pcb_false;
	if (h->plane_separation_set)
		layer_clearance[signal_layer_id] = xy2coord(h->plane_separation);

	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "signal layer: \"%s\"", pcb_layer_name(PCB->Data, signal_layer_id));
	hyp_debug_layer(h);

	return 0;
}

/*
 * DIELECTRIC subrecord of STACKUP record.
 * dielectric layer 
 */

pcb_bool exec_dielectric(parse_param * h)
{
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "dielectric layer: ");
	hyp_debug_layer(h);

	return 0;
}

/*
 * PLANE subrecord of STACKUP record.
 * plane layer - a layer which is flooded with copper.
 */

pcb_bool exec_plane(parse_param * h)
{
	pcb_layer_id_t plane_layer_id;

	if ((h->layer_name != NULL) && (pcb_layer_by_name(PCB->Data, h->layer_name) >= 0))
		pcb_message(PCB_MSG_WARNING, "duplicate PLANE layer name \"%s\"\n", h->layer_name);

	plane_layer_id = hyp_create_layer(h->layer_name);

	layer_is_plane[plane_layer_id] = pcb_true;
	if (h->plane_separation_set)
		layer_clearance[plane_layer_id] = xy2coord(h->plane_separation);

	/* XXX need to flood layer with copper */

	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "plane layer: \"%s\"", pcb_layer_name(PCB->Data, plane_layer_id));
	hyp_debug_layer(h);

	return 0;
}

/* DEVICE record. */
pcb_bool exec_devices(parse_param * h)
{
	device_t *new_device;
	char value[128];

	if (hyp_debug) {
		pcb_message(PCB_MSG_DEBUG, "device: device_type = \"%s\" ref = \"%s\"", h->device_type, h->ref);
		if (h->name_set)
			pcb_message(PCB_MSG_DEBUG, " name = \"%s\"", h->name);
		if (h->value_float_set)
			pcb_message(PCB_MSG_DEBUG, " value_float = %f", h->value_float);
		if (h->value_string_set)
			pcb_message(PCB_MSG_DEBUG, " value_string = \"%s\"", h->value_string);
		if (h->layer_name_set)
			pcb_message(PCB_MSG_DEBUG, " layer_name = \"%s\"", h->layer_name);
		if (h->package_set)
			pcb_message(PCB_MSG_DEBUG, " package = \"%s\"", h->package);
		pcb_message(PCB_MSG_DEBUG, "\n");
	}

	/* add device to list  */
	new_device = calloc(sizeof(device_t), 1);

	new_device->ref = pcb_strdup(h->ref);

	new_device->name = NULL;
	if (h->name_set)
		new_device->name = pcb_strdup(h->name);


	new_device->value = NULL;
	if (h->value_string_set)
		new_device->value = pcb_strdup(h->value_string);
	else if (h->value_float_set) {
		/* convert double to string */
		pcb_snprintf(value, sizeof(value), "%f", h->value_float);
		new_device->value = pcb_strdup(value);
	}

	new_device->layer_name = NULL;
	if (h->layer_name_set)
		new_device->layer_name = pcb_strdup(h->layer_name);

	new_device->next = device_head;
	device_head = new_device;

	return 0;
}

/*
 * SUPPLY record.
 * marks nets as power supply.
 */

pcb_bool exec_supplies(parse_param * h)
{
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "supplies: name = \"%s\" value_float = %f voltage_specified = %i conversion = %i\n", h->name,
								h->value_float, h->voltage_specified, h->conversion);

	return 0;
}

/* 
 * PADSTACK record
 */

pcb_bool exec_pstk_element(parse_param * h)
{
	/*
	 * Layer names with special meaning, used in padstack definition:
	 * MDEF: defines copper pad on all metal layers
	 * ADEF: defines anti-pad (clearance) on all power planes.
	 */

	if (hyp_debug) {
		pcb_message(PCB_MSG_DEBUG, "padstack_element:");
		if (h->padstack_name_set)
			pcb_message(PCB_MSG_DEBUG, " padstack_name = \"%s\"", h->padstack_name);
		if (h->drill_size_set)
			pcb_message(PCB_MSG_DEBUG, " drill_size = %ml", xy2coord(h->drill_size));
		pcb_message(PCB_MSG_DEBUG, " layer_name = \"%s\"", h->layer_name);
		pcb_message(PCB_MSG_DEBUG, " pad_shape = %f", h->pad_shape);
		if (h->pad_shape == 0)
			pcb_message(PCB_MSG_DEBUG, " oval");
		else if (h->pad_shape == 1)
			pcb_message(PCB_MSG_DEBUG, " rectangular");
		else if (h->pad_shape == 2)
			pcb_message(PCB_MSG_DEBUG, " oblong");
		else
			pcb_message(PCB_MSG_DEBUG, " ?");
		pcb_message(PCB_MSG_DEBUG, " pad_sx = %ml", xy2coord(h->pad_sx));
		pcb_message(PCB_MSG_DEBUG, " pad_sy = %ml", xy2coord(h->pad_sy));
		pcb_message(PCB_MSG_DEBUG, " pad_angle = %f", h->pad_angle);
		if (h->pad_type_set & (h->pad_type == PAD_TYPE_THERMAL_RELIEF)) {
			pcb_message(PCB_MSG_DEBUG, " thermal_clear_shape = %f", h->thermal_clear_shape);
			if (h->thermal_clear_shape == 0)
				pcb_message(PCB_MSG_DEBUG, " oval");
			else if (h->thermal_clear_shape == 1)
				pcb_message(PCB_MSG_DEBUG, " rectangular");
			else if (h->thermal_clear_shape == 2)
				pcb_message(PCB_MSG_DEBUG, " oblong");
			else
				pcb_message(PCB_MSG_DEBUG, " ?");
			pcb_message(PCB_MSG_DEBUG, " thermal_clear_sx = %ml", xy2coord(h->thermal_clear_sx));
			pcb_message(PCB_MSG_DEBUG, " thermal_clear_sy = %ml", xy2coord(h->thermal_clear_sy));
			pcb_message(PCB_MSG_DEBUG, " thermal_clear_angle = %f", h->thermal_clear_angle);
		}
		if (h->pad_type_set) {
			pcb_message(PCB_MSG_DEBUG, " pad_type = ");
			switch (h->pad_type) {
			case PAD_TYPE_METAL:
				pcb_message(PCB_MSG_DEBUG, "metal");
				break;
			case PAD_TYPE_ANTIPAD:
				pcb_message(PCB_MSG_DEBUG, "antipad");
				break;
			case PAD_TYPE_THERMAL_RELIEF:
				pcb_message(PCB_MSG_DEBUG, "thermal_relief");
				break;
			default:
				pcb_message(PCB_MSG_DEBUG, "error");
			}
		}
		pcb_message(PCB_MSG_DEBUG, "\n");
	}

	if (h->padstack_name_set) {
		/* add new padstack */
		current_pstk = malloc(sizeof(padstack_t));
		if (current_pstk == NULL)
			return 1;									/*malloc failed */
		current_pstk->name = pcb_strdup(h->padstack_name);
		current_pstk->drill_size = xy2coord(h->drill_size);
		current_pstk_element = malloc(sizeof(padstack_element_t));
		current_pstk->padstack = current_pstk_element;
	}
	else {
		/* add new padstack element */
		current_pstk_element->next = malloc(sizeof(padstack_element_t));
		current_pstk_element = current_pstk_element->next;
		if (current_pstk_element == NULL)
			return 1;									/*malloc failed */
	}

	/* fill in values */

	current_pstk_element->layer_name = pcb_strdup(h->layer_name);
	current_pstk_element->pad_shape = h->pad_shape;
	current_pstk_element->pad_sx = xy2coord(h->pad_sx);
	current_pstk_element->pad_sy = xy2coord(h->pad_sy);
	current_pstk_element->pad_angle = h->pad_angle;
	current_pstk_element->thermal_clear_sx = xy2coord(h->thermal_clear_sx);
	current_pstk_element->thermal_clear_sy = xy2coord(h->thermal_clear_sy);
	current_pstk_element->thermal_clear_angle = h->thermal_clear_angle;
	if (h->pad_type_set)
		current_pstk_element->pad_type = h->pad_type;
	else
		current_pstk_element->pad_type = PAD_TYPE_METAL;
	current_pstk_element->next = NULL;

	return 0;
}


pcb_bool exec_pstk_end(parse_param * h)
{
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "padstack_end\n");

	/* add current padstack to list of padstacks */
	if (current_pstk != NULL) {
		current_pstk->next = padstack_head;
		padstack_head = current_pstk;
		current_pstk = NULL;
	}

	current_pstk_element = NULL;

	return 0;
}

static int hyp_pstk_add_shp(pcb_pstk_shape_t *sh, int *used, int max, padstack_element_t *i, pcb_layer_type_t lyt, pcb_layer_combining_t comb, pcb_coord_t offs)
{
	if (*used >= max)
		return -1;
	sh[*used].layer_mask = lyt;
	sh[*used].comb = comb;
	switch(i->pad_shape) {
		case 0: /* round */
		case 2: /* oblong, rounded rectangle - emulate by line */
			pcb_shape_oval(&sh[*used], i->pad_sx+offs, i->pad_sy+offs);
			break;
		case 1: /* rectangle */
			pcb_shape_rect(&sh[*used], i->pad_sx+offs, i->pad_sy+offs);
			break;
		default:
			return -1;
	}
	(*used)++;
	return 0;
}

static pcb_pstk_t *hyp_new_pstk(padstack_t *padstk, pcb_data_t *data, pcb_coord_t x, pcb_coord_t y, pcb_bool with_paste, pcb_bool with_mask)
{
	pcb_pstk_t *ps;
	padstack_element_t *i;
	pcb_pstk_shape_t sh[8];
	int sh_max = sizeof(sh) / sizeof(sh[0]) - 1; /* leave one entry for terminating */
	int sh_used = 0, have_top = 0, have_bottom = 0, have_inner = 0;
	const char *ln_top, *ln_bot;

	memset(sh, 0, sizeof(sh));

	ln_top = pcb_layer_name(PCB->Data, top_layer_id);
	ln_bot = pcb_layer_name(PCB->Data, bottom_layer_id);

	/* loop over all layers of the padstack and create shapes in sh[] */
	for (i = padstk->padstack; i != NULL; i = i->next) {
		int mdef, top_or_bottom;
		if (i->layer_name == NULL)
			continue;
		if (i->pad_type != PAD_TYPE_METAL)
			continue;

		if (i->pad_angle != 0)
			pcb_message(PCB_MSG_ERROR, "ignoring pad rotation of padstack at %$mm;%$mm.\n", x, y);

		mdef = strcmp(i->layer_name, "MDEF") == 0;
		top_or_bottom = 0;
TODO(": check if mask/paste layers can be acquired explicitly as non-metal layers")
		if (mdef || (strcmp(i->layer_name, ln_top) == 0)) {
			if (!have_top) {
				hyp_pstk_add_shp(sh, &sh_used, sh_max, i, PCB_LYT_TOP | PCB_LYT_COPPER, 0, 0);
				if (with_mask) hyp_pstk_add_shp(sh, &sh_used, sh_max, i, PCB_LYT_TOP | PCB_LYT_MASK, PCB_LYC_SUB | PCB_LYC_AUTO, MASK_OFFS);
				if (with_paste) hyp_pstk_add_shp(sh, &sh_used, sh_max, i, PCB_LYT_TOP | PCB_LYT_PASTE, PCB_LYC_AUTO, PASTE_OFFS);
			}
			top_or_bottom = 1;
			have_top = 1;
		}
		if (mdef || (strcmp(i->layer_name, ln_bot) == 0)) {
			if (!have_bottom) {
				hyp_pstk_add_shp(sh, &sh_used, sh_max, i, PCB_LYT_BOTTOM | PCB_LYT_COPPER, 0, 0);
				if (with_mask) hyp_pstk_add_shp(sh, &sh_used, sh_max, i, PCB_LYT_BOTTOM | PCB_LYT_MASK, PCB_LYC_SUB | PCB_LYC_AUTO, MASK_OFFS);
				if (with_paste) hyp_pstk_add_shp(sh, &sh_used, sh_max, i, PCB_LYT_BOTTOM | PCB_LYT_PASTE, PCB_LYC_AUTO, PASTE_OFFS);
			}
			top_or_bottom = 1;
			have_bottom = 0;
		}
		if (!have_inner && (mdef || !top_or_bottom)) {
			hyp_pstk_add_shp(sh, &sh_used, sh_max, i, PCB_LYT_INTERN | PCB_LYT_COPPER, 0, 0);
			have_inner = 1;
		}
	}

	ps = pcb_pstk_new_from_shape(data, x, y, padstk->drill_size, 1, 0, sh);
	if (ps == NULL)
		pcb_message(PCB_MSG_ERROR, "Failed to convert padstack at %$mm;%$mm.\n", x, y);
	return ps;
}

/* draw padstack. Used when drawing vias, pins and pads.
 * ref is an optional string which gives pin reference as device.pin, eg. U1.VCC */
void hyp_draw_pstk(padstack_t *padstk, pcb_coord_t x, pcb_coord_t y, char *ref)
{
/* 
 * We try to map a hyperlynx padstack to its closest pcb-rnd primitive.
 * Choose between pcb_via_new(), pcb_element_pin_new(), pcb_element_pad_new() to draw a padstack.
 * if there is a drill hole but no pin reference, it's a via. Use pcb_via_new().
 * if there is a drill hole and a pin reference, it's a pin. Use pcb_element_pin_new().
 * if there is no drill hole, it's a pad. Use pcb_element_pad_new().
 */
	pcb_subc_t *subc = NULL;
	char *name = NULL;
	char *number = NULL;
	char *device_name = NULL;
	char *pin_name = NULL;
	pcb_pstk_t *pstk;
	pcb_data_t *data;

	if (padstk == NULL) {
		if (hyp_debug)
			pcb_message(PCB_MSG_DEBUG, "draw padstack: padstack not found.\n");
		return;
	}


	/* device and pin name, if any */
	device_name = NULL;
	pin_name = NULL;
	data = hyp_dest;

	if (ref != NULL) {
		char *dot;
		/* reference has format 'device_name.pin_name' */
		device_name = pcb_strdup(ref);
		dot = strrchr(device_name, '.');
		if (dot != NULL) {
			*dot = '\0';
			pin_name = pcb_strdup(dot + 1);
		}

		/* make sure device and pin name have valid values, even if reference has wrong format */
		if ((device_name == NULL) || (strcmp(device_name, "") == 0)) {
			device_name = malloc(MAX_STRING);
			pcb_sprintf(device_name, "NONAME%0d", unknown_device_number++);
		}

		if ((pin_name == NULL) || (strcmp(pin_name, "") == 0)) {
			pin_name = malloc(MAX_STRING);
			pcb_sprintf(pin_name, "NONUMBER%0d", unknown_pin_number++);
		}

		/* find device by name */
		subc = hyp_create_subc_by_name(device_name, x, y);
		data = subc->data;
	}

	/* name and number NULL if no reference given */
	name = device_name;
	number = pin_name;

	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "draw padstack: device_name = \"%s\" pin_name = \"%s\"\n", name, number);

	pstk = hyp_new_pstk(padstk, data, x, y, (subc != NULL), (subc != NULL));
	if (pin_name != NULL)
		pcb_attribute_put(&pstk->Attributes, "term", pin_name);

	if (subc != NULL) /* add pin to current net */
		hyp_netlist_add(name, number);

	return;
}


/*
 * NET record.
 */

pcb_bool exec_net(parse_param * h)
{
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "net: net_name = \"%s\"\n", h->net_name);

	net_name = pcb_strdup(h->net_name);
	net_clearance = -1;

	return 0;
}

/*
 * PS subrecord of NET record. Plane separation.
 */

pcb_bool exec_net_plane_separation(parse_param * h)
{
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "net_plane_separation: plane_separation = %ml\n", xy2coord(h->plane_separation));

	net_clearance = xy2coord(h->plane_separation);

	return 0;
}

/*
 * A subrecord of NET record. Attribute.
 */

pcb_bool exec_net_attribute(parse_param * h)
{
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "net_attribute: name = \"%s\" value = \"%s\"\n", h->name, h->value);

	return 0;
}

/*
 * SEG subrecord of NET record. line segment.
 */

pcb_bool exec_seg(parse_param * h)
{
	if (hyp_debug) {
		pcb_message(PCB_MSG_DEBUG, "seg: x1 = %ml y1 = %ml x2 = %ml y2 = %ml ", x2coord(h->x1), y2coord(h->y1), x2coord(h->x2),
								y2coord(h->y2));
		pcb_message(PCB_MSG_DEBUG, " width = %ml layer_name = \"%s\"", xy2coord(h->width), h->layer_name);
		if (h->plane_separation_set)
			pcb_message(PCB_MSG_DEBUG, " plane_separation = %ml ", xy2coord(h->plane_separation));
		if (h->left_plane_separation_set)
			pcb_message(PCB_MSG_DEBUG, " left_plane_separation = %ml ", xy2coord(h->left_plane_separation));
		pcb_message(PCB_MSG_DEBUG, "\n");
	}

	pcb_line_new(hyp_get_layer(h), x2coord(h->x1), y2coord(h->y1), x2coord(h->x2), y2coord(h->y2), xy2coord(h->width),
							 hyp_clearance(h), pcb_no_flags());

	return 0;
}

/*
 * ARC subrecord of NET record. arc segment, drawn clockwise.
 */

pcb_bool exec_arc(parse_param * h)
{

	if (hyp_debug) {
		pcb_message(PCB_MSG_DEBUG, "arc: x1 = %ml y1 = %ml x2 = %ml y2 = %ml", x2coord(h->x1), y2coord(h->y1), x2coord(h->x2),
								y2coord(h->y2));
		pcb_message(PCB_MSG_DEBUG, " xc = %ml yc = %ml r = %ml", x2coord(h->xc), y2coord(h->yc), xy2coord(h->r));
		pcb_message(PCB_MSG_DEBUG, " width = %ml layer_name = \"%s\"", xy2coord(h->width), h->layer_name);
		if (h->plane_separation_set)
			pcb_message(PCB_MSG_DEBUG, " plane_separation = %ml", xy2coord(h->plane_separation));
		if (h->left_plane_separation_set)
			pcb_message(PCB_MSG_DEBUG, " left_plane_separation = %ml", xy2coord(h->left_plane_separation));
		pcb_message(PCB_MSG_DEBUG, "\n");
	}

	hyp_arc_new(hyp_get_layer(h), x2coord(h->x1), y2coord(h->y1), x2coord(h->x2), y2coord(h->y2), x2coord(h->xc),
							y2coord(h->yc), xy2coord(h->r), xy2coord(h->r), pcb_true, xy2coord(h->width), hyp_clearance(h), pcb_no_flags());

	return 0;
}

/* 
 * Convert string-type pad shapes to numeric ones.
 */

int str2pad_shape(char *pad_shape)
{
	if (pad_shape == NULL)
		return 0;
	else if (strcmp(pad_shape, "OVAL") == 0)
		return 0;
	else if (strcmp(pad_shape, "RECT") == 0)
		return 1;
	else if (strcmp(pad_shape, "OBLONG") == 0)
		return 2;
	else
		return 0;
}

/*
 * VIA subrecord of NET record. 
 * Draws via using padstack definition.
 */

pcb_bool exec_via(parse_param * h)
{
	/* detect old-style v1.0 via */
	if (!h->padstack_name_set)
		return exec_via_v1(h);

	if (hyp_debug) {
		pcb_message(PCB_MSG_DEBUG, "via: x = %ml y = %ml", x2coord(h->x), y2coord(h->y));
		if (h->padstack_name_set)
			pcb_message(PCB_MSG_DEBUG, " padstack_name = \"%s\"", h->padstack_name);
		pcb_message(PCB_MSG_DEBUG, "\n");
	}

	if (!h->padstack_name_set) {
		if (hyp_debug)
			pcb_message(PCB_MSG_DEBUG, "pin: padstack not set. skipping pin \"%s\"\n", h->pin_reference);
		return 0;
	}

	hyp_draw_pstk(hyp_pstk_by_name(h->padstack_name), x2coord(h->x), y2coord(h->y), NULL);

	return 0;
}

/*
 * VIA subrecord of NET record. 
 * Draws deprecated v1.x via.
 */

pcb_bool exec_via_v1(parse_param * h)
{
	padstack_t *padstk;
	padstack_element_t *pad1;
	padstack_element_t *pad2;

	if (hyp_debug) {
		pcb_message(PCB_MSG_DEBUG, "old_via: x = %ml y = %ml", x2coord(h->x), y2coord(h->y));
		if (h->drill_size_set)
			pcb_message(PCB_MSG_DEBUG, " drill_size = %ml", xy2coord(h->drill_size));
		if (h->layer1_name_set)
			pcb_message(PCB_MSG_DEBUG, " layer1_name = \"%s\"", h->layer1_name);
		if (h->layer2_name_set)
			pcb_message(PCB_MSG_DEBUG, " layer2_name = \"%s\"", h->layer2_name);
		if (h->via_pad_shape_set)
			pcb_message(PCB_MSG_DEBUG, " via_pad_shape = \"%s\"", h->via_pad_shape);
		if (h->via_pad_sx_set)
			pcb_message(PCB_MSG_DEBUG, " via_pad_sx = \"%ml\"", xy2coord(h->via_pad_sx));
		if (h->via_pad_sy_set)
			pcb_message(PCB_MSG_DEBUG, " via_pad_sy = \"%ml\"", xy2coord(h->via_pad_sy));
		if (h->via_pad_angle_set)
			pcb_message(PCB_MSG_DEBUG, " via_pad_angle = \"%f\"", h->via_pad_angle);
		if (h->via_pad1_shape_set)
			pcb_message(PCB_MSG_DEBUG, " via_pad1_shape = \"%s\"", h->via_pad1_shape);
		if (h->via_pad1_sx_set)
			pcb_message(PCB_MSG_DEBUG, " via_pad1_sx = \"%ml\"", xy2coord(h->via_pad1_sx));
		if (h->via_pad1_sy_set)
			pcb_message(PCB_MSG_DEBUG, " via_pad1_sy = \"%ml\"", xy2coord(h->via_pad1_sy));
		if (h->via_pad1_angle_set)
			pcb_message(PCB_MSG_DEBUG, " via_pad1_angle = \"%f\"", h->via_pad1_angle);
		if (h->via_pad2_shape_set)
			pcb_message(PCB_MSG_DEBUG, " via_pad2_shape = \"%s\"", h->via_pad2_shape);
		if (h->via_pad2_sx_set)
			pcb_message(PCB_MSG_DEBUG, " via_pad2_sx = \"%ml\"", xy2coord(h->via_pad2_sx));
		if (h->via_pad2_sy_set)
			pcb_message(PCB_MSG_DEBUG, " via_pad2_sy = \"%ml\"", xy2coord(h->via_pad2_sy));
		if (h->via_pad2_angle_set)
			pcb_message(PCB_MSG_DEBUG, " via_pad2_angle = \"%f\"", h->via_pad2_angle);
		pcb_message(PCB_MSG_DEBUG, "\n");
	}

	/* create padstack for this via */
	padstk = malloc(sizeof(padstack_t));
	if (padstk == NULL)
		return 1;
	pad1 = malloc(sizeof(padstack_element_t));
	if (pad1 == NULL)
		return 1;
	pad2 = malloc(sizeof(padstack_element_t));
	if (pad2 == NULL)
		return 1;

	padstk->name = "*** VIA ***";
	padstk->drill_size = xy2coord(h->drill_size);
	padstk->padstack = pad1;
	padstk->next = NULL;

	pad1->layer_name = h->layer1_name;
	pad1->pad_shape = str2pad_shape(h->via_pad1_shape);
	pad1->pad_sx = xy2coord(h->via_pad1_sx);
	pad1->pad_sy = xy2coord(h->via_pad1_sy);
	pad1->pad_angle = h->via_pad1_angle;
	pad1->thermal_clear_sx = 0;
	pad1->thermal_clear_sy = 0;
	pad1->thermal_clear_angle = 0.0;
	pad1->pad_type = PAD_TYPE_METAL;

	if (h->layer2_name_set && h->via_pad2_sx_set && h->via_pad2_sy_set) {
		pad1->next = pad2;
		pad2->layer_name = h->layer2_name;
		pad2->pad_shape = str2pad_shape(h->via_pad2_shape);
		pad2->pad_sx = xy2coord(h->via_pad2_sx);
		pad2->pad_sy = xy2coord(h->via_pad2_sy);
		pad2->pad_angle = h->via_pad2_angle;
		pad2->thermal_clear_sx = 0;
		pad2->thermal_clear_sy = 0;
		pad2->thermal_clear_angle = 0.0;
		pad2->pad_type = PAD_TYPE_METAL;
		pad2->next = NULL;
	}
	else
		pad1->next = NULL;

	/* draw padstack */
	hyp_draw_pstk(padstk, x2coord(h->x), y2coord(h->y), NULL);

	/* free padstack for this via */
	free(pad2);
	free(pad1);
	free(padstk);

	return 0;
}

/*
 * PIN subrecord of NET record. 
 * Draws PIN using padstack definiton.
 */

pcb_bool exec_pin(parse_param * h)
{
	if (hyp_debug) {
		pcb_message(PCB_MSG_DEBUG, "pin: x = %ml y = %ml", x2coord(h->x), y2coord(h->y));
		pcb_message(PCB_MSG_DEBUG, " pin_reference = \"%s\"", h->pin_reference);
		if (h->padstack_name_set)
			pcb_message(PCB_MSG_DEBUG, " padstack_name = \"%s\"", h->padstack_name);
		if (h->pin_function_set)
			pcb_message(PCB_MSG_DEBUG, " pin_function = %i", h->pin_function);
		pcb_message(PCB_MSG_DEBUG, "\n");
	}

	if (!h->padstack_name_set) {
		if (hyp_debug)
			pcb_message(PCB_MSG_DEBUG, "pin: padstack not set. skipping pin \"%s\"\n", h->pin_reference);
		return 0;
	}

	hyp_draw_pstk(hyp_pstk_by_name(h->padstack_name), x2coord(h->x), y2coord(h->y), h->pin_reference);

	return 0;
}

/*
 * PAD subrecord of NET record. 
 * Draws deprecated v1.x pad.
 */

pcb_bool exec_pad(parse_param * h)
{
	padstack_t *padstk;
	padstack_element_t *pad;

	if (hyp_debug) {
		pcb_message(PCB_MSG_DEBUG, "pad: x = %ml y = %ml", x2coord(h->x), y2coord(h->y));
		if (h->layer_name_set)
			pcb_message(PCB_MSG_DEBUG, " layer_name = \"%s\"", h->layer_name);
		if (h->via_pad_shape_set)
			pcb_message(PCB_MSG_DEBUG, " via_pad_shape = \"%s\"", h->via_pad_shape);
		if (h->via_pad_sx_set)
			pcb_message(PCB_MSG_DEBUG, " via_pad_sx = \"%ml\"", xy2coord(h->via_pad_sx));
		if (h->via_pad_sy_set)
			pcb_message(PCB_MSG_DEBUG, " via_pad_sy = \"%ml\"", xy2coord(h->via_pad_sy));
		if (h->via_pad_angle_set)
			pcb_message(PCB_MSG_DEBUG, " via_pad_angle = \"%f\"", h->via_pad_angle);
		pcb_message(PCB_MSG_DEBUG, "\n");
	}

	if (!h->layer_name_set) {
		if (hyp_debug)
			pcb_message(PCB_MSG_DEBUG, "pad: layer name not set. skipping pad\n.");
		return 0;
	}

	/* create padstack for this pad */
	padstk = malloc(sizeof(padstack_t));
	if (padstk == NULL)
		return 1;
	pad = malloc(sizeof(padstack_element_t));
	if (pad == NULL)
		return 1;

	padstk->name = "*** PAD ***";
	padstk->drill_size = 0.0;
	padstk->padstack = pad;
	padstk->next = NULL;

	pad->layer_name = h->layer_name;
	pad->pad_shape = str2pad_shape(h->via_pad_shape);
	pad->pad_sx = xy2coord(h->via_pad_sx);
	pad->pad_sy = xy2coord(h->via_pad_sy);
	pad->pad_angle = h->via_pad_angle;
	pad->thermal_clear_sx = 0;
	pad->thermal_clear_sy = 0;
	pad->thermal_clear_angle = 0;
	pad->pad_type = PAD_TYPE_METAL;
	pad->next = NULL;

	/* draw padstack */
	hyp_draw_pstk(padstk, x2coord(h->x), y2coord(h->y), NULL);

	/* free padstack for this pad */
	free(pad);
	free(padstk);

	return 0;
}

/*
 * USEG subrecord of NET record.
 * unrouted segment.
 */

pcb_bool exec_useg(parse_param * h)
{
	pcb_layergrp_id_t layer1_grp_id, layer2_grp_id;

	if (hyp_debug) {
		pcb_message(PCB_MSG_DEBUG, "useg: x1 = %ml y1 = %ml layer1_name = \"%s\"", x2coord(h->x1), y2coord(h->y1), h->layer1_name);
		pcb_message(PCB_MSG_DEBUG, " x2 = %ml y2 = %ml layer2_name = \"%s\"", x2coord(h->x2), y2coord(h->y2), h->layer2_name);
		if (h->zlayer_name_set)
			pcb_message(PCB_MSG_DEBUG, " zlayer_name = \"%s\" width = %ml length = %ml", h->zlayer_name, xy2coord(h->width),
									xy2coord(h->length));
		if (h->impedance_set)
			pcb_message(PCB_MSG_DEBUG, " impedance = %f delay = %f ", h->impedance, h->delay);
		if (h->resistance_set)
			pcb_message(PCB_MSG_DEBUG, " resistance = %f ", h->resistance);
		pcb_message(PCB_MSG_DEBUG, "\n");
	}

	/* lookup layer group begin and end layer are on */
	layer1_grp_id = pcb_layer_get_group(PCB, hyp_create_layer(h->layer1_name));
	layer2_grp_id = pcb_layer_get_group(PCB, hyp_create_layer(h->layer2_name));

	if ((layer1_grp_id == -1) || (layer2_grp_id == -1)) {
		if (hyp_debug)
			pcb_message(PCB_MSG_DEBUG, "useg: skipping unrouted segment\n");
		return 0;
	}

	pcb_rat_new(hyp_dest, -1, x2coord(h->x1), y2coord(h->y1), x2coord(h->x2), y2coord(h->y2), layer1_grp_id, layer2_grp_id,
							xy2coord(h->width), pcb_no_flags(), NULL, NULL);

	return 0;
}

/*
 * POLYGON subrecord of NET record.
 * draws copper polygon.
 */

pcb_bool exec_polygon_begin(parse_param * h)
{
	hyp_polygon_t *new_poly;
	hyp_vertex_t *new_vertex;
	hyp_polygon_t *i;

	if (hyp_debug) {
		pcb_message(PCB_MSG_DEBUG, "polygon begin:");
		if (h->layer_name_set)
			pcb_message(PCB_MSG_DEBUG, " layer_name = \"%s\"", h->layer_name);
		if (h->width_set)
			pcb_message(PCB_MSG_DEBUG, " width = %ml", xy2coord(h->width));
		if (h->polygon_type_set) {
			pcb_message(PCB_MSG_DEBUG, " polygon_type = ", h->polygon_type, " ");
			switch (h->polygon_type) {
			case POLYGON_TYPE_PLANE:
				pcb_message(PCB_MSG_DEBUG, "POLYGON_TYPE_PLANE");
				break;
			case POLYGON_TYPE_POUR:
				pcb_message(PCB_MSG_DEBUG, "POLYGON_TYPE_POUR");
				break;
			case POLYGON_TYPE_COPPER:
				pcb_message(PCB_MSG_DEBUG, "POLYGON_TYPE_COPPER");
				break;
			default:
				pcb_message(PCB_MSG_DEBUG, "Error");
				break;
			}
		}
		if (h->id_set)
			pcb_message(PCB_MSG_DEBUG, " id = %i", h->id);
		pcb_message(PCB_MSG_DEBUG, " x = %ml y = %ml\n", x2coord(h->x), y2coord(h->y));
	}

	if (!h->layer_name_set) {
		hyp_error("expected polygon layer L = ");
		return pcb_true;
	}

	if (!h->id_set) {
		hyp_error("expected polygon id ID = ");
		return pcb_true;
	}

	/* make sure layer exists */
	hyp_create_layer(h->layer_name);

	/* check for other polygons with this id */
	if (hyp_debug)
		for (i = polygon_head; i != NULL; i = i->next)
			if (h->id == i->hyp_poly_id) {
				pcb_message(PCB_MSG_INFO, "info: duplicate polygon id %i.\n", h->id);
				break;
			}

	/* create first vertex */
	new_vertex = malloc(sizeof(hyp_vertex_t));
	new_vertex->x1 = x2coord(h->x);
	new_vertex->y1 = y2coord(h->y);
	new_vertex->x2 = 0;
	new_vertex->y2 = 0;
	new_vertex->xc = 0;
	new_vertex->yc = 0;
	new_vertex->r = 0;
	new_vertex->is_arc = pcb_false;
	new_vertex->is_first = pcb_true;
	new_vertex->next = NULL;

	/* create new polygon */
	new_poly = malloc(sizeof(hyp_polygon_t));
	new_poly->hyp_poly_id = h->id;	/* hyperlynx polygon/polyline id */
	new_poly->hyp_poly_type = h->polygon_type;
	new_poly->is_polygon = pcb_true;
	new_poly->layer_name = h->layer_name;
	new_poly->line_width = xy2coord(h->width);
	new_poly->clearance = hyp_clearance(h);
	new_poly->vertex = new_vertex;

	new_poly->next = polygon_head;
	polygon_head = new_poly;

	/* bookkeeping */
	current_vertex = new_vertex;

	return 0;
}

pcb_bool exec_polygon_end(parse_param * h)
{
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "polygon end:\n");

	/* bookkeeping */
	if ((current_vertex == NULL) && hyp_debug)
		pcb_message(PCB_MSG_WARNING, "polygon: unexpected polygon end. continuing.\n");
	current_vertex = NULL;

	return 0;
}

/*
 * POLYVOID subrecord of NET record.
 * same as POLYGON, but instead of creating copper, creates a hole in copper.
 */

pcb_bool exec_polyvoid_begin(parse_param * h)
{
	hyp_polygon_t *i;
	hyp_vertex_t *new_vertex;

	if (hyp_debug) {
		pcb_message(PCB_MSG_DEBUG, "polyvoid begin:");
		if (h->id_set)
			pcb_message(PCB_MSG_DEBUG, " id = %i", h->id);
		pcb_message(PCB_MSG_DEBUG, " x = %ml y = %ml\n", x2coord(h->x), y2coord(h->y));
	}

	if (!h->id_set) {
		hyp_error("expected polygon id ID = ");
		return pcb_true;
	}

	/* look up polygon with this id */
	for (i = polygon_head; i != NULL; i = i->next)
		if (h->id == i->hyp_poly_id)
			break;

	if (i == NULL) {
		current_vertex = NULL;
		pcb_message(PCB_MSG_WARNING, "polyvoid: polygon id %i not found\n", h->id);
		return 0;
	}

	/* find last vertex of polygon */
	for (current_vertex = i->vertex; current_vertex != NULL; current_vertex = current_vertex->next)
		if (current_vertex->next == NULL)
			break;

	assert(current_vertex != NULL);	/* at least one vertex in polygon */

	/* add new vertex at end of list of vertices */
	new_vertex = malloc(sizeof(hyp_vertex_t));
	new_vertex->x1 = x2coord(h->x);
	new_vertex->y1 = y2coord(h->y);
	new_vertex->x2 = 0;
	new_vertex->y2 = 0;
	new_vertex->xc = 0;
	new_vertex->yc = 0;
	new_vertex->r = 0;
	new_vertex->is_arc = pcb_false;
	new_vertex->is_first = pcb_true;
	new_vertex->next = NULL;

	/* bookkeeping */
	if (current_vertex != NULL) {
		current_vertex->next = new_vertex;
		current_vertex = new_vertex;
	}

	return 0;
}

pcb_bool exec_polyvoid_end(parse_param * h)
{
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "polyvoid end:\n");

	/* bookkeeping */
	if ((current_vertex == NULL) && hyp_debug)
		pcb_message(PCB_MSG_WARNING, "polyvoid: unexpected polyvoid end. continuing.\n");
	current_vertex = NULL;

	return 0;
}

/*
 * POLYLINE subrecord of NET record.
 */

pcb_bool exec_polyline_begin(parse_param * h)
{
	hyp_polygon_t *new_poly;
	hyp_vertex_t *new_vertex;
	hyp_polygon_t *i;

	if (hyp_debug) {
		pcb_message(PCB_MSG_DEBUG, "polyline begin:");
		if (h->layer_name_set)
			pcb_message(PCB_MSG_DEBUG, " layer_name = \"%s\"", h->layer_name);
		if (h->width_set)
			pcb_message(PCB_MSG_DEBUG, " width = %ml", xy2coord(h->width));
		if (h->polygon_type_set) {
			pcb_message(PCB_MSG_DEBUG, " polygon_type = ", h->polygon_type, " ");
			switch (h->polygon_type) {
			case POLYGON_TYPE_PLANE:
				pcb_message(PCB_MSG_DEBUG, "POLYGON_TYPE_PLANE");
				break;
			case POLYGON_TYPE_POUR:
				pcb_message(PCB_MSG_DEBUG, "POLYGON_TYPE_POUR");
				break;
			case POLYGON_TYPE_COPPER:
				pcb_message(PCB_MSG_DEBUG, "POLYGON_TYPE_COPPER");
				break;
			default:
				pcb_message(PCB_MSG_DEBUG, "Error");
				break;
			}
		}
		if (h->id_set)
			pcb_message(PCB_MSG_DEBUG, " id = %i", h->id);
		pcb_message(PCB_MSG_DEBUG, " x = %ml y = %ml\n", x2coord(h->x), y2coord(h->y));
	}

	if (!h->layer_name_set) {
		hyp_error("expected polygon layer L = ");
		return pcb_true;
	}

	if (!h->width_set) {
		hyp_error("expected polygon width W = ");
		return pcb_true;
	}

	if (!h->id_set) {
		hyp_error("expected polygon id ID = ");
		return pcb_true;
	}

	/* make sure layer exists */
	hyp_create_layer(h->layer_name);

	/* check for other polygons with this id */
	if (hyp_debug)
		for (i = polygon_head; i != NULL; i = i->next)
			if (h->id == i->hyp_poly_id) {
				pcb_message(PCB_MSG_DEBUG, "info: duplicate polygon id %i.\n", h->id);
				break;
			}

	/* create first vertex */
	new_vertex = malloc(sizeof(hyp_vertex_t));
	new_vertex->x1 = x2coord(h->x);
	new_vertex->y1 = y2coord(h->y);
	new_vertex->x2 = 0;
	new_vertex->y2 = 0;
	new_vertex->xc = 0;
	new_vertex->yc = 0;
	new_vertex->r = 0;
	new_vertex->is_arc = pcb_false;
	new_vertex->is_first = pcb_true;
	new_vertex->next = NULL;

	/* create new polyline */
	new_poly = malloc(sizeof(hyp_polygon_t));
	new_poly->hyp_poly_id = h->id;	/* hyperlynx polygon/polyline id */
	new_poly->hyp_poly_type = h->polygon_type;
	new_poly->is_polygon = pcb_false;
	new_poly->layer_name = h->layer_name;
	new_poly->line_width = xy2coord(h->width);
	new_poly->clearance = hyp_clearance(h);
	new_poly->vertex = new_vertex;

	new_poly->next = polygon_head;
	polygon_head = new_poly;

	/* bookkeeping */
	current_vertex = new_vertex;

	return 0;
}

pcb_bool exec_polyline_end(parse_param * h)
{
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "polyline end:\n");

	/* bookkeeping */
	if ((current_vertex == NULL) && hyp_debug)
		pcb_message(PCB_MSG_WARNING, "polyline: unexpected polyline end. continuing.\n");
	current_vertex = NULL;

	return 0;
}

/*
 * LINE subrecord of NET record.
 */

pcb_bool exec_line(parse_param * h)
{
	hyp_vertex_t *new_vertex;

	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "line: x = %ml y = %ml\n", x2coord(h->x), y2coord(h->y));

	if (current_vertex == NULL) {
		pcb_message(PCB_MSG_DEBUG, "line: skipping.");
		return 0;
	}

	/* add new vertex at end of list of vertices */
	new_vertex = malloc(sizeof(hyp_vertex_t));
	new_vertex->x1 = x2coord(h->x);
	new_vertex->y1 = y2coord(h->y);
	new_vertex->x2 = 0;
	new_vertex->y2 = 0;
	new_vertex->xc = 0;
	new_vertex->yc = 0;
	new_vertex->r = 0;
	new_vertex->is_arc = pcb_false;
	new_vertex->is_first = pcb_false;
	new_vertex->next = NULL;

	/* bookkeeping */
	current_vertex->next = new_vertex;
	current_vertex = new_vertex;

	return 0;
}

/*
 * CURVE subrecord of NET record. 
 * Clockwise from (x1, y1) to (x2, y2).
 */

pcb_bool exec_curve(parse_param * h)
{
	hyp_vertex_t *new_vertex;

	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "curve: x1 = %ml y1 = %ml x2 = %ml y2 = %ml xc = %ml yc = %ml r = %ml\n", x2coord(h->x1),
								y2coord(h->y1), x2coord(h->x2), y2coord(h->y2), x2coord(h->xc), y2coord(h->yc), xy2coord(h->r));

	if (current_vertex == NULL) {
		pcb_message(PCB_MSG_DEBUG, "curve: skipping.");
		return 0;
	}

	/* add new vertex at end of list of vertices */
	new_vertex = malloc(sizeof(hyp_vertex_t));
	new_vertex->x1 = x2coord(h->x1);
	new_vertex->y1 = y2coord(h->y1);
	new_vertex->x2 = x2coord(h->x2);
	new_vertex->y2 = y2coord(h->y2);
	new_vertex->xc = x2coord(h->xc);
	new_vertex->yc = y2coord(h->yc);
	new_vertex->r = xy2coord(h->r);
	new_vertex->is_arc = pcb_true;
	new_vertex->is_first = pcb_false;
	new_vertex->next = NULL;

	/* bookkeeping */
	current_vertex->next = new_vertex;
	current_vertex = new_vertex;

	return 0;
}

/*
 * NET_CLASS record
 */

pcb_bool exec_net_class(parse_param * h)
{
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "net_class: net_class_name = \"%s\"\n", h->net_class_name);

	return 0;
}

/*
 * N net membership subrecord of NET_CLASS record
 */

pcb_bool exec_net_class_element(parse_param * h)
{
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "net_class_element: net_name = \"%s\"\n", h->net_name);

	return 0;
}

/*
 * A attribute subrecord of NET_CLASS record
 */


pcb_bool exec_net_class_attribute(parse_param * h)
{
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "netclass_attribute: name = \"%s\" value = \"%s\"\n", h->name, h->value);

	return 0;
}

/* 
 * KEY record
 */

pcb_bool exec_key(parse_param * h)
{
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "key: key = \"%s\"\n", h->key);

	return 0;
}

/* 
 * END record
 */

pcb_bool exec_end(parse_param * h)
{
	if (hyp_debug)
		pcb_message(PCB_MSG_DEBUG, "end:\n");
	return 0;
}

/* not truncated */

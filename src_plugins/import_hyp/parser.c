/*
 * read hyperlynx files
 * Copyright 2016 Koen De Vleeschauwer.
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

/* 
 * ~/.indent.pro:
 *   --line-length128 -brs -br -nce --tab-size2 -ut -npsl -npcs -hnl
 * ~/.vimrc:
 *   set tabstop=2
 */

#include "parser.h"
#include "hyp_l.h"
#include "hyp_y.h"
#include "error.h"
#include "pcb-printf.h"
#include "obj_all.h"
#include "flag_str.h"
#include "polygon.h"
#include "board.h"
#include "layer.h"
#include "data.h"
#include "search.h"
#include "compat_misc.h"

/*
 * the board is shared between all routines.
 */

pcb_data_t *hyp_dest;

/*
 * board outline is doubly linked list of arcs and line segments.
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
void hyp_create_polygons();			/* add all created polygons to pcb */

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

/* devices */

typedef struct device_s {
	char *ref;
	char *name;										/* optional */
	char *value;									/* optional */
	char *layer_name;
	struct device_s *next;
} device_t;

device_t *device_head;

/* padstack */

typedef struct padstack_s {
	char *name;
	pcb_coord_t thickness;
	pcb_coord_t clearance;
	pcb_coord_t mask;
	pcb_coord_t drill_hole;
	pcb_flag_t flags;							/* round, rectangular or octagon */
	struct padstack_s *next;
} padstack_t;

padstack_t *padstack_head;
padstack_t *current_padstack;

/* polygons */

/* linked list to store correspondence between hyperlynx polygon id's and pcb polygons. */
typedef struct hyp_polygon_s {
	int hyp_poly_id;							/* hyperlynx polygon/polyline id */
	pcb_polygon_t *polygon;
	pcb_layer_t *layer;
	pcb_bool_t is_polygon;				/* whether polygon or polyline */
	struct hyp_polygon_s *next;
} hyp_polygon_t;

hyp_polygon_t *polygon_head = NULL;

/* keep track of current polygon */

pcb_polygon_t *current_polygon = NULL;

/* current polyline layer, position, width and clearance */
pcb_layer_t *current_polyline_layer = NULL;
pcb_coord_t current_polyline_xpos = 0;
pcb_coord_t current_polyline_ypos = 0;
pcb_coord_t current_polyline_width = 0;
pcb_coord_t current_polyline_clearance = 0;

/* State machine for polygon parsing:
 * HYP_POLYIDLE idle
 * HYP_POLYGON parsing a polygon
 * HYP_POLYLINE parsing a polyline (sequence of lines and arcs)
 * HYP_POLYGON_HOLE parsing a hole in a polygon
 * HYP_POLYLINE_HOLE parsing a hole in a polyline
 */

enum poly_state_e { HYP_POLYIDLE, HYP_POLYGON, HYP_POLYLINE, HYP_POLYGON_HOLE, HYP_POLYLINE_HOLE };
enum poly_state_e poly_state = HYP_POLYIDLE;

/* origin. Chosen so all coordinates are positive. */
pcb_coord_t origin_x;
pcb_coord_t origin_y;

/*
 * Conversion from hyperlynx to pcb_coord_t
 */

/* meter to pcb_coord_t */

pcb_coord_t inline m2coord(double m)
{
	return ((pcb_coord_t) PCB_MM_TO_COORD(1000.0 * m));
}

/* xy coordinates to pcb_coord_t, without offset */

pcb_coord_t inline xy2coord(double f)
{
	return (m2coord(unit * f));
}


/* x coordinates to pcb_coord_t, with offset */

pcb_coord_t inline x2coord(double f)
{
	return (m2coord(unit * f) - origin_x);
}

/* y coordinates to pcb_coord_t, with offset */

pcb_coord_t inline y2coord(double f)
{
	return (origin_y - m2coord(unit * f));
}

/* z coordinates to pcb_coord_t. No offset needed. */

pcb_coord_t inline z2coord(double f)
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

	/* clear padstack */
	padstack_head = NULL;
	current_padstack = NULL;

	/* clear devices */
	device_head = NULL;

	/* clear polygon data */
	polygon_head = NULL;
	current_polygon = NULL;
	poly_state = HYP_POLYIDLE;

	/* clear polyline data */
	current_polyline_layer = NULL;
	current_polyline_xpos = 0;
	current_polyline_ypos = 0;
	current_polyline_width = 0;
	current_polyline_clearance = 0;

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

	hyp_init();

	/* set debug levels */
	hyyset_debug(debug > 2);			/* switch flex logging on */
	hyydebug = (debug > 1);				/* switch bison logging on */
	hyp_debug = (debug > 0);			/* switch hyperlynx logging on */

	/* set shared board */
	hyp_dest = dest;

	/* parse hyperlynx file */
	hyyin = fopen(fname, "r");
	if (hyyin == NULL)
		return (1);
	retval = hyyparse();
	fclose(hyyin);

	/* finish setting up polygons */
	hyp_create_polygons();

	/* add board outline last */
	hyp_perimeter();

	/* clear */
	hyp_dest = NULL;

	return (retval);
}

/* print error message */
void hyp_error(const char *msg)
{
	pcb_message(PCB_MSG_DEBUG, "line %d: %s at '%s'\n", hyylineno, msg, hyytext);
}

/*
 * find padstack by name 
 */

padstack_t *hyp_padstack_by_name(char *padstack_name)
{
	padstack_t *i;
	for (i = padstack_head; i != NULL; i = i->next)
		if (strcmp(i->name, padstack_name) == 0)
			return i;
	return NULL;
}

/*
 * find hyperlynx device by name 
 */

device_t *hyp_device_by_name(char *device_name)
{
	device_t *i;
	for (i = device_head; i != NULL; i = i->next)
		if (strcmp(i->ref, device_name) == 0)
			return i;
	return NULL;
}

/*
 * create pcb element by name
 * parameters: name, (x, y) coordinates of element text.
 */

pcb_element_t *hyp_create_element_by_name(char *element_name, pcb_coord_t x, pcb_coord_t y)
{
	pcb_element_t *elem;
	pcb_flag_t flags;
	pcb_uint8_t text_direction = 0;
	int text_scale = 100;

	flags = pcb_no_flags();
	/* does the device already exist? */
	elem = pcb_search_elem_by_name(hyp_dest, element_name);
	if (elem == NULL) {
		/* device needs to be created. Search in DEVICE records. */
		device_t *dev = hyp_device_by_name(element_name);
		if (dev != NULL) {
			/* device on component or solder side? */
			if ((dev->layer_name != NULL) && (pcb_layer_flags(pcb_layer_by_name(dev->layer_name)) & PCB_LYT_BOTTOM))
				pcb_flag_add(flags, PCB_FLAG_ONSOLDER);
			/* create */
			if (hyp_debug)
				pcb_printf("creating device \"%s\".\n", dev->ref);
			elem =
				pcb_element_new(hyp_dest, NULL, pcb_font(PCB, 0, 1), flags, dev->name, dev->ref, dev->value, x, y, text_direction,
												text_scale, pcb_no_flags(), pcb_false);
		}
		else {
			/* no device with this name exists, and no such device has been listed in a DEVICE record. Let's create the device anyhow so we can continue. Assume device is on component side. */
			pcb_printf("device \"%s\" not specified in DEVICE record. continuing.\n", element_name);
			elem =
				pcb_element_new(hyp_dest, NULL, pcb_font(PCB, 0, 1), pcb_no_flags(), element_name, element_name, NULL, x, y, text_direction,
												text_scale, pcb_no_flags(), pcb_false);
		}
	}

	return elem;
}

/*
 * add segment to board outline
 */

void hyp_perimeter_segment_add(outline_t * s, pcb_bool_t forward)
{
	pcb_layer_id_t outline_id;
	pcb_layer_t *outline_layer;

	/* get outline layer */
	outline_id = pcb_layer_by_name("outline");
	if (outline_id < 0)
		outline_id = pcb_layer_create(-1, "outline");
	outline_layer = pcb_get_layer(outline_id);

	if (outline_layer == NULL) {
		pcb_printf("create outline layer failed.\n");
		return;
	}

	/* mark segment as used, so we don't use it twice */
	s->used = pcb_true;

	/* debugging */
	if (hyp_debug) {
		if (forward)
			pcb_printf("outline: fwd %s from (%ml, %ml) to (%ml, %ml)\n", s->is_arc ? "arc" : "line", s->x1, s->y1, s->x2, s->y2);
		else
			pcb_printf("outline: bwd %s from (%ml, %ml) to (%ml, %ml)\n", s->is_arc ? "arc" : "line", s->x2, s->y2, s->x1, s->y1);	/* add segment back to front */
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
			pcb_printf("perimeter: last_x = %ml last_y = %ml\n", last_x, last_y);
			for (i = outline_head; i != NULL; i = i->next)
				if (!i->used)
					pcb_printf("perimeter segments available: %s from (%ml, %ml) to (%ml, %ml)\n", i->is_arc ? "arc " : "line", i->x1,
										 i->y1, i->x2, i->y2);
#endif

			/* find segment to add to current polygon */
			segment_found = pcb_false;

			/* XXX prefer closed polygon over open polyline */

			for (i = outline_head; i != NULL; i = i->next) {
				if (i->used)
					continue;

				if ((last_x == i->x1) && (last_y == i->y1)) {
					if (!hyp_segment_connected(i->x2, i->y2, begin_x, begin_y, i))
						continue;						/* XXX Checkme */
					/* first point of segment is last point of current edge: add segment to edge */
					segment_found = pcb_true;
					hyp_perimeter_segment_add(i, pcb_true);
					last_x = i->x2;
					last_y = i->y2;
				}
				else if ((last_x == i->x2) && (last_y == i->y2)) {
					if (!hyp_segment_connected(i->x1, i->y1, begin_x, begin_y, i))
						continue;						/* XXX Checkme */
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
				pcb_printf("outline: closed\n");
		}
		else {
			if (hyp_debug)
				pcb_printf("outline: open\n");
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
		pcb_message(PCB_MSG_DEBUG, "warning: board outline not closed\n");

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
	char new_layer_name[PCB_MAX_LAYER];
	int n;

	layer_id = -1;
	if (lname != NULL) {
		/* we have a layer name. check whether layer already exists */
		layer_id = pcb_layer_by_name(lname);
		if (layer_id >= 0)
			return layer_id;					/* found */
		/* create new layer */
		if (hyp_debug)
			pcb_printf("create layer \"%s\"\n", lname);
		layer_id = pcb_layer_create(-1, pcb_strdup(lname));
	}
	else {
		/* no layer name given. find unused layer name in range 1..PCB_MAX_LAYER */
		for (n = 1; n < PCB_MAX_LAYER; n++) {
			pcb_sprintf(new_layer_name, "%i", n);
			if (pcb_layer_by_name(new_layer_name) < 0) {
				/* create new layer */
				if (hyp_debug)
					pcb_printf("create auto layer \"%s\"\n", new_layer_name);
				layer_id = pcb_layer_create(-1, pcb_strdup(new_layer_name));
				break;
			}
		}
	}
	/* check if layer valid */
	if (layer_id < 0) {
		/* layer creation failed. return the first copper layer you can find. */
		if (hyp_debug)
			pcb_printf("running out of layers\n");
		if (pcb_layer_list(PCB_LYT_COPPER, &layer_id, 1) <= 0) {
			if (hyp_debug)
				pcb_printf("fatal: no copper layers.\n");
			layer_id = 0;
		}
	}

	return layer_id;
}

/*
 * convert hyperlynx layer name to pcb layer
 */

pcb_layer_t *hyp_get_layer(parse_param * h)
{
	return pcb_get_layer(hyp_create_layer(h->layer_name));
}

/*
 * create all polygons 
 */

void hyp_create_polygons()
{
	hyp_polygon_t *i;

	/* loop over all created polygons and close them */
	for (i = polygon_head; i != NULL; i = i->next)
		if ((i->is_polygon) && (i->polygon != NULL) && (i->layer != NULL)) {
			pcb_add_polygon_on_layer(i->layer, i->polygon);
			pcb_poly_init_clip(hyp_dest, i->layer, i->polygon);
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

#undef XXX
#ifdef XXX
	if (hyp_debug)
		pcb_printf("hyp_arc: start_angle: %f end_angle: %f delta: %f\n", start_angle, end_angle, delta);
#endif

	new_arc = pcb_arc_new(Layer, XC, YC, Width, Height, start_angle, delta, Thickness, Clearance, Flags);

#ifdef XXX
	/* XXX remove debugging code */
	if (hyp_debug) {
		pcb_coord_t x1, y1, x2, y2;
		pcb_arc_get_end(new_arc, 1, &x1, &y1);
		pcb_arc_get_end(new_arc, 0, &x2, &y2);
		pcb_printf("hyp_arc: start_point: (%ml, %ml) end_point: (%ml, %ml)\n", x1, y1, x2, y2);
	}
#endif

	return new_arc;
}

/* exec_* routines are called by parser to interpret hyperlynx file */

/*
 * 'BOARD_FILE' record.
 */

pcb_bool exec_board_file(parse_param * h)
{
	if (hyp_debug)
		pcb_printf("board:\n");

	return 0;
}

/*
 * 'VERSION' record.
 * Specifies version number.
 */

pcb_bool exec_version(parse_param * h)
{
	if (hyp_debug)
		pcb_printf("version: vers = %f\n", h->vers);

	if (h->vers < 1.0)
		pcb_message(PCB_MSG_DEBUG, "warning: version 1.x deprecated\n");

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
		pcb_printf("data_mode: detailed = %i\n", h->detailed);

	return 0;
}

/*
 * 'UNITS' record.
 * Specifies measurement system (english/metric) for the rest of the file.
 */

pcb_bool exec_units(parse_param * h)
{
	if (hyp_debug)
		pcb_printf("units: unit_system_english = %d metal_thickness_weight = %d\n", h->unit_system_english,
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
		pcb_printf("units: unit = %f metal_thickness_unit = %f\n", unit, metal_thickness_unit);

	return 0;
}

/*
 * 'PLANE_SEP' record.
 * Defines default trace to plane clearance.
 */

pcb_bool exec_plane_sep(parse_param * h)
{
	board_clearance = xy2coord(h->plane_separation);

	if (hyp_debug)
		pcb_printf("plane_sep: default_plane_separation = %ml\n", board_clearance);

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
		pcb_printf("perimeter_segment: x1 = %ml y1 = %ml x2 = %ml y2 = %ml\n", peri_seg->x1, peri_seg->y1, peri_seg->x2,
							 peri_seg->y2);

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
		pcb_printf("perimeter_arc: x1 = %ml y1 = %ml x2 = %ml y2 = %ml xc = %ml yc = %ml r = %ml\n", peri_arc->x1, peri_arc->y1,
							 peri_arc->x2, peri_arc->y2, peri_arc->xc, peri_arc->yc, peri_arc->r);

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
		pcb_printf("board_attribute: name = \"%s\" value = \"%s\"\n", h->name, h->value);

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
			pcb_printf(" thickness = %ml", z2coord(h->thickness));
		if (h->plating_thickness_set)
			pcb_printf(" plating_thickness = %ml", z2coord(h->plating_thickness));
		if (h->bulk_resistivity_set)
			pcb_printf(" bulk_resistivity = %f", h->bulk_resistivity);
		if (h->temperature_coefficient_set)
			pcb_printf(" temperature_coefficient = %f", h->temperature_coefficient);
		if (h->epsilon_r_set)
			pcb_printf(" epsilon_r = %f", h->epsilon_r);
		if (h->loss_tangent_set)
			pcb_printf(" loss_tangent = %f", h->loss_tangent);
		if (h->conformal_set)
			pcb_printf(" conformal = %i", h->conformal);
		if (h->prepreg_set)
			pcb_printf(" prepreg = %i", h->prepreg);
		if (h->layer_name_set)
			pcb_printf(" layer_name = \"%s\"", h->layer_name);
		if (h->material_name_set)
			pcb_printf(" material_name = \"%s\"", h->material_name);
		if (h->plane_separation_set)
			pcb_printf(" plane_separation = %ml", xy2coord(h->plane_separation));
		pcb_printf("\n");
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
		pcb_printf("options: use_die_for_metal = %f\n", h->use_die_for_metal);
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
	signal_layer_id = hyp_create_layer(h->layer_name);

	layer_is_plane[signal_layer_id] = pcb_false;
	if (h->plane_separation_set)
		layer_clearance[signal_layer_id] = xy2coord(h->plane_separation);

	if (hyp_debug)
		pcb_printf("signal layer: \"%s\"", pcb_layer_name(signal_layer_id));
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
		pcb_printf("dielectric layer: ");
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
	plane_layer_id = hyp_create_layer(h->layer_name);

	layer_is_plane[plane_layer_id] = pcb_true;
	if (h->plane_separation_set)
		layer_clearance[plane_layer_id] = xy2coord(h->plane_separation);

	/* XXX need to flood layer with copper */

	if (hyp_debug)
		pcb_printf("plane layer: \"%s\"", pcb_layer_name(plane_layer_id));
	hyp_debug_layer(h);

	return 0;
}

/*
 * DEVICE record.
 */

pcb_bool exec_devices(parse_param * h)
{
	device_t *new_device;
	char value[128];

	if (hyp_debug) {
		pcb_printf("device: device_type = \"%s\" ref = \"%s\"", h->device_type, h->ref);
		if (h->name_set)
			pcb_printf(" name = \"%s\"", h->name);
		if (h->value_float_set)
			pcb_printf(" value_float = %f", h->value_float);
		if (h->value_string_set)
			pcb_printf(" value_string = \"%s\"", h->value_string);
		if (h->layer_name_set)
			pcb_printf(" layer_name = \"%s\"", h->layer_name);
		if (h->package_set)
			pcb_printf(" package = \"%s\"", h->package);
		pcb_printf("\n");
	}

	/* add device to list  */
	new_device = malloc(sizeof(device_t));

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
		pcb_printf("supplies: name = \"%s\" value_float = %f voltage_specified = %i conversion = %i\n", h->name, h->value_float,
							 h->voltage_specified, h->conversion);

	return 0;
}

/* 
 * PADSTACK record
 */

pcb_bool exec_padstack_element(parse_param * h)
{
	/*
	 * Layer names with special meaning, used in padstack definition:
	 * MDEF: defines copper pad on all metal layers
	 * ADEF: defines anti-pad (clearance) on all power planes.
	 */

	if (hyp_debug) {
		pcb_printf("padstack_element:");
		if (h->padstack_name_set)
			pcb_printf(" padstack_name = \"%s\"", h->padstack_name);
		if (h->drill_size_set)
			pcb_printf(" drill_size = %ml", xy2coord(h->drill_size));
		pcb_printf(" layer_name = \"%s\"", h->layer_name);
		pcb_printf(" pad_shape = %f", h->pad_shape);
		if (h->pad_shape == 0)
			pcb_printf(" oval");
		else if (h->pad_shape == 1)
			pcb_printf(" rectangular");
		else if (h->pad_shape == 2)
			pcb_printf(" oblong");
		else
			pcb_printf(" ?");
		pcb_printf(" pad_sx = %ml", xy2coord(h->pad_sx));
		pcb_printf(" pad_sy = %ml", xy2coord(h->pad_sy));
		pcb_printf(" pad_angle = %f", h->pad_angle);
		if (h->pad_type_set & (h->pad_type == PAD_TYPE_THERMAL_RELIEF)) {
			pcb_printf(" thermal_clear_shape = %f", h->thermal_clear_shape);
			if (h->thermal_clear_shape == 0)
				pcb_printf(" oval");
			else if (h->thermal_clear_shape == 1)
				pcb_printf(" rectangular");
			else if (h->thermal_clear_shape == 2)
				pcb_printf(" oblong");
			else
				pcb_printf(" ?");
			pcb_printf(" pad_sx = %ml", xy2coord(h->pad_sx));
			pcb_printf(" pad_sy = %ml", xy2coord(h->pad_sy));
			pcb_printf(" thermal_clear_sx = %ml", xy2coord(h->thermal_clear_sx));
			pcb_printf(" thermal_clear_sy = %ml", xy2coord(h->thermal_clear_sy));
			pcb_printf(" thermal_clear_angle = %f", h->thermal_clear_angle);
		}
		if (h->pad_type_set) {
			pcb_printf(" pad_type = ");
			switch (h->pad_type) {
			case PAD_TYPE_METAL:
				pcb_printf("metal");
				break;
			case PAD_TYPE_ANTIPAD:
				pcb_printf("antipad");
				break;
			case PAD_TYPE_THERMAL_RELIEF:
				pcb_printf("thermal_relief");
				break;
			default:
				pcb_printf("error");
			}
		}
		pcb_printf("\n");
	}


	/* XXX fixme. This reduces the padstack to a pcb_via_new call; a very basic approximation of what a padstack is. */

	if (h->padstack_name_set) {
		current_padstack = malloc(sizeof(padstack_t));
		if (current_padstack == NULL)
			return 1;
		current_padstack->name = pcb_strdup(h->padstack_name);
		current_padstack->thickness = (xy2coord(h->pad_sx) + xy2coord(h->pad_sy)) * 0.5;
		current_padstack->clearance = 0;
		current_padstack->mask = current_padstack->thickness;
		current_padstack->drill_hole = xy2coord(h->drill_size);
		if (h->pad_shape == 1)
			current_padstack->flags = pcb_flag_make(PCB_FLAG_SQUARE);	/* rectangular */
		else if (h->pad_shape == 2)
			current_padstack->flags = pcb_flag_make(PCB_FLAG_OCTAGON);	/* oblong */
		else
			current_padstack->flags = pcb_no_flags();	/* round */
	}

	if ((current_padstack != NULL) && h->pad_type_set & (h->pad_type == PAD_TYPE_THERMAL_RELIEF)) {
		current_padstack->clearance =
			(xy2coord(h->thermal_clear_sx) + xy2coord(h->thermal_clear_sy)) * 0.5 - current_padstack->thickness;
		if (current_padstack->clearance < 0)
			current_padstack->clearance = 0;
	}

	return 0;
}


pcb_bool exec_padstack_end(parse_param * h)
{
	if (hyp_debug)
		pcb_printf("padstack_end\n");

	/* add current padstack to list of padstacks */
	if (current_padstack != NULL) {
		current_padstack->next = padstack_head;
		padstack_head = current_padstack;
		current_padstack = NULL;
	}

	return 0;
}

/*
 * NET record.
 */

pcb_bool exec_net(parse_param * h)
{
	if (hyp_debug)
		pcb_printf("net: net_name = \"%s\"\n", h->net_name);

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
		pcb_printf("net_plane_separation: plane_separation = %ml\n", xy2coord(h->plane_separation));

	net_clearance = xy2coord(h->plane_separation);

	return 0;
}

/*
 * A subrecord of NET record. Attribute.
 */

pcb_bool exec_net_attribute(parse_param * h)
{
	if (hyp_debug)
		pcb_printf("net_attribute: name = \"%s\" value = \"%s\"\n", h->name, h->value);

	return 0;
}

/*
 * SEG subrecord of NET record. line segment.
 */

pcb_bool exec_seg(parse_param * h)
{
	if (hyp_debug) {
		pcb_printf("seg: x1 = %ml y1 = %ml x2 = %ml y2 = %ml ", x2coord(h->x1), y2coord(h->y1), x2coord(h->x2), y2coord(h->y2));
		pcb_printf(" width = %ml layer_name = \"%s\"", xy2coord(h->width), h->layer_name);
		if (h->plane_separation_set)
			pcb_printf(" plane_separation = %ml ", xy2coord(h->plane_separation));
		if (h->left_plane_separation_set)
			pcb_printf(" left_plane_separation = %ml ", xy2coord(h->left_plane_separation));
		pcb_printf("\n");
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
		pcb_printf("arc: x1 = %ml y1 = %ml x2 = %ml y2 = %ml", x2coord(h->x1), y2coord(h->y1), x2coord(h->x2), y2coord(h->y2));
		pcb_printf(" xc = %ml yc = %ml r = %ml", x2coord(h->xc), y2coord(h->yc), xy2coord(h->r));
		pcb_printf(" width = %ml layer_name = \"%s\"", xy2coord(h->width), h->layer_name);
		if (h->plane_separation_set)
			pcb_printf(" plane_separation = %ml", xy2coord(h->plane_separation));
		if (h->left_plane_separation_set)
			pcb_printf(" left_plane_separation = %ml", xy2coord(h->left_plane_separation));
		pcb_printf("\n");
	}

	hyp_arc_new(hyp_get_layer(h), x2coord(h->x1), y2coord(h->y1), x2coord(h->x2), y2coord(h->y2), x2coord(h->xc),
							y2coord(h->yc), xy2coord(h->r), xy2coord(h->r), pcb_true, xy2coord(h->width), hyp_clearance(h), pcb_no_flags());

	return 0;
}

/*
 * VIA subrecord of NET record. 
 * Draws via using padstack definition.
 */

pcb_bool exec_via(parse_param * h)
{
	padstack_t *padstack;

	/* detect old-style v1.0 via */
	if (!h->padstack_name_set)
		return exec_via_v1(h);

	if (hyp_debug) {
		pcb_printf("via: x = %ml y = %ml", x2coord(h->x), y2coord(h->y));
		if (h->padstack_name_set)
			pcb_printf(" padstack_name = \"%s\"", h->padstack_name);
		pcb_printf("\n");
	}

	padstack = hyp_padstack_by_name(h->padstack_name);
	if (padstack == NULL) {
		pcb_printf("via: padstack \"%s\" not found. skipping.\n", h->padstack_name);
		return 0;
	}

	/* XXX buried and blind vias not implemented yet */

	pcb_via_new(hyp_dest, x2coord(h->x), y2coord(h->y), padstack->thickness, padstack->clearance, padstack->mask,
							padstack->drill_hole, NULL, padstack->flags);

	return 0;
}

/*
 * VIA subrecord of NET record. 
 * Draws deprecated v1.x via. Does not use padstack.
 */

pcb_bool exec_via_v1(parse_param * h)
{
	pcb_coord_t thickness;
	pcb_coord_t clearance;
	pcb_coord_t mask;
	pcb_coord_t drill_hole = 0;
	char *shape = NULL;
	pcb_flag_t flags;

	if (hyp_debug) {
		pcb_printf("old_via: x = %ml y = %ml", x2coord(h->x), y2coord(h->y));
		if (h->drill_size_set)
			pcb_printf(" drill_size = %ml", xy2coord(h->drill_size));
		if (h->layer1_name_set)
			pcb_printf(" layer1_name = \"%s\"", h->layer1_name);
		if (h->layer2_name_set)
			pcb_printf(" layer2_name = \"%s\"", h->layer2_name);
		if (h->via_pad_shape_set)
			pcb_printf(" via_pad_shape = \"%s\"", h->via_pad_shape);
		if (h->via_pad_sx_set)
			pcb_printf(" via_pad_sx = \"%ml\"", xy2coord(h->via_pad_sx));
		if (h->via_pad_sy_set)
			pcb_printf(" via_pad_sy = \"%ml\"", xy2coord(h->via_pad_sy));
		if (h->via_pad_angle_set)
			pcb_printf(" via_pad_angle = \"%f\"", h->via_pad_angle);
		if (h->via_pad1_shape_set)
			pcb_printf(" via_pad1_shape = \"%s\"", h->via_pad1_shape);
		if (h->via_pad1_sx_set)
			pcb_printf(" via_pad1_sx = \"%ml\"", xy2coord(h->via_pad1_sx));
		if (h->via_pad1_sy_set)
			pcb_printf(" via_pad1_sy = \"%ml\"", xy2coord(h->via_pad1_sy));
		if (h->via_pad1_angle_set)
			pcb_printf(" via_pad1_angle = \"%f\"", h->via_pad1_angle);
		if (h->via_pad2_shape_set)
			pcb_printf(" via_pad2_shape = \"%s\"", h->via_pad2_shape);
		if (h->via_pad2_sx_set)
			pcb_printf(" via_pad2_sx = \"%ml\"", xy2coord(h->via_pad2_sx));
		if (h->via_pad2_sy_set)
			pcb_printf(" via_pad1_sy = \"%ml\"", xy2coord(h->via_pad2_sy));
		if (h->via_pad2_angle_set)
			pcb_printf(" via_pad2_angle = \"%f\"", h->via_pad2_angle);
		pcb_printf("\n");
	}

	if (h->via_pad_sx_set && h->via_pad_sy_set)
		mask = thickness = (xy2coord(h->via_pad_sx) + xy2coord(h->via_pad_sy)) * 0.5;
	else if (h->via_pad1_sx_set && h->via_pad1_sy_set)
		mask = thickness = (xy2coord(h->via_pad1_sx) + xy2coord(h->via_pad1_sy)) * 0.5;
	else if (h->via_pad2_sx_set && h->via_pad2_sy_set)
		mask = thickness = (xy2coord(h->via_pad2_sx) + xy2coord(h->via_pad2_sy)) * 0.5;
	else
		mask = thickness = 0;

	clearance = 2 * hyp_clearance(h);

	if (h->drill_size_set)
		drill_hole = xy2coord(h->drill_size);
	else
		drill_hole = 0;

	if (h->via_pad_shape_set)
		shape = h->via_pad_shape;
	else if (h->via_pad1_shape_set)
		shape = h->via_pad1_shape;
	else if (h->via_pad2_shape_set)
		shape = h->via_pad2_shape;
	else
		shape = NULL;

	if ((shape != NULL) && (strcmp(shape, "RECT") == 0))
		flags = pcb_flag_make(PCB_FLAG_SQUARE);	/* rectangular */
	else if ((shape != NULL) && (strcmp(shape, "OBLONG") == 0))
		flags = pcb_flag_make(PCB_FLAG_OCTAGON);	/* oblong */
	else
		flags = pcb_no_flags();			/* round */

	/* XXX buried and blind vias not implemented yet */

	pcb_via_new(hyp_dest, x2coord(h->x), y2coord(h->y), thickness, clearance, mask, drill_hole, NULL, flags);

	return 0;
}

/*
 * PIN subrecord of NET record. 
 * Draws PIN using padstack definiton.
 */

pcb_bool exec_pin(parse_param * h)
{
	pcb_element_t *device;
	padstack_t *padstack;
	char *device_name = NULL;
	char *pin_name = NULL;
	char *dot = NULL;
	char *pin_net_name = NULL;

	if (hyp_debug) {
		pcb_printf("pin: x = %ml y = %ml", x2coord(h->x), y2coord(h->y));
		pcb_printf(" pin_reference = \"%s\"", h->pin_reference);
		if (h->padstack_name_set)
			pcb_printf(" padstack_name = \"%s\"", h->padstack_name);
		if (h->pin_function_set)
			pcb_printf(" pin_function = %i", h->pin_function);
		pcb_printf("\n");
	}

	if (!h->padstack_name_set) {
		if (hyp_debug)
			pcb_printf("pin: padstack not set. skipping pin \"%s\"\n", h->pin_reference);
		return 0;
	}

	if (net_name != NULL)
		pin_net_name = pcb_strdup(net_name);
	else
		pin_net_name = pcb_strdup("?");

	/* h->pin_reference has format 'device_name.pin_name' */
	device_name = pcb_strdup(h->pin_reference);
	pin_name = pcb_strdup("?");
	dot = strrchr(device_name, '.');
	if (dot != NULL) {
		*dot = '\0';
		pin_name = pcb_strdup(dot + 1);
		if (hyp_debug)
			pcb_printf("pin: device_name = \"%s\" pin_name = \"%s\"\n", device_name, pin_name);
	}

	/* find device by name */
	device = hyp_create_element_by_name(device_name, x2coord(h->x), y2coord(h->y));

	if (device == NULL) {
		pcb_printf("pin: device \"%s\" not found. skipping pin \"%s\"\n", device_name, h->pin_reference);
		return 0;
	}

	/* find padstack */
	padstack = hyp_padstack_by_name(h->padstack_name);
	if (padstack == NULL) {
		pcb_printf("pin: padstack \"%s\" not found. skipping pin \"%s\"\n", h->padstack_name, h->pin_reference);
		return 0;
	}

	/* what if padstack = single layer & no drill ?  SMD XXX */

	/* add new pin */
	pcb_element_pin_new(device, x2coord(h->x), y2coord(h->y), padstack->thickness, padstack->clearance, padstack->mask,
											padstack->drill_hole, pin_net_name, pin_name, padstack->flags);

	return 0;
}

/*
 * PAD subrecord of NET record. 
 * Draws deprecated v1.x pad.
 */

pcb_bool exec_pad(parse_param * h)
{
	pcb_coord_t thickness;
	pcb_coord_t clearance;
	pcb_coord_t mask;
	pcb_element_t *device;
	pcb_flag_t flags;
	char hyperlynx_pad[] = "hyperlynx_pad";

	if (hyp_debug) {
		pcb_printf("pad: x = %ml y = %ml", x2coord(h->x), y2coord(h->y));
		if (h->layer_name_set)
			pcb_printf(" layer_name = \"%s\"", h->layer_name);
		if (h->via_pad_shape_set)
			pcb_printf(" via_pad_shape = \"%s\"", h->via_pad_shape);
		if (h->via_pad_sx_set)
			pcb_printf(" via_pad_sx = \"%ml\"", xy2coord(h->via_pad_sx));
		if (h->via_pad_sy_set)
			pcb_printf(" via_pad_sy = \"%ml\"", xy2coord(h->via_pad_sy));
		if (h->via_pad_angle_set)
			pcb_printf(" via_pad_angle = \"%f\"", h->via_pad_angle);
		pcb_printf("\n");
	}

	/* if necessary, create an element to connect the pad to */
	device = hyp_create_element_by_name(hyperlynx_pad, x2coord(h->x), y2coord(h->y));
	if (device == NULL) {
		pcb_printf("pad: can't create device. skipping pad.\n");
		return 0;
	}

	/* add new pad */
	flags = pcb_no_flags();

	/* pad shape */
	if (h->via_pad_shape_set && (h->via_pad_shape != NULL) && (strcmp(h->via_pad_shape, "RECT") == 0))
		pcb_flag_add(flags, PCB_FLAG_SQUARE);
	else if (h->via_pad_shape_set && (h->via_pad_shape != NULL) && (strcmp(h->via_pad_shape, "OBLONG") == 0))
		pcb_flag_add(flags, PCB_FLAG_OCTAGON);

	if (h->layer_name_set && (h->layer_name != NULL) && (pcb_layer_flags(pcb_layer_by_name(h->layer_name)) & PCB_LYT_BOTTOM))
		pcb_flag_add(flags, PCB_FLAG_ONSOLDER);

	if (h->via_pad_sx_set && h->via_pad_sy_set)
		mask = thickness = (xy2coord(h->via_pad_sx) + xy2coord(h->via_pad_sy)) * 0.5;
	else
		mask = thickness = 0;

	clearance = 2 * hyp_clearance(h); /* XXX hyp_layer_clearance */

	pcb_element_pin_new(device, x2coord(h->x), y2coord(h->y), thickness, clearance, mask, 0, net_name, "?", flags);

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
		pcb_printf("useg: x1 = %ml y1 = %ml layer1_name = \"%s\"", x2coord(h->x1), y2coord(h->y1), h->layer1_name);
		pcb_printf(" x2 = %ml y2 = %ml layer2_name = \"%s\"", x2coord(h->x2), y2coord(h->y2), h->layer2_name);
		if (h->zlayer_name_set)
			pcb_printf(" zlayer_name = \"%s\" width = %ml length = %ml", h->zlayer_name, xy2coord(h->width), xy2coord(h->length));
		if (h->impedance_set)
			pcb_printf(" impedance = %f delay = %f ", h->impedance, h->delay);
		if (h->resistance_set)
			pcb_printf(" resistance = %f ", h->resistance);
		pcb_printf("\n");
	}

  /* XXX fixme. want to put an unrouted segment between two layers, not two layer groups. */
	/* lookup layer group begin and end layer are on */
	layer1_grp_id = pcb_layer_get_group(hyp_create_layer(h->layer1_name));
	layer2_grp_id = pcb_layer_get_group(hyp_create_layer(h->layer2_name));

	if ((layer1_grp_id == -1) || (layer2_grp_id == -1)) {
		if (hyp_debug)
			pcb_printf("skipping unrouted segment\n");
		return 0;
	}

	pcb_rat_new(hyp_dest, x2coord(h->x1), y2coord(h->y1), x2coord(h->x2), y2coord(h->y2), layer1_grp_id, layer2_grp_id,
							xy2coord(h->width), pcb_no_flags());

	return 0;
}

/*
 * POLYGON subrecord of NET record.
 * draws copper polygon.
 */

/* XXX still need to implement different types (POUR, COPPER, PLANE) */

pcb_bool exec_polygon_begin(parse_param * h)
{
	pcb_layer_t *current_layer;
	hyp_polygon_t *new_poly;

	if (hyp_debug) {
		pcb_printf("polygon begin:");
		if (h->layer_name_set)
			pcb_printf(" layer_name = \"%s\"", h->layer_name);
		if (h->width_set)
			pcb_printf(" width = %ml", xy2coord(h->width));
		if (h->polygon_type_set) {
			pcb_printf(" polygon_type = ", h->polygon_type, " ");
			switch (h->polygon_type) {
			case POLYGON_TYPE_PLANE:
				pcb_printf("POLYGON_TYPE_PLANE");
				break;
			case POLYGON_TYPE_POUR:
				pcb_printf("POLYGON_TYPE_POUR");
				break;
			case POLYGON_TYPE_COPPER:
				pcb_printf("POLYGON_TYPE_COPPER");
				break;
			default:
				pcb_printf("Error");
				break;
			}
		}
		if (h->id_set)
			pcb_printf(" id = %i", h->id);
		pcb_printf(" x = %ml y = %ml\n", xy2coord(h->x), xy2coord(h->y));
	}

	if (!h->layer_name_set) {
		hyp_error("expected polygon layer L = ");
		return pcb_true;
	}

	if (!h->id_set) {
		hyp_error("expected polygon id ID = ");
		return pcb_true;
	}

	/* create empty pcb polygon */
	current_layer = hyp_get_layer(h);
	current_polygon = pcb_poly_new(current_layer, pcb_flag_make(PCB_FLAG_CLEARPOLY));

	/* add first vertex */
	if (current_polygon != NULL)
		pcb_poly_point_new(current_polygon, x2coord(h->x), y2coord(h->y));

	/* bookkeeping */
	if ((poly_state != HYP_POLYIDLE) && hyp_debug)
		pcb_printf("polygon: unexpected polygon. continuing.\n");
	poly_state = HYP_POLYGON;

	new_poly = malloc(sizeof(hyp_polygon_t));
	new_poly->hyp_poly_id = h->id;	/* hyperlynx polygon/polyline id */
	new_poly->polygon = current_polygon;
	new_poly->layer = current_layer;
	new_poly->is_polygon = pcb_true;

	new_poly->next = polygon_head;
	polygon_head = new_poly;

	return 0;
}

pcb_bool exec_polygon_end(parse_param * h)
{
	if (hyp_debug)
		pcb_printf("polygon end:\n");

	/* bookkeeping */
	if ((poly_state != HYP_POLYGON) && hyp_debug)
		pcb_printf("polygon: unexpected polygon end. continuing.\n");
	poly_state = HYP_POLYIDLE;
	current_polygon = NULL;

	return 0;
}

/*
 * POLYVOID subrecord of NET record.
 * same as POLYGON, but instead of creating copper, creates a hole in copper.
 */

pcb_bool exec_polyvoid_begin(parse_param * h)
{
	hyp_polygon_t *i;

	if (hyp_debug) {
		pcb_printf("polyvoid begin:");
		if (h->id_set)
			pcb_printf(" id = %i", h->id);
		pcb_printf(" x = %ml y = %ml\n", x2coord(h->x), y2coord(h->y));
	}

	if (!h->id_set) {
		hyp_error("expected polygon id ID = ");
		return pcb_true;
	}

	/* look up polygon with this id */
	current_polygon = NULL;
	for (i = polygon_head; i != NULL; i = i->next)
		if (h->id == i->hyp_poly_id) {
			if (i->is_polygon)
				current_polygon = i->polygon;
			else
				pcb_printf("polyvoid: polyvoid hole in polyline not implemented.\n");
		}

	if (i == NULL)
		pcb_printf("polyvoid: polygon id %i not found\n", h->id);

	if (current_polygon != NULL) {
		/* create hole in polygon */
		pcb_poly_hole_new(current_polygon);
		/* add first vertex of hole */
		pcb_poly_point_new(current_polygon, x2coord(h->x), y2coord(h->y));
	}

	/* bookkeeping */
	if ((poly_state != HYP_POLYIDLE) && hyp_debug)
		pcb_printf("polyvoid: unexpected polyvoid. continuing.\n");
	if (i->is_polygon)
		poly_state = HYP_POLYGON_HOLE;
	else
		poly_state = HYP_POLYLINE_HOLE;

	return 0;
}

pcb_bool exec_polyvoid_end(parse_param * h)
{
	if (hyp_debug)
		pcb_printf("polyvoid end:\n");

	/* bookkeeping */
	if ((poly_state != HYP_POLYGON_HOLE) && (poly_state != HYP_POLYLINE_HOLE) && hyp_debug)
		pcb_printf("polyvoid: unexpected polyvoid end. continuing.\n");
	poly_state = HYP_POLYIDLE;

	return 0;
}

/*
 * POLYLINE subrecord of NET record.
 */

pcb_bool exec_polyline_begin(parse_param * h)
{
	hyp_polygon_t *new_poly;

	if (hyp_debug) {
		pcb_printf("polyline begin:");
		if (h->layer_name_set)
			pcb_printf(" layer_name = \"%s\"", h->layer_name);
		if (h->width_set)
			pcb_printf(" width = %ml", xy2coord(h->width));
		if (h->polygon_type_set) {
			pcb_printf(" polygon_type = ", h->polygon_type, " ");
			switch (h->polygon_type) {
			case POLYGON_TYPE_PLANE:
				pcb_printf("POLYGON_TYPE_PLANE");
				break;
			case POLYGON_TYPE_POUR:
				pcb_printf("POLYGON_TYPE_POUR");
				break;
			case POLYGON_TYPE_COPPER:
				pcb_printf("POLYGON_TYPE_COPPER");
				break;
			default:
				pcb_printf("Error");
				break;
			}
		}
		if (h->id_set)
			pcb_printf(" id = %i", h->id);
		pcb_printf(" x = %ml y = %ml\n", x2coord(h->x), y2coord(h->y));
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

	/* create polyline */
	current_polygon = NULL;
	current_polyline_layer = hyp_get_layer(h);
	current_polyline_width = xy2coord(h->width);
	current_polyline_clearance = hyp_clearance(h);
	current_polyline_xpos = x2coord(h->x);
	current_polyline_ypos = y2coord(h->y);

	/* bookkeeping */
	if ((poly_state != HYP_POLYIDLE) && hyp_debug)
		pcb_printf("polyline: unexpected polyline. continuing.\n");
	poly_state = HYP_POLYLINE;

	/* we need to store the id of the polyline, in case a matching polyvoid occurs. */
	new_poly = malloc(sizeof(hyp_polygon_t));
	new_poly->hyp_poly_id = h->id;	/* hyperlynx polygon/polyline id */
	new_poly->polygon = NULL;
	new_poly->layer = current_polyline_layer;
	new_poly->is_polygon = pcb_false;

	new_poly->next = polygon_head;
	polygon_head = new_poly;

	return 0;
}

pcb_bool exec_polyline_end(parse_param * h)
{
	if (hyp_debug)
		pcb_printf("polyline end:\n");

	/* bookkeeping */
	if ((poly_state != HYP_POLYLINE) && hyp_debug)
		pcb_printf("polyline: unexpected polyline end. continuing.\n");

	poly_state = HYP_POLYIDLE;
	current_polygon = NULL;
	current_polyline_layer = NULL;
	current_polyline_width = 0;
	current_polyline_clearance = 0;
	current_polyline_xpos = 0;
	current_polyline_ypos = 0;

	return 0;
}

/*
 * LINE subrecord of NET record.
 */

pcb_bool exec_line(parse_param * h)
{
	if (hyp_debug)
		pcb_printf("line: x = %ml y = %ml\n", x2coord(h->x), y2coord(h->y));

	switch (poly_state) {
	case HYP_POLYIDLE:
		if (hyp_debug)
			pcb_printf("line: unexpected. continuing.\n");
		break;
	case HYP_POLYGON:
	case HYP_POLYGON_HOLE:
		/* add new vertex to polygon */
		if (current_polygon != NULL)
			pcb_poly_point_new(current_polygon, x2coord(h->x), y2coord(h->y));
		break;
	case HYP_POLYLINE:
		/* add new point to polyline */
		if (current_polyline_layer != NULL) {
			pcb_line_new(current_polyline_layer, current_polyline_xpos, current_polyline_ypos, x2coord(h->x), y2coord(h->y),
									 current_polyline_width, current_polyline_clearance, pcb_no_flags());
			/* move on */
			current_polyline_xpos = x2coord(h->x);
			current_polyline_ypos = y2coord(h->y);
		}
		break;
	case HYP_POLYLINE_HOLE:
		/* hole in polyline not implemented */
		break;
	default:
		if (hyp_debug)
			pcb_printf("line: error\n");
	}

	return 0;
}

/*
 * CURVE subrecord of NET record. 
 * Clockwise from (x1, y1) to (x2, y2).
 */

pcb_bool exec_curve(parse_param * h)
{
	if (hyp_debug)
		pcb_printf("curve: x1 = %ml y1 = %ml x2 = %ml y2 = %ml xc = %ml yc = %ml r = %ml\n", x2coord(h->x1), y2coord(h->y1),
							 x2coord(h->x2), y2coord(h->y2), x2coord(h->xc), y2coord(h->yc), xy2coord(h->r));

	switch (poly_state) {
	case HYP_POLYIDLE:
		if (hyp_debug)
			pcb_printf("curve: unexpected. continuing.\n");
		break;
	case HYP_POLYGON:
	case HYP_POLYGON_HOLE:
		if (current_polygon != NULL) {
			/* polygon contains line segments, not arc segments. convert arc segment to line segments. */
			pcb_poly_point_new(current_polygon, x2coord(h->x2), y2coord(h->y2));	/* 2do XXX */
		}
		break;
	case HYP_POLYLINE:
		if (current_polyline_layer != NULL) {
			/* clockwise arc from (x1, y2) to (x2, y2) */
			hyp_arc_new(current_polyline_layer, x2coord(h->x1), y2coord(h->y1), x2coord(h->x2), y2coord(h->y2), x2coord(h->xc),
									y2coord(h->yc), 2 * xy2coord(h->r), 2 * xy2coord(h->r), pcb_true, current_polyline_width,
									current_polyline_clearance, pcb_no_flags());

			/* move on */
			if ((current_polyline_xpos == x2coord(h->x1)) && (current_polyline_ypos == y2coord(h->y1))) {
				current_polyline_xpos = x2coord(h->x2);
				current_polyline_ypos = y2coord(h->y2);
			}
			else if ((current_polyline_xpos == x2coord(h->x2)) && (current_polyline_ypos == y2coord(h->y2))) {
				current_polyline_xpos = x2coord(h->x1);
				current_polyline_ypos = y2coord(h->y1);
			}
		}
		break;
	case HYP_POLYLINE_HOLE:
		/* not implemented */
		break;
	default:
		if (hyp_debug)
			pcb_printf("curve: error\n");
	}

	return 0;
}

/*
 * NET_CLASS record
 */

pcb_bool exec_net_class(parse_param * h)
{
	if (hyp_debug)
		pcb_printf("net_class: net_class_name = \"%s\"\n", h->net_class_name);

	return 0;
}

/*
 * N net membership subrecord of NET_CLASS record
 */

pcb_bool exec_net_class_element(parse_param * h)
{
	if (hyp_debug)
		pcb_printf("net_class_element: net_name = \"%s\"\n", h->net_name);

	return 0;
}

/*
 * A attribute subrecord of NET_CLASS record
 */


pcb_bool exec_net_class_attribute(parse_param * h)
{
	if (hyp_debug)
		pcb_printf("netclass_attribute: name = \"%s\" value = \"%s\"\n", h->name, h->value);

	return 0;
}

/* 
 * KEY record
 */

pcb_bool exec_key(parse_param * h)
{
	if (hyp_debug)
		pcb_printf("key: key = \"%s\"\n", h->key);

	return 0;
}

/* 
 * END record
 */

pcb_bool exec_end(parse_param * h)
{
	if (hyp_debug)
		pcb_printf("end\n");

	return 0;
}

/* not truncated */

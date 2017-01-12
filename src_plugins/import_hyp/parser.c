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

#include "parser.h"
#include "hyp_l.h"
#include "hyp_y.h"
#include "error.h"
#include "pcb-printf.h"
#include "obj_all.h"
#include "flag_str.h"
#include "board.h"
#include "layer.h"
#include "data.h"

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
  pcb_bool_t is_arc; /* arc or line */
  pcb_bool_t used; /* already included in outline */
  struct outline_s *next;
  } outline_t;

outline_t *outline_head;
outline_t *outline_tail;

void hyp_set_origin(); /* set origin so all coordinates are positive */
void hyp_perimeter(); /* add board outline to pcb */

int hyp_debug; /* logging on/off switch */

/* Physical constants */
double inches;                 /* inches to m */
double copper_imperial_weight; /* metal thickness in ounces/ft2 */
double copper_metric_weight;   /* metal thickness in grams/cm2 */
double copper_bulk_resistivity; /* metal resistivity in ohm meter */
double copper_temperature_coefficient; /* temperature coefficient of bulk resistivity */
double fr4_epsilon_r;          /* dielectric constant of substrate */
double fr4_loss_tangent;       /* loss tangent of substrate */
double conformal_epsilon_r;    /* dielectric constant of conformal coating */
  
/* Hyperlynx UNIT and OPTIONS */
double unit;                   /* conversion factor: pcb length units to meters */
double metal_thickness_unit;   /* conversion factor: metal thickness to meters */

pcb_bool use_die_for_metal;    /* use dielectric constant and loss tangent of dielectric for metal layers */
pcb_coord_t plane_separation;  /* distance between PLANE polygon and copper of different nets; -1 if not set */

/* stackup */
pcb_bool layer_is_plane_layer[PCB_MAX_LAYER]; /* whether layer is signal or plane layer */
pcb_coord_t layer_plane_separation[PCB_MAX_LAYER]; /* separation between fill copper and signals on layer */

/* origin. Chosen so all coordinates are positive. */
pcb_coord_t origin_x;
pcb_coord_t origin_y;

/*
 * Conversion from hyperlynx to pcb_coord_t - igor2
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

  inches  = 0.0254; /* inches to m */
  copper_imperial_weight = 1.341; /* metal thickness in ounces/ft2. 1 oz/ft2 copper = 1.341 mil */
  copper_metric_weight = 0.1116;  /* metal thickness in grams/cm2. 1 gr/cm2 copper = 0.1116 cm */
  copper_bulk_resistivity = 1.724e-8;
  copper_temperature_coefficient = 0.00393;
  fr4_epsilon_r = 4.3;
  fr4_loss_tangent = 0.020;
  conformal_epsilon_r = 3.3; /* dielectric constant of conformal layer */
  plane_separation = -1; /* distance between PLANE polygon and copper of different nets; -1 if not set */

  /* empty board outline */
  outline_head = NULL;
  outline_tail = NULL;

  /* clear layer info */
  for (n = 1; n < PCB_MAX_LAYER; n++) {
    layer_is_plane_layer[n] = pcb_false; /* signal layer */
    layer_plane_separation[n] = -1; /* no separation between fill copper and signals on layer */
    }

  /* set origin */
  origin_x = 0;
  origin_y = 0;

  return;
}

/* 
 * called by pcb-rnd to load hyperlynx file 
 */

int hyp_parse(pcb_data_t *dest, const char *fname, int debug)
{
  int retval;

  hyp_init();

  /* set debug levels */
  hyyset_debug(debug > 2); /* switch flex logging on */
  hyydebug = (debug > 1);  /* switch bison logging on */
  hyp_debug = (debug > 0); /* switch hyperlynx logging on */

  /* set shared board */
  hyp_dest = dest;

  /* parse hyperlynx file */
  hyyin =  fopen(fname, "r");
  if (hyyin == NULL) return (1);
  retval = hyyparse();
  fclose(hyyin);

  /* add board outline last */
  hyp_perimeter();

  pcb_layers_finalize(); /* XXX Check. */

  /* clear */
  hyp_dest = NULL;

  return(retval);
}
 
/* print error message */
void hyp_error(const char *msg)
{
  pcb_message(PCB_MSG_DEBUG, "line %d: %s at '%s'\n", hyylineno, msg, hyytext);
}

/* exec_* routines are called by parser to interpret hyperlynx file */ 

/*
 * Hyperlynx 'BOARD_FILE' section.
 * Hyperlynx file header.
 */

pcb_bool exec_board_file(parse_param *h)
{
  if (hyp_debug) pcb_printf(PCB_MSG_DEBUG, "board\n");

  return 0;
}

/*
 * Hyperlynx 'VERSION' record.
 * Specifies version number.
 * Required record; must be first record of the file.
 */

pcb_bool exec_version(parse_param *h)
{
  if (hyp_debug) pcb_printf("version: vers = %f\n", h->vers);

  if (h->vers < 1.0) pcb_message(PCB_MSG_DEBUG, "warning: version 1.x deprecated\n");

  return 0;
}

/*
 * Hyperlynx 'DATA_MODE' record.
 * If DATA_MODE is DETAILED, model can be used for power and signal simulation.
 * If DATA_MODE is SIMPLIFIED, model can be used for signal simulation only.
 */

pcb_bool exec_data_mode(parse_param *h)
{
  if (hyp_debug) pcb_printf("data_mode: detailed = %i\n", h->detailed);

  return 0;
}

/*
 * Hyperlynx 'UNITS' record.
 * Specifies measurement system (english/metric) for the rest of the file.
 */

pcb_bool exec_units(parse_param *h)
{
  if (hyp_debug) pcb_printf("units: unit_system_english = %d metal_thickness_weight = %d\n", h->unit_system_english, h->metal_thickness_weight);

  /* convert everything to meter */

  if (h->unit_system_english) {
    unit = inches;                                /* lengths in inches. 1 in = 2.54 cm = 0.0254 m */
    if (h->metal_thickness_weight)
      metal_thickness_unit = copper_imperial_weight * unit;        /* metal thickness in ounces/ft2. 1 oz/ft2 copper = 1.341 mil */
    else
      metal_thickness_unit = unit;                /* metal thickness in inches */
    }
  else {
    unit = 0.01;                                  /* lengths in centimeters. 1 cm = 0.01 m */
    if (h->metal_thickness_weight)
      metal_thickness_unit = copper_metric_weight * unit;       /* metal thickness in grams/cm2. 1 gr/cm2 copper = 0.1116 cm */
    else
      metal_thickness_unit = unit;                /* metal thickness in centimeters */
    }

  if (hyp_debug) pcb_printf("units: unit = %f metal_thickness_unit = %f\n", unit, metal_thickness_unit);

  return 0;
}

/*
 * Hyperlynx 'PLANE_SEP' record.
 * Defines default trace to plane separation
 */

pcb_bool exec_plane_sep(parse_param *h)
{
  plane_separation = m2coord(h->plane_separation);

  if (hyp_debug) pcb_printf("plane_sep: default_plane_separation = %mm\n", plane_separation);

  return 0;
}

/*
 * Hyperlynx 'PERIMETER_SEGMENT' subrecord of 'BOARD' record.
 * Draws linear board outline segment.
 * Linear segment drawn from (x1, y1) to (x2, y2).
 */

pcb_bool exec_perimeter_segment(parse_param *h)
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

  if (hyp_debug) pcb_printf("perimeter_segment: x1 = %mm y1 = %mm x2 = %mm y2 = %mm\n", peri_seg->x1, peri_seg->y1, peri_seg->x2, peri_seg->y2);

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
 * Hyperlynx 'PERIMETER_ARC' subrecord of 'BOARD' record.
 * Draws arc segment of board outline. 
 * Arc drawn counterclockwise from (x1, y1) to (x2, y2) with center (xc, yc) and radius r.
 */

pcb_bool exec_perimeter_arc(parse_param *h)
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

  if (hyp_debug) pcb_printf("perimeter_arc: x1 = %mm y1 = %mm x2 = %mm y2 = %mm xc = %mm yc = %mm r = %mm\n", peri_arc->x1, peri_arc->y1, peri_arc->x2, peri_arc->y2, peri_arc->xc, peri_arc->yc, peri_arc->r);

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
 * Hyperlynx 'A' attribute subrecord of 'BOARD' record.
 * Defines board attributes as name/value pairs.
 */

pcb_bool exec_board_attribute(parse_param *h)
{
  if (hyp_debug) pcb_printf("board_attribute: name = %s value = %s\n", h->name, h->value);

  return 0;
}

/*
 * add segment to board outline
 */

void perimeter_segment_add(outline_t *s, pcb_bool_t forward)
{
  pcb_layer_id_t outline_id;
  pcb_layer_t *outline_layer;

  /* get outline layer */
  outline_id = pcb_layer_create(PCB_LYT_OUTLINE, pcb_false, pcb_false, NULL);
  if (outline_id < 0) outline_id = pcb_layer_create(PCB_LYT_OUTLINE, pcb_true, pcb_false, NULL);
  outline_layer = pcb_get_layer(outline_id);

  if (outline_layer == NULL) 
  { 
    pcb_printf("create outline layer failed.\n"); 
    return; 
  }

  /* mark segment as used, so we don't use it twice */
  s->used = pcb_true;

  /* debugging */
  if (hyp_debug) {
    if (forward) pcb_printf("outline: fwd %s from (%mm, %mm) to (%mm, %mm)\n", s->is_arc ? "arc" : "line", s->x1, s->y1, s->x2, s->y2);
    else pcb_printf("outline: bwd %s from (%mm, %mm) to (%mm, %mm)\n", s->is_arc ? "arc" : "line", s->x2, s->y2, s->x1, s->y1); /* add segment back to front */
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

pcb_bool_t segment_connected(pcb_coord_t begin_x, pcb_coord_t begin_y, pcb_coord_t end_x, pcb_coord_t end_y, outline_t *s)
{
  outline_t *i;
  pcb_bool_t connected;

  connected = (begin_x == end_x) && (begin_y == end_y); /* trivial case */

  if (!connected)
  {
    /* recursive descent */
    s->used = pcb_true;

    for (i = outline_head; i != NULL; i = i->next)
    {
      if (i->used) continue;
      if ((i->x1 == begin_x) && (i->y1 == begin_y))
      {
        connected = ((i->x2 == end_x) && (i->y2 == end_y)) || segment_connected(i->x2, i->y2, end_x, end_y, i);
        if (connected) break;
      }
      /* try back-to-front */
      if ((i->x2 == begin_x) && (i->y2 == begin_y))
      {
        connected = ((i->x1 == end_x) && (i->y1 == end_y)) || segment_connected(i->x1, i->y1, end_x, end_y, i);
        if (connected) break;
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
  if (outline_head != NULL){
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
    if (i->x1 < origin_x) origin_x = i->x1;
    if (i->x2 < origin_x) origin_x = i->x2;
    if (i->y1 > origin_y) origin_y = i->y1;
    if (i->y2 > origin_y) origin_y = i->y2;
    if (i->is_arc) {
      if (i->xc - i->r < origin_x) origin_x = i->xc - i->r;
      if (i->yc + i->r > origin_y) origin_y = i->yc + i->r;
      }
    }
}

/* 
 * Draw board perimeter.
 * The first segment is part of the first polygon.
 * The first polygon of the board perimeter is positive, the rest are holes.
 * Segments do not necesarily occur in order.
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
  while (pcb_true)
  {

    /* find first free segment */
    for (i = outline_head; i != NULL; i = i->next)
      if (i->used == pcb_false) break;

    /* exit if no segments found */
    if (i == NULL) break;

    /* first point of polygon (begin_x, begin_y) */
    begin_x = i->x1;
    begin_y = i->y1;

    /* last point of polygon (last_x, last_y) */
    last_x = i->x2;
    last_y = i->y2;

    /* add segment */
    perimeter_segment_add(i, pcb_true);

    /* add polygon segments until the polygon is closed */
    polygon_closed = pcb_false;
    while (!polygon_closed)
    {

#undef XXX
#ifdef XXX
      pcb_printf("perimeter: last_x = %mm last_y = %mm\n", last_x, last_y);
      for (i = outline_head; i != NULL; i = i->next)
        if (!i->used) pcb_printf("perimeter segments available: %s from (%mm, %mm) to (%mm, %mm)\n", i->is_arc ? "arc" : "line", i->x1, i->y1, i->x2, i->y2);
#endif

      /* find segment to add to current polygon */
      segment_found = pcb_false;

      /* XXX prefer closed polygon over open polyline */

      for (i = outline_head; i != NULL; i = i->next)
      {
        if (i->used) continue;

        if ((last_x == i->x1) && (last_y == i->y1))
        {
          if (!segment_connected(i->x2, i->y2, begin_x, begin_y, i)) continue; /* XXX Checkme */
          /* first point of segment is last point of current edge: add segment to edge */
          segment_found = pcb_true;
          perimeter_segment_add(i, pcb_true);
          last_x = i->x2;
          last_y = i->y2;
        }
        else if ((last_x == i->x2) && (last_y == i->y2))
        {
          if (!segment_connected(i->x1, i->y1, begin_x, begin_y, i)) continue; /* XXX Checkme */
          /* last point of segment is last point of current edge: add segment to edge back to front */
          segment_found = pcb_true;
          /* add segment back to front */
          perimeter_segment_add(i, pcb_false);
          last_x = i->x1;
          last_y = i->y1;
        }
        if (segment_found) break;
      }
      polygon_closed = (begin_x == last_x) && (begin_y == last_y);
      if (!polygon_closed && !segment_found)
        break;                   /* can't find anything suitable */
    }
    if (polygon_closed) { 
      if (hyp_debug) pcb_printf("outline: closed\n");
      }
    else 
      {
        if (hyp_debug) pcb_printf("outline: open\n");
        warn_not_closed = pcb_true;
      }
  }

  /* free segment memory */
  for (i = outline_head; i != NULL; i = j)
  {
    j = i->next;
    free (i);
  }
  outline_head = outline_tail = NULL;

  if (warn_not_closed) pcb_message(PCB_MSG_DEBUG, "warning: board outline not closed\n");

  return;
}

/*
 * Hyperlynx STACKUP section 
 */

/*
 * 'OPTIONS' subrecord of 'STACKUP' record.
 * Defines dielectric constant and loss tangent of metal layers.
 */

/*
 * Debug output for layers
 */

void hyp_debug_layer(parse_param *h)
{
  if (hyp_debug) {
    if (h->thickness_set) pcb_printf(" thickness = %mm", z2coord(h->thickness));
    if (h->plating_thickness_set) pcb_printf(" plating_thickness = %mm", z2coord(h->plating_thickness));
    if (h->bulk_resistivity_set) pcb_printf(" bulk_resistivity = %f", h->bulk_resistivity);
    if (h->temperature_coefficient_set) pcb_printf(" temperature_coefficient = %f", h->temperature_coefficient);
    if (h->epsilon_r_set) pcb_printf(" epsilon_r = %f", h->epsilon_r);
    if (h->loss_tangent_set) pcb_printf(" loss_tangent = %f", h->loss_tangent);
    if (h->conformal_set) pcb_printf(" conformal = %i", h->conformal);
    if (h->prepreg_set) pcb_printf(" prepreg = %i", h->prepreg);
    if (h->layer_name_set) pcb_printf(" layer_name = %s", h->layer_name);
    if (h->material_name_set) pcb_printf(" material_name = %s", h->material_name);
    if (h->plane_separation_set) pcb_printf(" plane_separation = %mm", m2coord(h->plane_separation));
    pcb_printf("\n");
    }

  return;
}

pcb_bool exec_options(parse_param *h)
{
  /* Use dielectric for metal? */
  if (hyp_debug) pcb_printf("options: use_die_for_metal = %f\n", h->use_die_for_metal);
  if (h->use_die_for_metal) use_die_for_metal = pcb_true;
  return 0;
}

/*
 * Returns the pcb_layer_id of layer "lname". 
 * If layer lname does not exist, a new copper layer with name "lname" is created.
 * If the layer name is NULL, a new copper layer with a new, unused layer name is created.
 */

pcb_layer_id_t hyp_create_layer(char* lname)
{
  pcb_layer_id_t layer_id;
  char new_layer_name[PCB_MAX_LAYER];
  int n;

  layer_id = -1;
  if (lname != NULL) {
    /* we have a layer name. check whether layer already exists */
    layer_id = pcb_layer_by_name(new_layer_name);
    if (layer_id >= 0) return layer_id; /* found */
    /* create new layer */
    layer_id = pcb_layer_create(PCB_LYT_COPPER, pcb_false, pcb_false, lname); 
    }
  else {
    /* no layer name given. find unused layer name in range 1..PCB_MAX_LAYER */
    for (n = 1; n < PCB_MAX_LAYER; n++) {
      sprintf(new_layer_name,"%i", n);
      if (pcb_layer_by_name(new_layer_name) < 0) {
        /* create new layer */
        layer_id = pcb_layer_create(PCB_LYT_COPPER, pcb_false, pcb_false, new_layer_name);
        break;
        }
      }
    }
  /* check if layer valid */
  if (layer_id < 0) {
    /* layer creation failed. return the first copper layer you can find. */
    if (hyp_debug) pcb_printf("running out of layers\n");
    layer_id = PCB_COMPONENT_SIDE;
    }
  
  return layer_id;
}

/*
 * signal layer 
 */

pcb_bool exec_signal(parse_param *h)
{
  pcb_layer_id_t signal_layer_id;
  signal_layer_id = hyp_create_layer(h->layer_name);

  if (hyp_debug) pcb_printf("signal layer: \"%s\"", pcb_layer_name(signal_layer_id));
  hyp_debug_layer(h);

  /* XXX not forget adapt plane separation */
  return 0;
}

/*
 * dielectric layer 
 */

pcb_bool exec_dielectric(parse_param *h)
{
  if (hyp_debug) pcb_printf("dielectric layer: ");
  hyp_debug_layer(h);
  return 0;
}

/*
 * plane layer - a layer which is flooded with copper.
 */

pcb_bool exec_plane(parse_param *h)
{
  pcb_layer_id_t plane_layer_id;
  plane_layer_id = hyp_create_layer(h->layer_name);
  
  layer_is_plane_layer[plane_layer_id] = pcb_true;
  if (h->plane_separation_set) layer_plane_separation[plane_layer_id] = m2coord(h->plane_separation);

  if (hyp_debug) pcb_printf("plane layer: \"%s\"", pcb_layer_name(plane_layer_id));
  hyp_debug_layer(h);
  return 0;
}

/*
 * devices
 */

pcb_bool exec_devices(parse_param *h)
{
  if (hyp_debug) {
    pcb_printf("device: device_type = %s ref = %s", h->device_type, h->ref);
    if (h->name_set) pcb_printf(" name = %s", h->name);
    if (h->value_float_set) pcb_printf(" value_float = %f", h->value_float);
    if (h->value_string_set) pcb_printf(" value_string = %s", h->value_string);
    if (h->layer_name_set) pcb_printf(" layer_name = %s", h->layer_name);
    if (h->package_set) pcb_printf(" package = %s", h->package);
    pcb_printf("\n");
    }

  return 0;
}

/*
 * supply signals 
 */

pcb_bool exec_supplies(parse_param *h)
{
  if (hyp_debug) pcb_printf("supplies: name = %s value_float = %f voltage_specified = %i conversion = %i\n", h->name, h->value_float, h->voltage_specified, h->conversion);

  return 0;
}

pcb_bool exec_padstack_element(parse_param *h){return(0);}
pcb_bool exec_padstack_end(parse_param *h){return(0);}

pcb_bool exec_net(parse_param *h){return(0);}
pcb_bool exec_net_plane_separation(parse_param *h){return(0);}
pcb_bool exec_net_attribute(parse_param *h){return(0);}
pcb_bool exec_seg(parse_param *h){return(0);}
pcb_bool exec_arc(parse_param *h){return(0);}
pcb_bool exec_via(parse_param *h){return(0);}
pcb_bool exec_via_v1(parse_param *h){return(0);}
pcb_bool exec_pin(parse_param *h){return(0);}
pcb_bool exec_pad(parse_param *h){return(0);}
pcb_bool exec_useg(parse_param *h){return(0);}

pcb_bool exec_polygon_begin(parse_param *h){return(0);}
pcb_bool exec_polygon_end(parse_param *h){return(0);}
pcb_bool exec_polyvoid_begin(parse_param *h){return(0);}
pcb_bool exec_polyvoid_end(parse_param *h){return(0);}
pcb_bool exec_polyline_begin(parse_param *h){return(0);}
pcb_bool exec_polyline_end(parse_param *h){return(0);}
pcb_bool exec_line(parse_param *h){return(0);}
pcb_bool exec_curve(parse_param *h){return(0);}

pcb_bool exec_net_class(parse_param *h){return(0);}
pcb_bool exec_net_class_element(parse_param *h){return(0);}
pcb_bool exec_net_class_attribute(parse_param *h){return(0);}

pcb_bool exec_end(parse_param *h)
{
  if (hyp_debug) pcb_printf("end\n");

  return 0;
}

pcb_bool exec_key(parse_param *h)
{
  if (hyp_debug) pcb_printf("key: key = %s\n", h->key);

  return 0;
}

/*
 * Draw arc from (x1, y1) to (x2, y2) with center (xc, yc) and radius r. 
 * Direction of arc is clockwise or counter-clockwise, depending upon value of pcb_bool_t Clockwise.
 */

pcb_arc_t *hyp_arc_new(pcb_layer_t *Layer, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t XC, pcb_coord_t YC, pcb_coord_t Width, pcb_coord_t Height, pcb_bool_t Clockwise, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_flag_t Flags)
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
    start_angle = 180 + 180*atan2(YC - Y1, X1 - XC)/M_PI;
    end_angle = 180 + 180*atan2(YC - Y2, X2 - XC)/M_PI;
    }
  start_angle = pcb_normalize_angle(start_angle);
  end_angle = pcb_normalize_angle(end_angle);

  if (Clockwise)
    while (start_angle <= end_angle) start_angle += 360;
  else
    while (end_angle <= start_angle) end_angle += 360;

  delta = end_angle - start_angle;

  if (hyp_debug) pcb_printf("hyp_arc: start_angle: %f end_angle: %f delta: %f\n", start_angle, end_angle, delta);

  new_arc = pcb_arc_new(Layer, XC, YC, Width, Height, start_angle, delta, Thickness, Clearance, Flags);

  /* XXX remove debugging code */
  if (hyp_debug) {
    pcb_coord_t x1, y1, x2, y2; 
    pcb_arc_get_end(new_arc, 1, &x1, &y1);
    pcb_arc_get_end(new_arc, 0, &x2, &y2);
    pcb_printf("hyp_arc: start_point: (%mm, %mm) end_point: (%mm, %mm)\n", x1, y1, x2, y2);
    }

  return new_arc;
}

/* not truncated */

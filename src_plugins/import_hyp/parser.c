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
pcb_coord_t plane_separation;       /* distance between PLANE polygon and copper of different nets; -1 if not set */

/*
 * Conversion from hyperlynx to pcb_coord_t - igor2
 */

/* meter to pcb_coord_t */

pcb_coord_t inline meter2coord(double m)
{
  return ((pcb_coord_t) PCB_MM_TO_COORD(1000.0 * m));
}

/* x, y coordinates to pcb_coord_t */

pcb_coord_t inline xy2coord(double f)
{
  return (meter2coord(unit * f));
}

/* z coordinates to pcb_coord_t */

pcb_coord_t inline z2coord(double f)
{
  return (meter2coord(metal_thickness_unit * f));
}

/*
 * initialize physical constants 
 */

void hyp_init(void)
{
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

  outline_head = NULL;
  outline_tail = NULL;

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

  /* parse hyperlynx file */
  hyyin =  fopen(fname, "r");
  if (hyyin == NULL) return (1);
  retval = hyyparse();
  fclose(hyyin);

  /* add board outline */
  hyp_perimeter();

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
  plane_separation = xy2coord(h->plane_separation);

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
  s->used = pcb_true;
  if (forward)
    pcb_printf("\tperimeter segments: %s from (%mm, %mm) to (%mm, %mm)\n", s->is_arc ? "arc" : "line", s->x1, s->y1, s->x2, s->y2);
  else /* add segment back to front */
    pcb_printf("\tperimeter segments: %s from (%mm, %mm) to (%mm, %mm)\n", s->is_arc ? "arc" : "line", s->x2, s->y2, s->x1, s->y1);
}

/* 
 * Check whether point (x1, y1) is connected to point (begin_x, begin_y) via un-used segments
 */
#ifdef XXXX
pcb_bool_t segment_distance(pcb_coord_t begin_x, pcb_coord_t begin_y, outline_t *s)
{
  outline_t *i;
  
  
  s->used = pcb_true;

  for (i = outline_head; i != NULL; i = i->next)
  {
    if (i->used) continue;

    /* recursive descent */
    if ((i->x1 == s->x1) && (i->y1 == s->y1))
    {
      if ((i->x2 == begin_x) && (i->y2 == begin_y)) return pcb_true;
      else if segment_distance(begin_x, begin_y, i) return pcb_true;
    }
    if ((i->x2 == s->x1) && (i->y2 == s->y1))
    {
      if ((i->x1 == begin_x) && (i->y1 == begin_y)) return pcb_true;
      else if segment_distance(begin_x, begin_y, i) return pcb_true;
    }
  }

  s->used = pcb_false;

  return min_dist;
}
#endif

/* 
 * Draw board perimeter.
 * The first segment is part of the first polygon.
 * The first polygon of the board perimeter is positive, the rest are holes.
 * Segments do not necesarily occur in order.
 */

void hyp_perimeter()
{
  pcb_bool_t segment_found;
  pcb_bool_t polygon_closed;
  pcb_coord_t begin_x, begin_y, last_x, last_y;
  outline_t *i;
  outline_t *j;

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

      for (i = outline_head; i != NULL; i = i->next)
      {
        if (i->used) continue;

        if ((last_x == i->x1) && (last_y == i->y1))
        {
          /* first point of segment is last point of current edge: add segment to edge */
          segment_found = pcb_true;
          perimeter_segment_add(i, pcb_true);
          last_x = i->x2;
          last_y = i->y2;
        }
        else if ((last_x == i->x2) && (last_y == i->y2))
        {
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
    if (polygon_closed) pcb_printf("perimeter: closed\n");
    else pcb_printf("perimeter: open\n");
  }

  /* free segment memory */
  for (i = outline_head; i != NULL; i = j)
  {
    j = i->next;
    free (i);
  }
  outline_head = outline_tail = NULL;

  return;
}

pcb_bool exec_options(parse_param *h){return(0);}
pcb_bool exec_signal(parse_param *h){return(0);}
pcb_bool exec_dielectric(parse_param *h){return(0);}
pcb_bool exec_plane(parse_param *h){return(0);}

pcb_bool exec_devices(parse_param *h){return(0);}

pcb_bool exec_supplies(parse_param *h){return(0);}

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

pcb_bool exec_end(parse_param *h){return(0);}
pcb_bool exec_key(parse_param *h){return(0);}

/* not truncated */

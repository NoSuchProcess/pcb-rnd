/*
 * read hyperlynx files
 * Copyright 2012 Koen De Vleeschauwer.
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

/* x, y coordinates to pcb_coord_t */

pcb_coord_t inline xy2coord(double f)
{
  return ((pcb_coord_t) PCB_MM_TO_COORD(1000.0 * unit * f));
}

/* z coordinates to pcb_coord_t */

pcb_coord_t inline z2coord(double f)
{
  return ((pcb_coord_t) PCB_MM_TO_COORD(1000.0 * metal_thickness_unit * f));
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
  if (hyp_debug) pcb_message(PCB_MSG_DEBUG, "board\n");

  return 0;
}

/*
 * Hyperlynx 'VERSION' record.
 * Specifies version number.
 * Required record; must be first record of the file.
 */

pcb_bool exec_version(parse_param *h)
{
  if (hyp_debug) pcb_message(PCB_MSG_DEBUG, "version: vers = %f\n", h->vers);

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
  if (hyp_debug) pcb_message(PCB_MSG_DEBUG, "data_mode: detailed = %i\n", h->detailed);

  return 0;
}

/*
 * Hyperlynx 'UNITS' record.
 * Specifies measurement system (english/metric) for the rest of the file.
 */

pcb_bool exec_units(parse_param *h)
{
  if (hyp_debug) pcb_message(PCB_MSG_DEBUG, "units: unit_system_english = %d metal_thickness_weight = %d\n", h->unit_system_english, h->metal_thickness_weight);

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

  if (hyp_debug) pcb_message(PCB_MSG_DEBUG, "units: unit = %f metal_thickness_unit = %f\n", unit, metal_thickness_unit);

  return 0;
}

/*
 * Hyperlynx 'PLANE_SEP' record.
 * Defines default trace to plane separation
 */

pcb_bool exec_plane_sep(parse_param *h)
{
  if (hyp_debug) pcb_message(PCB_MSG_DEBUG, "plane_sep: default_plane_separation = %d\n", h->plane_separation);

  plane_separation = xy2coord(h->plane_separation);

  return 0;
}

/*
 * Hyperlynx 'PERIMETER_SEGMENT' subrecord of 'BOARD' record.
 * Draws linear board outline segment.
 */

pcb_bool exec_perimeter_segment(parse_param *h)
{
  if (hyp_debug) pcb_message(PCB_MSG_DEBUG, "perimeter_segment: x1 = %f y1 = %f x2 = %f y2 = %f\n", h->x1, h->y1, h->x2, h->y2);

  return 0;
}

/*
 * Hyperlynx 'PERIMETER_ARC' subrecord of 'BOARD' record.
 * Draws arc segment of board outline. Arc drawn counterclockwise.
 */

pcb_bool exec_perimeter_arc(parse_param *h)
{
  if (hyp_debug) pcb_message(PCB_MSG_DEBUG, "perimeter_arc: x1 = %f y1 = %f x2 = %f y2 = %f xc = %f yc = %f r = %f\n", h->x1, h->y1, h->x2, h->y2, h->xc, h->yc, h->r);

  return 0;
}

/*
 * Hyperlynx 'A' attribute subrecord of 'BOARD' record.
 * Defines board attributes as name/value pairs.
 */

pcb_bool exec_board_attribute(parse_param *h)
{
  if (hyp_debug) pcb_message(PCB_MSG_DEBUG, "board_attribute: name = %s value = %s\n", h->name, h->value);

  return 0;
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

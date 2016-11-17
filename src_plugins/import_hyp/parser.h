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

#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include "pcb_bool.h"
#include "board.h"

  /* 
   * Parameters passed on by the parser.
   * All variables added here are initialized in new_record().
   */

  typedef enum { PAD_TYPE_METAL, PAD_TYPE_ANTIPAD, PAD_TYPE_THERMAL_RELIEF } pad_type_enum;

  typedef enum { PIN_SIM_IN, PIN_SIM_OUT, PIN_SIM_BOTH } pin_function_enum;

  typedef enum { POLYGON_TYPE_POUR, POLYGON_TYPE_PLANE, POLYGON_TYPE_COPPER, POLYGON_TYPE_PAD, POLYGON_TYPE_ANTIPAD } polygon_type_enum;

  typedef struct {
    double vers;   /* version of the hyp file format */
    pcb_bool detailed; /* data detailed enough for power integrity */
    pcb_bool unit_system_english; /* english or metric units */
    pcb_bool metal_thickness_weight; /* copper by weight or by length */
    double default_plane_separation; /* trace to plane separation */
    
    /* stackup record */
    pcb_bool use_die_for_metal; /* dielectric constant and loss tangent of dielectric for metal layers */
    double bulk_resistivity;
    pcb_bool conformal;
    double epsilon_r;
    char *layer_name;
    double loss_tangent;
    char *material_name;
    double plane_separation;
    double plating_thickness;
    pcb_bool prepreg;
    double temperature_coefficient; /* temperature coefficient of resistivity */
    double thickness; /* layer thickness */

    /* stackup record flags */
    pcb_bool bulk_resistivity_set;
    pcb_bool conformal_set;
    pcb_bool epsilon_r_set;
    pcb_bool layer_name_set;
    pcb_bool loss_tangent_set;
    pcb_bool material_name_set;
    pcb_bool plane_separation_set;
    pcb_bool plating_thickness_set;
    pcb_bool prepreg_set;
    pcb_bool temperature_coefficient_set;
    pcb_bool thickness_set;
    
    /* device record */
    char *device_type;
    char *ref;
    double value_float;
    char *value_string;
    char *package;

    /* device record flags */
    pcb_bool name_set;
    pcb_bool value_float_set;
    pcb_bool value_string_set;
    pcb_bool package_set;

    /* supplies record */
    pcb_bool voltage_specified;
    pcb_bool conversion;

    /* padstack record */
    char *padstack_name;
    double drill_size;
    double pad_shape;
    double pad_sx;
    double pad_sy;
    double pad_angle;
    double thermal_clear_shape;
    double thermal_clear_sx;
    double thermal_clear_sy;
    double thermal_clear_angle;
    pad_type_enum pad_type;

    /* padstack record flags */
    pcb_bool padstack_name_set;
    pcb_bool drill_size_set;
    pcb_bool pad_type_set;

    /* net record */
    double width;
    double left_plane_separation;
    pcb_bool width_set;
    pcb_bool left_plane_separation_set;

    /* via subrecord of net */
    char *layer1_name;
    pcb_bool layer1_name_set;
    char *layer2_name;
    pcb_bool layer2_name_set;
    char *pad1_shape;
    double pad1_sx;
    double pad1_sy;
    double pad1_angle;
    char *pad2_shape;
    double pad2_sx;
    double pad2_sy;
    double pad2_angle;

    /* pin subrecord of net */
    char *pin_reference;
    pcb_bool pin_reference_set;
    pin_function_enum pin_function;
    pcb_bool pin_function_set;

    /* useg subrecord of net */
    char *zlayer_name;
    pcb_bool zlayer_name_set;
    double length;
    double impedance;
    pcb_bool impedance_set;
    double delay;
    double resistance;
    pcb_bool resistance_set;

    /* polygon subrecord of net */
    int id;
    pcb_bool id_set;
    polygon_type_enum polygon_type;
    pcb_bool polygon_type_set;

    /* net class record */
    char *net_class_name;
    char *net_name;

    /* key record */
    char *key;

    /* Attributes */
    char *name; /* attribute name */
    char *value; /* attribute value */
    
    /* point, line and arc coordinates */
    double x; /* coordinates point */
    double y; /* coordinates point */
    double x1; /* coordinates point 1 */
    double y1; /* coordinates point 1 */
    double x2; /* coordinates point 2 */
    double y2; /* coordinates point 2 */
    double xc; /* coordinates arc */
    double yc; /* coordinates arc */
    double r; /* coordinates arc */
  } parse_param;

  /* exec_* routines are called by parser to interpret hyperlynx file */ 
  pcb_bool exec_board_file(parse_param *h);
  pcb_bool exec_version(parse_param *h);
  pcb_bool exec_data_mode(parse_param *h);
  pcb_bool exec_units(parse_param *h);
  pcb_bool exec_plane_sep(parse_param *h);
  pcb_bool exec_perimeter_segment(parse_param *h);
  pcb_bool exec_perimeter_arc(parse_param *h);
  pcb_bool exec_board_attribute(parse_param *h);
  
  pcb_bool exec_options(parse_param *h);
  pcb_bool exec_signal(parse_param *h);
  pcb_bool exec_dielectric(parse_param *h);
  pcb_bool exec_plane(parse_param *h);
  
  pcb_bool exec_devices(parse_param *h);
  
  pcb_bool exec_supplies(parse_param *h);
  
  pcb_bool exec_padstack_element(parse_param *h);
  pcb_bool exec_padstack_end(parse_param *h);
  
  pcb_bool exec_net(parse_param *h);
  pcb_bool exec_net_plane_separation(parse_param *h);
  pcb_bool exec_net_attribute(parse_param *h);
  pcb_bool exec_seg(parse_param *h);
  pcb_bool exec_arc(parse_param *h);
  pcb_bool exec_via(parse_param *h);
  pcb_bool exec_via_v1(parse_param *h); /* Old style via format */ 
  pcb_bool exec_pin(parse_param *h);
  pcb_bool exec_pad(parse_param *h);
  pcb_bool exec_useg(parse_param *h);
  
  pcb_bool exec_polygon_begin(parse_param *h);
  pcb_bool exec_polygon_end(parse_param *h);
  pcb_bool exec_polyvoid_begin(parse_param *h);
  pcb_bool exec_polyvoid_end(parse_param *h);
  pcb_bool exec_polyline_begin(parse_param *h);
  pcb_bool exec_polyline_end(parse_param *h);
  pcb_bool exec_line(parse_param *h);
  pcb_bool exec_curve(parse_param *h);
  
  pcb_bool exec_net_class(parse_param *h);
  pcb_bool exec_net_class_element(parse_param *h);
  pcb_bool exec_net_class_attribute(parse_param *h);
  
  pcb_bool exec_end(parse_param *h);
  pcb_bool exec_key(parse_param *h);

  /* called by pcb-rnd to load hyperlynx file */
  pcb_bool hyp_parse(pcb_data_t *dest, const char *fname, int debug);
  void hyp_error(const char *msg);

#endif 

  /* not truncated */


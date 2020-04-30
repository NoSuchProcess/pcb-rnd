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

#ifndef PCB_IO_HYP_PARSER_H
#define PCB_IO_HYP_PARSER_H

#include <stdio.h>
#include <librnd/core/pcb_bool.h>
#include "board.h"

	/* 
	 * Parameters passed on by the parser.
	 * All variables added here are initialized in new_record().
	 */

typedef enum { PAD_TYPE_METAL, PAD_TYPE_ANTIPAD, PAD_TYPE_THERMAL_RELIEF } pad_type_enum;

typedef enum { PIN_SIM_IN, PIN_SIM_OUT, PIN_SIM_BOTH } pin_function_enum;

typedef enum { POLYGON_TYPE_POUR, POLYGON_TYPE_PLANE, POLYGON_TYPE_COPPER, POLYGON_TYPE_PAD,
	POLYGON_TYPE_ANTIPAD
} polygon_type_enum;

typedef struct {
	double vers;									/* version of the hyp file format */
	rnd_bool detailed;						/* data detailed enough for power integrity */
	rnd_bool unit_system_english;	/* english or metric units */
	rnd_bool metal_thickness_weight;	/* copper by weight or by length */
	double default_plane_separation;	/* trace to plane separation */

	/* stackup record */
	rnd_bool use_die_for_metal;		/* dielectric constant and loss tangent of dielectric for metal layers */
	double bulk_resistivity;
	rnd_bool conformal;
	double epsilon_r;
	char *layer_name;
	double loss_tangent;
	char *material_name;
	double plane_separation;
	double plating_thickness;
	rnd_bool prepreg;
	double temperature_coefficient;	/* temperature coefficient of resistivity */
	double thickness;							/* layer thickness */

	/* stackup record flags */
	rnd_bool bulk_resistivity_set;
	rnd_bool conformal_set;
	rnd_bool epsilon_r_set;
	rnd_bool layer_name_set;
	rnd_bool loss_tangent_set;
	rnd_bool material_name_set;
	rnd_bool plane_separation_set;
	rnd_bool plating_thickness_set;
	rnd_bool prepreg_set;
	rnd_bool temperature_coefficient_set;
	rnd_bool thickness_set;

	/* device record */
	char *device_type;
	char *ref;
	double value_float;
	char *value_string;
	char *package;

	/* device record flags */
	rnd_bool name_set;
	rnd_bool value_float_set;
	rnd_bool value_string_set;
	rnd_bool package_set;

	/* supplies record */
	rnd_bool voltage_specified;
	rnd_bool conversion;

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
	rnd_bool padstack_name_set;
	rnd_bool drill_size_set;
	rnd_bool pad_type_set;

	/* net record */
	double width;
	double left_plane_separation;
	rnd_bool width_set;
	rnd_bool left_plane_separation_set;

	/* via subrecord of net */
	char *layer1_name;
	rnd_bool layer1_name_set;
	char *layer2_name;
	rnd_bool layer2_name_set;
	char *via_pad_shape;
	rnd_bool via_pad_shape_set;
	double via_pad_sx;
	rnd_bool via_pad_sx_set;
	double via_pad_sy;
	rnd_bool via_pad_sy_set;
	double via_pad_angle;
	rnd_bool via_pad_angle_set;
	char *via_pad1_shape;
	rnd_bool via_pad1_shape_set;
	double via_pad1_sx;
	rnd_bool via_pad1_sx_set;
	double via_pad1_sy;
	rnd_bool via_pad1_sy_set;
	double via_pad1_angle;
	rnd_bool via_pad1_angle_set;
	char *via_pad2_shape;
	rnd_bool via_pad2_shape_set;
	double via_pad2_sx;
	rnd_bool via_pad2_sx_set;
	double via_pad2_sy;
	rnd_bool via_pad2_sy_set;
	double via_pad2_angle;
	rnd_bool via_pad2_angle_set;

	/* pin subrecord of net */
	char *pin_reference;
	rnd_bool pin_reference_set;
	pin_function_enum pin_function;
	rnd_bool pin_function_set;

	/* useg subrecord of net */
	char *zlayer_name;
	rnd_bool zlayer_name_set;
	double length;
	double impedance;
	rnd_bool impedance_set;
	double delay;
	double resistance;
	rnd_bool resistance_set;

	/* polygon subrecord of net */
	int id;
	rnd_bool id_set;
	polygon_type_enum polygon_type;
	rnd_bool polygon_type_set;

	/* net class record */
	char *net_class_name;
	char *net_name;

	/* key record */
	char *key;

	/* Attributes */
	char *name;										/* attribute name */
	char *value;									/* attribute value */

	/* point, line and arc coordinates */
	double x;											/* coordinates point */
	double y;											/* coordinates point */
	double x1;										/* coordinates point 1 */
	double y1;										/* coordinates point 1 */
	double x2;										/* coordinates point 2 */
	double y2;										/* coordinates point 2 */
	double xc;										/* coordinates arc */
	double yc;										/* coordinates arc */
	double r;											/* coordinates arc */
} parse_param;

	/* exec_* routines are called by parser to interpret hyperlynx file */
rnd_bool exec_board_file(parse_param * h);
rnd_bool exec_version(parse_param * h);
rnd_bool exec_data_mode(parse_param * h);
rnd_bool exec_units(parse_param * h);
rnd_bool exec_plane_sep(parse_param * h);
rnd_bool exec_perimeter_segment(parse_param * h);
rnd_bool exec_perimeter_arc(parse_param * h);
rnd_bool exec_board_attribute(parse_param * h);

rnd_bool exec_options(parse_param * h);
rnd_bool exec_signal(parse_param * h);
rnd_bool exec_dielectric(parse_param * h);
rnd_bool exec_plane(parse_param * h);

rnd_bool exec_devices(parse_param * h);

rnd_bool exec_supplies(parse_param * h);

rnd_bool exec_pstk_element(parse_param * h);
rnd_bool exec_pstk_end(parse_param * h);

rnd_bool exec_net(parse_param * h);
rnd_bool exec_net_plane_separation(parse_param * h);
rnd_bool exec_net_attribute(parse_param * h);
rnd_bool exec_seg(parse_param * h);
rnd_bool exec_arc(parse_param * h);
rnd_bool exec_via(parse_param * h);
rnd_bool exec_via_v1(parse_param * h);	/* Old style via format */
rnd_bool exec_pin(parse_param * h);
rnd_bool exec_pad(parse_param * h);
rnd_bool exec_useg(parse_param * h);

rnd_bool exec_polygon_begin(parse_param * h);
rnd_bool exec_polygon_end(parse_param * h);
rnd_bool exec_polyvoid_begin(parse_param * h);
rnd_bool exec_polyvoid_end(parse_param * h);
rnd_bool exec_polyline_begin(parse_param * h);
rnd_bool exec_polyline_end(parse_param * h);
rnd_bool exec_line(parse_param * h);
rnd_bool exec_curve(parse_param * h);

rnd_bool exec_net_class(parse_param * h);
rnd_bool exec_net_class_element(parse_param * h);
rnd_bool exec_net_class_attribute(parse_param * h);

rnd_bool exec_end(parse_param * h);
rnd_bool exec_key(parse_param * h);

	/* called by pcb-rnd to load hyperlynx file */
rnd_bool hyp_parse(pcb_data_t * dest, const char *fname, int debug);
void hyp_error(const char *msg);

	/* create arc, hyperlynx-style */
pcb_arc_t *hyp_arc_new(pcb_layer_t * Layer, rnd_coord_t X1, rnd_coord_t Y1, rnd_coord_t X2, rnd_coord_t Y2, rnd_coord_t XC,
											 rnd_coord_t YC, rnd_coord_t Width, rnd_coord_t Height, rnd_bool_t Clockwise, rnd_coord_t Thickness,
											 rnd_coord_t Clearance, pcb_flag_t Flags);

#endif

	/* not truncated */

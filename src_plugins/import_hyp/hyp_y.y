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

%code requires {
#include "parser.h"
}

%error-verbose
%debug
%defines "hyp_y.h"
%define api.prefix {hyy}

%union {
    int boolval;
    int intval;
    double floatval;
    char* strval;
}

%{
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void hyyerror(const char *);

/* HYYPRINT and hyyprint print values of the tokens when debugging is switched on */
void hyyprint(FILE *, int, HYYSTYPE);
#define HYYPRINT(file, type, value) hyyprint (file, type, value)

/* clear parse_param struct at beginning of new record */
void new_record();

/* struct to pass to calling class */
parse_param h;

%}

/*
 * Hyperlynx keywords
 */

 /* Punctuation: { } ( ) = , */

 /* Sections */

%token H_BOARD_FILE H_VERSION H_DATA_MODE H_UNITS H_PLANE_SEP
%token H_BOARD H_STACKUP H_DEVICES H_SUPPLIES
%token H_PAD H_PADSTACK H_NET H_NET_CLASS H_END H_KEY

 /* Keywords */

%token H_A H_ARC H_COPPER H_CURVE H_DETAILED H_DIELECTRIC H_ENGLISH H_LENGTH
%token H_LINE H_METRIC H_N H_OPTIONS H_PERIMETER_ARC H_PERIMETER_SEGMENT H_PIN
%token H_PLANE H_POLYGON H_POLYLINE H_POLYVOID H_POUR H_S H_SEG H_SIGNAL
%token H_SIMPLIFIED H_SIM_BOTH H_SIM_IN H_SIM_OUT H_USEG H_VIA H_WEIGHT

 /* Assignments */

%token H_A1 H_A2 H_BR H_C H_C_QM H_CO_QM H_D H_ER H_F H_ID
%token H_L H_L1 H_L2 H_LPS H_LT H_M H_NAME
%token H_P H_PKG H_PR_QM H_PS H_R H_REF H_SX H_SY H_S1 H_S1X H_S1Y H_S2 H_S2X H_S2Y H_T H_TC
%token H_USE_DIE_FOR_METAL H_V H_V_QM H_VAL H_W H_X H_X1 H_X2
%token H_XC H_Y H_Y1 H_Y2 H_YC H_Z H_ZL H_ZLEN H_ZW
 
 /* Booleans */

%token H_YES H_NO

%token <boolval> H_BOOL
%token <intval> H_POSINT
%token <floatval> H_FLOAT
%token <strval> H_STRING

%start hyp_file

%%

/*
 * Note:
 * Use left recursion when parsing board perimeter and nets. 
 * When using left recursion cpu time is linear with board size. 
 * When using right recursion we run out of memory on large boards. 
 * (Typical error message: line xxx: memory exhausted at 'yyy' )
 */

 /* 
    hyperlynx file sections: 

    board_file
    version 
    data_mode* 
    units 
    plane_sep* 
    board* 
    stackup* 
    devices 
    supplies* 
    padstack* 
    net 
    net_class* 
    end

    * = optional section

  */

hyp_file
  : hyp_file hyp_section
  | hyp_section ;

hyp_section
  : board_file
  | version 
  | data_mode 
  | units
  | plane_sep 
  | board 
  | stackup 
  | devices 
  | supplies 
  | padstack 
  | net 
  | netclass 
  | end 
  | key 
  | '{' error '}' ;

  /* board_file */

board_file
  : '{' H_BOARD_FILE { if (exec_board_file(&h)) YYERROR; } '}' ;

  /* version */

version
  : '{' H_VERSION '=' H_FLOAT { h.vers = yylval.floatval; } '}' { if (exec_version(&h)) YYERROR; } ;

  /* data_mode */

data_mode
  : '{' H_DATA_MODE '=' mode '}' { if (exec_data_mode(&h)) YYERROR; };

mode
  : H_SIMPLIFIED { h.detailed = pcb_false; }
  | H_DETAILED   { h.detailed = pcb_true; } ;

  /* units */

units
  : '{' H_UNITS '=' unit_system metal_thickness_unit '}' { if (exec_units(&h)) YYERROR; } ;

unit_system
  : H_ENGLISH { h.unit_system_english = pcb_true; }
  | H_METRIC  { h.unit_system_english = pcb_false; };

metal_thickness_unit
  : H_WEIGHT  { h.metal_thickness_weight = pcb_true; }
  | H_LENGTH  { h.metal_thickness_weight = pcb_false; } ;

  /* plane_sep */
plane_sep
  : '{' H_PLANE_SEP '=' H_FLOAT { h.default_plane_separation = yylval.floatval; } '}' { if (exec_plane_sep(&h)) YYERROR; } ;

  /* board */

board
  : '{' H_BOARD board_param_list '}'
  | '{' H_BOARD '}' ;
 
board_param_list
  : board_param_list board_param_list_item
  | board_param_list_item ;

board_param_list_item
  : '(' board_param ')'
  | '(' board_param { hyyerror("warning: missing ')'"); }
  ;

board_param
  : perimeter_segment
  | perimeter_arc
  | board_attribute 
  | error ;

perimeter_segment
  : H_PERIMETER_SEGMENT coord_line { if (exec_perimeter_segment(&h)) YYERROR; } ;

perimeter_arc
  : H_PERIMETER_ARC coord_arc { if (exec_perimeter_arc(&h)) YYERROR; } ;

board_attribute
  : H_A H_N '=' H_STRING { h.name = yylval.strval; } H_V '=' H_STRING { h.value = yylval.strval; } { if (exec_board_attribute(&h)) YYERROR; } ;

  /* stackup */

stackup
  : '{' H_STACKUP stackup_paramlist '}' ;

stackup_paramlist
  : stackup_paramlist stackup_param
  | stackup_param ;

stackup_param
  : options
  | signal
  | dielectric
  | plane
  | '(' error ')' ;

options
  : '(' H_OPTIONS options_params { if (exec_options(&h)) YYERROR; } ;

options_params
  : H_USE_DIE_FOR_METAL '=' H_BOOL { h.use_die_for_metal = yylval.boolval; } ')'
  | ')'
  ;

signal
  : '(' H_SIGNAL { new_record(); } signal_paramlist ')' { if (exec_signal(&h)) YYERROR; } ;

signal_paramlist
  : signal_paramlist signal_param
  | signal_param ;

signal_param
  : thickness
  | plating_thickness
  | H_C  '=' H_FLOAT { h.bulk_resistivity = yylval.floatval; h.bulk_resistivity_set = pcb_true; }
  | bulk_resistivity
  | temperature_coefficient
  | epsilon_r
  | loss_tangent
  | layer_name
  | material_name
  | plane_separation ;

dielectric
  : '(' H_DIELECTRIC { new_record(); } dielectric_paramlist ')' { if (exec_dielectric(&h)) YYERROR; } ;

dielectric_paramlist
  : dielectric_paramlist dielectric_param
  | dielectric_param ;

dielectric_param
  : thickness
  | H_C '=' H_FLOAT { h.epsilon_r = yylval.floatval; h.epsilon_r_set = pcb_true; }
  | epsilon_r
  | loss_tangent
  | conformal
  | prepreg
  | layer_name
  | material_name
  ;

plane
  : '(' H_PLANE { new_record(); } plane_paramlist ')' { if (exec_plane(&h)) YYERROR; } ;

plane_paramlist
  : plane_paramlist plane_param
  | plane_param ;

plane_param
  : thickness
  | H_C  '=' H_FLOAT { h.bulk_resistivity = yylval.floatval; h.bulk_resistivity_set = pcb_true; }
  | bulk_resistivity
  | temperature_coefficient
  | epsilon_r
  | loss_tangent
  | layer_name
  | material_name
  | plane_separation ;

thickness
  : H_T  '=' H_FLOAT  { h.thickness = yylval.floatval; h.thickness_set = pcb_true; }

plating_thickness
  : H_P  '=' H_FLOAT  { h.plating_thickness = yylval.floatval; h.plating_thickness_set = pcb_true; }

bulk_resistivity
  : H_BR '=' H_FLOAT  { h.bulk_resistivity = yylval.floatval; h.bulk_resistivity_set = pcb_true; }

temperature_coefficient
  : H_TC '=' H_FLOAT  { h.temperature_coefficient = yylval.floatval; h.temperature_coefficient_set = pcb_true; }

epsilon_r
  : H_ER '=' H_FLOAT  { h.epsilon_r = yylval.floatval; h.epsilon_r_set = pcb_true; }

loss_tangent
  : H_LT '=' H_FLOAT  { h.loss_tangent = yylval.floatval; h.loss_tangent_set = pcb_true; }

layer_name
  : H_L '=' H_STRING { h.layer_name = yylval.strval; h.layer_name_set = pcb_true; }

material_name
  : H_M  '=' H_STRING { h.material_name = yylval.strval; h.material_name_set = pcb_true; }

plane_separation
  : H_PS '=' H_FLOAT  { h.plane_separation = yylval.floatval; h.plane_separation_set = pcb_true; }

conformal
  : H_CO_QM '=' H_BOOL { h.conformal = yylval.boolval; h.conformal_set = pcb_true; }

prepreg
  : H_PR_QM '=' H_BOOL { h.prepreg = yylval.boolval; h.prepreg_set = pcb_true; }

  /* devices */

devices
  : '{' H_DEVICES device_list '}'
  | '{' H_DEVICES '}' ;

device_list
  : device_list device 
  | device ;

device
  : '(' { new_record(); } H_STRING { h.device_type = yylval.strval; } H_REF '=' H_STRING  { h.ref = yylval.strval; } device_paramlist ')' { if (exec_devices(&h)) YYERROR; } 
  | '(' error ')' ;

device_paramlist
  : name device_value
  | device_value 
  ;
 
device_value
  : value device_layer
  | device_layer 
  ;

device_layer
  : layer_name package 
  | layer_name
  ;

name
  : H_NAME '=' H_STRING { h.name = yylval.strval; h.name_set = pcb_true; } ;

value
  : value_float
  | value_string
  ;

value_float
  : H_VAL '=' H_FLOAT   { h.value_float = yylval.floatval; h.value_float_set = pcb_true; } ;

value_string
  : H_VAL '=' H_STRING  { h.value_string = yylval.strval; h.value_string_set = pcb_true; } ;

package
  : H_PKG '=' H_STRING  { h.package = yylval.strval; h.package_set = pcb_true; } ;

  /* supplies */

supplies
  : '{' H_SUPPLIES supply_list '}' ;

supply_list
  : supply_list supply
  | supply ;

supply
  : '(' H_S name value_float voltage_spec conversion ')'  { if (exec_supplies(&h)) YYERROR; } 
  | '(' error ')' ;

voltage_spec
  : H_V_QM '=' H_BOOL { h.voltage_specified = yylval.boolval; } ;

conversion
  : H_C_QM '=' H_BOOL { h.conversion = yylval.boolval; }

  /* padstack */

padstack
  : '{' H_PADSTACK { new_record(); } '=' H_STRING { h.padstack_name = yylval.strval; h.padstack_name_set = pcb_true; } drill_size '}' { if (exec_padstack_end(&h)) YYERROR; } ;

drill_size 
  : ',' H_FLOAT { h.drill_size = yylval.floatval; h.drill_size_set = pcb_true; } padstack_list
  | ',' padstack_list ;
  | padstack_list ;

padstack_list
  : padstack_list padstack_def 
  | padstack_def ;

padstack_def
  : '(' H_STRING { h.layer_name = yylval.strval; h.layer_name_set = pcb_true; } ',' pad_shape pad_coord pad_type { if (exec_padstack_element(&h)) YYERROR; new_record(); }
  | '(' error ')' ;

pad_shape
  : H_FLOAT { h.pad_shape = yylval.floatval; } ','
  | ',' { h.pad_shape = -1; } /* Workaround: Altium sometimes prints an empty pad shape */
  ;

pad_coord
  : H_FLOAT { h.pad_sx = yylval.floatval; } ',' H_FLOAT { h.pad_sy = yylval.floatval; } ',' H_FLOAT { h.pad_angle = yylval.floatval; } 

pad_type
  : ')' 
  | ',' H_M ')' { h.pad_type = PAD_TYPE_METAL; h.pad_type_set = pcb_true; } 
  | ',' H_A ')' { h.pad_type = PAD_TYPE_ANTIPAD; h.pad_type_set = pcb_true; } 
  | ',' H_FLOAT { h.thermal_clear_shape = yylval.floatval; } 
    ',' H_FLOAT { h.thermal_clear_sx = yylval.floatval; } 
    ',' H_FLOAT { h.thermal_clear_sy = yylval.floatval; } 
    ',' H_FLOAT { h.thermal_clear_angle = yylval.floatval; } 
    ',' H_T ')' { h.pad_type = PAD_TYPE_THERMAL_RELIEF; h.pad_type_set = pcb_true; } 
  ;

  /* net */

net
  : '{' H_NET '=' H_STRING { h.net_name = yylval.strval; if (exec_net(&h)) YYERROR; } net_separation ;

net_separation
  : plane_separation { if (exec_net_plane_separation(&h)) YYERROR; } net_copper
  | net_copper

net_copper
  : net_subrecord_list '}'
  | { hyyerror("warning: empty net"); } '}'
  ;

net_subrecord_list
  : net_subrecord_list net_subrecord
  | net_subrecord ;

net_subrecord
  : seg 
  | arc
  | via 
  | pin 
  | pad 
  | useg 
  | polygon 
  | polyvoid 
  | polyline 
  | net_attribute 
  | '(' error ')'
  | '{' error '}'
  ;

seg
  : '(' H_SEG { new_record(); } coord_line width layer_name ps_lps_param { if (exec_seg(&h)) YYERROR; } ;

arc
  : '(' H_ARC { new_record(); } coord_arc width layer_name ps_lps_param { if (exec_arc(&h)) YYERROR; } ;

ps_lps_param
  : plane_separation lps_param
  | lps_param
  ;

lps_param
  : left_plane_separation ')'
  | ')'
  ;

width
  : H_W '=' H_FLOAT    { h.width = yylval.floatval; h.width_set = pcb_true; } ;
  
left_plane_separation
  : H_LPS '=' H_FLOAT { h.left_plane_separation = yylval.floatval; h.left_plane_separation_set = pcb_true; } ;

via
  : '(' H_VIA { new_record(); } coord_point via_new_or_old_style
  ;

via_new_or_old_style
  : via_new_style
  | via_old_style
  ;

via_new_style
  : via_new_style_l1_param { if (exec_via(&h)) YYERROR; } ; 

via_new_style_l1_param
  : layer1_name via_new_style_l2_param
  | via_new_style_l2_param 
  ;

via_new_style_l2_param
  : layer2_name via_new_style_padstack_param
  | via_new_style_padstack_param 
  ;

via_new_style_padstack_param
  : padstack_name ')'
  ;

padstack_name
  : H_P '=' H_STRING  { h.padstack_name = yylval.strval; h.padstack_name_set = pcb_true; } ;

layer1_name
  : H_L1 '=' H_STRING { h.layer1_name = yylval.strval; h.layer1_name_set = pcb_true; } ;

layer2_name
  : H_L2 '=' H_STRING { h.layer2_name = yylval.strval; h.layer2_name_set = pcb_true; } ;

via_old_style
  : H_D '=' H_FLOAT   { h.drill_size = yylval.floatval; } /* deprecated hyperlynx v1.x VIA format */
    layer1_name
    layer2_name
    H_S1 '=' H_STRING { h.pad1_shape = yylval.strval; } 
    H_S1X '=' H_FLOAT { h.pad1_sx = yylval.floatval; }
    H_S1Y '=' H_FLOAT { h.pad1_sy = yylval.floatval; }
    H_A1 '=' H_FLOAT  { h.pad1_angle = yylval.floatval; }
    H_S2 '=' H_STRING { h.pad2_shape = yylval.strval; } 
    H_S2X '=' H_FLOAT { h.pad2_sx = yylval.floatval; }
    H_S2Y '=' H_FLOAT { h.pad2_sy = yylval.floatval; }
    H_A2 '=' H_FLOAT  { h.pad2_angle  = yylval.floatval; }
    ')' { if (exec_via_v1(&h)) YYERROR; } ;
  ;

pin
  : '(' H_PIN { new_record(); } coord_point pin_reference pin_param { if (exec_pin(&h)) YYERROR; } ;

pin_param
  : padstack_name pin_function_param
  | pin_function_param
  ;

pin_function_param
  : pin_function ')'
  | ')'
  ;

pin_reference
  : H_R '=' H_STRING  { h.pin_reference = yylval.strval; h.pin_reference_set = pcb_true; } ;

pin_function
  : H_F '=' H_SIM_OUT  { h.pin_function = PIN_SIM_OUT; h.pin_function_set = pcb_true; }
  | H_F '=' H_SIM_IN   { h.pin_function = PIN_SIM_IN; h.pin_function_set = pcb_true; }
  | H_F '=' H_SIM_BOTH { h.pin_function = PIN_SIM_BOTH; h.pin_function_set = pcb_true; }
  ;

pad
  : '(' H_PAD { new_record(); } /* deprecated hyperlynx v1.x only */
    coord_point
    layer_name
    H_S '=' H_STRING  { h.pad1_shape = yylval.strval; }
    H_SX '=' H_FLOAT  { h.pad1_sx = yylval.floatval; }
    H_SY '=' H_FLOAT  { h.pad1_sy = yylval.floatval; }
    H_A '=' H_FLOAT   { h.pad1_angle = yylval.floatval; }
    ')' { if (exec_pad(&h)) YYERROR; } ;
  ;

useg
  : '(' H_USEG { new_record(); } coord_point1 layer1_name coord_point2 layer2_name useg_param { if (exec_useg(&h)) YYERROR; } ; 

useg_param
  : useg_stackup
  | useg_impedance
  ;

useg_stackup
  : H_ZL '=' H_STRING  { h.zlayer_name = yylval.strval; h.zlayer_name_set = pcb_true; } 
    H_ZW '=' H_FLOAT   { h.width = yylval.floatval; }
    H_ZLEN '=' H_FLOAT { h.length = yylval.floatval; }
    ')'
  ;

useg_impedance
  : H_Z '=' H_FLOAT    { h.impedance = yylval.floatval; h.impedance_set = pcb_true; }
    H_D '=' H_FLOAT    { h.delay = yylval.floatval; }
    useg_resistance;

useg_resistance 
  : H_R '=' H_FLOAT    { h.resistance = yylval.floatval; h.resistance_set = pcb_true;}
    ')'
  | ')'
  ;

polygon
  : '{' H_POLYGON { new_record(); } polygon_param_list coord_point { if (exec_polygon_begin(&h)) YYERROR; }
    lines_and_curves '}' { if (exec_polygon_end(&h)) YYERROR; } ;

polygon_param_list
  : polygon_param_list polygon_param
  | polygon_param
  ;

polygon_param
  : layer_name
  | width
  | polygon_type
  | polygon_id
  ;

polygon_id
  : H_ID '=' H_POSINT { h.id = yylval.intval; h.id_set = pcb_true; } /* polygon id is a non-negative integer */
  ;

polygon_type
  : H_T '=' H_POUR    { h.polygon_type = POLYGON_TYPE_POUR; h.polygon_type_set = pcb_true; }
  | H_T '=' H_PLANE   { h.polygon_type = POLYGON_TYPE_PLANE; h.polygon_type_set = pcb_true; }
  | H_T '=' H_COPPER  { h.polygon_type = POLYGON_TYPE_COPPER; h.polygon_type_set = pcb_true; }
  ;

polyvoid
  : '{' H_POLYVOID { new_record(); } polygon_id coord_point { if (exec_polyvoid_begin(&h)) YYERROR; }
    lines_and_curves '}' { if (exec_polyvoid_end(&h)) YYERROR; } ;

polyline
  : '{' H_POLYLINE { new_record(); } polygon_param_list coord_point { if (exec_polyline_begin(&h)) YYERROR; }
    lines_and_curves '}' { if (exec_polyline_end(&h)) YYERROR; } ;

lines_and_curves
  : lines_and_curves line_or_curve
  | line_or_curve
  ;

line_or_curve
  : line
  | curve
  | '(' error ')'
  ;

line
  : '(' H_LINE { new_record(); } coord_point ')' { if (exec_line(&h)) YYERROR; } ;

curve
  : '(' H_CURVE { new_record(); } coord_arc ')' { if (exec_curve(&h)) YYERROR; } ;

net_attribute
  : '(' H_A H_N '=' H_STRING { h.name = yylval.strval; } H_V '=' H_STRING { h.value = yylval.strval; } ')' { if (exec_net_attribute(&h)) YYERROR; } ;

  /* net class */

netclass
  : '{' H_NET_CLASS '=' H_STRING { h.net_class_name = yylval.strval; if (exec_net_class(&h)) YYERROR; } netclass_subrecords ;

netclass_subrecords
  : netclass_paramlist '}'
  | '}' 
  ;

netclass_paramlist
  : netclass_paramlist netclass_param
  | netclass_param 
  ;

netclass_param
  : netclass_attribute 
  | net_name 
  | '(' error ')'
  ;

net_name
  : '(' H_N H_N '=' H_STRING { h.net_name = yylval.strval; } ')' { if (exec_net_class_element(&h)) YYERROR; } ;

netclass_attribute
  : '(' H_A H_N '=' H_STRING { h.name = yylval.strval; } H_V '=' H_STRING { h.value = yylval.strval; } ')' { if (exec_net_class_attribute(&h)) YYERROR; } ;

  /* end */

end
  : '{' H_END '}' { if (exec_end(&h)) YYERROR; } ;

  /* key */

key
  : '{' H_KEY '=' H_STRING { h.key = yylval.strval; } '}' { if (exec_key(&h)) YYERROR; } ;

  /* coordinates */

coord_point
  : H_X '=' H_FLOAT { h.x = yylval.floatval; } H_Y '=' H_FLOAT { h.y = yylval.floatval; } ;

coord_point1
  : H_X1 '=' H_FLOAT { h.x1 = yylval.floatval; } H_Y1 '=' H_FLOAT { h.y1 = yylval.floatval; } ;

coord_point2
  : H_X2 '=' H_FLOAT { h.x2 = yylval.floatval; } H_Y2 '=' H_FLOAT { h.y2 = yylval.floatval; } ;

coord_line
  : coord_point1 coord_point2 ;

coord_arc
  : coord_line H_XC '=' H_FLOAT { h.xc = yylval.floatval; } H_YC '=' H_FLOAT { h.yc = yylval.floatval; } H_R '=' H_FLOAT { h.r = yylval.floatval; } ;

%%

/*
 * Supporting C routines
 */

void hyyerror(const char *msg)
{
  /* log pcb-rnd message */
  hyp_error(msg);
}

void hyyprint(FILE *file, int type, YYSTYPE value)
{
  if (type == H_STRING)
    fprintf (file, "%s", value.strval);
   else if (type == H_FLOAT)
    fprintf (file, "%g", value.floatval);
   else if (type == H_BOOL)
    fprintf (file, "%i", value.boolval);
  return;
}

/*
 * reset parse_param struct at beginning of record
 */

void new_record()
{
  h.vers = 0;
  h.detailed = pcb_false;
  h.unit_system_english = pcb_false;
  h.metal_thickness_weight = pcb_false;
  h.default_plane_separation = 0;
  h.use_die_for_metal = pcb_false;
  h.bulk_resistivity = 0;
  h.conformal = pcb_false;
  h.epsilon_r = 0;
  h.layer_name = NULL;/* XXX */
  h.loss_tangent = 0;
  h.material_name = NULL;/* XXX */
  h.plane_separation = 0;
  h.plating_thickness = 0;
  h.prepreg = pcb_false;
  h.temperature_coefficient = 0;
  h.thickness = 0;
  h.bulk_resistivity_set = pcb_false;
  h.conformal_set = pcb_false;
  h.epsilon_r_set = pcb_false;
  h.layer_name_set = pcb_false;
  h.loss_tangent_set = pcb_false;
  h.material_name_set = pcb_false;
  h.plane_separation_set = pcb_false;
  h.plating_thickness_set = pcb_false;
  h.prepreg_set = pcb_false;
  h.temperature_coefficient_set = pcb_false;
  h.thickness_set = pcb_false;
  h.device_type = NULL;/* XXX */
  h.ref = NULL;/* XXX */
  h.value_float = 0;
  h.value_string = NULL;/* XXX */
  h.package = NULL;/* XXX */
  h.name_set = pcb_false;
  h.value_float_set = pcb_false;
  h.value_string_set = pcb_false;
  h.package_set = pcb_false;
  h.voltage_specified = pcb_false;
  h.conversion = pcb_false;
  h.padstack_name = NULL;/* XXX */
  h.drill_size = 0;
  h.pad_shape = 0;
  h.pad_sx = 0;
  h.pad_sy = 0;
  h.pad_angle = 0;
  h.thermal_clear_shape = 0;
  h.thermal_clear_sx = 0;
  h.thermal_clear_sy = 0;
  h.thermal_clear_angle = 0;
  h.pad_type = PAD_TYPE_METAL;
  h.padstack_name_set = pcb_false;
  h.drill_size_set = pcb_false;
  h.pad_type_set = pcb_false;
  h.width = 0;
  h.left_plane_separation = 0;
  h.width_set = pcb_false;
  h.left_plane_separation_set = pcb_false;
  h.layer1_name = NULL;/* XXX */
  h.layer1_name_set = pcb_false;
  h.layer2_name = NULL;/* XXX */
  h.layer2_name_set = pcb_false;
  h.pad1_shape = NULL;/* XXX */
  h.pad1_sx = 0;
  h.pad1_sy = 0;
  h.pad1_angle = 0;
  h.pad2_shape = NULL;/* XXX */
  h.pad2_sx = 0;
  h.pad2_sy = 0;
  h.pad2_angle = 0;
  h.pin_reference = NULL;/* XXX */
  h.pin_reference_set = pcb_false;
  h.pin_function = PIN_SIM_BOTH;
  h.pin_function_set = pcb_false;
  h.zlayer_name = NULL;/* XXX */
  h.zlayer_name_set = pcb_false;
  h.length = 0;
  h.impedance = 0;
  h.impedance_set = pcb_false;
  h.delay = 0;
  h.resistance = 0;
  h.resistance_set = pcb_false;
  h.id = -1;
  h.id_set = pcb_false;
  h.polygon_type = POLYGON_TYPE_PLANE;
  h.polygon_type_set = pcb_false;
  h.net_class_name = NULL;/* XXX */
  h.net_name = NULL;/* XXX */
  h.key = NULL;/* XXX */
  h.name = NULL;/* XXX */
  h.value = NULL;/* XXX */
  h.x = 0;
  h.y = 0;
  h.x1 = 0;
  h.y1 = 0;
  h.x2 = 0;
  h.y2 = 0;
  h.xc = 0;
  h.yc = 0;
  h.r = 0;

  return;
}

/* not truncated */

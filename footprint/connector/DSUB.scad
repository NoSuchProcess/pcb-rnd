// Model for generic DSUB PCB mounted through hole connectors
//  openscad = DSUB.scad
//  openscad-param = {pins=9,gender=0,rotation=0}
//
// Copyright (C) 2023 Erich Heinzle
//
// File distribution license:
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// The above distribution license applies when the file is distributed as a
// stand alone model file or as part of a library, in both cases intended
// for reuse combined by the user with other openscad scripts.
//
// Use license:
//  as a special exception, the content of the file may be
//  embedded in an openscad script that represents a printed circuit board,
//  for example when a board is exported by pcb-rnd. In such use case,
//  the content of this file may be copied into the resulting board file
//  with or without modifications, without affecting the board file's license
//  in any way.
//

module dsub(pins=9,gender=1,rotation=90,pin_descent=2.5)
{

//    body_width  = 1.378603*pins + 18.48503;
//    hex-socket-spacing = 1.375055*pins + 12.66131;
//    male_shroud_width = 1.374157*pins + 4.593115;
//    female_shroud_width = 1.374579*pins + 3.999058;

    body_width  = 1.379*pins + 18.485;
    hex_socket_spacing = 1.375*pins + 12.661;
    male_shroud_width = 1.374*pins + 4.593;
    female_shroud_width = 1.375*pins + 3.999;
    m_radius = 2.5908;
    f_radius = 2.2860;
    
    shroud_depth = 6;
    shroud_opening_height_male = 8.36;
    plug_height_female = 7.9;
    delta_m = tan(10)*(shroud_opening_height_male-2*m_radius);
    delta_f = tan(10)*(plug_height_female-2*f_radius);
    
    overall_depth = 18.4;
    face_height = 12.5;
    base_face_thickness = 2.5;
    rounding = 1;
    
    pin_diameter = 0.7;

    mounting_hole_dia = 3.3;
    mounting_hole_setback = 9.52;
    mounting_pin_thickness = 0.7;
    mounting_pin_descent=3.8;

    pin_row_spacing = 2.84;
    pin_spacing = 2.7432;
    pin_setback1 = 8.08;
    pin_setback2 = pin_setback1 + pin_row_spacing;
 
    module pin() {
        color([0.7,0.7,0.7])
            translate([0,0,-pin_descent])
                cylinder(r=pin_diameter/2, h=pin_descent);
    }
    
    module board_lock_pin() {
        translate([0,mounting_pin_thickness/2,0])
            rotate([90,0,0])
                linear_extrude(height=mounting_pin_thickness)
                    polygon([[-mounting_hole_dia/2,base_face_thickness],[-mounting_hole_dia/2,-1.6],[-mounting_pin_descent/2,-mounting_pin_descent/2],[-0.5,-mounting_pin_descent],[-0.5,0],[0.5,0],[0.5,-mounting_pin_descent,],[mounting_pin_descent/2,-mounting_pin_descent/2],[mounting_hole_dia/2,-1.6],[mounting_hole_dia/2,base_face_thickness]]);
    }

    module raw_F_plug() {
        // origin centred plug
        translate([-female_shroud_width/2,-plug_height_female/2,0])
        union() {
            linear_extrude(height=shroud_depth)
                polygon([[f_radius,plug_height_female],
[female_shroud_width-f_radius,plug_height_female],
[female_shroud_width,plug_height_female-f_radius],//approx
[female_shroud_width-delta_f,f_radius],
[female_shroud_width-delta_f-f_radius,0],
[delta_f+f_radius,0],
[delta_f,f_radius],
[0,plug_height_female-f_radius]]);
            translate([f_radius,plug_height_female-f_radius,0])
                cylinder(r=f_radius,h=shroud_depth);
            translate([f_radius+delta_f,f_radius,0])
                cylinder(r=f_radius,h=shroud_depth);
            translate([female_shroud_width-delta_f-f_radius,f_radius,0])
                cylinder(r=f_radius,h=shroud_depth);
            translate([female_shroud_width-f_radius,plug_height_female-f_radius,0])
                cylinder(r=f_radius,h=shroud_depth);
        }
    }

    module F_plug() {
        // face plate cenntred plug
        translate([body_width/2,face_height/2,base_face_thickness])
        raw_F_plug();
    }

    module M_plug() {
        translate([body_width/2,face_height/2,base_face_thickness])
            difference(){
                scale([(male_shroud_width-2*(shroud_opening_height_male-plug_height_female))/female_shroud_width,    shroud_opening_height_male/plug_height_female,1])
                        raw_F_plug();
                scale([0.95,0.95,1.1])
                        raw_F_plug();
            }
    }
    
    module face_pin() {
        color([0.7,0.7,0.7])
            translate([body_width/2,face_height/2,base_face_thickness])
                rotate([0,0,90])
                    cylinder(r=pin_diameter/2, h=shroud_depth+0.1);
    }

    module place_face_pins(){
        //bottom row
        bottom_row_num = floor(pins/2);
        top_row_num = pins - bottom_row_num;
        for (i = [0:(bottom_row_num-1)]) {
            translate([(i-(bottom_row_num-1)/2)*pin_spacing,-pin_row_spacing/2,0])
                face_pin();
        }
        // top row
        for (i = [0:top_row_num-1])
            translate([(i-(top_row_num-1)/2)*pin_spacing,pin_row_spacing/2,0])
                face_pin();
    }
    
    module connector_body() {
       //connector body proper
       color([0.3,0.3,0.3]) {
            translate([-body_width/2,0,0])
                difference() {
                    //base plate
                    union() {
                        linear_extrude(height = base_face_thickness)
                            polygon([[0,0],[body_width,0],
                                [body_width,overall_depth-rounding],
                                [body_width-rounding,overall_depth],
                                [rounding,overall_depth],[0,overall_depth-rounding]]);
                        translate([rounding,overall_depth-rounding,0])
                            cylinder(r=rounding, h=base_face_thickness);
                        translate([body_width-rounding,overall_depth-rounding,0])
                            cylinder(r=rounding, h=base_face_thickness);
                    }
                    // mounting holes
                    translate([body_width/2-hex_socket_spacing/2,mounting_hole_setback,-base_face_thickness])
                        cylinder(r=mounting_hole_dia /2, h=base_face_thickness*3);
                    translate([body_width/2+hex_socket_spacing /2,mounting_hole_setback ,-base_face_thickness])
                        cylinder(r=mounting_hole_dia/2, h=base_face_thickness*3);
                    }
                // pin enclosure
                translate([0,overall_depth/2,(shroud_opening_height_male+base_face_thickness)/2])
                    cube([-body_width+hex_socket_spacing*2,overall_depth,shroud_opening_height_male+base_face_thickness],true);
            }
            // front face
                translate([-body_width/2,base_face_thickness,0])
                    rotate([90,0,0])
                        union() {
                            color([0.3,0.3,0.3]) {
                                linear_extrude(height = base_face_thickness)
                                    polygon([[0,0],[body_width,0],
                                        [body_width,face_height-rounding],
                                        [body_width-rounding,face_height],
                                        [rounding,face_height],[0,face_height-rounding]]);
                                    translate([rounding,face_height-rounding,0])
                                        cylinder(r=rounding, h=base_face_thickness);
                                    translate([body_width-rounding,face_height-rounding,0])
                                        cylinder(r=rounding, h=base_face_thickness);
                                if (gender == 0) {
                                    M_plug();
                                } else {
                                    F_plug();
                                }
                            }
                            place_face_pins();
                        }                        
                // mounting hole lock pins
                color([0.7,0.7,0.7]) {
                    translate([-hex_socket_spacing/2,mounting_hole_setback,0])
                        board_lock_pin();
                    translate([hex_socket_spacing/2,mounting_hole_setback ,0])
                        board_lock_pin();
                    translate([hex_socket_spacing/2-mounting_hole_dia/2,base_face_thickness,base_face_thickness])
                        cube([mounting_hole_dia,mounting_hole_setback- base_face_thickness+mounting_pin_thickness/2,mounting_pin_thickness], false);
                    translate([-hex_socket_spacing/2-mounting_hole_dia/2,base_face_thickness,base_face_thickness])
                        cube([mounting_hole_dia,mounting_hole_setback- base_face_thickness+mounting_pin_thickness/2,mounting_pin_thickness], false);
                }

            // front face mounting_nuts * 2
            color([0.7,0.7,0.7]) {
                translate([-hex_socket_spacing/2,-base_face_thickness,face_height/2])
                    rotate([-90,0,0])
                        difference() {
                            cylinder(r=2.844, h=4.8, $fn=6);
                            translate([0,0,-0.1])
                                cylinder(r=1.4424, h=5.0);
                        }                        
                translate([hex_socket_spacing/2,-base_face_thickness,face_height/2])
                    rotate([-90,0,0])
                        difference() {
                            cylinder(r=2.844, h=4.8, $fn=6);
                            translate([0,0,-0.1])
                                cylinder(r=1.4424, h=5.0);
                        }
             }         
    }
    
    module place_pcb_pins() {
        //front row
        front_row_num = floor(pins/2);
        back_row_num = pins - front_row_num;
        for (i = [0:(front_row_num-1)]) {
            translate([(i-(front_row_num-1)/2)*pin_spacing,pin_setback1,0])
                pin();
        }
        // rear row
        for (i = [0:back_row_num-1])
            translate([(i-(back_row_num-1)/2)*pin_spacing,pin_setback2,0])
                pin();
    }

    rotate([0,0,rotation]) {
        back_row_num = pins - floor(pins/2);
        // legacy footprint origins do not coincide with pin number one exactly in y direction
        fp_offset_y = 0.1524-pin_setback2;
        // pin numbering in M is mirror of F footprint
        fp_offset_x = (1-gender)*(back_row_num-1)/2*pin_spacing + (0-gender)*(back_row_num-1)/2*pin_spacing;

        translate([fp_offset_x,fp_offset_y,0])
            union () {
                connector_body();
                place_pcb_pins();
        }
    }
}

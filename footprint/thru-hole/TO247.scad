// Model for vertical TO247 package 2 and 3 pin variants
//
// Copyright (C) 2017,2020 Tibor 'Igor2' Palinkas
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

module to247(pins=3,pin_descent=2.5)
{
    pin_thickness = 0.6;
    pin_width1 = 2.0;
    pin_width2 = 1.2;
    pin_neck = 4.2;
    pin_spacing = 5.56;

    tab_thickness = 0.5;
    tab_width = 13.9;
    tab_height = 17.3;
    tab_overhang = -1.2;
    tab_bevel = 2.0;
    
    device_height = 21;
    device_width = 16;
    body_thickness = 5;
    body_offset = body_thickness/2;

    hole_dia = 3.6;
    hole_height = 6.1;
    notch_size = 5.0;
    notch_height = 5.6;

    module pin() {
        linear_extrude(height=pin_thickness)
            polygon([[-pin_width1/2,0],[pin_width1/2,0],[pin_width1/2,pin_width1-pin_width2-pin_neck],[pin_width2/2,-pin_neck],[pin_width2/2,-pin_neck-0.01-pin_descent ],[-pin_width2/2,-pin_neck-0.01-pin_descent],[-pin_width2/2,-pin_neck],[-pin_width1/2,pin_width1-pin_width2-pin_neck]]);
    }

    module body() {
        union() {
            color([0.3,0.3,0.3]) {
                difference() {
                    translate([0,hole_height,-body_offset])
                        linear_extrude(height=body_thickness)
                            polygon([[-device_width/2,-device_height],[device_width/2,-device_height],[device_width/2,0],[-device_width/2,0]]);
                    translate([0,0,-body_thickness])
                        cylinder(r=hole_dia/2, h=body_thickness*3);
                    translate([-device_width/2,hole_height-notch_height,body_thickness/2])
                        cube(notch_size, true);
                    translate([device_width/2,hole_height-notch_height,body_thickness/2])
                        cube(notch_size, true);
                }
            }
            color([0.8,0.8,0.8])
                translate([0,hole_height+tab_overhang,-body_thickness/2-0.01])
                    difference() {
                        linear_extrude(height=tab_thickness)
                            polygon([[-tab_width/2,-tab_height],[tab_width/2,-tab_height],[tab_width/2,-tab_bevel],[tab_width/2-tab_bevel,0],[-tab_width/2+tab_bevel,0],[-tab_width/2,-tab_bevel]]);
                    translate([0,-hole_height-tab_overhang,-body_thickness])
                        cylinder(r=hole_dia, h=body_thickness*3);
                    }
        }
    }

    translate([-(pins-2)*3.556,(3-pins)*3.556,0]) {
        rotate([90,0,(pins-2)*90]) {
            translate([pin_spacing,0,0]) {
                union() {
                    color([0.9, 0.9, 0.9]) {
                        if (pins==3) {
                            translate([0,pin_neck+0.05,-pin_thickness/2])
                                pin();
                        }
                        translate([-pin_spacing,pin_neck+0.05,-pin_thickness/2])
                            pin();
                        translate([pin_spacing,pin_neck+0.05,-pin_thickness/2])
                            pin();
                    }
                    translate([0,pin_neck+device_height-hole_height,0])
                        body();
                }
            }
        }
    }
}

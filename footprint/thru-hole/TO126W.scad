// Model for vertical TO126 package and S offset middle pin variant
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

module part_to126w(pins=3, S=0, pin_descent=2.5)
{
    pin_thickness = 0.5;
    pin_width1 = 1.2;
    pin_width2 = 0.8;
    pin_neck = 2.1;
    pin_spacing = 2.28;

    tab_thickness = 1.3;
    tab_width = 5.2;
    tab_height = 8;
    tab_r = 0.76;
    tab_hole_height = 3.0;

    hole_dia = 3.4;
    hole_height = 3.6;
    notch_width = 0.8;
    
    device_height = 11.2;
    device_width = 8.2;
    body_thickness = 2.8;
    body_offset = body_thickness/2;
    
    module pin(S=0) {
        pin_descent2 = (1-S)*pin_descent + S*1.717;
        rotate([S*(-45),0,0]) {
            union() {
                linear_extrude(height=pin_thickness)
                    polygon([[-pin_width1/2,0],[pin_width1/2,0],[pin_width1/2,pin_width1-pin_width2-pin_neck],[pin_width2/2,-pin_neck],[pin_width2/2,-pin_neck-0.01-pin_descent2],[-pin_width2/2,-pin_neck-0.01-pin_descent2],[-pin_width2/2,-pin_neck],[-pin_width1/2,pin_width1-pin_width2-pin_neck]]);
            }
        }
        if (S==1) {
            translate([0,-2.1,2.56]) {
                linear_extrude(height=pin_thickness)
                    polygon([[-pin_width2/2,-0.253],[pin_width2/2,-0.253],[pin_width2/2,-2.5],[-pin_width2/2,-2.5]]);
            }
        }
    }

   module body() {
        union() {
            color([0.3,0.3,0.3])
                difference() {
                    translate([0,hole_height,-body_offset])
                        linear_extrude(height=body_thickness)
                            polygon([[-device_width/2,-device_height],[device_width/2,-device_height],[device_width/2,0],[-device_width/2,0]]);
            translate([0,0,-body_thickness])
                cylinder(r=hole_dia/2, h=body_thickness*3);
            rotate([0,0,120])
                translate([-notch_width/2,hole_dia/2-0.1,body_thickness/2-notch_width+0.01])
                    cube(notch_width, false);
            rotate([0,0,-120])
                translate([-notch_width/2,hole_dia/2-0.1,body_thickness/2-notch_width+0.01])
                    cube(notch_width, false);
            translate([-notch_width/2,hole_dia/2-0.1,body_thickness/2-notch_width+0.01])
                cube(notch_width, false);
            }
            difference() {
                color([0.8,0.8,0.8])
                    translate([0,tab_hole_height,-body_thickness/2-0.01])
                        linear_extrude(height=body_thickness/2)
                            polygon([[-tab_width/2,-tab_height],[tab_width/2,-tab_height],[tab_width/2,0],[-tab_width/2,0]]);
                translate([0,0,-body_thickness])
                    cylinder(r=hole_dia/2+0.05, h=body_thickness*3);
            }
        }
    }

    translate([S*2.56,(1-S)*2.56,S*0.2]) {
        rotate([90,0,(1-S)*90]) {
            union() {
                color([0.9, 0.9, 0.9]) {
                    if (pins==3) {
                        translate([0,pin_neck+0.05,-pin_thickness/2])
                            pin(S);
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

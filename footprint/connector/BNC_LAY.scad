// Model for generic BNC connector through hole package
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

module bnc_lay()
{
    body_length = 12.45;
    body_width = 14.50;
    body_height = 14.84;
    body_offset = 0.78;
    socket_centre_height = 7.42 + body_offset;
    pin_diameter = 0.74;
    pin_spacing = 2.54;
    mounting_pin_dia = 2.0;
    mounting_pin_spacing = 10.16;
    mounting_pin_setback = 7.37;
    pin_setback = body_length-1.35;
    pin_descent = 2.5;
    diameter = 3.0;
    height = 1.65*diameter + 0.35;
 
    module pin() {
        pin_straight = pin_descent+body_offset+socket_centre_height-pin_diameter*2;
        translate([0,0,pin_straight-pin_descent])
            color([0.7,0.7,0.7])
                rotate([0,180,0])
                    union() {
                    cylinder(r=pin_diameter/2, h=pin_straight);
                    translate([-3*pin_diameter/2,0,0])
                        rotate([90,0,0])
                            rotate_extrude(angle=-90, convexity=10)
                                translate([3*pin_diameter/2,0,0])
                                    circle(r=pin_diameter/2);
                }
    }
    
    module body() {
        union() {
                translate([body_length/2 - (body_length-pin_setback),0,body_height/2+body_offset])
                    union() {
                        color([0.3,0.3,0.3]) {
                            translate([-body_length/2+1,-body_width/2+1,-body_height/2-body_offset])
                                cylinder(r=1, h = 1);
                            translate([body_length/2-1,body_width/2-1,-body_height/2-body_offset])
                                cylinder(r=1, h = 1);
                            translate([body_length/2-1,-body_width/2+1,-body_height/2-body_offset])
                                cylinder(r=1, h = 1);
                            translate([-body_length/2+1,body_width/2-1,-body_height/2-body_offset])
                                cylinder(r=1, h = 1);
                        }
                       color([0.8,0.8,0.8]) {
                            difference() {
                                translate([-2,0,0])
                                    rotate([0,90,0])
                                        cylinder(r=2.36, h = 26.5);
                                translate([-2,0,0])
                                    rotate([0,90,0])
                                        cylinder(r=0.66, h = 27);
                            }
                            difference() {
                                union() {
                                    translate([24.5,-5.55,0])
                                        rotate([0,90,90])
                                            cylinder(r=1, h = 11.1);
                                    translate([-2.0,0,0])
                                        rotate([0,90,0])
                                            cylinder(r=4.85, h = 30.35);
                                    translate([-body_length/2+4,0,0.4])
                                        rotate([0,-90,0])
                                            cylinder(r=1.0, h = 2.0);
                                }
                                translate([-1,0,0])
                                    rotate([0,90,0])
                                        cylinder(r=4.4, h = 30);
                            }
                        }
                        color([1,1,1]) {
                            translate([-body_length/2+4,0,0.4])
                                rotate([0,-90,0])
                                    cylinder(r=3.5, h = 1.6);
                        }
                        color([0.3,0.3,0.3]) {
                            translate([-3.35,0,0])
                                rotate([0,90,0])
                                    cylinder(r=5.94, h = 18.35);
                            difference () {
                                cube([body_length,body_width,body_height],true);
                                translate([-body_length/2,0,0])
                                    cube([5.5,15,5.5],true);
                                translate([-body_length/2,0,-5])
                                    cube([5.5,6.5,15],true);
                                translate([-body_length/2+4,0,0])
                                    rotate([0,-90,0])
                                        cylinder(r=4, h = 5);                                 
                            }
                    }
            }
            color([0.7,0.7,0.7]) {
                translate([mounting_pin_setback-2.27, mounting_pin_spacing/2,-pin_descent])
                    cylinder(r=mounting_pin_dia/2, h=pin_descent+body_offset);
                translate([mounting_pin_setback-2.27, -mounting_pin_spacing/2,-pin_descent])
                    cylinder(r=mounting_pin_dia/2, h=pin_descent+body_offset);
            }
        }
    }
    
    rotate([0,0,90])
        union () {
            translate ([0,pin_spacing,0])
                pin();
            pin();
            body();
        }
}

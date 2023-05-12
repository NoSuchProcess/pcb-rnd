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

module part_bnc_lay(pin_descent = 2.5)
{
    overall_length = 34.7;
    body_length = 13.97;
    body_width = 14.50;
    body_height = 14.84;
    threaded_collar = 8.9;
    connector_length = 20.9;
    body_offset = 0.78;
    socket_centre_height = 7.42 + body_offset;
    pin_diameter = 0.74;
    pin_spacing = 2.54;
    mounting_pin_dia = 2.0;
    mounting_pin_spacing = 10.16;
    mounting_pin_setback = 7.37;
    pin_setback = body_length-1.35;
 
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
    
    module connector_body() {
       //connector body proper
       color([0.8,0.8,0.8]) {
            // outer tube
            difference() {
                union() {
                    // outer metal connector proper
                    translate([0,0,0])
                        cylinder(r=4.85, h = connector_length);
                    // side lugs x2
                    translate([-5.55,0,connector_length -4.25])
                        rotate([0,90,0])
                            cylinder(r=1, h = 1);
                    translate([4.55,0,connector_length -4.25])
                        rotate([0,90,0])
                            cylinder(r=1, h = 1);
                }
                // inner major cavity
                translate([0,0,0])
                        cylinder(r=4.4, h = 85);
            }
            // inner tube and pin receptacle
            difference() {
                    cylinder(r=2.36, h = connector_length - 3.6);
                    cylinder(r=0.66, h = connector_length);
            }
        }
    }
    
    module plastic_body() {
                        // centre pin
                        color([0.8,0.8, 0.8])
                            translate([-body_length/2+4,0,0.4])
                                rotate([0,-90,0])
                                    cylinder(r=1.0, h = 2.0);
                        // insulating cylinder
                        color([1,1,1])
                            translate([-body_length/2+4,0,0.4])
                                rotate([0,-90,0])
                                    cylinder(r=3.5, h = 1.6);
                        // basic feet, cubical shape and threaded section
                        color([0.3,0.3,0.3]) {
                            difference () {
                                union() {
                                    // four standoffs
                                    translate([-body_length/2+1,-body_width/2+1,-body_height/2-body_offset])
                                        cylinder(r=1, h = 1);
                                    translate([body_length/2-1,body_width/2-1,-body_height/2-body_offset])
                                        cylinder(r=1, h = 1);
                                    translate([body_length/2-1,-body_width/2+1,-body_height/2-body_offset])
                                        cylinder(r=1, h = 1);
                                    translate([-body_length/2+1,body_width/2-1,-body_height/2-body_offset])
                                        cylinder(r=1, h = 1);
                                    // threaded collar
                                    translate([-body_length/2,0,0])
                                        rotate([0,90,0])
                                            cylinder(r=5.94, h = threaded_collar + body_length);
                                    // body
                                    cube([body_length,body_width,body_height],true);
                                }
                                // apperture at the back of the body for pins
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
    module mounting_pins() {
        // mechanical mounting pins x2     
         color([0.7,0.7,0.7]) {
            translate([mounting_pin_setback-2.27, mounting_pin_spacing/2,-pin_descent])
                cylinder(r=mounting_pin_dia/2, h=pin_descent+body_offset);
            translate([mounting_pin_setback-2.27, -mounting_pin_spacing/2,-pin_descent])
                cylinder(r=mounting_pin_dia/2, h=pin_descent+body_offset);
        }
    }
    
    rotate([0,0,90])
        union () {
            translate([body_length/2 - (body_length-pin_setback),0,body_height/2+body_offset])
                plastic_body();
            translate([overall_length-body_offset-connector_length,0,socket_centre_height])
                rotate([0,90,0])
                    connector_body();
            translate ([0,pin_spacing,0])
                pin();
            pin();
            mounting_pins();
        }
}


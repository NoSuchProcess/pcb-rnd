// Model for generic DC power jack, defaults to 2.1mm pin
// openscad-param= {centre_pin_dia=2.X}
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

module pwrjack(centre_pin_dia = 2.1)
{
    pin_thickness = 0.3;
    pin1_width = 2.8;
    pin23_width = 2.0;
    pin_spacing = 7;
    pin_descent = 2.5;

    body_width = 8.8;
    body_length = 13.2;
    body_height = 10.9;
    body_front_lip = 3.3;
    axis_height = 6.5;
    
    opening = 6.3;
    
    module pin1() {
        cube([pin1_width,pin_thickness,axis_height+pin_descent],false);
    }

    module pin2() {
        cube([pin23_width,pin_thickness,axis_height+pin_descent],false);
    }

    module pin3() {
        cube([pin23_width,pin_thickness,pin_descent+0.05],false);
    }
    
    module body() {
        color([0.3,0.3,0.3])
            difference () {
                union() {
                    translate([0,body_length/2,axis_height/2])
                        cube([body_width,body_length,axis_height], true);
                    translate([0,body_length-body_front_lip/2,body_height/2])
                        cube([body_width,body_front_lip,body_height], true);
                    translate([0,0,axis_height])
                        rotate([-90,0,0])
                            cylinder(r=body_width/2, h=body_length);
                }
                 translate([0,body_front_lip,axis_height])
                     rotate([-90,0,0])
                         cylinder(r=opening /2, h=body_length);            
            }
            color([0.8,0.8,0.8]) {
                translate([0,-pin_thickness,axis_height])
                    rotate([-90,0,0])
                        cylinder(r=centre_pin_dia/2, h=body_length+pin_thickness);
                translate([0,0,axis_height])
                    rotate([90,0,0])
                        cylinder(r=2, h=pin_thickness);
            }
    }

    rotate([0,0,-90])
        union() {
            body();
            color([0.9, 0.9, 0.9]) {
                translate([-pin1_width/2,-pin_thickness+0.01,-pin_descent])
                    pin1();
                translate([-pin23_width/2,pin_spacing,-pin_descent])
                    pin3();
                translate([-body_width/2,pin_spacing/2-pin23_width/2,-pin_descent])
                    rotate([0,0,90])
                        pin2();
            }
        }
}

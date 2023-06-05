// Model for parametric alf() package
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
//  All sizes are in mm

module part_alf(pitch=7.68, pin_descent=2.5, pin_dia=0.3, body_dia=2)
{
    
    pitch_mm = pitch;
    D_width = body_dia;
    D_length = D_width*3;

    module pins(standing=0, link=0) {
        radius=pin_dia;
        component_height=D_width/2;

        color([0.8,0.8,0.8]) {
            //pin 1
            translate([0,0,-pin_descent])
                cylinder(r=pin_dia/2, h=pin_descent+component_height-radius);
            // pin 2
            translate([pitch_mm,0,-pin_descent])
                cylinder(r=pin_dia/2, h=pin_descent+component_height-radius);
            // link with ends bent
            rotate([90,0,0]) {
                translate([radius,component_height,0])
                    rotate([0,90,0])
                        cylinder(r=pin_dia/2,h=pitch_mm-radius*2);
                translate([pitch_mm-radius,component_height-radius,0])
                    rotate_extrude(angle=90, convexity=10)
                        translate([radius,0,0])
                            circle(pin_dia/2);
                translate([radius,component_height-radius,0])
                    rotate_extrude(angle=-90, convexity=10)
                        translate([-radius,0,0])
                            circle(pin_dia/2);
            }
        }
    }
    
    module body(band_end=0) {
        union() {
            color([0.3,0.3,0.3]) {
                translate([-(D_length)/2,0,0])
                    rotate([0,90,0])
                        cylinder(r=D_width/2, h=D_length);
            }
            color([0.9,0.9,0.9]) {
                translate([(band_end-1)*(D_length)/3 + band_end*(D_length)/6,0,0])
                    rotate([0,90,0])
                        cylinder(r=D_width/2+0.01, h=D_length/7);
            }
        }

    }
    
    module aligned_body(band_end) {
        translate([pitch_mm/2,0,D_width/2])
        body(band_end);
    }
    
    aligned_body(band_end=0);
    pins();
    
}


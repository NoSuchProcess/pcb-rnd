// Model for parametric acy() package
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

module part_acy(pitch=7.68, standing=0, link=0, pin_descent=2.5, pin_dia=0.3)
{
    
    pitch_mm = pitch;
    R_width = 2.3;
    R_length = 5.5;
    R_neck = 1.9;
    body_ell = 1.2;

    module pins(standing=0, link=0) {
        radius=pin_dia;
        component_height=(1-link)*((standing*(R_length+(body_ell-1)*R_width+pin_dia))+(1-standing)*R_width/2)+link*pin_dia/2;

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
    
    module body() {
        color([0.5,0.5,0.8]) {
            translate([-(R_length-R_width)/2,0,0])
                scale([body_ell,1,1])
                    sphere(R_width/2);
            translate([(R_length-R_width)/2,0,0])
                scale([body_ell,1,1])
                    sphere(R_width/2);
            translate([-(R_length-R_width)/2,0,0])
                rotate([0,90,0])
                    cylinder(r=R_neck/2, h=R_length-R_width);
        }
    }
    
    module aligned_body(standing=0, link=0) {
        if (!link) {
            if (standing) {
                translate([0,0,(R_length/2)+(body_ell-1)/2*R_width])
                    rotate([0,90,0])
                        body();
            } else {
                translate([pitch_mm/2,0,R_width/2])
                    body();
            }
        }
    }
    
    aligned_body(standing,link);
    pins(standing, link);
    
}

part_acy(standing=1,pitch=7.68, link=0);

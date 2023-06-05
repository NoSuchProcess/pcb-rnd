// Model for parametric rcy() package
// provides elco, LDR, piezo, vertical ferrite bead, trimmer cap, and coil models
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

module part_rcy(pitch=1.27, body=0, body_dia=5, body_len=8, pin_dia=0.3, pin_descent=2.5)
{

    pitch_mm = pitch;
    C_width = body_dia;
    C_length = body_len;

    // r[adius], h[eight], [rou]n[d]
    module rounded_cylinder(r,h,n) {
        rotate_extrude(convexity=1) {
            offset(r=n) offset(delta=-n) square([r,h]);
                square([n,h]);
        }
    }

    module pins() {
        color([0.8,0.8,0.8]) {
            //pin 1
            translate([0,0,-pin_descent])
                cylinder(r=pin_dia/2, h=pin_descent);
            // pin 2
            translate([pitch_mm,0,-pin_descent])
                cylinder(r=pin_dia/2, h=pin_descent);
        }
    }

    module body() {
        translate([pitch_mm/2,0,0])
            if (body == 0) {
                color([0.3,0.3,0.8]) { // elco
                    union () {
                        translate([0,0,1.2*C_length/6])
                            rounded_cylinder(r=C_width/2, h=4.8*C_length/6, n=0.5);
                        rounded_cylinder(r=C_width/2, h=C_length/6, n=0.5);
                        cylinder(r=C_width/2-0.3, h=C_length);
                    }
                }
            } else if (body == 1) { // coil
                union() {
                    color([0.6,0.6,0.6])
                        cylinder(r=C_width/3, h=C_length);
                    color([0.6,0.6,0.6])
                        cylinder(r=C_width/2,h=C_length/6);    
                    color([0.4,0.4,0.4])
                        translate([0,0,C_length/4])
                            cylinder(r=C_width/2, h=C_length/2);
                }
            } else if (body == 2) { // piezo
                color([0.3,0.3,0.3])
                    difference() {
                        rounded_cylinder(r=C_width/2, h=C_length, n=0.5);
                        translate([0,0,3*C_length/4])
                            cylinder(r=C_width/4, h=C_length/2);
                    }
            } else if (body == 3) { // LDR
                color([0.9,0.6,0.2])
                    intersection() {
                        cylinder(r=C_width/2,h=2.4);
                        cube([C_width*2,C_width*0.85,5],true);
                    }
            } else if (body == 4) { // trimmer capacitor
                union() {
                    color([0.4,0.4,0.7])
                        intersection() {
                            cylinder(r=C_width/2,h=2.4);
                            cube([C_width*2,C_width*0.85,5],true);
                        }
                    color([0.6,0.6,0.2])
                        difference() {
                            cylinder(r=1,h=3);
                            translate([0,0,2.8])
                                cube([3,0.3,0.6],true);
                        }
                    }
            } else if (body == 5) { // ferrite bead
                color([0.3,0.3,0.3])
                    rounded_cylinder(r=C_width/2, h=C_length, n=0.5);
            }
    }

    body();
    pins();

}

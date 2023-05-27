// Model for parametric dip socket
// openscad-param= {pins=8,spacing=7.62,pitch=2.54,pin_descent=2.5, pin_dia=0.6,modules=1}
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

module dip(pins=8,spacing=7.62,pitch=2.54,pin_descent=2.5, pin_dia=0.6, modules=1)
{
    pin_thickness = 0.3;
    row_spacing = spacing;
    pin_spacing = pitch;
    pcb_pin_width = pin_dia;
    standard_pitch = 2.54;
    pcb_offset = 0.31;

    end_height = 3.77;
    notch = 0.75;   // notch radius

    ic_overhang = 0.825;
    ic_length = ((pins/2)-1)*pitch+ic_overhang *2;
    ic_pin_thickness = 0.25;

    module pin() {
        rotate([-90,0,0])
            color([0.8,0.8,0.8])
                union() {
                    // folded socket portion
                    translate([0,0,-pin_spacing/4+0.1])
                        scale([0.93,1,1])
                            linear_extrude(height = pin_spacing/2-0.2)
                                polygon([[0.0,0.1],[-0.08,0.1],[-0.08,-0.68],[-0.48,-0.68],[-0.48,-3.04],[-0.70,-4.62],[-0.58,-4.63],[-0.34,-2.95],[-0.34,-0.82],[1.14,-0.82],[1.14,-3.66],[1.03,-3.73],[-0.18,-2.81],[-0.18,-2.60],[-0.05,-2.36],[-0.16,-2.29],[-0.30,-2.60],[-0.30,-2.86],[1.03,-3.87],[1.10,-3.87],[1.18,-3.83],[1.24,-3.74],[1.24,-0.68],[0.08,-0.68],[0.08,0.1]]);
                    // actual PCB pin
                    translate([0,0,-pcb_pin_width/2])
                        linear_extrude(height = pcb_pin_width)
                            polygon([[pin_thickness/2,0.1],[pin_thickness/2,pin_descent-pin_thickness],[0,pin_descent+pcb_offset],[-pin_thickness/2,pin_descent-pin_thickness],[-pin_thickness/2,0.1]]);
                }
    }

    module housing() {
        linear_extrude(height = pins/2*pin_spacing)
            polygon([[-0.98,-4.69],[-0.66,-4.69],[-0.40,-2.84],[-0.40,-0.46],[1.21,-0.46],[1.21,-3.77],[2.42,-3.77],[2.42,-0.02],[1.24,-0.02],[1.24,0.31],[0.32,0.31],[0.32,0.0],[-0.98,0.0]]);
    }

    module housingL() {
        rotate([-90,0,0])
            housing();
    }

    module housingR() {
        translate([0,pins/2*pin_spacing])
            rotate([90,0,0])
                rotate([0,0,180])
                    housing();
    }

    module frame_end() {
        linear_extrude(height = end_height)
            polygon([
        [-0.98-row_spacing/2,-pin_spacing/4],
        [-0.98-row_spacing/2,-pin_spacing/2],
        [0.98+row_spacing/2,-pin_spacing/2],
        [0.98+row_spacing/2,-pin_spacing/4],
        [(row_spacing-standard_pitch )/2,-pin_spacing/4],
        [(row_spacing-standard_pitch )/2,standard_pitch/2-pin_spacing/2],
        [-(row_spacing-standard_pitch )/2,standard_pitch/2-pin_spacing/2],
        [-(row_spacing-standard_pitch )/2,-pin_spacing/4]
        ]);
    }

    module frame_end_notched() {
           difference() {
               frame_end();
               translate([0,-pin_spacing/2,-end_height/2])
                    cylinder(r=notch, h=end_height*2,$fn =10);
           }
    }

    module frame_midbar() {
        if (pins > 8) {
            translate([0,(pins/2-1)/2*pin_spacing,end_height/2])
                cube([row_spacing-standard_pitch ,standard_pitch/2,end_height],true);
        }
    }

    module pin_spacer() {
            translate([-1.25,pin_spacing/4,0])
                cube([2.5,pin_spacing/2,end_height],false);
    }

    module pin_spacers() {
        for(i = [0:pins/2-2]) {
            translate([-row_spacing/2+0.5,i*pin_spacing,0])
                pin_spacer();
            translate([row_spacing/2-0.5,i*pin_spacing,0])
                pin_spacer();
        }
    }

    module build_frame() {
        color([0.3,0.3,0.3]) {
            union() {
                translate([-row_spacing/2,-pin_spacing/2,0])
                    housingL();
                translate([row_spacing/2,-pin_spacing/2,0])
                    housingR();
                frame_end_notched();
                translate([0,(pins/2-1)*pin_spacing,0])
                    rotate([0,0,-180])
                        frame_end();
                frame_midbar();
                pin_spacers();
            }
        }
    }

    module place_pins() {
        for(i = [0:pins/2-1]) {
            translate([-row_spacing/2,i*pin_spacing,0])
                pin();
            translate([row_spacing/2,i*pin_spacing,0])
                rotate([0,0,180])
                    pin();
        }
    }

    module socket() {
        translate([row_spacing/2,0,pcb_offset]) {
            rotate([0,0,180]) {
                translate([0,0,pcb_offset]) {
                    union () {
                        build_frame();
                        place_pins();
                    }
                }
            }
        }
    }

    module ic_pin() {
        translate([0,ic_pin_thickness/2,0])
            rotate([90,0,0])
                union() {
                    linear_extrude(height=ic_pin_thickness) {
                        polygon([[-0.23,-pin_descent+0.2],[-0.13,-pin_descent],[0.13,-pin_descent],[0.23,-pin_descent+0.2],[0.23,0.0],[0.39,0.0],[0.67,0.45],[0.67,2.2],[-0.23,2.2]]);
                    }
                    translate([0.22,2.2-ic_pin_thickness/2,0.4]) {
                        cube([0.9,ic_pin_thickness,0.8],true);
                    }
                }
    }

    module first_ic_pin() {
        mirror([1,0,0])
            rotate([0,0,90])
                ic_pin();
    }

    module last_ic_pin() {
        rotate([0,0,-90])
            ic_pin();
    }

    module middle_ic_pin() {
        union () {
            first_ic_pin();
            last_ic_pin();
        }
    }

    module place_ic_pins() {
        color([0.8,0.8,0.8]) {
            translate([-row_spacing/2,0,0])
                mirror([1,0,0])
                    first_ic_pin();
            translate([row_spacing/2,0,0])
                first_ic_pin();
            for (i = [1:(pins/2-2)]) {
                translate([-row_spacing/2,i*pitch,0])
                    mirror([1,0,0])
                        middle_ic_pin();
                translate([row_spacing/2,i*pitch,0])
                    middle_ic_pin();
            }
            translate([-row_spacing/2,(pins/2-1)*pitch,0])
                mirror([1,0,0])
                    last_ic_pin();
            translate([row_spacing/2,(pins/2-1)*pitch,0])
                last_ic_pin();
        }
    }

    module ic_body() {
        color([0.2,0.2,0.2]) {
            translate([0,ic_length-ic_overhang,0]) {
                difference() {
                    rotate([90,0,0])
                        linear_extrude(height=ic_length) {
                            polygon([[-row_spacing/2+0.51,1.9],[-row_spacing/2+0.88,0.38],[row_spacing/2-0.88,0.38],[row_spacing/2-0.51,1.9],[row_spacing/2-0.51,2.2],[row_spacing/2-0.88,3.68],[-row_spacing/2+0.88,3.68],[-row_spacing/2+0.51,2.2]]);
                        }
                    translate([0,0,2.3])
                        cylinder(r=pitch/3, h=3);
                }
            }
        }
    }

    module ic() {
        translate([row_spacing/2,-(pins/2-1)*pitch,0]) {
            union () {
                ic_body();
                place_ic_pins();
            }
        }
    }

    module socketed_ic() {
        translate([0,0,end_height+pcb_offset+0.05]) {
            ic();
        }
    }

    if (modules !=2) {
        socket();
    }
    if (modules == 2) {
        ic();
    }
    if (modules == 3) {
        socketed_ic();
    }
}

// Model for TO18 through hole package
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

module part_to18(pin_descent=2.5)
{
    height = 4.83;
    major_diameter = 5.6;
    minor_diameter = 4.7;
    rim_thickness = 0.5;
    tab_square = 1.0;
    pin_diameter = 0.45;
    pcb_offset = 3.0;
    
    // r[adius], h[eight], [rou]n[d]
    module rounded_cylinder(r,h,n) {
        rotate_extrude(convexity=1) {
            offset(r=n) offset(delta=-n) square([r,h]);
                square([n,h]);
        }
    }

    union() {
        color([0.7,0.7,0.7]) {
            translate ([0,0,pcb_offset]) {
                rounded_cylinder(minor_diameter/2,height,0.5);
                rotate([0,0,135])
                    translate([-tab_square/2,0,rim_thickness/2])
                        cube ([major_diameter+tab_square, tab_square, rim_thickness], true);
                cylinder(r = major_diameter/2, h = rim_thickness);
            }
        }
        color([0.8,0.8,0.8]) {
            translate ([-1.27,0,-pin_descent])
                cylinder(r = pin_diameter/2, h = pcb_offset +pin_descent);
            translate ([0,1.27,-pin_descent])
                cylinder(r = pin_diameter/2, h = pcb_offset + pin_descent);
            translate ([1.27,0,-pin_descent])
                cylinder(r = pin_diameter/2, h = pcb_offset + pin_descent);
        }
    }
}


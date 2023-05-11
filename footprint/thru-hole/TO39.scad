// Model for TO39 through hole package
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

module to39(pin_descent=2.5)
{
    height = 6.35;
    major_diameter = 8.95;
    minor_diameter = 8.1;
    rim_thickness = 0.89;
    tab_square = 0.82;
    pin_diameter = 0.41;
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
            translate ([-2.54,0,-pin_descent])
                cylinder(r = pin_diameter/2, h = pcb_offset +pin_descent);
            translate ([0,2.54,-pin_descent])
                cylinder(r = pin_diameter/2, h = pcb_offset + pin_descent);
            translate ([2.54,0,-pin_descent])
                cylinder(r = pin_diameter/2, h = pcb_offset + pin_descent);
        }
    }
}

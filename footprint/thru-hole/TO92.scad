// Model for TO92 through hole package
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

module to92(pin_descent=2.5)
{
    height = 5.0;
    diameter = 5.0;
    pin_thickness = 0.38;
    
    module bent_pin(pin_thickness) {
        translate([0,0,-pin_thickness/2])
              linear_extrude(height=pin_thickness)
                  polygon([[-pin_thickness/2,-pin_descent],[-pin_thickness/2,2.25],[1.27-pin_thickness/2,3.25],[1.27-pin_thickness/2,5.5],[1.27+pin_thickness/2,5.5],[1.27+pin_thickness/2,3.0],[pin_thickness/2,2.0],[pin_thickness/2,-pin_descent]]);
    }

    rotate([0,0,-90]) {
        union() {
            color([0.1,0.1,0.1]) {
                translate ([2.54,0,3*height/4]) {
                    intersection () {
                        translate ([0,-1.0,height/2])
                            cube ([diameter, diameter, height], true);
                        cylinder(r = diameter/2, h = height);
                    }
                }
            }
            color([0.8,0.8,0.8]) {
                rotate([90,0,0])
                    bent_pin(pin_thickness);
                translate ([2.54,0,height-(height+pin_descent)/2])
                    cube ([pin_thickness, pin_thickness, height+pin_descent], true);
                translate ([5.08,0,0])
                    rotate([90,0,180])
                    bent_pin(pin_thickness);
            }
        }
    }
}

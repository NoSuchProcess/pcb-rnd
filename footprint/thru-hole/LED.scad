// Model for round 3mm and 5mm LED through hole package
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

module led(diameter)
{
    height = 1.65*diameter + 0.35;
    union () {
        color([1.0,0.1,0.1]) {
            translate ([0.0,0.0,height-diameter/2])
                sphere(r = diameter/2);
            cylinder(r = diameter/2, h = height-diameter/2);
            intersection () {
                translate ([-0.45,0,0.5])
                    cube ([diameter+0.9, diameter+0.9, 1.0], true);
                cylinder(r = (diameter+0.9)/2, h = 1.0);
            }
        }
        color([0.8,0.8,0.8]) {
            translate ([-1.252,0,-1.2])
                cube ([0.5, 0.5, 2.5], true);
            translate ([1.252,0,-1.2])
                cube ([0.5, 0.5, 2.5], true);
        }
    }
}


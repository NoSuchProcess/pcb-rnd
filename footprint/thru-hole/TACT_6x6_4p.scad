// Model for 6x6mm tactile switch
// openscad parameter button_height defaults to 1mm
//
// Copyright (C) 2017,2020 Tibor 'Igor2' Palinkas
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

module tact6x6(button_height=1.0)
{
    pin_thickness = 0.3;
    pin_width = 0.7;
    pin_spacing = 4.5;

    body_width = 6;
    body_thickness = 3.5;
    plate_thickness = 0.3;

    button_dia = 3.0;
    dimple_size = 1.0;
    dimple_height = 0.3;
    dimple_spacing = 4.5;

    $fn = 30;

    module pin() {
        rotate([90,0,0])
            translate([-pin_thickness/2,1.2,-pin_width/2])
                linear_extrude(height=pin_width)
                    polygon([[0,0],[0.3,0],[0.3,-1.8],[0.6,-3.0],[0.3,-4.2],[0.3,-4.7],[0.15,-5.0],[0,-4.7],[0,-4.2],[0.3,-3.0],[0,-1.8]]);
    }

    module body() {
        union() {
            color([0.9, 0.9, 0.9])
                translate([0,0,body_thickness-plate_thickness/2])
                    cube([body_width,body_width,plate_thickness], true);
            color([0.1,0.1,0.1]) {
                translate([0,0,(body_thickness-plate_thickness)/2])
                    cube([body_width,body_width,body_thickness-plate_thickness], true);
                translate([0,0,body_thickness])
                    cylinder(r=button_dia/2, h=button_height);
                translate([dimple_spacing/2,dimple_spacing/2,body_thickness])
                    cylinder(r=dimple_size/2, h=dimple_height);
                translate([-dimple_spacing/2,dimple_spacing/2,body_thickness])
                    cylinder(r=dimple_size/2, h=dimple_height);
                translate([-dimple_spacing/2,-dimple_spacing/2,body_thickness])
                    cylinder(r=dimple_size/2, h=dimple_height);
                translate([dimple_spacing/2,-dimple_spacing/2,body_thickness])
                    cylinder(r=dimple_size/2, h=dimple_height);                
            }
        }
    }

    union() {
        body();
        color([0.9, 0.9, 0.9]) {
            translate([body_width/2+pin_thickness/2,pin_spacing/2,0])
                    pin();
            translate([body_width/2+pin_thickness/2,-pin_spacing/2,0])
                    pin();
            translate([-body_width/2-pin_thickness/2,pin_spacing/2,0])
               rotate([0,0,180])
                    pin();
            translate([-body_width/2-pin_thickness/2,-pin_spacing/2,0])
               rotate([0,0,180])
                    pin();
        }
    }
}

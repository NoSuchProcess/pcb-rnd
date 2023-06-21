// Model for parametric screw() package
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

module part_screw(hole=3.2, shank_descent = 3, washer_thickness = 0, washer_diameter = 0, shape="hex",head="button")
{
//optional head name for a standard screw: pan, button, cheese, flat-washer, internal-lock-washer

    module head() {
        head_size=2*hole; // average size
        height= head_size/3;

        if (shape=="hex") {
            cylinder(r=head_size/2, h=height, $fn=6);
        } else {
            difference() {
                if (head=="cheese") {
                    cylinder(r=hole*1.7/2, h=height);
                } else if (head=="button") {
                    cylinder(r=hole*1.9/2, h=height);
                } else { //pan, lock.washer, undefined
                    cylinder(r=hole, h=height);
                }
                if(shape=="allen")
                    translate([0,0,head_size/12])
                        cylinder(r=head_size/4, h=head_size, $fn=6);
                if(shape=="xzn") {
                    translate([0,0,head_size/1.6])
                        cube([head_size/3,head_size/3,head_size],true);
                    translate([0,0,head_size/1.6])
                        rotate([0,0,120])
                            cube([head_size/3,head_size/3,head_size],true);
                    translate([0,0,head_size/1.6])
                        rotate([0,0,240])
                            cube([head_size/3,head_size/3,head_size],true);
                }
                if(shape=="torx") {
                    difference() {
                        translate([0,0,head_size/1.6])
                            cube([2*head_size/3,head_size/3,head_size],true);
                        translate([15*head_size/26,0,-head_size/2])
                            cylinder(r=head_size/3,h=head_size, $fn=20);
                        translate([-15*head_size/26,0,-head_size/2])
                            cylinder(r=head_size/3,h=head_size, $fn=20);
                    }
                    rotate([0,0,60])
                    difference() {
                        translate([0,0,head_size/1.6])
                            cube([2*head_size/3,head_size/3,head_size],true);
                        translate([15*head_size/26,0,-head_size/2])
                            cylinder(r=head_size/3,h=head_size, $fn=20);
                        translate([-15*head_size/26,0,-head_size/2])
                            cylinder(r=head_size/3,h=head_size, $fn=20);
                    }
                    rotate([0,0,120])
                    difference() {
                        translate([0,0,head_size/1.6])
                            cube([2*head_size/3,head_size/3,head_size],true);
                        translate([15*head_size/26,0,-head_size/2])
                            cylinder(r=head_size/3,h=head_size, $fn=20);
                        translate([-15*head_size/26,0,-head_size/2])
                            cylinder(r=head_size/3,h=head_size, $fn=20);
                    }
                }
                if (shape=="slot") {
                    translate([0,0,head_size/2+height/2])
                        cube([head_size*2,head_size/6,head_size],true);
                }
                if (shape=="ph") {
                    translate([0,0,head_size/12])
                        linear_extrude(height=head_size)
                            polygon([[-head_size/3,-head_size/12],[-head_size/3,head_size/12],[-head_size/6,head_size/12],[-head_size/12,head_size/6],[-head_size/12,head_size/3],[head_size/12,head_size/3],[head_size/12,head_size/6],[head_size/6,head_size/12],[head_size/3,head_size/12],[head_size/3,-head_size/12],[head_size/6,-head_size/12],[head_size/12,-head_size/6],[head_size/12,-head_size/3],[-head_size/12,-head_size/3],[-head_size/12,-head_size/6],[-head_size/6,-head_size/12]]);
                }
            }
        }
    }

    module shank() {
        translate([0,0,-shank_descent ])
            cylinder(r=hole*0.95/2, h=shank_descent );
    }

    module washer() {
        if (washer_thickness > 0)
            cylinder(r=washer_diameter/2, h=washer_thickness);
    }

    color([0.7,0.7,0.7]) {
        translate([0,0,washer_thickness])
            head();
        shank();
        washer();
    }
}


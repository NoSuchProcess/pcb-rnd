// Model for vertical HEPTAWATT package
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

module part_heptawatt(pin_descent=2.5)
{
    pins=7;
    pin_thickness = 0.5;
    pin_width = 0.7;
    
    pin_neck = 3.0;
    pin_spacing = 1.27;

    tab_thickness = 1.6;
    tab_width = 10.05;
    tab_height = 6.81;
    tab_overhang = 6.8;
    tab_bevel = 0.1;
    
    device_height = 10;
    device_width = 10.05;
    body_thickness = 4.8;
    body_offset = 3.4;

    hole_dia = 3.75;
    hole_height = -4.0;
    notch_size = 3.0;
    notch_height = 6.0;

    module pin() {
        rotate([0,-90,180])
            translate([-pin_thickness/2,0,-pin_width/2])
                linear_extrude(height=pin_width)
                    scale([0.7553,1.0])
                        polygon([[0,0],[0.51,0],[0.51,0.56],[0.54,0.7],[0.6,0.82],[0.71,0.93],[0.84,1.02],[1.0,1.08],[6.26,2.24],[6.64,2.35],[6.86,2.47],[7.01,2.59],[7.11,2.74],[7.15,2.9],[7.15,3+pin_descent],[6.62,3+pin_descent],[6.62,3.29],[6.6,3.14],[6.57,2.99],[6.48,2.87],[6.22,2.76],[6.08,2.72],[0.72,1.55],[0.49,1.45],[0.3,1.34],[0.13,1.21],[0.02,0.99],[0,0.79]]);
    }

    module pin2() {
        rotate([0,-90,180])
            translate([-pin_thickness/2,0,-pin_width/2])
                linear_extrude(height=pin_width)
                    polygon([[0,0],[0.5,0],[0.5,3+pin_descent],[0.0,3+pin_descent]]);
    }

    module body() {
        union() {
            color([0.3,0.3,0.3]) {
                difference() {
                    translate([0,hole_height,-body_offset])
                        translate([device_width/2,0,0])
                            rotate([0,-90,0])
                                linear_extrude(height=device_width)
                                    polygon([[0,0],[1.6,0],[4.8,-0.3],[4.8,-9.6],[3.8,-10.0],[1.6,-10.0],[0,-9.7]]);
                    translate([-device_width/2,hole_height-notch_height,-2])
                        cylinder(r=notch_size/2, h=body_thickness*2);
                    translate([device_width/2,hole_height-notch_height,-2])
                        cylinder(r=notch_size/2, h=body_thickness*2);
                }
            }
            color([0.8,0.8,0.8])
                translate([0,hole_height+tab_overhang,-body_offset])
                    difference() {
                        linear_extrude(height=tab_thickness)
                            polygon([[-tab_width/2,-tab_height],[tab_width/2,-tab_height],[tab_width/2,-tab_bevel],[tab_width/2-tab_bevel,0],[-tab_width/2+tab_bevel,0],[-tab_width/2,-tab_bevel]]);
                    translate([0,-hole_height-tab_overhang,-body_thickness])
                        cylinder(r=hole_dia/2, h=body_thickness*3);
                    }
        }
    }

    translate([-3.2,device_width/2-pin_spacing/2-pin_width/2,0]) {
        rotate([90,0,90]) {
            translate([0,0,0]) {
                union() {
                    color([0.9, 0.9, 0.9]) {
                        for (index = [0:2:pins-1]) {
                            translate([(index+0.5)*pin_spacing-device_width/2+pin_width/2,pin_neck+0.05,-pin_thickness/2])
                                pin();
                        }
                        for (index = [1:2:pins-1]) {
                            translate([(index+0.5)*pin_spacing-device_width/2+pin_width/2,pin_neck+0.05,-pin_thickness/2])
                                pin2();
                        }
                    }
                    translate([-0.2,pin_neck+device_height-hole_height,0])
                        body();
                }
            }
        }
    }
}


// Model for TO220SW package
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

module to220sw()
{
    pin_thickness = 0.5;
    pin_width1 = 1.45;
    pin_width2 = 0.85;
    pin_neck = 3.55;

    tab_thickness = 1.3;
    tab_width1 = 10.3;
    tab_width2 = 7.88;
    tab_neck = 6.3;
    tab_length = 12.3;

    hole_dia = 4.25;
    hole_height = 2.8;
    notch_radius = 0.76;
    
    device_height = 15.1;
    body_thickness = 4.45;
    chamfer = 0.5;
    body_offset = 2.76;
    $fn = 30;
    
    module pin() {
        linear_extrude(height=pin_thickness)
	        polygon([[-pin_width1/2,0],[pin_width1/2,0],[pin_width1/2,pin_width1-pin_width2-pin_neck],[pin_width2/2,-pin_neck],[pin_width2/2,-pin_neck-2.5],[-pin_width2/2,-pin_neck-2.5],[-pin_width2/2,-pin_neck],[-pin_width1/2,pin_width1-pin_width2-pin_neck]]);        
    }
    
    module bent_pin() {
        union () {
            rotate([-45,0,0])
                linear_extrude(height=pin_thickness)
                    polygon([[-pin_width1/2,0],[pin_width1/2,0],[pin_width1/2,pin_width1-pin_width2-pin_neck],[pin_width2/2,-pin_neck],[pin_width2/2,-pin_neck-0.25],[-pin_width2/2,-pin_neck-0.25],
            [-pin_width2/2,-pin_neck],[-pin_width1/2,pin_width1-pin_width2-pin_neck]]);
            translate([-pin_width2/2,-6.04,2.54])
                cube([pin_width2,(3.55-2.54)+2.695,pin_thickness],false);
        }
    }

    module tab() {
        difference() {
            linear_extrude(height=tab_thickness)
                polygon([[-tab_width1/2,0],[tab_width1/2,0],[tab_width1/2,-tab_neck],[tab_width2/2,-tab_neck],[tab_width2/2,-tab_length],[-tab_width2/2,-tab_length],[-tab_width2/2,-tab_neck],[-tab_width1/2,-tab_neck]]);
            translate([0,-hole_height,-1*tab_thickness])
                cylinder(r=hole_dia/2,h=tab_thickness*3);
            translate([-tab_width1/2,notch_radius-hole_height,-tab_thickness])
                cylinder(r=notch_radius,h=tab_thickness*3);
            translate([tab_width1/2,notch_radius-hole_height,-tab_thickness])
                cylinder(r=notch_radius,h=tab_thickness*3);            
        }            
    }

    module body() {
        body_points = [
            [-tab_width1/2,device_height-tab_neck,0], // 0
            [-tab_width1/2,0,0], // 1
            [tab_width1/2,0,0], // 2
            [tab_width1/2,device_height-tab_neck,0], // 3

            [-tab_width1/2+chamfer,device_height-tab_neck-chamfer,body_thickness], // 4
            [-tab_width1/2+chamfer,0,body_thickness], // 5
            [tab_width1/2-chamfer,0,body_thickness], // 6
            [tab_width1/2-chamfer,device_height-tab_neck-chamfer,body_thickness]];// 7

        body_faces = [
            [0,1,2,3], // 0
            [4,5,1,0], // 1
            [7,6,5,4], // 2
            [5,6,2,1], // 3
            [6,7,3,2], // 4
            [7,4,0,3]];// 5

        translate([0,0,0.05])
            polyhedron(body_points, body_faces);        
    }
    translate([2.54,0,0]) {
        rotate([90,0,0]) {
            union() {
                color([0.9, 0.9, 0.9]) {
                    translate([0,pin_neck+0.05,-pin_thickness/2])
                            bent_pin();
                    translate([-2.54,pin_neck+0.05,-pin_thickness/2])
                        pin();
                    translate([2.54,pin_neck+0.05,-pin_thickness/2])
                        pin();
                    translate([0,pin_neck + device_height,-body_offset-pin_thickness/2])
                        tab();
                }
                color([0.1, 0.1, 0.1]) {
                    translate([0,pin_neck,-body_offset-pin_thickness/2])
                        body();
                }
            }
        }
    }
}


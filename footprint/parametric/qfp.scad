// Model for parametric QFP package
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

module part_qfp(pad_spacing=0.65, pins=8, size=7, fillet=0) {

    pin_width=0.3;
    pcb_offset=0.1;
    device_height=1.6;
    bevel = 0.2;

    module fillet(pad_width=pin_width, pad_len=0.5, pad_height= 0.3) {
        fillet_height = pad_height/3;
        fillet_width = pad_len/5;        
        overall_width = fillet_width*2 + pad_width;
        overall_length = pad_len + fillet_width;

        translate([-pad_len,0,0]) {
                fillet_points = [
                    [0,overall_width/2,0], // 0
                    [overall_length,overall_width/2,0], // 1
                    [overall_length,-overall_width/2,0], // 2
                    [0,-overall_width/2,0], // 3
                    [0,pad_width/2+fillet_width/2,fillet_height/3], // 4
                    [pad_len+fillet_width/2,pad_width/2+fillet_width/2,fillet_height/3], // 5
                    [pad_len+fillet_width/2,-pad_width/2-fillet_width/2,fillet_height/3], // 6
                    [0,-pad_width/2-fillet_width/2,fillet_height/3], // 7
                    [0,pad_width/2+fillet_width/6,2*fillet_height/3], // 8
                    [pad_len+fillet_width/6,pad_width/2+fillet_width/6,2*fillet_height/3], // 9
                    [pad_len+fillet_width/6,-pad_width/2-fillet_width/6,2*fillet_height/3], // 10
                    [0,-pad_width/2-fillet_width/6,2*fillet_height/3], // 11
                    [0,pad_width/2,fillet_height], // 12
                    [pad_len,pad_width/2,fillet_height], // 13
                    [pad_len,-pad_width/2,fillet_height], // 14
                    [0,-pad_width/2,fillet_height]]; // 15
                fillet_faces = [
                    [0,4,8,12,15,11,7,3], // 0
                    [0,1,5,4], // 1
                    [1,2,6,5], // 2
                    [7,6,2,3], // 3
                    [4,5,9,8], // 4
                    [5,6,10,9],// 5
                    [10,6,7,11],// 6
                    [8,9,13,12],// 7
                    [9,10,14,13],// 8
                    [15,14,10,11],// 9
                    [12,13,14,15],// 10
                    [3,2,1,0]];// 11
    
                polyhedron(fillet_points, fillet_faces);        
            }
    }

    module opposite_fillet() {
        translate([-size-2,0,0])
            mirror([1,0,0])
                fillet();
    }

    module pin() {
        rotate([90,0,180])
            translate([0,0,-pin_width/2])
                linear_extrude(height=pin_width)
                    polygon([[1.05,0.9],[0.69,0.9],[0.63,0.89],[0.59,0.88],[0.56,0.86],[0.54,0.83],[0.52,0.78],[0.51,0.74],[0.44,0.33],[0.43,0.28],[0.41,0.25],[0.38,0.23],[0.33,0.21],[0.26,0.2],[0.12,0.19],[0.0,0.18],[0.01,0.0],[0.26,0.01],[0.34,0.02],[0.41,0.04],[0.48,0.07],[0.52,0.1],[0.55,0.14],[0.57,0.19],[0.59,0.26],[0.64,0.53],[0.66,0.64],[0.67,0.68],[0.69,0.7],[0.72,0.71],[0.79,0.72],[1.05,0.72]]
);
    }
 
    module opposite_pin() {
        translate([-size-2,0,0])
            mirror([1,0,0])
                pin();
    }

    module body() {
        body_points = [
            [-size/2+bevel,-size/2+0.3+bevel,pcb_offset],//0
            [-size/2+bevel,size/2-0.3-bevel,pcb_offset],
            [-size/2+0.3+bevel,size/2-bevel,pcb_offset],
            [size/2-0.3-bevel,size/2-bevel,pcb_offset],
            [size/2-bevel,size/2-0.3-bevel,pcb_offset],
            [size/2-bevel,-size/2+0.3+bevel*2,pcb_offset],
            [size/2-0.3-bevel*2,-size/2+bevel,pcb_offset],
            [-size/2+0.3+bevel,-size/2+bevel,pcb_offset],//7

            [-size/2,-size/2+0.3,(device_height-pcb_offset)/2],//8
            [-size/2,size/2-0.3,(device_height-pcb_offset)/2],
            [-size/2+0.3,size/2,(device_height-pcb_offset)/2],
            [size/2-0.3,size/2,(device_height-pcb_offset)/2],
            [size/2,size/2-0.3,(device_height-pcb_offset)/2],
            [size/2,-size/2+0.3+bevel,(device_height-pcb_offset)/2],
            [size/2-0.3-bevel,-size/2,(device_height-pcb_offset)/2],
            [-size/2+0.3,-size/2,(device_height-pcb_offset)/2],//15
 
            [-size/2+bevel,-size/2+0.3+bevel,device_height],//16
            [-size/2+bevel,size/2-0.3-bevel,device_height],
            [-size/2+0.3+bevel,size/2-bevel,device_height],
            [size/2-0.3-bevel,size/2-bevel,device_height],
            [size/2-bevel,size/2-0.3-bevel,device_height],
            [size/2-bevel,-size/2+0.3+bevel*2,device_height],
            [size/2-0.3-bevel*2,-size/2+bevel,device_height],
            [-size/2+0.3+bevel,-size/2+bevel,device_height]//23
            ];

        body_faces=[
            [7,6,5,4,3,2,1,0],
            [16,17,18,19,20,21,22,23],

            [0,1,9,8],
            [8,9,17,16],
            [1,2,10,9],
            [9,10,18,17],
            [2,3,11,10],
            [10,11,19,18],
            [3,4,12,11],
            [11,12,20,19],

            [4,5,13,12],
            [12,13,21,20],
            [5,6,14,13],
            [13,14,22,21],
            [6,7,15,14],
            [14,15,23,22],
            [7,0,8,15],
            [15,8,16,23],
            ];

        translate([-size/2-1,(pins/4-1)*pad_spacing/2,0])
            color([0.2,0.2,0.2])
                difference() {
                    polyhedron(body_points, body_faces);
                    translate([size/2-1,-size/2+1,device_height-0.1])
                        cylinder(r=0.5, h=3);
                }
    }

    module place_pins() {
        body_offset = (size-(pins/4-1)*pad_spacing)/2;
        color([0.7,0.7,0.7]) {
            for(i = [0:(pins/4)-1]) {
                translate([0,i*pad_spacing,0]){
                    pin();
                    opposite_pin();
                    if (fillet) {
                        fillet();
                        opposite_fillet();
                    }
                }
                translate([-i*pad_spacing-body_offset-1,
                -1-body_offset,0]){
                    rotate([0,0,-90]) {
                        pin();
                        opposite_pin();
                        if (fillet) {
                            fillet();
                            opposite_fillet();
                        }
                    }
                }
            }
        }
    }
    
    rotate([0,0,0]) {
        body();
        place_pins();
    }

}


// Model for parametric SO family packages
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

module part_so(pad_spacing=0.65, row_spacing=6.4, pins=8, pin_width=0.3, fillet=0, body=0)
{

        module body(body=0, body_overhang=0, pad_spacing=0.65, row_spacing=7.8) {

        if (body==0) { //so
            color([0.2,0.2,0.2])
                translate([0,0,-body_overhang])
                    difference() {
                        linear_extrude(height=pad_spacing*(pins/2-1)+body_overhang*2)
                            polygon([
[1.47,1.6],[1.11,1.28],[0.97,0.65],[1.06,0.3],[1.12,0.21],[1.19,0.17],[1.27,0.15],[4.73,0.15],[4.81,0.17],[4.88,0.21],[4.94,0.33],[5.03,0.66],[4.89,1.38],[4.85,1.49],[4.8,1.55],[4.73,1.58],[4.63,1.6]                            ]);
                        translate([1.31+pin_width,1.65,pin_width])
                            rotate([-90,0,0])
                                cylinder(r=pin_width/2,h=1);
                    }
        } else if (body == 1) { //ssop
            color([0.2,0.2,0.2])
                translate([0,0,-body_overhang])
                    difference() {
                        linear_extrude(height=pad_spacing*(pins/2-1)+body_overhang*2)
                            polygon([
               [1.52,0.15],[1.47,0.16],[1.44,0.17],[1.41,0.2],[1.39,0.24],[1.38,0.28],[1.31,1.08],[1.38,1.9],[1.39,1.94],[1.41,1.97],[1.44,1.99],[1.47,2.0],
               [6.33,2.0],[6.36,1.99],[6.39,1.97],[6.41,1.94],[6.42,1.9],[6.49,1.08],[6.42,0.28],[6.41,0.24],[6.39,0.2],[6.36,0.17],[6.33,0.16],[6.28,0.15]
                            ]);
                        translate([1.31+pin_width*2,1.85,pin_width*2])
                            rotate([-90,0,0])
                                cylinder(r=0.3,h=1);
                    }
         } else if (body == 2) { //tssop
            color([0.2,0.2,0.2])
                translate([0,0,-body_overhang])
                    difference() {
                        linear_extrude(height=pad_spacing*(pins/2-1)+body_overhang*2)
                            polygon([
                        [4.92,1.20],[4.99,1.19],[5.05,1.17],[5.10,1.11],[5.12,1.04],[5.13,0.73],[5.10,0.35],[5.09, 0.32],[5.07,0.29],[5.05,0.27],[5.03,0.26],[4.99,0.25],[4.92,0.24],
                        [1.48,0.24],[1.41,0.25],[1.37,0.26],[1.35,0.27],[1.33,0.29],[1.31,0.32],[1.30,0.35],[1.27,0.73],[1.28,1.04],[1.30,1.11],[1.35,1.17],[1.41,1.19],[1.48,1.20]]);
                        translate([1.27+pin_width*2,1.05,pin_width*2])
                            rotate([-90,0,0])
                                cylinder(r=pin_width/2,h=1);
                    }
         } else if (body == 3) { //msop
            color([0.2,0.2,0.2])
                translate([0,0,-body_overhang])
                    difference() {
                        linear_extrude(height=pad_spacing*(pins/2-1)+body_overhang*2)
                            polygon([
                        [1.13,0.155],[1.09,0.16],[1.05,0.17],[1.015,0.19],[0.99,0.215],[0.975,0.245],[0.955,0.63],[1.005,0.98],[1.015,1.02],[1.035,1.06],[1.06,1.085],[1.095,1.10],
                        [3.805,1.10],[3.84,1.085],[3.865,1.06],[3.885,1.02],[3.895,0.98],[3.945,0.63],[3.925,0.245],[3.91,0.215],[3.885,0.19],[3.85,0.17],[3.81,0.16],[3.77,0.155]]);
                        translate([1.095+0.75,1.0,0.75])
                            rotate([-90,0,0])
                                cylinder(r=0.3,h=1);
                    }
         } else if (body == 4) { //qsop
            color([0.2,0.2,0.2])
                translate([0,0,-body_overhang])
                    difference() {
                        linear_extrude(height=pad_spacing*(pins/2-1)+body_overhang*2)
                            polygon([
                        [1.05,0.2],[0.95,0.84],[0.95,1.12],[0.97,1.33],[1.31,1.75],
                        [4.37,1.75],[4.46,1.1],[4.46,0.83],[4.37,0.2]
                            ]);
                        translate([1.31+pin_width,1.65,pin_width])
                            rotate([-90,0,0])
                                cylinder(r=pin_width/2,h=1);
                    }
         }
    }

    module pin(body=0) {
        if (body == 0) { // so
            translate([0,0,-pin_width/2])
                linear_extrude(height=pin_width)
                    polygon([
[1.12,0.9],[0.8,0.9],[0.72,0.89],[0.66,0.85],[0.61,0.78],[0.58,0.69],[0.53,0.42],[0.5,0.35],[0.46,0.3],[0.41,0.28],[0.0,0.21],[0.03,0.0],[0.52,0.07],[0.59,0.1],[0.64,0.16],[0.68,0.24],[0.74,0.53],[0.77,0.6],[0.8,0.64],[0.85,0.67],[0.89,0.68],[1.12,0.68]
            ]);
        } else if (body == 1) { //ssop
            translate([0,0,-pin_width/2])
                linear_extrude(height=pin_width)
                    polygon([[0.01,0.0],[0.74,0.07],[0.8,0.08],[0.85,0.1],[0.89,0.14],[0.93,0.21],[1.2,0.91],[1.22,0.95],[1.24,0.97],[1.26,0.98],[1.32,0.99],[1.37,0.99],[1.37,1.18],[1.28,1.18],[1.15,1.17],[1.12,1.16],[1.08,1.14],[1.04,1.1],[1.0,1.04],[0.97,0.98],[0.73,0.37],[0.7,0.3],[0.67,0.28],[0.62,0.27],[0.0,0.21]]);
        } else if (body == 2) { //tssop
            translate([0,0,-pin_width/2])
                linear_extrude(height=pin_width)
                    polygon([[0.0,0.0],[0.50,0.01],[0.59,0.03],[0.66,0.06],[0.71,0.10],[0.76,0.15],[0.80,0.20],[0.82,0.26],[0.83,0.33],[0.84,0.44],[0.85,0.50],[0.88,0.57],[0.92,0.61],[0.99,0.63],[1.29,0.63],[1.29,0.83],[0.93,0.83],[0.87,0.81],[0.80,0.77],[0.73,0.70],[0.67,0.60],[0.65,0.50],[0.64,0.42],[0.63,0.36],[0.62,0.32],[0.60,0.28],[0.58,0.26],[0.54,0.24],[0.47,0.22],[0.38,0.21],[0.0,0.20]]);
        } else if (body == 3) { //msop
            translate([0,0,-pin_width/2])
                linear_extrude(height=pin_width)
                    polygon([[0.0,0.0],[0.445,0.025],[0.465,0.03],[0.485,0.04],[0.51,0.055],[0.54,0.09],[0.555,0.115],[0.565,0.145],[0.57,0.175],[0.585,0.485],[0.59,0.505],[0.60,0.52],[0.615,0.53],[0.63,0.535],[1.0,0.535],[1.00,0.735],[0.55,0.735],[0.515,0.73],[0.485,0.72],[0.465,0.71],[0.45,0.695],[0.435,0.675],[0.42,0.65],[0.41,0.63],[0.40,0.585],[0.395,0.545],[0.385,0.30],[0.38,0.255],[0.375,0.245],[0.367,0.235],[0.355,0.23],[0.33,0.225],[0.0,0.20]]);
        } else if (body == 4) { //qsop
            translate([0,0,-pin_width/2])
                linear_extrude(height=pin_width)
                    polygon([[0.01,0.0],[0.24,0.01],[0.3,0.02],[0.37,0.05],[0.43,0.09],[0.48,0.14],[0.52,0.2],[0.55,0.25],[0.57,0.3],[0.59,0.37],[0.6,0.48],[0.6,0.68],[0.62,0.74],[0.67,0.8],[0.73,0.84],[0.78,0.85],[0.97,0.85],[0.97,1.12],[0.75,1.12],[0.67,1.11],[0.6,1.08],[0.53,1.03],[0.48,0.97],[0.43,0.89],[0.4,0.82],[0.38,0.76],[0.37,0.66],[0.37,0.48],[0.36,0.41],[0.34,0.37],[0.32,0.34],[0.28,0.31],[0.23,0.29],[0.16,0.28],[0.0,0.27]]);
        }
    }

    module fillet(pad_width=pin_width, pad_len=0.5, pad_height= 0.3) {
        fillet_height = pad_height/3;
        fillet_width = pad_len/5;
        overall_width = fillet_width*2 + pad_width;
        overall_length = pad_len + fillet_width;

        translate([pad_len,0,0])
            rotate([0,0,180]) {
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

    module opposite_fillet(row_spacing=5.41) {
        translate([row_spacing,0,0])
            mirror([1,0,0])
                fillet();
    }

    module opposite_pin(body=body, row_spacing=7.8) {
        translate([row_spacing,0,0])
            mirror([1,0,0])
                pin(body);
    }

    module place_pins(body=0, pad_spacing=0.65, row_spacing=7.8,pin_width=0.3) {
        color([0.7,0.7,0.7]) {
            for(i = [0:(pins/2)-1]) {
                translate([0,0,i*pad_spacing]){
                    pin(body);
                    opposite_pin(row_spacing=row_spacing);
                }
            }
        }
    }

    module place_fillets(pad_spacing=0.64, row_spacing=5.41) {
        color([0.8,0.8,0.8]) {
            for(i = [0:(pins/2)-1]) {
                translate([0,-i*pad_spacing,0]){
                    fillet();
                    opposite_fillet(row_spacing=row_spacing);
                }
            }
        }
    }

    if (body == 0) {
        rotate([90,0,0]) { // so
            body(body=body, body_overhang=1.09/2,pad_spacing=1.27, row_spacing=6);
            place_pins(body=body, pad_spacing=1.27, row_spacing=6);
        }
        if (fillet)
            place_fillets(pad_spacing=1.27, row_spacing=6);
    } else if (body == 1) { // ssop
        rotate([90,0,0]) {
            body(body=body, body_overhang=0.75/2, pad_spacing=0.65, row_spacing=7.8);
            place_pins(body=body,pad_spacing=0.65, row_spacing=7.8);
        }
        if (fillet)
            place_fillets(pad_spacing=0.65, row_spacing=7.8);
    } else if (body == 2) { // tssop
        rotate([90,0,0]) {
            body(body=body, body_overhang=0.75/2, pad_spacing=0.65, row_spacing=6.4);
            place_pins(body=body,pad_spacing=0.65, row_spacing=6.4);
        }
        if (fillet)
            place_fillets(pad_spacing=0.65, row_spacing=6.4);
    } else if (body == 3) { // msop
        rotate([90,0,0]) {
            body(body=body, body_overhang=0.75/2, pad_spacing=0.65, row_spacing=4.9);
            place_pins(body=body,pad_spacing=0.65, row_spacing=4.9);
        }
        if (fillet)
            place_fillets(pad_spacing=0.65, row_spacing=4.9);
    } else if (body == 4) { // qsop
        rotate([90,0,0]) {
            body(body=body, body_overhang=0.52/2, pad_spacing=0.64, row_spacing=5.41);
            place_pins(body=body, pad_spacing=0.64, row_spacing=5.41);
        }
        if (fillet)
            place_fillets(pad_spacing=0.64, row_spacing=5.41);
    }

}

part_so(body=0, pins=28, fillet=0, pin_width=0.4);

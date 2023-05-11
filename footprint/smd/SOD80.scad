// Model for SOD80 package
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

module package_sod80(fillet=0) {
   body_len=2.8;
   width=1.46;
   height=1.46;
   pad_len=0.35;
   pad_width=1.0;
   pad_height= 1.48; 

   module fillet() {
        fillet_height = pad_height/3;
        fillet_width = pad_len/3;        
        overall_width = fillet_width + pad_width;
        overall_length = pad_len + fillet_width;

        translate([-pad_len/2,0,-pad_height/2]) {
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

	// r[adius], h[eight], [rou]n[d]
	module rounded_cylinder(r,h,n) {
		rotate_extrude(convexity=1) {
			offset(r=n) offset(delta=-n) square([r,h]);
			square([n,h]);
		}
	}

	translate([-1.575,0,0.8]) {
        if(fillet) {
            color([0.8,0.8,0.8]) {
                translate([+body_len+pad_len, 0, -0.06])
                    fillet();
                translate([0, 0, -0.06])
                    rotate([0,0,180])
                        fillet();
            }
        }
		rotate([0,90,0]) {
			union () {
				color([1,0.1,0.1]) {
					translate([0,0,0.35/2])
						rounded_cylinder(r=0.73,h=2.8,n=0.2);
				}
				color([0.1,0.1,0.1]) {
					translate([0,0,0.7])
						cylinder(r=0.74, h=0.35);
				}
				color([0.8,0.8,0.8]) {
					translate([0,0,-0.35/2])
						cylinder(r=0.8, h=0.35);
					translate([0,0,0.35/2 + 2.8])
						cylinder(r=0.8, h=0.35);
				}
			}
		}
	}
}

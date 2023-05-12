// Model for SC90 package
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

module part_sc90(fillet=0)
{

   module fillet(pad_len=0.2, pad_width=0.2, pad_height= 0.15) {
        fillet_height = pad_height/3;
        fillet_width = pad_len/3;        
        overall_width = fillet_width + pad_width;
        overall_length = pad_len + fillet_width;

        translate([-pad_len/2,0,0]) {
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

	module sc90_()
	{
		// pin prototype
		module pin(length, height, thick)
		{
			linear_extrude(height=thick)
				scale([length, -2*height, 1])
					polygon([[0.3400,0.0000],[0.3900,0.0100],[0.4300,0.0300],[0.4600,0.0600],[0.4789,0.0908],[0.6279,0.8307],[0.6500,0.8600],[0.6900,0.8900],[0.7300,0.9000],[1.0000,0.9000],[1.0000,1.0000],[0.6700,1.0012],[0.6100,0.9900],[0.5600,0.9600],[0.5300,0.9200],[0.5200,0.9000],[0.3721,0.1693],[0.3500,0.1400],[0.3100,0.1100],[0.2700,0.1000],[0.0000,0.1000],[0.0000,0.0000]]);
		}

		rotate([90,0,-90]) scale([1.13,1.13,1.0]) translate([-0.625, 0.0, -1.28]) {
			// body
			color([0.1,0.1,0.1])
				linear_extrude(height=1.6)
					polygon([[-0.3,0.1],[-0.4,0.7],[0.4,0.7],[0.3,0.1]]);

			// 3 pins
			color([0.9, 0.9, 0.9]) {
				translate([0.21,0.5,0.3-0.2/2])
					pin(0.5, 0.25, 0.2);
				translate([0.21,0.5,1.3-0.2/2])
					pin(0.5, 0.25, 0.2);

				translate([-0.21,0.5,0.8-0.3/2])
					pin(-0.5, 0.25, 0.3);

			}
		}
	}

    if (fillet) {
			color([0.9, 0.9, 0.9]) {
				translate([0,-0.02,0])
                    fillet();
				translate([0,1.08-0.2/2,0])
					fillet();
  				translate([-1.41,0.63-0.3/2,0])
                    rotate([0,0,180])
                        fillet(pad_width=0.3);
			}
    }

	// match rotation with stock footprint's
	rotate([0,0,90])
		sc90_();
}


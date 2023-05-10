// Model for SOD106A package
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

module part_SOD106A(body_len=4.4, width=2.6, height=2.2, pad_len=1.15, pad_width=1.5, pad_height=1.65)
{
	union() {
        translate([0,width/2,0])
            rotate([90,0,0])
                color([0.1,0.1,0.1])
                    scale([0.89,1,1])
                        linear_extrude(height=width)
                            polygon([
                            [1.4478,0.5842],
                            [1.61036,0.35306],
                            [2.4765,0.24892],
                            [2.64414,1.45796],
                            [2.64414,1.64846],
                            [2.53492,1.64846],
                            [2.45872,2.032],
                            [2.44856,2.05740],
                            [2.43486,2.07772],
                            [2.42062,2.09550],
                            [2.40284,2.11074],
                            [2.37744,2.12598],
                            [2.35966,2.13360],
                            [2.33426,2.13868],
                            [-2.33426,2.13868],
                            [-2.35966,2.13360],
                            [-2.37744,2.12598],
                            [-2.40284,2.11074],
                            [-2.42062,2.09550],
                            [-2.43486,2.07772],
                            [-2.44856,2.05740],
                            [-2.45872,2.032],
                            [-2.53492,1.64846],
                            [-2.64414,1.64846],
                            [-2.64414,1.45796],
                            [-2.4765,0.24892],
                            [-1.61036,0.35306],
                            [-1.4478,0.5842]       
                            ]);

		translate([0,0,pad_height/2]) {
			// terminals
			color([0.8,0.8,0.8]) {
				translate([+body_len/2-pad_len/5, 0, 0])
					cube([pad_len, pad_width, pad_height], center=true);
			}
			color([0.8,0.8,0.8]) {
				translate([-body_len/2+pad_len/5, 0, 0])
					cube([pad_len, pad_width, pad_height], center=true);
			}
        }
        translate([0,0,pad_height/2]) {
			color([0.95,0.95,0.95]) {
				translate([-body_len/2.3+pad_len/2, 0, height/2+0.2])
					cube([pad_len/3, pad_width, 0.1], center=true);
			}
		}
	}
}

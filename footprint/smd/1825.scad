// Model for 1825 package
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

module part_1825(len=4.5, width=6.4, height=1.6, pad_len=0.6)
{
	union() {
		translate([0,0,height/2]) {
			// body
			color([0.1,0.1,0.1])
				cube([len-2*pad_len,width,height], center=true);
			// terminals
			color([0.8,0.8,0.8]) {
				translate([+len/2-pad_len/2, 0, 0])
					cube([pad_len, width, height], center=true);
			}
			color([0.8,0.8,0.8]) {
				translate([-len/2+pad_len/2, 0, 0])
					cube([pad_len, width, height], center=true);
			}
		}
	}
}
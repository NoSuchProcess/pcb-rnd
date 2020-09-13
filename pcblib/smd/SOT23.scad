// Model for SOT23 package
//
// Copyright (C) 2017,2020 Tibor 'Igor2' Palinkas
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

module sot23()
{
	module sot23_()
	{
		// pin prototype
		module pin(length, height, thick)
		{
			linear_extrude(height=thick)
				scale([length, -height, 1])
					polygon([[0.3400,0.0000],[0.3900,0.0100],[0.4300,0.0300],[0.4600,0.0600],[0.4789,0.0908],[0.6279,0.8307],[0.6500,0.8600],[0.6900,0.8900],[0.7300,0.9000],[1.0000,0.9000],[1.0000,1.0000],[0.6700,1.0012],[0.6100,0.9900],[0.5600,0.9600],[0.5300,0.9200],[0.5200,0.9000],[0.3721,0.1693],[0.3500,0.1400],[0.3100,0.1100],[0.2700,0.1000],[0.0000,0.1000],[0.0000,0.0000]]);
		}

		rotate([90,0,-90]) scale([1.13,1.13,1.13]) translate([-0.9, 0.4, -2.1 - 0.43/2]) {
			// body
			color([0.1,0.1,0.1])
				linear_extrude(height=2.9)
					polygon([[-0.55,0],[-0.45,0.53],[0.45,0.53],[0.55,0],[0.45,-0.35],[-0.45,-0.35]]);

			// 3 pins
			color([0.9, 0.9, 0.9]) {
				translate([0.5,0,0.5-0.43/2])
					pin(0.5, 0.4, 0.43);

				translate([0.5,0,2.3-0.43/2])
					pin(0.5, 0.4, 0.43);

				translate([-0.5,0,1.45-0.43/2])
					pin(-0.5, 0.4, 0.43);
			}
		}
	}

	// match rotation with stock footprint's
	rotate([0,0,90])
		sot23_();
}

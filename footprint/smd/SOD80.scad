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

module package_sod80() {
	// r[adius], h[eight], [rou]n[d]
	module rounded_cylinder(r,h,n) {
		rotate_extrude(convexity=1) {
			offset(r=n) offset(delta=-n) square([r,h]);
			square([n,h]);
		}
	}

	translate([-1.575,0,0.8]) {
		rotate([0,90,0]) {
			union () {
				color([1,0.1,0.1]) {
					translate([0,0,0.35/2])
						rounded_cylinder(r=0.73,h=2.8,n=0.2,$fn=80);
				}
				color([0.1,0.1,0.1]) {
					translate([0,0,0.7])
						cylinder(r=0.74, h=0.35, $fn = 30);
				}
				color([0.8,0.8,0.8]) {
					translate([0,0,-0.35/2])
						cylinder(r=0.8, h=0.35, $fn = 30);

					translate([0,0,0.35/2 + 2.8])
						cylinder(r=0.8, h=0.35, $fn = 30);
				}
			}
		}
	}
}


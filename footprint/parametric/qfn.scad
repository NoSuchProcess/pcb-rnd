// Model for parametric QFN package
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

module part_qfn(pins=16,size_x=4,size_y=4,size_z=0.9,pitch=0.65, pin_width=0.3)
{
 
    pin_height=0.2;
 
    module body(pcb_standoff=0.02) {
        color([0.3,0.3,0.3])
            translate([-size_x/2,size_y/2-(size_y-(pins/4-1)*pitch)/2,(size_z-pcb_standoff)/2+pcb_standoff])
                cube([size_x-0.01,size_y-0.01,size_z-pcb_standoff],true);
    }

    module pin() {
        translate([-pin_width/2,0,pin_height/2])
            cube([pin_width,pin_width,pin_height],true);
    }

    module place_pins() {
        y_offset = (size_y-(pins/4-1)*pitch)/2;
        x_offset = (size_x-(pins/4-1)*pitch)/2;
        color([0.7,0.7,0.7]) {
                for(i = [0:pins/4-1]) {
                    translate([0,i*pitch,0])
                        pin();
                    translate([-size_x+pin_width,i*pitch,0])
                        pin();
                    translate([-x_offset+pin_width/2-i*pitch,-y_offset+pin_width/2,0])
                        pin();
                    translate([-x_offset+pin_width/2-i*pitch,size_y-y_offset-pin_width/2,0])
                        pin();

                }
        }
    }

    body();
    place_pins();
}


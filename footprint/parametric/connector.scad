// Model for parametric connector footprint
// openscad-param= {nx=2,ny=3,spacing_mm=2.54,pin_descent=2.5}
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

module part_connector(nx=2, ny=6, spacing=2.54, pin_dia=0.64, pin_descent=2.5) {
    
    module frame(x,y,notch_side) {
        frame_height = 9.1;
        wall_thickness = 1.1;
        notch_width = 4.5;
        //notch_side = 1 or 0
        frame_length =(x-1)*spacing_mm+5.07*2;
        frame_width = (y)*spacing_mm+3.13;

        color([0.3,0.3,0.3])
            translate([-(frame_length-spacing_mm*(x-1))/2,-(frame_width-spacing_mm*(y-1))/2,0])
                difference() {
                    cube([frame_length,frame_width,frame_height],false);
                    translate([wall_thickness,wall_thickness,wall_thickness])
                        cube([frame_length-wall_thickness*2,frame_width-wall_thickness*2,frame_height],false);
                    // notch
                    translate([frame_length/2,notch_side*frame_width-wall_thickness/2,5*frame_height/6])
                        cube([notch_width,notch_width,frame_height],true);
                }
   }
        
    module pin() {
        pin_height=9.1-0.3;
        pin_thickness = pin_dia;
        translate([0,0,pin_height/2-pin_descent/2])
            cube([pin_thickness,pin_thickness,pin_height+pin_descent],true);
    }
   
    module place_pins(x,y) {
        color([0.7,0.7,0.7])
            for(xx=[0:x-1])
                for(yy=[0:y-1])
                    translate([xx*spacing_mm,yy*spacing_mm,0])
                        pin();
    }
   
    if (nx>=ny) {
        frame(x=nx,y=ny,notch_side=1);
        place_pins(x=nx,y=ny);
    } else {
        // +/- translate([0,0,0])
        rotate([0,0,-90]) {
            frame(x=ny,y=nx,notch_side=0);
            place_pins(x=ny,y=nx);
        }
    }
   
}


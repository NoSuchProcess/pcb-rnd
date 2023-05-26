// Model for parametric connector footprint
// openscad-param= {nx=2,ny=6,spacing=2.54,pin_dia=0.64,pin_descent=2.5,shroud=1,female=0,angle=0,elevation=8.8}
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

module part_connector(nx=2, ny=6, spacing=2.54, pin_dia=0.64, pin_descent=2.5, shroud=1, female=0, angle=0, elevation=8.8) {

    module base_block(x,y) {
        base_length = x*spacing;
        base_width = y*spacing;
        base_height = spacing + (1-angle)*female*(elevation-spacing-0.001);
        color([0.3,0.3,0.3]) {
            translate([-spacing/2,-spacing/2,0])
                cube([base_length,base_width, base_height],false);
        }
    }
    
    module shroud(x,y,notch_side) {
        frame_height = elevation+0.3;
        wall_thickness = 1.1;
        notch_width = 4.5;
        //notch_side = 1 or 0
        frame_length =(x-1)*spacing+5.07*2;
        frame_width = (y)*spacing+3.13;

        color([0.3,0.3,0.3])
            translate([-(frame_length-spacing*(x-1))/2,-(frame_width-spacing*(y-1))/2,0])
                difference() {
                    cube([frame_length,frame_width,frame_height],false);
                    translate([wall_thickness,wall_thickness,wall_thickness])
                        cube([frame_length-wall_thickness*2,frame_width-wall_thickness*2,frame_height],false);
                    // notch
                    translate([frame_length/2,notch_side*frame_width-wall_thickness/2,5*frame_height/6])
                        cube([notch_width,notch_width,frame_height],true);
                }
   }
        
    module straight_pin() {
        pin_height = elevation;
        pin_thickness = pin_dia;
        translate([0,0,pin_height/2-pin_descent/2])
            cube([pin_thickness,pin_thickness,pin_height+pin_descent],true);
    }
    
    module bent_pin(row,nx,ny) {
        pin_height = elevation;
        pin_thickness = pin_dia;
            union() {
                translate([0,0,(pin_height-pin_descent-row*spacing)/2])
                    cube([pin_thickness,pin_thickness,pin_height+pin_descent-row*spacing],true);
                translate([0,((ny-row+1)*spacing-pin_thickness)/2,pin_height-row*spacing])
                    rotate([-90,0,0])
                        cube([pin_thickness,pin_thickness,(ny-row+1)*spacing],true);
            }
    }        
   
    module place_pins(x,y) {
        if (angle==0) {
            color([0.7,0.7,0.7])
                for(xx=[0:x-1])
                    for(yy=[0:y-1])
                        translate([xx*spacing,yy*spacing,0])
                                straight_pin();
        } else {
            color([0.7,0.7,0.7])
                for(xx=[0:x-1])
                    for(yy=[0:y-1])
                        translate([xx*spacing,yy*spacing,0])
                                bent_pin(yy,x,y);
        }
    }
   
    if (nx>=ny) {
        translate([0,-(ny-1)*spacing,0]) {
            if (shroud && !angle && !female) {
                shroud(x=nx,y=ny,notch_side=1);
            } else {
                base_block(x=nx,y=ny);
            }
            place_pins(x=nx,y=ny);
        }
    } else {
        rotate([0,0,-90]) {
            if (shroud && !angle && !female) {
                shroud(x=ny,y=nx,notch_side=0);
            } else {
                base_block(x=ny,y=nx);
            }            
            place_pins(x=ny,y=nx);
        }
    }   
}


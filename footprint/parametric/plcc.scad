// Model for parametric plcc() package
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

module part_plcc(pins=44,contact_pitch=1.27,pitch=2.54,row_spacing = 16.0, elevation = 4.57, pin_descent=2.5, through_hole=0, body = 0)
{
    // common sizes 20/,28/7,32=14/18,44/11,52/13,68/17,84/21
    // non square PLCC supported
    module_height=7.4;
    pcb_offset=0.8;
    body_offset = 0.51;
    body_bevel = 0.5;
    body_bevel_pin1 = 1.2;
    body_bevel_corner = 1.2;
    module_overhang=11/2;
    plcc_overhang=5/2;
    pin_width = 0.7;

    pins_per_side = pins/4;

    pins_x = floor((pins_per_side-1)/2 + 3);
    pins_y = ceil((pins_per_side-1)/2 + 3);

    contacts_x = pins_per_side - (pins_y-pins_x);
    contacts_y = pins_per_side + (pins_y-pins_x);

    side_x=contacts_x*contact_pitch+module_overhang*2;
    side_y=contacts_y*contact_pitch+module_overhang*2;

    module body() {
        color([0.3,0.3,0.3])
            intersection() {
                rotate([90,0,0])
                    translate([0,0,-row_spacing])
                        linear_extrude(height= row_spacing*2)
                            polygon([
                                [row_spacing/2-body_bevel, body_offset],
                                [row_spacing/2, (elevation-body_offset)/2+body_offset],
                                [row_spacing/2-body_bevel, elevation],
                                [-row_spacing/2+body_bevel, elevation],
                                [-row_spacing/2, (elevation-body_offset)/2+body_offset],
                                [-row_spacing/2+body_bevel, body_offset]
                        ]);
                rotate([90,0,0])
                    rotate([0,90,0])
                        translate([0,0,-row_spacing])
                            linear_extrude(height= row_spacing*2)
                                polygon([
                                    [row_spacing/2-body_bevel, body_offset],
                                    [row_spacing/2, (elevation-body_offset)/2+body_offset],
                                    [row_spacing/2, elevation-body_bevel_pin1],
                                    [row_spacing/2-body_bevel_pin1, elevation],
                                    [-row_spacing/2+body_bevel, elevation],
                                    [-row_spacing/2, (elevation-body_offset)/2+body_offset],
                                    [-row_spacing/2+body_bevel, body_offset]
                            ]);
                rotate([0,0,-45])
                    translate([sqrt((row_spacing/2)*(row_spacing/2)+(row_spacing/2-body_bevel_corner)*(row_spacing/2-body_bevel_corner))/2,0,0])
                        cube([3*sqrt((row_spacing/2)*(row_spacing/2)+(row_spacing/2-body_bevel_corner)*(row_spacing/2-body_bevel_corner)),3*sqrt((row_spacing/2)*(row_spacing/2)+(row_spacing/2-body_bevel_corner)*(row_spacing/2-body_bevel_corner)),elevation*2], true);
        }
    }        
    
    module body_pin() {
        intersection() {
            rotate([90,0,90])
                translate([0,0,-3])
                    linear_extrude(height=6) {
                        polygon([
           [-0.4,2.8],
           [0.4,2.8],
           [0.4,1.3],
           [0.2,0.9],
           [0.2,0.0],
           [-0.2,0.0],
           [-0.2,0.9],
           [-0.4,1.3]]);
                    }
        
            rotate([90,0,0]) 
                translate([0,0,-0.73/2])
                    linear_extrude(height=0.73) {
                       polygon([
                            [0.95,0.75],
                           [0.95,0.55],
                           [0.85,0.35],
                           [0.75,0.25],
                           [0.55,0.1],
                           [0.35,0.05],
                           [0.1,0.0],
                           [-0.1,0.0],
                           [-0.3,0.05],
                           [-0.45,0.1],
                           [-0.65,0.25],
                           [-0.75,0.35],
                           [-0.85,0.6],
                           [-0.85,0.65],
                           [-0.85,2.4],
                           [-0.8,2.55],
                           [-0.7,2.65],
                           [-0.55,2.75],
                           [-0.4,2.8],
                           [0.0,2.8],
                           [0.0,2.45],
                           [-0.25,2.45],
                           [-0.4,2.4],
                           [-0.5,2.3],
                           [-0.55,2.15],
                           [-0.55,0.7],
                           [-0.5,0.55],
                           [-0.4,0.45],
                           [-0.25,0.35],
                           [-0.05,0.3],
                           [0.05,0.3],
                           [0.3,0.35],
                           [0.5,0.45],
                           [0.65,0.65],
                           [0.65,0.75]
                            ]);
                    }
            }
    }
    
    module place_body_pins() {
        color ([0.8,0.8,0.8]) {
            for(x = [0:contacts_x-1]) {
                    translate([-(contacts_x-1)/2*contact_pitch+x*contact_pitch,row_spacing/2,0])
                        rotate([0,0,-90])
                            body_pin();
                    translate([-(contacts_x-1)/2*contact_pitch+x*contact_pitch,-row_spacing/2,0])
                        rotate([0,0,90])
                            body_pin();          
            }            
            
            for(y = [0:contacts_y-1]) {
                    translate([row_spacing/2,-(contacts_y-1)/2*contact_pitch+y*contact_pitch,0])
                        rotate([0,0,180])
                            body_pin();
                    translate([-row_spacing/2,-(contacts_y-1)/2*contact_pitch+y*contact_pitch,0])
                        body_pin();
            }            
        }
    }

    module socket_body() {
        rotate([0,0,-90])
            color([0.3,0.3,0.3])
                translate([0,0,module_height/2+pcb_offset])
                    union() {
                        translate([0,0,-module_height/2])
                            linear_extrude(height=module_height)
                                polygon([[-side_x/2+module_overhang,-side_y/2],[-side_x/2+module_overhang+2.5,-side_y/2],[-side_x/2,-side_y/2+module_overhang+2.5],[-side_x/2,-side_y/2+module_overhang]]);
        
                        difference() {
                            cube([side_x,side_y,module_height],true);
                            translate([0,0,1])
                                cube([contacts_x*contact_pitch+plcc_overhang*2,contacts_y*contact_pitch+plcc_overhang*2,module_height-1],true);
                            translate([0,0,-module_height])
                                linear_extrude(height=module_height*3)
                                    polygon([[-side_x+2,0],[0,-side_y+2],[0,-side_y-2],[-side_x-2,0]]);
                    }
                }
    }

    module contact() {
        pin_thickness=0.65;
        translate([-pin_thickness/2,-side_x/2+module_overhang/4,0.01])
            rotate([90,0,90])
                linear_extrude(height=pin_thickness)
                    polygon([[0,module_height+pcb_offset],[module_overhang/4+0.3,module_height+pcb_offset],[module_overhang/2+0.01,(module_height+pcb_offset)/2],[module_overhang/2+0.01,pcb_offset],[module_overhang/4,pcb_offset]]);
    }

    module place_contacts() {
        color([0.8,0.8,0.8]) {
            for(i=[0:contacts_x-1]) {
                translate([(-(contacts_x-1)/2+i)*contact_pitch,(side_x-side_y)/2,0])
                    contact();
                translate([(-(contacts_x-1)/2+i)*contact_pitch,(side_y-side_x)/2,0])
                    rotate([0,0,180])
                        contact();
            }
            for(i=[0:contacts_y-1]) {
                translate([0,(-(contacts_y-1)/2+i)*contact_pitch,0])
                    rotate([0,0,90])
                        contact();
                translate([0,(-(contacts_y-1)/2+i)*contact_pitch,0])
                    rotate([0,0,-90])
                        contact();
            }
        }
    }

    module socket_pin() {
        color([0.8,0.8,0.8])
            translate([0,0,(pcb_offset-pin_descent)/2])
                cube([pin_width,pin_width,pcb_offset+pin_descent],true);
    }

    module place_socket_pins() {
        // common sizes 20/,28/7,32=14/18,44/11,52/13,68/17,84/21
//        pins_per_side = pins/4;
//        pins_x = (pins_per_side-1)/2 + 3;
//        pins_y = pins_x;
        outer_x = pins_x;
        outer_y = pins_y;
        translate([-(outer_x-1)/2*pitch,-(outer_y-1)/2*pitch,0])
            for(x = [0:outer_x-1])
                for(y = [0:outer_y-1])
                    if(!(x==0 && y==0) && !(x==outer_x-1 && y==0) && !(x==0 && y==outer_y-1) && !(x==outer_x-1 && y==outer_y-1))
                        if(((x<2) || (x>outer_x-3)) || ((y<2) || (y>outer_y-3)))
                            translate([x*pitch,y*pitch,0])
                                pin();
    }

    translate([0,0,0]) {
        if (body == 2) {
            translate([0,0,4.5]) {
                    body();
                    place_body_pins();
            }
        } else if (body == 0){
            body();
            place_body_pins();
        }
        if (body != 0) {
            socket_body();
            place_contacts();
            if (through_hole)
                place_socket_pins();
        }
    }
}


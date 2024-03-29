// Model for parametric BGA package
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

module part_bga(spacing=0.5, balldia=0.3, nx=3, ny=3, fillet=0, width, height, omit_map, slant=0, elevation=1.2)
{
    pcb_offset=0.9*balldia;
    device_height=elevation;
    bevel = 0.1;
    sizex=width;
    sizey=height;

    module ball() {
        translate([0,0,balldia/2])
            sphere(r=balldia/2);
    }

    module body() {
        body_points = [
            [-sizex/2,-sizey/2+slant,pcb_offset],//0
            [-sizex/2,sizey/2-slant,pcb_offset],
            [-sizex/2+slant,sizey/2,pcb_offset],
            [sizex/2-slant,sizey/2,pcb_offset],
            [sizex/2,sizey/2-slant,pcb_offset],
            [sizex/2,-sizey/2+slant+bevel,pcb_offset],
            [sizex/2-slant-bevel,-sizey/2,pcb_offset],
            [-sizex/2+slant,-sizey/2,pcb_offset],//7

            [-sizex/2,-sizey/2+slant,(device_height-pcb_offset)/2],//8
            [-sizex/2,sizey/2-slant,(device_height-pcb_offset)/2],
            [-sizex/2+slant,sizey/2,(device_height-pcb_offset)/2],
            [sizex/2-slant,sizey/2,(device_height-pcb_offset)/2],
            [sizex/2,sizey/2-slant,(device_height-pcb_offset)/2],
            [sizex/2,-sizey/2+slant+bevel,(device_height-pcb_offset)/2],
            [sizex/2-slant-bevel,-sizey/2,(device_height-pcb_offset)/2],
            [-sizex/2+slant,-sizey/2,(device_height-pcb_offset)/2],//15

            [-sizex/2+bevel,-sizey/2+slant+bevel,device_height],//16
            [-sizex/2+bevel,sizey/2-slant-bevel,device_height],
            [-sizex/2+slant+bevel,sizey/2-bevel,device_height],
            [sizex/2-slant-bevel,sizey/2-bevel,device_height],
            [sizex/2-bevel,sizey/2-slant-bevel,device_height],
            [sizex/2-bevel,-sizey/2+slant+bevel*2,device_height],
            [sizex/2-slant-bevel*2,-sizey/2+bevel,device_height],
            [-sizex/2+slant+bevel,-sizey/2+bevel,device_height]//23
            ];

        body_faces=[
            [7,6,5,4,3,2,1,0],
            [16,17,18,19,20,21,22,23],

            [0,1,9,8],
            [8,9,17,16],
            [1,2,10,9],
            [9,10,18,17],
            [2,3,11,10],
            [10,11,19,18],
            [3,4,12,11],
            [11,12,20,19],

            [4,5,13,12],
            [12,13,21,20],
            [5,6,14,13],
            [13,14,22,21],
            [6,7,15,14],
            [14,15,23,22],
            [7,0,8,15],
            [15,8,16,23],
            ];

        rotate([0,0,180])
                color([0.2,0.2,0.2])
                    difference() {
                        if (slant) {
                            polyhedron(body_points, body_faces);
                        } else {
                            translate([0,0,(device_height+pcb_offset)/2])
                                cube([sizex, sizey, device_height-pcb_offset], true);
                        }
                        translate([sizex/2-1.3*spacing,-sizey/2+1.3*spacing,device_height-0.1])
                            cylinder(r=spacing/2, h=3);
                    }
    }

    module place_balls() {
        color([0.7,0.7,0.7]) {
            for(x = [0:nx-1]) {
                for(y = [0:ny-1]) {
                    translate([x*spacing,y*spacing,0])
                        if (!omit_map[x][(ny-1)-y]) {
                            ball();
                        }
                }
            }
        }
    }

    rotate([0,0,0]) {
        body();
        translate([-(nx-1)*spacing/2,-(ny-1)*spacing/2,0]) {
            place_balls();
        }
    }
}


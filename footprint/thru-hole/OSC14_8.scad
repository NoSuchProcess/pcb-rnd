// Model for OSC8_14 vertical 2 and 3 pin through hole packages
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

module OSC8_14(size=0)
{
    height = 5.3;
    major_diameter = 9.2;
    minor_diameter = 8.94;
    rim_thickness = 0.56;
    pin_diameter = 0.45;
    rim_width = 1.0;
    major_axis1 = 20.5 - size*7.6;
    major_axis2 = major_axis1-rim_width*2;
    minor_axis1 = 13.08 - size*0.18;
    minor_axis2 = minor_axis1-rim_width*2;
    pcb_offset = 0.56;
    rounding = 1;
    minor_rounding = 0.4;
    pin_spacing_x = 2.54*3;
    pin_spacing_y = 2.54*(6-size*3);
    $fn = 50;
    
    // r[adius], h[eight], [rou]n[d]
    module rounded_cylinder(r,h,n) {
        rotate_extrude(convexity=0.1) {
            offset(r=n) offset(delta=-n) square([r,h]);
                square([n,h]);
        }
    }
    
    union() {
        color([0.7,0.7,0.7]) {
            translate ([-(minor_axis1-pin_spacing_x)/2,-(major_axis1-pin_spacing_y)/2-pin_spacing_y,pcb_offset]) {

                // main body
                translate ([rim_width+rounding, major_axis2+rim_width-rounding,0])
                        rounded_cylinder(1, height, minor_rounding);
                translate ([minor_axis2+rim_width-rounding, major_axis2+rim_width-rounding,0])
                        rounded_cylinder(1, height, minor_rounding);
                translate ([minor_axis2+rim_width-rounding, rim_width+rounding,0])
                        rounded_cylinder(1, height, minor_rounding);
                translate ([rim_width+rounding, rim_width+rounding,0])
                        rounded_cylinder(1, height, minor_rounding);

                translate([rim_width,rim_width,0])
                    linear_extrude(height=height-minor_rounding)
                        polygon([[0,major_axis2-rounding],[rounding,major_axis2],[minor_axis2-rounding,major_axis2],[minor_axis2,major_axis2-rounding],[minor_axis2,rounding],[minor_axis2-rounding,0],[rounding,0],[0,rounding]]);
                translate ([rim_width, rim_width,0])
                    linear_extrude(height=height)
                        polygon([[minor_rounding,major_axis2-rounding],[rounding,major_axis2-minor_rounding],[minor_axis2-rounding,major_axis2-minor_rounding],[minor_axis2-minor_rounding,major_axis2-rounding],[minor_axis2-minor_rounding,rounding],[minor_axis2-rounding,minor_rounding],[rounding,minor_rounding],[minor_rounding,rounding]]);

                translate ([rim_width+(minor_axis2-minor_rounding), rim_width+rounding,height-minor_rounding])
                    rotate([-90,0,0])
                        cylinder(r=minor_rounding, h =(major_axis2-2*rounding));
                translate ([rim_width+minor_rounding, rim_width+rounding,height-minor_rounding])
                    rotate([-90,0,0])
                        cylinder(r=minor_rounding, h =(major_axis2-2*rounding));
                translate ([rim_width+rounding, rim_width+minor_rounding,height-minor_rounding])
                    rotate([0,90,0])
                        cylinder(r=minor_rounding, h =(minor_axis2-2*rounding));
                translate ([rim_width+rounding, rim_width+(major_axis2-minor_rounding),height-minor_rounding])
                    rotate([0,90,0])
                        cylinder(r=minor_rounding, h =(minor_axis2-2*rounding));

                //base plate                        
                translate ([rounding,rounding,0])
                    cylinder(r = rounding, h = rim_thickness);
                translate ([minor_axis1-rounding,major_axis1-rounding,0])
                    cylinder(r = rounding, h = rim_thickness);
                translate ([minor_axis1-rounding,rounding,0])
                    cylinder(r = rounding, h = rim_thickness);
                linear_extrude(height=rim_thickness)
                        polygon([[0,major_axis1],[minor_axis1-rounding,major_axis1],[minor_axis1,major_axis1-rounding],[minor_axis1,rounding],[minor_axis1-rounding,0],[rounding,0],[0,rounding]]);

            }

        }
        color([0.8,0.8,0.8]) {
            translate ([0,0,-2.45])
                cylinder(r = pin_diameter/2, h = pcb_offset +2.5);
            translate ([pin_spacing_x,0,-2.45])
                cylinder(r = pin_diameter/2, h = pcb_offset + 2.5);
            translate ([pin_spacing_x,-pin_spacing_y,-2.45])
                cylinder(r = pin_diameter/2, h = pcb_offset + 2.5);
            translate ([0,-pin_spacing_y,-2.45])
                cylinder(r = pin_diameter/2, h = pcb_offset + 2.5);
         }
     }
}


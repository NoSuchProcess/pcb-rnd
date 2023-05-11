// Model for HC51 horizontal 2 and 3 pin through hole packages
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

module HC51H(height=19.7, pins=2, pin_descent=2.5)
{
    major_diameter = 9.2;
    minor_diameter = 8.94;
    rim_thickness = 0.43;
    pin_diameter = 0.43;
    major_axis1 = 18.4;
    major_axis2 = 19.2;
    pcb_offset = 0.1;
    rounding = 0.5;
    
    // r[adius], h[eight], [rou]n[d]
    module rounded_cylinder(r,h,n) {
        rotate_extrude(convexity=1) {
            offset(r=n) offset(delta=-n) square([r,h]);
                square([n,h]);
        }
    }
    
    module curved_pin(pin_sense) {
        rotate([90,0,pin_sense*90])
            translate([major_diameter/2, 0,0])
                rotate_extrude(angle=90, convexity=10)
                    translate([-major_diameter/2, 0,0])
                        circle(r = pin_diameter/2);
    }
    
    // rotate sorted out default lib 3 pin vs 2 pin HC49 alignment difference
    rotate([0,0,(pins-2)*90]) {
        translate([0,major_diameter/2,major_diameter/2]) {
            rotate([-90,0,0]) {
                union() {
                    color([0.7,0.7,0.7]) {
                        translate ([6.15,0,0]) {
                            translate ([(major_axis1-minor_diameter)/2,0,0])
                                rounded_cylinder(minor_diameter/2,height,rounding);
                            translate ([-(major_axis1-minor_diameter)/2,0,0])
                                rounded_cylinder(minor_diameter/2,height,rounding);
                            translate ([0,0,(height-rounding)/2])
                                cube([major_axis1-minor_diameter,minor_diameter, height - rounding], true);
                            translate ([0,0,height-rounding])
                                cube([major_axis1-minor_diameter,minor_diameter-rounding*2, rounding*2], true);
                            translate ([-(major_axis1-minor_diameter)/2,-minor_diameter/2+rounding,height-rounding])
                                rotate([0,90,0])
                                    cylinder(r = rounding, h = major_axis1-minor_diameter);
                            translate ([-(major_axis1-minor_diameter)/2,minor_diameter/2-rounding,height-rounding])
                                rotate([0,90,0])
                                    cylinder(r = rounding, h = major_axis1-minor_diameter);
                            
                            translate ([-(major_axis2-major_diameter)/2,0,0])
                                cylinder(r = major_diameter/2, h = rim_thickness);
                            translate ([(major_axis2-major_diameter)/2,0,0])
                                cylinder(r = major_diameter/2, h = rim_thickness);
                            translate ([0,0,rim_thickness/2])
                                cube([major_axis2-major_diameter,major_diameter, rim_thickness], true);
                        }
                    }
                    color([0.8,0.8,0.8]) {
                        pin_sense = 1; // or -1 for opposite direction
                        curved_pin(pin_sense);
                        translate ([12.3,0,0])
                            curved_pin(pin_sense);
                        if (pins == 3) {
                            translate ([6.15,0,0])
                                curved_pin(pin_sense);
                        }
                        rotate([-90*pin_sense,0,0]) {
                            translate ([0,pin_sense*major_diameter/2,major_diameter-2.45]) {
                                translate ([0,0,-2.15])
                                    cylinder(r = pin_diameter/2, h = pin_descent);
                                translate ([12.3,0,-2.15])
                                    cylinder(r = pin_diameter/2, h = pin_descent);
                                if (pins == 3) {
                                    translate ([6.15,0,-pin_descent])
                                        cylinder(r = pin_diameter/2, h = pin_descent);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

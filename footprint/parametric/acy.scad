// Generic axial thru-hole parts
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

module part_acy(pitch=7.68, standing=0, body=0, body_dia=2.3,pin_descent=2.5, pin_dia=0.3)
{

    pitch_mm = pitch;
    R_width = body_dia;
    R_length = body_dia*2.4;
    R_neck = 4*body_dia/5;
    body_ell = 1.2;

    module rounded_cylinder(r,h,n) {
        rotate_extrude(convexity=1) {
            offset(r=n) offset(delta=-n) square([r,h]);
                square([n,h]);
        }
    }

    module pins(standing=0, link=0) {
        radius=pin_dia;
        component_height=(1-link)*((standing*(R_length+(body_ell-1)*R_width+pin_dia))+(1-standing)*R_width/2)+link*pin_dia/2;

        color([0.8,0.8,0.8]) {
            //pin 1
            translate([0,0,-pin_descent])
                cylinder(r=pin_dia/2, h=pin_descent+component_height-radius);
            // pin 2
            translate([pitch_mm,0,-pin_descent])
                cylinder(r=pin_dia/2, h=pin_descent+component_height-radius);
            // link with ends bent
            rotate([90,0,0]) {
                translate([radius,component_height,0])
                    rotate([0,90,0])
                        cylinder(r=pin_dia/2,h=pitch_mm-radius*2);
                translate([pitch_mm-radius,component_height-radius,0])
                    rotate_extrude(angle=90, convexity=10)
                        translate([radius,0,0])
                            circle(pin_dia/2);
                translate([radius,component_height-radius,0])
                    rotate_extrude(angle=-90, convexity=10)
                        translate([-radius,0,0])
                            circle(pin_dia/2);
            }
        }
    }

    module body(body) {
        if (body == 1 || body == 2) { // resistor, inductor
            color([0.5,0.4+body*body/10,0.9-body*body/10]) {
                translate([-(R_length-R_width)/2,0,0])
                    scale([body_ell,1,1])
                        sphere(R_width/2);
                translate([(R_length-R_width)/2,0,0])
                    scale([body_ell,1,1])
                        sphere(R_width/2);
                translate([-(R_length-R_width)/2,0,0])
                    rotate([0,90,0])
                        cylinder(r=R_neck/2, h=R_length-R_width);
            }
        } else if (body == 3) { //ferrite bead
            color([0.3,0.3,0.3])
                translate([-(R_length)/3,0,0])
                    rotate([0,90,0])
                        cylinder(r=R_width/2, h=2*R_length/3);
        } else if (body == 4) { //axial electro
            color([0.3,0.5,0.6])
                translate([-R_length/2,0,R_width/2])
                    rotate([0,90,0])
                        union () {
                            translate([R_width/2,0,1.5*R_length/6])
                                rounded_cylinder(r=R_width/2, h=4.5*R_length/6, n=0.5);
                            translate([R_width/2,0,0])
                                rounded_cylinder(r=R_width/2, h=R_length/5, n=0.5);
                            translate([R_width/2,0,0])
                                cylinder(r=R_width/2-0.3, h=R_length);
                        }
        } else if (body == 5) {
            band_end = 1; // or 0
            union() {
                color([0.3,0.3,0.3]) {
                    translate([-(R_length)/2,0,0])
                        rotate([0,90,0])
                            cylinder(r=R_width/2, h=R_length);
                }
                color([0.9,0.9,0.9]) {
                    translate([(band_end-1)*(R_length)/3 + band_end*(R_length)/6,0,0])
                        rotate([0,90,0])
                            cylinder(r=R_width/2+0.01, h=R_length/7);
                }
            }
        } else if (body == 6) { // wirewound power resistor
            color([0.9,0.9,0.9])
                cube ([R_length,R_width,R_width],true);
        } else if (body == 7) { // axial monolithic capacitor
            color([0.9,0.9,0.5])
                rotate([0,90,0])
                    translate([0,0,-R_length/2])
                        rounded_cylinder(r=R_width/2, h=R_length, n=R_width/5);
        } else if (body == 8) { // ceramic capacitor
            color([0.9,0.9,0.5])
                intersection () {
                    translate([0,-R_width*4.5,R_width*1.65])
                        sphere(r=R_width*5);
                    translate([0,R_width*4.5,R_width*1.65])
                        sphere(r=R_width*5);
                }
        } else if (body == 9) { // monolithic capacitor, vertical
            color([0.9,0.9,0.5])
                translate([0,0,R_length/3-R_width/2])
                    cube([R_length,R_width,2*R_length/3],true);
        } else if (body == 10) { // 2 pin trimpot
            translate([0,0,10.03/2-R_width/2])
                union() {
                    color([0.4,0.4,0.9])
                        cube([9.53,4.83,10.03],true);
                    color([0.6,0.6,0.2])
                        translate([-3.0,-0.6,5.0])
                            cylinder(r=1.2, h=1.52);
                }
        } else if (body == 11) { // polyester/greencap
            translate([0,0,-R_width/2])
                color([0.3,0.7,0.3])
                    intersection() {
                        translate([0,R_width/4,0])
                            rounded_cylinder(r=R_width/2, h=R_length, n=R_width/5);
                        translate([0,-R_width/4,0])
                            rounded_cylinder(r=R_width/2, h=R_length, n=R_width/5);
                    }
        } else if (body == 12) { // tantalum
                translate([0,0,R_length-R_width])
                    rotate([0,180,0])
                        color([0.9,0.6,0.2])
                            intersection () {
                                rotate([10,0,0])
                                    union() {
                                        cylinder(r=R_width/2, h=R_length*2);
                                        sphere(r=R_width/2);
                                    }
                                rotate([-10,0,0]) 
                                    union() {
                                        cylinder(r=R_width/2, h=R_length*2);
                                        sphere(r=R_width/2);
                                    }
                                translate([-R_length,-R_length,-R_width/2])
                                    cube([R_length*2,R_length*2,R_length],false);
                        }
        }
    }

    module aligned_body(is_standing=0, link=0,body) {
        if (!link) {
            if (is_standing) {
                translate([0,0,(R_length/2)+(body_ell-1)/2*R_width])
                    rotate([0,-90,0])
                        body(body);
            } else {
                translate([pitch_mm/2,0,R_width/2])
                    body(body);
            }
        }
    }

    if (body == 0) { // wire link
        pins(standing=0, link=1);
    } else if (body == 1) { // resistor
        aligned_body(is_standing=standing,link=0,body=1);
        pins(standing, link=0);
    } else if (body == 2) { // inductor
        aligned_body(is_standing=standing,link=0,body=2);
        pins(standing, link=0);
    } else if (body == 3) { // ferrite bead
        aligned_body(is_standing=standing,link=0,body=3);
        pins(standing, link=0);
    } else if (body == 4) { // axial electro
        aligned_body(is_standing=standing,link=0,body=4);
        pins(standing, link=0);
    } else if (body == 5) { // axial diode
        aligned_body(is_standing=standing,link=0,body=5);
        pins(standing, link=0);
    } else if (body == 6) { // axial wirewound power resistor
        aligned_body(is_standing=standing,link=0,body=6);
        pins(standing, link=0);
    } else if (body == 7) { // axial monolithic capacitor
        aligned_body(is_standing=standing,link=0,body=7);
        pins(standing, link=0);
    } else if (body == 8) { // ceramic disc capacitor
        aligned_body(is_standing=0,link=0,body=8);
        pins(link=0);
    } else if (body == 9) { // monoblock/multilayer capacitor
        aligned_body(is_standing=0,link=0,body=9);
        pins(link=0);
    } else if (body == 10) { // two pin potentiometer
        aligned_body(is_standing=0,link=0,body=10);
        pins(standing=0, link=0);
    } else if (body == 11) { // greencap/polyester capacitor
        aligned_body(is_standing=0,link=0,body=11);
        pins(link=1);
    } else if (body == 12) { // tantalum capacitor
        aligned_body(is_standing=0,link=0,body=12);
        pins(link=1);
    }

}


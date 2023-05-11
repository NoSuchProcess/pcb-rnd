// Model for SOD123 package
//
// Copyright (C) 2020 Alexey Kosilin
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

module part_sod123(fillet=0)
{
   A = 1.35 ;
   D = 1.8 ;
   E = 2.84 ;
   He = 3.86 ;
   
   c = 0.15 ;
   L = 0.25 ;
   b = 0.71 ;
   l2 = 0.15 ;

   body_len=E;
   width=D;
   height=A;
   pad_len=L;
   pad_width=b;
   pad_height= c; 
   
    module fillet() {
        fillet_height = pad_height/3;
        fillet_width = pad_len/3;        
        overall_width = fillet_width + pad_width;
        overall_length = pad_len + fillet_width;

        translate([-pad_len/2,0,-pad_height/2]) {
            fillet_points = [
                [0,overall_width/2,0], // 0
                [overall_length,overall_width/2,0], // 1
                [overall_length,-overall_width/2,0], // 2
                [0,-overall_width/2,0], // 3
                [0,pad_width/2+fillet_width/2,fillet_height/3], // 4
                [pad_len+fillet_width/2,pad_width/2+fillet_width/2,fillet_height/3], // 5
                [pad_len+fillet_width/2,-pad_width/2-fillet_width/2,fillet_height/3], // 6
                [0,-pad_width/2-fillet_width/2,fillet_height/3], // 7
                [0,pad_width/2+fillet_width/6,2*fillet_height/3], // 8
                [pad_len+fillet_width/6,pad_width/2+fillet_width/6,2*fillet_height/3], // 9
                [pad_len+fillet_width/6,-pad_width/2-fillet_width/6,2*fillet_height/3], // 10
                [0,-pad_width/2-fillet_width/6,2*fillet_height/3], // 11
                [0,pad_width/2,fillet_height], // 12
                [pad_len,pad_width/2,fillet_height], // 13
                [pad_len,-pad_width/2,fillet_height], // 14
                [0,-pad_width/2,fillet_height]]; // 15
    
            fillet_faces = [
                [0,4,8,12,15,11,7,3], // 0
                [0,1,5,4], // 1
                [1,2,6,5], // 2
                [7,6,2,3], // 3
                [4,5,9,8], // 4
                [5,6,10,9],// 5
                [10,6,7,11],// 6
                [8,9,13,12],// 7
                [9,10,14,13],// 8
                [15,14,10,11],// 9
                [12,13,14,15],// 10
                [3,2,1,0]];// 11

            polyhedron(fillet_points, fillet_faces);        
        }
    }

   module lead()
      color ([0.9, 0.9, 0.9])
      {
         translate ([0, -b/2, L+c])
            rotate ([-90, 0, 0])
               rotate_extrude (angle = 90)
                  translate ([L, 0, 0])
                     square ([c, b]) ;
         
         translate ([L, -b/2, L+c])
            cube ([c, b, A/2 - (L+c)]) ;

         translate ([c+l2+L, b/2, A/2])
            rotate ([90, -90, 0])
               rotate_extrude (angle = 90)
                  translate ([l2, 0, 0])
                     square ([c, b]) ;
      }

   module case()
      translate ([He/2 - E/2 - He/2, -D/2, 0])
         cube ([E, D, A]) ;

       translate ([-He/2, 0, 0])
          lead() ;
       translate ([He/2, 0, 0])
          mirror ([1, 0, 0])
             lead() ;
       if (fillet) {
            color ([0.8, 0.8, 0.8]) {
                translate([+body_len/2+3*pad_len/2, 0, pad_height/2])
                    fillet(); 
                translate([-body_len/2-3*pad_len/2, 0, pad_height/2])
                    rotate([0,0,180])
                        fillet(); 
            }
   }
       
   color ([0.5, 0.5, 0.5])
      intersection()
      {
         translate ([-He/4, -5, -5])
            cube ([0.3, 10, 10]) ;
      
         scale (1.002)
            case() ;
      }

   color ([0.3, 0.3, 0.3])
      case() ;
}

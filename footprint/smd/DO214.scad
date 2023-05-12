// Parametric model for DO214 AA/AB/AC packages
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

module part_do214 (var, fillet=0)
{
   module fillet(pad_len=Tl-L, pad_width=Tw, pad_height= H/4) {
        fillet_height = pad_height/3;
        fillet_width = pad_len/3;        
        overall_width = fillet_width + pad_width;
        overall_length = pad_len + fillet_width;

        translate([-pad_len/2,0,0]) {
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

   module impl (L, W, H, P, Tw, Th)
   {
      delta = 1e-3 ;

      module lead (P, Tw, Th)
         color ([0.9, 0.9, 0.9])
            translate ([-delta, -Tw/2, -delta])
            {
               translate ([0.3/2, 0, 0.3/2])
                  rotate ([-90, 0, 0])
                     cylinder (d = 0.3, h = Tw) ;

               translate ([0.3/2, 0, Th])
                  rotate ([-90, 0, 0])
                     cylinder (d = 0.3, h = Tw) ;
               
               translate ([0, 0, 0.3/2])
                  cube ([0.3, Tw, Th - 0.3/2]) ;
               
               translate ([0.3/2, 0, 0])
                  cube ([3*P - 0.3/2, Tw, 0.3]) ;

               translate ([0.3/2, 0, Th - 0.3/2])
                  cube ([2*P - 0.3/2, Tw, 0.3]) ;
            }

      module case()
         hull()
         {
            translate ([0, 0, H/2])
               cube ([L*0.95, W*0.95, H], center = true) ;

            translate ([0, 0, Th])
               cube ([L, W, delta], center = true) ;
         }

      color ([0.5, 0.5, 0.5])
         intersection()
         {
            translate ([-L/3, -5, -5])
               cube ([0.5, 10, 10]) ;
         
            scale (1.002)
               case() ;
         }

      color ([0.3, 0.3, 0.3])
         case() ;
      
      translate ([-L/2-P, 0, 0])
         lead (P, Tw, Th) ;

      translate ([L/2+P, 0, 0])
         mirror ([1, 0, 0])
            lead (P, Tw, Th) ;
         
       if (fillet) {
            color ([0.8, 0.8, 0.8]) {
                translate ([L/2+P/2, 0, 0])
                    fillet(pad_len=P, pad_width=Tw, pad_height= H/4);
                translate ([-L/2-P/2, 0, 0])
                    rotate([0,0,180])
                        fillet(pad_len=P, pad_width=Tw, pad_height= H/4);
            }
        }         

   }

   variants = [ "AA", "AB", "AC" ] ;

   L = [ 4.57, 7.11, 4.6 ] ;
   W = [ 3.94, 6.22, 2.90 ] ;
   H = [ 2.5, 2.62, 2.45 ] ;
   Tw = [ 2.21, 3.2, 1.65 ] ;
   Tl = [ 5.59, 8.13, 5.35 ] ;

   idx = search ([var], variants)[0] ;
   
   impl (L[idx], W[idx], H[idx], (Tl[idx]-L[idx]) / 2, Tw[idx], H[idx]/2) ;
}


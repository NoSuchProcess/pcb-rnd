// Various tantalum (from A to E) packages model
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

module part_tantalum (sz)
{
   module impl (L, W, H, P, Tw, Th)
   {
      delta = 1e-3 ;
      $fn = 30 ;

      module lead (P, Tw, Th)
         color ([0.9, 0.9, 0.9])
            translate ([-delta, -Tw/2, -delta])
            {
               translate ([0.3/2, 0, 0.3/2])
                  rotate ([-90, 0, 0])
                     cylinder (d = 0.3, h = Tw) ;
               
               translate ([0, 0, 0.3/2])
                  cube ([0.3, Tw, Th - 0.3/2]) ;
               
               translate ([0.3/2, 0, 0])
                  cube ([P - 0.3/2, Tw, 0.3]) ;
            }
      
      module case()
         hull()
         {
            translate ([0, 0, H/2])
               cube ([L - 0.3*2, W - 0.2, H], center = true) ;

            translate ([0, 0, Th])
               cube ([L, W, delta], center = true) ;
         }

      color ([0.7, 0.55, 0])
         intersection()
         {
            translate ([-L/3, -5, -5])
               cube ([P/2, 10, 10]) ;
         
            scale (1.001)
               case() ;
         }
      
      color ([0.9, 0.8, 0.2])
         case() ;

      translate ([-L/2, 0, 0])
         lead (P, Tw, Th) ;
         
      translate ([L/2, 0, 0])
         mirror ([1, 0, 0])
            lead (P, Tw, Th) ;
   }

   sizes = "ABCDE" ;
   L = [ 3.2, 3.5, 6.0, 7.3, 7.3 ] ;
   W = [ 1.6, 2.8, 3.2, 4.3, 4.3 ] ;
   H = [ 1.6, 1.9, 2.5, 2.8, 4 ] ;
   P = [ 0.8, 0.8, 1.3, 1.3, 1.3 ] ;
   Tw = [ 1.2, 2.2, 2.2, 2.4, 2.4 ] ;
   
   idx = search (sz, sizes)[0] ;
   impl (L[idx], W[idx], H[idx], P[idx], Tw[idx], H[idx]/2) ;
}

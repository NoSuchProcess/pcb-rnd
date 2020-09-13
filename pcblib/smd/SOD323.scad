// Model for SOD323 package
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

module part_sod323()
{
   $fn = 30 ;

   module lead()
      color ([0.9, 0.9, 0.9])
      {
         translate ([0, -0.35/2, 0.4])
            rotate ([-90, 0, 0])
               rotate_extrude (angle = 90)
                  translate ([0.25, 0, 0])
                     square ([0.15, 0.35]) ;
         
         translate ([0.25, -0.35/2, 0.4])
            cube ([0.15, 0.35, 1.35/2 - 0.4]) ;

         translate ([0.5, 0.35/2, 1.35/2])
            rotate ([90, -90, 0])
               rotate_extrude (angle = 90)
                  translate ([0.1, 0, 0])
                     square ([0.15, 0.35]) ;
      }

   module case()
      translate ([-0.9, -0.7, 0])
         cube ([1.8, 1.4, 1.35]) ;
      
   translate ([-0.5-0.9, 0, 0])
      lead() ;
   
   mirror ([1, 0, 0])
      translate ([-0.5-0.9, 0, 0])
         lead() ;
   
   color ([0.5, 0.5, 0.5])
      intersection()
      {
         translate ([-0.7, -5, -5])
            cube ([0.3, 10, 10]) ;
      
         scale (1.002)
            case() ;
      }

   color ([0.3, 0.3, 0.3])
      case() ;
}

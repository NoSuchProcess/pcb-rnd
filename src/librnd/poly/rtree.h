 /*
  *                            COPYRIGHT
  *
  *  pcb-rnd, interactive printed circuit board design
  *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
  *
  *  This program is free software; you can redistribute it and/or modify
  *  it under the terms of the GNU General Public License as published by
  *  the Free Software Foundation; either version 2 of the License, or
  *  (at your option) any later version.
  *
  *  This program is distributed in the hope that it will be useful,
  *  but WITHOUT ANY WARRANTY; without even the implied warranty of
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  *  GNU General Public License for more details.
  *
  *  You should have received a copy of the GNU General Public License
  *  along with this program; if not, write to the Free Software
  *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
  *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
  */

#ifndef RND_RTREE_H
#define RND_RTREE_H

#include <librnd/core/global_typedefs.h>

typedef long int rnd_rtree_cardinal_t;
typedef rnd_coord_t rnd_rtree_coord_t;

/* Instantiate an rtree */
#define RTR(n)  rnd_rtree_ ## n
#define RTRU(n) rnd_RTREE_ ## n
#define rnd_rtree_privfunc static
#define rnd_rtree_size 6
#define rnd_rtree_stack_max 1024

#define RTREE_NO_TREE_TYPEDEFS

#include <genrtree/genrtree_api.h>

#endif /* RND_RTREE_H */

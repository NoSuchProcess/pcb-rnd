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
  */

#include "config.h"

#include <assert.h>
#include <string.h>

#include "unit.h"
#include "rtree2.h"

/* Instantiate an rtree */
#define RTR(n)  pcb_rtree_ ## n
#define RTRU(n) pcb_RTREE_ ## n
#define pcb_rtree_privfunc static
#define pcb_rtree_size 6

#include <genrtree/genrtree_impl.h>
#include <genrtree/genrtree_search.h>
#include <genrtree/genrtree_delete.h>

/* Temporary compatibility layer for the transition */
pcb_rtree_t *pcb_r_create_tree(void)
{

}



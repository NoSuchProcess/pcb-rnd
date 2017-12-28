/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1998,1999,2000,2001 harry eaton
 *
 *  this file, rtree.h, was written and is
 *  Copyright (c) 2004 harry eaton, it's based on C. Scott's kdtree.h template
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
 *  Contact addresses for paper mail and Email:
 *  harry eaton, 6697 Buttonhole Ct, Columbia, MD 21044 USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

/* Compatibility layer between the new rtree and the old rtree for the
   period of transition */

/* callback direction to the search engine */
typedef enum pcb_r_dir_e {
	PCB_R_DIR_NOT_FOUND = 0,         /* object not found or not accepted */
	PCB_R_DIR_FOUND_CONTINUE = 1,    /* object found or accepted, go on searching */
	PCB_R_DIR_CANCEL = 2             /* cancel the search and return immediately */
} pcb_r_dir_t;

pcb_rtree_t *pcb_r_create_tree(void);

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* some commonly used macros not related to a special C-file
 * the file is included by global.h after const.h */

#ifndef	PCB_MACRO_H
#define	PCB_MACRO_H

#ifndef	PCB_SWAP_IDENT
#define	PCB_SWAP_IDENT			conf_core.editor.show_solder_side
#endif

#define PCB_ENTRIES(x)         (sizeof((x))/sizeof((x)[0]))
#define PCB_UNKNOWN(a)         ((a) && *(a) ? (a) : "(unknown)")
#define PCB_NSTRCMP(a, b)      ((a) ? ((b) ? strcmp((a),(b)) : 1) : -1)
#define PCB_EMPTY(a)           ((a) ? (a) : "")
#define PCB_EMPTY_STRING_P(a)  ((a) ? (a)[0]==0 : 1)
#define PCB_XOR(a,b)           (((a) && !(b)) || (!(a) && (b)))

#endif

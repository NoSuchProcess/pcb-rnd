/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
 *  pcb-rnd Copyright (C) 2017, Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* This file was originally written by Bill Wilson for the PCB Gtk port;
   refactored for pcb-rnd by Tibor 'Igor2' Palinkas */

#include <gtk/gtk.h>
#include "in_mouse.h"

pcb_hid_cfg_mouse_t ghid_mouse;
int ghid_wheel_zoom = 0;


pcb_hid_cfg_mod_t ghid_mouse_button(int ev_button)
{
	/* GDK numbers buttons from 1..5, there seem to be no symbolic names */
	return (PCB_MB_LEFT << (ev_button-1));
}

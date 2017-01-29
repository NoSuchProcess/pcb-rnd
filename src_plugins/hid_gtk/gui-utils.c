/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* This module, gui-utils.c, was written by Bill Wilson and the functions
 * here are Copyright (C) 2004 by Bill Wilson.  These functions are utility
 * functions which are taken from my other GPL'd projects gkrellm and
 * gstocks and are copied here for the Gtk PCB port.
 */

#include "config.h"
//#include "conf_core.h"

#include "gui.h"


void ghid_draw_area_update(GHidPort * port, GdkRectangle * rect)
{
	gdk_window_invalidate_rect(gtk_widget_get_window(port->drawing_area), rect, FALSE);
}

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 */


#include "config.h"

#include <gtk/gtk.h>

#include "bu_icons.h"
#include "bu_icons.data"

/* It's enough to load these once, globally - shared data among all callers */
static int inited = 0;

GdkPixmap *XC_clock_source, *XC_clock_mask, *XC_hand_source, *XC_hand_mask, *XC_lock_source, *XC_lock_mask;

void pcb_gtk_icons_init(GdkWindow *top_window)
{
	GdkPixbuf *icon;

	if (inited)
		return;

	icon = gdk_pixbuf_new_from_xpm_data((const gchar **) icon_bits);
	gtk_window_set_default_icon(icon);

	XC_clock_source = gdk_bitmap_create_from_data(top_window, (char *) rotateIcon_bits, rotateIcon_width, rotateIcon_height);
	XC_clock_mask = gdk_bitmap_create_from_data(top_window, (char *) rotateMask_bits, rotateMask_width, rotateMask_height);

	XC_hand_source = gdk_bitmap_create_from_data(top_window, (char *) handIcon_bits, handIcon_width, handIcon_height);
	XC_hand_mask = gdk_bitmap_create_from_data(top_window, (char *) handMask_bits, handMask_width, handMask_height);

	XC_lock_source = gdk_bitmap_create_from_data(top_window, (char *) lockIcon_bits, lockIcon_width, lockIcon_height);
	XC_lock_mask = gdk_bitmap_create_from_data(top_window, (char *) lockMask_bits, lockMask_width, lockMask_height);

	inited = 1;
}


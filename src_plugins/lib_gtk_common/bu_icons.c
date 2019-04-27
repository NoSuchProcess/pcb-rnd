/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *  pcb-rnd Copyright (C) 2017 Alain Vigne
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */


#include "config.h"

#include <gtk/gtk.h>

#include "bu_icons.h"
#include "bu_icons.data"

/* It's enough to load these once, globally - shared data among all callers */
static int inited = 0;

GdkPixbuf *XC_clock_source, *XC_hand_source, *XC_lock_source;

/* Allocates (with refco=1) an RGBA 24x24 GdkPixbuf from data, using mask;
   width and height should be smaller than 24. Use g_object_unref() to free
   the structure. */
static GdkPixbuf *pcb_gtk_cursor_from_xbm_data(unsigned char *data, unsigned char *mask, unsigned int width, unsigned int height)
{
	GdkPixbuf *dest;
	unsigned char *src_data, *mask_data, *dest_data;
	int dest_stride;
	unsigned char reg, reg_m;
	unsigned char data_b, mask_b;
	int bits;
	int x, y;

	g_return_val_if_fail((width < 25 && height < 25), NULL);

	/* Creates a new suitable GdkPixbuf structure for cursor. */
	dest = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 24, 24);
	dest_data = gdk_pixbuf_get_pixels(dest);
	dest_stride = gdk_pixbuf_get_rowstride(dest);

	src_data = data;
	mask_data = mask;

	/* Copy data + mask into GdkPixbuf RGBA pixels. */
	for (y = 0; y < height; y++) {
		bits = 0;
		for (x = 0; x < width; x++) {
			if (bits == 0) {
				reg = *src_data++;
				reg_m = *mask_data++;
				bits = 8;
			}
			data_b = (reg & 0x01) ? 255 : 0;
			reg >>= 1;
			mask_b = (reg_m & 0x01) ? 255 : 0;
			reg_m >>= 1;
			bits--;

			dest_data[x * 4 + 0] = data_b;
			dest_data[x * 4 + 1] = data_b;
			dest_data[x * 4 + 2] = data_b;
			dest_data[x * 4 + 3] = mask_b;
		}

		dest_data += dest_stride;
	}

	return dest;
}

void pcb_gtk_icons_init(GtkWindow *top_window)
{
	GdkPixbuf *icon;

	if (inited)
		return;

	icon = gdk_pixbuf_new_from_xpm_data((const gchar **)icon_bits);
	gtk_window_set_default_icon(icon);
	gtk_window_set_icon(top_window, icon);

	XC_clock_source = pcb_gtk_cursor_from_xbm_data(rotateIcon_bits, rotateMask_bits, rotateIcon_width, rotateIcon_height);

	XC_hand_source = pcb_gtk_cursor_from_xbm_data(handIcon_bits, handMask_bits, handIcon_width, handIcon_height);

	XC_lock_source = pcb_gtk_cursor_from_xbm_data(lockIcon_bits, lockMask_bits, lockIcon_width, lockIcon_height);

	inited = 1;
}

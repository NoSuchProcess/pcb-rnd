/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <Xm/PushB.h>

#include "compat_misc.h"
#include "color.h"

static void fill_bar(Display *display, XImage *image, const pcb_color_t *color, int width, int height)
{
	int i, j, bytes_per_pixel;
	char *q;
	XColor cl;
	unsigned long c;
	Colormap colormap;

	colormap = DefaultColormap(display, DefaultScreen(display));

	cl.red = color->r << 8;
	cl.green = color->g << 8;
	cl.blue = color->b << 8;
	if (!XAllocColor(display, colormap, &cl))
		return 0;
	c = cl.pixel;

	bytes_per_pixel = image->bytes_per_line / width;

	/* fill in server's format */
	for(j = 0; j < height; j++) {

		q = image->data + image->bytes_per_line * j;
		if (image->byte_order == MSBFirst) {
			switch (bytes_per_pixel) {
				case 4:
					for(i = 0; i < width; i++) {
						*q++ = c >> 24;
						*q++ = c >> 16;
						*q++ = c >> 8;
						*q++ = c;
					}
					break;
				case 3:
					for(i = 0; i < width; i++) {
						*q++ = c >> 16;
						*q++ = c >> 8;
						*q++ = c;
					}
					break;
				case 2:
					for(i = 0; i < width; i++) {
						*q++ = c >> 8;
						*q++ = c;
					}
					break;
				case 1:
					for(i = 0; i < width; i++)
						*q++ = c;
					break;
			}
		}
		else {
			switch (bytes_per_pixel) {
				case 4:
					for(i = 0; i < width; i++) {
						*q++ = c;
						*q++ = c >> 8;
						*q++ = c >> 16;
						*q++ = c >> 24;
					}
					break;
				case 3:
					for(i = 0; i < width; i++) {
						*q++ = c;
						*q++ = c >> 8;
						*q++ = c >> 16;
					}
					break;
				case 2:
					for(i = 0; i < width; i++) {
						*q++ = c;
						*q++ = c >> 8;
					}
					break;
				case 1:
					for(i = 0; i < width; i++)
						*q++ = c;
					break;
			}
		}
	}
}


static Pixmap gen_color_bar(Display *display, const pcb_color_t *color, int width, int height)
{
	Visual *visual;
	Pixmap px;
	XImage *image;
	int depth;
	GC gc;
	XGCValues gcv;

	depth = DefaultDepth(display, DefaultScreen(display));
	visual = DefaultVisual(display, DefaultScreen(display));
	px = XCreatePixmap(display, DefaultRootWindow(display), width, height, depth);
	gc = XCreateGC(display, px, 0, &gcv);
	image = XCreateImage(display, visual, depth, ZPixmap, 0, 0, width, height, 8, 0);
	image->data = malloc(image->bytes_per_line * height + 16);

	fill_bar(display, image, color, width, height);

	XPutImage(display, px, gc, image, 0, 0, 0, 0, width, height);
	free(image->data);

	return px;
}

Widget pcb_ltf_color_button(Display *display, Widget parent, String name, const pcb_color_t *color)
{
	Widget btn;
	Pixel background;
	Pixmap px;
	Arg args[3];
	int n = 0;

	px = gen_color_bar(display, color, 32, 16);
	if (px == 0)
		return 0;

	btn = XmCreatePushButton(parent, name, (Arg *)NULL, (Cardinal) 0);
	XtVaGetValues(btn, XmNbackground, &background, NULL);

	XtSetArg(args[n], XmNlabelType, XmPIXMAP); n++;
	XtSetArg(args[n], XmNarmPixmap, px); n++;
	XtSetValues(btn, args, n);

	return btn;
}

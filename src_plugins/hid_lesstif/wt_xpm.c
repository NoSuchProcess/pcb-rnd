/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
#include <Xm/Label.h>
#include <Xm/PushB.h>

#include "compat_misc.h"


static unsigned long hex_digit(char c)
{
	if ((c >= '0') && (c <= '9')) return c - '0';
	if ((c >= 'a') && (c <= 'f')) return c - 'a' + 10;
	if ((c >= 'A') && (c <= 'F')) return c - 'A' + 10;
	return 0;
}

static unsigned long hex_byte(const char *s)
{
	return (hex_digit(s[0]) << 12) + (hex_digit(s[1]) << 8);
}

static unsigned long hex_raw(unsigned long byte)
{
	return (byte & 0xff) << 8;
}

Pixmap pcb_ltf_parse_xpm(Display *display, const char **xpm, Pixel bgcolor)
{
	int i, j, width, height, n_colors, depth = 0;
	const char **p, *hdr;
	char *q;
	unsigned long colors[256];
	int bytes_per_pixel;
	XGCValues gcv;
	Visual *visual;
	Pixmap pixmap;
	XImage *image;
	Colormap colormap;
	GC gc;

	depth = DefaultDepth(display, DefaultScreen(display));
	visual = DefaultVisual(display, DefaultScreen(display));
	colormap = DefaultColormap(display, DefaultScreen(display));
	p = xpm;

	/* Parse header */
	hdr = p[0];
	width = atoi(hdr);
	hdr = strchr(hdr, ' ');
	if (hdr == NULL)
		return 0;
	hdr++;
	height = atoi(hdr);
	hdr = strchr(hdr, ' ');
	if (hdr == NULL)
		return 0;
	hdr++;
	n_colors = atoi(hdr);

	/* sanity checks */
	if (n_colors > sizeof(colors) / sizeof(colors[0]))
		return 0;
	if ((height < 1) || (width < 1))
		return 0;

	pixmap = XCreatePixmap(display, DefaultRootWindow(display), width, height, depth);
	gc = XCreateGC(display, pixmap, 0, &gcv);
	image = XCreateImage(display, visual, depth, ZPixmap, 0, 0, width, height, 8, 0);
	image->data = malloc(image->bytes_per_line * height + 16);

	/* Parse and store colors */
	for(p = p + 1, i = 0; i < n_colors; p++, i++) {
		XColor c, c1;
		const char *x;
		int idx = ((*p)[0]);

		x = *p + 4;
		if (*x == '#') {
			c.red = hex_byte(x+1);
			c.green = hex_byte(x+3);
			c.blue = hex_byte(x+5);
			if (!XAllocColor(display, colormap, &c))
				goto error;
		}
		else if (pcb_strcasecmp(x, "None") == 0) {
/*			c.red = hex_raw((bgcolor & 0xff0000) >> 16);
			c.green = hex_raw((bgcolor & 0x00ff00) >> 8);
			c.blue = hex_raw(bgcolor & 0x0000ff);
			if (!XAllocColor(display, colormap, &c))
				goto error;*/
			c.pixel = bgcolor;
		}
		else {
			if (!XAllocNamedColor(display, colormap, (char *)x, &c, &c1))
				goto error;
		}
		colors[idx] = c.pixel;
	}

	bytes_per_pixel = image->bytes_per_line / width;

	/* convert to server's format */
	for(j = 0; j < height; j++, p++) {
		const char *r;
		unsigned long c;
		q = image->data + image->bytes_per_line * j;
		r = *p;
		if (image->byte_order == MSBFirst) {
			switch (bytes_per_pixel) {
				case 4:
					for(i = 0; i < width; i++) {
						c = colors[(int)(*r++)];
						*q++ = c >> 24;
						*q++ = c >> 16;
						*q++ = c >> 8;
						*q++ = c;
					}
					break;
				case 3:
					for(i = 0; i < width; i++) {
						c = colors[(int)(*r++)];
						*q++ = c >> 16;
						*q++ = c >> 8;
						*q++ = c;
					}
					break;
				case 2:
					for(i = 0; i < width; i++) {
						c = colors[(int)(*r++)];
						*q++ = c >> 8;
						*q++ = c;
					}
					break;
				case 1:
					for(i = 0; i < width; i++)
						*q++ = colors[(int)(*r++)];
					break;
			}
		}
		else {
			switch (bytes_per_pixel) {
				case 4:
					for(i = 0; i < width; i++) {
						c = colors[(int)(*r++)];
						*q++ = c;
						*q++ = c >> 8;
						*q++ = c >> 16;
						*q++ = c >> 24;
					}
					break;
				case 3:
					for(i = 0; i < width; i++) {
						c = colors[(int)(*r++)];
						*q++ = c;
						*q++ = c >> 8;
						*q++ = c >> 16;
					}
					break;
				case 2:
					for(i = 0; i < width; i++) {
						c = colors[(int)(*r++)];
						*q++ = c;
						*q++ = c >> 8;
					}
					break;
				case 1:
					for(i = 0; i < width; i++)
						*q++ = colors[(int)(*r++)];
					break;
			}
		}
	}

	XPutImage(display, pixmap, gc, image, 0, 0, 0, 0, width, height);
	free(image->data);
	return pixmap;

	error:;
	free(image->data);
	return 0;
}


Widget pcb_ltf_xpm_label(Display *display, Widget parent, String name, const char **xpm)
{
	Widget Label1;
	Pixel background;
	Pixmap px_disarm;
	Arg args[3];
	int n = 0;

	Label1 = XmCreateLabel(parent, name, (Arg *)NULL, (Cardinal) 0);
	XtVaGetValues(Label1, XmNbackground, &background, NULL);

	px_disarm = pcb_ltf_parse_xpm(display, xpm, background);

	XtSetArg(args[n], XmNlabelType, XmPIXMAP); n++;
	XtSetArg(args[n], XmNlabelPixmap, px_disarm); n++;
	XtSetValues(Label1, args, n);

	return Label1;
}

Widget pcb_ltf_xpm_button(Display *display, Widget parent, String name, const char **xpm)
{
	Widget Label1;
	Pixel background;
	Pixmap px_disarm;
	Arg args[3];
	int n = 0;

	Label1 = XmCreatePushButton(parent, name, (Arg *)NULL, (Cardinal) 0);
	XtVaGetValues(Label1, XmNbackground, &background, NULL);

	px_disarm = pcb_ltf_parse_xpm(display, xpm, background);

	XtSetArg(args[n], XmNlabelType, XmPIXMAP); n++;
	XtSetArg(args[n], XmNarmPixmap, px_disarm); n++;
	XtSetValues(Label1, args, n);

	return Label1;
}

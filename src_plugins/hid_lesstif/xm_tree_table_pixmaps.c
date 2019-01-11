/*-----------------------------------------------------------------------------
 * ListTree	A list widget that displays a file manager style tree
 *
 * Copyright (c) 1996 Robert W. McMullen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Author: Rob McMullen <rwmcm@mail.ae.utexas.edu>
 *         http://www.ae.utexas.edu/~rwmcm
 *
 * Minor modifications and cleaning-ups by E. Stambulchik
 */

#include "xm_tree_table_priv.h"

/*----------- pixmap bits ------------*/
#define folder_width 16
#define folder_height 12
static unsigned char folder_bits[] = {
	0x00, 0x1f, 0x80, 0x20, 0x7c, 0x5f, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40,
	0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0xfc, 0x3f,
};

#define folderopen_width 16
#define folderopen_height 12
static unsigned char folderopen_bits[] = {
	0x00, 0x3e, 0x00, 0x41, 0xf8, 0xd5, 0xac, 0xaa, 0x54, 0xd5, 0xfe, 0xaf,
	0x01, 0xd0, 0x02, 0xa0, 0x02, 0xe0, 0x04, 0xc0, 0x04, 0xc0, 0xf8, 0x7f,
};

#define document_width 9
#define document_height 14
static unsigned char document_bits[] = {
	0x1f, 0x00, 0x31, 0x00, 0x51, 0x00, 0x91, 0x00, 0xf1, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0xff, 0x01,
};

/*----------- end of pixmap bits --------*/
int make_pixmap_data(XmTreeTableWidget w, Pixinfo *pix);


int init_pixmaps(XmTreeTableWidget w)
{
	w->tree_table.n_max_pixmap_height = 0;

	if (w->tree_table.pix_branch_closed.bitmap == XtUnspecifiedPixmap)
		w->tree_table.pix_branch_closed.bitmap = XCreateBitmapFromData(XtDisplay((Widget)w), RootWindowOfScreen(XtScreen((Widget)w)), (char *)folder_bits, folder_width, folder_height);
	if (0 != make_pixmap_data(w, &w->tree_table.pix_branch_closed))
		return -1;

	if (w->tree_table.pix_branch_open.bitmap == XtUnspecifiedPixmap)
		w->tree_table.pix_branch_open.bitmap = XCreateBitmapFromData(XtDisplay((Widget)w), RootWindowOfScreen(XtScreen((Widget)w)), (char *)folderopen_bits, folderopen_width, folderopen_height);
	if (0 != make_pixmap_data(w, &w->tree_table.pix_branch_open))
		return -1;

	if (w->tree_table.pix_leaf.bitmap == XtUnspecifiedPixmap)
		w->tree_table.pix_leaf.bitmap = XCreateBitmapFromData(XtDisplay((Widget)w), RootWindowOfScreen(XtScreen((Widget)w)), (char *)document_bits, document_width, document_height);
	if (0 != make_pixmap_data(w, &w->tree_table.pix_leaf))
		return -1;

	if (w->tree_table.pix_leaf_open.bitmap == XtUnspecifiedPixmap)
		w->tree_table.pix_leaf_open.bitmap = XCreateBitmapFromData(XtDisplay((Widget)w), RootWindowOfScreen(XtScreen((Widget)w)), (char *)document_bits, document_width, document_height);
	if (0 != make_pixmap_data(w, &w->tree_table.pix_leaf_open))
		return -1;

	w->tree_table.pix_width = w->tree_table.pix_branch_closed.width;
	if (w->tree_table.pix_branch_open.width > w->tree_table.pix_width)
		w->tree_table.pix_width = w->tree_table.pix_branch_open.width;
	if (w->tree_table.pix_leaf.width > w->tree_table.pix_width)
		w->tree_table.pix_width = w->tree_table.pix_leaf.width;
	if (w->tree_table.pix_leaf_open.width > w->tree_table.pix_width)
		w->tree_table.pix_width = w->tree_table.pix_leaf_open.width;
	w->tree_table.pix_branch_closed.xoff = (w->tree_table.pix_width - w->tree_table.pix_branch_closed.width) / 2;
	w->tree_table.pix_branch_open.xoff = (w->tree_table.pix_width - w->tree_table.pix_branch_open.width) / 2;
	w->tree_table.pix_leaf.xoff = (w->tree_table.pix_width - w->tree_table.pix_leaf.width) / 2;
	w->tree_table.pix_leaf_open.xoff = (w->tree_table.pix_width - w->tree_table.pix_leaf_open.width) / 2;
	return 0;
}

int make_pixmap_data(XmTreeTableWidget w, Pixinfo *pix)
{
	Window root;
	int x, y;
	unsigned int width, height, bw, depth;

	if (pix->bitmap && XGetGeometry(XtDisplay((Widget)w), pix->bitmap, &root, &x, &y, &width, &height, &bw, &depth)) {
		pix->width = (int)width;
		pix->height = (int)height;
		if (pix->height > w->tree_table.n_max_pixmap_height)
			w->tree_table.n_max_pixmap_height = pix->height;

		/* Xmu dependency removed by Alan Marcinkowski */
		if (depth == 1) {
			GC gc;
			XGCValues gcv;

			gcv.background = w->core.background_pixel;
			gcv.foreground = w->tree_table.foreground_pixel;
			gc = XCreateGC(XtDisplay((Widget)w), RootWindowOfScreen(XtScreen((Widget)w)), GCForeground | GCBackground, &gcv);
			pix->pix = XCreatePixmap(XtDisplay((Widget)w), RootWindowOfScreen(XtScreen((Widget)w)), width, height, w->core.depth);
			XCopyPlane(XtDisplay((Widget)w), pix->bitmap, pix->pix, gc, 0, 0, width, height, 0, 0, 1);
			XFreeGC(XtDisplay((Widget)w), gc);
		}
		else
			pix->pix = pix->bitmap;
		return 0;
	}
	else {
		pix->width = pix->height = 0;
		pix->pix = 0;
		return -1;
	}
}

int free_pixmap_data(XmTreeTableWidget w, Pixinfo *pix)
{
	if (pix->pix) {
		XFreePixmap(XtDisplay((Widget)w), pix->pix);
	}
	pix->width = pix->height = 0;
	pix->pix = 0;
	return 0;
}

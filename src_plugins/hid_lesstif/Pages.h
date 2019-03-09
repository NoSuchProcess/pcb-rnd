/*
 * Pages widget for Motif
 *
 * Copyright (c) 1987-2012, The Open Group. All rights reserved.
 * Copyright (c) 2019, Tibor 'Igor2' Palinkas
 * (widget code based on Exm Grid)
 *
 * These libraries and programs are free software; you can
 * redistribute them and/or modify them under the terms of the GNU
 * Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * These libraries and programs are distributed in the hope that
 * they will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with these librararies and programs; if not, write
 * to the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301 USA
 */


#ifndef _PxmPages_h
#define _PxmPages_h


#include <Xm/Xm.h> /* widget public header file for XmManager */

#ifdef __cplusplus
extern "C" {
#endif


/* Define the widget class and widget record. */
externalref WidgetClass pxmPagesWidgetClass;

typedef struct _PxmPagesClassRec *PxmPagesWidgetClass;
typedef struct _PxmPagesRec *PxmPagesWidget;


/* Define an IsSubclass macro. */
#ifndef PxmIsPages
#define PxmIsPages(w) XtIsSubclass(w, pxmPagesWidgetClass)
#endif


/* Define string equivalents of new resource names. */
#define PxmNpagesAt "pagesAt"
#define PxmCPagesAt "PagesAt"

/* Specify the API for this widget. */
extern Widget PxmCreatePages(Widget parent, char *name, ArgList arglist, Cardinal argcount);

#ifdef __cplusplus
}
#endif

#endif /* _PxmPages_h */


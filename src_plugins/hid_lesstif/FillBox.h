/*
 * FillBox widget for Motif
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


#ifndef _PxmFillBox_h
#define _PxmFillBox_h


#include <Xm/Xm.h> /* widget public header file for XmManager */

#ifdef __cplusplus
extern "C" {
#endif


/* Define the widget class and widget record. */
externalref WidgetClass pxmFillBoxWidgetClass;

typedef struct _PxmFillBoxClassRec *PxmFillBoxWidgetClass;
typedef struct _PxmFillBoxRec *PxmFillBoxWidget;


/* Define an IsSubclass macro. */
#ifndef PxmIsFillBox
#define PxmIsFillBox(w) XtIsSubclass(w, pxmFillBoxWidgetClass)
#endif


/* Define string equivalents of new resource names. */
#define PxmNfillBoxFill     "fillBoxFill"
#define PxmNfillBoxVertical "fillBoxVertical"

#define PxmCFillBoxFill     "FillBoxFill"
#define PxmCFillBoxVertical "FillBoxVertical"

/* Specify the API for this widget. */
extern Widget PxmCreateFillBox(Widget parent, char *name, ArgList arglist, Cardinal argcount);

#ifdef __cplusplus
}
#endif

#endif /* _PxmFillBox_h */


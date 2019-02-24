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

#ifndef _PxmFillBoxP_h
#define _PxmFillBoxP_h


#include "FillBox.h" /* public header file for PxmFillBox */
#include <Xm/ManagerP.h> /* private header file for XmManager */


#ifdef __cplusplus
extern "C" {
#endif


/* Make the following two methods inheritable by subclasses of PxmFillBox. */
#define PxmInheritLayout   ((PxmLayoutProc) _XtInherit)
#define PxmInheritCalcSize ((PxmCalcSizeProc) _XtInherit)
#define PxmInheritNeedRelayout ((PxmNeedRelayoutProc) _XtInherit)


/* Define new data types for these two inheritable methods. */
typedef void (*PxmLayoutProc) (Widget, Widget);
typedef void (*PxmCalcSizeProc) (Widget, Widget, Dimension *, Dimension *);
typedef Boolean(*PxmNeedRelayoutProc) (Widget, Widget);

/* Define the widget class part. */
typedef struct {
	PxmLayoutProc layout;
	PxmCalcSizeProc calc_size;
	PxmNeedRelayoutProc need_relayout;
	XtPointer extension;
} PxmFillBoxClassPart;


/* Define the full class record. */
typedef struct _PxmFillBoxClassRec {
	CoreClassPart core_class;
	CompositeClassPart composite_class;
	ConstraintClassPart constraint_class;
	XmManagerClassPart manager_class;
	PxmFillBoxClassPart fillBox_class;
} PxmFillBoxClassRec;

externalref PxmFillBoxClassRec pxmFillBoxClassRec;


/* Define the widget instance part. */
typedef struct {
	/* Provide space for the values of the four resources of PxmFillBox. */
	Dimension margin_width;
	Dimension margin_height;
	XtCallbackList map_callback;
	XtCallbackList unmap_callback;
	Boolean vertical;
#ifdef HAVE_XM_TRAIT
	XmRenderTable button_render_table;
	XmRenderTable label_render_table;
	XmRenderTable text_render_table;
#endif

	/* processing_constraints is a flag.  If its value is True, then 
	   it means that the ConstraintSetValues method is requesting a 
	   geometry change. */
	Boolean processing_constraints;

} PxmFillBoxPart;

/* Establish an arbitrary limit */
#define EXM_GRID_MAX_NUMBER_OF_ROWS 100
#define EXM_GRID_MAX_NUMBER_OF_COLUMNS 100

/* Define the full instance record. */
typedef struct _PxmFillBoxRec {
	CorePart core;
	CompositePart composite;
	ConstraintPart constraint;
	XmManagerPart manager;
	PxmFillBoxPart fillBox;
} PxmFillBoxRec;

/* Define the subclassing level index to be used with ResolvePartOffset */
#define PxmFillBoxIndex (XmManagerIndex + 1)

/* Define the constraint part structure. */
typedef struct _PxmFillBoxConstraintPart {
	Boolean fill;
} PxmFillBoxConstraintPart, *PxmFillBoxConstraint;


/* Define the full constraint structure. */
typedef struct _PxmFillBoxConstraintRec {
	XmManagerConstraintPart manager;
	PxmFillBoxConstraintPart fillBox;
} PxmFillBoxConstraintRec, *PxmFillBoxConstraintPtr;


/* Define macros for this class. */
#define PxmFillBoxCPart(w) \
        (&((PxmFillBoxConstraintPtr) (w)->core.constraints)->fillBox)



#ifdef __cplusplus
}
#endif

#endif /* _PxmFillBoxP_h */

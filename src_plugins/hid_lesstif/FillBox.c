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

/******************************************************************************
 *
 * FillBox.c - PxmFillBox widget.  This widget manages its children in
 *             a horizontal or vertical box. Any of the child widget
 *             may have the fill flag set to get a share of excess space.
 *
******************************************************************************/

#include "config.h"

/* Include appropriate header files. */
#include "FillBoxP.h" /* private header file for the PxmFillBox widget */
#include <Xm/GadgetP.h> /* for gadget management functions */

#ifdef HAVE_XM_TRAIT
#include <Xm/TraitP.h> /* for trait access functions */
#include <Xm/DialogSavvyT.h> /* for XmQTdialogSavvy trait */
#include <Xm/SpecRenderT.h> /* for XmQTspecifyRenderTable trait */
#endif

#include "pxm_helper.h"

/* Define macros. */
#define Max(x, y) (((x) > (y)) ? (x) : (y))
#define WARNING_TOO_MANY_ROWS "Too many rows specified for PxmFillBox.\n"
#define WARNING_TOO_MANY_COLUMNS "Too many columns specified for PxmFillBox.\n"


/* Declare static functions. */
static void ClassPartInitialize(WidgetClass widgetClass);
static void Initialize(Widget request_w, Widget new_w, ArgList args, Cardinal *num_args);
static void Destroy(Widget wid);
static void Resize(Widget w);
static void Redisplay(Widget w, XEvent *event, Region region);
static Boolean SetValues(Widget old_w, Widget request_w, Widget new_w, ArgList args, Cardinal *num_args);
static void SetValuesAlmost(Widget cw, Widget nw, XtWidgetGeometry *request, XtWidgetGeometry *reply);
static XtGeometryResult QueryGeometry(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *reply);
static XtGeometryResult GeometryManager(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *reply);
static void ChangeManaged(Widget w);
static Boolean ConstraintSetValues(Widget cw, Widget rw, Widget nw, ArgList args, Cardinal *num_args);
static void Layout(Widget wid, Widget instigator);
static void CalcSize(Widget wid, Widget instigator, Dimension *fillBoxWidth, Dimension *fillBoxHeight);
static Boolean NeedRelayout(Widget new, Widget cur);

#ifdef HAVE_XM_TRAIT
static void CallMapUnmap(Widget wid, Boolean map_unmap);
static XmRenderTable GetTable(Widget wid, XtEnum type);
#endif

/* No translations and no actions. */


/* Define the resources for the PxmFillBox widget. */
static XtResource resources[] = {
	{
	 XmNmarginWidth,
	 XmCMarginWidth,
	 XmRHorizontalDimension,
	 sizeof(Dimension),
	 XtOffsetOf(PxmFillBoxRec, fillBox.margin_width),
	 XmRImmediate,
	 (XtPointer) 10}
	,
	{
	 XmNmarginHeight,
	 XmCMarginHeight,
	 XmRVerticalDimension,
	 sizeof(Dimension),
	 XtOffsetOf(PxmFillBoxRec, fillBox.margin_height),
	 XmRImmediate,
	 (XtPointer) 10}
	,
	{
	 XmNmapCallback,
	 XmCCallback,
	 XmRCallback,
	 sizeof(XtCallbackList),
	 XtOffsetOf(PxmFillBoxRec, fillBox.map_callback),
	 XmRImmediate,
	 (XtPointer) NULL}
	,
	{
	 XmNunmapCallback,
	 XmCCallback,
	 XmRCallback,
	 sizeof(XtCallbackList),
	 XtOffsetOf(PxmFillBoxRec, fillBox.unmap_callback),
	 XmRImmediate,
	 (XtPointer) NULL}
	,
	{
	 PxmNfillBoxVertical,
	 PxmCFillBoxVertical,
	 XmRBoolean, sizeof(Boolean),
	 XtOffsetOf(PxmFillBoxRec, fillBox.vertical),
	 XmRImmediate,
	 (XtPointer) False}
	,
#ifdef HAVE_XM_TRAIT
	{
	 XmNbuttonRenderTable,
	 XmCButtonRenderTable,
	 XmRButtonRenderTable,
	 sizeof(XmRenderTable),
	 XtOffsetOf(PxmFillBoxRec, fillBox.button_render_table),
	 XmRCallProc, (XtPointer) NULL}
	,
	{
	 XmNlabelRenderTable,
	 XmCLabelRenderTable,
	 XmRLabelRenderTable,
	 sizeof(XmRenderTable),
	 XtOffsetOf(PxmFillBoxRec, fillBox.label_render_table),
	 XmRCallProc, (XtPointer) NULL}
	,
	{
	 XmNtextRenderTable,
	 XmCTextRenderTable,
	 XmRTextRenderTable,
	 sizeof(XmRenderTable),
	 XtOffsetOf(PxmFillBoxRec, fillBox.text_render_table),
	 XmRCallProc, (XtPointer) NULL}
	,
#endif
};

/* Three of the preceding resources will be handled as synthetic 
   resources. */
static XmSyntheticResource syn_resources[] = {
#if HAVE_XM_SYNTHETIC_RESOURCE
	{
	 XmNmarginWidth,
	 sizeof(Dimension),
	 XtOffsetOf(PxmFillBoxRec, fillBox.margin_width),
	 XmeFromHorizontalPixels,
	 XmeToHorizontalPixels}
	,
	{
	 XmNmarginHeight,
	 sizeof(Dimension),
	 XtOffsetOf(PxmFillBoxRec, fillBox.margin_height),
	 XmeFromVerticalPixels,
	 XmeToVerticalPixels}
	,
#endif
0
};


/* Define the two constraints of PxmFillBox. */
static XtResource constraints[] = {
	{
	 PxmNfillBoxFill,
	 PxmCFillBoxFill,
	 XmRPacking,
	 sizeof(Boolean),
	 XtOffsetOf(PxmFillBoxConstraintRec, fillBox.fill),
	 XmRImmediate,
	 (XtPointer) 0}
	,
};

/* The preceding constraint will be handled as synthetic constraint. */
static XmSyntheticResource syn_constraints[] = {
#if 0
	{
	 PxmNfillBoxFill,
	 sizeof(Boolean),
	 XtOffsetOf(PxmFillBoxConstraintRec, fillBox.fill),
	 XmeFromHorizontalPixels,
	 XmeToHorizontalPixels}
	,
#endif
0
};



/* Define the widget class record.  See Chapter 4 of the 
   "OSF/Motif Widget Writer's Guide" for details. */
externaldef(pxmfillBoxclassrec)
		 PxmFillBoxClassRec pxmFillBoxClassRec = {
			 { /* Here is the Core class record. */
				/* superclass */ (WidgetClass) & xmManagerClassRec,
				/* class_name */ "PxmFillBox",
				/* widget_size */ sizeof(PxmFillBoxRec),
				/* class_initialize */ NULL,
				/* class_part_initialize */ ClassPartInitialize,
				/* class_inited */ FALSE,
				/* initialize */ Initialize,
				/* initialize_hook */ NULL,
				/* realize */ XtInheritRealize,
				/* actions */ NULL,
				/* num_actions */ 0,
				/* resources */ resources,
				/* num_resources */ XtNumber(resources),
				/* xrm_class */ NULLQUARK,
				/* compress_motion */ TRUE,
				/* compress_exposure */ XtExposeCompressMaximal,
				/* compress_enterleave */ TRUE,
				/* visible_interest */ FALSE,
				/* destroy */ Destroy,
				/* resize */ Resize,
				/* expose */ Redisplay,
				/* set_values */ SetValues,
				/* set_values_hook */ NULL,
				/* set_values_almost */ SetValuesAlmost,
				/* get_values_hook */ NULL,
				/* accept_focus */ NULL,
				/* version */ XtVersion,
				/* callback_private */ NULL,
				/* tm_table */ XtInheritTranslations,
				/* query_geometry */ QueryGeometry,
				/* display_accelerator */ NULL,
				/* extension */ NULL,
				}
			 ,
			 { /* Here is the Composite class record. */
				/* geometry_manager */ GeometryManager,
				/* change_managed */ ChangeManaged,
				/* insert_child */ XtInheritInsertChild,
				/* delete_child */ XtInheritDeleteChild,
				/* extension */ NULL,
				}
			 ,
			 { /* Here is the Constaint class record. */
				/* constraint_resources */ constraints,
				/* constraint_num_resources */ XtNumber(constraints),
				/* constraint_size */ sizeof(PxmFillBoxConstraintRec),
				/* constraint_initialize */ NULL,
				/* constraint_destroy */ NULL,
				/* constraint_set_values */ ConstraintSetValues,
				/* extension */ NULL,
				}
			 ,
			 { /* Here is the XmManager class record. */
				/* translations */ XtInheritTranslations,
				/* syn_resources */ syn_resources,
				/* num_syn_resources */ XtNumber(syn_resources),
				/* syn_constraint_resources */ syn_constraints,
				/* num_syn_constraint_resources */ XtNumber(syn_constraints),
				/* parent_process */ XmInheritParentProcess,
				/* extension */ NULL,
				}
			 ,
			 { /* Here is the PxmFillBox class record. */
				/* layout */ Layout,
				/* calc_size */ CalcSize,
				/* need_relayout */ NeedRelayout,
				/* extension */ NULL,
				}
		 };

/* Establish the widget class name as an externally accessible symbol.
   Use the "externaldef" macro rather than the "extern" keyword. */
externaldef(pxmfillBoxwidgetclass) WidgetClass pxmFillBoxWidgetClass = (WidgetClass) & pxmFillBoxClassRec;

#ifdef HAVE_XM_TRAIT
/* Define trait record variables. */

/* Here is the trait record variable for the XmQTdialogSavvy trait. */
static XmConst XmDialogSavvyTraitRec fillBoxDST = {
	0, /* version */
	CallMapUnmap, /* trait method */
};


/* Here is the trait record variable for the XmQTspecifyRenderTable trait. */
static XmConst XmSpecRenderTraitRec fillBoxSRTT = {
	0, /* version */
	GetTable, /* trait method */
};
#endif


/****************************************************************************
 *
 *  ClassPartInitialize:
 *      Called when this widget or a subclass of this widget is instantiated.
 *
 ****************************************************************************/
static void ClassPartInitialize(WidgetClass widgetClass)
{
	PxmFillBoxWidgetClass wc = (PxmFillBoxWidgetClass) widgetClass;
	PxmFillBoxWidgetClass sc = (PxmFillBoxWidgetClass) wc->core_class.superclass;

	/* The following code allows subclasses of PxmFillBox to inherit three of  
	   PxmFillBox's methods. */
	if (wc->fillBox_class.layout == PxmInheritLayout)
		wc->fillBox_class.layout = sc->fillBox_class.layout;
	if (wc->fillBox_class.calc_size == PxmInheritCalcSize)
		wc->fillBox_class.calc_size = sc->fillBox_class.calc_size;
	if (wc->fillBox_class.need_relayout == PxmInheritNeedRelayout)
		wc->fillBox_class.need_relayout = sc->fillBox_class.need_relayout;

#ifdef HAVE_XM_TRAIT
	/* Install the XmQTdialogShellSavyy trait on this class and on
	   all its future subclasses. */
	XmeTraitSet(widgetClass, XmQTdialogShellSavvy, (XtPointer) & fillBoxDST);

	/* Install the XmQTspecifyRenderTable trait on this class and on
	   all its future subclasses. */
	XmeTraitSet(widgetClass, XmQTspecifyRenderTable, (XtPointer) & fillBoxSRTT);
#endif
}


/**************************************************************************
 *
 *  Initialize:
 *      Called when this widget is first instantiated.
 *
 ***************************************************************************/
static void Initialize(Widget request_w, Widget new_w, ArgList args, Cardinal *num_args)
{
/*	PxmFillBoxWidget rw = (PxmFillBoxWidget) request_w;*/
	PxmFillBoxWidget nw = (PxmFillBoxWidget) new_w;

	/* Initialize internal fields of the PxmFillBox widget. */
	nw->fillBox.processing_constraints = False;
	nw->core.width = nw->core.height = 0;
	nw->fillBox.margin_width = nw->fillBox.margin_height = 0;
}


/****************************************************************************
 *
 *  Destroy:
 *      Called when the widget is destroyed.
 *
 ****************************************************************************/
static void Destroy(Widget wid)
{
/*	PxmFillBoxWidget fillBox = (PxmFillBoxWidget) wid;*/
}


/****************************************************************************
 *
 *  Resize:
 *
 ****************************************************************************/
static void Resize(Widget w)
{
	PxmFillBoxWidgetClass gwc = (PxmFillBoxWidgetClass) XtClass(w);

	/* Configure the children by calling Layout. */
	if (gwc->fillBox_class.layout)
		(*(gwc->fillBox_class.layout)) (w, NULL);
	else
		Layout(w, NULL);
}




/****************************************************************************
 *
 *  Redisplay:
 *      Called by the Intrinsics in response to an exposure event.
 *
 ***************************************************************************/
static void Redisplay(Widget w, XEvent *event, Region region)
{
	/* Pass exposure event down to gadget children. */
	PxmRedisplayGadgets(w, event, region);
}



/*****************************************************************************
 *
 *  SetValues:
 *      Called by the Intrinsics whenever any of the resource values change.
 *
 ****************************************************************************/
static Boolean SetValues(Widget old_w, Widget request_w, Widget new_w, ArgList args, Cardinal *num_args)
{
	PxmFillBoxWidget cw = (PxmFillBoxWidget) old_w;
/*	PxmFillBoxWidget rw = (PxmFillBoxWidget) request_w;*/
	PxmFillBoxWidget nw = (PxmFillBoxWidget) new_w;
	Boolean redisplay = False;
	Boolean need_relayout;
	PxmFillBoxWidgetClass gwc = (PxmFillBoxWidgetClass) XtClass(new_w);

	/* See if any class or subclass resources have changed. */
	if (gwc->fillBox_class.need_relayout)
		need_relayout = (*(gwc->fillBox_class.need_relayout)) (old_w, new_w);
	else
		need_relayout = NeedRelayout(old_w, new_w);

	/* If any geometry resources changed and a new size wasn't specified,
	   recalculate a new ideal size. */
	if (need_relayout) {
		/* Reset the widget size so that CalcSize can affect them. */
		if (nw->core.width == cw->core.width)
			nw->core.width = 0;
		if (nw->core.height == cw->core.height)
			nw->core.height = 0;

		/* Call CalcSize. */
		if (gwc->fillBox_class.calc_size)
			(*(gwc->fillBox_class.calc_size)) (new_w, NULL, &nw->core.width, &nw->core.height);
		else
			CalcSize(new_w, NULL, &nw->core.width, &nw->core.height);


		/* If the geometry resources have changed but the size hasn't, 
		   we need to relayout manually, because Xt won't generate a 
		   Resize at this point. */
		if ((nw->core.width == cw->core.width) && (nw->core.height == cw->core.height)) {

			/* Call Layout to configure the children. */
			if (gwc->fillBox_class.layout)
				(*(gwc->fillBox_class.layout)) (new_w, NULL);
			else
				Layout(new_w, NULL);
			redisplay = True;
		}
	}

#ifdef HAVE_XM_TRAIT
	/* PxmFillBox installs the XmQTdialogShellSavvy trait.  Therefore, PxmFillBox
	   has to process the Xm_DIALOG_SAVVY_FORCE_ORIGIN case, which is as
	   follows.  A DialogShell always mimics the child position on itself.
	   That is, the "current" position of an PxmFillBox within a DialogShell is
	   always 0.  Therefore, if an application tries to set PxmFillBox's x or
	   y position to 0, the Intrinsics will not detect a position change and
	   wll not trigger a geometry request.  PxmFillBox has to detect this special 
	   request and set core.x and core.y to the special value, 
	   XmDIALOG_SAVVY_FORCE_ORIGIN.  That is, XmDIALOG_SAVVY_FORCE_ORIGIN
	   tells DialogShell that PxmFillBox really does want to move to an x or y
	   position of 0. */

	if (XmIsDialogShell(XtParent(new_w))) { /* Is parent a DialogShell? */
		Cardinal i;

		/* We have to look in the arglist since old_w->core.x is always 0, and 
		   if new_w->core.x is also set to 0, we see no change. */
		for(i = 0; i < *num_args; i++) {
			if (strcmp(args[i].name, XmNx) == 0) {
				if ((args[i].value == 0) && (new_w->core.x == 0))
					new_w->core.x = XmDIALOG_SAVVY_FORCE_ORIGIN;
			}
			if (strcmp(args[i].name, XmNy) == 0) {
				if ((args[i].value == 0) && (new_w->core.y == 0))
					new_w->core.y = XmDIALOG_SAVVY_FORCE_ORIGIN;
			}
		} /* end for */
	} /* end of if */
#endif

	return (redisplay);
}



/*************************************************************************
 *
 *  SetValuesAlmost:
 *       Called by the Intrinsics when an XtMakeGeometryRequest call
 *       returns either XmGeometryAlmost or XtGeometryNo.  
 *
 ***************************************************************************/
static void SetValuesAlmost(Widget cw, /* unused */
														Widget nw, XtWidgetGeometry *request, XtWidgetGeometry *reply)
{
	PxmFillBoxWidgetClass gwc = (PxmFillBoxWidgetClass) XtClass(nw);

	/* PxmFillBox's parent said XtGeometryNo to PxmFillBox's geometry request. 
	   Therefore, we need to relayout because this request
	   was due to a change in internal geometry resource of the PxmFillBox */
	if (!reply->request_mode) {
		if (gwc->fillBox_class.layout)
			(*(gwc->fillBox_class.layout)) (nw, NULL);
		else
			Layout(nw, NULL);
	}

	*request = *reply;
}


/*************************************************************************
 *
 *  QueryGeometry:
 *       Called by a parent of FillBox when the parent needs to find out FillBox's
 *       preferred size.  QueryGeometry calls CalcSize to do find the
 *       preferred size. 
 *
 ***************************************************************************/
static XtGeometryResult QueryGeometry(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *reply)
{
	PxmFillBoxWidgetClass gwc = (PxmFillBoxWidgetClass) XtClass(w);

	/* If PxmFillBox's parent calls XtQueryGeometry before PxmFillBox has been 
	   realized, use the current size of PxmFillBox as the preferred size. */
	/* Deal with user initial size setting */
	if (!XtIsRealized(w)) { /* Widget is not yet realized. */
		reply->width = XtWidth(w); /* might be 0 */
		reply->height = XtHeight(w); /* might be 0 */
	}
	else { /* Widget is realized. */
		/* always computes natural size afterwards */
		reply->width = 0;
		reply->height = 0;
	}

	/* Call CalcSize to figure out what the preferred size really is. */
	if (gwc->fillBox_class.calc_size)
		(*(gwc->fillBox_class.calc_size)) (w, NULL, &reply->width, &reply->height);
	else
		CalcSize(w, NULL, &reply->width, &reply->height);

	/* This function handles CWidth and CHeight */
	return PxmReplyToQueryGeometry(w, request, reply);
}


/****************************************************************************
 *
 *  GeometryManager:
 *       Called by Intrinsics in response to a geometry change request from 
 *       one of the children of PxmFillBox.
 *
 ***************************************************************************/
static XtGeometryResult GeometryManager(Widget w, /* instigator */
																				XtWidgetGeometry *request, XtWidgetGeometry *reply)
{
	PxmFillBoxWidget gw = (PxmFillBoxWidget) XtParent(w);
	XtWidgetGeometry parentRequest;
	XtGeometryResult result;
	Dimension curWidth, curHeight, curBW;
	PxmFillBoxWidgetClass gwc = (PxmFillBoxWidgetClass) XtClass((Widget) gw);

	/* If the request was caused by ConstraintSetValues reset the flag */
	if (gw->fillBox.processing_constraints) {
		gw->fillBox.processing_constraints = False;
		/* The ConstraintSetValues added one to border_width; 
		   This is the Xt trick used to fire the GM when a non core 
		   geometry resource (like a constraint) changes.
		   now take it away. */
		request->border_width -= 1;
	}

	/* Save the original child resources. */
	curWidth = w->core.width;
	curHeight = w->core.height;
	curBW = w->core.border_width;

	/* Deny any requests for a new position. */
	if ((request->request_mode & CWX) || (request->request_mode & CWY))
		return XtGeometryNo;

	if (request->request_mode & CWWidth)
		w->core.width = request->width;
	if (request->request_mode & CWHeight)
		w->core.height = request->height;
	if (request->request_mode & CWBorderWidth)
		w->core.border_width = request->border_width;

	/* Calculate a new ideal size based on these requests. */
	/* Setting width and height to 0 tells CalcSize to override these
	   fields with the calculated width and height. */
	parentRequest.width = 0;
	parentRequest.height = 0;
	if (gwc->fillBox_class.calc_size)
		(*(gwc->fillBox_class.calc_size)) ((Widget) gw, w, &parentRequest.width, &parentRequest.height);
	else
		CalcSize((Widget) gw, w, &parentRequest.width, &parentRequest.height);


	/* Ask the FillBox's parent if new calculated size is acceptable. */
	parentRequest.request_mode = CWWidth | CWHeight;
	if (request->request_mode & XtCWQueryOnly)
		parentRequest.request_mode |= XtCWQueryOnly;
	result = XtMakeGeometryRequest((Widget) gw, &parentRequest, NULL);

	/*  Turn XtGeometryAlmost into XtGeometryNo. */
	if (result == XtGeometryAlmost)
		result = XtGeometryNo;

	if (result == XtGeometryNo || request->request_mode & XtCWQueryOnly) {
		/* Restore original geometry. */
		w->core.width = curWidth;
		w->core.height = curHeight;
		w->core.border_width = curBW;
	}
	else {
		/* result == XtGeometryYes and this wasn't just a query */
		if (gwc->fillBox_class.layout)
			(*(gwc->fillBox_class.layout)) ((Widget) gw, w);
		else
			Layout((Widget) gw, w); /* Layout with this child as the instigator,
														    so that we don't resize this child. */
	}

	return (result);
}



/**************************************************************************
 *
 *  ChangeManaged:
 *      Called by the Intrinsics whenever either of the following happens:
 *           * a managed child becomes unmanaged.
 *           * an unmanaged child becomes managed.
 *
 *************************************************************************/
static void ChangeManaged(Widget w)
{
	Dimension fillBoxWidth, fillBoxHeight;
	PxmFillBoxWidgetClass gwc = (PxmFillBoxWidgetClass) XtClass(w);

	/* If you get an initial (C) size from the user or application, keep it.  
	   Otherwise, just force width and height to 0 so that CalcSize will
	   overwrite the appropriate fields. */
	if (!XtIsRealized(w)) {
		/* The first time, only attempts to change non specified sizes */
		fillBoxWidth = XtWidth(w); /* might be 0 */
		fillBoxHeight = XtHeight(w); /* might be 0 */
	}
	else {
		fillBoxWidth = 0;
		fillBoxHeight = 0;
	}

	/* Determine the ideal size of FillBox. */
	if (gwc->fillBox_class.calc_size)
		(*(gwc->fillBox_class.calc_size)) (w, NULL, &fillBoxWidth, &fillBoxHeight);
	else
		CalcSize(w, NULL, &fillBoxWidth, &fillBoxHeight);

	/* Ask parent of FillBox if FillBox's new size is acceptable.  Keep asking until
	   parent returns either XtGeometryYes or XtGeometryNo. */
	while(XtMakeResizeRequest(w, fillBoxWidth, fillBoxHeight, &fillBoxWidth, &fillBoxHeight) == XtGeometryAlmost);

	/* Now that we have a size for the FillBox, we can layout the children
	   of the fillBox. */
	if (gwc->fillBox_class.layout)
		(*(gwc->fillBox_class.layout)) (w, NULL);
	else
		Layout(w, NULL);

#ifdef HAVE_XME
	/* Update keyboard traversal */
	XmeNavigChangeManaged(w);
#endif
}




/**************************************************************************
 *
 *  ConstraintSetValues:
 *      Called by Intrinsics if there is any change in any of the constraint
 *      resources. 
 *
 **************************************************************************/
static Boolean ConstraintSetValues(Widget cw, Widget rw, Widget nw, ArgList args, Cardinal *num_args)
{
	PxmFillBoxConstraint nc;
	PxmFillBoxConstraint cc;
	PxmFillBoxWidget gw;

	if (!XtIsRectObj(nw))
		return (False);

	gw = (PxmFillBoxWidget) XtParent(nw);
	nc = PxmFillBoxCPart(nw);
	cc = PxmFillBoxCPart(cw);

	/* Check for change in PxmNfillBoxMarginWidth or PxmNfillBoxMarginHeight */
	if ((nc->fill != cc->fill) && XtIsManaged(nw)) {
		/* Tell the Intrinsics and the GeometryManager method that a 
		   reconfigure is needed. */
		gw->fillBox.processing_constraints = True;
		/* A trick: by altering one of the core geometry fields, Xt will
		   call the parent's geometry_manager method. */
		nw->core.border_width += 1;
	}

	return (False);
}


/***************************************************************************
 *
 *  Layout:
 *     Does all the placement of children.
 *     Instigator tells whether or not to resize all children.
 *
 *************************************************************************/
static void Layout(Widget wid, Widget instigator)
{
	PxmFillBoxWidget gw = (PxmFillBoxWidget) wid;
	Dimension mw = gw->fillBox.margin_width;
	Dimension mh = gw->fillBox.margin_height;
	Dimension TotalWidthOfFillBoxWidget = gw->core.width;
	Dimension TotalHeightOfFillBoxWidget = gw->core.height;
	long AvailWidthForChildren, AvailHeightForChildren;
	long pos, extra = 0, total = 0, wextra = 0;
	unsigned int i;
	int numfills, vert = gw->fillBox.vertical;

	AvailWidthForChildren = (long)TotalWidthOfFillBoxWidget - 2 * (long)mw;
	AvailHeightForChildren = (long)TotalHeightOfFillBoxWidget - 2 * (long)mh;


	/* calculate the extra space we need to allocate and reset width/height of each widget temporarily */
	total = 0;
	numfills = 0;
	for(i = 0; i < gw->composite.num_children; i++) {
		Widget ic = gw->composite.children[i];
		Dimension cb = ic->core.border_width;
		PxmFillBoxConstraint glc = PxmFillBoxCPart(ic);
		Boolean gmf = glc->fill;
		Dimension cw, ch;
		XtWidgetGeometry reply;

		if (!XtIsManaged(ic))
			continue;

		XtQueryGeometry(ic, NULL, &reply);
		cw = (reply.request_mode & CWWidth) ? reply.width : ic->core.width;
		ch = (reply.request_mode & CWHeight) ? reply.height : ic->core.height;
		ic->core.width = cw;
		ic->core.height = ch;

		if (vert)
			total += ch + 2*cb;
		else
			total += cw + 2*cb;

/*printf("[%d] %d %d cw=%d\n", i, gmf, ic->core.width + 2*cb, cw);*/
		if (gmf)
			numfills++;
	}
	if (numfills != 0) {
		if (vert)
			extra = AvailHeightForChildren - total;
		else
			extra = AvailWidthForChildren - total;
		if (extra < 0)
			extra = 0;
		wextra = extra / numfills;
	}

	/* printf("avail=%ld:%ld total=%ld extra=%ld numfills=%d wextra=%ld vert=%d\n", AvailWidthForChildren, AvailHeightForChildren, total, extra, numfills, wextra, gw->fillBox.vertical); */

	/* sequential layout, insert extra space in children with fill turned on */
	pos = mw;
	for(i = 0; i < gw->composite.num_children; i++) {
		Widget ic = gw->composite.children[i];
		PxmFillBoxConstraint glc = PxmFillBoxCPart(ic);
		Boolean gmf = glc->fill;
		Position ChildsStartingX, ChildsStartingY;
		Dimension ChildsActualWidth, ChildsActualHeight, cb;

		if (!XtIsManaged(ic))
			continue; /* ignored unmanaged children */

		cb = ic->core.border_width;

		ChildsActualWidth = ic->core.width;
		ChildsActualHeight = ic->core.height;

		if (vert) {
			ChildsStartingX = mw+cb;
			ChildsStartingY = pos+cb;
			ChildsActualWidth = AvailWidthForChildren;
		}
		else {
			ChildsStartingX = pos+cb;
			ChildsStartingY = mh+cb;
			ChildsActualHeight = AvailHeightForChildren;
		}

		if (gmf) {
			numfills--;
			if (numfills > 0) {
				if (vert)
					ChildsActualHeight += wextra;
				else
					ChildsActualWidth += wextra;
				extra -= wextra;
			}
			else { /* the last fill widget gets whatever remains - this fixes rounding errors */
				if (vert)
					ChildsActualHeight += extra;
				else
					ChildsActualWidth += extra;
				extra = 0;
			}
		}

		/* If layout is instigated by the GeometryManager don't 
		   configure the requesting child, just set its geometry and 
		   let Xt configure it.   */
		if (ic != instigator) {
			PxmConfigureObject(ic, ChildsStartingX, ChildsStartingY, ChildsActualWidth, ChildsActualHeight, cb);
		}
		else {
			ic->core.x = ChildsStartingX;
			ic->core.y = ChildsStartingY;
/*			printf(" {%d %d}\n", ic->core.x, ic->core.y);*/
			ic->core.width = ChildsActualWidth;
			ic->core.height = ChildsActualHeight;
			ic->core.border_width = cb;
		}

		if (vert)
			pos += ChildsActualHeight + cb*2;
		else
			pos += ChildsActualWidth + cb*2;
	}
}



/******************************************************************************
 *
 *  CalcSize:
 *     Called by QueryGeometry, SetValues, GeometryManager, and ChangeManaged.
 *     Calculate the ideal size of the PxmFillBox widget. 
 *     Only affects the returned size if it is 0.
 *
 ****************************************************************************/
static void CalcSize(Widget wid, Widget instigator, Dimension *TotalWidthOfFillBoxWidget, Dimension *TotalHeightOfFillBoxWidget)
{
	PxmFillBoxWidget gw = (PxmFillBoxWidget) wid;
	Dimension mw = gw->fillBox.margin_width;
	Dimension mh = gw->fillBox.margin_height;
	Dimension maxWidth = 1;
	Dimension maxHeight = 1;
	unsigned int i;
	int vert = gw->fillBox.vertical;

	/* minimal size is a sequential placement of children plus the margins */
	for(i = 0; i < gw->composite.num_children; i++) {
		Widget ic = gw->composite.children[i];
		Dimension width, height;
		Dimension cw, ch, cb;
		XtWidgetGeometry reply;

		if (!XtIsManaged(ic))
			continue;

		/* Get child's preferred geometry if not the instigator. */
		if (ic != instigator) {
			XtQueryGeometry(ic, NULL, &reply);
			cw = (reply.request_mode & CWWidth) ? reply.width : ic->core.width;
			ch = (reply.request_mode & CWHeight) ? reply.height : ic->core.height;
		}
		else {
			cw = ic->core.width;
			ch = ic->core.height;
		}
		cb = ic->core.border_width;

		width = cw + 2 * cb;
		height = ch + 2 * cb;

		if (vert) {
			maxWidth = Max(width, maxWidth);
			maxHeight += height;
		}
		else {
			maxWidth += width;
			maxHeight = Max(height, maxHeight);
		}
	}

	*TotalWidthOfFillBoxWidget = maxWidth + 2*mw;
	*TotalHeightOfFillBoxWidget = maxHeight + 2*mh;
}


/****************************************************************************
 *
 *  NeedRelayout:
 *     Called by SetValues. 
 *     Returns True if a relayout is needed.  
 *     based on this class and all superclass resources' changes. 
 *
 ***************************************************************************/
static Boolean NeedRelayout(Widget old_w, Widget new_w)
{
#if 0
	PxmFillBoxWidget cw = (PxmFillBoxWidget) old_w;
	PxmFillBoxWidget nw = (PxmFillBoxWidget) new_w;
#endif

	return False;
}


#ifdef HAVE_XM_TRAIT
/*-- Trait methods --*/


/****************************************************************
 *
 * Trait method for XmQTdialogShellSavvy trait. 
 *
 **************************************************************/
static void CallMapUnmap(Widget wid, Boolean map_unmap)
{
	PxmFillBoxWidget fillBox = (PxmFillBoxWidget) wid;
	XmAnyCallbackStruct call_data;

	call_data.reason = (map_unmap) ? XmCR_MAP : XmCR_UNMAP;
	call_data.event = NULL;

	if (map_unmap) {
		XtCallCallbackList(wid, fillBox->fillBox.map_callback, &call_data);
	}
	else {
		XtCallCallbackList(wid, fillBox->fillBox.unmap_callback, &call_data);
	}
}

/*****************************************************************
 *
 * Trait method for XmQTspecifyRenderTable.   
 *
*****************************************************************/

static XmRenderTable GetTable(Widget wid, XtEnum type)
{
	PxmFillBoxWidget fillBox = (PxmFillBoxWidget) wid;

	switch (type) {
		case XmLABEL_RENDER_TABLE:
			return fillBox->fillBox.label_render_table;
		case XmBUTTON_RENDER_TABLE:
			return fillBox->fillBox.button_render_table;
		case XmTEXT_RENDER_TABLE:
			return fillBox->fillBox.text_render_table;
	}

	return NULL;
}
#endif

/*******************************************************************************
 *
 *  PxmCreateFillBox:
 *      Called by an application. 
 *
 ******************************************************************************/
Widget PxmCreateFillBox(Widget parent, char *name, ArgList arglist, Cardinal argcount)
{
	/* This is a convenience function to instantiate an PxmFillBox widget. */
	return (XtCreateWidget(name, pxmFillBoxWidgetClass, parent, arglist, argcount));
}


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

/******************************************************************************
 *
 * Pages.c - PxmPages widget.  This widget manages its children in
 *           a notebook-like setup. Each child is a page, only one page
 *           is shown at a time. There's no tab button system, the caller
 *           needs to set that up.
******************************************************************************/

/* Include appropriate header files. */
#include "PagesP.h" /* private header file for the PxmPages widget */
#include <Xm/GadgetP.h> /* for gadget management functions */

#ifdef HAVE_XM_TRAIT
#include <Xm/TraitP.h> /* for trait access functions */
#include <Xm/DialogSavvyT.h> /* for XmQTdialogSavvy trait */
#include <Xm/SpecRenderT.h> /* for XmQTspecifyRenderTable trait */
#endif

#include "pxm_helper.h"

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
static void CalcSize(Widget wid, Widget instigator, Dimension *pagesWidth, Dimension *pagesHeight);
static Boolean NeedRelayout(Widget new, Widget cur);

#ifdef HAVE_XM_TRAIT
static void CallMapUnmap(Widget wid, Boolean map_unmap);
static XmRenderTable GetTable(Widget wid, XtEnum type);
#endif

/* No translations and no actions. */


/* Define the resources for the PxmPages widget. */
static XtResource resources[] = {
	{
	 XmNmarginWidth,
	 XmCMarginWidth,
	 XmRHorizontalDimension,
	 sizeof(Dimension),
	 XtOffsetOf(PxmPagesRec, pages.margin_width),
	 XmRImmediate,
	 (XtPointer) 10}
	,
	{
	 XmNmarginHeight,
	 XmCMarginHeight,
	 XmRVerticalDimension,
	 sizeof(Dimension),
	 XtOffsetOf(PxmPagesRec, pages.margin_height),
	 XmRImmediate,
	 (XtPointer) 10}
	,
	{
	 XmNmapCallback,
	 XmCCallback,
	 XmRCallback,
	 sizeof(XtCallbackList),
	 XtOffsetOf(PxmPagesRec, pages.map_callback),
	 XmRImmediate,
	 (XtPointer) NULL}
	,
	{
	 XmNunmapCallback,
	 XmCCallback,
	 XmRCallback,
	 sizeof(XtCallbackList),
	 XtOffsetOf(PxmPagesRec, pages.unmap_callback),
	 XmRImmediate,
	 (XtPointer) NULL}
	,
	{
	 PxmNpagesAt,
	 PxmCPagesAt,
	 XmRCardinal, sizeof(Cardinal),
	 XtOffsetOf(PxmPagesRec, pages.at),
	 XmRImmediate,
	 (XtPointer) False}
	,
#ifdef HAVE_XM_TRAIT
	{
	 XmNbuttonRenderTable,
	 XmCButtonRenderTable,
	 XmRButtonRenderTable,
	 sizeof(XmRenderTable),
	 XtOffsetOf(PxmPagesRec, pages.button_render_table),
	 XmRCallProc, (XtPointer) NULL}
	,
	{
	 XmNlabelRenderTable,
	 XmCLabelRenderTable,
	 XmRLabelRenderTable,
	 sizeof(XmRenderTable),
	 XtOffsetOf(PxmPagesRec, pages.label_render_table),
	 XmRCallProc, (XtPointer) NULL}
	,
	{
	 XmNtextRenderTable,
	 XmCTextRenderTable,
	 XmRTextRenderTable,
	 sizeof(XmRenderTable),
	 XtOffsetOf(PxmPagesRec, pages.text_render_table),
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
	 XtOffsetOf(PxmPagesRec, pages.margin_width),
	 XmeFromHorizontalPixels,
	 XmeToHorizontalPixels}
	,
	{
	 XmNmarginHeight,
	 sizeof(Dimension),
	 XtOffsetOf(PxmPagesRec, pages.margin_height),
	 XmeFromVerticalPixels,
	 XmeToVerticalPixels}
	,
#endif
0
};


/* Define the two constraints of PxmPages. */
static XtResource constraints[] = { 0 };

/* The preceding constraint will be handled as synthetic constraint. */
static XmSyntheticResource syn_constraints[] = { 0 };

/* Define the widget class record.  See Chapter 4 of the 
   "OSF/Motif Widget Writer's Guide" for details. */
externaldef(pxmpagesclassrec)
		 PxmPagesClassRec pxmPagesClassRec = {
			 { /* Here is the Core class record. */
				/* superclass */ (WidgetClass) & xmManagerClassRec,
				/* class_name */ "PxmPages",
				/* widget_size */ sizeof(PxmPagesRec),
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
				/* constraint_size */ sizeof(PxmPagesConstraintRec),
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
			 { /* Here is the PxmPages class record. */
				/* layout */ Layout,
				/* calc_size */ CalcSize,
				/* need_relayout */ NeedRelayout,
				/* extension */ NULL,
				}
		 };

/* Establish the widget class name as an externally accessible symbol.
   Use the "externaldef" macro rather than the "extern" keyword. */
externaldef(pxmpageswidgetclass) WidgetClass pxmPagesWidgetClass = (WidgetClass) & pxmPagesClassRec;

#ifdef HAVE_XM_TRAIT
/* Define trait record variables. */

/* Here is the trait record variable for the XmQTdialogSavvy trait. */
static XmConst XmDialogSavvyTraitRec pagesDST = {
	0, /* version */
	CallMapUnmap, /* trait method */
};


/* Here is the trait record variable for the XmQTspecifyRenderTable trait. */
static XmConst XmSpecRenderTraitRec pagesSRTT = {
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
	PxmPagesWidgetClass wc = (PxmPagesWidgetClass) widgetClass;
	PxmPagesWidgetClass sc = (PxmPagesWidgetClass) wc->core_class.superclass;

	/* The following code allows subclasses of PxmPages to inherit three of  
	   PxmPages's methods. */
	if (wc->pages_class.layout == PxmInheritLayout)
		wc->pages_class.layout = sc->pages_class.layout;
	if (wc->pages_class.calc_size == PxmInheritCalcSize)
		wc->pages_class.calc_size = sc->pages_class.calc_size;
	if (wc->pages_class.need_relayout == PxmInheritNeedRelayout)
		wc->pages_class.need_relayout = sc->pages_class.need_relayout;

#ifdef HAVE_XM_TRAIT
	/* Install the XmQTdialogShellSavyy trait on this class and on
	   all its future subclasses. */
	XmeTraitSet(widgetClass, XmQTdialogShellSavvy, (XtPointer) & pagesDST);

	/* Install the XmQTspecifyRenderTable trait on this class and on
	   all its future subclasses. */
	XmeTraitSet(widgetClass, XmQTspecifyRenderTable, (XtPointer) & pagesSRTT);
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
	PxmPagesWidget nw = (PxmPagesWidget)new_w;

	/* Initialize one of the internal fields of the PxmPages widget. */
	nw->pages.processing_constraints = False;
}


/****************************************************************************
 *
 *  Destroy:
 *      Called when the widget is destroyed.
 *
 ****************************************************************************/
static void Destroy(Widget wid)
{
}


/****************************************************************************
 *
 *  Resize:
 *
 ****************************************************************************/
static void Resize(Widget w)
{
	PxmPagesWidgetClass gwc = (PxmPagesWidgetClass) XtClass(w);

	/* Configure the children by calling Layout. */
	if (gwc->pages_class.layout)
		(*(gwc->pages_class.layout)) (w, NULL);
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
	PxmPagesWidget cw = (PxmPagesWidget) old_w;
	PxmPagesWidget nw = (PxmPagesWidget) new_w;
	Boolean redisplay = False;
	Boolean need_relayout;
	PxmPagesWidgetClass gwc = (PxmPagesWidgetClass) XtClass(new_w);

	/* See if any class or subclass resources have changed. */
	if (gwc->pages_class.need_relayout)
		need_relayout = (*(gwc->pages_class.need_relayout)) (old_w, new_w);
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
		if (gwc->pages_class.calc_size)
			(*(gwc->pages_class.calc_size)) (new_w, NULL, &nw->core.width, &nw->core.height);
		else
			CalcSize(new_w, NULL, &nw->core.width, &nw->core.height);


		/* If the geometry resources have changed but the size hasn't, 
		   we need to relayout manually, because Xt won't generate a 
		   Resize at this point. */
		if ((nw->core.width == cw->core.width) && (nw->core.height == cw->core.height)) {

			/* Call Layout to configure the children. */
			if (gwc->pages_class.layout)
				(*(gwc->pages_class.layout)) (new_w, NULL);
			else
				Layout(new_w, NULL);
			redisplay = True;
		}
	}

#ifdef HAVE_XM_TRAIT
	/* PxmPages installs the XmQTdialogShellSavvy trait.  Therefore, PxmPages
	   has to process the Xm_DIALOG_SAVVY_FORCE_ORIGIN case, which is as
	   follows.  A DialogShell always mimics the child position on itself.
	   That is, the "current" position of an PxmPages within a DialogShell is
	   always 0.  Therefore, if an application tries to set PxmPages's x or
	   y position to 0, the Intrinsics will not detect a position change and
	   wll not trigger a geometry request.  PxmPages has to detect this special 
	   request and set core.x and core.y to the special value, 
	   XmDIALOG_SAVVY_FORCE_ORIGIN.  That is, XmDIALOG_SAVVY_FORCE_ORIGIN
	   tells DialogShell that PxmPages really does want to move to an x or y
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
	PxmPagesWidgetClass gwc = (PxmPagesWidgetClass) XtClass(nw);

	/* PxmPages's parent said XtGeometryNo to PxmPages's geometry request. 
	   Therefore, we need to relayout because this request
	   was due to a change in internal geometry resource of the PxmPages */
	if (!reply->request_mode) {
		if (gwc->pages_class.layout)
			(*(gwc->pages_class.layout)) (nw, NULL);
		else
			Layout(nw, NULL);
	}

	*request = *reply;
}


/*************************************************************************
 *
 *  QueryGeometry:
 *       Called by a parent of Pages when the parent needs to find out Pages's
 *       preferred size.  QueryGeometry calls CalcSize to do find the
 *       preferred size. 
 *
 ***************************************************************************/
static XtGeometryResult QueryGeometry(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *reply)
{
	PxmPagesWidgetClass gwc = (PxmPagesWidgetClass) XtClass(w);

	/* If PxmPages's parent calls XtQueryGeometry before PxmPages has been 
	   realized, use the current size of PxmPages as the preferred size. */
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
	if (gwc->pages_class.calc_size)
		(*(gwc->pages_class.calc_size)) (w, NULL, &reply->width, &reply->height);
	else
		CalcSize(w, NULL, &reply->width, &reply->height);

	/* This function handles CWidth and CHeight */
	return PxmReplyToQueryGeometry(w, request, reply);
}


/****************************************************************************
 *
 *  GeometryManager:
 *       Called by Intrinsics in response to a geometry change request from 
 *       one of the children of PxmPages.
 *
 ***************************************************************************/
static XtGeometryResult GeometryManager(Widget w, /* instigator */
																				XtWidgetGeometry *request, XtWidgetGeometry *reply)
{
	PxmPagesWidget gw = (PxmPagesWidget) XtParent(w);
	XtWidgetGeometry parentRequest;
	XtGeometryResult result;
	Dimension curWidth, curHeight, curBW;
	PxmPagesWidgetClass gwc = (PxmPagesWidgetClass) XtClass((Widget) gw);

	/* If the request was caused by ConstraintSetValues reset the flag */
	if (gw->pages.processing_constraints) {
		gw->pages.processing_constraints = False;
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
	if (gwc->pages_class.calc_size)
		(*(gwc->pages_class.calc_size)) ((Widget) gw, w, &parentRequest.width, &parentRequest.height);
	else
		CalcSize((Widget) gw, w, &parentRequest.width, &parentRequest.height);


	/* Ask the Pages's parent if new calculated size is acceptable. */
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
		if (gwc->pages_class.layout)
			(*(gwc->pages_class.layout)) ((Widget) gw, w);
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
	Dimension pagesWidth, pagesHeight;
	PxmPagesWidgetClass gwc = (PxmPagesWidgetClass) XtClass(w);

	/* If you get an initial (C) size from the user or application, keep it.  
	   Otherwise, just force width and height to 0 so that CalcSize will
	   overwrite the appropriate fields. */
	if (!XtIsRealized(w)) {
		/* The first time, only attempts to change non specified sizes */
		pagesWidth = XtWidth(w); /* might be 0 */
		pagesHeight = XtHeight(w); /* might be 0 */
	}
	else {
		pagesWidth = 0;
		pagesHeight = 0;
	}

	/* Determine the ideal size of Pages. */
	if (gwc->pages_class.calc_size)
		(*(gwc->pages_class.calc_size)) (w, NULL, &pagesWidth, &pagesHeight);
	else
		CalcSize(w, NULL, &pagesWidth, &pagesHeight);

	/* Ask parent of Pages if Pages's new size is acceptable.  Keep asking until
	   parent returns either XtGeometryYes or XtGeometryNo. */
	while(XtMakeResizeRequest(w, pagesWidth, pagesHeight, &pagesWidth, &pagesHeight) == XtGeometryAlmost);

	/* Now that we have a size for the Pages, we can layout the children
	   of the pages. */
	if (gwc->pages_class.layout)
		(*(gwc->pages_class.layout)) (w, NULL);
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
	PxmPagesWidget gw;

	if (!XtIsRectObj(nw))
		return (False);

	gw = (PxmPagesWidget) XtParent(nw);

	if (XtIsManaged(nw)) {
		/* Tell the Intrinsics and the GeometryManager method that a 
		   reconfigure is needed. */
		gw->pages.processing_constraints = True;
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
	PxmPagesWidget gw = (PxmPagesWidget) wid;
	Dimension mw = gw->pages.margin_width;
	Dimension mh = gw->pages.margin_height;
	Cardinal i;

	/* show only one page */
	for(i = 0; i < gw->composite.num_children; i++) {
		Widget ic = gw->composite.children[i];
		if (i == gw->pages.at)
			XtManageChild(ic);
		else
			XtUnmanageChild(ic);

		ic->core.x = mw;
		ic->core.y = mh;
		ic->core.width = gw->core.width - 2*mw;
		ic->core.height = gw->core.height - 2*mh;
	}
}



/******************************************************************************
 *
 *  CalcSize:
 *     Called by QueryGeometry, SetValues, GeometryManager, and ChangeManaged.
 *     Calculate the ideal size of the PxmPages widget. 
 *     Only affects the returned size if it is 0.
 *
 ****************************************************************************/
static void CalcSize(Widget wid, Widget instigator, Dimension *TotalWidthOfPagesWidget, Dimension *TotalHeightOfPagesWidget)
{
	PxmPagesWidget gw = (PxmPagesWidget)wid;
	Dimension maxcw = 0, mw = gw->pages.margin_width;
	Dimension maxch = 0, mh = gw->pages.margin_height;
	Cardinal i;

	/* calculate the maximum dims we need to allocate for children */
	for(i = 0; i < gw->composite.num_children; i++) {
		Widget ic = gw->composite.children[i];
		Dimension cb = ic->core.border_width;
		Dimension cw, ch;
		XtWidgetGeometry reply;

		XtQueryGeometry(ic, NULL, &reply);
		cw = 2*cb + (reply.request_mode & CWWidth) ? reply.width : 0;
		ch = 2*cb + (reply.request_mode & CWHeight) ? reply.height : 0;
		if (cw > maxcw)
			maxcw = cw;
		if (ch > maxch)
			maxch = ch;
	}

	*TotalWidthOfPagesWidget = maxcw + 2*mw;
	*TotalHeightOfPagesWidget = maxch + 2*mh;
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
	PxmPagesWidget pages = (PxmPagesWidget) wid;
	XmAnyCallbackStruct call_data;

	call_data.reason = (map_unmap) ? XmCR_MAP : XmCR_UNMAP;
	call_data.event = NULL;

	if (map_unmap) {
		XtCallCallbackList(wid, pages->pages.map_callback, &call_data);
	}
	else {
		XtCallCallbackList(wid, pages->pages.unmap_callback, &call_data);
	}
}

/*****************************************************************
 *
 * Trait method for XmQTspecifyRenderTable.   
 *
*****************************************************************/

static XmRenderTable GetTable(Widget wid, XtEnum type)
{
	PxmPagesWidget pages = (PxmPagesWidget) wid;

	switch (type) {
		case XmLABEL_RENDER_TABLE:
			return pages->pages.label_render_table;
		case XmBUTTON_RENDER_TABLE:
			return pages->pages.button_render_table;
		case XmTEXT_RENDER_TABLE:
			return pages->pages.text_render_table;
	}

	return NULL;
}
#endif

/*******************************************************************************
 *
 *  PxmCreatePages:
 *      Called by an application. 
 *
 ******************************************************************************/
Widget PxmCreatePages(Widget parent, char *name, ArgList arglist, Cardinal argcount)
{
	/* This is a convenience function to instantiate an PxmPages widget. */
	return (XtCreateWidget(name, pxmPagesWidgetClass, parent, arglist, argcount));
}


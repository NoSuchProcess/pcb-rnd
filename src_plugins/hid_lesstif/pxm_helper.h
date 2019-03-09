/* 
 * FillBox widget for Motif - portable helpers
 *
 * Copyright (c) 1987-2012, The Open Group. All rights reserved.
 * Copyright (c) 2019, Tibor 'Igor2' Palinkas
 * (widget code based on motif internals)
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

#define _XmAppLock(app)	XtAppLock(app)
#define _XmAppUnlock(app)	XtAppUnlock(app)
#define _XmProcessLock()	XtProcessLock()
#define _XmProcessUnlock()	XtProcessUnlock()
#define GMode(g)	    ((g)->request_mode)
#define IsWidth(g)	    (GMode (g) & CWWidth)
#define IsHeight(g)	    (GMode (g) & CWHeight)

#define _XmWidgetToAppContext(w) \
        XtAppContext app = XtWidgetToApplicationContext(w)

PCB_INLINE void PxmConfigureObject(Widget wid,
#if NeedWidePrototypes
												int x, int y, int width, int height, int border_width)
#else
												Position x, Position y, Dimension width, Dimension height, Dimension border_width)
#endif /* NeedWidePrototypes */
{
	_XmWidgetToAppContext(wid);
	XmDropSiteStartUpdate(wid);

	_XmAppLock(app);

	if (!width && !height) {
		XtWidgetGeometry desired, preferred;
		desired.request_mode = 0;
		XtQueryGeometry(wid, &desired, &preferred);
		width = preferred.width;
		height = preferred.height;
	}
	if (!width)
		width++;
	if (!height)
		height++;
	XtConfigureWidget(wid, x, y, width, height, border_width);

	XmDropSiteEndUpdate(wid);
	_XmAppUnlock(app);
}

/************************************************************************
 *
 *  XmeRedisplayGadgets
 *	Redisplay any gadgets contained within the manager mw which
 *	are intersected by the region.
 *
 ************************************************************************/
PCB_INLINE void PxmRedisplayGadgets(Widget w, register XEvent *event, Region region)
{
	CompositeWidget mw = (CompositeWidget) w;
	register int i;
	register Widget child;
	XtExposeProc expose;

	_XmWidgetToAppContext(w);

	_XmAppLock(app);
	for(i = 0; i < mw->composite.num_children; i++) {
		child = mw->composite.children[i];
		if (XmIsGadget(child) && XtIsManaged(child)) {
			if (region == NULL) {
				if (child->core.x < event->xexpose.x + event->xexpose.width && child->core.x + child->core.width > event->xexpose.x && child->core.y < event->xexpose.y + event->xexpose.height && child->core.y + child->core.height > event->xexpose.y) {

					_XmProcessLock();
					expose = child->core.widget_class->core_class.expose;
					_XmProcessUnlock();

					if (expose)
						(*(expose))
							(child, event, region);
				}
			}
			else {
				if (XRectInRegion(region, child->core.x, child->core.y, child->core.width, child->core.height)) {
					_XmProcessLock();
					expose = child->core.widget_class->core_class.expose;
					_XmProcessUnlock();

					if (expose)
						(*(expose))
							(child, event, region);
				}
			}
		}
	}
	_XmAppUnlock(app);
}

/****************************************************************
 *
 * XmeReplyToQueryGeometry.
 *
 * This is the generic handling of Almost, No and Yes replies
 * based on the intended values and the given desirs.
 *
 * This can be used by any widget that really only care about is
 * width and height dimension. It just has to compute its desired size
 * using its own private layout routine and resources before calling
 * this one that will deal with the Xt reply value cuisine.
 *
 ****************/
PCB_INLINE XtGeometryResult PxmReplyToQueryGeometry(Widget widget, XtWidgetGeometry *intended, XtWidgetGeometry *desired)
{
	_XmWidgetToAppContext(widget);
	/* the caller should have set desired width and height */
	desired->request_mode = (CWWidth | CWHeight);

	/* Accept any x, y, border and stacking. If the proposed
	   geometry matches the desired one, and the parent intends
	   to use these values (flags are set in intended),
	   return Yes. Otherwise, the parent intends to use values for
	   width and height that differ from the desired size, return No
	   if the desired is the current and Almost if the desired size
	   is different from the current size */
	if ((IsWidth(intended)) && (intended->width == desired->width) && (IsHeight(intended)) && (intended->height == desired->height)) {
		return XtGeometryYes;
	}

	_XmAppLock(app);
	if ((desired->width == XtWidth(widget)) && (desired->height == XtHeight(widget))) {
		_XmAppUnlock(app);
		return XtGeometryNo;
	}

	_XmAppUnlock(app);
	return XtGeometryAlmost;
}

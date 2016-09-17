/*
 *
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* Dead code - no calls to these functions from anywhere in the code */
#error do not compile this


static int LOT_Linecallback(const BoxType * b, void *cl)
{
	LineTypePtr line = (LineTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!TEST_FLAG(TheFlag, line) && LineLineIntersect(&i->line, line))
		longjmp(i->env, 1);
	return 0;
}

static int LOT_Arccallback(const BoxType * b, void *cl)
{
	ArcTypePtr arc = (ArcTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!arc->Thickness)
		return 0;
	if (!TEST_FLAG(TheFlag, arc) && LineArcIntersect(&i->line, arc))
		longjmp(i->env, 1);
	return 0;
}

static int LOT_Padcallback(const BoxType * b, void *cl)
{
	PadTypePtr pad = (PadTypePtr) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!TEST_FLAG(TheFlag, pad) && i->layer == (TEST_FLAG(PCB_FLAG_ONSOLDER, pad) ? SOLDER_LAYER : COMPONENT_LAYER)
			&& LinePadIntersect(&i->line, pad))
		longjmp(i->env, 1);
	return 0;
}

static pcb_bool PVTouchesLine(LineTypePtr line)
{
	struct lo_info info;

	info.line = *line;
	EXPAND_BOUNDS(&info.line);
	if (setjmp(info.env) == 0)
		r_search(PCB->Data->via_tree, (BoxType *) & info.line, NULL, pv_touch_callback, &info, NULL);
	else
		return pcb_true;
	if (setjmp(info.env) == 0)
		r_search(PCB->Data->pin_tree, (BoxType *) & info.line, NULL, pv_touch_callback, &info, NULL);
	else
		return pcb_true;

	return (pcb_false);
}

static pcb_bool LOTouchesLine(LineTypePtr Line, Cardinal LayerGroup)
{
	Cardinal entry;
	struct lo_info info;


	/* the maximum possible distance */

	info.line = *Line;
	EXPAND_BOUNDS(&info.line);

	/* loop over all layers of the group */
	for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++) {
		Cardinal layer = PCB->LayerGroups.Entries[LayerGroup][entry];

		/* handle normal layers */
		if (layer < max_copper_layer) {
			gdl_iterator_t it;
			PolygonType *polygon;

			/* find the first line that touches coordinates */

			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->line_tree, (BoxType *) & info.line, NULL, LOT_Linecallback, &info, NULL);
			else
				return (pcb_true);
			if (setjmp(info.env) == 0)
				r_search(LAYER_PTR(layer)->arc_tree, (BoxType *) & info.line, NULL, LOT_Arccallback, &info, NULL);
			else
				return (pcb_true);

			/* now check all polygons */
			polylist_foreach(&(PCB->Data->Layer[layer].Polygon), &it, polygon) {
				if (!TEST_FLAG(TheFlag, polygon)
						&& IsLineInPolygon(Line, polygon))
					return (pcb_true);
			}
		}
		else {
			/* handle special 'pad' layers */
			info.layer = layer - max_copper_layer;
			if (setjmp(info.env) == 0)
				r_search(PCB->Data->pad_tree, &info.line.BoundingBox, NULL, LOT_Padcallback, &info, NULL);
			else
				return pcb_true;
		}
	}
	return (pcb_false);
}

/* returns pcb_true if nothing un-found touches the passed line
 * returns pcb_false if it would touch something not yet found
 * doesn't include rat-lines in the search
 */

pcb_bool lineClear(LineTypePtr line, Cardinal group)
{
	if (LOTouchesLine(line, group))
		return (pcb_false);
	if (PVTouchesLine(line))
		return (pcb_false);
	return (pcb_true);
}

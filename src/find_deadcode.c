/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* Dead code - no calls to these functions from anywhere in the code */
#error do not compile this


static int LOT_Linecallback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, line) && pcb_isc_line_line(&i->line, line))
		longjmp(i->env, 1);
	return 0;
}

static int LOT_Arccallback(const pcb_box_t * b, void *cl)
{
	pcb_arc_t *arc = (pcb_arc_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!arc->Thickness)
		return 0;
	if (!PCB_FLAG_TEST(TheFlag, arc) && pcb_isc_line_arc(&i->line, arc))
		longjmp(i->env, 1);
	return 0;
}

static int LOT_Padcallback(const pcb_box_t * b, void *cl)
{
	pcb_pad_t *pad = (pcb_pad_t *) b;
	struct lo_info *i = (struct lo_info *) cl;

	if (!PCB_FLAG_TEST(TheFlag, pad) && i->layer == (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) ? PCB_SOLDER_SIDE : PCB_COMPONENT_SIDE)
			&& pcb_intersect_line_pad(&i->line, pad))
		longjmp(i->env, 1);
	return 0;
}

static pcb_bool PVTouchesLine(pcb_line_t *line)
{
	struct lo_info info;

	info.line = *line;
	EXPAND_BOUNDS(&info.line);
	if (setjmp(info.env) == 0)
		pcb_r_search(PCB->Data->via_tree, (pcb_box_t *) & info.line, NULL, pv_touch_callback, &info, NULL);
	else
		return pcb_true;
	if (setjmp(info.env) == 0)
		pcb_r_search(PCB->Data->pin_tree, (pcb_box_t *) & info.line, NULL, pv_touch_callback, &info, NULL);
	else
		return pcb_true;

	return pcb_false;
}

static pcb_bool LOTouchesLine(pcb_line_t *Line, pcb_cardinal_t LayerGroup)
{
	pcb_cardinal_t entry;
	struct lo_info info;


	/* the maximum possible distance */

	info.line = *Line;
	EXPAND_BOUNDS(&info.line);

	/* loop over all layers of the group */
	for (entry = 0; entry < PCB->LayerGroups.Number[LayerGroup]; entry++) {
		pcb_cardinal_t layer = PCB->LayerGroups.Entries[LayerGroup][entry];

		/* handle normal layers */
		if (layer < pcb_max_copper_layer) {
			gdl_iterator_t it;
			pcb_poly_t *polygon;

			/* find the first line that touches coordinates */

			if (setjmp(info.env) == 0)
				pcb_r_search(LAYER_PTR(layer)->line_tree, (pcb_box_t *) & info.line, NULL, LOT_Linecallback, &info, NULL);
			else
				return pcb_true;
			if (setjmp(info.env) == 0)
				pcb_r_search(LAYER_PTR(layer)->arc_tree, (pcb_box_t *) & info.line, NULL, LOT_Arccallback, &info, NULL);
			else
				return pcb_true;

			/* now check all polygons */
			polylist_foreach(&(PCB->Data->Layer[layer].Polygon), &it, polygon) {
				if (!PCB_FLAG_TEST(TheFlag, polygon)
						&& pcb_is_line_in_poly(Line, polygon))
					return pcb_true;
			}
		}
		else {
			/* handle special 'pad' layers */
			info.layer = layer - pcb_max_copper_layer;
			if (setjmp(info.env) == 0)
				pcb_r_search(PCB->Data->pad_tree, &info.line.BoundingBox, NULL, LOT_Padcallback, &info, NULL);
			else
				return pcb_true;
		}
	}
	return pcb_false;
}

/* returns pcb_true if nothing un-found touches the passed line
 * returns pcb_false if it would touch something not yet found
 * doesn't include rat-lines in the search
 */

pcb_bool lineClear(pcb_line_t *line, pcb_cardinal_t group)
{
	if (LOTouchesLine(line, group))
		return pcb_false;
	if (PVTouchesLine(line))
		return pcb_false;
	return pcb_true;
}

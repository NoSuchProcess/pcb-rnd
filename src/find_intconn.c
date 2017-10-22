/*
 *
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

void LOC_int_conn_element(pcb_element_t *e, int ic, int from_type, void *from_ptr)
{
/*	int tlayer = -1;*/

	/* Internal connection: if pins/pads in the same element have the same
	   internal connection group number, they are connected */
	PCB_PIN_LOOP(e);
	{
		if ((from_ptr != pin) && (ic == pin->intconn)) {
			if (!PCB_FLAG_TEST(TheFlag, pin))
				ADD_PV_TO_LIST(pin, from_type, from_ptr, PCB_FCT_INTERNAL);
		}
	}
	PCB_END_LOOP;

/*
	for (entry = 0; entry < PCB->LayerGroups.grp[LayerGroup].len; entry++) {
		pcb_layer_id_t layer;
		layer = PCB->LayerGroups.grp[LayerGroup].lid[entry];
		if (layer == PCB_COMPONENT_SIDE)
			tlayer = PCB_COMPONENT_SIDE;
		else if (layer == PCB_SOLDER_SIDE)
			tlayer = PCB_SOLDER_SIDE;
	}
*/

/*	if (tlayer >= 0)*/ {
		PCB_PAD_LOOP(e);
		{
			if ((from_ptr != pad) && (ic == pad->intconn)) {
				int padlayer = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) ? PCB_SOLDER_SIDE : PCB_COMPONENT_SIDE;
				if ((!PCB_FLAG_TEST(TheFlag, pad)) /* && (tlayer != padlayer)*/) {
					ADD_PAD_TO_LIST(padlayer, pad, from_type, from_ptr, PCB_FCT_INTERNAL);
/*					if (LookupLOConnectionsToPad(pad, LayerGroup))
						retv = pcb_true;*/
				}
			}
		}
		PCB_END_LOOP;
	}
}

static void LOC_int_conn_subc(pcb_subc_t *s, int ic, int from_type, void *from_ptr)
{
	if (s == NULL)
		return;

	PCB_VIA_LOOP(s->data);
	{
		if ((via != from_ptr) && (via->term != NULL) && (via->intconn == ic) && (!PCB_FLAG_TEST(TheFlag, via))) {
			PCB_FLAG_SET(PCB_FLAG_DRC_INTCONN, via);
			ADD_PV_TO_LIST(via, from_type, from_ptr, PCB_FCT_INTERNAL);
		}
	}
	PCB_END_LOOP;

	PCB_PADSTACK_LOOP(s->data);
	{
		if ((padstack != from_ptr) && (padstack->term != NULL) && (padstack->intconn == ic) && (!PCB_FLAG_TEST(TheFlag, padstack))) {
			PCB_FLAG_SET(PCB_FLAG_DRC_INTCONN, padstack);
			ADD_PADSTACK_TO_LIST(padstack, from_type, from_ptr, PCB_FCT_INTERNAL);
		}
	}
	PCB_END_LOOP;

	PCB_LINE_COPPER_LOOP(s->data);
	{
		if ((line != from_ptr) && (line->term != NULL) && (line->intconn == ic) && (!PCB_FLAG_TEST(TheFlag, line))) {
			PCB_FLAG_SET(PCB_FLAG_DRC_INTCONN, line);
			ADD_LINE_TO_LIST(l, line, from_type, from_ptr, PCB_FCT_INTERNAL);
		}
	}
	PCB_ENDALL_LOOP;

	PCB_ARC_COPPER_LOOP(s->data);
	{
		if ((arc != from_ptr) && (arc->term != NULL) && (arc->intconn == ic) && (!PCB_FLAG_TEST(TheFlag, arc))) {
			PCB_FLAG_SET(PCB_FLAG_DRC_INTCONN, arc);
			ADD_ARC_TO_LIST(l, arc, from_type, from_ptr, PCB_FCT_INTERNAL);
		}
	}
	PCB_ENDALL_LOOP;

	PCB_POLY_COPPER_LOOP(s->data);
	{
		if ((polygon != from_ptr) && (polygon->term != NULL) && (polygon->intconn == ic) && (!PCB_FLAG_TEST(TheFlag, polygon))) {
			PCB_FLAG_SET(PCB_FLAG_DRC_INTCONN, polygon);
			ADD_POLYGON_TO_LIST(l, polygon, from_type, from_ptr, PCB_FCT_INTERNAL);
		}
	}
	PCB_ENDALL_LOOP;

#warning subc TODO
#if 0
no find through text yet
	PCB_TEXT_COPPER_LOOP(s->data);
	{
		if ((text != from_ptr) && (text->term != NULL) && (text->intconn == ic) && (!PCB_FLAG_TEST(TheFlag, text))) {
			PCB_FLAG_SET(PCB_FLAG_DRC_INTCONN, text);
			ADD_TEXT_TO_LIST(l, text, from_type, from_ptr, PCB_FCT_INTERNAL);
		}
	}
	PCB_ENDALL_LOOP;
#endif
}

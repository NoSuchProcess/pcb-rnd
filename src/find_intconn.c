/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

static void LOC_int_conn_subc(pcb_subc_t *s, int ic, int from_type, void *from_ptr)
{
	if (s == NULL)
		return;

	PCB_PADSTACK_LOOP(s->data);
	{
		if ((padstack != from_ptr) && (padstack->term != NULL) && (padstack->intconn == ic) && (!PCB_FLAG_TEST(TheFlag, padstack))) {
			PCB_FLAG_SET(PCB_FLAG_DRC_INTCONN, padstack);
			ADD_PADSTACK_TO_LIST(padstack, from_type, from_ptr, PCB_FCT_INTERNAL, NULL);
		}
	}
	PCB_END_LOOP;

	PCB_LINE_COPPER_LOOP(s->data);
	{
		if ((line != from_ptr) && (line->term != NULL) && (line->intconn == ic) && (!PCB_FLAG_TEST(TheFlag, line))) {
			PCB_FLAG_SET(PCB_FLAG_DRC_INTCONN, line);
			ADD_LINE_TO_LIST(l, line, from_type, from_ptr, PCB_FCT_INTERNAL, NULL);
		}
	}
	PCB_ENDALL_LOOP;

	PCB_ARC_COPPER_LOOP(s->data);
	{
		if ((arc != from_ptr) && (arc->term != NULL) && (arc->intconn == ic) && (!PCB_FLAG_TEST(TheFlag, arc))) {
			PCB_FLAG_SET(PCB_FLAG_DRC_INTCONN, arc);
			ADD_ARC_TO_LIST(l, arc, from_type, from_ptr, PCB_FCT_INTERNAL, NULL);
		}
	}
	PCB_ENDALL_LOOP;

	PCB_POLY_COPPER_LOOP(s->data);
	{
		if ((polygon != from_ptr) && (polygon->term != NULL) && (polygon->intconn == ic) && (!PCB_FLAG_TEST(TheFlag, polygon))) {
			PCB_FLAG_SET(PCB_FLAG_DRC_INTCONN, polygon);
			ADD_POLYGON_TO_LIST(l, polygon, from_type, from_ptr, PCB_FCT_INTERNAL, NULL);
		}
	}
	PCB_ENDALL_LOOP;

TODO("find: no find through text yet")
#if 0
	PCB_TEXT_COPPER_LOOP(s->data);
	{
		if ((text != from_ptr) && (text->term != NULL) && (text->intconn == ic) && (!PCB_FLAG_TEST(TheFlag, text))) {
			PCB_FLAG_SET(PCB_FLAG_DRC_INTCONN, text);
			ADD_TEXT_TO_LIST(l, text, from_type, from_ptr, PCB_FCT_INTERNAL, NULL);
		}
	}
	PCB_ENDALL_LOOP;
#endif
}

/* return whether a and b are in the same internal-no-connection group */
static pcb_bool int_noconn(pcb_any_obj_t *a, pcb_any_obj_t *b)
{
	pcb_subc_t *pa, *pb;

	/* cheap test: they need to have valid and matching intnoconn */
	if ((a->intnoconn == 0) || (a->intnoconn != b->intnoconn))
		return pcb_false;

	/* expensive tests: they need to be in the same subc */
	pa = pcb_obj_parent_subc(a);
	if (pa == NULL)
		return pcb_false;

	pb = pcb_obj_parent_subc(b);

	return (pa == pb);
}

#define INOCN(a,b) int_noconn((pcb_any_obj_t *)a, (pcb_any_obj_t *)b)

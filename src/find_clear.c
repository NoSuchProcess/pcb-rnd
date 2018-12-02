/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *
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

/* Resets all used flags of pins and vias. */
TODO(": this file could be removed if ipcd356 used the operation API")
pcb_bool pcb_clear_flag_on_pins_vias_pads(pcb_bool AndDraw, int flag)
{
	pcb_bool change = pcb_false;

	PCB_PADSTACK_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(flag, padstack)) {
			if (AndDraw)
				pcb_undo_add_obj_to_flag(padstack);
			PCB_FLAG_CLEAR(flag, padstack);
			if (AndDraw)
				pcb_pstk_invalidate_draw(padstack);
			change = pcb_true;
		}
	}
	PCB_END_LOOP;

	PCB_SUBC_LOOP(PCB->Data);
	{
		PCB_PADSTACK_LOOP(subc->data);
		{
			if (padstack->term == NULL)
				continue;
			if (PCB_FLAG_TEST(flag, padstack)) {
				if (AndDraw)
					pcb_undo_add_obj_to_flag(padstack);
				PCB_FLAG_CLEAR(flag, padstack);
				if (AndDraw)
					pcb_pstk_invalidate_draw(padstack);
				change = pcb_true;
			}
		}
		PCB_END_LOOP;

		PCB_LINE_ALL_LOOP(subc->data);
		{
			if (line->term == NULL)
				continue;
			if (PCB_FLAG_TEST(flag, line)) {
				if (AndDraw)
					pcb_undo_add_obj_to_flag(line);
				PCB_FLAG_CLEAR(flag, line);
				if (AndDraw)
					pcb_line_invalidate_draw(layer, line);
				change = pcb_true;
			}
		}
		PCB_ENDALL_LOOP;

		PCB_ARC_ALL_LOOP(subc->data);
		{
			if (arc->term == NULL)
				continue;
			if (PCB_FLAG_TEST(flag, arc)) {
				if (AndDraw)
					pcb_undo_add_obj_to_flag(arc);
				PCB_FLAG_CLEAR(flag, arc);
				if (AndDraw)
					pcb_arc_invalidate_draw(layer, arc);
				change = pcb_true;
			}
		}
		PCB_ENDALL_LOOP;

		PCB_POLY_ALL_LOOP(subc->data);
		{
			if (polygon->term == NULL)
				continue;
			if (PCB_FLAG_TEST(flag, polygon)) {
				if (AndDraw)
					pcb_undo_add_obj_to_flag(polygon);
				PCB_FLAG_CLEAR(flag, polygon);
				if (AndDraw)
					pcb_poly_invalidate_draw(layer, polygon);
				change = pcb_true;
			}
		}
		PCB_ENDALL_LOOP;

		PCB_TEXT_ALL_LOOP(subc->data);
		{
			if (text->term == NULL)
				continue;
			if (PCB_FLAG_TEST(flag, text)) {
				if (AndDraw)
					pcb_undo_add_obj_to_flag(text);
				PCB_FLAG_CLEAR(flag, text);
				if (AndDraw)
					pcb_text_invalidate_draw(layer, text);
				change = pcb_true;
			}
		}
		PCB_ENDALL_LOOP;
	}
	PCB_END_LOOP;

	if (change)
		pcb_board_set_changed_flag(pcb_true);
	return change;
}

/* Resets all used flags of LOs. */
pcb_bool pcb_clear_flag_on_lines_polys(pcb_bool AndDraw, int flag)
{
	pcb_bool change = pcb_false;

	PCB_RAT_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(flag, line)) {
			if (AndDraw)
				pcb_undo_add_obj_to_flag(line);
			PCB_FLAG_CLEAR(flag, line);
			if (AndDraw)
				pcb_rat_invalidate_draw(line);
			change = pcb_true;
		}
	}
	PCB_END_LOOP;
	PCB_LINE_COPPER_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(flag, line)) {
			if (AndDraw)
				pcb_undo_add_obj_to_flag(line);
			PCB_FLAG_CLEAR(flag, line);
			if (AndDraw)
				pcb_line_invalidate_draw(layer, line);
			change = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_COPPER_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(flag, arc)) {
			if (AndDraw)
				pcb_undo_add_obj_to_flag(arc);
			PCB_FLAG_CLEAR(flag, arc);
			if (AndDraw)
				pcb_arc_invalidate_draw(layer, arc);
			change = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_COPPER_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(flag, polygon)) {
			if (AndDraw)
				pcb_undo_add_obj_to_flag(polygon);
			PCB_FLAG_CLEAR(flag, polygon);
			if (AndDraw)
				pcb_poly_invalidate_draw(layer, polygon);
			change = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;
	if (change)
		pcb_board_set_changed_flag(pcb_true);
	return change;
}

/*
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996, 2005 Thomas Nau
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 */

/*!
 * \brief Resets all used flags of pins and vias.
 */

pcb_bool pcb_clear_flag_on_pins_vias_pads(pcb_bool AndDraw, int flag)
{
	pcb_bool change = pcb_false;

	PCB_VIA_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(flag, via)) {
			if (AndDraw)
				pcb_undo_add_obj_to_flag(PCB_TYPE_VIA, via, via, via);
			PCB_FLAG_CLEAR(flag, via);
			if (AndDraw)
				pcb_via_invalidate_draw(via);
			change = pcb_true;
		}
	}
	PCB_END_LOOP;
	PCB_ELEMENT_LOOP(PCB->Data);
	{
		PCB_PIN_LOOP(element);
		{
			if (PCB_FLAG_TEST(flag, pin)) {
				if (AndDraw)
					pcb_undo_add_obj_to_flag(PCB_TYPE_PIN, element, pin, pin);
				PCB_FLAG_CLEAR(flag, pin);
				if (AndDraw)
					pcb_pin_invalidate_draw(pin);
				change = pcb_true;
			}
		}
		PCB_END_LOOP;
		PCB_PAD_LOOP(element);
		{
			if (PCB_FLAG_TEST(flag, pad)) {
				if (AndDraw)
					pcb_undo_add_obj_to_flag(PCB_TYPE_PAD, element, pad, pad);
				PCB_FLAG_CLEAR(flag, pad);
				if (AndDraw)
					pcb_pad_invalidate_draw(pad);
				change = pcb_true;
			}
		}
		PCB_END_LOOP;
	}
	PCB_END_LOOP;
#warning subc term TODO
	if (change)
		pcb_board_set_changed_flag(pcb_true);
	return change;
}

/*!
 * \brief Resets all used flags of LOs.
 */
pcb_bool pcb_clear_flag_on_lines_polys(pcb_bool AndDraw, int flag)
{
	pcb_bool change = pcb_false;

	PCB_RAT_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(flag, line)) {
			if (AndDraw)
				pcb_undo_add_obj_to_flag(PCB_TYPE_RATLINE, line, line, line);
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
				pcb_undo_add_obj_to_flag(PCB_TYPE_LINE, layer, line, line);
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
				pcb_undo_add_obj_to_flag(PCB_TYPE_ARC, layer, arc, arc);
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
				pcb_undo_add_obj_to_flag(PCB_TYPE_POLYGON, layer, polygon, polygon);
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

/*!
 * \brief Resets all found connections.
 */
pcb_bool pcb_clear_flag_on_all_objs(pcb_bool AndDraw, int flag)
{
	pcb_bool change = pcb_false;

	change = pcb_clear_flag_on_pins_vias_pads(AndDraw, flag) || change;
	change = pcb_clear_flag_on_lines_polys(AndDraw, flag) || change;

	return change;
}

/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

static void pse_tab_ps(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{

}

static void pse_tab_proto(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{

}


static const char pcb_acts_PadstackEdit[] = "PadstackEdit()\n";
static const char pcb_acth_PadstackEdit[] = "interactive pad stack editor";
static int pcb_act_PadstackEdit(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	PCB_DAD_DECL(dlg);
	PCB_DAD_BEGIN_VBOX(dlg);
		PCB_DAD_BEGIN_HBOX(dlg);
			PCB_DAD_BUTTON(dlg, "this instance");
				PCB_DAD_CHANGE_CB(dlg, pse_tab_ps);
			PCB_DAD_BUTTON(dlg, "prototype");
				PCB_DAD_CHANGE_CB(dlg, pse_tab_proto);
		PCB_DAD_END(dlg);

		/* this instance */
		PCB_DAD_BEGIN_VBOX(dlg);
			PCB_DAD_COMPFLAG(dlg, PCB_HATF_FRAME);
			PCB_DAD_BEGIN_HBOX(dlg);
				PCB_DAD_LABEL(dlg, "prototype");
				PCB_DAD_BUTTON(dlg, "#5");
			PCB_DAD_END(dlg);
			PCB_DAD_BEGIN_HBOX(dlg);
				PCB_DAD_LABEL(dlg, "Clearance");
				PCB_DAD_COORD(dlg, "");
					PCB_DAD_MINVAL(dlg, 1);
					PCB_DAD_MAXVAL(dlg, PCB_MM_TO_COORD(1000));
					PCB_DAD_DEFAULT(dlg, 3);
			PCB_DAD_END(dlg);
		PCB_DAD_END(dlg);

	PCB_DAD_END(dlg);

	PCB_DAD_AUTORUN(dlg, "dlg_padstack_edit", "Edit padstack", NULL);

	PCB_DAD_FREE(dlg);
	return 0;
}


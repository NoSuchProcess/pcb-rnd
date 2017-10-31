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

typedef struct pse_s {
	int tab_instance, tab_prototype;
	int but_instance, but_prototype;

	int tab;
} pse_t;

static void pse_tab_update(void *hid_ctx, pse_t *pse)
{
	switch(pse->tab) {
		case 0:
			pcb_gui->attr_dlg_widget_state(hid_ctx, pse->tab_instance, pcb_true);
			pcb_gui->attr_dlg_widget_state(hid_ctx, pse->tab_prototype, pcb_false);
			pcb_gui->attr_dlg_widget_state(hid_ctx, pse->but_instance, pcb_false);
			pcb_gui->attr_dlg_widget_state(hid_ctx, pse->but_prototype, pcb_true);
			break;
		case 1:
			pcb_gui->attr_dlg_widget_state(hid_ctx, pse->tab_instance, pcb_false);
			pcb_gui->attr_dlg_widget_state(hid_ctx, pse->tab_prototype, pcb_true);
			pcb_gui->attr_dlg_widget_state(hid_ctx, pse->but_instance, pcb_true);
			pcb_gui->attr_dlg_widget_state(hid_ctx, pse->but_prototype, pcb_false);
			break;
	}
}

static void pse_tab_ps(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_t *pse = caller_data;
	pse->tab = 0;
	pse_tab_update(hid_ctx, pse);
}

static void pse_tab_proto(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pse_t *pse = caller_data;
	pse->tab = 1;
	pse_tab_update(hid_ctx, pse);
}


static const char pcb_acts_PadstackEdit[] = "PadstackEdit()\n";
static const char pcb_acth_PadstackEdit[] = "interactive pad stack editor";
static int pcb_act_PadstackEdit(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pse_t pse;
	PCB_DAD_DECL(dlg);

	memset(&pse, 0, sizeof(pse));

	PCB_DAD_BEGIN_VBOX(dlg);
		PCB_DAD_BEGIN_HBOX(dlg);
			PCB_DAD_BUTTON(dlg, "this instance");
				pse.but_instance = PCB_DAD_CURRENT(dlg);
				PCB_DAD_CHANGE_CB(dlg, pse_tab_ps);
			PCB_DAD_BUTTON(dlg, "prototype");
				pse.but_prototype = PCB_DAD_CURRENT(dlg);
				PCB_DAD_CHANGE_CB(dlg, pse_tab_proto);
		PCB_DAD_END(dlg);

		/* this instance */
		PCB_DAD_BEGIN_VBOX(dlg);
			PCB_DAD_COMPFLAG(dlg, PCB_HATF_FRAME);
			pse.tab_instance = PCB_DAD_CURRENT(dlg);
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

		/* prototype */
		PCB_DAD_BEGIN_VBOX(dlg);
			PCB_DAD_COMPFLAG(dlg, PCB_HATF_FRAME);
			pse.tab_prototype = PCB_DAD_CURRENT(dlg);
		PCB_DAD_END(dlg);
	PCB_DAD_END(dlg);


	PCB_DAD_NEW(dlg, "dlg_padstack_edit", "Edit padstack", &pse);
	pse_tab_update(dlg_hid_ctx, &pse);
	PCB_DAD_RUN(dlg);

	PCB_DAD_FREE(dlg);
	return 0;
}


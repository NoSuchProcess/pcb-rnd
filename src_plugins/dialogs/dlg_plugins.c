/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2015,2018 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

static const char pcb_acts_ManagePlugins[] = "ManagePlugins()\n";
static const char pcb_acth_ManagePlugins[] = "Manage plugins dialog.";
static fgw_error_t pcb_act_ManagePlugins(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pup_plugin_t *p;
	int nump = 0, numb = 0;
	gds_t str;
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	PCB_DAD_DECL(dlg);

	gds_init(&str);

	for (p = pcb_pup.plugins; p != NULL; p = p->next)
		if (p->flags & PUP_FLG_STATIC)
			numb++;
		else
			nump++;

	gds_append_str(&str, "Plugins loaded:\n");
	if (nump > 0) {
		for (p = pcb_pup.plugins; p != NULL; p = p->next) {
			if (!(p->flags & PUP_FLG_STATIC)) {
				gds_append(&str, ' ');
				gds_append_str(&str, p->name);
				gds_append(&str, ' ');
				gds_append_str(&str, p->path);
				gds_append(&str, '\n');
			}
		}
	}
	else
		gds_append_str(&str, " (none)\n");

	gds_append_str(&str, "\n\nBuildins:\n");
	if (numb > 0) {
		for (p = pcb_pup.plugins; p != NULL; p = p->next) {
			if (p->flags & PUP_FLG_STATIC) {
				gds_append(&str, ' ');
				gds_append_str(&str, p->name);
				gds_append(&str, '\n');
			}
		}
	}
	else
		gds_append_str(&str, " (none)\n");

	gds_append_str(&str, "\n\nNOTE: this is the alpha version, can only list plugins/buildins\n");


	PCB_DAD_BEGIN_VBOX(dlg);
		PCB_DAD_COMPFLAG(dlg, PCB_HATF_SCROLL | PCB_HATF_EXPFILL);
		PCB_DAD_LABEL(dlg, str.array);
		PCB_DAD_BUTTON_CLOSES(dlg, clbtn);
	PCB_DAD_END(dlg);

	PCB_DAD_NEW("plugins", dlg, "Manage plugins", NULL, pcb_true, NULL);
	PCB_DAD_RUN(dlg);
	PCB_DAD_FREE(dlg);

	gds_uninit(&str);
	PCB_ACT_IRES(0);
	return 0;
}

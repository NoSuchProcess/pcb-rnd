/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  2d drafting plugin
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

static const char pcb_acts_ddraft[] = "ddraft()";
static const char pcb_acth_ddraft[] = "Enter 2d drafting CLI mode or execute command";
static fgw_error_t pcb_act_ddraft(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *cmd;

	if (argc == 1) {
		pcb_cli_enter("ddraft", "ddraft");
		PCB_ACT_IRES(0);
		return 0;
	}

	PCB_ACT_CONVARG(1, FGW_STR, ddraft, cmd = argv[1].val.str);
	if (strcmp(cmd, "/exit") == 0) {
		pcb_cli_leave();
		PCB_ACT_IRES(0);
		return 0;
	}
	if (strcmp(cmd, "/click") == 0) {
		PCB_ACT_IRES(-1); /* ignore clicks for now */
		return 0;
	}

	PCB_ACT_IRES(0);
	return 0;
}

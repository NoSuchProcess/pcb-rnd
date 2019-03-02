/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"
#include "board.h"
#include "data.h"
#include "change.h"
#include "error.h"
#include "undo.h"
#include "plugins.h"
#include "actions.h"
#include "conf_core.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "netlist2.h"
#include "safe_fs.h"
#include "macro.h"

#include "pcb-printf.h"

static const char pcb_acts_Renumber[] = "Renumber()\n" "Renumber(filename)";
static const char pcb_acth_Renumber[] =
	"Renumber all subcircuits.  The changes will be recorded to filename\n"
	"for use in backannotating these changes to the schematic.";

#define WTF 0

static const char *or_empty(const char *s)
{
	if (s == NULL) return "";
	return s;
}

static fgw_error_t pcb_act_Renumber(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_bool changed = pcb_false;
	pcb_subc_t **subc_list;
	pcb_subc_t **locked_subc_list;
	pcb_coord_t *ox_list, *oy_list;
	unsigned int i, j, k, cnt, lock_cnt;
	unsigned int tmpi;
	size_t sz;
	char *tmps;
	const char *name = NULL;
	FILE *out;
	static char *default_file = NULL;
	size_t cnt_list_sz = 100;
	struct _cnt_list {
		char *name;
		unsigned int cnt;
	} *cnt_list;
	char **was, **is, *pin;
	unsigned int c_cnt = 0, numsubc;
	int ok;
	pcb_bool free_name = pcb_false;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, Renumber, name = argv[1].val.str);

	if (name == NULL) {
		/*
		 * We deal with the case where name already exists in this
		 * function so the GUI doesn't need to deal with it
		 */
		name = pcb_gui->fileselect(_("Save Renumber Annotation File As ..."),
													 _("Choose a file to record the renumbering to.\n"
														 "This file may be used to back annotate the\n"
														 "change to the schematics.\n"), default_file, ".eco", "eco", 0);

		free_name = pcb_true;
	}

	if (default_file) {
		free(default_file);
		default_file = NULL;
	}

	if (name && *name) {
		default_file = pcb_strdup(name);
	}

	if ((out = pcb_fopen(name, "r"))) {
		fclose(out);
		if (pcb_hid_message_box("warning", "Renumber: overwrite", "File exists!  Ok to overwrite?", "cancel", 0, "overwrite", 1, NULL) != 1) {
			if (free_name && name)
				free((char*)name);
			PCB_ACT_IRES(1);
			return 0;
		}
	}

	if ((out = pcb_fopen(name, "w")) == NULL) {
		pcb_message(PCB_MSG_ERROR, _("Could not open %s\n"), name);
		if (free_name && name)
			free((char*)name);
		PCB_ACT_IRES(1);
		return 0;
	}

	if (free_name && name)
		free((char*)name);

	fprintf(out, "*COMMENT* PCB Annotation File\n");
	fprintf(out, "*FILEVERSION* 20061031\n");

	/*
	 * Make a first pass through all of the subcircuits and sort them out
	 * by location on the board.  While here we also collect a list of
	 * locked subcircuits.
	 *
	 * We'll actually renumber things in the 2nd pass.
	 */
	numsubc = pcb_subclist_length(&PCB->Data->subc);
	subc_list = calloc(numsubc, sizeof(pcb_subc_t *));
	ox_list = calloc(numsubc, sizeof(pcb_coord_t));
	oy_list = calloc(numsubc, sizeof(pcb_coord_t));
	locked_subc_list =  calloc(numsubc, sizeof(pcb_subc_t *));
	was = calloc(numsubc, sizeof(char *));
	is = calloc(numsubc, sizeof(char *));
	if (subc_list == NULL || locked_subc_list == NULL || was == NULL || is == NULL || ox_list == NULL || oy_list == NULL) {
		fprintf(stderr, "calloc() failed in pcb_act_Renumber\n");
		exit(1);
	}


	cnt = 0;
	lock_cnt = 0;

	PCB_SUBC_LOOP(PCB->Data);
	{
		pcb_coord_t ox, oy;
		
		if (pcb_subc_get_origin(subc, &ox, &oy) != 0) {
			ox = (subc->BoundingBox.X1 + subc->BoundingBox.X2)/2;
			oy = (subc->BoundingBox.Y1 + subc->BoundingBox.Y2)/2;
		}

		if (PCB_FLAG_TEST(PCB_FLAG_LOCK, subc)) {
			/* add to the list of locked subcircuits which we won't try to renumber and whose reference designators are now reserved. */
			pcb_fprintf(out,
				"*WARN* subc \"%s\" at %$md is locked and will not be renumbered.\n",
				PCB_UNKNOWN(subc->refdes), ox, oy);
			locked_subc_list[lock_cnt] = subc;
			lock_cnt++;
		}
		else {
			/* count of devices which will be renumbered */
			cnt++;

			/* search for correct position in the list */
			i = 0;
			while (subc_list[i] && oy > oy_list[i])
				i++;

			/* We have found the position where we have the first subcircuits that
			 * has the same Y value or a lower Y value.  Now move forward if
			 * needed through the X values */
			while (subc_list[i] && oy == oy_list[i] && ox > ox_list[i])
				i++;

			for (j = cnt - 1; j > i; j--) {
				subc_list[j] = subc_list[j - 1];
				ox_list[j] = ox_list[j - 1];
				oy_list[j] = oy_list[j - 1];
			}
			subc_list[i] = subc;
			ox_list[j] = ox;
			oy_list[j] = oy;
		}
	}
	PCB_END_LOOP;


	/*
	 * Now that the subcircuits are sorted by board position, we go through
	 * and renumber them.
	 */

	cnt_list = (struct _cnt_list *) calloc(cnt_list_sz, sizeof(struct _cnt_list));
	for (i = 0; i < cnt; i++) {
		if (subc_list[i]->refdes) {
			/* figure out the prefix */
			tmps = pcb_strdup(or_empty(subc_list[i]->refdes));
			j = 0;
			while (tmps[j] && (tmps[j] < '0' || tmps[j] > '9')
						 && tmps[j] != '?')
				j++;
			tmps[j] = '\0';

			/* check the counter for this prefix */
			for (j = 0; cnt_list[j].name && (strcmp(cnt_list[j].name, tmps) != 0)
					 && j < cnt_list_sz; j++);

			/* grow the list if needed */
			if (j == cnt_list_sz) {
				cnt_list_sz += 100;
				cnt_list = (struct _cnt_list *) realloc(cnt_list, cnt_list_sz);
				if (cnt_list == NULL) {
					fprintf(stderr, "realloc() failed in pcb_act_Renumber\n");
					exit(1);
				}
				/* zero out the memory that we added */
				for (tmpi = j; tmpi < cnt_list_sz; tmpi++) {
					cnt_list[tmpi].name = NULL;
					cnt_list[tmpi].cnt = 0;
				}
			}

			/* start a new counter if we don't have a counter for this prefix */
			if (!cnt_list[j].name) {
				cnt_list[j].name = pcb_strdup(tmps);
				cnt_list[j].cnt = 0;
			}

			/* check to see if the new refdes is already used by a locked subcircuit */
			do {
				ok = 1;
				cnt_list[j].cnt++;
				free(tmps);

				/* space for the prefix plus 1 digit plus the '\0' */
				sz = strlen(cnt_list[j].name) + 2;

				/* and 1 more per extra digit needed to hold the number */
				tmpi = cnt_list[j].cnt;
				while (tmpi > 10) {
					sz++;
					tmpi = tmpi / 10;
				}
				tmps = (char *) malloc(sz * sizeof(char));
				sprintf(tmps, "%s%d", cnt_list[j].name, cnt_list[j].cnt);

				/*
				 * now compare to the list of reserved (by locked
				 * subcircuits) names
				 */
				for (k = 0; k < lock_cnt; k++) {
					if (strcmp(PCB_UNKNOWN(locked_subc_list[k]->refdes), tmps) == 0) {
						ok = 0;
						break;
					}
				}

			}
			while (!ok);

			if (strcmp(tmps, or_empty(subc_list[i]->refdes)) != 0) {
				fprintf(out, "*RENAME* \"%s\" \"%s\"\n", or_empty(subc_list[i]->refdes), tmps);

				/* add this rename to our table of renames so we can update the netlist */
				was[c_cnt] = pcb_strdup(or_empty(subc_list[i]->refdes));
				is[c_cnt] = pcb_strdup(tmps);
				c_cnt++;

				pcb_undo_add_obj_to_change_name(PCB_OBJ_SUBC, NULL, NULL, subc_list[i], (char *)or_empty(subc_list[i]->refdes));

				pcb_chg_obj_name(PCB_OBJ_SUBC, subc_list[i], NULL, NULL, tmps);
				changed = pcb_true;

				/* we don't free tmps in this case because it is used */
			}
			else
				free(tmps);
		}
		else {
			/* If there is no refdes, maybe just spit out a warning */
			pcb_fprintf(out, "*WARN* Subcircuit at %$md has no name.\n", ox_list[i], oy_list[i]);
		}
	}

	fclose(out);

	/* restore the unique flag setting */
	conf_update(NULL, -1);

	if (changed) {

		/* iterate over each net */
		for (i = 0; i < PCB->NetlistLib[WTF].MenuN; i++) {

			/* iterate over each pin on the net */
			for (j = 0; j < PCB->NetlistLib[WTF].Menu[i].EntryN; j++) {

				/* figure out the pin number part from strings like U3-21 */
				tmps = pcb_strdup(PCB->NetlistLib[WTF].Menu[i].Entry[j].ListEntry);
				for (k = 0; tmps[k] && tmps[k] != '-'; k++);
				tmps[k] = '\0';
				pin = tmps + k + 1;

				/* iterate over the list of changed reference designators */
				for (k = 0; k < c_cnt; k++) {
					/*
					 * if the pin needs to change, change it and quit
					 * searching in the list.
					 */
					if (strcmp(tmps, was[k]) == 0) {
						char *buffer;
						free((char*)PCB->NetlistLib[WTF].Menu[i].Entry[j].ListEntry);
						buffer = (char *) malloc((strlen(is[k]) + 1 + strlen(pin) + 1) * sizeof(char));
						sprintf(buffer, "%s-%s", is[k], pin);
						PCB->NetlistLib[WTF].Menu[i].Entry[j].ListEntry = buffer;
						k = c_cnt;
					}

				}
				free(tmps);
			}
		}
		for (k = 0; k < c_cnt; k++) {
			free(was[k]);
			free(is[k]);
		}

		pcb_netlist_changed(0);
		pcb_undo_inc_serial();
		pcb_board_set_changed_flag(pcb_true);
	}

	free(locked_subc_list);
	free(subc_list);
	free(ox_list);
	free(oy_list);
	free(cnt_list);
	PCB_ACT_IRES(0);
	return 0;
}

extern const char pcb_acts_RenumberBlock[];
extern const char pcb_acth_RenumberBlock[];
fgw_error_t pcb_act_RenumberBlock(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv);

extern const char pcb_acts_RenumberBuffer[];
extern const char pcb_acth_RenumberBuffer[];
fgw_error_t pcb_act_RenumberBuffer(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv);


static const char *renumber_cookie = "renumber plugin";

pcb_action_t renumber_action_list[] = {
	{"Renumber", pcb_act_Renumber, pcb_acth_Renumber, pcb_acts_Renumber},
	{"RenumberBlock", pcb_act_RenumberBlock, pcb_acth_RenumberBlock, pcb_acts_RenumberBlock},
	{"RenumberBuffer", pcb_act_RenumberBuffer, pcb_acth_RenumberBuffer, pcb_acts_RenumberBuffer}
};

int pplg_check_ver_renumber(int ver_needed) { return 0; }

void pplg_uninit_renumber(void)
{
	pcb_remove_actions_by_cookie(renumber_cookie);
}

PCB_REGISTER_ACTIONS(renumber_action_list, renumber_cookie)

#include "dolists.h"
int pplg_init_renumber(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(renumber_action_list, renumber_cookie)
	return 0;
}

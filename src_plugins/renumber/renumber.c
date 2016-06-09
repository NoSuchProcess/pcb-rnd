/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"
#include "global.h"
#include "data.h"
#include "action_helper.h"
#include "change.h"
#include "error.h"
#include "undo.h"
#include "misc.h"
#include "set.h"
#include "plugins.h"
#include "hid_actions.h"
#include "conf_core.h"

#include "pcb-printf.h"

/* --------------------------------------------------------------------------- */

static const char renumber_syntax[] = "Renumber()\n" "Renumber(filename)";

static const char renumber_help[] =
	"Renumber all elements.  The changes will be recorded to filename\n"
	"for use in backannotating these changes to the schematic.";

/* %start-doc actions Renumber

%end-doc */

#define WTF 0

static int ActionRenumber(int argc, char **argv, Coord x, Coord y)
{
	bool changed = false;
	ElementTypePtr *element_list;
	ElementTypePtr *locked_element_list;
	unsigned int i, j, k, cnt, lock_cnt;
	unsigned int tmpi;
	size_t sz;
	char *tmps;
	char *name;
	FILE *out;
	static char *default_file = NULL;
	size_t cnt_list_sz = 100;
	struct _cnt_list {
		char *name;
		unsigned int cnt;
	} *cnt_list;
	char **was, **is, *pin;
	unsigned int c_cnt = 0, numele;
	int ok;
	int free_name = 0;

	if (argc < 1) {
		/*
		 * We deal with the case where name already exists in this
		 * function so the GUI doesn't need to deal with it 
		 */
		name = gui->fileselect(_("Save Renumber Annotation File As ..."),
													 _("Choose a file to record the renumbering to.\n"
														 "This file may be used to back annotate the\n"
														 "change to the schematics.\n"), default_file, ".eco", "eco", 0);

		free_name = 1;
	}
	else
		name = argv[0];

	if (default_file) {
		free(default_file);
		default_file = NULL;
	}

	if (name && *name) {
		default_file = strdup(name);
	}

	if ((out = fopen(name, "r"))) {
		fclose(out);
		if (!gui->confirm_dialog(_("File exists!  Ok to overwrite?"), 0)) {
			if (free_name && name)
				free(name);
			return 0;
		}
	}

	if ((out = fopen(name, "w")) == NULL) {
		Message(_("Could not open %s\n"), name);
		if (free_name && name)
			free(name);
		return 1;
	}

	if (free_name && name)
		free(name);

	fprintf(out, "*COMMENT* PCB Annotation File\n");
	fprintf(out, "*FILEVERSION* 20061031\n");

	/*
	 * Make a first pass through all of the elements and sort them out
	 * by location on the board.  While here we also collect a list of
	 * locked elements.
	 *
	 * We'll actually renumber things in the 2nd pass.
	 */
	numele = elementlist_length(&PCB->Data->Element);
	element_list = (ElementType **) calloc(numele, sizeof(ElementTypePtr));
	locked_element_list = (ElementType **) calloc(numele, sizeof(ElementTypePtr));
	was = (char **) calloc(numele, sizeof(char *));
	is = (char **) calloc(numele, sizeof(char *));
	if (element_list == NULL || locked_element_list == NULL || was == NULL || is == NULL) {
		fprintf(stderr, "calloc() failed in %s\n", __FUNCTION__);
		exit(1);
	}


	cnt = 0;
	lock_cnt = 0;
	ELEMENT_LOOP(PCB->Data);
	{
		if (TEST_FLAG(LOCKFLAG, element->Name) || TEST_FLAG(LOCKFLAG, element)) {
			/* 
			 * add to the list of locked elements which we won't try to
			 * renumber and whose reference designators are now reserved.
			 */
			pcb_fprintf(out,
									"*WARN* Element \"%s\" at %$md is locked and will not be renumbered.\n",
									UNKNOWN(NAMEONPCB_NAME(element)), element->MarkX, element->MarkY);
			locked_element_list[lock_cnt] = element;
			lock_cnt++;
		}

		else {
			/* count of devices which will be renumbered */
			cnt++;

			/* search for correct position in the list */
			i = 0;
			while (element_list[i] && element->MarkY > element_list[i]->MarkY)
				i++;

			/* 
			 * We have found the position where we have the first element that
			 * has the same Y value or a lower Y value.  Now move forward if
			 * needed through the X values
			 */
			while (element_list[i]
						 && element->MarkY == element_list[i]->MarkY && element->MarkX > element_list[i]->MarkX)
				i++;

			for (j = cnt - 1; j > i; j--) {
				element_list[j] = element_list[j - 1];
			}
			element_list[i] = element;
		}
	}
	END_LOOP;


	/* 
	 * Now that the elements are sorted by board position, we go through
	 * and renumber them.
	 */

	/* 
	 * turn off the flag which requires unique names so it doesn't get
	 * in our way.  When we're done with the renumber we will have unique
	 * names.
	 */

	conf_force_set_bool(conf_core.editor.unique_names, 0);

	cnt_list = (struct _cnt_list *) calloc(cnt_list_sz, sizeof(struct _cnt_list));
	for (i = 0; i < cnt; i++) {
		/* If there is no refdes, maybe just spit out a warning */
		if (NAMEONPCB_NAME(element_list[i])) {
			/* figure out the prefix */
			tmps = strdup(NAMEONPCB_NAME(element_list[i]));
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
					fprintf(stderr, "realloc failed() in %s\n", __FUNCTION__);
					exit(1);
				}
				/* zero out the memory that we added */
				for (tmpi = j; tmpi < cnt_list_sz; tmpi++) {
					cnt_list[tmpi].name = NULL;
					cnt_list[tmpi].cnt = 0;
				}
			}

			/* 
			 * start a new counter if we don't have a counter for this
			 * prefix 
			 */
			if (!cnt_list[j].name) {
				cnt_list[j].name = strdup(tmps);
				cnt_list[j].cnt = 0;
			}

			/*
			 * check to see if the new refdes is already used by a
			 * locked element
			 */
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
				 * elements) names 
				 */
				for (k = 0; k < lock_cnt; k++) {
					if (strcmp(UNKNOWN(NAMEONPCB_NAME(locked_element_list[k])), tmps) == 0) {
						ok = 0;
						break;
					}
				}

			}
			while (!ok);

			if (strcmp(tmps, NAMEONPCB_NAME(element_list[i])) != 0) {
				fprintf(out, "*RENAME* \"%s\" \"%s\"\n", NAMEONPCB_NAME(element_list[i]), tmps);

				/* add this rename to our table of renames so we can update the netlist */
				was[c_cnt] = strdup(NAMEONPCB_NAME(element_list[i]));
				is[c_cnt] = strdup(tmps);
				c_cnt++;

				AddObjectToChangeNameUndoList(ELEMENT_TYPE, NULL, NULL, element_list[i], NAMEONPCB_NAME(element_list[i]));

				ChangeObjectName(ELEMENT_TYPE, element_list[i], NULL, NULL, tmps);
				changed = true;

				/* we don't free tmps in this case because it is used */
			}
			else
				free(tmps);
		}
		else {
			pcb_fprintf(out, "*WARN* Element at %$md has no name.\n", element_list[i]->MarkX, element_list[i]->MarkY);
		}

	}

	fclose(out);

	/* restore the unique flag setting */
	conf_update(NULL);

	if (changed) {

		/* update the netlist */
		AddNetlistLibToUndoList(PCB->NetlistLib);

		/* iterate over each net */
		for (i = 0; i < PCB->NetlistLib[WTF].MenuN; i++) {

			/* iterate over each pin on the net */
			for (j = 0; j < PCB->NetlistLib[WTF].Menu[i].EntryN; j++) {

				/* figure out the pin number part from strings like U3-21 */
				tmps = strdup(PCB->NetlistLib[WTF].Menu[i].Entry[j].ListEntry);
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
						free(PCB->NetlistLib[WTF].Menu[i].Entry[j].ListEntry);
						PCB->NetlistLib[WTF].Menu[i].Entry[j].ListEntry = (char *) malloc((strlen(is[k]) + strlen(pin) + 2) * sizeof(char));
						sprintf(PCB->NetlistLib[WTF].Menu[i].Entry[j].ListEntry, "%s-%s", is[k], pin);
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

		NetlistChanged(0);
		IncrementUndoSerialNumber();
		SetChangedFlag(true);
	}

	free(locked_element_list);
	free(element_list);
	free(cnt_list);
	return 0;
}

static const char *renumber_cookie = "renumber plugin";

HID_Action renumber_action_list[] = {
	{"Renumber", 0, ActionRenumber,
	 renumber_help, renumber_syntax}
};

static void hid_renumber_uninit(void)
{
	hid_remove_actions_by_cookie(renumber_cookie);
}

REGISTER_ACTIONS(renumber_action_list, renumber_cookie)

#include "dolists.h"
pcb_uninit_t hid_renumber_init(void)
{
	REGISTER_ACTIONS(renumber_action_list, renumber_cookie)
	return hid_renumber_uninit;
}


/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2015 Tibor 'Igor2' Palinkas
 *
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
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

#include "rats_patch.h"
#include "genht/htsp.h"
#include "genht/hash.h"
#include "create.h"

#include "config.h"
#include "data.h"
#include "error.h"
#include "buffer.h"
#include "remove.h"
#include "copy.h"
#include "compat_misc.h"

static void rats_patch_remove(PCBTypePtr pcb, rats_patch_line_t * n, int do_free);

void rats_patch_append(PCBTypePtr pcb, rats_patch_op_t op, const char *id, const char *a1, const char *a2)
{
	rats_patch_line_t *n;

	n = malloc(sizeof(rats_patch_line_t));

	n->op = op;
	n->id = pcb_strdup(id);
	n->arg1.net_name = pcb_strdup(a1);
	if (a2 != NULL)
		n->arg2.attrib_val = pcb_strdup(a2);
	else
		n->arg2.attrib_val = NULL;

	/* link in */
	n->prev = pcb->NetlistPatchLast;
	if (pcb->NetlistPatches != NULL) {
		pcb->NetlistPatchLast->next = n;
		pcb->NetlistPatchLast = n;
	}
	else
		pcb->NetlistPatchLast = pcb->NetlistPatches = n;
	n->next = NULL;
}

static void rats_patch_free_fields(rats_patch_line_t *n)
{
	if (n->id != NULL)
		free(n->id);
	if (n->arg1.net_name != NULL)
		free(n->arg1.net_name);
	if (n->arg2.attrib_val != NULL)
		free(n->arg2.attrib_val);
}

void rats_patch_destroy(PCBTypePtr pcb)
{
	rats_patch_line_t *n, *next;

	for(n = pcb->NetlistPatches; n != NULL; n = next) {
		next = n->next;
		rats_patch_free_fields(n);
		free(n);
	}
}

void rats_patch_append_optimize(PCBTypePtr pcb, rats_patch_op_t op, const char *id, const char *a1, const char *a2)
{
	rats_patch_op_t seek_op;
	rats_patch_line_t *n;

	switch (op) {
	case RATP_ADD_CONN:
		seek_op = RATP_DEL_CONN;
		break;
	case RATP_DEL_CONN:
		seek_op = RATP_ADD_CONN;
		break;
	case RATP_CHANGE_ATTRIB:
		seek_op = RATP_CHANGE_ATTRIB;
		break;
	}

	/* keep the list clean: remove the last operation that becomes obsolete by the new one */
	for (n = pcb->NetlistPatchLast; n != NULL; n = n->prev) {
		if ((n->op == seek_op) && (strcmp(n->id, id) == 0)) {
			switch (op) {
			case RATP_CHANGE_ATTRIB:
				if (strcmp(n->arg1.attrib_name, a1) != 0)
					break;
				rats_patch_remove(pcb, n, 1);
				goto quit;
			case RATP_ADD_CONN:
			case RATP_DEL_CONN:
				if (strcmp(n->arg1.net_name, a1) != 0)
					break;
				rats_patch_remove(pcb, n, 1);
				goto quit;
			}
		}
	}

quit:;
	rats_patch_append(pcb, op, id, a1, a2);
}

/* Unlink n from the list; if do_free is non-zero, also free fields and n */
static void rats_patch_remove(PCBTypePtr pcb, rats_patch_line_t * n, int do_free)
{
	/* if we are the first or last... */
	if (n == pcb->NetlistPatches)
		pcb->NetlistPatches = n->next;
	if (n == pcb->NetlistPatchLast)
		pcb->NetlistPatchLast = n->prev;

	/* extra ifs, just in case n is already unlinked */
	if (n->prev != NULL)
		n->prev->next = n->next;
	if (n->next != NULL)
		n->next->prev = n->prev;

	if (do_free) {
		rats_patch_free_fields(n);
		free(n);
	}
}

static void netlist_free(LibraryTypePtr dst)
{
	int n, p;

	if (dst->Menu == NULL)
		return;

	for (n = 0; n < dst->MenuN; n++) {
		LibraryMenuTypePtr dmenu = &dst->Menu[n];
		free(dmenu->Name);
		for (p = 0; p < dmenu->EntryN; p++) {
			LibraryEntryTypePtr dentry = &dmenu->Entry[p];
			free(dentry->ListEntry);
		}
		free(dmenu->Entry);
	}
	free(dst->Menu);
	dst->Menu = NULL;
}

static void netlist_copy(LibraryTypePtr dst, LibraryTypePtr src)
{
	int n, p;
	dst->MenuMax = dst->MenuN = src->MenuN;
	if (src->MenuN != 0) {
		dst->Menu = calloc(sizeof(LibraryMenuType), dst->MenuN);
		for (n = 0; n < src->MenuN; n++) {
			LibraryMenuTypePtr smenu = &src->Menu[n];
			LibraryMenuTypePtr dmenu = &dst->Menu[n];
/*		fprintf(stderr, "Net %d name='%s': ", n, smenu->Name+2);*/
			dmenu->Name = pcb_strdup(smenu->Name);
			dmenu->EntryMax = dmenu->EntryN = smenu->EntryN;
			dmenu->Entry = calloc(sizeof(LibraryEntryType), dmenu->EntryN);
			dmenu->flag = smenu->flag;
			dmenu->internal = smenu->internal;
			for (p = 0; p < smenu->EntryN; p++) {
			LibraryEntryTypePtr sentry = &smenu->Entry[p];
				LibraryEntryTypePtr dentry = &dmenu->Entry[p];
				dentry->ListEntry = pcb_strdup(sentry->ListEntry);
				dentry->ListEntry_dontfree = 0;
/*			fprintf (stderr, " '%s'/%p", dentry->ListEntry, dentry->ListEntry);*/
			}
/*		fprintf(stderr, "\n");*/
		}
	}
	else
		dst->Menu = NULL;
}


int rats_patch_apply_conn(PCBTypePtr pcb, rats_patch_line_t * patch, int del)
{
	int n;

	for (n = 0; n < pcb->NetlistLib[NETLIST_EDITED].MenuN; n++) {
		LibraryMenuTypePtr menu = &pcb->NetlistLib[NETLIST_EDITED].Menu[n];
		if (strcmp(menu->Name + 2, patch->arg1.net_name) == 0) {
			int p;
			for (p = 0; p < menu->EntryN; p++) {
				LibraryEntryTypePtr entry = &menu->Entry[p];
/*				fprintf (stderr, "C'%s'/%p '%s'/%p\n", entry->ListEntry, entry->ListEntry, patch->id, patch->id);*/
				if (strcmp(entry->ListEntry, patch->id) == 0) {
					if (del) {
						/* want to delete and it's on the list */
						memmove(&menu->Entry[p], &menu->Entry[p + 1], (menu->EntryN - p - 1) * sizeof(LibraryEntryType));
						menu->EntryN--;
						return 0;
					}
					/* want to add, and pin is on the list -> already added */
					return 1;
				}
			}
			/* If we got here, pin is not on the list */
			if (del)
				return 1;

			/* Wanted to add, let's add it */
			CreateNewConnection(menu, (char *) patch->id);
			return 0;
		}
	}

	/* couldn't find the net: create it */
	{
		LibraryMenuType *net = NULL;
		net = CreateNewNet(&pcb->NetlistLib[NETLIST_EDITED], patch->arg1.net_name, NULL);
		if (net == NULL)
			return 1;
		CreateNewConnection(net, (char *) patch->id);
	}
	return 0;
}


int rats_patch_apply(PCBTypePtr pcb, rats_patch_line_t * patch)
{
	switch (patch->op) {
	case RATP_ADD_CONN:
		return rats_patch_apply_conn(pcb, patch, 0);
	case RATP_DEL_CONN:
		return rats_patch_apply_conn(pcb, patch, 1);
	case RATP_CHANGE_ATTRIB:
#warning TODO: just check wheter it is still valid
		break;
	}
	return 0;
}

void rats_patch_make_edited(PCBTypePtr pcb)
{
	rats_patch_line_t *n;

	netlist_free(&(pcb->NetlistLib[NETLIST_EDITED]));
	netlist_copy(&(pcb->NetlistLib[NETLIST_EDITED]), &(pcb->NetlistLib[NETLIST_INPUT]));
	for (n = pcb->NetlistPatches; n != NULL; n = n->next)
		rats_patch_apply(pcb, n);
}


LibraryMenuTypePtr rats_patch_find_net4pin(PCBTypePtr pcb, const char *pin)
{
	int n;

	for (n = 0; n < pcb->NetlistLib[NETLIST_EDITED].MenuN; n++) {
		LibraryMenuTypePtr menu = &pcb->NetlistLib[NETLIST_EDITED].Menu[n];
		int p;
		for (p = 0; p < menu->EntryN; p++) {
			LibraryEntryTypePtr entry = &menu->Entry[p];
			if (strcmp(entry->ListEntry, pin) == 0)
				return menu;
		}
	}
	return NULL;
}

static LibraryMenuTypePtr rats_patch_find_net(PCBTypePtr pcb, const char *netname, int listidx)
{
	int n;

	for (n = 0; n < pcb->NetlistLib[listidx].MenuN; n++) {
		LibraryMenuTypePtr menu = &pcb->NetlistLib[listidx].Menu[n];
		if (strcmp(menu->Name + 2, netname) == 0)
			return menu;
	}
	return NULL;
}

int rats_patch_fexport(PCBTypePtr pcb, FILE * f, int fmt_pcb)
{
	rats_patch_line_t *n;
	const char *q, *po, *pc, *line_prefix;

	if (fmt_pcb) {
		q = "\"";
		po = "(";
		pc = ")";
		line_prefix = "\t";
	}
	else {
		q = "";
		po = " ";
		pc = "";
		line_prefix = "";
	}

	if (!fmt_pcb) {
		htsp_t *seen;
		seen = htsp_alloc(strhash, strkeyeq);

		/* have to print net_info lines */
		for (n = pcb->NetlistPatches; n != NULL; n = n->next) {
			switch (n->op) {
			case RATP_ADD_CONN:
			case RATP_DEL_CONN:
				if (htsp_get(seen, n->arg1.net_name) == NULL) {
					LibraryMenuTypePtr net;
					int p;

					net = rats_patch_find_net(pcb, n->arg1.net_name, NETLIST_INPUT);
					printf("net: '%s' %p\n", n->arg1.net_name, net);
					if (net != NULL) {
						htsp_set(seen, n->arg1.net_name, net);
						fprintf(f, "%snet_info%s%s%s%s", line_prefix, po, q, n->arg1.net_name, q);
						for (p = 0; p < net->EntryN; p++) {
							LibraryEntryTypePtr entry = &net->Entry[p];
							fprintf(f, " %s%s%s", q, entry->ListEntry, q);
						}
						fprintf(f, "%s\n", pc);
					}
				}
			case RATP_CHANGE_ATTRIB:
				break;
			}
		}
		htsp_free(seen);
	}


	for (n = pcb->NetlistPatches; n != NULL; n = n->next) {
		switch (n->op) {
		case RATP_ADD_CONN:
			fprintf(f, "%sadd_conn%s%s%s%s %s%s%s%s\n", line_prefix, po, q, n->id, q, q, n->arg1.net_name, q, pc);
			break;
		case RATP_DEL_CONN:
			fprintf(f, "%sdel_conn%s%s%s%s %s%s%s%s\n", line_prefix, po, q, n->id, q, q, n->arg1.net_name, q, pc);
			break;
		case RATP_CHANGE_ATTRIB:
			fprintf(f, "%schange_attrib%s%s%s%s %s%s%s %s%s%s%s\n", line_prefix, po, q, n->id, q, q, n->arg1.attrib_name, q, q,
							n->arg2.attrib_val, q, pc);
			break;
		}
	}
	return 0;
}

/* ---------------------------------------------------------------- */
static const char replacefootprint_syntax[] = "ReplaceFootprint()\n";

static const char replacefootprint_help[] = "Replace the footprint of the selected components with the footprint specified.";

static int ReplaceFootprint(int argc, char **argv, Coord x, Coord y)
{
	char *a[4];
	char *fpname;
	int found = 0;

	/* check if we have elements selected and quit if not */
	ELEMENT_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_SELECTED, element)) {
			found = 1;
			break;
		}
	}
	END_LOOP;

	if (!(found)) {
		Message(PCB_MSG_DEFAULT, "ReplaceFootprint works on selected elements, please select elements first!\n");
		return 1;
	}

	/* fetch the name of the new footprint */
	if (argc == 0) {
		fpname = gui->prompt_for("Footprint name", "");
		if (fpname == NULL) {
			Message(PCB_MSG_DEFAULT, "No footprint name supplied\n");
			return 1;
		}
	}
	else
		fpname = argv[0];

	/* check if the footprint is available */
	a[0] = fpname;
	a[1] = NULL;
	if (LoadFootprint(1, a, x, y) != 0) {
		Message(PCB_MSG_DEFAULT, "Can't load footprint %s\n", fpname);
		return 1;
	}


	/* action: replace selected elements */
	ELEMENT_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_SELECTED, element)) {
			a[0] = fpname;
			a[1] = element->Name[1].TextString;
			a[2] = element->Name[2].TextString;
			a[3] = NULL;
			LoadFootprint(3, a, element->MarkX, element->MarkY);
			CopyPastebufferToLayout(element->MarkX, element->MarkY);
			rats_patch_append_optimize(PCB, RATP_CHANGE_ATTRIB, a[1], "footprint", fpname);
			RemoveElement(element);
		}
	}
	END_LOOP;
	return 0;
}

static const char savepatch_syntax[] = "SavePatch(filename)";

static const char savepatch_help[] = "Save netlist patch for back annotation.";

/* %start-doc actions SavePatch
Save netlist patch for back annotation.
%end-doc */
static int SavePatch(int argc, char **argv, Coord x, Coord y)
{
	const char *fn;
	FILE *f;

	if (argc < 1) {
		char *default_file;

		if (PCB->Filename != NULL) {
			char *end;
			int len;
			len = strlen(PCB->Filename);
			default_file = malloc(len + 8);
			memcpy(default_file, PCB->Filename, len + 1);
			end = strrchr(default_file, '.');
			if ((end == NULL) || (strcasecmp(end, ".pcb") != 0))
				end = default_file + len;
			strcpy(end, ".bap");
		}
		else
			default_file = pcb_strdup("unnamed.bap");

		fn = gui->fileselect(_("Save netlist patch as ..."),
												 _("Choose a file to save netlist patch to\n"
													 "for back annotation\n"), default_file, ".bap", "patch", 0);

		free(default_file);
	}
	else
		fn = argv[0];
	f = fopen(fn, "w");
	if (f == NULL) {
		Message(PCB_MSG_DEFAULT, "Can't open netlist patch file %s for writing\n", fn);
		return 1;
	}
	rats_patch_fexport(PCB, f, 0);
	fclose(f);
	return 0;
}

HID_Action rats_patch_action_list[] = {
	{"ReplaceFootprint", 0, ReplaceFootprint,
	 replacefootprint_help, replacefootprint_syntax}
	,
	{"SavePatch", 0, SavePatch,
	 savepatch_help, savepatch_syntax}
};

REGISTER_ACTIONS(rats_patch_action_list, NULL)

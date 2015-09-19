/* $Id$ */

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

/* Change History:
 * Started 6/10/97
 * Added support for minimum length rat lines 6/13/97
 * rat lines to nearest line/via 8/29/98
 * support for netlist window 10/24/98
 */

#include "rats_patch.h"

static void rats_patch_remove(PCBTypePtr pcb, rats_patch_line_t *n, int do_free);

void rats_patch_append(PCBTypePtr pcb, rats_patch_op_t op, const char *id, const char *a1, const char *a2)
{
	rats_patch_line_t *n;

	n = malloc(sizeof(rats_patch_line_t));

	n->op = op;
	n->id = strdup(id);
	n->arg1.net_name = strdup(a1);
	if (a2 != NULL)
		n->arg2.attrib_val = strdup(a2);
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

void rats_patch_append_optimize(PCBTypePtr pcb, rats_patch_op_t op, const char *id, const char *a1, const char *a2)
{
	rats_patch_op_t seek_op;
	rats_patch_line_t *n;

	switch(op) {
		case RATP_ADD_CONN:      seek_op = RATP_DEL_CONN; break;
		case RATP_DEL_CONN:      seek_op = RATP_ADD_CONN; break;
		case RATP_CHANGE_ATTRIB: seek_op = RATP_CHANGE_ATTRIB; break;
	}

	/* keep the list clean: remove the last operation that becomes obsolete by the new one */
	for(n = pcb->NetlistPatchLast; n != NULL; n = n->prev) {
		if ((n->op == seek_op) && (strcmp(n->id, id) == 0)) {
			switch(op) {
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
static void rats_patch_remove(PCBTypePtr pcb, rats_patch_line_t *n, int do_free)
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
		if (n->id != NULL)
			free(n->id);
		if (n->arg1.net_name != NULL)
			free(n->arg1.net_name);
		if (n->arg2.attrib_val != NULL)
			free(n->arg2.attrib_val);
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
	dst->Menu = calloc(sizeof(LibraryMenuType), dst->MenuN);
	for (n = 0; n < src->MenuN; n++) {
		LibraryMenuTypePtr smenu = &src->Menu[n];
		LibraryMenuTypePtr dmenu = &dst->Menu[n];
/*		fprintf(stderr, "Net %d name='%s': ", n, smenu->Name+2);*/
		dmenu->Name = strdup(smenu->Name);
		dmenu->EntryMax = dmenu->EntryN = smenu->EntryN;
		dmenu->Entry = calloc(sizeof(LibraryEntryType), dmenu->EntryN);
		dmenu->flag = smenu->flag;
		dmenu->internal = smenu->internal;
		for (p = 0; p < smenu->EntryN; p++) {
			LibraryEntryTypePtr sentry = &smenu->Entry[p];
			LibraryEntryTypePtr dentry = &dmenu->Entry[p];
			dentry->ListEntry = strdup(sentry->ListEntry);
/*			fprintf (stderr, " '%s'/%p", dentry->ListEntry, dentry->ListEntry);*/
		}
/*		fprintf(stderr, "\n");*/
	}
}


int rats_patch_apply_conn(PCBTypePtr pcb, rats_patch_line_t *patch, int del)
{
	int n;

	for (n = 0; n < pcb->NetlistLib[NETLIST_EDITED].MenuN; n++) {
		LibraryMenuTypePtr menu = &pcb->NetlistLib[NETLIST_EDITED].Menu[n];
		if (strcmp(menu->Name+2, patch->arg1.net_name) == 0) {
			int p;
			for (p = 0; p < menu->EntryN; p++) {
				LibraryEntryTypePtr entry = &menu->Entry[p];
/*				fprintf (stderr, "C'%s'/%p '%s'/%p\n", entry->ListEntry, entry->ListEntry, patch->id, patch->id);*/
				if (strcmp(entry->ListEntry, patch->id) == 0) {
					if (del) {
						/* want to delete and it's on the list */
						memmove(&menu->Entry[p], &menu->Entry[p+1], (menu->EntryN-p-1) * sizeof(LibraryEntryType));
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
			CreateNewConnection(menu, (char *)patch->id);
			return 0;
		}
	}

	/* couldn't find the net */
	return 1;
}


int rats_patch_apply(PCBTypePtr pcb, rats_patch_line_t *patch)
{
	switch(patch->op) {
		case RATP_ADD_CONN: return rats_patch_apply_conn(pcb, patch, 0);
		case RATP_DEL_CONN: return rats_patch_apply_conn(pcb, patch, 1);
		case RATP_CHANGE_ATTRIB:
#warning TODO: just check wheter it's still valid
			break;
	}
	return 0;
}

void rats_patch_make_edited(PCBTypePtr pcb)
{
	rats_patch_line_t *n;

	netlist_free(&(pcb->NetlistLib[NETLIST_EDITED]));
	netlist_copy(&(pcb->NetlistLib[NETLIST_EDITED]), &(pcb->NetlistLib[NETLIST_INPUT]));
	for(n = pcb->NetlistPatches; n != NULL; n = n->next)
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


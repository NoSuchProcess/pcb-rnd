/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2015..2019 Tibor 'Igor2' Palinkas
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

/* Utility functions for manipulating the old netlist - will be removed soon */

static void netlist_free_old(pcb_lib_t *dst)
{
	int n, p;

	if (dst->Menu == NULL)
		return;

	for (n = 0; n < dst->MenuN; n++) {
		pcb_lib_menu_t *dmenu = &dst->Menu[n];
		free(dmenu->Name);
		for (p = 0; p < dmenu->EntryN; p++) {
			pcb_lib_entry_t *dentry = &dmenu->Entry[p];
			free((char*)dentry->ListEntry);
		}
		free(dmenu->Entry);
	}
	free(dst->Menu);
	dst->Menu = NULL;
}

static void netlist_copy_old(pcb_lib_t *dst, pcb_lib_t *src)
{
	int n, p;
	dst->MenuMax = dst->MenuN = src->MenuN;
	if (src->MenuN != 0) {
		dst->Menu = calloc(sizeof(pcb_lib_menu_t), dst->MenuN);
		for (n = 0; n < src->MenuN; n++) {
			pcb_lib_menu_t *smenu = &src->Menu[n];
			pcb_lib_menu_t *dmenu = &dst->Menu[n];
/*		fprintf(stderr, "Net %d name='%s': ", n, smenu->Name+2);*/
			dmenu->Name = pcb_strdup(smenu->Name);
			dmenu->EntryMax = dmenu->EntryN = smenu->EntryN;
			dmenu->Entry = calloc(sizeof(pcb_lib_entry_t), dmenu->EntryN);
			dmenu->flag = smenu->flag;
			dmenu->internal = smenu->internal;
			for (p = 0; p < smenu->EntryN; p++) {
			pcb_lib_entry_t *sentry = &smenu->Entry[p];
				pcb_lib_entry_t *dentry = &dmenu->Entry[p];
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


int rats_patch_apply_conn_old(pcb_board_t *pcb, pcb_ratspatch_line_t * patch, int del)
{
	int n;

	for (n = 0; n < pcb->NetlistLib[PCB_NETLIST_EDITED].MenuN; n++) {
		pcb_lib_menu_t *menu = &pcb->NetlistLib[PCB_NETLIST_EDITED].Menu[n];
		if (strcmp(menu->Name + 2, patch->arg1.net_name) == 0) {
			int p;
			for (p = 0; p < menu->EntryN; p++) {
				pcb_lib_entry_t *entry = &menu->Entry[p];
/*				fprintf (stderr, "C'%s'/%p '%s'/%p\n", entry->ListEntry, entry->ListEntry, patch->id, patch->id);*/
				if (strcmp(entry->ListEntry, patch->id) == 0) {
					if (del) {
						/* want to delete and it's on the list */
						memmove(&menu->Entry[p], &menu->Entry[p + 1], (menu->EntryN - p - 1) * sizeof(pcb_lib_entry_t));
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
			pcb_lib_conn_new(menu, (char *) patch->id);
			return 0;
		}
	}

	/* couldn't find the net: create it */
	{
		pcb_lib_menu_t *net = NULL;
		net = pcb_lib_net_new(&pcb->NetlistLib[PCB_NETLIST_EDITED], patch->arg1.net_name, NULL);
		if (net == NULL)
			return 1;
		pcb_lib_conn_new(net, (char *) patch->id);
	}
	return 0;
}

static pcb_lib_menu_t *rats_patch_find_net_old(pcb_board_t *pcb, const char *netname, int listidx)
{
	int n;

	for (n = 0; n < pcb->NetlistLib[listidx].MenuN; n++) {
		pcb_lib_menu_t *menu = &pcb->NetlistLib[listidx].Menu[n];
		if (strcmp(menu->Name + 2, netname) == 0)
			return menu;
	}
	return NULL;
}

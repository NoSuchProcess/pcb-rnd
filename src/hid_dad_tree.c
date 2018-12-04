/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Non-inline utility functions for the DAD tree widget */

#include "config.h"
#include "hid_dad_tree.h"

/* recursively free a row list subtree */
static void pcb_dad_tree_free_rowlist(pcb_hid_attribute_t *attr, gdl_list_t *list)
{
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attr->enumerations;
	pcb_hid_row_t *r;

	while((r = gdl_first(list)) != NULL) {
		gdl_remove(list, r, link);
		pcb_dad_tree_free_rowlist(attr, &r->children);

		if (tree->hid_free_cb != NULL)
			tree->hid_free_cb(tree->attrib, tree->hid_ctx, r);

		if (tree->user_free_cb != NULL)
			tree->user_free_cb(tree->attrib, tree->hid_ctx, r);

		if (attr->pcb_hatt_flags & PCB_HATF_TREE_COL)
			free(r->path);

		free(r);
	}
}

/* Internal: free all rows and caches and the tree itself */
void pcb_dad_tree_free(pcb_hid_attribute_t *attr)
{
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attr->enumerations;
	htsp_uninit(&tree->paths);
	pcb_dad_tree_free_rowlist(attr, &tree->rows);
	free(tree);
}


/*** these shouldn't be in pcb_dad_tree.c, but they are so short that a new
     file would be overkill ***/

void pcb_hid_dad_close_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_dad_retovr_t *retovr = (pcb_dad_retovr_t *)attr->enumerations;

	retovr->valid = 1;
	retovr->value = attr->default_val.int_value;
	retovr->already_freed = 1;
	pcb_gui->attr_dlg_free(hid_ctx);
}


int pcb_hid_dad_run(void *hid_ctx, pcb_dad_retovr_t *retovr)
{
	int ret;

	retovr->already_freed = 0;
	retovr->valid = 0;
	ret = pcb_gui->attr_dlg_run(hid_ctx);
	if (retovr->valid)
		return retovr->value;
	return ret;
}

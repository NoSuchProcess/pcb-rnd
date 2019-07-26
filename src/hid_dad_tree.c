/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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
			tree->hid_free_cb(tree->attrib, tree->hid_wdata, r);

		if (tree->user_free_cb != NULL)
			tree->user_free_cb(tree->attrib, tree->hid_wdata, r);

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

void pcb_dad_tree_hide_all(pcb_hid_tree_t *tree, gdl_list_t *rowlist, int val)
{
	pcb_hid_row_t *r;
	for(r = gdl_first(rowlist); r != NULL; r = gdl_next(rowlist, r)) {
		r->hide = val;
		pcb_dad_tree_hide_all(tree, &r->children, val);
	}
}

void pcb_dad_tree_unhide_filter(pcb_hid_tree_t *tree, gdl_list_t *rowlist, int col, const char *text)
{
	pcb_hid_row_t *r, *pr;

	for(r = gdl_first(rowlist); r != NULL; r = gdl_next(rowlist, r)) {
		if (strstr(r->cell[col], text) != NULL) {
			pcb_dad_tree_hide_all(tree, &r->children, 0); /* if this is a node with children, show all children */
			for(pr = r; pr != NULL; pr = pcb_dad_tree_parent_row(tree, pr)) /* also show all parents so it is visible */
				pr->hide = 0;
		}
		pcb_dad_tree_unhide_filter(tree, &r->children, col, text);
	}
}


/*** these shouldn't be in pcb_dad_tree.c, but they are so short that a new
     file would be overkill ***/
void pcb_hid_dad_close(void *hid_ctx, pcb_dad_retovr_t *retovr, int retval)
{
	retovr->valid = 1;
	retovr->value = retval;
	pcb_gui->attr_dlg_free(hid_ctx);
}

void pcb_hid_dad_close_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_dad_retovr_t **retovr = (pcb_dad_retovr_t **)attr->enumerations;
	pcb_hid_dad_close(hid_ctx, *retovr, attr->val.lng);
}

int pcb_hid_dad_run(void *hid_ctx, pcb_dad_retovr_t *retovr)
{
	int ret;

	retovr->valid = 0;
	retovr->dont_free++;
	ret = pcb_gui->attr_dlg_run(hid_ctx);
	if (retovr->valid)
		ret = retovr->value;
	retovr->dont_free--;
	return ret;
}

pcb_hid_row_t *pcb_dad_tree_mkdirp(pcb_hid_tree_t *tree, char *path, char **cells)
{
	char *cell[2] = {NULL};
	pcb_hid_row_t *parent;
	char *last, *old;

	parent = htsp_get(&tree->paths, path);
	if (parent != NULL)
		return parent;

	last = strrchr(path, '/');

	if (last == NULL) {
		/* root dir */
		parent = htsp_get(&tree->paths, path);
		if (parent != NULL)
			return parent;
		cell[0] = pcb_strdup(path);
		return pcb_dad_tree_append(tree->attrib, NULL, cell);
	}

/* non-root-dir: get or create parent */
	*last = '\0';
	last++;
	parent = pcb_dad_tree_mkdirp(tree, path, NULL);

	if (cells == NULL) {
		cell[0] = pcb_strdup(last);
		return pcb_dad_tree_append_under(tree->attrib, parent, cell);
	}

	old = cell[0];
	cells[0] = pcb_strdup(last);
	free(old);
	return pcb_dad_tree_append_under(tree->attrib, parent, cells);
}

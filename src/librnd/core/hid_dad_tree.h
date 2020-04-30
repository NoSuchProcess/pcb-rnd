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

/* Inline helpers called by the DAD macros for the tree-table widget */

#ifndef RND_HID_DAD_TREE_H
#define RND_HID_DAD_TREE_H

#include <librnd/core/hid_attrib.h>
#include <librnd/core/hid_dad.h>
#include <assert.h>
#include <genvector/gds_char.h>
#include <genht/hash.h>
#include <genht/htsp.h>

/* recursively sets the hide flag on all nodes to val */
void rnd_dad_tree_hide_all(rnd_hid_tree_t *tree, gdl_list_t *rowlist, int val);

/* recursively unhide items that match text in the given column; parents are unhidden too */
void rnd_dad_tree_unhide_filter(rnd_hid_tree_t *tree, gdl_list_t *rowlist, int col, const char *text);

/* Recursively create the node and all parents in a tree. If cells is not NULL,
   the target path row is created with these cells, else only the first col
   is filled in. Temporarily modifies path (but changes it back) */
rnd_hid_row_t *rnd_dad_tree_mkdirp(rnd_hid_tree_t *tree, char *path, char **cells);

/* Internal: Allocate a new row and load the cells (but do not insert it anywhere) */
RND_INLINE rnd_hid_row_t *rnd_dad_tree_new_row(char **cols)
{
	int num_cols;
	rnd_hid_row_t *nrow;
	char **s, **o;
	for(s = cols, num_cols = 0; *s != NULL; s++, num_cols++);

	nrow = calloc(sizeof(rnd_hid_row_t) + sizeof(char *) * num_cols, 1);
	nrow->cols = num_cols;
	for(s = cols, o = nrow->cell; *s != NULL; s++, o++)
		*o = *s;

	return nrow;
}

RND_INLINE void rnd_dad_tree_free_row(rnd_hid_tree_t *tree, rnd_hid_row_t *row)
{
	int do_free_path = 0;
	/* do this before the user callback just in case row->path == row->cell[0]
	   and the user callback free's it */
	if (row->path != NULL) {
		htsp_pop(&tree->paths, row->path);
		do_free_path = (row->path != row->cell[0]); /* user_free_cb may set row->cell[0] to NULL */
	}

	if (tree->user_free_cb != NULL)
		tree->user_free_cb(tree->attrib, tree->hid_wdata, row);

	if (do_free_path)
		free(row->path);

	free(row);
}


RND_INLINE rnd_hid_row_t *rnd_dad_tree_parent_row(rnd_hid_tree_t *tree, rnd_hid_row_t *row)
{
	char *ptr = (char *)row->link.parent;
	if ((ptr == NULL) || ((gdl_list_t *)ptr == &tree->rows))
		return NULL;

	ptr -= offsetof(gdl_elem_t, parent);
	ptr -= offsetof(rnd_hid_row_t, children);
	return (rnd_hid_row_t *)ptr;
}

RND_INLINE rnd_hid_row_t *rnd_dad_tree_prev_row(rnd_hid_tree_t *tree, rnd_hid_row_t *row)
{
	return row->link.prev;
}

RND_INLINE rnd_hid_row_t *rnd_dad_tree_next_row(rnd_hid_tree_t *tree, rnd_hid_row_t *row)
{
	return row->link.next;
}

/* recursively build a full path of a tree node in path */
RND_INLINE void rnd_dad_tree_build_path(rnd_hid_tree_t *tree, gds_t *path, rnd_hid_row_t *row)
{
	rnd_hid_row_t *par = rnd_dad_tree_parent_row(tree, row);
	if (par != NULL)
		rnd_dad_tree_build_path(tree, path, par);
	if (path->used > 0)
		gds_append(path, '/');
	gds_append_str(path, row->cell[0]);
}

/* calculate path of a row and insert it in the tree hash */
RND_INLINE void rnd_dad_tree_set_hash(rnd_hid_attribute_t *attr, rnd_hid_row_t *row)
{
	rnd_hid_tree_t *tree = attr->wdata;
	if (attr->rnd_hatt_flags & RND_HATF_TREE_COL) {
		gds_t path;
		gds_init(&path);
		rnd_dad_tree_build_path(tree, &path, row);
		row->path = path.array;
	}
	else
		row->path = row->cell[0];
	htsp_set(&tree->paths, row->path, row);
}

/* allocate a new row and append it after aft; if aft is NULL, the new row is appended at the
   end of the list of entries in the root (== at the bottom of the list) */
RND_INLINE rnd_hid_row_t *rnd_dad_tree_append(rnd_hid_attribute_t *attr, rnd_hid_row_t *aft, char **cols)
{
	rnd_hid_tree_t *tree = attr->wdata;
	rnd_hid_row_t *nrow = rnd_dad_tree_new_row(cols);
	gdl_list_t *par; /* the list that is the common parent of aft and the new row */

	assert(attr == tree->attrib);

	if (aft == NULL) {
		par = &tree->rows;
		aft = gdl_last(par);
	}
	else
		par = aft->link.parent;

	gdl_insert_after(par, aft, nrow, link);
	rnd_dad_tree_set_hash(attr, nrow);
	if (tree->hid_insert_cb != NULL)
		tree->hid_insert_cb(tree->attrib, tree->hid_wdata, nrow);
	return nrow;
}

/* allocate a new row and inert it before bfr; if bfr is NULL, the new row is inserted at the
   beginning of the list of entries in the root (== at the top of the list) */
RND_INLINE rnd_hid_row_t *rnd_dad_tree_insert(rnd_hid_attribute_t *attr, rnd_hid_row_t *bfr, char **cols)
{
	rnd_hid_tree_t *tree = attr->wdata;
	rnd_hid_row_t *nrow = rnd_dad_tree_new_row(cols);
	gdl_list_t *par; /* the list that is the common parent of bfr and the new row */

	assert(attr == tree->attrib);

	if (bfr == NULL) {
		par = &tree->rows;
		bfr = gdl_first(par);
	}
	else
		par = bfr->link.parent;

	gdl_insert_before(par, bfr, nrow, link);
	rnd_dad_tree_set_hash(attr, nrow);
	if (tree->hid_insert_cb != NULL)
		tree->hid_insert_cb(tree->attrib, tree->hid_wdata, nrow);
	return nrow;
}

/* allocate a new row and append it under prn; if aft is NULL, the new row is appended at the
   end of the list of entries in the root (== at the bottom of the list) */
RND_INLINE rnd_hid_row_t *rnd_dad_tree_append_under(rnd_hid_attribute_t *attr, rnd_hid_row_t *prn, char **cols)
{
	rnd_hid_tree_t *tree = attr->wdata;
	rnd_hid_row_t *nrow = rnd_dad_tree_new_row(cols);
	gdl_list_t *par; /* the list that is the common parent of aft and the new row */

	assert(attr == tree->attrib);

	if (prn == NULL)
		par = &tree->rows;
	else
		par = &prn->children;

	gdl_append(par, nrow, link);
	rnd_dad_tree_set_hash(attr, nrow);
	if (tree->hid_insert_cb != NULL)
		tree->hid_insert_cb(tree->attrib, tree->hid_wdata, nrow);
	return nrow;
}

RND_INLINE int rnd_dad_tree_remove(rnd_hid_attribute_t *attr, rnd_hid_row_t *row)
{
	rnd_hid_tree_t *tree = attr->wdata;
	rnd_hid_row_t *r, *rn, *par = rnd_dad_tree_parent_row(tree, row);
	gdl_list_t *lst = (par == NULL) ? &tree->rows : &par->children;
	int res = 0;

	assert(attr == tree->attrib);

	/* recursively remove all children */
	for(r = gdl_first(&row->children); r != NULL; r = rn) {
		rn = gdl_next(&row->children, r);
		res |= rnd_dad_tree_remove(attr, r);
	}

	/* remove from gui */
	if (tree->hid_remove_cb != NULL)
		tree->hid_remove_cb(tree->attrib, tree->hid_wdata, row);
	else
		res = -1;

	/* remove from dad */
	gdl_remove(lst, row, link);
	rnd_dad_tree_free_row(tree, row);
	return res;
}

RND_INLINE void rnd_dad_tree_clear(rnd_hid_tree_t *tree)
{
	rnd_hid_row_t *r;
	for(r = gdl_first(&tree->rows); r != NULL; r = gdl_first(&tree->rows))
		rnd_dad_tree_remove(tree->attrib, r);
}

RND_INLINE rnd_hid_row_t *rnd_dad_tree_get_selected(rnd_hid_attribute_t *attr)
{
	rnd_hid_tree_t *tree = attr->wdata;

	assert(attr == tree->attrib);

	if (tree->hid_get_selected_cb == NULL)
		return NULL;

	return tree->hid_get_selected_cb(tree->attrib, tree->hid_wdata);
}

RND_INLINE void rnd_dad_tree_update_hide(rnd_hid_attribute_t *attr)
{
	rnd_hid_tree_t *tree = attr->wdata;

	assert(attr == tree->attrib);

	if (tree->hid_update_hide_cb != NULL)
		tree->hid_update_hide_cb(tree->attrib, tree->hid_wdata);
}

RND_INLINE int rnd_dad_tree_modify_cell(rnd_hid_attribute_t *attr, rnd_hid_row_t *row, int col, char *new_val)
{
	rnd_hid_tree_t *tree = attr->wdata;

	assert(attr == tree->attrib);

	if ((col < 0) || (col >= row->cols))
		return 0;

	if (col == 0) {
		htsp_pop(&tree->paths, row->path);
		free(row->path);
		row->path = NULL;
	}

	row->cell[col] = new_val;

	if (col == 0)
		rnd_dad_tree_set_hash(attr, row);

	if (tree->hid_modify_cb != NULL)
		tree->hid_modify_cb(tree->attrib, tree->hid_wdata, row, col);

	return 0;
}

RND_INLINE void rnd_dad_tree_jumpto(rnd_hid_attribute_t *attr, rnd_hid_row_t *row)
{
	rnd_hid_tree_t *tree = attr->wdata;

	assert(attr == tree->attrib);

	if (tree->hid_jumpto_cb != NULL)
		tree->hid_jumpto_cb(tree->attrib, tree->hid_wdata, row);
}

RND_INLINE void rnd_dad_tree_expcoll_(rnd_hid_tree_t *tree, rnd_hid_row_t *row, rnd_bool expanded, rnd_bool recursive)
{
	if (recursive) {
		rnd_hid_row_t *r;
		for(r = gdl_first(&row->children); r != NULL; r = gdl_next(&row->children, r))
			rnd_dad_tree_expcoll_(tree, r, expanded, recursive);
	}
	if (gdl_first(&row->children) != NULL)
		tree->hid_expcoll_cb(tree->attrib, tree->hid_wdata, row, expanded);
}

RND_INLINE void rnd_dad_tree_expcoll(rnd_hid_attribute_t *attr, rnd_hid_row_t *row, rnd_bool expanded, rnd_bool recursive)
{
	rnd_hid_tree_t *tree = attr->wdata;

	assert(attr == tree->attrib);

	if (tree->hid_expcoll_cb != NULL) {
		if (row == NULL) {
			for(row = gdl_first(&tree->rows); row != NULL; row = gdl_next(&tree->rows, row))
				rnd_dad_tree_expcoll_(tree, row, expanded, recursive);
		}
		else
			rnd_dad_tree_expcoll_(tree, row, expanded, recursive);
	}
}

#endif

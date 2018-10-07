#ifndef PCB_HID_DAD_TREE_H
#define PCB_HID_DAD_TREE_H

#include <genvector/gds_char.h>
#include <genht/hash.h>
#include <genht/htsp.h>

/* Internal: Allocate a new row and load the cells (but do not insert it anywhere) */
PCB_INLINE pcb_hid_row_t *pcb_dad_tree_new_row(char **cols)
{
	int num_cols;
	pcb_hid_row_t *nrow;
	char **s, **o;
	for(s = cols, num_cols = 0; *s != NULL; s++, num_cols++);

	nrow = calloc(sizeof(pcb_hid_row_t) + sizeof(char *) * num_cols, 1);
	nrow->cols = num_cols;
	for(s = cols, o = nrow->cell; *s != NULL; s++, o++)
		*o = *s;

	return nrow;
}

PCB_INLINE pcb_hid_row_t *pcb_dad_tree_parent_row(pcb_hid_tree_t *tree, pcb_hid_row_t *row)
{
	char *ptr = (char *)row->link.parent;
	if ((ptr == NULL) || ((gdl_list_t *)ptr == &tree->rows))
		return NULL;
	ptr -= offsetof(gdl_elem_t, parent);
	ptr -= offsetof(pcb_hid_row_t, link);
	return (pcb_hid_row_t *)ptr;
}

/* recursively build a full path of a tree node in path */
PCB_INLINE void pcb_dad_tree_build_path(pcb_hid_tree_t *tree, gds_t *path, pcb_hid_row_t *row)
{
	pcb_hid_row_t *par = pcb_dad_tree_parent_row(tree, row);
	if (par != NULL)
		pcb_dad_tree_build_path(tree, path, par);
	if (path->used > 0)
		gds_append(path, '/');
	gds_append_str(path, row->cell[0]);
}

/* calculate path of a row and insert it in the tree hash */
PCB_INLINE void pcb_dad_tree_set_hash(pcb_hid_attribute_t *attr, pcb_hid_row_t *row)
{
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attr->enumerations;
	if (attr->pcb_hatt_flags & PCB_HATF_TREE_COL) {
		gds_t path;
		gds_init(&path);
		pcb_dad_tree_build_path(tree, &path, row);
		row->path = path.array;
	}
	else
		row->path = row->cell[0];
	htsp_set(&tree->paths, row->path, row);
}

/* allocate a new row and append it after aft; if aft is NULL, the new row is appended at the
   end of the list of entries in the root (== at the bottom of the list) */
PCB_INLINE pcb_hid_row_t *pcb_dad_tree_append(pcb_hid_attribute_t *attr, pcb_hid_row_t *aft, char **cols)
{
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attr->enumerations;
	pcb_hid_row_t *nrow = pcb_dad_tree_new_row(cols);
	gdl_list_t *par; /* the list that is the common parent of aft and the new row */

	if (aft == NULL) {
		par = &tree->rows;
		aft = gdl_last(par);
	}
	else
		par = aft->link.parent;

	gdl_insert_after(par, aft, nrow, link);
	pcb_dad_tree_set_hash(attr, nrow);
	return nrow;
}

/* allocate a new row and inert it before bfr; if bfr is NULL, the new row is inserted at the
   beginning of the list of entries in the root (== at the top of the list) */
PCB_INLINE pcb_hid_row_t *pcb_dad_tree_insert(pcb_hid_attribute_t *attr, pcb_hid_row_t *bfr, char **cols)
{
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attr->enumerations;
	pcb_hid_row_t *nrow = pcb_dad_tree_new_row(cols);
	gdl_list_t *par; /* the list that is the common parent of bfr and the new row */

	if (bfr == NULL) {
		par = &tree->rows;
		bfr = gdl_first(par);
	}
	else
		par = bfr->link.parent;

	gdl_insert_before(par, bfr, nrow, link);
	pcb_dad_tree_set_hash(attr, nrow);
	return nrow;
}

#endif

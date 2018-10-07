/* Internal: Allocate a new row and load the cells (but do not insert it anywhere) */
PCB_INLINE pcb_hid_row_t *pcb_dad_tree_new_row(char **cols)
{
	int num_cols;
	pcb_hid_row_t *nrow;
	char **s, **o;
	for(s = cols, num_cols = 0; *s != NULL; s++, num_cols++);

	nrow = calloc(sizeof(pcb_hid_row_t) + sizeof(char *) * num_cols, 0);
	nrow->cols = num_cols;
	for(s = cols, o = nrow->cell; *s != NULL; s++, o++)
		*o = *s;

	return nrow;
}

/* allocate a new row and append it after aft; if aft is NULL, the new row is appended at the
   end of the list of entries in the root (== at the bottom of the list) */
PCB_INLINE pcb_hid_row_t *pcb_dad_tree_append(pcb_hid_tree_t *tree, pcb_hid_row_t *aft, char **cols)
{
	pcb_hid_row_t *nrow = pcb_dad_tree_new_row(cols);
	gdl_list_t *par; /* the list that is the common parent of aft and the new row */

	if (aft == NULL) {
		par = &tree->rows;
		aft = gdl_last(par);
	}
	else
		par = aft->link.parent;

	if (aft == NULL)
		gdl_append(par, nrow, link);
	else
		gdl_insert_after(par, aft, nrow, link);

	return nrow;
}

/* allocate a new row and inert it before bfr; if bfr is NULL, the new row is inserted at the
   beginning of the list of entries in the root (== at the top of the list) */
PCB_INLINE pcb_hid_row_t *pcb_dad_tree_insert(pcb_hid_tree_t *tree, pcb_hid_row_t *bfr, char **cols)
{
	pcb_hid_row_t *nrow = pcb_dad_tree_new_row(cols);
	gdl_list_t *par; /* the list that is the common parent of bfr and the new row */

	if (bfr == NULL) {
		par = &tree->rows;
		bfr = gdl_first(par);
	}
	else
		par = bfr->link.parent;

	if (bfr == NULL)
		gdl_insert(par, nrow, link);
	else
		gdl_insert_before(par, bfr, nrow, link);

	return nrow;
}

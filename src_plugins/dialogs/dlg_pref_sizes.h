#ifndef PCB_DLG_PREF_SIZES_H
#define PCB_DLG_PREF_SIZES_H

typedef struct {
	int wwidth, wheight;
	int lock; /* a change in on the dialog box causes a change on the board but this shouldn't in turn casue a changein the dialog */
} pref_sizes_t;

#endif

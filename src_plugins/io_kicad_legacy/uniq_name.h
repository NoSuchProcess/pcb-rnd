#ifndef PCB_UNIQ_NAME_H
#define PCB_UNIQ_NAME_H

#include <genht/htsp.h>

typedef struct unm_s {
	const char *unnamed;     /* name to use when orig_name is NULL */
	const char *suffix_sep;  /* separator for the suffix appended to generate unique names */
	htsp_t seen;             /* hash for storing names already handed out */
	unsigned long int ctr;   /* duplication counter - this will become the suffix for duplicated items */
} unm_t;

/* Initialize a new group of unique names */
void unm_init(unm_t *state);

/* Free all memory claimed by the group */
void unm_uninit(unm_t *state);

/* Generate and return a unique name:
    - if orig_name is NULL, generate an unnamed item
    - if orig_name is not-NULL and is unseen so far, return a copy of orig_name
    - if orig_name is not-NULL and has been already seen, return a modified version
    - an empty, non-NULL orig_name handled as if it was NULL
   Strings returned are newly allocated and can be used until unm_uninit()
   is called on state. */
const char *unm_name(unm_t *state, const char *orig_name);

#endif

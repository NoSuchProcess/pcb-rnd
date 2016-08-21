#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <genht/hash.h>
#include "uniq_name.h"
#include "compat_misc.h"

static const char *unm_default_unnamed = "unnamed";
static const char *unm_default_suffix_sep = "_dup";

void unm_init(unm_t *state)
{
	state->unnamed    = unm_default_unnamed;
	state->suffix_sep = unm_default_suffix_sep;
	htsp_init(&state->seen, strhash, strkeyeq);
	state->ctr = 0;
}


void unm_uninit(unm_t *state)
{
	htsp_entry_t *e;
	for (e = htsp_first(&state->seen); e; e = htsp_next(&state->seen, e)) {
		free(e->key);
		htsp_delentry(&state->seen, e);
	}
	htsp_uninit(&state->seen);
}

const char *unm_name(unm_t *state, const char *orig_name)
{
	int l1, l2;
	char *name, *end;
	const char *head;

	if (orig_name == NULL) {
		head = state->unnamed;
		l1 = strlen(state->unnamed);
		l2 = strlen(state->suffix_sep);
	}
	else {
		if (!htsp_has(&state->seen, (char *)orig_name)) {
			name = pcb_strdup(orig_name);
			htsp_set(&state->seen, name, name);
			return name;
		}
		else {
			head = orig_name;
			l1 = strlen(orig_name);
			l2 = strlen(state->suffix_sep);
		}
	}

	/* have to generate a new name, allocate memory and print the static part
	   leave end to point where the integer part goes
	   21 is large enough to store the widest 64 bit integer decimal, and a \0 */
		end = name = malloc(l1+l2+21);
		memcpy(end, head, l1); end += l1;
		memcpy(end, state->suffix_sep, l2); end += l2;

	/* look for the first unused entry with this suffix. Note: the entries
	   this function creates won't collide as ctr is a state of the group, but
	   it is possible that a new name collides with a past unsuffixed orig_name;
	   all in all, this loop should exit in the first iteration */
	do {
		sprintf(end, "%lu", state->ctr++);
	} while(htsp_has(&state->seen, name));

	htsp_set(&state->seen, name, name);
	return name;
}


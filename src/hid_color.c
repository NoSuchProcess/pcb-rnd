#include <string.h>
#include "config.h"
#include "hid_color.h"
#include <genht/hash.h>
#include "compat_misc.h"

static pcb_hidval_t invalid_color = { 0 };

#define HT_HAS_CONST_KEY
typedef char *htsh_key_t;
typedef const char *htsh_const_key_t;
typedef pcb_hidval_t htsh_value_t;
#define HT_INVALID_VALUE invalid_color
#define HT(x) htsh_ ## x
#include <genht/ht.h>
#include <genht/ht.c>
#undef HT


int pcb_hid_cache_color(int set, const char *name, pcb_hidval_t * val, void **vcache)
{
	htsh_t *cache;
	htsh_entry_t *e;

	cache = (htsh_t *) * vcache;
	if (cache == 0) {
		cache = htsh_alloc(strhash, strkeyeq);
		*vcache = cache;
	}

	if (!set) { /* read */
		e = htsh_getentry(cache, (char *)name);
		if (e == NULL) /* not found */
			return 0;
		memcpy(val, &e->value, sizeof(pcb_hidval_t));
	}
	else
		htsh_set(cache, pcb_strdup(name), *val); /* write */

	return 1;
}

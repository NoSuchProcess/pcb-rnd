#include "config.h"
#include "global.h"
#include "hid_color.h"

#define HASH_SIZE 31

typedef struct ecache {
	struct ecache *next;
	const char *name;
	hidval val;
} ecache;

typedef struct ccache {
	ecache *colors[HASH_SIZE];
	ecache *lru;
} ccache;

static void copy_color(int set, hidval * cval, hidval * aval)
{
	if (set)
		memcpy(cval, aval, sizeof(hidval));
	else
		memcpy(aval, cval, sizeof(hidval));
}

int hid_cache_color(int set, const char *name, hidval * val, void **vcache)
{
	unsigned long hash;
	const char *cp;
	ccache *cache;
	ecache *e;

	cache = (ccache *) * vcache;
	if (cache == 0) {
		cache = (ccache *) calloc(sizeof(ccache), 1);
		*vcache = cache;
	}
	if (cache->lru && strcmp(cache->lru->name, name) == 0) {
		copy_color(set, &(cache->lru->val), val);
		return 1;
	}

	/* djb2: this algorithm (k=33) was first reported by dan bernstein many
	 * years ago in comp.lang.c. another version of this algorithm (now favored
	 * by bernstein) uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic
	 * of number 33 (why it works better than many other constants, prime or
	 * not) has never been adequately explained.
	 */
	hash = 5381;
	for (cp = name, hash = 0; *cp; cp++)
		hash = ((hash << 5) + hash) + (*cp & 0xff);	/* hash * 33 + c */
	hash %= HASH_SIZE;

	for (e = cache->colors[hash]; e; e = e->next)
		if (strcmp(e->name, name) == 0) {
			copy_color(set, &(e->val), val);
			cache->lru = e;
			return 1;
		}
	if (!set)
		return 0;

	e = (ecache *) malloc(sizeof(ecache));
	e->next = cache->colors[hash];
	cache->colors[hash] = e;
	e->name = strdup(name);
	memcpy(&(e->val), val, sizeof(hidval));
	cache->lru = e;

	return 1;
}

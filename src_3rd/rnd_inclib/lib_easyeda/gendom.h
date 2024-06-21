#ifndef GENDOM_H
#define GENDOM_H

#include <stdio.h>
#include <genht/htip.h>

typedef enum gdom_node_type_e {
	GDOM_ARRAY,
	GDOM_HASH,

	/* leaves */
	GDOM_STRING,
	GDOM_DOUBLE,
	GDOM_LONG
} gdom_node_type_t;

typedef struct gdom_node_s gdom_node_t;

struct gdom_node_s {
	long name;                 /* format-specific enum declared by the low level parser */
	gdom_node_type_t type;
	gdom_node_t *parent;
	union {
		struct {
			long used, alloced;
			gdom_node_t **child;
		} array;
		htip_t hash;             /* key: name; value: (gdom_node_t *) */
		char *str;               /* strdup'd */
		double dbl;
		long lng;
	} value;
};

gdom_node_t *gdom_alloc(long name, gdom_node_type_t type);
void gdom_free(gdom_node_t *tree); /* recursive */

/* Insert child in parent's hash table. Parent must be of type GDOM_HASH.
   Returns 0 on success. Errors: parent is not a hash, child already has
   a parent or parent hash already has a child of the same name */
int gdom_hash_put(gdom_node_t *parent, gdom_node_t *child);

/* Returns the named child node of a hash parent */
gdom_node_t *gdom_hash_get(gdom_node_t *parent, long name);


/* Append child to parent's array. Parent must be of type GDOM_ARRAY.
   Returns 0 on success. Errors: parent is not an array or child already has
   a parent. */
int gdom_array_append(gdom_node_t *parent, gdom_node_t *child);

/* Either gdom_hash_put() or gdom_array_append(), depending on parent type */
int gdom_append(gdom_node_t *parent, gdom_node_t *child);

void gdom_dump(FILE *f, gdom_node_t *tree, int ind, const char *(*name2str)(long name));

char *gdom_strdup(const char *s);

#endif

/* eagle binary file tree representation */
#ifndef PCB_EGB_TREE_H
#define PCB_EGB_TREE_H
#include <stdio.h>
#include <genht/htss.h>

typedef struct egb_node_s egb_node_t;

struct egb_node_s {
	int id;
	const char *id_name;
	htss_t props;
	egb_node_t *parent;
	egb_node_t *next;
	egb_node_t *first_child, *last_child;
};


egb_node_t *egb_node_alloc(int id, const char *id_name);
egb_node_t *egb_node_append(egb_node_t *parent, egb_node_t *node);
void egb_node_free(egb_node_t *node);

void egb_node_prop_set(egb_node_t *node, const char *key, const char *val);
char *egb_node_prop_get(egb_node_t *node, const char *key);

void egb_dump(FILE *f, egb_node_t *node);

#endif

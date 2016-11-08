#include <stdio.h>
#include <string.h>

static char ind[] = "                                ";
static void proto_node_print(proto_node_t *n, int level)
{
	if (level < sizeof(ind)-1)
		ind[level] = '\0';
	printf("%s%s (%s):", ind, (n->is_list ? "list" : "str"), (n->is_bin ? "bin" : "text"));
	if (level < sizeof(ind)-1)
		ind[level] = ' ';

	if (n->is_list) {
		proto_node_t *ch;
		printf("\n");
		for(ch = n->data.l.first_child; ch != NULL; ch = ch->next)
			proto_node_print(ch, level+1);
	}
	else
		printf(" %lu/'%s'\n", n->data.s.len, n->data.s.str);
}

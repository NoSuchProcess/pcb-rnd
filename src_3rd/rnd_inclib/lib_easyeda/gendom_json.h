#include "gendom.h"

/* Parse a json from f into a newly allocated gdom tree using str2name()
   to convert string names to long names. Returns NULL on error. */
gdom_node_t *gdom_json_parse(FILE *f, long (*str2name)(const char *str));

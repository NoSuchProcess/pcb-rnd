#include "gendom.h"

/* Parse a json from f into a newly allocated gdom tree using str2name()
   to convert string names to long names. Returns NULL on error. */
gdom_node_t *gdom_json_parse(FILE *f, long (*str2name)(const char *str));

/* Same but parse from \0 terminated str */
gdom_node_t *gdom_json_parse_str(const char *str, long (*str2name)(const char *str));

/* Same, but parse a stream using user provided getchr() */
gdom_node_t *gdom_json_parse_any(void *uctx, int (*getchr)(void *uctx), long (*str2name)(const char *str));

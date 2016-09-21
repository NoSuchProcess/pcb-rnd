
/* Print all configuration items to f, prefixing each line with prefix
   If match_prefix is not NULL, print only items with matching path prefix */
void conf_dump(FILE *f, const char *prefix, int verbose, const char *match_prefix);

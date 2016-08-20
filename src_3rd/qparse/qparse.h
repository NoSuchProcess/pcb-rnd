/* Split input into a list of strings in argv_ret[] using whitepsace as field
   separator. It supports double quoted fields and backslash as escape character.
   Returns the number of arguments. argv_ret should be free'd using
   qparse_free(). */
int qparse(const char *input, char **argv_ret[]);

/* Free an argv_ret array allocated by qparse. */
void qparse_free(int argc, char **argv_ret[]);


#include <stdlib.h>

/* Split input into a list of strings in argv_ret[] using whitepsace as field
   separator. It supports double quoted fields and backslash as escape character.
   Returns the number of arguments. argv_ret should be free'd using
   qparse_free(). */
int qparse(const char *input, char **argv_ret[]);

/* Free an argv_ret array allocated by qparse. */
void qparse_free(int argc, char **argv_ret[]);

/* for C89 - that doesn't have strdup()*/
char *qparse_strdup(const char *s);

/* More advanced API with more control over the format */
typedef enum {
	QPARSE_DOUBLE_QUOTE = 1,
	QPARSE_SINGLE_QUOTE = 2,
	QPARSE_PAREN = 4,
	QPARSE_MULTISEP = 8,        /* multiple separators are taken as a single separator */
	QPARSE_TERM_NEWLINE = 16,   /* terminate parsing at newline */
	QPARSE_TERM_SEMICOLON = 32, /* terminate parsing at semicolon */
	QPARSE_SEP_COMMA = 64,      /* comma is a separator, like whitespace */
	QPARSE_COLON_LAST = 128     /* if an argument starts with a colon, it's the last argument until the end of the message or line (IRC) */
} flags_t;

int qparse2(const char *input, char **argv_ret[], flags_t flg);

/* This variant returns the number of characters consumed from the input
   so it can be used for multi-command parsing */
int qparse3(const char *input, char **argv_ret[], flags_t flg, size_t *consumed_out);

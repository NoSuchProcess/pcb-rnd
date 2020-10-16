#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* remove leading whitespace */
#define ltrim(s) while(isspace(*s)) s++

/* remove trailing newline;; trailing backslash is an error */
#define rtrim(s) \
	do { \
		char *end; \
		for(end = s + strlen(s) - 1; (end >= s) && ((*end == '\r') || (*end == '\n')); end--) \
			*end = '\0'; \
		if ((end[0] == '\\') && ((end == s) || (end[-1] != '\\'))) \
			return -1; \
	} while(0)

#define null_empty(s) ((s) == NULL ? "" : (s))

int tedax_getline(FILE *f, char *buff, int buff_size, char *argv[], int argv_size);

int tedax_seek_hdr(FILE *f, char *buff, int buff_size, char *argv[], int argv_size);
int tedax_seek_block(FILE *f, const char *blk_name, const char *blk_ver, const char *blk_id, int silent, char *buff, int buff_size, char *argv[], int argv_size);


/* print val with special chars escaped. Prints a single dash if val is NULL or empty. */
void tedax_fprint_escape(FILE *f, const char *val);
void tedax_fnprint_escape(FILE *f, const char *val, int len);
int tedax_strncpy_escape(char *dst, int dstlen, const char *val);


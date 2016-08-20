/*
    libgpmi - General Package/Module Interface - qparse package
    Copyright (C) 2006-2007 Tibor 'Igor2' Palinkas
  
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
  
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
  
    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdlib.h>
#include <string.h>

typedef enum qp_state_e {
	qp_normal,
	qp_dquote,
} qp_state_t;

#define qpush(chr) \
	{ \
	 if (buff_used >= buff_len) { \
	 	buff_len += 128; \
		buff = realloc(buff, buff_len); \
	 } \
	 buff[buff_used] = chr; \
	 buff_used++; \
	}

#define qbackslash(chr) \
	switch(chr) { \
		case 'n': qpush('\n'); break; \
		case 'r': qpush('\r'); break; \
		case 't': qpush('\t'); break; \
		case 'b': qpush('\b'); break; \
		case '"': qpush('"'); break; \
		case ' ': qpush(' '); break; \
		case '\0': break; \
		default: \
			qpush('\\'); \
			qpush(chr); \
	}

#define qnext() \
	{ \
		if (argc >= allocated) { \
			allocated += 8; \
			argv = realloc(argv, sizeof(char *) * allocated); \
		} \
		buff[buff_used] = '\0'; \
		argv[argc] = strdup(buff); \
		argc++; \
		*buff = '\0'; \
		buff_used = 0; \
	}


int qparse(const char *input, char **argv_ret[])
{
	int argc;
	int allocated;
	qp_state_t state;
	const char *s;
	char *buff;
	int buff_len, buff_used;
	char **argv;

	argc      = 0;
	allocated = 0;
	argv      = NULL;
	state     = qp_normal;

	buff_len  = 128;
	buff_used = 0;
	buff      = malloc(buff_len);

	for(s = input; *s != '\0'; s++) {
		switch (state) {
			case qp_normal:
				switch (*s) {
					case '\\':
						s++;
						qbackslash(*s);
						break;
					case '"':
						state = qp_dquote;
						break;
					case ' ':
					case '\t':
					case '\n':
					case '\r':
						qnext();
						break;
					default:
						qpush(*s);
				}
				/* End of qp_normal */
				break;
			case qp_dquote:
				switch (*s) {
					case '\\':
						s++;
						qbackslash(*s);
						break;
					case '"':
						state = qp_normal;
						break;
					default:
						qpush(*s);
				}
				/* End of qp_dquote */
				break;
		}
	}

	qnext();

	if (buff != NULL)
		free(buff);

	*argv_ret = argv;
	return argc;
}

void qparse_free(int argc, char **argv_ret[])
{
	int n;

	for (n = 0; n < argc; n++)
		free((*argv_ret)[n]);

	free(*argv_ret);
	*argv_ret = NULL;
}

/*
    libgpmi - General Package/Module Interface - qparse package
    Copyright (C) 2006-2007, 2017 Tibor 'Igor2' Palinkas
  
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
#include <ctype.h>
#include "qparse.h"

typedef enum qp_state_e {
	qp_normal,
	qp_dquote,
	qp_squote,
	qp_paren,
	qp_lastarg
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
		case '\'': qpush('\''); break; \
		case ' ': qpush(' '); break; \
		case '\0': break; \
		default: \
			qpush('\\'); \
			qpush(chr); \
	}

#define qnext() \
	do { \
		int ran_out = 0; \
		if (argc >= *argv_allocated) { \
			if (!(flg & QPARSE_NO_ARGV_REALLOC)) { \
				*argv_allocated += 8; \
				argv = realloc(argv, sizeof(char *) * (*argv_allocated)); \
			} \
			else \
				ran_out = 1; \
		} \
		if (!ran_out) { \
			buff[buff_used] = '\0'; \
			argv[argc] = qparse_strdup(buff); \
			argc++; \
			*buff = '\0'; \
			buff_used = 0; \
		} \
	} while(0)

int qparse4(const char *input, char **argv_ret[], unsigned int *argv_allocated, flags_t flg, size_t *consumed_out, char **buffer, size_t *buffer_alloced)
{
	int argc, allocated_;
	qp_state_t state;
	const char *s;
	char *buff;
	size_t buff_len, buff_used;
	char **argv;

	if (argv_allocated == NULL) {
		argv           = NULL;
		allocated_     = 0;
		argv_allocated = &allocated_;
	}

	argc      = 0;
	state     = qp_normal;

	if (buffer == NULL) {
		buff_len  = 128;
		buff_used = 0;
		buff      = malloc(buff_len);
	}
	else {
		buff_len = *buffer_alloced;
		buff     = *buffer;
	}

	for(s = input; *s != '\0'; s++) {
		switch (state) {
			case qp_lastarg:
				switch (*s) {
					case '\n':
					case '\r':
						if (flg & QPARSE_TERM_NEWLINE)
							goto stop;
					default:
						qpush(*s);
				}
				break;
			case qp_normal:
				switch (*s) {
					case '"':
						if (flg & QPARSE_DOUBLE_QUOTE)
							state = qp_dquote;
						else
							qpush(*s);
						break;
					case '\'':
						if (flg & QPARSE_SINGLE_QUOTE)
							state = qp_squote;
						else
							qpush(*s);
						break;
					case '(':
						if (flg & QPARSE_PAREN)
							state = qp_paren;
						else
							qpush(*s);
						break;

					case '\\':
						s++;
						qbackslash(*s);
						break;

					case ';':
						if (flg & QPARSE_TERM_SEMICOLON)
							goto stop;
						qpush(*s);  /* plain ';', don't care */
						break;

					case '\n':
					case '\r':
						if (flg & QPARSE_TERM_NEWLINE)
							goto stop;
					case ',':
						if (!(flg & QPARSE_SEP_COMMA)) {
							qpush(*s);  /* plain ',', don't care */
							break;
						}
					case ' ':
					case '\t':
						if (flg & QPARSE_MULTISEP)
							while(isspace(s[1])) s++;
						qnext();
						break;
					default:
						if ((flg & QPARSE_COLON_LAST) && (buff_used == 0) && *s == ':') {
							state = qp_lastarg;
							break;
						}
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

			case qp_squote:
				switch (*s) {
					case '\\':
						s++;
						qbackslash(*s);
						break;
					case '\'':
						state = qp_normal;
						break;
					default:
						qpush(*s);
				}
				/* End of qp_dquote */
				break;

			case qp_paren:
				switch (*s) {
					case '\\':
						s++;
						qbackslash(*s);
						break;
					case ')':
						state = qp_normal;
						break;
					default:
						qpush(*s);
				}
				/* End of qp_paren */
				break;
		}
	}

	stop:;

	qnext();

	if (buffer == NULL) {
		if (buff != NULL)
			free(buff);
	}
	else {
		*buffer = buff;
		*buffer_alloced = buff_len;
	}

	if (consumed_out != NULL)
		*consumed_out = s-input;
	*argv_ret = argv;
	return argc;
}

int qparse3(const char *input, char **argv_ret[], flags_t flg, size_t *consumed_out)
{
	return qparse4(input, argv_ret, 0, flg, consumed_out, NULL, NULL);
}


int qparse2(const char *input, char **argv_ret[], flags_t flg)
{
	return qparse4(input, argv_ret, 0, flg, NULL, NULL, NULL);
}

int qparse(const char *input, char **argv_ret[])
{
	return qparse4(input, argv_ret, 0, QPARSE_DOUBLE_QUOTE, NULL, NULL, NULL);
}


void qparse_free_strs(int argc, char **argv_ret[])
{
	int n;
	for (n = 0; n < argc; n++) {
		free((*argv_ret)[n]);
		(*argv_ret)[n] = NULL;
	}
}

void qparse_free(int argc, char **argv_ret[])
{
	qparse_free_strs(argc, argv_ret);

	free(*argv_ret);
	*argv_ret = NULL;
}

void qparse4_free(char **argv_ret[], unsigned int *argv_allocated, flags_t flg, char **buffer, size_t *buffer_alloced)
{
	qparse_free_strs(*argv_allocated, argv_ret);

	if (!(flg & QPARSE_NO_ARGV_REALLOC)) {
		free(*argv_ret);
		*argv_ret = NULL;
		*argv_allocated = 0;
	}

	if (*buffer != NULL) {
		free(*buffer);
		*buffer_alloced = 0;
	}
}


char *qparse_strdup(const char *s)
{
	int l = strlen(s);
	char *o;
	o = malloc(l+1);
	memcpy(o, s, l+1);
	return o;
}

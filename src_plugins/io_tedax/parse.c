/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - low level parser, similar to the reference implementation
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <assert.h>

#include "parse.h"
#include <librnd/core/error.h>
#include <librnd/core/compat_misc.h>

int tedax_getline(FILE *f, char *buff, int buff_size, char *argv[], int argv_size)
{
	int argc;

	for(;;) {
		char *s, *o;

		if (fgets(buff, buff_size, f) == NULL)
			return -1;

		s = buff;
		if (*s == '#') /* comment */
			continue;
		ltrim(s);
		rtrim(s);
		if (*s == '\0') /* empty line */
			continue;

		/* argv split */
		for(argc = 0, o = argv[0] = s; *s != '\0';) {
			if (*s == '\\') {
				s++;
				switch(*s) {
					case 'r': *o = '\r'; break;
					case 'n': *o = '\n'; break;
					case 't': *o = '\t'; break;
					default: *o = *s;
				}
				o++;
				s++;
				continue;
			}
			if ((argc+1 < argv_size) && ((*s == ' ') || (*s == '\t'))) {
				*o = *s = '\0';
				s++;
				o++;
				while((*s == ' ') || (*s == '\t'))
					s++;
				argc++;
				argv[argc] = o;
			}
			else {
				*o = *s;
				s++;
				o++;
			}
		}
		*o = '\0';
		return argc+1; /* valid line, split up */
	}

	return -1; /* can't get here */
}

int tedax_seek_hdr(FILE *f, char *buff, int buff_size, char *argv[], int argv_size)
{
	int argc;

	/* look for the header */
	if ((argc = tedax_getline(f, buff, buff_size, argv, argv_size)) < 2) {
		rnd_message(RND_MSG_ERROR, "Can't find tEDAx header (no line)\n");
		return -1;
	}

	if ((argv[1] == NULL) || (rnd_strcasecmp(argv[0], "tEDAx") != 0) || (rnd_strcasecmp(argv[1], "v1") != 0)) {
		rnd_message(RND_MSG_ERROR, "Can't find tEDAx header (wrong line)\n");
		return -1;
	}

	return argc;
}


int tedax_seek_block(FILE *f, const char *blk_name, const char *blk_ver, const char *blk_id, int silent, char *buff, int buff_size, char *argv[], int argv_size)
{
	int argc;

	/* seek block begin */
	while((argc = tedax_getline(f, buff, buff_size, argv, argv_size)) >= 0)
		if ((argc > 2) && (strcmp(argv[0], "begin") == 0) && (strcmp(argv[1], blk_name) == 0) && ((blk_ver == NULL) || (strcmp(argv[2], blk_ver) == 0)) && ((blk_id == NULL) || (strcmp(argv[3], blk_id) == 0)))
			break;

	if (argc < 2) {
		if (!silent)
			rnd_message(RND_MSG_ERROR, "Can't find %s %s block in tEDAx\n", blk_ver, blk_name);
		return -1;
	}

	return argc;
}

void tedax_fnprint_escape(FILE *f, const char *val, int len)
{
	if ((val == NULL) || (*val == '\0')) {
		fputc('-', f);
		return;
	}
	for(; (*val != '\0') && (len > 0); val++,len--) {
		switch(*val) {
			case '\\': fputc('\\', f); fputc('\\', f); break;
			case '\n': fputc('\\', f); fputc('n', f); break;
			case '\r': fputc('\\', f); fputc('r', f); break;
			case '\t': fputc('\\', f); fputc('t', f); break;
			case ' ': fputc('\\', f); fputc(' ', f); break;
			default:
				fputc(*val, f);
		}
	}
}

void tedax_fprint_escape(FILE *f, const char *val)
{
	tedax_fnprint_escape(f, val, 510);
}

#define APPEND(c)  \
	do { \
		if (dstlen == 0) { \
			res = -1; \
			goto quit; \
		} \
		*d = c; \
		d++; \
		dstlen--; \
	} while(0)

int tedax_strncpy_escape(char *dst, int dstlen, const char *val)
{
	int res = 0;
	char *d = dst;

	assert(dstlen > 2);
	if ((val == NULL) || (*val == '\0')) {
		dst[0] = '-';
		dst[1] = '\0';
		return 0;
	}
	dstlen--; /* one for \0 */
	for(; *val != '\0'; val++) {
		switch(*val) {
			case '\\': APPEND('\\'); APPEND('\\'); break;
			case '\n': APPEND('\\'); APPEND('n'); break;
			case '\r': APPEND('\\'); APPEND('r'); break;
			case '\t': APPEND('\\'); APPEND('t'); break;
			case ' ': APPEND('\\'); APPEND(' '); break;
			default:
				APPEND(*val);
		}
	}
	quit:;
	*d = '\0';
	return res;
}


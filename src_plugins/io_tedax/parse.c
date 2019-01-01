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
#include "parse.h"
#include "error.h"
#include "compat_misc.h"

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
				*s = '\0';
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
		pcb_message(PCB_MSG_ERROR, "Can't find tEDAx header (no line)\n");
		return -1;
	}

	if ((argv[1] == NULL) || (pcb_strcasecmp(argv[0], "tEDAx") != 0) || (pcb_strcasecmp(argv[1], "v1") != 0)) {
		pcb_message(PCB_MSG_ERROR, "Can't find tEDAx header (wrong line)\n");
		return -1;
	}

	return argc;
}


int tedax_seek_block(FILE *f, const char *blk_name, const char *blk_ver, int silent, char *buff, int buff_size, char *argv[], int argv_size)
{
	int argc;

	/* seek block begin */
	while((argc = tedax_getline(f, buff, buff_size, argv, argv_size)) >= 0)
		if ((argc > 2) && (strcmp(argv[0], "begin") == 0) && (strcmp(argv[1], blk_name) == 0) && (strcmp(argv[2], blk_ver) == 0))
			break;

	if (argc < 2) {
		if (!silent)
			pcb_message(PCB_MSG_ERROR, "Can't find %s %s block in tEDAx\n", blk_ver, blk_name);
		return -1;
	}

	return argc;
}

void tedax_fprint_escape(FILE *f, const char *val)
{
	if ((val == NULL) || (*val == '\0')) {
		fputc('-', f);
		return;
	}
	for(; *val != '\0'; val++) {
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

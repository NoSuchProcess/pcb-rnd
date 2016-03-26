/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998,2005,2006 Thomas Nau
 *  Copyright (C) 2015 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* file save, load, merge ... routines
 * getpid() needs a cast to (int) to get rid of compiler warnings
 * on several architectures
 */
#include "config.h"

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "global.h"

#include <time.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif


#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "error.h"
#include "compat_fs.h"
#include "libpcb_fp.h"
#include "plug_footprint.h"


#include "paths.h"

RCSID("$Id$");

/* Decide about the type of a footprint file:
   - it is a file element if the first non-comment is "Element(" or "Element["
   - else it is a parametric element (footprint generator) if it contains 
     "@@" "purpose"
   - else it's not an element.
   - if a line of a file element starts with ## and doesn't contain @, it's a tag
   - if tags is not NULL, it's a pointer to a void *tags[] - an array of tag IDs
*/
fp_type_t pcb_fp_file_type(const char *fn, void ***tags)
{
	int c, comment_len;
	int first_element = 1;
	FILE *f;
	enum {
		ST_WS,
		ST_COMMENT,
		ST_ELEMENT,
		ST_TAG,
	} state = ST_WS;
	char *tag = NULL;
	int talloced = 0, tused = 0;
	int Talloced = 0, Tused = 0;
	fp_type_t ret = PCB_FP_INVALID;

	if (tags != NULL)
		*tags = NULL;

	f = fopen(fn, "r");
	if (f == NULL)
		return PCB_FP_INVALID;

	while ((c = fgetc(f)) != EOF) {
		switch (state) {
		case ST_ELEMENT:
			if (isspace(c))
				break;
			if ((c == '(') || (c == '[')) {
				ret = PCB_FP_FILE;
				goto out;
			}
		case ST_WS:
			if (isspace(c))
				break;
			if (c == '#') {
				comment_len = 0;
				state = ST_COMMENT;
				break;
			}
			else if ((first_element) && (c == 'E')) {
				char s[8];
				/* Element */
				fgets(s, 7, f);
				s[6] = '\0';
				if (strcmp(s, "lement") == 0) {
					state = ST_ELEMENT;
					break;
				}
			}
			first_element = 0;
			/* fall-thru for detecting @ */
		case ST_COMMENT:
			comment_len++;
			if ((c == '#') && (comment_len == 1)) {
				state = ST_TAG;
				break;
			}
			if ((c == '\r') || (c == '\n'))
				state = ST_WS;
			if (c == '@') {
				char s[10];
			maybe_purpose:;
				/* "@@" "purpose" */
				fgets(s, 9, f);
				s[8] = '\0';
				if (strcmp(s, "@purpose") == 0) {
					ret = PCB_FP_PARAMETRIC;
					goto out;
				}
			}
			break;
		case ST_TAG:
			if ((c == '\r') || (c == '\n')) {	/* end of a tag */
				if (tags != NULL) {
					tag[tused] = '\0';
					if (Tused >= Talloced) {
						Talloced += 8;
						*tags = realloc(*tags, (Talloced + 1) * sizeof(void *));
					}
					(*tags)[Tused] = (void *) fp_tag(tag, 1);
					Tused++;
					(*tags)[Tused] = NULL;
				}

				tused = 0;
				state = ST_WS;
				break;
			}
			if (c == '@')
				goto maybe_purpose;
			if (tused >= talloced) {
				talloced += 64;
				tag = realloc(tag, talloced + 1);	/* always make room for an extra \0 */
			}
			tag[tused] = c;
			tused++;
		}
	}

out:;
	if (tag != NULL)
		free(tag);
	fclose(f);
	return ret;
}

FILE *pcb_fp_fopen(const char *libshell, const char *path, const char *name, int *is_parametric)
{
	char *basename, *params, *fullname;
	FILE *f = NULL;

	*is_parametric = fp_dupname(name, &basename, &params);
	if (basename == NULL)
		return NULL;

	fullname = fp_fs_search(path, basename, *is_parametric);
/*	fprintf(stderr, "basename=%s fullname=%s\n", basename, fullname);*/
/*	printf("pcb_fp_fopen: %d '%s' '%s' fullname='%s'\n", *is_parametric, basename, params, fullname);*/


	if (fullname != NULL) {
/*fprintf(stderr, "fullname=%s param=%d\n",  fullname, *is_parametric);*/
		if (*is_parametric) {
			char *cmd, *sep = " ";
			if (libshell == NULL) {
				libshell = "";
				sep = "";
			}
			cmd = malloc(strlen(libshell) + strlen(fullname) + strlen(params) + 16);
			sprintf(cmd, "%s%s%s %s", libshell, sep, fullname, params);
/*fprintf(stderr, " cmd=%s\n",  cmd);*/
			f = popen(cmd, "r");
			free(cmd);
		}
		else
			f = fopen(fullname, "r");
		free(fullname);
	}

	free(basename);
	return f;
}

void pcb_fp_fclose(FILE * f, int *is_parametric)
{
	if (*is_parametric)
		pclose(f);
	else
		fclose(f);
}

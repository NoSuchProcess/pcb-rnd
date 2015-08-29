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

#include "ds.h"

#include "config.h"

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "global.h"

#include <dirent.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include <time.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <sys/stat.h>

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
#include "portability.h"
#include "libpcb_fp.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#include "3rd/genht/htsp.h"
#include "3rd/genht/hash.h"

RCSID("$Id$");

static htsp_t *pcb_fp_tags = NULL;

static int keyeq(char *a, char *b) {
	return !strcmp(a,b);
}

const void *pcb_fp_tag(const char *tag, int alloc)
{
	htsp_entry_t *e;
	static int counter = 0;

	if (pcb_fp_tags == NULL)
		pcb_fp_tags = htsp_alloc(strhash, keyeq);
	e = htsp_getentry(pcb_fp_tags, (char *)tag);
	if ((e == NULL) && alloc) {
		htsp_set(pcb_fp_tags, strdup(tag), (void *)counter);
		counter++;
		e = htsp_getentry(pcb_fp_tags, (char *)tag);
	}
	return e->key;
}

const char *pcb_fp_tagname(const void *tagid)
{
	return (char *)tagid;
}

/* Decide about the type of a footprint file:
   - it is a file element if the first non-comment is "Element(" or "Element["
   - else it is a parametric element (footprint generator) if it contains 
     "@@" "purpose"
   - else it's not an element.
   - if a line of a file element starts with ## and doesn't contain @, it's a tag
   - if tags is not NULL, it's a pointer to a void *tags[] - an array of tag IDs
*/
pcb_fp_type_t pcb_fp_file_type(const char *fn, void ***tags)
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
	pcb_fp_type_t ret = PCB_FP_INVALID;

	if (tags != NULL)
		*tags = NULL;

	f = fopen(fn, "r");
	if (f == NULL)
		return PCB_FP_INVALID;

	while((c = fgetc(f)) != EOF) {
		switch(state) {
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
				if ((c == '\r') || (c == '\n')) { /* end of a tag */
					if (tags != NULL) {
						tag[tused] = '\0';
						if (Tused >= Talloced) {
							Talloced += 8;
							*tags = realloc(*tags, (Talloced+1) * sizeof(void *));
						}
						(*tags)[Tused] = (void *)pcb_fp_tag(tag, 1);
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
					tag = realloc(tag, talloced+1); /* always make room for an extra \0 */
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

int pcb_fp_list(const char *subdir, int recurse, int (*cb) (void *cookie, const char *subdir, const char *name, pcb_fp_type_t type, void *tags[]), void *cookie, int subdir_may_not_exist, int need_tags)
{
	char olddir[MAXPATHLEN + 1];	/* The directory we start out in (cwd) */
	char new_subdir[MAXPATHLEN + 1];
	char fn[MAXPATHLEN + 1], *fn_end;
	DIR *subdirobj;								/* Interable object holding all subdir entries */
	struct dirent *subdirentry;		/* Individual subdir entry */
	struct stat buffer;						/* Buffer used in stat */
	size_t l;
	size_t len;
	int n_footprints = 0;					/* Running count of footprints found in this subdir */
	char *full_path;

	/* Cache old dir, then cd into subdir because stat is given relative file names. */
	memset(olddir, 0, sizeof olddir);
	if (GetWorkingDirectory(olddir) == NULL) {
		Message(_("pcb_fp_list(): Could not determine initial working directory\n"));
		return 0;
	}

	if (chdir(subdir)) {
		if (!subdir_may_not_exist)
			ChdirErrorMessage(subdir);
		return 0;
	}


	/* Determine subdir's abs path */
	if (GetWorkingDirectory(new_subdir) == NULL) {
		Message(_("pcb_fp_list(): Could not determine new working directory\n"));
		if (chdir(olddir))
			ChdirErrorMessage(olddir);
		return 0;
	}

	l = strlen(new_subdir);
	memcpy(fn, new_subdir, l);
	fn[l] = PCB_DIR_SEPARATOR_C;
	fn_end = fn+l+1;

	/* First try opening the directory specified by path */
	if ((subdirobj = opendir(new_subdir)) == NULL) {
		OpendirErrorMessage(new_subdir);
		if (chdir(olddir))
			ChdirErrorMessage(olddir);
		return 0;
	}

	/* Now loop over files in this directory looking for files.
	 * We ignore certain files which are not footprints.
	 */
	while ((subdirentry = readdir(subdirobj)) != NULL) {
#ifdef DEBUG
/*    printf("...  Examining file %s ... \n", subdirentry->d_name); */
#endif


		/* Ignore non-footprint files found in this directory
		 * We're skipping .png and .html because those
		 * may exist in a library tree to provide an html browsable
		 * index of the library.
		 */
		l = strlen(subdirentry->d_name);
		if (!stat(subdirentry->d_name, &buffer)
				&& subdirentry->d_name[0] != '.'
				&& NSTRCMP(subdirentry->d_name, "CVS") != 0
				&& NSTRCMP(subdirentry->d_name, "Makefile") != 0
				&& NSTRCMP(subdirentry->d_name, "Makefile.am") != 0
				&& NSTRCMP(subdirentry->d_name, "Makefile.in") != 0 && (l < 4 || NSTRCMP(subdirentry->d_name + (l - 4), ".png") != 0)
				&& (l < 5 || NSTRCMP(subdirentry->d_name + (l - 5), ".html") != 0)
				&& (l < 4 || NSTRCMP(subdirentry->d_name + (l - 4), ".pcb") != 0)) {

#ifdef DEBUG
/*	printf("...  Found a footprint %s ... \n", subdirentry->d_name); */
#endif
			strcpy(fn_end, subdirentry->d_name);
			if ((S_ISREG(buffer.st_mode)) || (WRAP_S_ISLNK(buffer.st_mode))) {
				pcb_fp_type_t ty;
				void **tags = NULL;
				ty = pcb_fp_file_type(subdirentry->d_name, (need_tags ? &tags : NULL));
				if ((ty == PCB_FP_FILE) || (ty == PCB_FP_PARAMETRIC)) {
					n_footprints++;
					if (cb(cookie, new_subdir, subdirentry->d_name, ty, tags))
						break;
					continue;
				}
			}

			if ((S_ISDIR(buffer.st_mode)) || (WRAP_S_ISLNK(buffer.st_mode))) {
				cb(cookie, new_subdir, subdirentry->d_name, PCB_FP_DIR, NULL);
				if (recurse) {
					n_footprints += pcb_fp_list(fn, recurse, cb, cookie, 0, need_tags);
				}
				continue;
			}

		}
	}
	/* Done.  Clean up, cd back into old dir, and return */
	closedir(subdirobj);
	if (chdir(olddir))
		ChdirErrorMessage(olddir);
	return n_footprints;
}

int pcb_fp_dupname(const char *name, char **basename, char **params)
{
	char *newname, *s;

	*basename = newname = strdup(name);
	s = strchr(newname, '(');
	if (s == NULL) {
		*params = NULL;
		return 0;
	}

	/* split at '(' */
	*s = '\0';
	*params = s+1;
	s = *params + strlen(*params)-1;

	/* strip ')' */
	if (*s == ')')
		*s = '\0';

	return 1;
}

typedef struct {
	const char *target;
	int target_len;
	int parametric;
	char *path;
	char *real_name;
} pcb_fp_search_t;

static int pcb_fp_search_cb(void *cookie, const char *subdir, const char *name, pcb_fp_type_t type, void *tags[])
{
	pcb_fp_search_t *ctx = (pcb_fp_search_t *)cookie;
	if ((strncmp(ctx->target, name, ctx->target_len) == 0) && ((!!ctx->parametric) == (type == PCB_FP_PARAMETRIC))) {
		const char *suffix = name + ctx->target_len;
		/* ugly heuristics: footprint names may end in .fp or .ele */
		if ((*suffix == '\0') || (strcasecmp(suffix, ".fp") == 0) || (strcasecmp(suffix, ".ele") == 0)) {
			ctx->path = strdup(subdir);
			ctx->real_name = strdup(name);
			return 1;
		}
	}
	return 0;
}

char *pcb_fp_search(const char *search_path, const char *basename, int parametric)
{
	int found;
	const char *p, *end;
	char path[MAXPATHLEN + 1];
	pcb_fp_search_t ctx;

	if ((*basename == '/') || (*basename == PCB_DIR_SEPARATOR_C))
		return strdup(basename);

	ctx.target = basename;
	ctx.target_len = strlen(ctx.target);
	ctx.parametric = parametric;
	ctx.path = NULL;

/*	fprintf("Looking for %s\n", ctx.target);*/

	for(p = search_path; end = strchr(p, ':'); p = end+1) {
		char *fpath;
		memcpy(path, p, end-p);
		path[end-p] = '\0';

		resolve_path(path, &fpath);
/*		fprintf(stderr, " in '%s'\n", fpath);*/

		pcb_fp_list(fpath, 1, pcb_fp_search_cb, &ctx, 1, 0);
		if (ctx.path != NULL) {
			sprintf(path, "%s%c%s", ctx.path, PCB_DIR_SEPARATOR_C, ctx.real_name);
			free(ctx.path);
			free(ctx.real_name);
/*			fprintf("  found '%s'\n", path);*/
			free(fpath);
			return strdup(path);
		}
		free(fpath);
		if (end == NULL)
			break;
	}
	return NULL;
}


FILE *pcb_fp_fopen(const char *libshell, const char *path, const char *name, int *is_parametric)
{
	char *basename, *params, *fullname;
	FILE *f = NULL;

	*is_parametric = pcb_fp_dupname(name, &basename, &params);
	if (basename == NULL)
		return NULL;

	fullname = pcb_fp_search(path, basename, *is_parametric);
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

void pcb_fp_fclose(FILE *f, int *is_parametric)
{
	if (*is_parametric)
		pclose(f);
	else
		fclose(f);
}

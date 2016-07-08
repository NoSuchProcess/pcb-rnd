/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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

#include "config.h"

#include <dirent.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include <sys/stat.h>


#include <stdlib.h>
#include <string.h>

#include "mymem.h"
#include "data.h"
#include "paths.h"
#include "plugins.h"
#include "plug_footprint.h"
#include "compat_fs.h"
#include "error.h"
#include "misc.h"
#include "conf.h"
#include "conf_core.h"

static fp_type_t pcb_fp_file_type(const char *fn, void ***tags);

/* ---------------------------------------------------------------------------
 * Parse the directory tree where newlib footprints are found
 */
typedef struct list_dir_s list_dir_t;

struct list_dir_s {
	char *parent;
	char *subdir;
	list_dir_t *next;
};

typedef struct {
	library_t *menu;
	list_dir_t *subdirs;
	int children;
} list_st_t;

static int list_cb(void *cookie, const char *subdir, const char *name, fp_type_t type, void *tags[])
{
	list_st_t *l = (list_st_t *) cookie;
	library_t *e;

	if (type == PCB_FP_DIR) {
		list_dir_t *d;
		/* can not recurse directly from here because that would ruin the menu
		   pointer: GetLibraryMenuMemory (&Library) calls realloc()! 
		   Build a list of directories to be visited later, instead. */
		d = malloc(sizeof(list_dir_t));
		d->subdir = strdup(name);
		d->parent = strdup(subdir);
		d->next = l->subdirs;
		l->subdirs = d;
		return 0;
	}

	l->children++;
	e = fp_append_entry(l->menu, name, type, tags);

/* Avoid using Concat() - would be a new dependency for gsch2pcb-rnd */
	{
		int sl = strlen(subdir);
		int nl = strlen(name);
		char *end;
		
		end = e->data.fp.loc_info = malloc(sl+nl+3);
		memcpy(end, subdir, sl); end += sl;
		*end = '/'; end++;
		memcpy(end, name, nl+1); end += nl;
	}

	return 0;
}

static int fp_fs_list(library_t *pl, const char *subdir, int recurse,
								int (*cb) (void *cookie, const char *subdir, const char *name, fp_type_t type, void *tags[]), void *cookie,
								int subdir_may_not_exist, int need_tags)
{
	char olddir[MAXPATHLEN + 1];	/* The directory we start out in (cwd) */
	char new_subdir[MAXPATHLEN + 1];
	char fn[MAXPATHLEN + 1], *fn_end;
	DIR *subdirobj;								/* Interable object holding all subdir entries */
	struct dirent *subdirentry;		/* Individual subdir entry */
	struct stat buffer;						/* Buffer used in stat */
	size_t l;
	int n_footprints = 0;					/* Running count of footprints found in this subdir */

	/* Cache old dir, then cd into subdir because stat is given relative file names. */
	memset(olddir, 0, sizeof olddir);
	if (GetWorkingDirectory(olddir) == NULL) {
		Message(_("fp_fs_list(): Could not determine initial working directory\n"));
		return 0;
	}

	if (strcmp(subdir, ".svn") == 0)
		return 0;

	if (chdir(subdir)) {
		if (!subdir_may_not_exist)
			ChdirErrorMessage(subdir);
		return 0;
	}


	/* Determine subdir's abs path */
	if (GetWorkingDirectory(new_subdir) == NULL) {
		Message(_("fp_fs_list(): Could not determine new working directory\n"));
		if (chdir(olddir))
			ChdirErrorMessage(olddir);
		return 0;
	}

	l = strlen(new_subdir);
	memcpy(fn, new_subdir, l);
	fn[l] = PCB_DIR_SEPARATOR_C;
	fn_end = fn + l + 1;

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
				fp_type_t ty;
				void **tags = NULL;
				ty = pcb_fp_file_type(subdirentry->d_name, (need_tags ? &tags : NULL));
				if ((ty == PCB_FP_FILE) || (ty == PCB_FP_PARAMETRIC)) {
					n_footprints++;
					if (cb(cookie, new_subdir, subdirentry->d_name, ty, tags))
						break;
					continue;
				}
				else
					if (tags != NULL)
						free(tags);
			}

			if ((S_ISDIR(buffer.st_mode)) || (WRAP_S_ISLNK(buffer.st_mode))) {
				cb(cookie, new_subdir, subdirentry->d_name, PCB_FP_DIR, NULL);
				if (recurse) {
					n_footprints += fp_fs_list(pl, fn, recurse, cb, cookie, 0, need_tags);
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

static int fp_fs_load_dir_(library_t *pl, const char *subdir, const char *toppath, int is_root)
{
	list_st_t l;
	list_dir_t *d, *nextd;
	char working_[MAXPATHLEN + 1];
	const char *visible_subdir;
	char *working;								/* String holding abs path to working dir */

	sprintf(working_, "%s%c%s", toppath, PCB_DIR_SEPARATOR_C, subdir);
	resolve_path(working_, &working, 0);

	if (strcmp(subdir, ".") == 0)
		visible_subdir = "fs";
	else
		visible_subdir = subdir;

	l.menu = fp_lib_search(pl, visible_subdir);
	if (l.menu == NULL)
		l.menu = fp_mkdir_len(pl, visible_subdir, -1);
	l.subdirs = NULL;
	l.children = 0;

	fp_fs_list(l.menu, working, 0, list_cb, &l, is_root, 1);

	/* now recurse to each subdirectory mapped in the previous call;
	   by now we don't care if menu is ruined by the realloc() in GetLibraryMenuMemory() */
	for (d = l.subdirs; d != NULL; d = nextd) {
		l.children += fp_fs_load_dir_(l.menu, d->subdir, d->parent, 0);
		nextd = d->next;
		free(d->subdir);
		free(d->parent);
		free(d);
	}
	if ((l.children == 0) && (l.menu->data.dir.children.used == 0))
		fp_rmdir(l.menu);
	free(working);
	return l.children;
}


static int fp_fs_load_dir(plug_fp_t *ctx, const char *path)
{
	return fp_fs_load_dir_(&library, ".", path, 1);
}

typedef struct {
	const char *target;
	int target_len;
	int parametric;
	char *path;
	char *real_name;
} fp_search_t;

static int fp_search_cb(void *cookie, const char *subdir, const char *name, fp_type_t type, void *tags[])
{
	fp_search_t *ctx = (fp_search_t *) cookie;
	if ((strncmp(ctx->target, name, ctx->target_len) == 0) && ((! !ctx->parametric) == (type == PCB_FP_PARAMETRIC))) {
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

/* TODO: make this static */
char *fp_fs_search(const char *search_path, const char *basename, int parametric)
{
	const char *p, *end;
	char path[MAXPATHLEN + 1];
	fp_search_t ctx;

	if ((*basename == '/') || (*basename == PCB_DIR_SEPARATOR_C))
		return strdup(basename);

	ctx.target = basename;
	ctx.target_len = strlen(ctx.target);
	ctx.parametric = parametric;
	ctx.path = NULL;

/*	fprintf("Looking for %s\n", ctx.target);*/

	for (p = search_path; (end = strchr(p, ':')) != NULL; p = end + 1) {
		char *fpath;
		memcpy(path, p, end - p);
		path[end - p] = '\0';

		resolve_path(path, &fpath, 0);
/*		fprintf(stderr, " in '%s'\n", fpath);*/

		fp_fs_list(&library, fpath, 1, fp_search_cb, &ctx, 1, 0);
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

/* Decide about the type of a footprint file:
   - it is a file element if the first non-comment is "Element(" or "Element["
   - else it is a parametric element (footprint generator) if it contains 
     "@@" "purpose"
   - else it's not an element.
   - if a line of a file element starts with ## and doesn't contain @, it's a tag
   - if tags is not NULL, it's a pointer to a void *tags[] - an array of tag IDs
*/
static fp_type_t pcb_fp_file_type(const char *fn, void ***tags)
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

#define F_IS_PARAMETRIC 0
static FILE *fp_fs_fopen(plug_fp_t *ctx, const char *path, const char *name, fp_fopen_ctx_t *fctx)
{
	char *basename, *params, *fullname;
	FILE *f = NULL;
	const char *libshell = conf_core.rc.library_shell;

	fctx->field[F_IS_PARAMETRIC].i = fp_dupname(name, &basename, &params);
	if (basename == NULL)
		return NULL;

	fctx->backend = ctx;

	fullname = fp_fs_search(path, basename, fctx->field[F_IS_PARAMETRIC].i);
/*	fprintf(stderr, "basename=%s fullname=%s\n", basename, fullname);*/
/*	printf("pcb_fp_fopen: %d '%s' '%s' fullname='%s'\n", fctx->field[F_IS_PARAMETRIC].i, basename, params, fullname);*/


	if (fullname != NULL) {
/*fprintf(stderr, "fullname=%s param=%d\n",  fullname, fctx->field[F_IS_PARAMETRIC].i);*/
		if (fctx->field[F_IS_PARAMETRIC].i) {
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

static void fp_fs_fclose(plug_fp_t *ctx, FILE * f, fp_fopen_ctx_t *fctx)
{
	if (fctx->field[F_IS_PARAMETRIC].i)
		pclose(f);
	else
		fclose(f);
}


static plug_fp_t fp_fs;

pcb_uninit_t hid_fp_fs_init(void)
{
	fp_fs.plugin_data = NULL;
	fp_fs.load_dir = fp_fs_load_dir;
	fp_fs.fopen = fp_fs_fopen;
	fp_fs.fclose = fp_fs_fclose;
	HOOK_REGISTER(plug_fp_t, plug_fp_chain, &fp_fs);
#warning TODO: make an uninit that calls HOOK_UNREGISTER
	return NULL;
}

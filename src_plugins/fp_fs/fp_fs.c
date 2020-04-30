/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <librnd/core/compat_inc.h>

#include "board.h"
#include "data.h"
#include <librnd/core/paths.h>
#include <librnd/core/plugins.h>
#include "plug_footprint.h"
#include "plug_io.h"
#include <librnd/core/compat_fs.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/error.h>
#include <librnd/core/conf.h>
#include "conf_core.h"
#include <librnd/core/hid_init.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/safe_fs_dir.h>

/* ---------------------------------------------------------------------------
 * Parse the directory tree where newlib footprints are found
 */
typedef struct list_dir_s list_dir_t;

struct list_dir_s {
	char *parent;
	char *subdir;
	pcb_plug_fp_map_t *children;
	list_dir_t *next;
};

typedef struct {
	pcb_fplibrary_t *menu;
	list_dir_t *subdirs;
	int children;
	int is_virtual_dir;
} list_st_t;

static int list_cb(void *cookie, const char *subdir, const char *name, pcb_fptype_t type, void *tags[], pcb_plug_fp_map_t *children)
{
	list_st_t *l = (list_st_t *) cookie;
	pcb_fplibrary_t *e;

	if ((type == PCB_FP_DIR) || (type == PCB_FP_FILEDIR)) {
		list_dir_t *d;
		/* can not recurse directly from here because that would ruin the menu
		   pointer: pcb_lib_menu_new(&Library) calls realloc()!
		   Build a list of directories to be visited later, instead. */
		d = malloc(sizeof(list_dir_t));
		d->subdir = rnd_strdup(name);
		d->parent = rnd_strdup(subdir);
		d->next = l->subdirs;
		d->children = children;
		l->subdirs = d;
		return 0;
	}

	l->children++;
	e = pcb_fp_append_entry(l->menu, name, type, tags);

/* Avoid using rnd_concat() - would be a new dependency for gsch2pcb-rnd */
	{
		int sl = strlen(subdir);
		int nl = strlen(name);
		char *end;

		end = e->data.fp.loc_info = malloc(sl+nl+4);
		memcpy(end, subdir, sl); end += sl;
		if (l->is_virtual_dir) {
			*end = ':'; end++;
			*end = ':'; end++;
		}
		else {
			*end = RND_DIR_SEPARATOR_C; end++;
		}
		memcpy(end, name, nl+1); end += nl;
	}

	return 0;
}

static int fp_fs_list(pcb_fplibrary_t *pl, const char *subdir, int recurse,
								int (*cb) (void *cookie, const char *subdir, const char *name, pcb_fptype_t type, void *tags[], pcb_plug_fp_map_t *children),
								void *cookie, int subdir_may_not_exist, int need_tags)
{
	char olddir[RND_PATH_MAX + 1];	/* The directory we start out in (cwd) */
	char new_subdir[RND_PATH_MAX + 1];
	char fn[RND_PATH_MAX + 1], *fn_end;
	DIR *subdirobj;								/* Interable object holding all subdir entries */
	struct dirent *subdirentry;		/* Individual subdir entry */
	struct stat buffer;						/* Buffer used in stat */
	size_t l;
	int n_footprints = 0;					/* Running count of footprints found in this subdir */

	/* Cache old dir, then cd into subdir because stat is given relative file names. */
	memset(olddir, 0, sizeof olddir);
	if (rnd_get_wd(olddir) == NULL) {
		rnd_message(RND_MSG_ERROR, "fp_fs_list(): Could not determine initial working directory\n");
		return 0;
	}

	if (strcmp(subdir, ".svn") == 0)
		return 0;

	if (chdir(subdir)) {
		if (!subdir_may_not_exist)
			rnd_chdir_error_message(subdir);
		return 0;
	}


	/* Determine subdir's abs path */
	if (rnd_get_wd(new_subdir) == NULL) {
		rnd_message(RND_MSG_ERROR, "fp_fs_list(): Could not determine new working directory\n");
		if (chdir(olddir))
			rnd_chdir_error_message(olddir);
		return 0;
	}

	l = strlen(new_subdir);
	memcpy(fn, new_subdir, l);
	fn[l] = RND_DIR_SEPARATOR_C;
	fn[l+1] = '\0';
	fn_end = fn + l + 1;

	/* First try opening the directory specified by path */
	if ((subdirobj = pcb_opendir(&PCB->hidlib, new_subdir)) == NULL) {
		rnd_opendir_error_message(new_subdir);
		if (chdir(olddir))
			rnd_chdir_error_message(olddir);
		return 0;
	}

	/* Now loop over files in this directory looking for files.
	 * We ignore certain files which are not footprints.
	 */
	while ((subdirentry = pcb_readdir(subdirobj)) != NULL) {
#ifdef DEBUG
/*    printf("...  Examining file %s ... \n", subdirentry->d_name); */
#endif

		/* Ignore non-footprint files found in this directory
		 * We're skipping .png and .html because those
		 * may exist in a library tree to provide an html browsable
		 * index of the library.
		 */
TODO("fp: make this a configurable list")
		l = strlen(subdirentry->d_name);
		if (!stat(subdirentry->d_name, &buffer)
				&& subdirentry->d_name[0] != '.'
				&& RND_NSTRCMP(subdirentry->d_name, "CVS") != 0
				&& RND_NSTRCMP(subdirentry->d_name, "Makefile") != 0
				&& RND_NSTRCMP(subdirentry->d_name, "Makefile.am") != 0
				&& RND_NSTRCMP(subdirentry->d_name, "Makefile.in") != 0 && (l < 4 || RND_NSTRCMP(subdirentry->d_name + (l - 4), ".png") != 0)
				&& (l < 5 || RND_NSTRCMP(subdirentry->d_name + (l - 5), ".html") != 0)
				&& (l < 4 || RND_NSTRCMP(subdirentry->d_name + (l - 4), ".pcb") != 0)) {

#ifdef DEBUG
/*	printf("...  Found a footprint %s ... \n", subdirentry->d_name); */
#endif
			strcpy(fn_end, subdirentry->d_name);
			if ((S_ISREG(buffer.st_mode)) || (RND_WRAP_S_ISLNK(buffer.st_mode))) {
				pcb_plug_fp_map_t head = {0}, *res;
				res = pcb_io_map_footprint_file(&PCB->hidlib, subdirentry->d_name, &head, need_tags);
				if (res->libtype == PCB_LIB_DIR) {
					cb(cookie, new_subdir, subdirentry->d_name, PCB_FP_FILEDIR, NULL, res->next);
					res->next = NULL;
				}
				else if ((res->libtype == PCB_LIB_FOOTPRINT) && ((res->type == PCB_FP_FILE) || (res->type == PCB_FP_PARAMETRIC))) {
					n_footprints++;
					if (cb(cookie, new_subdir, subdirentry->d_name, res->type, (void **)res->tags.array, NULL))
						break;
					continue;
				}
				else {
					if (head.tags.array != NULL) {
						free(head.tags.array);
						head.tags.alloced = head.tags.used = 0;
					}
				}
			}

			if ((S_ISDIR(buffer.st_mode)) || (RND_WRAP_S_ISLNK(buffer.st_mode))) {
				cb(cookie, new_subdir, subdirentry->d_name, PCB_FP_DIR, NULL, NULL);
				if (recurse) {
					n_footprints += fp_fs_list(pl, fn, recurse, cb, cookie, 0, need_tags);
				}
				continue;
			}

		}
	}
	/* Done.  Clean up, cd back into old dir, and return */
	pcb_closedir(subdirobj);
	if (chdir(olddir))
		rnd_chdir_error_message(olddir);
	return n_footprints;
}

static int fp_fs_load_dir_(pcb_fplibrary_t *pl, const char *subdir, const char *toppath, int is_root, pcb_plug_fp_map_t *children)
{
	list_st_t l;
	list_dir_t *d, *nextd;
	char working_[RND_PATH_MAX + 1];
	const char *visible_subdir;
	char *working;								/* String holding abs path to working dir */

	sprintf(working_, "%s%c%s", toppath, RND_DIR_SEPARATOR_C, subdir);
	rnd_path_resolve(&PCB->hidlib, working_, &working, 0, rnd_false);

	/* Return error if the root is not a directory, to give other fp_ plugins a chance */
	if ((is_root) && (!rnd_is_dir(&PCB->hidlib, working))) {
		free(working);
		return -1;
	}

	if (strcmp(subdir, ".") == 0)
		visible_subdir = toppath;
	else
		visible_subdir = subdir;

	l.menu = pcb_fp_lib_search(pl, visible_subdir);
	if (l.menu == NULL)
		l.menu = pcb_fp_mkdir_len(pl, visible_subdir, -1);
	l.subdirs = NULL;
	l.children = 0;
	l.is_virtual_dir = 0;

	if (children != NULL) {
		pcb_plug_fp_map_t *n, *next;
		l.is_virtual_dir = 1;
		for(n = children; n != NULL; n = next) {
			list_cb(&l, working, n->name, n->type, (void **)n->tags.array, NULL);
			next = n->next;
			free(n);
			l.children++;
		}
		return l.children;
	}

	fp_fs_list(l.menu, working, 0, list_cb, &l, is_root, 1);

	/* now recurse to each subdirectory mapped in the previous call;
	   by now we don't care if menu is ruined by the realloc() in pcb_lib_menu_new() */
	for (d = l.subdirs; d != NULL; d = nextd) {
		l.children += fp_fs_load_dir_(l.menu, d->subdir, d->parent, 0, d->children);
		nextd = d->next;
		free(d->subdir);
		free(d->parent);
		free(d);
	}
	if ((l.children == 0) && (l.menu->data.dir.children.used == 0))
		pcb_fp_rmdir(l.menu);
	free(working);
	return l.children;
}


static int fp_fs_load_dir(pcb_plug_fp_t *ctx, const char *path, int force)
{
	int res;

	res = fp_fs_load_dir_(&pcb_library, ".", path, 1, NULL);
	if (res >= 0) {
		pcb_fplibrary_t *l = pcb_fp_lib_search(&pcb_library, path);
		if ((l != NULL) && (l->type == PCB_LIB_DIR))
			l->data.dir.backend = ctx;
	}
	return res;
}

typedef struct {
	const char *target;
	int target_len;
	int parametric;
	char *path;
	char *real_name;
} fp_search_t;

static int fp_search_cb(void *cookie, const char *subdir, const char *name, pcb_fptype_t type, void *tags[], pcb_plug_fp_map_t *children)
{
	fp_search_t *ctx = (fp_search_t *) cookie;
	if ((strncmp(ctx->target, name, ctx->target_len) == 0) && ((! !ctx->parametric) == (type == PCB_FP_PARAMETRIC))) {
		const char *suffix = name + ctx->target_len;
		/* ugly heuristics: footprint names may end in .fp or .ele or .subc.lht or .lht */
		if ((*suffix == '\0') || (rnd_strcasecmp(suffix, ".fp") == 0) || (rnd_strcasecmp(suffix, ".ele") == 0) || (rnd_strcasecmp(suffix, ".subc.lht") == 0) || (rnd_strcasecmp(suffix, ".lht") == 0)) {
			ctx->path = rnd_strdup(subdir);
			ctx->real_name = rnd_strdup(name);
			return 1;
		}
	}
	return 0;
}

/* walk the search_path for finding the first footprint for basename (shall not contain "(") */
static char *fp_fs_search(const char *search_path, const char *basename, int parametric)
{
	const char *p, *end;
	char path[RND_PATH_MAX + 1];
	fp_search_t ctx;

	if (rnd_is_path_abs(basename))
		return rnd_strdup(basename);

	ctx.target = basename;
	ctx.target_len = strlen(ctx.target);
	ctx.parametric = parametric;
	ctx.path = NULL;

/*	fprintf("Looking for %s\n", ctx.target);*/

	for (p = search_path; *p != '\0'; p = end + 1) {
		char *fpath;
		end = strchr(p, ':');
		if (end != NULL) {
			memcpy(path, p, end - p);
			path[end - p] = '\0';
		}
		else
			strcpy(path, p);

		rnd_path_resolve(&PCB->hidlib, path, &fpath, 0, rnd_false);
/*		fprintf(stderr, " in '%s'\n", fpath);*/

		fp_fs_list(&pcb_library, fpath, 1, fp_search_cb, &ctx, 1, 0);
		if (ctx.path != NULL) {
			sprintf(path, "%s%c%s", ctx.path, RND_DIR_SEPARATOR_C, ctx.real_name);
			free(ctx.path);
			free(ctx.real_name);
/*			fprintf("  found '%s'\n", path);*/
			free(fpath);
			return rnd_strdup(path);
		}
		free(fpath);
		if (end == NULL)
			break;
	}
	return NULL;
}

#define F_IS_PARAMETRIC 0
#define F_TMPNAME 1
static FILE *fp_fs_fopen(pcb_plug_fp_t *ctx, const char *path, const char *name, pcb_fp_fopen_ctx_t *fctx, pcb_data_t *dst)
{
	char *basename, *params, *fullname;
	FILE *fp, *f = NULL;
	const char *libshell;

	fctx->field[F_TMPNAME].p = NULL;
	fctx->field[F_IS_PARAMETRIC].i = pcb_fp_dupname(name, &basename, &params);
	if (basename == NULL)
		return NULL;

	fctx->backend = ctx;

	fullname = fp_fs_search(path, basename, fctx->field[F_IS_PARAMETRIC].i);
/*	fprintf(stderr, "basename=%s fullname=%s\n", basename, fullname);*/
/*	printf("pcb_fp_fopen: %d '%s' '%s' fullname='%s'\n", fctx->field[F_IS_PARAMETRIC].i, basename, params, fullname);*/


	if (fullname != NULL) {
/*fprintf(stderr, "fullname=%s param=%d\n",  fullname, fctx->field[F_IS_PARAMETRIC].i);*/
		if (fctx->field[F_IS_PARAMETRIC].i) {
			char *cmd;
			const char *sep = " ";
			libshell = conf_core.rc.library_shell;
			if (libshell == NULL) {
				libshell = "";
				sep = "";
			}
#ifdef __WIN32__
			{
				char *s;
				cmd = malloc(strlen(rnd_w32_bindir) + strlen(libshell) + strlen(fullname) + strlen(params) + 32);
				sprintf(cmd, "%s/sh -c \"%s%s%s %s\"", rnd_w32_bindir, libshell, sep, fullname, params);
				for(s = cmd; *s != '\0'; s++)
					if (*s == '\\')
						*s = '/';
			}
#else
			cmd = malloc(strlen(libshell) + strlen(fullname) + strlen(params) + 16);
			sprintf(cmd, "%s%s%s %s", libshell, sep, fullname, params);
#endif
/*fprintf(stderr, " cmd=%s\n",  cmd);*/
			/* Make a copy of the output of the parametric so rewind() can be called on it */
			fctx->field[F_TMPNAME].p = rnd_tempfile_name_new("pcb-rnd-pfp");
			f = rnd_fopen(&PCB->hidlib, (char *)fctx->field[F_TMPNAME].p, "wb+");
			if (f != NULL) {
				char buff[4096];
				int len;
				fp = rnd_popen(&PCB->hidlib, cmd, "r");
				if (fp != NULL) {
					while((len = fread(buff, 1, sizeof(buff), fp)) > 0)
						fwrite(buff, 1, len, f);
					rnd_pclose(fp);
				}
				else
					rnd_message(RND_MSG_ERROR, "Parametric footprint: failed to execute %s\n", cmd);
				rewind(f);
			}
			free(cmd);
		}
		else
			f = rnd_fopen(&PCB->hidlib, fullname, "rb");
		free(fullname);
	}

	free(basename);
	return f;
}

static void fp_fs_fclose(pcb_plug_fp_t *ctx, FILE * f, pcb_fp_fopen_ctx_t *fctx)
{
	fclose(f);
	if (fctx->field[F_TMPNAME].p != NULL)
		rnd_tempfile_unlink((char *)fctx->field[F_TMPNAME].p);
}


static pcb_plug_fp_t fp_fs;

int pplg_check_ver_fp_fs(int ver_needed) { return 0; }

void pplg_uninit_fp_fs(void)
{
	RND_HOOK_UNREGISTER(pcb_plug_fp_t, pcb_plug_fp_chain, &fp_fs);
}

int pplg_init_fp_fs(void)
{
	RND_API_CHK_VER;
	fp_fs.plugin_data = NULL;
	fp_fs.load_dir = fp_fs_load_dir;
	fp_fs.fp_fopen = fp_fs_fopen;
	fp_fs.fp_fclose = fp_fs_fclose;
	RND_HOOK_REGISTER(pcb_plug_fp_t, pcb_plug_fp_chain, &fp_fs);
	return 0;
}

 /*
  *                            COPYRIGHT
  *
  *  pcb-rnd, interactive printed circuit board design
  *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

/* permit direct access to the libc calls (turn off config.h masking) */
#define PCB_SAFE_FS

/* temporary */
#define TODO_hidlib NULL

/* opendir, readdir */
#include "compat_inc.h"

#include "config.h"


#include <stdio.h>
#include <stdlib.h>

#include "safe_fs.h"

#include "compat_fs.h"
#include "compat_misc.h"
#include "error.h"
#include "globalconst.h"
#include "paths.h"


/* Evaluates op(arg1,arg2); returns 0 if the operation is permitted */
static int pcb_safe_fs_check(const char *op, const char *arg1, const char *arg2)
{
	return 0;
}

/* Evaluate op(arg1,arg2) and print error and return err_ret if not permitted */
#define CHECK(func, op, arg1, arg2, err_inst) \
do { \
	if (pcb_safe_fs_check(op, arg1, arg2) != 0) { \
		pcb_message(PCB_MSG_ERROR, "File system operation %s(): access denied on %s(%s,%s)\n", func, op, arg1, arg2); \
		err_inst; \
	} \
} while(0)

FILE *pcb_fopen_fn(const char *path, const char *mode, char **fn_out)
{
	FILE *f;
	char *path_exp;

	/* skip expensive path building for empty paths that are going to fail anyway */
	if ((path == NULL) || (*path == '\0')) {
		if (fn_out != NULL)
			*fn_out = NULL;
		return NULL;
	}

	path_exp = pcb_build_fn(TODO_hidlib, path);

	CHECK("fopen", "access", path_exp, mode, goto err);
	CHECK("fopen", "fopen", path_exp, mode, goto err);

	f = fopen(path_exp, mode);
	if (f == NULL)
		goto err;

	if (fn_out != NULL)
		*fn_out = path_exp;
	else
		free(path_exp);

	return f;

	err:;
	free(path_exp);
	if (fn_out != NULL)
		*fn_out = NULL;
	return NULL;
}

FILE *pcb_fopen(const char *path, const char *mode)
{
	return pcb_fopen_fn(path, mode, NULL);
}

char *pcb_fopen_check(const char *path, const char *mode)
{
	char *path_exp = pcb_build_fn(TODO_hidlib, path);

	CHECK("fopen", "access", path_exp, mode, goto err);
	CHECK("fopen", "fopen", path_exp, mode, goto err);
	return path_exp;

	err:;
	free(path_exp);
	return NULL;
}

FILE *pcb_popen(const char *cmd, const char *mode)
{
	FILE *f = NULL;
	char *cmd_exp = pcb_build_fn(TODO_hidlib, cmd);

	CHECK("popen", "access", cmd_exp, mode, goto err);
	CHECK("popen", "exec", cmd_exp, NULL, goto err);
	CHECK("popen", "popen", cmd_exp, mode, goto err);

	f = popen(cmd_exp, mode);

	err:;
	free(cmd_exp);
	return f;
}

int pcb_pclose(FILE *f)
{
	return pclose(f);
}


int pcb_system(const char *cmd)
{
	int res = -1;
	char *cmd_exp = pcb_build_fn(TODO_hidlib, cmd);

	CHECK("access", "access", cmd_exp, "r", goto err);
	CHECK("access", "exec", cmd_exp, NULL, goto err);
	CHECK("access", "system", cmd_exp, NULL, goto err);

	res = system(cmd_exp);

	err:;
	free(cmd_exp);
	return res;
}

int pcb_remove(const char *path)
{
	int res = -1;
	char *path_exp = pcb_build_fn(TODO_hidlib, path);

	CHECK("remove", "access", path_exp, "w", goto err);
	CHECK("remove", "remove", path_exp, NULL, goto err);

	res = remove(path);

	err:;
	free(path_exp);
	return res;
}

int pcb_rename(const char *old_path, const char *new_path)
{
	int res = -1;
	char *old_path_exp = pcb_build_fn(TODO_hidlib, old_path);
	char *new_path_exp = pcb_build_fn(TODO_hidlib, new_path);

	CHECK("rename", "access", old_path_exp, "w", goto err);
	CHECK("rename", "access", new_path_exp, "w", goto err);
	CHECK("rename", "rename", old_path_exp, new_path_exp, goto err);

	res = rename(old_path_exp, new_path_exp);

	err:;
	free(old_path_exp);
	free(new_path_exp);
	return res;
}

int pcb_unlink(const char *path)
{
	int res;
	char *path_exp = pcb_build_fn(TODO_hidlib, path);
	res = unlink(path_exp);
	free(path_exp);
	return res;
}


DIR *pcb_opendir(const char *name)
{
	DIR *d;
	char *path_exp = pcb_build_fn(TODO_hidlib, name);
	d = opendir(path_exp);
	free(path_exp);
	return d;
}

struct dirent *pcb_readdir(DIR *dir)
{
	return readdir(dir);
}

int pcb_closedir(DIR *dir)
{
	return closedir(dir);
}


static FILE *pcb_fopen_at_(const char *from, const char *fn, const char *mode, char **full_path, int recursive)
{
	char tmp[PCB_PATH_MAX];
	DIR *d;
	struct dirent *de;
	FILE *res;

	/* try the trivial: directly under this  dir */
	pcb_snprintf(tmp, sizeof(tmp), "%s%c%s", from, PCB_DIR_SEPARATOR_C, fn);
	res = pcb_fopen(tmp, mode);

	if (res != NULL) {
		if (full_path != NULL)
			*full_path = pcb_strdup(tmp);
		return res;
	}

	/* no luck, recurse into each subdir */
	if (!recursive)
		return NULL;

	d = pcb_opendir(from);
	if (d == NULL)
		return NULL;

	while((de = pcb_readdir(d)) != NULL) {
		struct stat st;
		if (de->d_name[0] == '.')
			continue;
		pcb_snprintf(tmp, sizeof(tmp), "%s%c%s", from, PCB_DIR_SEPARATOR_C, de->d_name);
		if (stat(tmp, &st) != 0)
			continue;
		if (!S_ISDIR(st.st_mode))
			continue;

		/* dir: recurse */
		res = pcb_fopen_at_(tmp, fn, mode, full_path, recursive);
		if (res != NULL) {
			closedir(d);
			return res;
		}
	}
	closedir(d);
	return NULL;
}

FILE *pcb_fopen_at(const char *dir, const char *fn, const char *mode, char **full_path, int recursive)
{
	if (full_path != NULL)
		*full_path = NULL;

	return pcb_fopen_at_(dir, fn, mode, full_path, recursive);
}

FILE *pcb_fopen_first(const conflist_t *paths, const char *fn, const char *mode, char **full_path, int recursive)
{
	FILE *res;
	char *real_fn = pcb_build_fn(TODO_hidlib, fn);
	conf_listitem_t *ci;

	if (full_path != NULL)
		*full_path = NULL;

	if (real_fn == NULL)
		return NULL;

	if (pcb_is_path_abs(fn)) {
		res = pcb_fopen(real_fn, mode);
		if ((res != NULL) && (full_path != NULL))
			*full_path = real_fn;
		else
			free(real_fn);
		return res;
	}

	/* have to search paths */
	{
		for (ci = conflist_first((conflist_t *)paths); ci != NULL; ci = conflist_next(ci)) {
			const char *p = ci->val.string[0];
			char *real_p;
			size_t pl;

			if (ci->type != CFN_STRING)
				continue;
			if (*p == '?')
				p++;

			/* resolve the path from the list, truncate trailing '/' */
			real_p = pcb_build_fn(TODO_hidlib, p);
			pl = strlen(real_p);
			if ((pl > 0) && (real_p[pl-1] == '/'))
				real_p[pl-1] = '\0';
	
			res = pcb_fopen_at(real_p, real_fn, mode, full_path, recursive);
			free(real_p);

			if (res != NULL) {
				free(real_fn);
				return res;
			}
		}
	}

	return NULL;
}

extern int pcb_mkdir_(const char *path, int mode);
int pcb_mkdir(const char *path, int mode)
{
	CHECK("mkdir", "access", path, NULL, return -1);
	CHECK("mkdir", "mkdir", path, NULL, return -1);

	return pcb_mkdir_(path, mode);
}


extern long pcb_file_size_(const char *path);
long pcb_file_size(const char *path)
{
	CHECK("file_size", "access", path, NULL, return -1);
	CHECK("file_size", "stat", path, NULL, return -1);
	return pcb_file_size_(path);
}

extern int pcb_is_dir_(const char *path);
int pcb_is_dir(const char *path)
{
	CHECK("is_dir", "access", path, NULL, return -1);
	CHECK("is_dir", "stat", path, NULL, return -1);
	return pcb_is_dir_(path);
}

extern double pcb_file_mtime_(const char *path);
double pcb_file_mtime(const char *path)
{
	CHECK("file_mtime", "access", path, NULL, return -1);
	CHECK("file_mtime", "stat", path, NULL, return -1);
	return pcb_file_mtime_(path);
}

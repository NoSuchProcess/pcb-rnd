 /*
  *                            COPYRIGHT
  *
  *  pcb-rnd, interactive printed circuit board design
  *  Copyright (C) 2017,2019 Tibor 'Igor2' Palinkas
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

/* opendir, readdir */
#include <librnd/core/compat_inc.h>

#include <librnd/config.h>


#include <stdio.h>
#include <stdlib.h>

#include <librnd/core/safe_fs.h>

#include <librnd/core/actions.h>
#include <librnd/core/compat_fs.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/error.h>
#include <librnd/core/globalconst.h>
#include <librnd/core/paths.h>
#include <librnd/core/hidlib.h>


/* Evaluates op(arg1,arg2); returns 0 if the operation is permitted */
static int pcb_safe_fs_check(const char *op, const char *arg1, const char *arg2)
{
	return 0;
}

/* Evaluate op(arg1,arg2) and print error and return err_ret if not permitted */
#define CHECK(func, op, arg1, arg2, err_inst) \
do { \
	if (pcb_safe_fs_check(op, arg1, arg2) != 0) { \
		rnd_message(PCB_MSG_ERROR, "File system operation %s(): access denied on %s(%s,%s)\n", func, op, arg1, arg2); \
		err_inst; \
	} \
} while(0)

FILE *pcb_fopen_fn(rnd_hidlib_t *hidlib, const char *path, const char *mode, char **fn_out)
{
	FILE *f;
	char *path_exp;

	if (fn_out != NULL)
		*fn_out = NULL;

	/* skip expensive path building for empty paths that are going to fail anyway */
	if ((path == NULL) || (*path == '\0'))
		return NULL;

	path_exp = pcb_build_fn(hidlib, path);
	if (path_exp == NULL)
		return NULL;

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

FILE *pcb_fopen(rnd_hidlib_t *hidlib, const char *path, const char *mode)
{
	return pcb_fopen_fn(hidlib, path, mode, NULL);
}

FILE *pcb_fopen_askovr(rnd_hidlib_t *hidlib, const char *path, const char *mode, int *all)
{
	if (hidlib->batch_ask_ovr != NULL)
		all = hidlib->batch_ask_ovr;
	if (strchr(mode, 'w') != NULL) {
		fgw_func_t *fun = rnd_act_lookup("gui_mayoverwritefile");

		/* if the action does not exist, use the old behavor: just overwrite anything */
		if (fun != NULL) {
			FILE *f = pcb_fopen(hidlib, path, "r");
			if (f != NULL) {
				int res = 0;
				fclose(f);
				if (all != NULL)
					res = *all;
				if (res != 2) {
					fgw_arg_t ares, argv[3];

					argv[0].type = FGW_FUNC; argv[0].val.argv0.func = fun; argv[0].val.argv0.user_call_ctx = hidlib;
					argv[1].type = FGW_STR;  argv[1].val.cstr = path;
					argv[2].type = FGW_INT;  argv[2].val.nat_int = (all != NULL);
					ares.type = FGW_INVALID;

					if ((rnd_actionv_(fun, &ares, 3, argv) != 0) || (fgw_arg_conv(&rnd_fgw, &ares, FGW_INT) != 0))
						res = -1;
					else
						res = ares.val.nat_int;
				}
				if (res == 0)
					return NULL;
				if ((all != NULL) && (res == 2)) /* remember 'overwrite all' for this session */
					*all = 2;
			}
		}
	}
	return pcb_fopen(hidlib, path, mode);
}

int *pcb_batched_ask_ovr_init(rnd_hidlib_t *hidlib, int *storage)
{
	int *old = hidlib->batch_ask_ovr;
	if (hidlib->batch_ask_ovr != NULL)
		*storage = *hidlib->batch_ask_ovr;
	hidlib->batch_ask_ovr = storage;
	return old;
}

void pcb_batched_ask_ovr_uninit(rnd_hidlib_t *hidlib, int *init_retval)
{
	if (init_retval != NULL)
		*init_retval = *hidlib->batch_ask_ovr;
	hidlib->batch_ask_ovr = init_retval;
}

char *pcb_fopen_check(rnd_hidlib_t *hidlib, const char *path, const char *mode)
{
	char *path_exp = pcb_build_fn(hidlib, path);

	CHECK("fopen", "access", path_exp, mode, goto err);
	CHECK("fopen", "fopen", path_exp, mode, goto err);
	return path_exp;

	err:;
	free(path_exp);
	return NULL;
}

FILE *pcb_popen(rnd_hidlib_t *hidlib, const char *cmd, const char *mode)
{
	FILE *f = NULL;
	char *cmd_exp = pcb_build_fn(hidlib, cmd);

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


int pcb_system(rnd_hidlib_t *hidlib, const char *cmd)
{
	int res = -1;
	char *cmd_exp = pcb_build_fn(hidlib, cmd);

	CHECK("access", "access", cmd_exp, "r", goto err);
	CHECK("access", "exec", cmd_exp, NULL, goto err);
	CHECK("access", "system", cmd_exp, NULL, goto err);

	res = system(cmd_exp);

	err:;
	free(cmd_exp);
	return res;
}

int pcb_remove(rnd_hidlib_t *hidlib, const char *path)
{
	int res = -1;
	char *path_exp = pcb_build_fn(hidlib, path);

	CHECK("remove", "access", path_exp, "w", goto err);
	CHECK("remove", "remove", path_exp, NULL, goto err);

	res = remove(path);

	err:;
	free(path_exp);
	return res;
}

int pcb_rename(rnd_hidlib_t *hidlib, const char *old_path, const char *new_path)
{
	int res = -1;
	char *old_path_exp = pcb_build_fn(hidlib, old_path);
	char *new_path_exp = pcb_build_fn(hidlib, new_path);

	CHECK("rename", "access", old_path_exp, "w", goto err);
	CHECK("rename", "access", new_path_exp, "w", goto err);
	CHECK("rename", "rename", old_path_exp, new_path_exp, goto err);

	res = rename(old_path_exp, new_path_exp);

	err:;
	free(old_path_exp);
	free(new_path_exp);
	return res;
}

int pcb_unlink(rnd_hidlib_t *hidlib, const char *path)
{
	int res;
	char *path_exp = pcb_build_fn(hidlib, path);
	res = unlink(path_exp);
	free(path_exp);
	return res;
}


DIR *pcb_opendir(rnd_hidlib_t *hidlib, const char *name)
{
	DIR *d;
	char *path_exp = pcb_build_fn(hidlib, name);
	if (path_exp == NULL)
		return NULL;
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


static FILE *pcb_fopen_at_(rnd_hidlib_t *hidlib, const char *from, const char *fn, const char *mode, char **full_path, int recursive)
{
	char tmp[PCB_PATH_MAX];
	DIR *d;
	struct dirent *de;
	FILE *res;

	/* try the trivial: directly under this  dir */
	pcb_snprintf(tmp, sizeof(tmp), "%s%c%s", from, RND_DIR_SEPARATOR_C, fn);
	res = pcb_fopen(hidlib, tmp, mode);

	if (res != NULL) {
		if (full_path != NULL)
			*full_path = pcb_strdup(tmp);
		return res;
	}

	/* no luck, recurse into each subdir */
	if (!recursive)
		return NULL;

	d = pcb_opendir(hidlib, from);
	if (d == NULL)
		return NULL;

	while((de = pcb_readdir(d)) != NULL) {
		struct stat st;
		if (de->d_name[0] == '.')
			continue;
		pcb_snprintf(tmp, sizeof(tmp), "%s%c%s", from, RND_DIR_SEPARATOR_C, de->d_name);
		if (stat(tmp, &st) != 0)
			continue;
		if (!S_ISDIR(st.st_mode))
			continue;

		/* dir: recurse */
		res = pcb_fopen_at_(hidlib, tmp, fn, mode, full_path, recursive);
		if (res != NULL) {
			closedir(d);
			return res;
		}
	}
	closedir(d);
	return NULL;
}

FILE *pcb_fopen_at(rnd_hidlib_t *hidlib, const char *dir, const char *fn, const char *mode, char **full_path, int recursive)
{
	if (full_path != NULL)
		*full_path = NULL;

	return pcb_fopen_at_(hidlib, dir, fn, mode, full_path, recursive);
}

FILE *pcb_fopen_first(rnd_hidlib_t *hidlib, const pcb_conflist_t *paths, const char *fn, const char *mode, char **full_path, int recursive)
{
	FILE *res;
	char *real_fn = pcb_build_fn(hidlib, fn);
	pcb_conf_listitem_t *ci;

	if (full_path != NULL)
		*full_path = NULL;

	if (real_fn == NULL)
		return NULL;

	if (rnd_is_path_abs(fn)) {
		res = pcb_fopen(hidlib, real_fn, mode);
		if ((res != NULL) && (full_path != NULL))
			*full_path = real_fn;
		else
			free(real_fn);
		return res;
	}

	/* have to search paths */
	{
		for (ci = pcb_conflist_first((pcb_conflist_t *)paths); ci != NULL; ci = pcb_conflist_next(ci)) {
			const char *p = ci->val.string[0];
			char *real_p;
			size_t pl;

			if (ci->type != CFN_STRING)
				continue;
			if (*p == '?')
				p++;

			/* resolve the path from the list, truncate trailing '/' */
			real_p = pcb_build_fn(hidlib, p);
			pl = strlen(real_p);
			if ((pl > 0) && (real_p[pl-1] == '/'))
				real_p[pl-1] = '\0';
	
			res = pcb_fopen_at(hidlib, real_p, real_fn, mode, full_path, recursive);
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
int pcb_mkdir(rnd_hidlib_t *hidlib, const char *path, int mode)
{
	CHECK("mkdir", "access", path, NULL, return -1);
	CHECK("mkdir", "mkdir", path, NULL, return -1);

	return pcb_mkdir_(path, mode);
}


extern long pcb_file_size_(const char *path);
long pcb_file_size(rnd_hidlib_t *hidlib, const char *path)
{
	CHECK("file_size", "access", path, NULL, return -1);
	CHECK("file_size", "stat", path, NULL, return -1);
	return pcb_file_size_(path);
}

extern int pcb_is_dir_(const char *path);
int pcb_is_dir(rnd_hidlib_t *hidlib, const char *path)
{
	CHECK("is_dir", "access", path, NULL, return -1);
	CHECK("is_dir", "stat", path, NULL, return -1);
	return pcb_is_dir_(path);
}

extern double pcb_file_mtime_(const char *path);
double pcb_file_mtime(rnd_hidlib_t *hidlib, const char *path)
{
	CHECK("file_mtime", "access", path, NULL, return -1);
	CHECK("file_mtime", "stat", path, NULL, return -1);
	return pcb_file_mtime_(path);
}

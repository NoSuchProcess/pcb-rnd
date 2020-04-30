/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,2004,2006 Thomas Nau
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


/* misc functions used by several modules */

#define PCB_SAFE_FS
#include <librnd/config.h>

#include <librnd/core/compat_inc.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <signal.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <librnd/core/compat_fs.h>
#include <librnd/core/compat_misc.h>
#include "globalconst.h"
#include <librnd/core/safe_fs.h>
#include <librnd/core/hid_init.h>

#include <genvector/gds_char.h>

#include <librnd/core/error.h>

char *rnd_get_wd(char *path)
{
#if defined(RND_HAVE_GETCWD)
	return getcwd(path, RND_PATH_MAX);
#elif defined(RND_HAVE__GETCWD)
	return _getcwd(path, RND_PATH_MAX);
#else
	/* seems that some BSD releases lack of a prototype for getwd() */
	return getwd(path);
#endif
}

#if defined(RND_USE_MKDIR)
#	define MKDIR mkdir
#elif defined(RND_USE__MKDIR)
#	define MKDIR _mkdir
#else
#	error no mkdir() available
#endif
int pcb_mkdir_(const char *path, int mode)
{
#if RND_MKDIR_NUM_ARGS == 1
	return MKDIR(path);
#elif RND_MKDIR_NUM_ARGS == 2
	return MKDIR(path, mode);
#else
#	error invalid number of arguments for mkdir
#endif
}
#undef MKDIR

int rnd_file_readable(const char *path)
{
	FILE *f;
	f = pcb_fopen(NULL, path, "r");
	if (f != NULL) {
		fclose(f);
		return 1;
	}
	return 0;
}

int rnd_spawnvp(const char **argv)
{
#if defined(RND_USE_SPAWNVP)
	int result = _spawnvp(_P_WAIT, argv[0], (const char *const *) argv);
	if (result == -1)
		return 1;
	else
		return 0;
#elif defined(RND_USE_FORK_WAIT)
	int pid;
	pid = fork();
	if (pid < 0) {
		/* error */
		rnd_message(RND_MSG_ERROR, "Cannot fork!");
		return 1;
	}
	else if (pid == 0) {
		/* Child */
		execvp(argv[0], (char* const*) argv);
		exit(1);
	}
	else {
		int rv;
		/* Parent */
		wait(&rv);
	}
	return 0;
#else
#	error Do not know how to run a background process.
#endif
}


/* Hopefully the operating system
 * provides a mkdtemp() function to securely create a temporary
 * directory with mode 0700.  If so then that directory is created and
 * the returned string is made up of the directory plus the name
 * variable.  For example:
 *
 * rnd_tempfile_name_new("myfile") might return
 * "/var/tmp/pcb.123456/myfile".
 *
 * If mkdtemp() is not available then 'name' is ignored and the
 * insecure tmpnam() function is used.
 *
 * Files/names created with rnd_tempfile_name_new() should be unlinked
 * with tempfile_unlink to make sure the temporary directory is also
 * removed when mkdtemp() is used.
 */
char *rnd_tempfile_name_new(const char *name)
{
	char *tmpfile = NULL;

#ifdef __WIN32__
	const char *tmpdir;
	char *res, *c, num[256];
	unsigned int r1 = rand(), r2 = rand(); /* weaker fallback */

	rand_s(&r1); rand_s(&r2); /* crypto secure according to the API doc */

	sprintf(num, "%lu%lu", r1, r2);
	tmpdir = rnd_w32_cachedir;
	res = pcb_concat(tmpdir, "/", num, NULL);
	for(c = res; *c; c++)
		if (*c == '\\')
			*c = '/';
	return res;
#else
#ifdef RND_HAVE_MKDTEMP
#ifdef inline
	/* Suppress compiler warnings; -Dinline means we are compiling in
	   --debug with -ansi -pedantic; we do know that mkdtemp exists on the system,
	   since RND_HAVE_MKDTEMP is set. */
	char *mkdtemp(char *template);
#endif
	const char *tmpdir;
	char *mytmpdir;
	size_t len;
#endif

	assert(name != NULL);

#ifdef RND_HAVE_MKDTEMP
#define TEMPLATE "pcb.XXXXXXXX"


	tmpdir = getenv("TMPDIR");

	/* FIXME -- what about win32? */
	if (tmpdir == NULL) {
		tmpdir = "/tmp";
	}

	mytmpdir = (char *) malloc(sizeof(char) * (strlen(tmpdir) + 1 + strlen(TEMPLATE) + 1));
	if (mytmpdir == NULL) {
		fprintf(stderr, "rnd_tempfile_name_new(): malloc failed()\n");
		exit(1);
	}

	*mytmpdir = '\0';
	(void) strcat(mytmpdir, tmpdir);
	(void) strcat(mytmpdir, RND_DIR_SEPARATOR_S);
	(void) strcat(mytmpdir, TEMPLATE);
	if (mkdtemp(mytmpdir) == NULL) {
		fprintf(stderr, "rnd_spawnvp():  mkdtemp (\"%s\") failed\n", mytmpdir);
		free(mytmpdir);
		return NULL;
	}


	len = strlen(mytmpdir) +  /* the temp directory name */
		1 +                     /* the directory sep. */
		strlen(name) +          /* the file name */
		1                       /* the \0 termination */
		;

	tmpfile = (char *) malloc(sizeof(char) * len);

	*tmpfile = '\0';
	(void) strcat(tmpfile, mytmpdir);
	(void) strcat(tmpfile, RND_DIR_SEPARATOR_S);
	(void) strcat(tmpfile, name);

	free(mytmpdir);
#undef TEMPLATE
#else
	/*
	 * tmpnam() uses a static buffer so rnd_strdup() the result right away
	 * in case someone decides to create multiple temp names.
	 */
	tmpfile = rnd_strdup(tmpnam(NULL));
#endif
	return tmpfile;
#endif
}

/* If we have mkdtemp() then our temp file lives in a temporary directory and
 * we need to remove that directory too. */
int rnd_tempfile_unlink(char *name)
{
#ifdef DEBUG
	/* SDB says:  Want to keep old temp files for examination when debugging */
	return 0;
#endif

#ifdef RND_HAVE_MKDTEMP
	int e, rc2 = 0;
	char *dname;

	unlink(name);
	/* it is possible that the file was never created so it is OK if the
	   unlink fails */

	/* now figure out the directory name to remove */
	e = strlen(name) - 1;
	while (e > 0 && name[e] != RND_DIR_SEPARATOR_C) {
		e--;
	}

	dname = rnd_strdup(name);
	dname[e] = '\0';

	/*
	 * at this point, e *should* point to the end of the directory part
	 * but lets make sure.
	 */
	if (e > 0) {
		rc2 = rmdir(dname);
		if (rc2 != 0) {
			perror(dname);
		}

	}
	else {
		fprintf(stderr, "rnd_tempfile_unlink():  Unable to determine temp directory name from the temp file\n");
		fprintf(stderr, "rnd_tempfile_unlink():  \"%s\"\n", name);
		rc2 = -1;
	}

	/* name was allocated with malloc */
	free(dname);
	free(name);

	/*
	 * FIXME - should also return -1 if the temp file exists and was not
	 * removed.
	 */
	if (rc2 != 0) {
		return -1;
	}

#else
	int rc = unlink(name);

	if (rc != 0) {
		fprintf(stderr, "Failed to unlink \"%s\"\n", name);
		free(name);
		return rc;
	}
	free(name);

#endif

	return 0;
}

int pcb_is_dir_(const char *path)
{
	struct stat st;
	if (stat(path, &st) != 0)
		return 0;
	return S_ISDIR(st.st_mode);
}

long pcb_file_size_(const char *path)
{
	struct stat st;
	if (stat(path, &st) != 0)
		return -1;
	if (st.st_size > LONG_MAX)
		return -1;
	return st.st_size;
}

double pcb_file_mtime_(const char *path)
{
	struct stat st;
	if (stat(path, &st) != 0)
		return -1;
	return st.st_mtime;
}

int rnd_is_path_abs(const char *fn)
{
#ifdef __WIN32__
	/* full path with drive, e.g. c:\foo */
	if (isalpha(fn[0]) && (fn[1] == ':') && ((fn[2] == '\\') || (fn[2] == '/')))
		return 1;

	/* full path without drive and remote paths, e.g. \\workgroup\foo */
	if ((fn[0] == '\\') || (fn[0] == '/'))
		return 1;

	return 0;
#else
	return (*fn == '/');
#endif
}

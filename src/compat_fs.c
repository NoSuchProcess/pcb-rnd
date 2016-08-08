/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */


/* misc functions used by several modules */

#include "config.h"

#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <memory.h>
#include <ctype.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include <assert.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <genvector/gds_char.h>

#include "global.h"

#include "error.h"
#include "mymem.h"

/* ----------------------------------------------------------------------
 * returns pointer to current working directory.  If 'path' is not
 * NULL, then the current working directory is copied to the array
 * pointed to by 'path'
 */
char *GetWorkingDirectory(char *path)
{
#ifdef HAVE_GETCWD
	return getcwd(path, MAXPATHLEN);
#else
	/* seems that some BSD releases lack of a prototype for getwd() */
	return getwd(path);
#endif
}

#ifdef MKDIR_IS_PCBMKDIR
#error "Don't know how to create a directory on this system."
int pcb_mkdir(const char *path, int mode)
{
	return MKDIR(path, mode);
}
#endif

int pcb_spawnvp(char **argv)
{
#ifdef HAVE__SPAWNVP
	int result = _spawnvp(_P_WAIT, argv[0], (const char *const *) argv);
	if (result == -1)
		return 1;
	else
		return 0;
#else
	int pid;
	pid = fork();
	if (pid < 0) {
		/* error */
		Message(_("Cannot fork!"));
		return 1;
	}
	else if (pid == 0) {
		/* Child */
		execvp(argv[0], argv);
		exit(1);
	}
	else {
		int rv;
		/* Parent */
		wait(&rv);
	}
	return 0;
#endif
}


/* 
 * Creates a new temporary file name.  Hopefully the operating system
 * provides a mkdtemp() function to securily create a temporary
 * directory with mode 0700.  If so then that directory is created and
 * the returned string is made up of the directory plus the name
 * variable.  For example:
 *
 * tempfile_name_new ("myfile") might return
 * "/var/tmp/pcb.123456/myfile".
 *
 * If mkdtemp() is not available then 'name' is ignored and the
 * insecure tmpnam() function is used.
 *  
 * Files/names created with tempfile_name_new() should be unlinked
 * with tempfile_unlink to make sure the temporary directory is also
 * removed when mkdtemp() is used.
 */
char *tempfile_name_new(char *name)
{
	char *tmpfile = NULL;
#ifdef HAVE_MKDTEMP
	char *tmpdir, *mytmpdir;
	size_t len;
#endif

	assert(name != NULL);

#ifdef HAVE_MKDTEMP
#define TEMPLATE "pcb.XXXXXXXX"


	tmpdir = getenv("TMPDIR");

	/* FIXME -- what about win32? */
	if (tmpdir == NULL) {
		tmpdir = "/tmp";
	}

	mytmpdir = (char *) malloc(sizeof(char) * (strlen(tmpdir) + 1 + strlen(TEMPLATE) + 1));
	if (mytmpdir == NULL) {
		fprintf(stderr, "%s(): malloc failed()\n", __FUNCTION__);
		exit(1);
	}

	*mytmpdir = '\0';
	(void) strcat(mytmpdir, tmpdir);
	(void) strcat(mytmpdir, PCB_DIR_SEPARATOR_S);
	(void) strcat(mytmpdir, TEMPLATE);
	if (mkdtemp(mytmpdir) == NULL) {
		fprintf(stderr, "%s():  mkdtemp (\"%s\") failed\n", __FUNCTION__, mytmpdir);
		free(mytmpdir);
		return NULL;
	}


	len = strlen(mytmpdir) +			/* the temp directory name */
		1 +													/* the directory sep. */
		strlen(name) +							/* the file name */
		1														/* the \0 termination */
		;

	tmpfile = (char *) malloc(sizeof(char) * len);

	*tmpfile = '\0';
	(void) strcat(tmpfile, mytmpdir);
	(void) strcat(tmpfile, PCB_DIR_SEPARATOR_S);
	(void) strcat(tmpfile, name);

	free(mytmpdir);
#undef TEMPLATE
#else
	/*
	 * tmpnam() uses a static buffer so strdup() the result right away
	 * in case someone decides to create multiple temp names.
	 */
	tmpfile = strdup(tmpnam(NULL));
#ifdef __WIN32__
	{
		/* Guile doesn't like \ separators */
		char *c;
		for (c = tmpfile; *c; c++)
			if (*c == '\\')
				*c = '/';
	}
#endif
#endif

	return tmpfile;
}

/*
 * Unlink a temporary file.  If we have mkdtemp() then our temp file
 * lives in a temporary directory and we need to remove that directory
 * too.
 */
int tempfile_unlink(char *name)
{
#ifdef DEBUG
	/* SDB says:  Want to keep old temp files for examiniation when debugging */
	return 0;
#endif

#ifdef HAVE_MKDTEMP
	int e, rc2 = 0;
	char *dname;

	unlink(name);
	/* it is possible that the file was never created so it is OK if the
	   unlink fails */

	/* now figure out the directory name to remove */
	e = strlen(name) - 1;
	while (e > 0 && name[e] != PCB_DIR_SEPARATOR_C) {
		e--;
	}

	dname = strdup(name);
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
		fprintf(stderr, _("%s():  Unable to determine temp directory name from the temp file\n"), __FUNCTION__);
		fprintf(stderr, "%s():  \"%s\"\n", __FUNCTION__, name);
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
		fprintf(stderr, _("Failed to unlink \"%s\"\n"), name);
		free(name);
		return rc;
	}
	free(name);

#endif

	return 0;
}

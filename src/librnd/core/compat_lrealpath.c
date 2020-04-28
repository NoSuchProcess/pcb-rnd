/* Libiberty realpath.  Like realpath, but more consistent behavior.
   Based on gdb_realpath from GDB.

   Copyright 2003 Free Software Foundation, Inc.

   This file is part of the libiberty library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/*

@deftypefn Replacement {const char*} rnd_lrealpath(const char *@var{name})

Given a pointer to a string containing a pathname, returns a canonical
version of the filename.  Symlinks will be resolved, and ``.'' and ``..''
components will be simplified.  The returned value will be allocated using
@code{malloc}, or @code{NULL} will be returned on a memory allocation error.

@end deftypefn

*/

#include <librnd/core/compat_inc.h>
#include <librnd/config.h>
#include <librnd/core/compat_lrealpath.h>
#include <limits.h>
#include <stdlib.h>
#ifdef RND_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <librnd/core/compat_misc.h>

/* On GNU libc systems the declaration is only visible with _GNU_SOURCE.  */
#if defined(HAVE_CANONICALIZE_FILE_NAME) \
    && defined(NEED_DECLARATION_CANONICALIZE_FILE_NAME)
extern char *canonicalize_file_name(const char *);
#endif

#if defined(RND_HAVE_REALPATH)
#if defined (PATH_MAX)
#define REALPATH_LIMIT PATH_MAX
#else
#if defined (PCB_PATH_MAX)
#define REALPATH_LIMIT PCB_PATH_MAX
#endif
#endif
#else
	/* cygwin has realpath, so it won't get here.  */
#if defined (_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>						/* for GetFullPathName */
#endif
#endif

char *rnd_lrealpath(const char *filename)
{
	/* Method 1: The system has a compile time upper bound on a filename
	   path.  Use that and realpath() to canonicalize the name.  This is
	   the most common case.  Note that, if there isn't a compile time
	   upper bound, you want to avoid realpath() at all costs.  */
#if defined(REALPATH_LIMIT)
	{
		char buf[REALPATH_LIMIT];
		const char *rp = realpath(filename, buf);
		if (rp == NULL)
			rp = filename;
		return pcb_strdup(rp);
	}
	/* REALPATH_LIMIT */

	/* Method 2: The host system (i.e., GNU) has the function
	   canonicalize_file_name() which malloc's a chunk of memory and
	   returns that, use that.  */
#elif defined(HAVE_CANONICALIZE_FILE_NAME)
	{
		char *rp = canonicalize_file_name(filename);
		if (rp == NULL)
			return pcb_strdup(filename);
		else
			return rp;
	}
	/* HAVE_CANONICALIZE_FILE_NAME */

	/* Method 3: Now we're getting desperate!  The system doesn't have a
	   compile time buffer size and no alternative function.  Query the
	   OS, using pathconf(), for the buffer limit.  Care is needed
	   though, some systems do not limit PATH_MAX (return -1 for
	   pathconf()) making it impossible to pass a correctly sized buffer
	   to realpath() (it could always overflow).  On those systems, we
	   skip this.  */
#elif defined (RND_HAVE_REALPATH) && defined (RND_HAVE_UNISTD_H)
	{
#ifdef inline
	/* Suppress compiler warnings; -Dinline means we are compiling in
	   --debug with -ansi -pedantic; we do know that realpath exists on the system,
	   because of the the ifdefs. */
	char *realpath(const char *path, char *resolved_path);
#endif

		/* Find out the max path size.  */
		long path_max = pathconf("/", _PC_PATH_MAX);
		if (path_max > 0) {
			/* PATH_MAX is bounded.  */
			char *buf, *rp, *ret;
			buf = (char *) malloc(path_max);
			if (buf == NULL)
				return NULL;
			rp = realpath(filename, buf);
			ret = pcb_strdup(rp ? rp : filename);
			free(buf);
			return ret;
		}
		return NULL;
	}
	/* RND_HAVE_REALPATH && RND_HAVE_UNISTD_H */

	/* The MS Windows method.  If we don't have realpath, we assume we
	   don't have symlinks and just canonicalize to a Windows absolute
	   path.  GetFullPath converts ../ and ./ in relative paths to
	   absolute paths, filling in current drive if one is not given
	   or using the current directory of a specified drive (eg, "E:foo").
	   It also converts all forward slashes to back slashes.  */
#elif defined (_WIN32)
	{
		char buf[MAX_PATH];
		char *basename;
		DWORD len = GetFullPathName(filename, MAX_PATH, buf, &basename);
		if (len == 0 || len > MAX_PATH - 1)
			return pcb_strdup(filename);
		else {
			/* The file system is case-preserving but case-insensitive,
			   Canonicalize to lowercase, using the codepage associated
			   with the process locale.  */
			CharLowerBuff(buf, len);
			return pcb_strdup(buf);
		}
	}
#else

	/* This system is a lost cause, just duplicate the filename.  */
	return pcb_strdup(filename);
#endif
}

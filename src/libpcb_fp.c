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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

RCSID ("$Id$");

/* This is a helper function for ParseLibrary Tree.   Given a char *path,
 * it finds all newlib footprints in that dir and sticks them into the
 * library menu structure named entry.
 */
int pcb_fp_list(char *subdir, int recurse,  int (*cb)(void *cookie, const char *subdir, const char *name, pcb_fp_type_t type), void *cookie)
{
  char olddir[MAXPATHLEN + 1];    /* The directory we start out in (cwd) */
  DIR *subdirobj;                 /* Interable object holding all subdir entries */
  struct dirent *subdirentry;     /* Individual subdir entry */
  struct stat buffer;             /* Buffer used in stat */
  size_t l;
  size_t len;
  int n_footprints = 0;           /* Running count of footprints found in this subdir */
	char *full_path;

  /* Cache old dir, then cd into subdir because stat is given relative file names. */
  memset (olddir, 0, sizeof olddir);
  if (GetWorkingDirectory (olddir) == NULL)
    {
      Message (_("pcb_fp_list(): Could not determine initial working directory\n"));
      return 0;
    }

  if (chdir (subdir))
    {
      ChdirErrorMessage (subdir);
      return 0;
    }

  /* Determine subdir is abs path */
  if (GetWorkingDirectory (subdir) == NULL)
    {
      Message (_("pcb_fp_list(): Could not determine new working directory\n"));
      if (chdir (olddir))
        ChdirErrorMessage (olddir);
      return 0;
    }

  /* First try opening the directory specified by path */
  if ( (subdirobj = opendir (subdir)) == NULL )
    {
      OpendirErrorMessage (subdir);
      if (chdir (olddir))
        ChdirErrorMessage (olddir);
      return 0;
    }


  /* Now loop over files in this directory looking for files.
   * We ignore certain files which are not footprints.
   */
  while ((subdirentry = readdir (subdirobj)) != NULL)
  {
#ifdef DEBUG
/*    printf("...  Examining file %s ... \n", subdirentry->d_name); */
#endif


    /* Ignore non-footprint files found in this directory
     * We're skipping .png and .html because those
     * may exist in a library tree to provide an html browsable
     * index of the library.
     */
    l = strlen (subdirentry->d_name);
    if (!stat (subdirentry->d_name, &buffer) && S_ISREG (buffer.st_mode)
      && subdirentry->d_name[0] != '.'
      && NSTRCMP (subdirentry->d_name, "CVS") != 0
      && NSTRCMP (subdirentry->d_name, "Makefile") != 0
      && NSTRCMP (subdirentry->d_name, "Makefile.am") != 0
      && NSTRCMP (subdirentry->d_name, "Makefile.in") != 0
      && (l < 4 || NSTRCMP(subdirentry->d_name + (l - 4), ".png") != 0) 
      && (l < 5 || NSTRCMP(subdirentry->d_name + (l - 5), ".html") != 0)
      && (l < 4 || NSTRCMP(subdirentry->d_name + (l - 4), ".pcb") != 0) )
      {

#ifdef DEBUG
/*	printf("...  Found a footprint %s ... \n", subdirentry->d_name); */
#endif

			if (subdirentry->d_type == DT_DIR) {
				cb(cookie, subdir, subdirentry->d_name, PCB_FP_DIR);
				if (recurse) {
					char newdir[MAXPATHLEN + 1];
					sprintf(newdir, "%s/%s", subdir, subdirentry->d_name);
					n_footprints += pcb_fp_list(newdir, recurse, cb, cookie);
				}
				continue;
			}
			if ((subdirentry->d_type == DT_REG) || (subdirentry->d_type == DT_LNK)) {
				n_footprints++;
        if (cb(cookie, subdir, subdirentry->d_name, PCB_FP_FILE))
        	break;
     	}
  	}
  }
  /* Done.  Clean up, cd back into old dir, and return */
  closedir (subdirobj);
  if (chdir (olddir))
    ChdirErrorMessage (olddir);
  return n_footprints;
}

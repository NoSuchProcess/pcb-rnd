/* $Id$ */

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


/* misc functions used by several modules
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include "global.h"

#include "box.h"
#include "crosshair.h"
#include "create.h"
#include "data.h"
#include "draw.h"
#include "file.h"
#include "error.h"
#include "mymem.h"
#include "misc.h"
#include "move.h"
#include "pcb-printf.h"
#include "polygon.h"
#include "remove.h"
#include "rtree.h"
#include "rotate.h"
#include "rubberband.h"
#include "search.h"
#include "set.h"
#include "undo.h"
#include "action.h"

/* ----------------------------------------------------------------------
 * returns pointer to current working directory.  If 'path' is not
 * NULL, then the current working directory is copied to the array
 * pointed to by 'path'
 */
char *
GetWorkingDirectory (char *path)
{
#ifdef HAVE_GETCWD
  return getcwd (path, MAXPATHLEN);
#else
  /* seems that some BSD releases lack of a prototype for getwd() */
  return getwd (path);
#endif

}


char *pcb_author (void)
{
#ifdef HAVE_GETPWUID
  static struct passwd *pwentry;
  static char *fab_author = 0;

  if (!fab_author)
    {
      if (Settings.FabAuthor && Settings.FabAuthor[0])
        fab_author = Settings.FabAuthor;
      else
        {
          int len;
          char *comma, *gecos;

          /* ID the user. */
          pwentry = getpwuid (getuid ());
          gecos = pwentry->pw_gecos;
          comma = strchr (gecos, ',');
          if (comma)
            len = comma - gecos;
          else
            len = strlen (gecos);
          fab_author = (char *)malloc (len + 1);
          if (!fab_author)
            {
              perror ("pcb: out of memory.\n");
              exit (-1);
            }
          memcpy (fab_author, gecos, len);
          fab_author[len] = 0;
        }
    }
  return fab_author;
#else
  return "Unknown";
#endif
}

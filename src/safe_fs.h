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

/* Wrap standard file system calls, giving the user a chance to control
   where pcb-rnd may go on the file system. Where hidlib is NULL, some
   of the % substitutions will not be performed (the ones that depend on
   design (file) name) */

#ifndef PCB_SAFE_FS_H
#define PCB_SAFE_FS_H

#include <stdio.h>
#include "global_typedefs.h"

/* file name templating wrappers around file system calls; later they
   will also execute checks to avoid unsafe access */

FILE *pcb_fopen(pcb_hidlib_t *hidlib, const char *path, const char *mode);
FILE *pcb_fopen_askovr(pcb_hidlib_t *hidlib, const char *path, const char *mode, int *all);
FILE *pcb_popen(pcb_hidlib_t *hidlib, const char *cmd, const char *mode);
int pcb_pclose(FILE *f);
int pcb_system(pcb_hidlib_t *hidlib, const char *cmd);
int pcb_remove(pcb_hidlib_t *hidlib, const char *path);
int pcb_rename(pcb_hidlib_t *hidlib, const char *old_path, const char *new_path);
int pcb_mkdir(pcb_hidlib_t *hidlib, const char *path, int mode);
int pcb_unlink(pcb_hidlib_t *hidlib, const char *path);

/* Return the size of non-large files; on error or for large files
   (size larger than the value long can hold) return -1 */
long pcb_file_size(pcb_hidlib_t *hidlib, const char *path);

/* Return -1 on error or the last modification time (in sec from epoch) */
double pcb_file_mtime(pcb_hidlib_t *hidlib, const char *path);

/* Return non-zero if path is a directory */
int pcb_is_dir(pcb_hidlib_t *hidlib, const char *path);

/* Check if path could be open with mode; if yes, return the substituted/expanded
   file name, if no, return NULL */
char *pcb_fopen_check(pcb_hidlib_t *hidlib, const char *path, const char *mode);

/* Same as pcb_fopen(), but on success load fn_out() with the malloc()'d
   file name as it looked after the substitution */
FILE *pcb_fopen_fn(pcb_hidlib_t *hidlib, const char *path, const char *mode, char **fn_out);

/* Open a file given as a basename fn, under the directory dir, optionally
   doing a recusrive search in the directory tree. If full_path is not NULL,
   and the call succeeds, load it with the full path of the file opened. */
FILE *pcb_fopen_at(pcb_hidlib_t *hidlib, const char *dir, const char *fn, const char *mode, char **full_path, int recursive);


#include "conf.h"

/* Open a file with standard path search and substitutions performed on
   the file name. If fn is not an absolute path, search paths for the
   first directory from which fn is accessible. If the call doesn't fail
   and full_path is not NULL, it is set to point to the final full path
   (or NULL on failure); the caller needs to call free() on it.
   If recursive is set, all subcirectories under each path is also searched for the file.
   */
FILE *pcb_fopen_first(pcb_hidlib_t *hidlib, const pcb_conflist_t *paths, const char *fn, const char *mode, char **full_path, int recursive);

#endif

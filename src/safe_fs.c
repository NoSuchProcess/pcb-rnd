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
  *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  *
  */

/* for popen() */
#define _DEFAULT_SOURCE
#define _BSD_SOURCE

/* permit direct access to the libc calls (turn off config.h masking) */
#define PCB_SAFE_FS

#include "config.h"
#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "safe_fs.h"

/* Evaluates op(arg1,arg2); returns 0 if the operation is permitted */
static int pcb_safe_fs_check(const char *op, const char *arg1, const char *arg2)
{
	return 0;
}

/* Evaluate op(arg1,arg2) and print error and return err_ret if not permitted */
#define CHECK(func, op, arg1, arg2, err_ret) \
do { \
	if (pcb_safe_fs_check(op, arg1, arg2) != 0) { \
		pcb_message(PCB_MSG_ERROR, "File system operation %s(): access denied on %s(%s,%s)\n", func, op, arg1, arg2); \
		return err_ret; \
	} \
} while(0)

FILE *pcb_fopen(const char *path, const char *mode)
{
	CHECK("fopen", "access", path, mode, NULL);
	CHECK("fopen", "fopen", path, mode, NULL);
	return fopen(path, mode);
}

FILE *pcb_popen(const char *cmd, const char *mode)
{
	CHECK("popen", "access", cmd, mode, NULL);
	CHECK("popen", "exec", cmd, NULL, NULL);
	CHECK("popen", "popen", cmd, mode, NULL);
	return popen(cmd, mode);
}

int pcb_system(const char *cmd)
{
	CHECK("access", "access", cmd, "r", -1);
	CHECK("access", "exec", cmd, NULL, -1);
	CHECK("access", "system", cmd, NULL, -1);
	return system(cmd);
}

int pcb_remove(const char *path)
{
	CHECK("remove", "access", path, "w", -1);
	CHECK("remove", "remove", path, NULL, -1);
	return remove(path);
}

int pcb_rename(const char *old_path, const char *new_path)
{
	CHECK("rename", "access", old_path, "w", -1);
	CHECK("rename", "access", new_path, "w", -1);
	CHECK("rename", "rename", old_path, new_path, -1);
	return pcb_rename(old_path, new_path);
}


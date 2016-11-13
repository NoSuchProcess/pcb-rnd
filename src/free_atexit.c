/*                            COPYRIGHT
 *
 *  Copyright (C) 2010 PCB Contributors (see ChangeLog for details)
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
 *  harry eaton, 6697 Buttonhole Ct, Columbia, MD 21044 USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

#include <stdlib.h>
#include <string.h>
#include "config.h"

/* we need one ID per context - short int with 64k IDs should be enough */
typedef unsigned int leaky_idx_t;


/* This structure should be as big as void *, which should be the natural
bit-width of the architecture. We allocate extra admin space to be as big
as this union, to preserve alignment of pointers returned by malloc().
NOTE: in the special corner case when leaky_idx_t is wider than void * but
not multiple of it, the alignment will be messed up, potentially causing slower
memory access. */
typedef union {
	leaky_idx_t idx;
	void *ptr;
} leaky_admin_t;

static void **free_list = NULL;
static leaky_idx_t free_size = 0;


void *pcb_leaky_malloc(size_t size)
{
	char *new_memory = malloc(size + sizeof(leaky_admin_t));

	free_list = (void **) realloc(free_list, (free_size + 1) * sizeof(void *));
	free_list[free_size] = new_memory;
	*(leaky_idx_t *) new_memory = free_size;

	free_size++;
	return new_memory + sizeof(leaky_admin_t);
}

void *pcb_leaky_calloc(size_t nmemb, size_t size)
{
	size_t size_ = size * nmemb;
	void *new_memory = pcb_leaky_malloc(size_);

	memset(new_memory, 0, size_);
	return new_memory;
}

void *pcb_leaky_realloc(void *old_memory_, size_t size)
{
	char *new_memory;
	char *old_memory = old_memory_;
	leaky_idx_t i;

	if (old_memory == NULL)
		return pcb_leaky_malloc(size);

	old_memory -= sizeof(leaky_admin_t);

	i = *(leaky_idx_t *) old_memory;

	new_memory = realloc(old_memory, size + sizeof(leaky_admin_t));
	free_list[i] = new_memory;

	return new_memory + sizeof(leaky_admin_t);
}

char *pcb_leaky_strdup(const char *src)
{
	int len = strlen(src)+1;
	char *res = pcb_leaky_malloc(len);
	memcpy(res, src, len);
	return res;
}

void pcb_leaky_uninit(void)
{
	int i;

	for (i = 0; i < free_size; i++)
		free(free_list[i]);

	free(free_list);
	free_size = 0;
}

void pcb_leaky_init(void)
{
	atexit(pcb_leaky_uninit);
}

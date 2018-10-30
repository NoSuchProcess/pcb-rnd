/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2005 DJ Delorie
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
 *
 *  Old contact info:
 *  DJ Delorie, 334 North Road, Deerfield NH 03037-1110, USA
 *  dj@delorie.com
 *
 */

#ifndef PCB_STRFLAGS_H
#define PCB_STRFLAGS_H

#include "flag.h"

typedef struct {

	/* This is the bit that we're setting.  */
	pcb_flag_values_t mask;
	const char *mask_name;

	/* The name used in the output file.  */
	const char *name;
	int nlen;

	/* The entry won't be output unless the object type is one of these.  */
	int object_types;

	char *help;

	/* in compatibility mode also accept these object types to have the flag */
	int compat_types;

	/* when non-zero, omit from all-flag listings */
	int omit_list;
} pcb_flag_bits_t;

/* All flags natively known by the core */
extern pcb_flag_bits_t pcb_object_flagbits[];
extern const int pcb_object_flagbits_len;

/* The purpose of this interface is to make the file format able to
   handle more than 32 flags, and to hide the internal details of
   flags from the file format. In compat mode, compat_types are
   also considered (so an old PCB PAD (padstack now) can have
   a square flag). */

/* When passed a string, parse it and return an appropriate set of
   flags.  Errors cause error() to be called with a suitable message;
   if error is NULL, errors are ignored.  */
pcb_flag_t pcb_strflg_s2f(const char *flagstring, int (*error) (const char *msg), unsigned char *intconn, int compat);

/* Given a set of flags for a given object type, return a string which
   can be output to a file.  The returned pointer must not be
   freed.  */
char *pcb_strflg_f2s(pcb_flag_t flags, int object_type, unsigned char *intconn, int compat);


/* Call cb for each flag bit for a given object type */
void pcb_strflg_map(unsigned long fbits, int object_type, void *ctx, void (*cb)(void *ctx, unsigned long flg, const pcb_flag_bits_t *fb));

/* Return flag bit info for a single bit for an object type, or NULL is not available - slow linear search */
const pcb_flag_bits_t *pcb_strflg_1bit(unsigned long bit, int object_type);

/* Return flag bit info by flag name for an object type, or NULL is not available - slow linear search */
const pcb_flag_bits_t *pcb_strflg_name(const char *name, int object_type);

/* same as above, for pcb level flags */
char *pcb_strflg_board_f2s(pcb_flag_t flags);
pcb_flag_t pcb_strflg_board_s2f(const char *flagstring, int (*error) (const char *msg));

void pcb_strflg_uninit_buf(void);
void pcb_strflg_uninit_layerlist(void);

/* low level */
pcb_flag_t pcb_strflg_common_s2f(const char *flagstring, int (*error) (const char *msg), pcb_flag_bits_t * flagbits, int n_flagbits, unsigned char *intconn, int compat);
char *pcb_strflg_common_f2s(pcb_flag_t flags, int object_type, pcb_flag_bits_t * flagbits, int n_flagbits, unsigned char *intconn, int compat);

#endif

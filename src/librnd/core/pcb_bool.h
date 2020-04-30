/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016, 2017 Tibor 'Igor2' Palinkas
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

#ifndef RND_BOOL_H
#define RND_BOOL_H
/* Because stdbool is not c89 */
typedef int rnd_bool;
typedef enum rnd_bool_e {
	rnd_false = 0,
	rnd_true = 1
} rnd_bool_t;


	/* for arguments optionally changing the value of a bool */
typedef enum rnd_bool_op_e {
	RND_BOOL_CLEAR = 0,
	RND_BOOL_SET = 1,
	RND_BOOL_TOGGLE = -1,
	RND_BOOL_PRESERVE = -2,
	RND_BOOL_INVALID = -8
} rnd_bool_op_t;

/* changes the value of rnd_bool dst as requested by rnd_bool_op_t op
   WARNING: evaluates dst multiple times */
#define rnd_bool_op(dst, op) \
do { \
	switch(op) { \
		case RND_BOOL_CLEAR:   (dst) = 0; break; \
		case RND_BOOL_SET:     (dst) = 1; break; \
		case RND_BOOL_TOGGLE:  (dst) = !(dst); break; \
		case RND_BOOL_INVALID: \
		case RND_BOOL_PRESERVE: break; \
	} \
} while(0)

rnd_bool_op_t rnd_str2boolop(const char *s);

#endif

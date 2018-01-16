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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#ifndef PCB_BOOL_H
#define PCB_BOOL_H
/* Because stdbool is not c89 */
typedef int pcb_bool;
typedef enum pcb_bool_e {
	pcb_false = 0,
	pcb_true = 1
} pcb_bool_t;


	/* for arguments optionally changing the value of a bool */
typedef enum pcb_bool_op_e {
	PCB_BOOL_CLEAR = 0,
	PCB_BOOL_SET = 1,
	PCB_BOOL_TOGGLE = -1,
	PCB_BOOL_PRESERVE = -2,
	PCB_BOOL_INVALID = -8
} pcb_bool_op_t;

/* changes the value of pcb_bool dst as requested by pcb_bool_op_t op
   WARNING: evaluates dst multiple times */
#define pcb_bool_op(dst, op) \
do { \
	switch(op) { \
		case PCB_BOOL_CLEAR:   (dst) = 0; break; \
		case PCB_BOOL_SET:     (dst) = 1; break; \
		case PCB_BOOL_TOGGLE:  (dst) = !(dst); break; \
		case PCB_BOOL_INVALID: \
		case PCB_BOOL_PRESERVE: break; \
	} \
} while(0)

pcb_bool_op_t pcb_str2boolop(const char *s);

#endif

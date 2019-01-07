/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include <ctype.h>

typedef enum {
	PCB_MKS_RED       = 1,
	PCB_MKS_GREEN     = 2,
	PCB_MKS_BLUE      = 4,
	PCB_MKS_BOLD      = 8,
	PCB_MKS_ITALIC    = 16
} pcb_markup_state_t;


PCB_INLINE const char *pcb_markup_next(pcb_markup_state_t *st, const char **at_, long *len)
{
	const char *start, *at = *at_;

	next:;
	if (*at == '\0')
		return NULL;

	start = at;
	if (*at == '<') {
		int neg = 0;
		pcb_markup_state_t bit = 0;
		at++;
		if (*at == '/') {
			neg = 1;
			at++;
		}
		switch(*at) {
			case 'R': bit = PCB_MKS_RED; break;
			case 'G': bit = PCB_MKS_GREEN; break;
			case 'B': bit = PCB_MKS_BLUE; break;
			case 'b': bit = PCB_MKS_BOLD; break;
			case 'i': bit = PCB_MKS_ITALIC; break;
		}
		at++;
		if ((bit != 0) && (*at == '>')) {
			if (neg)
				*st &= ~bit;
			else
				*st |= bit;
			at++; /* move after the '>' */
			goto next;
		}
		/* only <.> and </.> triggers, where . is one of the known tags; leave
		   anything else intact so we don't need escaping (e.g. &lt;) */
	}

	/* segment */
	while((*at != '<') && (*at != '\0')) at++;
	*at_ = at;
	*len = at - start;
	return start;
}

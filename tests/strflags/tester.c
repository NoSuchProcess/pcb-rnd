/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  DJ Delorie, 334 North Road, Deerfield NH 03037-1110, USA
 *  dj@delorie.com
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include "compat_misc.h"

#define FLAG_TEST
#include "strflags.c"
#include "flags.c"
#include "compat_misc.c"

static void dump_flag(pcb_flag_t * f)
{
	int l;
	printf("F:%08x T:[", f->f);
	for (l = 0; l < (PCB_MAX_LAYER + 7) / 8; l++)
		printf(" %02x", f->t[l]);
	printf("]");
}


int pcb_mem_any_set(unsigned char *ptr, int bytes)
{
	while (bytes--)
		if (*ptr++)
			return 1;
	return 0;
}

int main()
{
	time_t now;
	int i;
	int errors = 0, count = 0;

	time(&now);
	srandom((unsigned int) now + getpid());

	grow_layer_list(0);
	for (i = 0; i < 16; i++) {
		int j;
		char *p;
		if (i != 1 && i != 4 && i != 5 && i != 9)
			set_layer_list(i, 1);
		else
			set_layer_list(i, 0);
		p = print_layer_list();
		printf("%2d : %20s =", i, p);
		parse_layer_list(p + 1, 0);
		for (j = 0; j < num_layers; j++)
			printf(" %d", layers[j]);
		printf("\n");
	}

	while (count < 1000000) {
		FlagHolder fh;
		char *str;
		pcb_flag_t new_flags;
		int i;
		int otype;

		otype = PCB_TYPEMASK_ALL;
		fh.Flags = empty_flags;
		for (i = 0; i < PCB_ENTRIES(object_flagbits); i++) {
			if (PCB_FLAG_TEST(object_flagbits[i].mask, &fh))
				continue;
			if ((otype & object_flagbits[i].object_types) == 0)
				continue;
			if ((random() & 4) == 0)
				continue;

			otype &= object_flagbits[i].object_types;
			PCB_FLAG_SET(object_flagbits[i].mask, &fh);
		}

		if (otype & PCB_TYPEMASK_PIN)
			for (i = 0; i < PCB_MAX_LAYER; i++)
				if (random() & 4)
					PCB_FLAG_THERM_ASSIGN(i, 3, &fh);

		str = pcb_strflg_f2s(fh.Flags, otype);
		new_flags = pcb_strflg_s2f(str, 0);

		count++;
		if (PCB_FLAG_EQ(fh.Flags, new_flags))
			continue;
		dump_flag(&fh.Flags);
		printf(" ");
		dump_flag(&new_flags);
		printf("\n");
		if (++errors == 5)
			goto bad;
	}

	while (count < 1000000) {
		FlagHolder fh;
		char *str;
		pcb_flag_t new_flags;
		int i;
		int otype;

		otype = PCB_TYPEMASK_ALL;
		fh.Flags = empty_flags;
		for (i = 0; i < PCB_ENTRIES(pcb_flagbits); i++) {
			if (PCB_FLAG_TEST(pcb_flagbits[i].mask, &fh))
				continue;
			if ((random() & 4) == 0)
				continue;

			otype &= pcb_flagbits[i].object_types;
			PCB_FLAG_SET(pcb_flagbits[i].mask, &fh);
		}

		str = pcb_strflg_board_f2s(fh.Flags);
		new_flags = pcb_strflg_board_s2f(str, 0);

		count++;
		if (PCB_FLAG_EQ(fh.Flags, new_flags))
			continue;

		dump_flag(&fh.Flags);
		printf(" ");
		dump_flag(&new_flags);
		printf("\n");

		if (++errors == 5)
			goto bad;
	}
	printf("%d out of %d failed\n", errors, count);
	return errors;

	bad:;
	printf("%d out of %d FAILED\n", errors, count);
	return errors;
}


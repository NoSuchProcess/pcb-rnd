/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  BXL IO plugin - file format read, parser
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2020)
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

typedef struct hnode_s hnode_t;

struct hnode_s {
	int level, symbol, weight;
	hnode_t *parent, *left, *right;
};

typedef struct htree_s {
	hnode_t *root;
} htree_t;

typedef struct hdecode_s {
	int chr, bitpos;
	htree_t *tree;
	hnode_t *node;
	int out[10];
	int out_len;
	int hdr_pos;
	int hdr[4];
	unsigned long int plain_len, opos;
} hdecode_t;

void pcb_bxl_decode_init(hdecode_t *ctx);
void pcb_bxl_decode_uninit(hdecode_t *ctx);


/* Feed the state machine with an input character. Returns the number of
   output characters available in ctx->out[]; these must be saved before
   the next call. */
int pcb_bxl_decode_char(hdecode_t *ctx, int inchr);

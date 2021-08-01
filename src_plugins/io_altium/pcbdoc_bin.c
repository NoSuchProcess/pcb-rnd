/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  altium plugin - pcbdoc low level read: parse binary into a tree in mem
 *  pcb-rnd Copyright (C) 2021 Tibor 'Igor2' Palinkas
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


#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libucdf/ucdf.h>
#include "pcbdoc_ascii.h"
#include "pcbdoc_bin.h"

TODO("remove:")
#include <stdio.h>

#define MAX_REC_LEN (1UL << 20)

static const union {
	short int sh;
	char c[2];
} endianness = {0x0102};

#define is_host_litend (endianness.c[0] == 2)

/* convert a file integer (len bytes wide) into a native integer, assuming source is little-endian */
static long load_int(unsigned const char *src, int len)
{
	int n, sh;
	long tmp, res = 0;

	for(sh = n = 0; n < len; n++,sh += 8,src++) {
		tmp = *src;
		tmp = (tmp << sh);
		res |= tmp;
	}

	/* "sign extend" */
	if (res >= (1UL << (len*8-1)))
		res = -((1UL << (len*8)) - res);

	return res;
}

/* convert a double, assuming source is little-endian */
static double load_dbl(unsigned const char *src)
{
	double d;

	if (is_host_litend) { /* le to le -> copy */
		memcpy(&d, src, 8);
		return d;
	}
	else { /* le to be -> swap */
		unsigned char tmp[8];
		tmp[0] = src[7]; tmp[1] = src[6]; tmp[2] = src[5]; tmp[3] = src[4];
		tmp[4] = src[3]; tmp[5] = src[2]; tmp[6] = src[1]; tmp[7] = src[0];
		memcpy(&d, tmp, 8);
		return d;
	}
}

static double bmil(unsigned const char *src)
{
	long raw = load_int(src, 4);
	return (double)raw / 10000.0;
}

#define buf_grow(tmp, len) \
do { \
	if (tmp->alloced < len) { \
		free(tmp->data); \
		tmp->data = malloc(len); \
		if (tmp->data == NULL) { \
			tmp->alloced = 0; \
			return -1; \
		} \
		tmp->alloced = len; \
	} \
} while(0)

static long read_len(ucdf_file_t *fp, int field_width)
{
	unsigned char lens[4];
	long len, res;

	assert(field_width <= 4);

	if ((res = ucdf_fread(fp, (void *)lens, field_width)) != field_width) {
		if (res <= 0)
			return res;
		return -1; /* short read means broken field */
	}

	len = load_int(lens, field_width);
	if ((len < 0) || (len >= MAX_REC_LEN))
		return -1;

	return len;
}

static long read_rec_l4b(ucdf_file_t *fp, altium_buf_t *tmp)
{
	long len, res;

	len = read_len(fp, 4);
	if (len < 0)
		return len;

	buf_grow(tmp, len+1);
	res = ucdf_fread(fp, (void *)tmp->data, len);
	if (res == 0)
		return 0;

	if (res != len)
		return -1;

	tmp->data[len] = '\0'; /* safety: always make it a valid string just in case it is text */
	tmp->used = len+1;

	return len;
}

static long read_rec_tlb(ucdf_file_t *fp, altium_buf_t *tmp, int *type)
{
	char typec;
	long res;

	if ((res = ucdf_fread(fp, (void *)&typec, 1)) != 1)
		return res;

	*type = typec;

	return read_rec_l4b(fp, tmp);
}

static long read_rec_ilb(ucdf_file_t *fp, altium_buf_t *tmp, int *id)
{
	unsigned char ids[2];
	long res;

	if ((res = ucdf_fread(fp, (void *)ids, 2)) != 2)
		return res;

	*id = load_int(ids, 2);

	return read_rec_l4b(fp, tmp);
}

static int pcbdoc_bin_parse_ascii(rnd_hidlib_t *hidlib, altium_tree_t *tree, const char *record, altium_buf_t *tmp)
{
	TODO("take over tmps buff and make it a block");
	return 0;
}

int pcbdoc_bin_parse_board6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	for(;;) {
		long len = read_rec_l4b(fp, tmp);
		if (len <= 0)
			return len;
		if (pcbdoc_bin_parse_ascii(hidlib, tree, "Board", tmp) != 0)
			return -1;
	}
	return 0;
}

int pcbdoc_bin_parse_polygons6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	for(;;) {
		long len = read_rec_l4b(fp, tmp);
		if (len <= 0)
			return len;
		if (pcbdoc_bin_parse_ascii(hidlib, tree, "Polygon", tmp) != 0)
			return -1;
	}
	return 0;
}

int pcbdoc_bin_parse_classes6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	for(;;) {
		long len = read_rec_l4b(fp, tmp);
		if (len <= 0)
			return len;
		if (pcbdoc_bin_parse_ascii(hidlib, tree, "Class", tmp) != 0)
			return -1;
	}
	return 0;
}

int pcbdoc_bin_parse_nets6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	for(;;) {
		long len = read_rec_l4b(fp, tmp);
		if (len <= 0)
			return len;
		if (pcbdoc_bin_parse_ascii(hidlib, tree, "Net", tmp) != 0)
			return -1;
	}
	return 0;
}

int pcbdoc_bin_parse_components6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	for(;;) {
		long len = read_rec_l4b(fp, tmp);
		if (len <= 0)
			return len;
		if (pcbdoc_bin_parse_ascii(hidlib, tree, "Component", tmp) != 0)
			return -1;
	}
	return 0;
}

int pcbdoc_bin_parse_rules6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	for(;;) {
		int id;
		long len = read_rec_ilb(fp, tmp, &id);
		if (len <= 0)
			return len;
		TODO("Do we need the id?");
		printf("id=%d\n", id);
		if (pcbdoc_bin_parse_ascii(hidlib, tree, "Rule", tmp) != 0)
			return -1;
	}
	return 0;
}


int pcbdoc_bin_parse_tracks6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	for(;;) {
		unsigned char *d;
		int rectype;
		long len;
		
		
		len = read_rec_tlb(fp, tmp, &rectype);
		if (len <= 0)
			return len;
		if (rectype != 4) {
			rnd_message(RND_MSG_ERROR, "Tracks6 record with wrong type; expected 4, got %d\n", rectype);
			continue;
		}
		if (len < 45) {
			rnd_message(RND_MSG_ERROR, "Tracks6 record too short; expected 45, got %ld\n", len);
			continue;
		}

		d = tmp->data;

		printf("line: layer=%d ko=%d net=%ld poly=%ld comp=%ld width=%.2f uu=%d%d\n", d[0], d[1], load_int(d+3, 2), load_int(d+5, 2), load_int(d+7, 2), bmil(d+29), d[36], d[40]);
		printf("  x1=%.2f y1=%.2f x2=%.2f y2=%.2f\n", bmil(d+13), bmil(d+17), bmil(d+21), bmil(d+25));
	}
	return 0;
}


int pcbdoc_bin_parse_arcs6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	for(;;) {
		unsigned char *d;
		int rectype;
		long len;
		
		
		len = read_rec_tlb(fp, tmp, &rectype);
		if (len <= 0)
			return len;
		if (rectype != 1) {
			rnd_message(RND_MSG_ERROR, "Arcs6 record with wrong type; expected 1, got %d\n", rectype);
			continue;
		}
		if (len < 56) {
			rnd_message(RND_MSG_ERROR, "Arcs6 record too short; expected 56, got %ld\n", len);
			continue;
		}

		d = tmp->data;

		printf("arc: layer=%d ko=%d net=%ld poly=%ld comp=%ld width=%.2f uu=%d%d\n", d[0], d[1], load_int(d+3, 2), load_int(d+5, 2), load_int(d+7, 2), bmil(d+41), d[48], d[55]);
		printf("  cx=%.2f cy=%.2f r=%.2f sa=%.3f ea=%.3f\n", bmil(d+13), bmil(d+17), bmil(d+21), load_dbl(d+25), load_dbl(d+33));
	}
	return 0;
}


int pcbdoc_bin_parse_texts6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	for(;;) {
		unsigned char *d;
		int rectype;
		long len;

		len = read_rec_tlb(fp, tmp, &rectype);
		if (len <= 0)
			return len;
		if (rectype != 5) {
			rnd_message(RND_MSG_ERROR, "Texts6 record with wrong type; expected 5, got %d\n", rectype);
			continue;
		}
		if (len < 230) {
			rnd_message(RND_MSG_ERROR, "Texts6 record too short; expected 230, got %ld\n", len);
			continue;
		}

		d = tmp->data;

		printf("text: layer=%d comp=%ld height=%.2f comment=%d designator=%d\n", d[0], load_int(d+7, 2), bmil(d+21), d[40], d[41]);
		printf("  x=%.2f y=%.2f h=%.2f rot=%.3f mirr=%d\n", bmil(d+13), bmil(d+17), bmil(d+21), load_dbl(d+27), d[35]);

		len = read_rec_l4b(fp, tmp);
		printf("  string='%s'\n", tmp->data+1);

	}
	return 0;
}


int pcbdoc_bin_parse_fills6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	for(;;) {
		unsigned char *d;
		int rectype;
		long len;

		len = read_rec_tlb(fp, tmp, &rectype);
		if (len <= 0)
			return len;
		if (rectype != 6) {
			rnd_message(RND_MSG_ERROR, "Fills6 record with wrong type; expected 6, got %d\n", rectype);
			continue;
		}
		if (len < 46) {
			rnd_message(RND_MSG_ERROR, "Fills6 record too short; expected 46, got %ld\n", len);
			continue;
		}

		d = tmp->data;

		printf("fill: layer=%d ko=%d net=%ld comp=%ld u=%d\n", d[0], d[1], load_int(d+3, 2), load_int(d+7, 2), d[45]);
		printf("  x1=%.2f y1=%.2f x2=%.2f y2=%.2f rot=%.3f dir2=%.3f\n", bmil(d+13), bmil(d+17), bmil(d+21), bmil(d+25), load_dbl(d+29), load_dbl(d+38));

	}
	return 0;
}


int pcbdoc_bin_parse_vias6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	for(;;) {
		unsigned char *d;
		int rectype;
		long len;

		len = read_rec_tlb(fp, tmp, &rectype);
		if (len <= 0)
			return len;
		if (rectype != 3) {
			rnd_message(RND_MSG_ERROR, "vias6 record with wrong type; expected 3, got %d\n", rectype);
			continue;
		}
		if (len < 209) {
			rnd_message(RND_MSG_ERROR, "vias6 record too short; expected 209, got %ld\n", len);
			continue;
		}

		d = tmp->data;

		printf("via: layer=%d..%d net=%ld (comp=%ld) u=%d C*=%d\n", d[0], d[1], load_int(d+3, 2), load_int(d+7, 2), d[70], d[40]);
		printf("  x=%.2f y=%.2f copperD=%.2f holD=%.2f\n", bmil(d+13), bmil(d+17), bmil(d+21), bmil(d+25));
	}
	return 0;
}

/* parse the fields of a pad (the 120 long block of pads6) */
static int pcbdoc_bin_parse_pads6_fields(rnd_hidlib_t *hidlib, altium_tree_t *tree, altium_buf_t *tmp, const char *name)
{
	unsigned char *d = tmp->data;

	/* if layer[0] is 74 (multilayer), use all shapes, else use only top on the specified layer */

	printf("pad: layer=%d..%d net=%ld (comp=%ld) '%s'\n", d[0], d[1], load_int(d+3, 2), load_int(d+7, 2), name);
	printf("  x=%.2f y=%.2f hole=%.2f plated=%d mode=%d rot=%.3f\n", bmil(d+13), bmil(d+17), bmil(d+45), d[60], d[62], load_dbl(d+52));
	printf("  top: %d sx=%.2f sy=%.2f; mid: %d sx=%.2f sy=%.2f; bot: %d sx=%.2f mid-sy=%.2f\n", d[49], bmil(d+21), bmil(d+25), d[50], bmil(d+29), bmil(d+33), d[51], bmil(d+37), bmil(d+41));
	return 0;
}

int pcbdoc_bin_parse_pads6(rnd_hidlib_t *hidlib, altium_tree_t *tree, ucdf_file_t *fp, altium_buf_t *tmp)
{
	for(;;) {
		char rtype;
		long res;

		if ((res = ucdf_fread(fp, (void *)&rtype, 1)) != 1)
			return res;

		if (rtype == 2) {
			long len;
			char name[260];
			int n;

			len = read_rec_l4b(fp, tmp);
			if (len <= 0)
				return len;

			/* 0: tmp is name */
			if (len >= 256) {
				rnd_message(RND_MSG_ERROR, "pads6 name too long: %ld (truncating)\n", len);
				len = 256;
			}
			tmp->data[len] = '\0';
			strcpy(name, (char *)tmp->data+1);

			for(n = 0; ; n++) {
				len = read_rec_l4b(fp, tmp);
/*				printf("len[%d]=%ld\n", n, len);*/
				if (len == 0)
					break;
				if (len < 0)
					return len;
/*				if ((n == 0)||(n == 2)) { printf("#%d: %d (len=%ld)\n", n, tmp->data[0], len); }*/
/*				if (n == 1) { printf("#%d: %d %d %d %d %d\n", n, tmp->data[0], tmp->data[1], tmp->data[2], tmp->data[3], tmp->data[4]); }*/
				if (n == 3) {
					if (len < 120)
						rnd_message(RND_MSG_ERROR, "pads6 record block3 with wrong lengt; expected 120, got %ld\n", len);
					else if (pcbdoc_bin_parse_pads6_fields(hidlib, tree, tmp, name) != 0)
						return -1;
				}
			}
/*			printf("tell=%ld %x\n", ftell(fp), ftell(fp));*/
		}
		else { /* not a pad */
			long len;

			fprintf(stderr, "non-pad object in padstack!\n");
			len = read_rec_l4b(fp, tmp);
			if (len <= 0)
				return len;

			TODO("read and handle snippet of slen in tmp");
			switch(rtype) {
				case 1: /* arc */
				case 4: /* track */
				case 6: /* fill */
					break;
			}
		}

	}
	return 0;
}

int pcbdoc_bin_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *file_name, FILE *f)
{
	return ucdf_test_parse(file_name);
}

